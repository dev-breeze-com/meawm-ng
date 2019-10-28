/* Window.cc

Copyright © 2003 David Reveman.

This file is part of Meawm_NG.

Meawm_NG is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Meawm_NG is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Meawm_NG; see the file COPYING. If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA. */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    LIMITS_H
#  include <limits.h>
#endif // LIMITS_H

}

#include "Window.hh"

WaWindow::WaWindow(Window win_id, WaScreen *scrn) :
    AWindowObject(NULL, win_id, WindowType, NULL, "client") {
    XWindowAttributes init_attrib;
    XSetWindowAttributes attrib_set;
    
    ws = scrn;
    display = ws->display;
    screen_number = ws->screen_number;
    meawm_ng = ws->meawm_ng;
    net = meawm_ng->net;
    wm_strut = NULL;
    move_resize = false;
    name = WA_STRDUP("");
    wclass = WA_STRDUP("");
    wclassname = WA_STRDUP("");
    host = WA_STRDUP("");
    pid = WA_STRDUP("");
    realnamelen = 0;
    master = NULL;
    init_done = remap = outline_state = false;
    window_group = (Window) 0;
    transient_for = (Window) 0;
    wm_icon_image = wm_icon_svg = NULL;
    protocol_mask = 0;
    input_field = true;
    urgent = false;
    functions = FunctionAllMask;
    pending_unmaps = 0;

    attrib_set.event_mask = PropertyChangeMask | StructureNotifyMask |
        FocusChangeMask | EnterWindowMask | LeaveWindowMask;
    attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask;
    
    init_attrib.border_width = 0;
    init_attrib.colormap = ws->colormap;
    init_attrib.win_gravity = NorthWestGravity;
    init_attrib.x = 0;
    init_attrib.y = 0;
    init_attrib.width = 1;
    init_attrib.height = 1;

    XChangeWindowAttributes(display, id, CWEventMask | CWDontPropagate,
                            &attrib_set);
    XGetWindowAttributes(display, id, &init_attrib);
    XSetWindowBorderWidth(display, id, 0);
    
    old_bw = init_attrib.border_width;
    attrib.colormap = init_attrib.colormap;
    size.win_gravity = init_attrib.win_gravity;
    attrib.x = init_attrib.x;
    attrib.y = init_attrib.y;
    attrib.width  = init_attrib.width;
    attrib.height = init_attrib.height;
    
    want_focus = mapped = dontsend = deleted = hidden = false;

    desktop_mask = (1L << ws->current_desktop->number);
    
    top_spacing = bottom_spacing = left_spacing = right_spacing = 0;
    has_focus = mergedback = false;
    wstate = StateFocusableMask | StateTasklistMask |
        StateDecorTitleMask | StateDecorBorderMask |
        StateDecorHandlesMask | StateDecorAllMask | StateUserTimeMask;
    old_wstate = wstate;
    decormask = DecorAllMask;
    transient_for = (Window) 0;
    mergemode = NullMergeType;
    frame = NULL;

    net->getWMProtocols(this);
    net->getClassHint(this);
    if (! net->getNetName(this)) net->getXaName(this);
    net->getWMClientMachineHint(this);
    net->getWmPid(this);

    char namechar_tmp = name[realnamelen];
    name[realnamelen] = '\0';

    char window_id_str[32];
    snprintf(window_id_str, 32, "0x%lx", id);
    rid = WA_STRDUP(window_id_str);
    
    WaStringMap *sm = new WaStringMap();
    sm->add(WindowIDName, name);
    sm->add(WindowIDClassName, wclassname);
    sm->add(WindowIDClass, wclass);
    sm->add(WindowIDPID, pid);
    sm->add(WindowIDHost, host);
    sm->add(WindowIDWinID, rid);
    resetActionList(sm);
    
    frame = new WaFrameWindow(this, sm->ref());

#ifdef    SHAPE
    shaped = false;
    shapeinfo = new ShapeInfo(id);
    frame->addShapeInfo(shapeinfo);
#endif // SHAPE

    frame->commonStyleUpdate();

    name[realnamelen] = namechar_tmp;

    meawm_ng->window_table.insert(make_pair(id, this));
    
    if (deleted)
        goto window_create_end;

    net->getWMHints(this);
    net->getWMNormalHints(this);
    initPosition();
    net->getTransientForHint(this);
    net->getWmState(this);
    net->getMWMHints(this);
    net->getWmType(this);
    net->getWmIconImage(this);
    net->getWmIconSvg(this);
    net->getVirtualPos(this);
    net->getWmStrut(this);
    net->getDesktop(this);
    net->setDesktop(this);
    net->setDesktopMask(this);

    calcSpacing();
    gravitate(ApplyGravity);
   
    reparentWin();
    updateGrabs();

    if (deleted) 
        goto window_create_end;
    
    updateAllAttributes();

    if (wstate & StateShadedMask) shade();
    
    ws->wawindow_list.push_back(this);
    ws->wawindow_list_map_order.push_back(this);
    if (! (wstate & (StateAlwaysOnTopMask | StateAlwaysAtBottomMask)))
        frame->setStacking(NormalStackingType);
    
    ws->raiseWindow(frame->id);
    net->setAllowedActions(this);
    net->sendNotify(this, CreateWindowNotify);
    net->setWmState(this);
    
    window_create_end:
    init_done = true;
    drawDecor();
    windowStateCheck();
}

WaWindow::~WaWindow(void) {
    explode();
    if (master) master->unmerge(this);
    
    XEvent ev;
    meawm_ng->window_table.erase(id);

    if (! meawm_ng->running)
        remap = true;

    if (wm_icon_image) wm_icon_image->unref();
    if (wm_icon_svg) wm_icon_svg->unref();
    
    net->sendNotify(this, DestroyWindowNotify);

    withdrawTransient();

    gravitate(RemoveGravity);
    if (wstate & StateShadedMask) attrib.height = restore_shade;
    if (attrib.x >= (int) ws->width)
        attrib.x = attrib.x % ws->width;
    if (attrib.y >= (int) ws->height)
        attrib.y = attrib.y % ws->height;
    
    if (attrib.x + attrib.width <= 0)
        attrib.x = ws->width + (attrib.x % ws->width);
    if (attrib.y + attrib.height <= 0)
        attrib.y = ws->height + (attrib.y % ws->height);

    if ((! deleted) && validate_window_mapped(id)) {
        XChangeSaveSet(display, id, SetModeDelete);
        XSelectInput(display, id, NoEventMask);
        XSelectInput(display, frame->id, NoEventMask);
        XUnmapWindow(display, frame->id);
        XUnmapWindow(display, id);
        XSetWindowBorderWidth(display, id, old_bw);
        if (XCheckTypedWindowEvent(display, id, ReparentNotify, &ev))
            remap = true;
        else
            XReparentWindow(display, id, ws->id, attrib.x, attrib.y);
        
        if (remap) XMapWindow(display, id);

        net->removeAllowedActions(this);
        net->removeVisibleName(this);
    }

    ws->smartNameRemove(this);
    delete [] name;
    delete [] wclass;
    delete [] wclassname;
    delete [] host;
    delete [] pid;
    delete [] rid;

    ws->wawindow_list.remove(this);
    ws->wawindow_list_map_order.remove(this);
    if (wm_strut) {
        ws->strut_list.remove(wm_strut);
        delete wm_strut;
        if (! ws->shutdown) ws->updateWorkarea();
    }

#ifdef    SHAPE
    frame->removeShapeInfo(shapeinfo);
    delete shapeinfo;
#endif // SHAPE
    
    delete frame;

    meawm_ng->removeFromFocusHistory(id);

    if (! ws->shutdown) {
        if (has_focus) meawm_ng->focusRevertFrom(ws, id);
        ws->net->setClientList(ws);
        ws->net->setClientListStacking(ws);
    }
}

void WaWindow::withdrawTransient(void) {
    if (transient_for) {
        if (transient_for == ws->id) {
            list<WaWindow *>::iterator it = ws->wawindow_list.begin();
            for (; it != ws->wawindow_list.end(); ++it)
                if ((*it) != this) (*it)->transients.remove(id);
        }
        else {
            WaWindow *transfor = (WaWindow *)
                meawm_ng->findWin(transient_for, WindowType);
            if (transfor)
                transfor->transients.remove(id);
            else if (window_group) {
                list<WaWindow *>::iterator it = ws->wawindow_list.begin();
                for (;it != ws->wawindow_list.end(); ++it) {
                    if ((*it) != this &&
                        (*it)->window_group == window_group &&
                        (*it)->transient_for == (Window) 0)
                        (*it)->transients.remove(id);
                }
            }
        }
    }
    transient_for = (Window) 0;
}

void WaWindow::calcSpacing(void) {
    double _top_spacing, _bottom_spacing,_left_spacing, _right_spacing;

    calc_length(frame->style->top_spacing, frame->style->top_spacing_u,
                ws->vdpi, ws->height, &_top_spacing);
    calc_length(frame->style->bottom_spacing, frame->style->bottom_spacing_u,
                ws->vdpi, ws->height, &_bottom_spacing);
    calc_length(frame->style->left_spacing, frame->style->left_spacing_u,
                ws->hdpi, ws->width, &_left_spacing);
    calc_length(frame->style->right_spacing, frame->style->right_spacing_u,
                ws->hdpi, ws->width, &_right_spacing);

    top_spacing = WA_ROUND_U(_top_spacing);
    bottom_spacing = WA_ROUND_U(_bottom_spacing);
    left_spacing = WA_ROUND_U(_left_spacing);
    right_spacing = WA_ROUND_U(_right_spacing);
}

void WaWindow::gravitate(int multiplier) {
    switch (size.win_gravity) {
        case NorthWestGravity:
            attrib.x += multiplier * (left_spacing + right_spacing);
        case NorthEastGravity:
            attrib.x -= multiplier * right_spacing;
        case NorthGravity:
            attrib.y += multiplier * top_spacing;
            break;
        case SouthWestGravity:
            attrib.x += multiplier * (left_spacing + right_spacing);
        case SouthEastGravity:
            attrib.x -= multiplier * right_spacing;
        case SouthGravity:
            attrib.y -= multiplier * bottom_spacing;
            break;
        case CenterGravity:
            attrib.x += multiplier * ((left_spacing + right_spacing) / 2);
            attrib.y += multiplier * ((top_spacing + bottom_spacing) / 2);
            break;
        case WestGravity:
            attrib.x += multiplier * left_spacing;
            break;
        case EastGravity:
            attrib.x -= multiplier * right_spacing;
        case StaticGravity:
            break;
    }
}

void WaWindow::initPosition(void) {
    if (size.min_width > attrib.width) attrib.width = size.min_width;
    if (size.min_height > attrib.height) attrib.height = size.min_height;
    restore_max.x = attrib.x;
    restore_max.y = attrib.y;
    restore_max.width = attrib.width;
    restore_shade = restore_max.height = attrib.height;
    restore_max.misc0 = restore_max.misc1 = 0;
    old_attrib.x = old_attrib.y = INT_MIN;
    old_attrib.height = old_attrib.width = INT_MAX;
}

void WaWindow::mapWindow(void) {
    if (deleted) return;

    XMapWindow(display, id);
    
    if (desktop_mask & (1L << ws->current_desktop->number)) {
        if (! master) frame->mapRequest = true;
    } else {
        hidden = true;
    }
    mapped = true;
}

void WaWindow::show(void) {
    if ((! (wstate & StateMinimizedMask)) && hidden && mapped && (! master)) {
        XMapWindow(display, id);
        frame->mapRequest = true;
        hidden = false;
        frame->pushRenderEvent();
    }
}

