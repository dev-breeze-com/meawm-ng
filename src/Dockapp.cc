/* Dockapp.cc

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
#include <X11/Xatom.h>
}

#include "Dockapp.hh"

DockappHandler::DockappHandler(WaScreen *scrn, char *_name) :
    RootWindowObject(scrn, 0, DockHandlerType,
                     new WaStringMap(WindowIDName, _name), "frame") {
    XSetWindowAttributes attrib_set;
    
    hidden = true;
    x = 0;
    y = 0;
    width = 0;
    height = 0;
    desktop_mask = (1L << 16) - 1;
    inworkspace = false;
    name = WA_STRDUP(_name);

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

    attrib_set.background_pixmap = ParentRelative;
    attrib_set.colormap = ws->colormap;
    attrib_set.override_redirect = true;
    attrib_set.event_mask = SubstructureRedirectMask | ButtonPressMask |
        ButtonReleaseMask | EnterWindowMask | LeaveWindowMask;
    
    id = XCreateWindow(ws->display, ws->id, 0, 0, 1, 1, 0, ws->screen_depth,
                       CopyFromParent, ws->visual, create_mask,
                       &attrib_set);

    wm_strut = new WMstrut;
    wm_strut->window = id;
    wm_strut->left = 0;
    wm_strut->right = 0;
    wm_strut->top = 0;
    wm_strut->bottom = 0;
    ws->strut_list.push_back(wm_strut);
    
    ws->meawm_ng->window_table.insert(make_pair(id, this));

    setStacking(ws->config.dock_stacking);
}

DockappHandler::~DockappHandler(void) {
    ws->docks.remove(this);
    
    LISTUNREF(dockapp_list);
    
    ws->strut_list.remove(wm_strut);
    delete wm_strut;
    
    ws->meawm_ng->window_table.erase(id);
    
    delete [] name;
}

void DockappHandler::update(void) {
    if (dockapp_list.empty()) {
        unref();
        return;
    }

    width = height = 0;
    double _top_spacing, _bottom_spacing, _left_spacing, _right_spacing,
        _grid_spacing;
    unsigned int top_spacing, bottom_spacing, left_spacing, right_spacing,
        grid_spacing;

    calc_length(style->top_spacing, style->top_spacing_u,
                ws->vdpi, ws->height, &_top_spacing);
    calc_length(style->bottom_spacing, style->bottom_spacing_u,
                ws->vdpi, ws->height, &_bottom_spacing);
    calc_length(style->left_spacing, style->left_spacing_u,
                ws->hdpi, ws->width, &_left_spacing);
    calc_length(style->right_spacing, style->right_spacing_u,
                ws->hdpi, ws->width, &_right_spacing);
    calc_length(style->grid_spacing, style->grid_spacing_u,
                ws->hdpi, ws->width, &_grid_spacing);

    switch (style->orientation) {
        case VerticalOrientationType:
            calc_length(style->grid_spacing, style->grid_spacing_u,
                        ws->vdpi, ws->height, &_grid_spacing);
            break;
        case HorizontalOrientationType:
            calc_length(style->grid_spacing, style->grid_spacing_u,
                        ws->hdpi, ws->width, &_grid_spacing);
            break;
    }

    top_spacing = WA_ROUND_U(_top_spacing);
    bottom_spacing = WA_ROUND_U(_bottom_spacing);
    left_spacing = WA_ROUND_U(_left_spacing);
    right_spacing = WA_ROUND_U(_right_spacing);
    grid_spacing = WA_ROUND_U(_grid_spacing);
    
    list<Dockapp *>::iterator it = dockapp_list.begin();
    for (; it != dockapp_list.end(); ++it) {
        switch (style->orientation) {
            case VerticalOrientationType:
                if ((*it)->width > width) width = (*it)->width;
                break;
            case HorizontalOrientationType:
                if ((*it)->height > height) height = (*it)->height;
                break;
        }
    }

    switch (style->orientation) {
        case VerticalOrientationType:
            width += left_spacing + right_spacing;
            height = top_spacing;
            break;
        case HorizontalOrientationType:
            height += top_spacing + bottom_spacing;
            width = left_spacing;
            break;
    }
    
    it = dockapp_list.begin();
    for (; it != dockapp_list.end(); ++it) {
        int dock_x, dock_y;
        switch (style->orientation) {
            case VerticalOrientationType:
                dock_y = height;
                height += ((*it)->height + grid_spacing);
                dock_x = width / 2 - (*it)->width / 2;
                break;
            case HorizontalOrientationType:
                dock_x = width;
                width += ((*it)->width + grid_spacing);
                dock_y = height / 2 - (*it)->height / 2;
                break;
        }
        (*it)->x = dock_x;
        (*it)->y = dock_y;
        XMoveWindow(ws->display, (*it)->id, dock_x, dock_y);
    }

    switch (style->orientation) {
        case VerticalOrientationType:
            height -= grid_spacing;
            height += bottom_spacing;
            break;
        case HorizontalOrientationType:
            width -= grid_spacing;
            width += right_spacing;
            break;
    }

    if ((! width) || (! height)) return;
    
    double map_x, map_y;
    style->calcPositionFromSize(ws->width, ws->height, width, height,
                                PXLenghtUnitType, PXLenghtUnitType,
                                ws->hdpi, ws->vdpi, &map_x, &map_y);
    x = (int) map_x;
    y = (int) map_y;
    
    XMoveResizeWindow(ws->display, id, x, y, width, height);

    wm_strut->left = wm_strut->right = wm_strut->top =
        wm_strut->bottom = 0;
    
    if (! inworkspace) {
        if (style->orientation == HorizontalOrientationType) {
            switch (style->gravity) {
                case NorthWestGravity:
                case NorthGravity:
                case NorthEastGravity:
                    wm_strut->top = y + height;
                    break;
                case SouthWestGravity:
                case SouthGravity:
                case SouthEastGravity:
                    wm_strut->bottom = ws->height - y;
                    break;
                default:
                    break;
                    
            }
        } else {
            switch (style->gravity) {
                case NorthWestGravity:
                case SouthWestGravity:
                case WestGravity:
                    wm_strut->left = x + width;
                    break;
                case NorthEastGravity:
                case SouthEastGravity:
                case EastGravity:
                    wm_strut->right = ws->width - x;
                    break;
                default:
                    break;
            }
        }
    }
    
    if (desktop_mask & (1L << ws->current_desktop->number)) {
        mapRequest = true;
        hidden = false;
        pushRenderEvent();
        ws->updateWorkarea();
    }
}

void DockappHandler::styleUpdate(bool pos_change, bool size_change) {
    if (pos_change || size_change)
        update();
    else
        pushRenderEvent();
}

Dockapp::Dockapp(WaScreen *_ws, Window _id) :
    AWindowObject(NULL, _id, DockAppType, NULL, "client") {
    char *host, *wclass, *wclassname;
    int status, n;
    char **cl;
    XTextProperty text_prop;
    XSetWindowAttributes attrib_set;
    XWindowAttributes attrib;
    
    name = WA_STRDUP("");
    wclass = WA_STRDUP("");
    wclassname = WA_STRDUP("");
    host = WA_STRDUP("");
    dh = NULL;
    client_id = _id;
    deleted = false;
    prio = 0;
    prio_set = false;
    status = 0;
    ws = _ws;
    
    XWMHints *wmhints = XGetWMHints(ws->display, _id);
    if (wmhints) {
        if ((wmhints->flags & IconWindowHint) &&
            (wmhints->icon_window != None)) {
            XUnmapWindow(ws->display, client_id);
            icon_id = wmhints->icon_window;
            id = icon_id;
        } else {
            icon_id = None;
            id = client_id;
        }
        XFree(wmhints);
    } else {
        icon_id = None;
        id = client_id;
    }

    attrib_set.event_mask = StructureNotifyMask |
        FocusChangeMask | EnterWindowMask | LeaveWindowMask;
    attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask;
    
    XChangeWindowAttributes(ws->display, id, CWEventMask | CWDontPropagate,
                            &attrib_set);
    XSetWindowBorderWidth(ws->display, id, 0);

    if (XGetWindowAttributes(ws->display, id, &attrib)) {
        width = attrib.width;
        height = attrib.height;
    } else
        width = height = 64;

    status = XGetWMName(ws->display, client_id, &text_prop);
    
    if (status) {
	    if (text_prop.encoding == XA_STRING)
            name = wa_locale_to_utf8((char *) text_prop.value);
        else {
            
#ifndef   X_HAVE_UTF8_STRING
#  define Xutf8TextPropertyToTextList XmbTextPropertyToTextList
#endif // !X_HAVE_UTF8_STRING

            Xutf8TextPropertyToTextList(ws->display, &text_prop, &cl, &n);
            if (cl) {
                name = WA_STRDUP(cl[0]);
                XFreeStringList(cl);
            }
        }
    }
    
    status = 0;

    status = XGetWMClientMachine(ws->display, client_id, &text_prop);
    
    if (status) {
        if (text_prop.encoding == XA_STRING)
            host = wa_locale_to_utf8((char *) text_prop.value);
        else {
            
#ifndef   X_HAVE_UTF8_STRING
#  define Xutf8TextPropertyToTextList XmbTextPropertyToTextList
#endif // !X_HAVE_UTF8_STRING
            
            Xutf8TextPropertyToTextList(ws->display, &text_prop, &cl, &n);
            if (cl) {
                host = WA_STRDUP(cl[0]);
                XFreeStringList(cl);
            }
        }
    }

    XClassHint *classhint = XAllocClassHint();
    status = 0;

    status = XGetClassHint(ws->display, client_id, classhint);
    
    if (status) {
        if (classhint->res_class) {
            wclass = wa_locale_to_utf8((char *) classhint->res_class);
            XFree(classhint->res_class);
        }
        if (classhint->res_name) {            
            wclassname = wa_locale_to_utf8((char *) classhint->res_name);
            XFree(classhint->res_name);
        }
    }
    XFree(classhint);

    char window_id_str[32];
    snprintf(window_id_str, 32, "0x%lx", client_id);

    WaStringMap *sm = new WaStringMap();
    sm->add(WindowIDName, name);
    sm->add(WindowIDClassName, wclassname);
    sm->add(WindowIDClass, wclass);
    sm->add(WindowIDHost, host);
    sm->add(WindowIDWinID, window_id_str);
    resetActionList(sm);
    
    delete [] wclassname;
    delete [] wclass;
    delete [] host;
    
    ws->meawm_ng->window_table.insert(make_pair(id, this));
}

Dockapp::~Dockapp(void) {
    ws->meawm_ng->window_table.erase(id);
    if (! deleted) {
        if (validate_window_mapped(id)) {
            XChangeSaveSet(ws->display, id, SetModeDelete);
            XSelectInput(ws->display, id, NoEventMask);
            if (icon_id) XUnmapWindow(ws->display, id);
            XReparentWindow(ws->display, id, ws->id, 0, 0);
            XMapWindow(ws->display, client_id);
        }
    }
    if (name) delete [] name;
}