void WaWindow::hide(void) {
    if (! hidden) {
        if (has_focus) meawm_ng->focusRevertFrom(ws, id);
        XUnmapWindow(display, frame->id);
        XUnmapWindow(display, id);
        pending_unmaps++;
        hidden = true;
    }
}

void WaWindow::updateAllAttributes(void) {
    if (master) { master->updateAllAttributes(); return; }

    old_attrib.x = old_attrib.y = INT_MIN;
    old_attrib.height = old_attrib.width = INT_MAX;

    gravitate(RemoveGravity);
    calcSpacing();
    gravitate(ApplyGravity);

    XMoveWindow(display, id, left_spacing, top_spacing);

#ifdef    SHAPE
    frame->apply_shape = false;
    shapeinfo->setShapeOffset(left_spacing, top_spacing);
    frame->shapeUpdateNotify();
#endif // SHAPE

    int m_x, m_y, m_w, m_h;
    if (wstate & StateMaximizedMask) {
        m_x = restore_max.x;
        m_y = restore_max.y;
        m_w = restore_max.width;
        m_h = restore_max.height;
        wstate &= ~StateMaximizedMask;
        _maximize(restore_max.misc0, restore_max.misc1);
        restore_max.x = m_x;
        restore_max.y = m_y;
        restore_max.width = m_w;
        restore_max.height = m_h;
    } else
        redrawWindow();
}

void WaWindow::redrawWindow(bool force_if_viewable) {
    if (master) {
        sendcf = false;
        master->redrawWindow(force_if_viewable);
        if (! sendcf) {
            net->setVirtualPos(this);
            sendConfig();
        }
        return;
    }
    bool move = false, resizew = false, resizeh = false;

    if (old_attrib.x != attrib.x) {
        frame->attrib.x = attrib.x - left_spacing;
        old_attrib.x = attrib.x;
        move = true;
    }
    if (old_attrib.y != attrib.y) {
        frame->attrib.y = attrib.y - top_spacing;
        old_attrib.y = attrib.y;
        move = true;
    }
    if (old_attrib.width != attrib.width) {
        frame->attrib.width = left_spacing + attrib.width + right_spacing;
        old_attrib.width = attrib.width;

        list<WaWindow *>::iterator it = merged.begin();
        for (; it != merged.end(); it++) {
            if ((*it)->mergetype == VertMergeType)
                frame->attrib.width += (*it)->attrib.width;
        }

        resizew = true;
    }
    if (old_attrib.height != attrib.height) {
        frame->attrib.height = top_spacing + attrib.height + bottom_spacing;
        if (! (wstate & StateShadedMask)) {
            list<WaWindow *>::iterator it = merged.begin();
            for (; it != merged.end(); it++) {
                if ((*it)->attrib.height < 1) (*it)->attrib.height = 1;
                if ((*it)->mergetype == HorizMergeType)
                    frame->attrib.height += (*it)->attrib.height;
            }
        }
        old_attrib.height = attrib.height;
        
        resizeh = true;
    }
    if (move) {
        if (wstate & StateMaximizedMask) {
            restore_max.misc0 = ws->v_x + frame->attrib.x;
            restore_max.misc1 = ws->v_y + frame->attrib.y;
            net->setWmState(this);
        }
        if (! (resizew || resizeh))
            XMoveWindow(display, frame->id, frame->attrib.x, frame->attrib.y);
    }

    if ((wstate & StateMaximizedMask) &&
        (resizew || (resizeh && (! (wstate & StateShadedMask))))) {
        wstate &= ~StateMaximizedMask;
        net->setWmState(this);
    }

    if (resizew || resizeh) {
        if (wstate & StateShadedMask)
            XResizeWindow(display, id, attrib.width, restore_shade);
        else
            XResizeWindow(display, id, attrib.width, attrib.height);

        XMoveResizeWindow(display, frame->id, frame->attrib.x,
             frame->attrib.y, frame->attrib.width, frame->attrib.height);

#ifdef    SHAPE
        XResizeWindow(display, frame->apply_shape_buffer,
             frame->attrib.width, frame->attrib.height);
        
        if (! shaped) {
            frame->apply_shape = false;
            frame->shapeUpdateNotify();
        }
#endif // SHAPE        

        if (! (wstate & StateShadedMask)) {
            int cx = left_spacing + attrib.width;
            int cy = top_spacing + attrib.height;
            list<WaWindow *>::iterator mit = merged.begin();
            for (; mit != merged.end(); mit++) {
                Window wd;
                switch ((*mit)->mergetype) {
                    case VertMergeType:
                        (*mit)->attrib.height = cy - top_spacing;
                        XMoveResizeWindow(display, (*mit)->id, cx,
                                          top_spacing,
                                          (*mit)->attrib.width,
                                          (*mit)->attrib.height);
                        cx += (*mit)->attrib.width;
                        XTranslateCoordinates(display, (*mit)->id, ws->id,
                                              0, 0, &(*mit)->attrib.x,
                                              &(*mit)->attrib.y, &wd);
                        
                        break;
                    case HorizMergeType:
                        if (! (wstate & StateShadedMask)) {
                            (*mit)->attrib.width = cx - left_spacing;
                            XMoveResizeWindow(display, (*mit)->id,
                                              left_spacing,
                                              cy, (*mit)->attrib.width,
                                              (*mit)->attrib.height);
                            cy += (*mit)->attrib.height;
                            XTranslateCoordinates(display, (*mit)->id,
                                                  ws->id,
                                                  0, 0, &(*mit)->attrib.x,
                                                  &(*mit)->attrib.y, &wd);
                        }
                        break;
                    case CloneMergeType:
                        (*mit)->attrib.width = attrib.width;
                        (*mit)->attrib.height = attrib.height;
                        XMoveResizeWindow(display, (*mit)->id,
                                          left_spacing,
                                          top_spacing,
                                          (*mit)->attrib.width,
                                          (*mit)->attrib.height);
                        XTranslateCoordinates(display, (*mit)->id, ws->id,
                                              0, 0, &(*mit)->attrib.x,
                                              &(*mit)->attrib.y, &wd);
                }
            }
        }
    }
    if ((move || resizew || resizeh) && (! (wstate & StateShadedMask)) &&
        (! dontsend)) {
        net->setVirtualPos(this);
        if ((! resizew) && (! resizeh)) sendConfig();
    }
    
    drawDecor();
}

void WaWindow::reparentWin(void) {
    XSetWindowAttributes attrib_set;
    
    attrib_set.event_mask = PropertyChangeMask | StructureNotifyMask |
        FocusChangeMask | EnterWindowMask | LeaveWindowMask;
    attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask;
    
    if (validate_window_mapped(id)) {
        XSelectInput(display, id, NoEventMask);
        XReparentWindow(display, id, frame->id, left_spacing, top_spacing);
        XChangeSaveSet(display, id, SetModeInsert);
        XFlush(display);

        XChangeWindowAttributes(display, id, CWEventMask | CWDontPropagate,
                                &attrib_set);
        
#ifdef    SHAPE
        XRectangle *dummy = NULL;
        int n, order;
        if (meawm_ng->shape) {
            XShapeSelectInput(display, id, ShapeNotifyMask);
            dummy = XShapeGetRectangles(display, id, ShapeBounding, &n,
                                        &order);
            if (n > 1) {
                shaped = true;
            }
        }
        XFree(dummy);

        if (shaped) {
            shapeinfo->setShapeOffset(attrib.x, attrib.y);
            frame->shapeUpdateNotify();
        } else {
            shapeinfo->setShapeOffset(attrib.x, attrib.y);
            frame->shapeUpdateNotify();
        }
#endif // SHAPE
        
    }
}

void WaWindow::updateGrabs(void) {
    XUngrabButton(display, AnyButton, AnyModifier, id);
    XUngrabKey(display, AnyKey, AnyModifier, id);

    ActionList *al = actionlists[
        STATE_FROM_MASK_AND_LIST(window_state_mask, actionlists)];
    
    list<Action *>::iterator it = al->actionlist.begin();
    for (; it != al->actionlist.end(); ++it) {
        if ((*it)->type == ButtonPress || (*it)->type == ButtonRelease ||
            (*it)->type == DoubleClick)
		{
            XGrabButton(display, (*it)->detail ? (*it)->detail: AnyButton,
                AnyModifier, id, true, ButtonPressMask |
                ButtonReleaseMask | ButtonMotionMask,
                GrabModeSync, GrabModeSync, None, None);     
        } else if ((*it)->type == KeyPress || (*it)->type == KeyRelease) {
            XGrabKey(display, (*it)->detail ? (*it)->detail: AnyKey,
                AnyModifier, id, true, GrabModeSync, GrabModeSync);
        }
    }
}

#ifdef    SHAPE
void WaWindow::shapeEvent(void) {
    WaFrameWindow *real_frame;

    if (master) {
		real_frame = master->frame;
	} else {
		real_frame = frame;
	}
        
    real_frame->shapeUpdateNotify();
}
#endif // SHAPE

void WaWindow::sendConfig(void) {
    XConfigureEvent ce;

    ce.type              = ConfigureNotify;
    ce.event             = id;
    ce.window            = id;
    ce.x                 = attrib.x;
    ce.y                 = attrib.y;
    ce.width             = attrib.width;
    ce.border_width      = old_bw;
    ce.override_redirect = false;

    if (master)
        ce.above = master->frame->id;
    else
        ce.above = frame->id;

    if (wstate & StateShadedMask)
        ce.height = restore_shade;
    else
        ce.height = attrib.height;

    sendcf = true;

    XSendEvent(display, id, false, StructureNotifyMask, (XEvent *) &ce);
    XSendEvent(display, ws->id, false, StructureNotifyMask,
               (XEvent *) &ce);

    list<WaWindow *>::iterator mit = merged.begin();
    for (; mit != merged.end(); mit++)
        (*mit)->sendConfig();

    XFlush(display);
}

void WaWindow::drawOutline(int x, int y, int width, int height) {
    if (outline_state) clearOutline();
    outline_state = true;

    outl_x = x - left_spacing;
    outl_y = y - top_spacing;
    outl_w = width + left_spacing + right_spacing;
    outl_h = height + top_spacing + bottom_spacing;

    XDrawRectangle(display, ws->id, ws->xor_gc,
                   outl_x, outl_y, outl_w, outl_h);
}

void WaWindow::clearOutline(void) {
    if (! outline_state) return;
    outline_state = false;

    XDrawRectangle(display, ws->id, ws->xor_gc,
                   outl_x, outl_y, outl_w, outl_h);
}

void WaWindow::drawDecor(void) {
    if (! init_done) return;

#ifdef    SHAPE
    frame->apply_shape = false;
#endif // SHAPE
    
    frame->pushRenderEvent();
}

bool WaWindow::incSizeCheck(int width, int height,
                            unsigned int *n_w, unsigned int *n_h) {
    bool resize = false;

    if (size.max_width > size.min_width ||
        size.max_height > size.min_height) {
        if (width < (int) size.min_width) width = size.min_width;
        if (height > 0 && height < (int) size.min_height)
            height = size.min_height;
        if (width > (int) size.max_width) width = size.max_width;
        if (height > (int) size.max_height) height = size.max_height;
    }
    
    *n_w = attrib.width;
    *n_h = attrib.height;
    if ((width >= (int) (attrib.width + size.width_inc)) ||
        (width <= (int) (attrib.width - size.width_inc)) ||
        (int) attrib.width == width) {
        if (width >= (int) size.min_width && width <= (int) size.max_width) {
            resize = true;
            if (size.width_inc == 1)
                *n_w = width;
            else
                *n_w = width - ((width - size.base_width) % size.width_inc);
        }
    }
    if ((height == 0) && top_spacing) {
        if (! (wstate & StateShadedMask)) {
            wstate |= StateShadedMask;
            restore_shade = attrib.height;
            net->setWmState(this);
        }
        *n_h = 0;
        return resize;
    }
    if ((height >= (int) (attrib.height + size.height_inc)) ||
        (height <= (int) (attrib.height - size.height_inc)) ||
        (int) attrib.height == height) {
        if ((height < 1) && (size.min_height <= 1) && top_spacing) {
            resize = true;
            height = 0;
            if (! (wstate & StateShadedMask)) {
                wstate |= StateShadedMask;
                restore_shade = attrib.height;
                net->setWmState(this);
            }
            *n_h = height;
        }
        else if (height > 0 &&
                 (height >= (int) size.min_height &&
                  height <= (int) size.max_height)) {
            resize = true;
            if (wstate & StateShadedMask) {
                wstate &= ~StateShadedMask;
                net->setWmState(this);
            }
            if (size.height_inc == 1)
                *n_h = height;
            else
                *n_h = height -
                    ((height - size.base_height) % size.height_inc);
        }
    }
    if (resize && size.aspect) {
        if (size.base_size) {
            *n_w -= size.base_width;
            *n_h -= size.base_height;
        }
        double aspect = (double) *n_w / (double) *n_h;
        if (aspect < size.min_aspect)
            *n_w = (unsigned int) ((double) *n_h * size.min_aspect);
        else if (aspect > size.max_aspect)
            *n_h = (unsigned int) ((double) *n_w / size.max_aspect);

        if (size.base_size) {
            *n_w += size.base_width;
            *n_h += size.base_height;
        }
    }
    return resize;
}

void WaWindow::raise(void) {
    if (master) {
        master->raise(); return;
    } else {
        list<WaWindow * >::iterator it = merged.begin();
        for (; it != merged.end(); it++)
            ws->raiseWindow((*it)->frame->id, false);
    }
    ws->raiseWindow(frame->id, false);
    ws->restackWindows();
    net->setClientListStacking(ws);
    net->sendNotify(this, RaiseNotify);
}

void WaWindow::lower(void) {
    if (master) {
        master->lower(); return;
    } else {
        list<WaWindow * >::iterator it = merged.begin();
        for (; it != merged.end(); it++)
            ws->lowerWindow((*it)->frame->id, false);
    }
    ws->lowerWindow(frame->id, false);
    ws->restackWindows();
    net->setClientListStacking(ws);
    net->sendNotify(this, LowerNotify);
}

void WaWindow::move(XEvent *e) {
    WaWindow *w;
    XEvent event, *map_ev;
    int px, py, nx, ny, i;
    list<XEvent *> *maprequest_list;
    Window wd;
    unsigned int ui;

    if (! (functions & FunctionMoveMask)) return;

    if (master) w = master;
    else w = this;
    
    XQueryPointer(display, ws->id, &wd, &wd, &px, &py, &i, &i, &ui);

    if (meawm_ng->eh->move_resize != EndMoveResizeType) return;
    nx = w->attrib.x;
    ny = w->attrib.y;
    meawm_ng->eh->move_resize = MoveType;
    setWindowState(wstate | StateMoveMask);
    move_resize = true;
    
    Window grab_window = frame->id;
    Cursor grab_cursor;
    DWindowObject *dwo = (DWindowObject *)
        meawm_ng->findWin(e->xany.window, ANY_DECOR_WINDOW_TYPE);
    if (dwo) grab_cursor = dwo->sb->style->cursor;
    else grab_cursor = None;

    if (e && e->type == MapRequest) {
        nx = w->attrib.x = px + left_spacing;
        ny = w->attrib.y = py + top_spacing;
        drawOutline(nx, ny, w->attrib.width, w->attrib.height);
    }
    maprequest_list = new list<XEvent *>;
    if (XGrabPointer(display, grab_window, true, ButtonReleaseMask |
                     ButtonPressMask | PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask, GrabModeAsync, GrabModeAsync,
                     None, grab_cursor, CurrentTime) != GrabSuccess) {
        move_resize = false;
        meawm_ng->eh->move_resize = EndMoveResizeType;
        setWindowState(wstate & ~StateMoveMask);
        delete maprequest_list;
        return;
    }
    if (XGrabKeyboard(display, grab_window, true, GrabModeAsync,
                      GrabModeAsync, CurrentTime) != GrabSuccess) {
        move_resize = false;
        meawm_ng->eh->move_resize = EndMoveResizeType;
        setWindowState(wstate & ~StateMoveMask);
        XUngrabPointer(display, CurrentTime);
        delete maprequest_list;
        return;
    }
    for (;;) {
        meawm_ng->eh->eventLoop(&meawm_ng->eh->moveresize_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                while (XCheckTypedWindowEvent(display, event.xmotion.window,
                                              MotionNotify, &event));
                nx += event.xmotion.x_root - px;
                ny += event.xmotion.y_root - py;
                px  = event.xmotion.x_root;
                py  = event.xmotion.y_root;
                w->drawOutline(nx, ny, w->attrib.width, w->attrib.height);
                break;
            case LeaveNotify:
            case EnterNotify:
                if (ws->west->id == event.xcrossing.window ||
                    ws->east->id == event.xcrossing.window ||
                    ws->north->id == event.xcrossing.window ||
                    ws->south->id == event.xcrossing.window) {
                    clearOutline();
                    meawm_ng->eh->handleEvent(&event);
                    w->drawOutline(nx, ny, w->attrib.width, w->attrib.height);
                } else if (event.type == LeaveNotify) {
                    int cx, cy;
                    XQueryPointer(display, ws->id, &wd, &wd, &cx, &cy,
                                  &i, &i, &ui);
                    nx += cx - px;
                    ny += cy - py;
                    px = cx;
                    py = cy;
                    w->drawOutline(nx, ny, w->attrib.width, w->attrib.height);
                }
                break;
            case DestroyNotify:
            case UnmapNotify:
                if ((((event.type == UnmapNotify)? event.xunmap.window:
                      event.xdestroywindow.window) == w->id)) {
                    while (! maprequest_list->empty()) {
                        XPutBackEvent(display, maprequest_list->front());
                        delete maprequest_list->front();
                        maprequest_list->pop_front();
                    }
                    delete maprequest_list;
                    XPutBackEvent(display, &event);
                    w->clearOutline();
                    XUngrabKeyboard(display, CurrentTime);
                    XUngrabPointer(display, CurrentTime);
                    meawm_ng->eh->move_resize = EndMoveResizeType;
                    setWindowState(wstate & ~StateMoveMask);
                    move_resize = false;
                    return;
                }
                meawm_ng->eh->evUnmapDestroy(&event);
                break;
            case ConfigureRequest:
                if (event.xconfigurerequest.window != id)
                    meawm_ng->eh->evConfigureRequest(&event.xconfigurerequest);
                break;
            case MapRequest:
                map_ev = new XEvent;
                *map_ev = event;
                maprequest_list->push_front(map_ev); break;
            case ButtonPress:
            case ButtonRelease:
                event.xbutton.window = w->id;
            case KeyPress:
            case KeyRelease:
                if (event.type == KeyPress || event.type == KeyRelease)
                    event.xkey.window = w->id;
                w->clearOutline();
                meawm_ng->eh->handleEvent(&event);
                w->drawOutline(nx, ny, attrib.width, attrib.height);
                if (meawm_ng->eh->move_resize != EndMoveResizeType) break;
                setWindowState(wstate & ~StateMoveMask);
                w->clearOutline();
                w->attrib.x = nx;
                w->attrib.y = ny;
                w->redrawWindow();
                checkMoveMerge(w->attrib.x, w->attrib.y);
                while (! maprequest_list->empty()) {
                    XPutBackEvent(display, maprequest_list->front());
                    delete maprequest_list->front();
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                move_resize = false;
                XUngrabKeyboard(display, CurrentTime);
                XUngrabPointer(display, CurrentTime);
                return;
        }
    }
}

void WaWindow::moveOpaque(XEvent *e) {
    int px, py, prelx, prely, i;
    int sw = attrib.width;
    int sh = attrib.height;
    unsigned int ui;
    Window wd;
    XEvent ed;
    list<XEvent *> maprequest_list;

    if (! (functions & FunctionMoveMask)) return;
  
    XQueryPointer(display, ws->id, &wd, &wd, &px, &py, &i, &i, &ui);
    prelx = px - attrib.x;
    prely = py - attrib.y;
    int pposx = px;
    int pposy = py;
  
    for (;;) {
        bool status = _moveOpaque(e, sw, sh, pposx, pposy, &maprequest_list);
        if (! status) return;

        XQueryPointer(display, ws->id, &wd, &wd, &px, &py, &i, &i, &ui);

        pposx = attrib.x + prelx;
        pposy = attrib.y + prely;   
    
        XWarpPointer(display, None, None, 0, 0, 0, 0, pposx - px, pposy - py);
        XSync(display, false);
        while (XCheckTypedEvent(display, MotionNotify, &ed));
        while (XCheckTypedEvent(display, EnterNotify, &ed));
        while (XCheckTypedEvent(display, LeaveNotify, &ed));
    }
}

bool WaWindow::_moveOpaque(XEvent *e, int saved_w, int saved_h, int px,
                           int py, list<XEvent *> *maprequest_list) {
    WaWindow *w;
    XEvent event, *map_ev;
    int sx, sy, nx, ny, mnx, mny;

    if (master && mergemode == NullMergeType) w = master;
    else w = this;
    
    if (meawm_ng->eh->move_resize != EndMoveResizeType) return false;
    sx = nx = mnx = attrib.x;
    sy = ny = mny = attrib.y;
    meawm_ng->eh->move_resize = MoveOpaqueType;
    setWindowState(wstate | StateMoveMask);
    move_resize = true;
    if (master) {
        mnx = w->attrib.x;
        mny = w->attrib.y;
    }

    Window grab_window = frame->id;
    Cursor grab_cursor;
    DWindowObject *dwo = (DWindowObject *)
        meawm_ng->findWin(e->xany.window, ANY_DECOR_WINDOW_TYPE);
    if (dwo) grab_cursor = dwo->sb->style->cursor;
    else grab_cursor = None;

    if (e && e->type == MapRequest) {
        nx = w->attrib.x = px + left_spacing;
        ny = w->attrib.y = py + top_spacing;
        w->redrawWindow();
        net->setState(this, NormalState);
        net->setVirtualPos(this);
    }
    dontsend = true;    
    if (XGrabPointer(display, grab_window, true,
                     ButtonReleaseMask | ButtonPressMask |
                     PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask, GrabModeAsync, GrabModeAsync,
                     None, grab_cursor, CurrentTime) != GrabSuccess) {
        move_resize = false;
        meawm_ng->eh->move_resize = EndMoveResizeType;
        setWindowState(wstate & ~StateMoveMask);
        return false;
    }
    if (XGrabKeyboard(display, grab_window, true, GrabModeAsync,
                      GrabModeAsync, CurrentTime) != GrabSuccess) {
        move_resize = false;
        meawm_ng->eh->move_resize = EndMoveResizeType;
        setWindowState(wstate & ~StateMoveMask);
        XUngrabPointer(display, CurrentTime);
        return false;
    }
    for (;;) {
        meawm_ng->eh->eventLoop(&meawm_ng->eh->moveresize_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                while (XCheckTypedWindowEvent(display, event.xmotion.window,
                                              MotionNotify, &event));
                nx += event.xmotion.x_root - px;
                ny += event.xmotion.y_root - py;
                if (master) {
                    mnx += event.xmotion.x_root - px;
                    mny += event.xmotion.y_root - py;
                }
                px = event.xmotion.x_root;
                py = event.xmotion.y_root;
                if (mergemode != NullMergeType) {
                    if (checkMoveMerge(nx, ny, saved_w, saved_h)) {
                        XSync(display, false);
                        while (XCheckTypedEvent(display, FocusIn, &event));
                        while (XCheckTypedEvent(display, FocusOut, &event));
                        dontsend = move_resize = false;
                        meawm_ng->eh->move_resize = EndMoveResizeType;
                        setWindowState(wstate & ~StateMoveMask);
                        return true;
                    } else if (! master) {
                        w->attrib.x = nx;
                        w->attrib.y = ny;
                        w->redrawWindow();
                    }
                } else {
                    if (master) {
                        w->attrib.x = mnx;
                        w->attrib.y = mny;
                    } else {
                        w->attrib.x = nx;
                        w->attrib.y = ny;
                    }
                    w->redrawWindow();
                }
                break;
            case LeaveNotify:
            case EnterNotify:
                if (ws->west->id == event.xcrossing.window ||
                    ws->east->id == event.xcrossing.window ||
                    ws->north->id == event.xcrossing.window ||
                    ws->south->id == event.xcrossing.window) {
                    meawm_ng->eh->handleEvent(&event);
                } else if (event.type == LeaveNotify) {
                    unsigned int ui;
                    Window wd;
                    int cx, cy, i;
                    XQueryPointer(display, ws->id, &wd, &wd, &cx, &cy,
                                  &i, &i, &ui);
                    nx += cx - px;
                    ny += cy - py;
                    if (master) {
                        mnx += cx - px;
                        mny += cy - py;
                    }
                    px = cx;
                    py = cy;
                    if (mergemode != NullMergeType) {
                        if (checkMoveMerge(nx, ny, saved_w, saved_h)) {
                            XSync(display, false);
                            while (XCheckTypedEvent(display, FocusIn, &event));
                            while (XCheckTypedEvent(display, FocusOut,
                                                    &event));
                            dontsend = move_resize = false;
                            meawm_ng->eh->move_resize = EndMoveResizeType;
                            setWindowState(wstate & ~StateMoveMask);
                            return true;
                        } else if (! master) {
                            w->attrib.x = nx;
                            w->attrib.y = ny;
                            w->redrawWindow();
                        }
                    } else {
                        if (master) {
                            w->attrib.x = mnx;
                            w->attrib.y = mny;
                        } else {
                            w->attrib.x = nx;
                            w->attrib.y = ny;
                        }
                        w->redrawWindow();
                    }
                }
                break;
            case DestroyNotify:
            case UnmapNotify:
                if ((((event.type == UnmapNotify)? event.xunmap.window:
                      event.xdestroywindow.window) == w->id)) {
                    while (! maprequest_list->empty()) {
                        XPutBackEvent(display, maprequest_list->front());
                        delete maprequest_list->front();
                        maprequest_list->pop_front();
                    }
                    XPutBackEvent(display, &event);
                    XUngrabKeyboard(display, CurrentTime);
                    XUngrabPointer(display, CurrentTime);
                    meawm_ng->eh->move_resize = EndMoveResizeType;
                    setWindowState(wstate & ~StateMoveMask);
                    dontsend = move_resize = false;
                    return false;
                }
                meawm_ng->eh->evUnmapDestroy(&event);
                break;
            case ConfigureRequest:
                if (event.xconfigurerequest.window != w->id)
                    meawm_ng->eh->evConfigureRequest(&event.xconfigurerequest);
                break;
            case MapRequest:
                map_ev = new XEvent;
                *map_ev = event;
                maprequest_list->push_front(map_ev); break;
            case ButtonPress:
            case ButtonRelease:
                event.xbutton.window = id;
            case KeyPress:
            case KeyRelease:
                if (event.type == KeyPress || event.type == KeyRelease)
                    event.xkey.window = id;
                int merge_state = mergemode;
                meawm_ng->eh->handleEvent(&event);
                if (merge_state != mergemode) {
                    if (checkMoveMerge(nx, ny, saved_w, saved_h)) {
                        XSync(display, false);
                        while (XCheckTypedEvent(display, FocusIn, &event));
                        while (XCheckTypedEvent(display, FocusOut,
                                                &event));
                        dontsend = move_resize = false;
                        if (meawm_ng->eh->move_resize != EndMoveResizeType) {
                            meawm_ng->eh->move_resize = EndMoveResizeType;
                            setWindowState(wstate & ~StateMoveMask);
                            return true;
                        }
                    }
                }
                if (meawm_ng->eh->move_resize != EndMoveResizeType) break;
                setWindowState(wstate & ~StateMoveMask);
                if (w->attrib.x != sx || w->attrib.y != sy) {
                    w->sendConfig();
                    net->setVirtualPos(w);
                }
                while (! maprequest_list->empty()) {
                    XPutBackEvent(display, maprequest_list->front());
                    delete maprequest_list->front();
                    maprequest_list->pop_front();
                }
                dontsend = move_resize = false;
                XUngrabKeyboard(display, CurrentTime);
                XUngrabPointer(display, CurrentTime);
                return false;
        }
    }
}

void WaWindow::resize(XEvent *e, int how) {
    XEvent event, *map_ev;
    int px, py, width, height, n_x, o_x, n_y, o_y, i;
    list<XEvent *> *maprequest_list;
    Window wd;
    unsigned int ui, n_w, n_h, o_w, o_h;
    WaWindow *w;

    if (! (functions & FunctionResizeMask)) return;
    
    if (master && mergetype == CloneMergeType) w = master;
    else w = this;
    
    XQueryPointer(display, ws->id, &wd, &wd, &px, &py, &i, &i, &ui);

    if (meawm_ng->eh->move_resize != EndMoveResizeType) return;
    n_x    = o_x = attrib.x;
    n_y    = o_y = attrib.y;
    width  = n_w = o_w = attrib.width;
    height = n_h = o_h = attrib.height;
    meawm_ng->eh->move_resize = ResizeType;
    setWindowState(wstate | StateResizeMask);
    move_resize = true;

    Window grab_window = frame->id;
    Cursor grab_cursor;
    DWindowObject *dwo = (DWindowObject *)
        meawm_ng->findWin(e->xany.window, ANY_DECOR_WINDOW_TYPE);
    if (dwo) grab_cursor = dwo->sb->style->cursor;
    else grab_cursor = None;

    if (e && e->type == MapRequest) {
        if (how & EastResizeTypeMask)
            n_x = attrib.x = px - attrib.width - left_spacing - right_spacing;
        else n_x = attrib.x = px;
        if (how & SouthResizeTypeMask)
            n_y = attrib.y = py - attrib.height - top_spacing - bottom_spacing;
        else n_y = attrib.y = py;
        drawOutline(n_x, n_y, n_w, n_h);
    }    
    maprequest_list = new list<XEvent *>;
    if (XGrabPointer(display, grab_window, true,
                     ButtonReleaseMask | ButtonPressMask |
                     PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask, GrabModeAsync, GrabModeAsync,
                     None, grab_cursor, CurrentTime) != GrabSuccess) {
        move_resize = false;
        meawm_ng->eh->move_resize = EndMoveResizeType;
        setWindowState(wstate & ~StateResizeMask);
        delete maprequest_list;
        return;
    }
    if (XGrabKeyboard(display, grab_window, true, GrabModeAsync,
                      GrabModeAsync, CurrentTime) != GrabSuccess) {
        move_resize = false;
        meawm_ng->eh->move_resize = EndMoveResizeType;
        setWindowState(wstate & ~StateResizeMask);
        XUngrabPointer(display, CurrentTime);
        delete maprequest_list;
        return;
    }
    for (;;) {
        meawm_ng->eh->eventLoop(&meawm_ng->eh->moveresize_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                while (XCheckTypedWindowEvent(display, event.xmotion.window,
                                              MotionNotify, &event));
                if (how & EastResizeTypeMask)
                    width += event.xmotion.x_root - px;
                else if (how & WestResizeTypeMask)
                    width -= event.xmotion.x_root - px;
                if (how & SouthResizeTypeMask)
                    height += event.xmotion.y_root - py;
                else if (how & NorthResizeTypeMask)
                    height -= event.xmotion.y_root - py;
                px = event.xmotion.x_root;
                py = event.xmotion.y_root;
                if (w->incSizeCheck(width, height, &n_w, &n_h)) {
                    if (how & WestResizeTypeMask)
                        n_x -= n_w - o_w;
                    if (how & NorthResizeTypeMask)
                        n_y -= n_h - o_h;
                    o_x = n_x;
                    o_y = n_y;
                    o_w = n_w;
                    o_h = n_h;
                    drawOutline(n_x, n_y, n_w, n_h);
                }
                break;
            case LeaveNotify:
            case EnterNotify:
                if (ws->west->id == event.xcrossing.window ||
                    ws->east->id == event.xcrossing.window ||
                    ws->north->id == event.xcrossing.window ||
                    ws->south->id == event.xcrossing.window) {
                    int old_vx = ws->v_x;
                    int old_vy = ws->v_y;
                    clearOutline();
                    meawm_ng->eh->handleEvent(&event);
                    px -= (ws->v_x - old_vx);
                    py -= (ws->v_y - old_vy);
                    n_x = attrib.x;
                    n_y = attrib.y;
                    if (how & WestResizeTypeMask)
                        n_x -= n_w - attrib.width;
                    else if (how & NorthResizeTypeMask)
                        n_y -= n_h - attrib.height;
                    drawOutline(n_x, n_y, n_w, n_h);
                } else if (event.type == LeaveNotify) {
                    int cx, cy;
                    XQueryPointer(display, ws->id, &wd, &wd, &cx, &cy,
                                  &i, &i, &ui);
                    if (how & EastResizeTypeMask)
                        width += cx - px;
                    else if (how & WestResizeTypeMask)
                        width -= cx - px;
                    if (how & SouthResizeTypeMask)
                        height += cy - py;
                    else if (how & NorthResizeTypeMask)
                        height -= cy - py;
                    px = cx;
                    py = cy;
                    if (incSizeCheck(width, height, &n_w, &n_h)) {
                        if (how & WestResizeTypeMask)
                            n_x -= n_w - o_w;
                        if (how & NorthResizeTypeMask)
                            n_y -= n_h - o_h;
                        o_x = n_x;
                        o_y = n_y;
                        o_w = n_w;
                        o_h = n_h;
                        drawOutline(n_x, n_y, n_w, n_h);
                    }
                }
                break;
            case DestroyNotify:
            case UnmapNotify:
                if ((((event.type == UnmapNotify)? event.xunmap.window:
                      event.xdestroywindow.window) == id)) {
                    while (! maprequest_list->empty()) {
                        XPutBackEvent(display, maprequest_list->front());
                        delete maprequest_list->front();
                        maprequest_list->pop_front();
                    }
                    delete maprequest_list;
                    XPutBackEvent(display, &event);
                    clearOutline();
                    XUngrabKeyboard(display, CurrentTime);
                    XUngrabPointer(display, CurrentTime);
                    meawm_ng->eh->move_resize = EndMoveResizeType;
                    setWindowState(wstate & ~StateResizeMask);
                    move_resize = false;
                    return;
                }
                meawm_ng->eh->evUnmapDestroy(&event);
                break;
            case ConfigureRequest:
                if (event.xconfigurerequest.window != id)
                    meawm_ng->eh->evConfigureRequest(&event.xconfigurerequest);
                break;
            case MapRequest:
                map_ev = new XEvent;
                *map_ev = event;
                maprequest_list->push_front(map_ev); break;
            case ButtonPress:
            case ButtonRelease:
                event.xbutton.window = id;
            case KeyPress:
            case KeyRelease:
                if (event.type == KeyPress || event.type == KeyRelease)
                    event.xkey.window = id;
                clearOutline();
                meawm_ng->eh->handleEvent(&event);
                drawOutline(n_x, n_y, n_w, n_h);
                if (meawm_ng->eh->move_resize != EndMoveResizeType) break;
                setWindowState(wstate & ~StateResizeMask);
                clearOutline();
                w->attrib.width = n_w;
                w->attrib.height = n_h;
                w->attrib.x = n_x;
                w->attrib.y = n_y;
                if (master) {
                    if (mergetype == VertMergeType)
                        master->old_attrib.width++;
                    if (mergetype == HorizMergeType)
                        master->old_attrib.height++;
                }
                w->redrawWindow();
                while (! maprequest_list->empty()) {
                    XPutBackEvent(display, maprequest_list->front());
                    delete maprequest_list->front();
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                move_resize = false;
                XUngrabKeyboard(display, CurrentTime);
                XUngrabPointer(display, CurrentTime);
                return;
        }
    }
}

void WaWindow::resizeOpaque(XEvent *e, int how) {
    XEvent event, *map_ev;
    int px, py, width, height, i;
    list<XEvent *> *maprequest_list;
    Window wd;
    unsigned int n_w, n_h, sw, sh, ui;
    WaWindow *w;

    if (! (functions & FunctionResizeMask)) return;
    
    if (master && mergetype == CloneMergeType) w = master;
    else w = this;
    
    XQueryPointer(display, ws->id, &wd, &wd, &px, &py, &i, &i, &ui);

    if (meawm_ng->eh->move_resize != EndMoveResizeType) return;
    dontsend = true;
    sw = width  = n_w = attrib.width;
    sh = height = n_h = attrib.height;
    meawm_ng->eh->move_resize = ResizeOpaqueType;
    setWindowState(wstate | StateResizeMask);
    move_resize = true;

    Window grab_window = frame->id;
    Cursor grab_cursor;
    DWindowObject *dwo = (DWindowObject *)
        meawm_ng->findWin(e->xany.window, ANY_DECOR_WINDOW_TYPE);
    if (dwo) grab_cursor = dwo->sb->style->cursor;
    else grab_cursor = None;
    
    if (e && e->type == MapRequest) {
        if (how & EastResizeTypeMask)
            attrib.x = px - attrib.width - left_spacing - right_spacing;
        else attrib.x = px;
        if (how & SouthResizeTypeMask)
            attrib.y = py - attrib.height - top_spacing - bottom_spacing;
        else attrib.y = py;
        redrawWindow();
        net->setState(this, NormalState);
        net->setVirtualPos(this);
    }
    
    maprequest_list = new list<XEvent *>;
    if (XGrabPointer(display, grab_window, true,
                     ButtonReleaseMask | ButtonPressMask |
                     PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask, GrabModeAsync, GrabModeAsync,
                     None, grab_cursor, CurrentTime) != GrabSuccess) {
        move_resize = false;
        meawm_ng->eh->move_resize = EndMoveResizeType;
        setWindowState(wstate & ~StateResizeMask);
        delete maprequest_list;
        return;
    }
    if (XGrabKeyboard(display, grab_window, true, GrabModeAsync,
                      GrabModeAsync, CurrentTime) != GrabSuccess) {
        move_resize = false;
        meawm_ng->eh->move_resize = EndMoveResizeType;
        setWindowState(wstate & ~StateResizeMask);
        XUngrabPointer(display, CurrentTime);
        delete maprequest_list;
        return;
    }
    for (;;) {
        meawm_ng->eh->eventLoop(&meawm_ng->eh->moveresize_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                while (XCheckTypedWindowEvent(display, event.xmotion.window,
                                              MotionNotify, &event));
                if (how & EastResizeTypeMask)
                    width += event.xmotion.x_root - px;
                else if (how & WestResizeTypeMask)
                    width -= event.xmotion.x_root - px;
                if (how & SouthResizeTypeMask)
                    height += event.xmotion.y_root - py;
                else if (how & NorthResizeTypeMask)
                    height -= event.xmotion.y_root - py;
                px = event.xmotion.x_root;
                py = event.xmotion.y_root;
                if (w->incSizeCheck(width, height, &n_w, &n_h)) {
                    if (how & WestResizeTypeMask)
                        w->attrib.x -= n_w - attrib.width;
                    w->attrib.width  = n_w;
                    if (how & NorthResizeTypeMask)
                        w->attrib.y -= n_h - attrib.height;
                    w->attrib.height = n_h;
                    if (master) {
                        if (mergetype == VertMergeType)
                            master->old_attrib.width++;
                        if (mergetype == HorizMergeType)
                            master->old_attrib.height++;
                    }
                    w->redrawWindow();
                }
                break;
            case LeaveNotify:
            case EnterNotify:
                if (ws->west->id == event.xcrossing.window ||
                    ws->east->id == event.xcrossing.window ||
                    ws->north->id == event.xcrossing.window ||
                    ws->south->id == event.xcrossing.window) {
                    int old_vx = ws->v_x;
                    int old_vy = ws->v_y;
                    meawm_ng->eh->handleEvent(&event);
                    px -= (ws->v_x - old_vx);
                    py -= (ws->v_y - old_vy);
                } else if (event.type == LeaveNotify) {
                    int cx, cy;
                    XQueryPointer(display, ws->id, &wd, &wd, &cx, &cy,
                                  &i, &i, &ui);
                    if (how & EastResizeTypeMask)
                        width += cx - px;
                    else if (how & WestResizeTypeMask)
                        width -= cx - px;
                    if (how & SouthResizeTypeMask)
                        height += cy - py;
                    else if (how & NorthResizeTypeMask)
                        height -= cy - py;
                    px = cx;
                    py = cy;
                    if (w->incSizeCheck(width, height, &n_w, &n_h)) {
                        if (how & WestResizeTypeMask)
                            w->attrib.x -= n_w - attrib.width;
                        w->attrib.width  = n_w;
                        if (how & NorthResizeTypeMask)
                            w->attrib.y -= n_h - attrib.height;
                        w->attrib.height = n_h;
                        if (master) {
                            if (mergetype == VertMergeType)
                                master->old_attrib.width++;
                            if (mergetype == HorizMergeType)
                                master->old_attrib.height++;
                        }
                        w->redrawWindow();
                    }
                }
                break;
            case DestroyNotify:
            case UnmapNotify:
                if ((((event.type == UnmapNotify)? event.xunmap.window:
                      event.xdestroywindow.window) == id)) {
                    while (! maprequest_list->empty()) {
                        XPutBackEvent(display, maprequest_list->front());
                        delete maprequest_list->front();
                        maprequest_list->pop_front();
                    }
                    delete maprequest_list;
                    XPutBackEvent(display, &event);
                    XUngrabKeyboard(display, CurrentTime);
                    XUngrabPointer(display, CurrentTime);
                    meawm_ng->eh->move_resize = EndMoveResizeType;
                    setWindowState(wstate & ~StateResizeMask);
                    dontsend = move_resize = false;
                    return;
                }
                meawm_ng->eh->evUnmapDestroy(&event);
                break;
            case ConfigureRequest:
                if (event.xconfigurerequest.window != id)
                    meawm_ng->eh->evConfigureRequest(&event.xconfigurerequest);
                break;
            case MapRequest:
                map_ev = new XEvent;
                *map_ev = event;
                maprequest_list->push_front(map_ev); break;
            case ButtonPress:
            case ButtonRelease:
                event.xbutton.window = id;
            case KeyPress:
            case KeyRelease:
                if (event.type == KeyPress || event.type == KeyRelease)
                    event.xkey.window = id;
                meawm_ng->eh->handleEvent(&event);
                width = w->attrib.width;
                height = w->attrib.height;
                if (meawm_ng->eh->move_resize != EndMoveResizeType) break;
                setWindowState(wstate & ~StateResizeMask);
                if (w->attrib.width != sw || w->attrib.height != sh) {
                    net->setVirtualPos(w);
                }
                while (! maprequest_list->empty()) {
                    XPutBackEvent(display, maprequest_list->front());
                    delete maprequest_list->front();
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                dontsend = move_resize = false;
                XUngrabKeyboard(display, CurrentTime);
                XUngrabPointer(display, CurrentTime);
                return;
        }
    }
}

void WaWindow::resizeSmart(XEvent *e) {
    int x, y, i;
    Window wd;
    unsigned int ui;
    int resize_mask = 0;
    
    XQueryPointer(display, ws->id, &wd, &wd, &x, &y, &i, &i, &ui);

    if (x >= (attrib.x + (int) attrib.width / 2))
        resize_mask |= EastResizeTypeMask;
    else
        resize_mask |= WestResizeTypeMask;

    if (y >= (attrib.y + (int) attrib.height / 2))
        resize_mask |= SouthResizeTypeMask;
    else
        resize_mask |= NorthResizeTypeMask;

    resize(e, resize_mask);
}

void WaWindow::resizeSmartOpaque(XEvent *e) {
    int x, y, i;
    Window wd;
    unsigned int ui;
    int resize_mask = 0;
    
    XQueryPointer(display, ws->id, &wd, &wd, &x, &y, &i, &i, &ui);

    if (x >= (attrib.x + (int) attrib.width / 2))
        resize_mask |= EastResizeTypeMask;
    else
        resize_mask |= WestResizeTypeMask;

    if (y >= (attrib.y + (int) attrib.height / 2))
        resize_mask |= SouthResizeTypeMask;
    else
        resize_mask |= NorthResizeTypeMask;

    resizeOpaque(e, resize_mask);
}

void WaWindow::_maximize(int x, int y) {
    if (master) return;
    int new_width, new_height;
    unsigned int n_w, n_h;

    if (! (functions & FunctionResizeMask)) return;
    if (! (functions & FunctionMoveMask)) return;
    
    if (wstate & StateMaximizedMask) return;
    
    int workx, worky;
    unsigned int workw, workh;
    ws->getWorkareaSize(&workx, &worky, &workw, &workh);

    if (wstate & StateFullscreenMask) {
        new_width = ws->width - left_spacing - right_spacing;
        new_height = ws->height - top_spacing - bottom_spacing;
    } else {
        new_width = workw - left_spacing - right_spacing;
        new_height = workh - top_spacing - bottom_spacing;
    }

    restore_max.width = attrib.width;
    restore_max.height = attrib.height;
    int rest_x = attrib.x;
    int rest_y = attrib.y;

    list<WaWindow *>::iterator mit = merged.begin();
    for (; mit != merged.end(); mit++) {
        switch ((*mit)->mergetype) {
            case VertMergeType:
                new_width -= (*mit)->attrib.width;
                break;
            case HorizMergeType:
                new_height -= (*mit)->attrib.height;
                break;
        }
    }
    
    if (wstate & StateShadedMask) {
        restore_max.height = restore_shade;
        restore_shade = new_height;
        new_height = attrib.height;
    }
    if (incSizeCheck(new_width, new_height, &n_w, &n_h)) {
        attrib.x = workx;
        attrib.y = worky;
        restore_max.x = rest_x - attrib.x;
        restore_max.y = rest_y - attrib.y;
        if (x >= 0 && y >= 0) {
            attrib.x = (x - ws->v_x);
            attrib.y = (y - ws->v_y);
            restore_max.misc0 = x;
            restore_max.misc1 = y;
        } else {
            restore_max.misc0 = ws->v_x + attrib.x;
            restore_max.misc1 = ws->v_y + attrib.y;
        }
        if (wstate & StateFullscreenMask) {
            attrib.x = 0;
            attrib.y = 0;
        }
        attrib.x += left_spacing;
        attrib.y += top_spacing;
        attrib.width = n_w;
        attrib.height = n_h;
        redrawWindow();
        wstate |= StateMaximizedMask;
        
        net->setWmState(this);
    }
}

void WaWindow::unMaximize(void) {
    if (master) { master->unMaximize(); return; }
    int rest_height, tmp_shade_height = 0;
    unsigned int n_w, n_h;
    
    if (wstate & StateMaximizedMask) {
        if (wstate & StateShadedMask) {
            rest_height = attrib.height;
            tmp_shade_height = restore_max.height;
        }
        else rest_height = restore_max.height;
        if (incSizeCheck(restore_max.width, rest_height, &n_w, &n_h)) {
            attrib.x = restore_max.x + (restore_max.misc0 - ws->v_x);
            attrib.y = restore_max.y + (restore_max.misc1 - ws->v_y);
            attrib.width = n_w;
            attrib.height = n_h;
            wstate &= ~StateMaximizedMask;
            redrawWindow();
            if (wstate & StateShadedMask) restore_shade = tmp_shade_height;
            net->setWmState(this);
        }
    }
}

void WaWindow::toggleMaximize(void) {
    if (! (wstate & StateMaximizedMask)) maximize();
    else unMaximize();
}

void WaWindow::close(void) {
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = id;
    ev.xclient.message_type = net->wm_protocols;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = net->wm_delete_window;
    ev.xclient.data.l[1] = CurrentTime;

    XSendEvent(display, id, false, NoEventMask, &ev);
}

void WaWindow::kill(void) {
    XKillClient(display, id);
}

void WaWindow::closeKill(void) {
    if (protocol_mask & DeleteWindowProtocalMask) close();
    else kill();
}

void WaWindow::shade(void) {
    if (! (functions & FunctionResizeMask)) return;
    
    if (master) { master->shade(); return; }
    unsigned int n_w, n_h;
    
    if (incSizeCheck(attrib.width, 0, &n_w, &n_h)) {
        attrib.width = n_w;
        attrib.height = n_h;
        redrawWindow();

        MERGED_LOOP {
            if (wstate & StateShadedMask) _mw->wstate |= StateShadedMask;
            else _mw->wstate &= ~StateShadedMask;
            net->setWmState(_mw);
        }
    }
}

void WaWindow::unShade(void) {
    if (master) { master->unShade(); return; }
    if (wstate & StateShadedMask) {
        attrib.height = restore_shade;
        
        MERGED_LOOP {
            _mw->wstate &= ~StateShadedMask;
            net->setWmState(_mw);
        }
        redrawWindow();
    }
}

void WaWindow::toggleShade(void) {
    if (wstate & StateShadedMask) unShade();
    else shade();
}

void WaWindow::sticky(void) {
    if (master) { master->sticky(); return; }
    MERGED_LOOP {
        _mw->wstate |= StateStickyMask;
        net->setWmState(_mw);
    }
}

void WaWindow::unSticky(void) {
    if (master) { master->unSticky(); return; }
    MERGED_LOOP {
        _mw->wstate &= ~StateStickyMask;
        net->setWmState(_mw);
    }
}

void WaWindow::toggleSticky(void) {
    if (master) { master->toggleSticky(); return; }

    if (wstate & StateStickyMask) wstate &= ~StateStickyMask;
    else wstate |= StateStickyMask;
    
    MERGED_LOOP {
        if (wstate & StateStickyMask) _mw->wstate |= StateStickyMask;
        else _mw->wstate &= ~StateStickyMask;
        net->setWmState(_mw);
    }
}

void WaWindow::minimize(void) {
    if (master) { master->minimize(); return; }
    if (wstate & StateMinimizedMask) return;
    MERGED_LOOP {
        net->setState(_mw, IconicState);
        net->setWmState(_mw);
    }
}

void WaWindow::unMinimize(void) {
    if (master) { master->unMinimize(); return; }
    if (! (wstate & StateMinimizedMask)) return;
    MERGED_LOOP {
        net->setState(_mw, NormalState);
        net->setWmState(_mw);
    }
}

void WaWindow::toggleMinimize(void) {
    if (master) { master->toggleMinimize(); return; }
    if (wstate & StateMinimizedMask) unMinimize();
    else minimize();
}

void WaWindow::fullscreenOn(void) {
    if (! (functions & FunctionResizeMask)) return;
    if (! (functions & FunctionMoveMask)) return;
    
    if (master) { master->fullscreenOn(); return; }
    
    if (wstate & StateFullscreenMask) return;

    wstate |= StateFullscreenMask;

    if (wstate & StateMaximizedMask) {
        wstate &= ~StateMaximizedMask;
        int res_x = restore_max.x;
        int res_y = restore_max.y;
        int res_w = restore_max.width;
        int res_h = restore_max.height;
        _maximize(restore_max.misc0, restore_max.misc1);
        restore_max.x = res_x;
        restore_max.y = res_y;
        restore_max.width = res_w;
        restore_max.height = res_h;
    }
    net->setWmState(this);
}

void WaWindow::fullscreenOff(void) {
    if (master) { master->fullscreenOff(); return; }
    if (! (wstate & StateFullscreenMask)) return;

    wstate &= ~StateFullscreenMask;

    if (wstate & StateMaximizedMask) {
        wstate &= ~StateMaximizedMask;
        int res_x = restore_max.x;
        int res_y = restore_max.y;
        int res_w = restore_max.width;
        int res_h = restore_max.height;
        _maximize(restore_max.misc0, restore_max.misc1);
        restore_max.x = res_x;
        restore_max.y = res_y;
        restore_max.width = res_w;
        restore_max.height = res_h;
    }
    net->setWmState(this);
}

void WaWindow::fullscreenToggle(void) {
    if (wstate & StateFullscreenMask) fullscreenOff();
    else fullscreenOn();
}

void WaWindow::setDecor(long newdecormask) {
    if (! (functions & FunctionSetDecorMask)) return;
    
    if (master) { master->setDecor(newdecormask); return; }

    if (newdecormask == decormask) return;

    decormask = newdecormask;
    
    MERGED_LOOP {
        if (! (_mw->functions & FunctionSetDecorMask)) return;
        
        _mw->decormask = decormask;
        if (decormask & DecorTitleMask) _mw->wstate |= StateDecorTitleMask;
        else _mw->wstate &= ~StateDecorTitleMask;

        if (decormask & DecorBorderMask) _mw->wstate |= StateDecorBorderMask;
        else _mw->wstate &= ~StateDecorBorderMask;

        if (decormask & DecorHandlesMask) _mw->wstate |= StateDecorHandlesMask;
        else _mw->wstate &= ~StateDecorHandlesMask;
        
        if (decormask & DecorAllMask) _mw->wstate |= StateDecorAllMask;
        else _mw->wstate &= ~StateDecorAllMask;
        
        net->setWmState(_mw);
    }
}

void WaWindow::alwaysontopOn(void) {
    if (! (functions & FunctionSetStackingMask)) return;
    
    if (master) { master->alwaysontopOn(); return; }
    if (wstate & StateAlwaysOnTopMask) return;
    
    MERGED_LOOP {
        _mw->wstate &= ~StateAlwaysAtBottomMask;
        _mw->wstate |= StateAlwaysOnTopMask;
        _mw->frame->setStacking(AlwaysOnTopStackingType);
        net->setWmState(_mw);
    }
    ws->restackWindows();
    net->setClientListStacking(ws);
}

void WaWindow::alwaysatbottomOn(void) {
    if (! (functions & FunctionSetStackingMask)) return;
    
    if (master) { master->alwaysatbottomOn(); return; }
    if (wstate & StateAlwaysAtBottomMask) return;

    MERGED_LOOP {
        _mw->wstate &= ~StateAlwaysOnTopMask;
        _mw->wstate |= StateAlwaysAtBottomMask;
        _mw->frame->setStacking(AlwaysAtBottomStackingType);
        net->setWmState(_mw);
    }
    ws->restackWindows();
    net->setClientListStacking(ws);
}

void WaWindow::alwaysontopOff(void) {
    if (master) { master->alwaysontopOff(); return; }
    if (! (wstate & StateAlwaysOnTopMask)) return;

    MERGED_LOOP {
        _mw->wstate &= ~StateAlwaysOnTopMask;
        _mw->frame->setStacking(NormalStackingType);
        net->setWmState(_mw);
    }
    ws->restackWindows();
    net->setClientListStacking(ws);
}

void WaWindow::alwaysatbottomOff(void) {
    if (master) { master->alwaysatbottomOff(); return; }
    if (! (wstate & StateAlwaysAtBottomMask)) return;

    MERGED_LOOP {
        _mw->wstate &= ~StateAlwaysAtBottomMask;
        _mw->frame->setStacking(NormalStackingType);
        net->setWmState(_mw);
    }
    net->setClientListStacking(ws);
}

void WaWindow::alwaysontopToggle(void) {
    if (wstate & StateAlwaysOnTopMask) alwaysontopOff();
    else alwaysontopOn();
}

void WaWindow::alwaysatbottomToggle(void) {
    if (wstate & StateAlwaysAtBottomMask) alwaysatbottomOff();
    else alwaysatbottomOn();
}

void WaWindow::moveResize(char *s) {
    int x, y, geometry;
    unsigned int width, height;

    if (! (functions & FunctionMoveMask)) return;
    if (! (functions & FunctionResizeMask)) return;
    
    if (meawm_ng->eh->move_resize != EndMoveResizeType || ! s) return;

    width = attrib.width;
    height = attrib.height;

    geometry = XParseGeometry(s, &x, &y, &width, &height);
    incSizeCheck(width, height, &attrib.width, &attrib.height);
    
    if (geometry & XValue) {
        if (geometry & XNegative)
            attrib.x = ws->width + x - attrib.width;
        else attrib.x = x;
    }
    if (geometry & YValue) {
        if (geometry & YNegative)
            attrib.y = ws->height + y - attrib.height;
        else attrib.y = y;
    }

    redrawWindow();
    checkMoveMerge(attrib.x, attrib.y);
}

void WaWindow::moveResizeVirtual(char *s) {
    int x, y, geometry;
    unsigned int width, height;

    if (! (functions & FunctionMoveMask)) return;
    if (! (functions & FunctionResizeMask)) return;
    
    if (meawm_ng->eh->move_resize != EndMoveResizeType || ! s) return;
    width = attrib.width;
    height = attrib.height;

    geometry = XParseGeometry(s, &x, &y, &width, &height);
    incSizeCheck(width, height, &attrib.width, &attrib.height);

    gravitate(RemoveGravity);
    if (geometry & XValue) {
        if (geometry & XNegative)
            attrib.x = ((ws->v_xmax + ws->width) +
                        x - attrib.width) - ws->v_x;
        else attrib.x = x - ws->v_x;
    }
    if (geometry & YValue) {
        if (geometry & YNegative)
            attrib.y = ((ws->v_ymax + ws->height) +
                        y - attrib.height) - ws->v_y;
        else attrib.y = y - ws->v_y;
    }
    gravitate(ApplyGravity);

    redrawWindow();
    checkMoveMerge(attrib.x, attrib.y);
}

void WaWindow::moveWindowToPointer(XEvent *e) {
    if (! (functions & FunctionMoveMask)) return;
    
    attrib.x = e->xbutton.x_root - attrib.width / 2;
    attrib.y = e->xbutton.y_root - attrib.height / 2;

    int workx, worky;
    unsigned int workw, workh;
    ws->getWorkareaSize(&workx, &worky, &workw, &workh);
    
    if ((attrib.x + right_spacing + attrib.width) > (workx + workw))
        attrib.x = (workx + workw) - attrib.width - right_spacing;
    else if (attrib.x < workx) attrib.x = workx + left_spacing;
    
    if ((attrib.y + attrib.height) > (worky + workh))
        attrib.y = (worky + workh) - bottom_spacing - attrib.height;
    else if (attrib.y < worky)
        attrib.y = worky + top_spacing;

    redrawWindow();
    checkMoveMerge(attrib.x, attrib.y);
}

void WaWindow::moveWindowToSmartPlace(void) {
    if (! (functions & FunctionMoveMask)) return;
    
    int temp_h, temp_w;
    int workx, worky;
    unsigned int workw, workh;
    ws->getWorkareaSize(&workx, &worky, &workw, &workh);
    int test_x = frame->attrib.x;
    int test_y = frame->attrib.y;
    int loc_ok = false, tw, tx, ty, th;
    temp_h = frame->attrib.height;
    temp_w = frame->attrib.width;
    int first = true;

    while (((test_y + temp_h) < (int) workh) && !loc_ok) {
        if (! first) test_x = 0;
        while (((test_x + temp_w) < (int) workw) && !loc_ok) {
            loc_ok = true;
            list<WaWindow *>::iterator it = ws->wawindow_list.begin();
            for (; it != ws->wawindow_list.end() && (loc_ok == True); it++) {
                if ((*it != this) && ((*it)->wstate & StateTasklistMask) &&
                    (! (*it)->master) &&
                    ((*it)->desktop_mask &
                     (1L << ws->current_desktop->number)) &&
                    ((((*it)->frame->attrib.x + 
                       (*it)->frame->attrib.width) > 0 &&
                      (*it)->frame->attrib.x < (int) workw) && 
                     (((*it)->frame->attrib.y + 
                       (*it)->frame->attrib.height) > 0 &&
                      (*it)->frame->attrib.y < (int) workh))) {
                    
                    th = (*it)->frame->attrib.height;
                    tw = (*it)->frame->attrib.width;

                    tx = (*it)->frame->attrib.x - workx;
                    ty = (*it)->frame->attrib.y - worky;

                    if ((tx < (test_x + temp_w)) &&
                        ((tx + tw) > test_x) &&
                        (ty < (test_y + temp_h)) &&
                        ((ty + th) > test_y)) {
                        loc_ok = False;
                        test_x = tx + tw;
                    }
                }
            }
            test_x += 1;
            
            if ((!loc_ok) && first) {
              first = false;
              test_x = test_y = 0;
            }
        }
        test_y += 1;
    }
    if (loc_ok) {
        attrib.x = left_spacing + test_x + workx - 2;
        attrib.y = top_spacing + test_y + worky - 1;
        redrawWindow();
        checkMoveMerge(attrib.x, attrib.y);
    }
}

void WaWindow::desktopMask(char *s) {
    if (! (functions & FunctionDesktopMask)) return;
    
    if (master) { master->desktopMask(s); return; }
    if (s) {
        if (! strncasecmp("all", s, 3))
            desktop_mask = (1L << 16) - 1;
        else {
            desktop_mask = 0;
            char *token = strtok(s, " \t");
            while (token) {
                unsigned int desk = (unsigned int) atoi(token);
                if (desk < ws->config.desktops)
                    desktop_mask |= (1L << desk);
                token = strtok(NULL, " \t");
            }
        }
        if (desktop_mask == 0) desktop_mask = (1L << 0);
        
        if (desktop_mask & (1L << ws->current_desktop->number))
            show();
        else
            hide();

        MERGED_LOOP {
            _mw->desktop_mask = desktop_mask;
            net->setDesktop(_mw);
            net->setDesktopMask(_mw);
        }
    }
}

void WaWindow::joinDesktop(char *s) {
    if (! (functions & FunctionDesktopMask)) return;
    
    if (master) { master->joinDesktop(s); return; }
    if (s) {
        unsigned int desk = (unsigned int) atoi(s);
        if (desk < ws->config.desktops) {
            desktop_mask |= (1L << desk);
            if (desktop_mask & (1L << ws->current_desktop->number))
                show();
            MERGED_LOOP {
                _mw->desktop_mask = desktop_mask;
                net->setDesktop(_mw);
                net->setDesktopMask(_mw);
            }
        }
    }
}

void WaWindow::partDesktop(char *s) {
    if (! (functions & FunctionDesktopMask)) return;
    
    if (master) { master->partDesktop(s); return; }
    if (s) {
        unsigned int desk = (unsigned int) atoi(s);
        if (desk < ws->config.desktops) {
            long int new_mask = desktop_mask & ~(1L << desk);
            if (new_mask) {
                desktop_mask = new_mask;
                if (! (desktop_mask &
                       (1L << ws->current_desktop->number))) 
                    hide();
                MERGED_LOOP {
                    _mw->desktop_mask = desktop_mask;
                    net->setDesktop(_mw);
                    net->setDesktopMask(_mw);
                }
            }
        }
    }
}

void WaWindow::partCurrentDesktop(void) {
    if (! (functions & FunctionDesktopMask)) return;
    
    if (master) { master->partCurrentDesktop(); return; }
    long int new_mask = desktop_mask &
        ~(1L << ws->current_desktop->number);
    if (new_mask) {
        desktop_mask = new_mask;
        hide();
        MERGED_LOOP {
            _mw->desktop_mask = desktop_mask;
            net->setDesktop(_mw);
            net->setDesktopMask(_mw);
        }
    }
}

void WaWindow::joinCurrentDesktop(void) {
    if (! (functions & FunctionDesktopMask)) return;
    
    if (master) { master->joinCurrentDesktop(); return; }
    desktop_mask |= (1L << ws->current_desktop->number);
    show();
    MERGED_LOOP {
        _mw->desktop_mask = desktop_mask;
        net->setDesktop(_mw);
        net->setDesktopMask(_mw);
    }
}

void WaWindow::joinAllDesktops(void) {
    if (! (functions & FunctionDesktopMask)) return;
    
    if (master) { master->joinAllDesktops(); return; }
    desktop_mask = (1L << 16) - 1;
    show();
    MERGED_LOOP {
        _mw->desktop_mask = desktop_mask;
        net->setDesktop(_mw);
        net->setDesktopMask(_mw);
    }
}

void WaWindow::partAllDesktopsExceptCurrent(void) {
    if (! (functions & FunctionDesktopMask)) return;
    
    desktop_mask = (1L << ws->current_desktop->number);
    show();
    MERGED_LOOP {
        _mw->desktop_mask = desktop_mask;
        net->setDesktop(_mw);
        net->setDesktopMask(_mw);
    }
}

void WaWindow::partCurrentJoinDesktop(char *s) {
    if (! (functions & FunctionDesktopMask)) return;
    
    if (master) { master->partCurrentJoinDesktop(s); return; }
    if (s) {
        unsigned int desk = (unsigned int) atoi(s);
        if (desk < ws->config.desktops) {
            desktop_mask = desktop_mask &
                ~(1L << ws->current_desktop->number);
            desktop_mask |= (1L << desk);
            if (desktop_mask & (1L << ws->current_desktop->number))
                show();
            else hide();
            MERGED_LOOP {
                _mw->desktop_mask = desktop_mask;
                net->setDesktop(_mw);
                net->setDesktopMask(_mw);
            }
        }
    }
}

void WaWindow::merge(WaWindow *child, int mtype) {
    if (! (functions & FunctionMergeMask)) return;
    
    if (mtype != CloneMergeType && mtype != VertMergeType &&
        mtype != HorizMergeType) return;
    if (child->master) child->master->unmerge(child);
    if (child == this) return;
    if (! child->merged.empty()) return;
    if (master) return;

    bool had_focus = child->has_focus;
        
    wa_grab_server();
    if (validate_drawable(id)) {
        XSelectInput(display, child->id, NoEventMask);
        XReparentWindow(display, child->id, frame->id,
                        -child->attrib.width, -child->attrib.height);
        XSelectInput(display, child->id, PropertyChangeMask |
                     StructureNotifyMask | FocusChangeMask |
                     EnterWindowMask | LeaveWindowMask);
    } else {
        wa_ungrab_server();
        return;
    }
    wa_ungrab_server();

    merged.push_back(child);

    child->master = this;
    child->mergetype = mtype;
    child->hide();

    /* merge decorations */
    
    if (mtype == CloneMergeType) {
        child->mergedback = true;
        if (meawm_ng->eh) child->toFront();
    }

    updateAllAttributes();
    if (! meawm_ng->eh) {
        net->getMergeOrder(this);
        net->getMergeAtfront(this);
    }
    net->setMergedState(child);
    net->setMergeOrder(this);

    if (had_focus) {
        child->has_focus = false;
        meawm_ng->focusNew(child->id, false);
    }
}

void WaWindow::unmerge(WaWindow *child) {
    list<WaWindow *>::iterator it = merged.begin();
    for (; it != merged.end() && *it != child; it++);

    if (it == merged.end()) return;

    bool had_focus = child->has_focus;

    wa_grab_server();
    if (validate_drawable(child->id)) {
        XSelectInput(display, child->id, NoEventMask);
        XReparentWindow(display, child->id, child->frame->id, 0,
                        child->top_spacing);
        XLowerWindow(display, child->id);
        XSelectInput(display, child->id, PropertyChangeMask |
                     StructureNotifyMask | FocusChangeMask |
                     EnterWindowMask | LeaveWindowMask);
    }
    wa_ungrab_server();

    merged.remove(child);

    /* merge decorations */

    if (child->mergetype == CloneMergeType && !child->mergedback)
        toFront();

    updateAllAttributes();   

    child->master = NULL;
    child->mergetype = NullMergeType;
    if (child->desktop_mask & (1L << ws->current_desktop->number)) {
        child->hidden = true;
        child->show();
    }
    child->updateAllAttributes();
    
    if (! ws->shutdown) {
        net->setMergedState(child);
        net->setMergeOrder(this);
    }
    
    if (had_focus) {
        meawm_ng->focusNew(child->id, false);
    } else
        child->has_focus = false;
}

void WaWindow::mergeWithWindow(char *s, int mtype) {
    if (! s) return;
    if (! (functions & FunctionMergeMask)) return;

    char *str = WA_STRDUP(s);
    WaWindow *mw = NULL;
    list<AWindowObject *> awos;
    Tst<char *> *attr;
    Doing *doing = new Doing(ws);
    
    attr = short_do_string_to_tst(str);

    doing->applyAttributes(NULL, attr);

    delete attr;

    ws->getRegexTargets(doing->wreg, WindowType, false, &awos);
    if (! awos.empty()) {
        mw = (WaWindow *) *awos.begin();
        awos.clear();
    }

    delete doing;
    delete [] str;
    
    if (mw && mw != this)
        mw->merge(this, mtype);
}

void WaWindow::explode(void) {
    while (! merged.empty())
        unmerge(merged.back());
}

void WaWindow::setMergeMode(char *s) {
    if (s) {
        if (! strncasecmp(s, "vert", 4))
            mergemode = VertMergeType;
        else if (! strncasecmp(s, "horiz", 5))
            mergemode = HorizMergeType;
        else if (! strncasecmp(s, "clone", 5))
            mergemode = CloneMergeType;
        else
            mergemode = NullMergeType;
    }
}

void WaWindow::nextMergeMode(void) {
    if (mergemode == VertMergeType)
        mergemode = NullMergeType;
    else mergemode++;
}

void WaWindow::prevMergeMode(void) {
    if (mergemode == NullMergeType)
        mergemode = VertMergeType;
    else mergemode--;
}

void WaWindow::mergeTo(XEvent *e, int mtype) {
    WaWindow *wt = (WaWindow *) meawm_ng->findWin(e->xany.window, WindowType);
    if (wt)
        merge(wt, mtype);
}

void WaWindow::toFront(void) {
    if (mergedback) {
        list<WaWindow *>::iterator it = merged.begin();
        list<WaWindow *>::iterator it_end = merged.end();
        if (master) {
            it = master->merged.begin();
            it_end = master->merged.end();
            XLowerWindow(display, master->id);
        }   
        for (; it != it_end; it++) {
            if ((*it)->id != id)
                XLowerWindow(display, (*it)->id);
        }
        
        int focused = false;
        if (master) {
            net->setMergeAtfront(master, id);
            master->mergedback = true;
            it = master->merged.begin();
            for (; it != master->merged.end(); it++) {
                if ((*it)->mergetype == CloneMergeType) {
                    if ((*it)->has_focus) focused = true;
                    (*it)->mergedback = true;
                }
            }
        } else {
            net->setMergeAtfront(this, id);
            it = merged.begin();
            for (; it != merged.end(); it++) {
                if ((*it)->mergetype == CloneMergeType) {
                    if ((*it)->has_focus) focused = true;
                    (*it)->mergedback = true;
                }
            }
        }        
        mergedback = false;        

        if (focused) meawm_ng->focusNew(id, false);
    }
}

bool WaWindow::checkMoveMerge(int x, int y, int width, int height) {
    if (! merged.empty()) return false;
    if (! (functions & FunctionMergeMask)) return false;
    
    list<WaWindow *> matchlist;
    WaWindow *bestmatch = NULL;

    if (width == 0) width = attrib.width;
    if (height == 0) height = attrib.height;
    int _x = x + width / 2;
    int _y = y + height / 2;

    if (mergemode == NullMergeType && master) {
        attrib.x = x;
        attrib.y = y;
        old_attrib.width = attrib.width;
        old_attrib.height = attrib.height;
        attrib.width = width;
        attrib.height = height;
        master->unmerge(this);
        return true;
    }

    list<WaWindow *>::iterator it = ws->wawindow_list.begin();
    for (; it != ws->wawindow_list.end(); it++) {     
        if (*it != this && (! (*it)->master) && (! (*it)->hidden) &&
            (! ((*it)->wstate & StateShadedMask)) &&
            ((*it)->wstate & StateTasklistMask)) {
            if (_x > (*it)->frame->attrib.x &&
                _x < ((*it)->frame->attrib.x +
                      (int) (*it)->frame->attrib.width) &&
                _y > (*it)->frame->attrib.y &&
                _y < ((*it)->frame->attrib.y +
                      (int) (*it)->frame->attrib.height)) {
                matchlist.push_back(*it);
            }
        }
    }
    if (! matchlist.empty()) {
        if (matchlist.size() > 1) {
            list<Window>::iterator wit = ws->aot_stacking_list.begin();
            for (; !bestmatch &&
                     wit != ws->aot_stacking_list.end(); wit++) {
                for (it = matchlist.begin(); it != matchlist.end(); it++)
                    if ((*it)->frame->id == *wit) {
                        bestmatch = *it;
                        break;
                    }
            }
            wit = ws->stacking_list.begin();
            for (; !bestmatch &&
                     wit != ws->stacking_list.end(); wit++) {
                for (it = matchlist.begin(); it != matchlist.end(); it++)
                    if ((*it)->frame->id == *wit) {
                        bestmatch = *it;
                        break;
                    }
            }
            wit = ws->aab_stacking_list.begin();
            for (; !bestmatch &&
                     wit != ws->aab_stacking_list.end(); wit++) {
                for (it = matchlist.begin(); it != matchlist.end(); it++)
                    if ((*it)->frame->id == *wit) {
                        bestmatch = *it;
                        break;
                    }
            }
        }
        else {
            bestmatch = matchlist.front();
        }
    }

    LISTCLEAR(matchlist);

    if (bestmatch) {
        if (master != bestmatch) {
            bestmatch->merge(this, mergemode);
            return true;
        } else if (mergemode != mergetype) {
            bestmatch->merge(this, mergemode);
            return true;
        }
    }
    else if (master) {
        if ((_x + 25) > master->frame->attrib.x &&
            _x < (master->frame->attrib.x +
                  (int) master->frame->attrib.width + 25) &&
            (_y + 25) > master->frame->attrib.y &&
            _y < (master->frame->attrib.y +
                  (int) master->frame->attrib.height) + 25) {
            return false;
        }
        attrib.x = x;
        attrib.y = y;
        old_attrib.width = attrib.width;
        old_attrib.height = attrib.height;
        attrib.width = width;
        attrib.height = height;
        master->unmerge(this);
        return true;
    }
    return false;
}

void WaWindow::windowStateCheck(bool force) {
    if (! init_done) return;
    
    if (force || wstate != old_wstate) {
        EventDetail ed;
        ed.x = ed.y = INT_MAX;
        ed.x11mod = ed.wamod = 0;
        ed.type = WindowStateChangeNotify;
        ws->meawm_ng->eh->evAct(NULL, frame->id, &ed);
        
        signalMonitors();

        old_wstate = wstate;
    }
}

void WaWindow::setWindowState(long int newstate) {
    if (newstate != wstate) {
        wstate = newstate;
        windowStateCheck();
    }
}

void WaWindow::addMonitor(AWindowObject *monitor, long int type) {
    EventDetail ed;
    ed.x = ed.y = INT_MAX;
    ed.x11mod = ed.wamod = 0;

    monitors.push_back(new FlagMonitor(monitor, type));

    if (type) {
        if (wstate & type) ed.type = StateAddNotify;
        else ed.type = StateRemoveNotify;
    
        ws->meawm_ng->eh->evAct(NULL, monitors.back()->monitor->id, &ed);
    }
}

void WaWindow::removeMonitor(AWindowObject *monitor) {
    list<FlagMonitor *>::iterator it = monitors.begin();
    for (; it != monitors.end(); it++)
        if ((*it)->monitor == monitor)
            it = monitors.erase(it);
}

void WaWindow::signalMonitors(void) {
    EventDetail ed;
    ed.x = ed.y = INT_MAX;
    ed.x11mod = ed.wamod = 0;
    
    list<FlagMonitor *>::iterator it = monitors.begin();
    for (; it != monitors.end(); it++) {
        ed.type = WindowStateChangeNotify;
        ws->meawm_ng->eh->evAct(NULL, (*it)->monitor->id, &ed);

        if ((*it)->type) {
            if ((old_wstate & (*it)->type) != (wstate & (*it)->type)) {
                
                if (wstate & (*it)->type) ed.type = StateAddNotify;
                else ed.type = StateRemoveNotify;
            
                ws->meawm_ng->eh->evAct(NULL, (*it)->monitor->id, &ed);
            }
        }
    }
}

WaFrameWindow::WaFrameWindow(WaWindow *wa_win, WaStringMap *sm) :
    RootWindowObject(wa_win->ws, 0, WindowFrameType, sm, "frame") {
    XSetWindowAttributes attrib_set;
    
    wa = wa_win;

    int create_mask = CWOverrideRedirect | CWEventMask | CWColormap;

    if (ws->screen_depth == 32) {
        attrib_set.background_pixmap = None;
        attrib_set.background_pixel = 0x00000000;
        attrib_set.border_pixel = 0x00000000;
        create_mask |= CWBackPixel | CWBorderPixel;
    } else {
        attrib_set.background_pixmap = ParentRelative;
    }

    create_mask |= CWBackPixmap;
    
    attrib_set.colormap = ws->colormap;
    attrib_set.override_redirect = true;
    attrib_set.event_mask = SubstructureRedirectMask |
      StructureNotifyMask | SubstructureNotifyMask |
      EnterWindowMask | LeaveWindowMask;

    attrib.x = wa->attrib.x;
    attrib.y = wa->attrib.y;
    attrib.width = wa->attrib.width;
    attrib.height = wa->attrib.height;
    
    id = XCreateWindow(ws->display, ws->id, attrib.x, attrib.y,
	   attrib.width, attrib.height, 0, ws->screen_depth,
	   InputOutput, ws->visual, create_mask,
	   &attrib_set);
    
#ifdef SHAPE
    shape_count = 0;
    apply_shape = false;
    apply_shape_buffer = XCreateWindow(ws->display, ws->id, 0, 0,
	   attrib.width, attrib.height, 0,
	   CopyFromParent, CopyFromParent,
	   CopyFromParent, CWOverrideRedirect,
	   &attrib_set);
#endif // SHAPE
    
    wa->meawm_ng->window_table.insert(make_pair(id, this));
}

WaFrameWindow::~WaFrameWindow(void) {

#ifdef    SHAPE
    while (! shape_infos.empty()) {
        shape_infos.back()->in_list = false;
        shape_infos.pop_back();
    }
    XDestroyWindow(ws->display, apply_shape_buffer);
#endif // SHAPE
    
    wa->meawm_ng->window_table.erase(id);
}

void WaFrameWindow::styleUpdate(bool, bool size_change) {
    if (size_change)
        wa->updateAllAttributes();
    else
        wa->drawDecor();
}

#ifdef    SHAPE
void WaFrameWindow::startRender(void) {
    apply_shape = false;
}

void WaFrameWindow::endRender(Pixmap) {
    apply_shape = true;
    applyShape();
}

void WaFrameWindow::addShapeInfo(ShapeInfo *sinfo) {
    shape_infos.push_back(sinfo);
}

void WaFrameWindow::removeShapeInfo(ShapeInfo *sinfo) {
    shape_infos.remove(sinfo);
    shapeUpdateNotify();
}

void WaFrameWindow::shapeUpdateNotify(void) {
    shape_count++;
    applyShape();
}

void WaFrameWindow::applyShape(void) {
    if (! apply_shape) return;
    if (! shape_count) return;
    
    XShapeCombineMask(ws->display, apply_shape_buffer, ShapeBounding,
                      0, 0, None, ShapeSet);

    XRectangle rect;
    rect.x = rect.y = 0;
    rect.width = attrib.width;
    rect.height = attrib.height;
    
    XShapeCombineRectangles(ws->display, apply_shape_buffer, ShapeBounding,
                            0, 0, &rect, 1, ShapeSubtract, Unsorted);

    list<ShapeInfo *>::iterator it = shape_infos.begin();
    for (; it != shape_infos.end(); it++)
        XShapeCombineShape(ws->display, apply_shape_buffer, ShapeBounding,
                           (*it)->xoff, (*it)->yoff, (*it)->window,
                           ShapeBounding, ShapeUnion);
    
    XShapeCombineShape(ws->display, id, ShapeBounding,
                       0, 0, apply_shape_buffer, ShapeBounding, ShapeSet);
    shape_count = 0;
}

ShapeInfo::ShapeInfo(Window _window) {
    xoff = 0;
    yoff = 0;
    in_list = false;
    window = _window;
}

void ShapeInfo::setShapeOffset(int newxoff, int newyoff) {
    xoff = newxoff;
    yoff = newyoff;
}
#endif // SHAPE
