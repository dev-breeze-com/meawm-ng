/* Net.cc

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

#ifdef    LIMITS_H
#  include <limits.h>
#endif // LIMITS_H
    
}

#include "Net.hh"

NetHandler::NetHandler(Meawm_NG *wa) {
    meawm_ng = wa;
    display = meawm_ng->display;
    size_hints = XAllocSizeHints();
    classhint = XAllocClassHint();
    
    utf8_string = XInternAtom(display, "UTF8_STRING", false);

    wm_protocols = XInternAtom(display, "WM_PROTOCOLS", false);
    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", false);
    wm_take_focus = XInternAtom(display, "WM_TAKE_FOCUS", false);
    wm_state = XInternAtom(display, "WM_STATE", false);
    wm_change_state = XInternAtom(display, "WM_CHANGE_STATE", false);

    mwm_hints = XInternAtom(display, "_MOTIF_WM_HINTS", false);
 
    net_supported = XInternAtom(display, "_NET_SUPPORTED", false);
    net_supported_wm_check =
        XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", false);

    net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", false);
    net_client_list_stacking =
        XInternAtom(display, "_NET_CLIENT_LIST_STACKING", false);
    net_active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", false);

    net_desktop_viewport =
        XInternAtom(display, "_NET_DESKTOP_VIEWPORT", false);
    net_desktop_geometry =
        XInternAtom(display, "_NET_DESKTOP_GEOMETRY", false);
    net_current_desktop = XInternAtom(display, "_NET_CURRENT_DESKTOP", false);
    net_number_of_desktops =
        XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", false);
    net_desktop_names = XInternAtom(display, "_NET_DESKTOP_NAMES", false);
    net_workarea = XInternAtom(display, "_NET_WORKAREA", false);
    
    net_wm_desktop = XInternAtom(display, "_NET_WM_DESKTOP", false);
    net_wm_name = XInternAtom(display, "_NET_WM_NAME", false);
    net_wm_visible_name = XInternAtom(display, "_NET_WM_VISIBLE_NAME", false);
    net_wm_strut = XInternAtom(display, "_NET_WM_STRUT", false);
    net_wm_strut_partial = XInternAtom(display, "_NET_WM_STRUT_PARTIAL",
                                       false);
    net_wm_pid = XInternAtom(display, "_NET_WM_PID", false);
    net_wm_user_time = XInternAtom(display, "_NET_WM_USER_TIME", false);
    
    net_wm_state = XInternAtom(display, "_NET_WM_STATE", false);
    net_wm_state_sticky = XInternAtom(display, "_NET_WM_STATE_STICKY", false);
    net_wm_state_shaded = XInternAtom(display, "_NET_WM_STATE_SHADED", false);
    net_wm_state_hidden = XInternAtom(display, "_NET_WM_STATE_HIDDEN", false);
    net_wm_maximized_vert =
        XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", false);
    net_wm_maximized_horz =
        XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
    net_wm_state_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", false);
    net_wm_state_below = XInternAtom(display, "_NET_WM_STATE_BELOW", false);
    net_wm_state_stays_on_top =
        XInternAtom(display, "_NET_WM_STATE_STAYS_ON_TOP", false);
    net_wm_state_stays_at_bottom =
        XInternAtom(display, "_NET_WM_STATE_STAYS_AT_BOTTOM", false);
    net_wm_state_fullscreen =
        XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", false);
    net_wm_state_skip_taskbar =
        XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", false);

    net_wm_allowed_actions =
        XInternAtom(display, "_NET_WM_ALLOWED_ACTIONS", false);
    net_wm_action_move = XInternAtom(display, "_NET_WM_ACTION_MOVE", false);
    net_wm_action_resize =
        XInternAtom(display, "_NET_WM_ACTION_RESIZE", false);
    net_wm_action_minimize =
        XInternAtom(display, "_NET_WM_ACTION_MINIMIZE", false);
    net_wm_action_shade = XInternAtom(display, "_NET_WM_ACTION_SHADE", false);
    net_wm_action_stick = XInternAtom(display, "_NET_WM_ACTION_STICK", false);
    net_wm_action_maximize_horz =
        XInternAtom(display, "_NET_WM_ACTION_MAXIMIZE_HORZ", false);
    net_wm_action_maximize_vert =
        XInternAtom(display, "_NET_WM_ACTION_MAXIMIZE_VERT", false);
    net_wm_action_fullscreen =
        XInternAtom(display, "_NET_WM_ACTION_FULLSCREEN", false);
    net_wm_action_change_desktop =
        XInternAtom(display, "_NET_WM_ACTION_CHANGE_DESKTOP", false);
    net_wm_action_close =
        XInternAtom(display, "_NET_WM_ACTION_CLOSE", false);

    net_wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", false);
    net_wm_window_type_desktop =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", false);
    net_wm_window_type_dock =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", false);
    net_wm_window_type_toolbar =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", false);
    net_wm_window_type_menu =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_MENU", false);
    net_wm_window_type_splash =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", false);
    net_wm_window_type_normal =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", false);
    net_wm_window_type_dialog =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", false);
    net_wm_window_type_utility =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", false);

    net_close_window = XInternAtom(display, "_NET_CLOSE_WINDOW", false);
    net_moveresize_window =
        XInternAtom(display, "_NET_MOVERESIZE_WINDOW", false);
    net_wm_moveresize = XInternAtom(display, "_NET_WM_MOVERESIZE", false);

    net_wm_icon = XInternAtom(display, "_NET_WM_ICON", false);
    net_wm_icon_image = XInternAtom(display, "_NET_WM_ICON_IMAGE", false);
    net_wm_icon_svg = XInternAtom(display, "_NET_WM_ICON_SVG", false);
    
    meawm_ng_net_wm_state_decor =
        XInternAtom(display, "_WAIMEA_NET_WM_STATE_DECOR", false);
    meawm_ng_net_wm_state_decortitle =
        XInternAtom(display, "_WAIMEA_NET_WM_STATE_DECOR_TITLE", false);
    meawm_ng_net_wm_state_decorborder =
        XInternAtom(display, "_WAIMEA_NET_WM_STATE_DECOR_BORDER", false);
    meawm_ng_net_wm_state_decorhandles =
        XInternAtom(display, "_WAIMEA_NET_WM_STATE_DECOR_HANDLES", false);    
    
    meawm_ng_net_maximized_restore =
        XInternAtom(display, "_WAIMEA_NET_MAXIMIZED_RESTORE", false);
    meawm_ng_net_virtual_pos =
        XInternAtom(display, "_WAIMEA_NET_VIRTUAL_POS", false);
    meawm_ng_net_wm_desktop_mask =
        XInternAtom(display, "_WAIMEA_NET_WM_DESKTOP_MASK", false);
    
    meawm_ng_net_wm_merged_to =
        XInternAtom(display, "_WAIMEA_NET_WM_MERGED_TO", false);
    meawm_ng_net_wm_merged_type =
        XInternAtom(display, "_WAIMEA_NET_WM_MERGED_TYPE", false);
    meawm_ng_net_wm_merge_order =
        XInternAtom(display, "_WAIMEA_NET_WM_MERGE_ORDER", false);
    meawm_ng_net_wm_merge_atfront =
        XInternAtom(display, "_WAIMEA_NET_WM_MERGE_ATFRONT", false);

    meawm_ng_net_restart = XInternAtom(display, "_WAIMEA_NET_RESTART", false);
    meawm_ng_net_shutdown = XInternAtom(display, "_WAIMEA_NET_SHUTDOWN", false);
    
    xdndaware = XInternAtom(display, "XdndAware", false);
    xdndenter = XInternAtom(display, "XdndEnter", false);
    xdndleave = XInternAtom(display, "XdndLeave", false);

    kde_net_system_tray_windows =
        XInternAtom(display, "_KDE_NET_SYSTEM_TRAY_WINDOWS", false);
    kde_net_wm_system_tray_window_for =
        XInternAtom(display, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", false);
    
    xrootpmap_id = XInternAtom(display, "_XROOTPMAP_ID", false);
	xsetroot_id = XInternAtom(display, "_XSETROOT_ID", false);

    meawm_ng_net_event_notify =
        XInternAtom(display, "_WAIMEA_NET_EVENT_NOTIFY", false);

    meawm_ng_net_dockapp_holder =
        XInternAtom(display, "_WAIMEA_NET_DOCKAPP_HOLDER", false);
    meawm_ng_net_dockapp_prio =
        XInternAtom(display, "_WAIMEA_NET_DOCKAPP_PRIO", false);

    meawm_ng_net_cfg = XInternAtom(display, "_WAIMEA_NET_CFG", false);

    cme.type = ClientMessage;
    cme.display = display;
    cme.format = 32;
    cme.data.l[0] = cme.data.l[1] = cme.data.l[2] = cme.data.l[3] =
        cme.data.l[4] = 0L;

    meawm_ng_xrootpmap_id = None;
}

void NetHandler::getWMProtocols(WaWindow *ww) {
    int n;
    Atom *protocols;
    
    if (XGetWMProtocols(display, ww->id, &protocols, &n)) {
        for (int i = 0; i < n; i++) {
            if (protocols[i] == wm_delete_window)
                ww->protocol_mask |= DeleteWindowProtocalMask;
            else if (protocols[i] == wm_take_focus)
                ww->protocol_mask |= TakeFocusProtocalMask;
        }
        XFree(protocols);
    }
}

void NetHandler::getWMHints(WaWindow *ww) {
    XWMHints *wm_hints = NULL;
    ww->state = NormalState;

    wm_hints = XGetWMHints(display, ww->id);
    
    if (wm_hints) {
        if (wm_hints->flags & StateHint) {
            ww->state = wm_hints->initial_state;
            if (ww->state != IconicState)
                ww->state = NormalState;
        }
        if (wm_hints->flags & InputHint) {
            ww->input_field = wm_hints->input;
        }
        
        if (wm_hints->flags & WindowGroupHint) {
            if (ww->window_group != wm_hints->window_group) {
                ww->wstate &= ~StateGroupLeaderMask;
                ww->wstate &= ~StateGroupMemberMask;
                
                ww->window_group = wm_hints->window_group;
                
                if (ww->window_group == ww->id)
                    ww->wstate |= StateGroupLeaderMask;
                else
                    ww->wstate |= StateGroupMemberMask;
            }
        } else {
            if (ww->window_group) {
                ww->wstate &= ~StateGroupLeaderMask;
                ww->wstate &= ~StateGroupMemberMask;
            }
            ww->window_group = None;
        }

        if (wm_hints->flags & UrgencyHint)
            ww->wstate |= StateUrgentMask;
        else
            ww->wstate &= ~StateUrgentMask;

        XFree(wm_hints);

        ww->windowStateCheck();
    }
}

void NetHandler::getClassHint(WaWindow *ww) {
    int status = 0;
    
    status = XGetClassHint(ww->display, ww->id, classhint);
    
    if (status) {
        if (classhint->res_class) {
            char *newwclass = wa_locale_to_utf8((char *) classhint->res_class);
            if (newwclass) {
                if (ww->frame) DWIN_RENDER_GET(ww->frame);
                delete [] ww->wclass;
                ww->wclass = newwclass;
                if (ww->frame) DWIN_RENDER_RELEASE(ww->frame);
            }            
            XFree(classhint->res_class);
        }
        if (classhint->res_name) {            
            char *newwclassname =
                wa_locale_to_utf8((char *) classhint->res_name);
            if (newwclassname) {
                if (ww->frame) DWIN_RENDER_GET(ww->frame);
                delete [] ww->wclassname;
                ww->wclassname = newwclassname;
                if (ww->frame) DWIN_RENDER_RELEASE(ww->frame);
            }
            XFree(classhint->res_name);
        }
    }
}

void NetHandler::getWMClientMachineHint(WaWindow *ww) {
    int status = 0, n;
    XTextProperty text_prop;
    char **cl;
    
    status = XGetWMClientMachine(ww->display, ww->id, &text_prop);
    
    if (status) {
	     if (text_prop.encoding == XA_STRING) {
            char *newhostname =
                wa_locale_to_utf8((char *) text_prop.value);
            if (newhostname) {
                if (ww->frame) DWIN_RENDER_GET(ww->frame);
                delete [] ww->host;
                ww->host = newhostname;
                if (ww->frame) DWIN_RENDER_RELEASE(ww->frame);
            }
        } else {

#ifndef   X_HAVE_UTF8_STRING
#  define Xutf8TextPropertyToTextList XmbTextPropertyToTextList
#endif // !X_HAVE_UTF8_STRING

            Xutf8TextPropertyToTextList(display, &text_prop, &cl, &n);
            if (cl) {
                if (ww->frame) DWIN_RENDER_GET(ww->frame);
                delete [] ww->host;
                ww->host = WA_STRDUP(cl[0]);
                if (ww->frame) DWIN_RENDER_RELEASE(ww->frame);
                XFreeStringList(cl);
            }
        }
    }
}

void NetHandler::getTransientForHint(WaWindow *ww) {
    int status = 0;
    Window trans;

    Window was_transient_for = ww->transient_for;
    ww->withdrawTransient();
    
    status = XGetTransientForHint(display, ww->id, &trans);
    
    if (ww->ws->config.transient_above) {
        if (status && (trans != ww->id)) {
            if (trans == None || trans == ww->ws->id) {
                ww->transient_for = ww->ws->id;
                list<WaWindow *>::iterator it = ww->ws->wawindow_list.begin();
                for (; it != ww->ws->wawindow_list.end(); ++it)
                    if ((*it) != ww) (*it)->transients.push_back(ww->id);
            } else {
                WaWindow *transfor = (WaWindow *)
                    meawm_ng->findWin(trans, WindowType);
                if (transfor) {
                    if (transfor != ww) {
                        ww->transient_for = trans;
                        transfor->transients.push_back(ww->id);
                    }
                } else if (ww->window_group) {
                    list<WaWindow *>::iterator it =
                        ww->ws->wawindow_list.begin();
                    for (;it != ww->ws->wawindow_list.end(); ++it) {
                        if ((*it) != ww &&
                            (*it)->window_group == ww->window_group &&
                            (*it)->transient_for == (Window) 0)
                            (*it)->transients.push_back(ww->id);
                    }
                }
            }
        }
    }

    if (status && (trans != ww->id))
        ww->setWindowState(ww->wstate | StateTransientMask);
    else if (was_transient_for)
        ww->setWindowState(ww->wstate & ~StateTransientMask);
}

void NetHandler::getMWMHints(WaWindow *ww) {
    MwmHints *mwmhints;
    int status = Success - 1;
    
    status = XGetWindowProperty(display, ww->id, mwm_hints, 0L, 20L,
                                false, mwm_hints, &real_type,
                                &real_format, &items_read, &items_left,
                                (unsigned char **) &mwmhints);

    if (status == Success && items_read) {
        if (items_read >= PropMotifWmHintsElements) {
            if (mwmhints->flags & MwmHintsDecorations) {
                if (mwmhints->decorations & MwmDecorAll) {
                    ww->decorAllOn();
                } else {
                    long decormask = 0L;
                    if (mwmhints->decorations & MwmDecorTitle)
                        decormask |= DecorTitleMask;
                    if (mwmhints->decorations & MwmDecorBorder)
                        decormask |= DecorBorderMask;
                    if (mwmhints->decorations & MwmDecorHandle)
                        decormask |= DecorHandlesMask;
                    ww->setDecor(decormask);
                }
            }
        }
        XFree(mwmhints);
    }
}

void NetHandler::getWMNormalHints(WaWindow *ww) {
    long dummy;
    int status = 0;

    ww->size.max_width = ww->size.max_height = 65536;
    ww->size.min_width = ww->size.min_height = ww->size.width_inc =
        ww->size.height_inc = 1;
    ww->size.base_size = false;
    ww->size.base_width = ww->size.base_height = 0;
    ww->size.win_gravity = NorthWestGravity;
    ww->size.min_aspect = ww->size.max_aspect = 0.0;
    ww->size.aspect = false;

    size_hints->flags = 0;

    status = XGetWMNormalHints(display, ww->id, size_hints, &dummy);
    
    if (status) {
        if (size_hints->flags & (PPosition | USPosition))
            ww->wstate |= StatePositionedMask;
        if (size_hints->flags & PBaseSize) {
            ww->size.base_size = true;
            ww->size.base_width = size_hints->base_width;
            ww->size.base_height = size_hints->base_height;
        }
        if (size_hints->flags & PMinSize) {
            if (size_hints->min_width >= 0)
                ww->size.min_width = size_hints->min_width;
            if (size_hints->min_height >= 0)
                ww->size.min_height = size_hints->min_height;
        } else if (ww->size.base_size) {
            ww->size.min_width = ww->size.base_width;
            ww->size.min_height = ww->size.base_height;
        }
        if (size_hints->flags & PMaxSize) {
            if (size_hints->max_width > (signed int) ww->size.max_width)
                ww->size.max_width = size_hints->max_width;
            else
                ww->size.max_width = ww->size.min_width;
            
            if (size_hints->max_height > (signed int) ww->size.max_height)
                ww->size.max_height = size_hints->max_height;
            else
                ww->size.max_height = ww->size.min_height;
        }
        if (size_hints->flags & PResizeInc) {
            ww->size.width_inc = size_hints->width_inc;
            ww->size.height_inc = size_hints->height_inc;
        }
        if (size_hints->flags & PWinGravity)
            ww->size.win_gravity = size_hints->win_gravity;
        
        if (size_hints->flags & PAspect) {
            ww->size.min_aspect = (double) size_hints->min_aspect.x /
                (double) size_hints->min_aspect.y;
            ww->size.max_aspect = (double) size_hints->max_aspect.x /
                (double) size_hints->max_aspect.y;
            if (ww->size.min_aspect <= ww->size.max_aspect)
                ww->size.aspect = true;
        }
    }
    ww->size.base_width = (ww->size.base_width) ? ww->size.base_width :
        ww->size.min_width;
    ww->size.base_height = (ww->size.base_height) ? ww->size.base_height :
        ww->size.min_height;
}

void NetHandler::getState(WaWindow *ww) {
    CARD32 *data;

    ww->state = WithdrawnState;

    if (XGetWindowProperty(display, ww->id, wm_state, 0L, 1L, false,
                           wm_state, &real_type, &real_format, &items_read,
                           &items_left, (unsigned char **) &data) ==
        Success && items_read) {
        ww->state = *data;
        XFree(data);
    } else ww->deleted = true;
}

void NetHandler::setState(WaWindow *ww, int newstate) {
    CARD32 data[2];
    
    ww->state = newstate;
    switch (ww->state) {
        case IconicState:
            ww->wstate |= StateMinimizedMask;
            ww->hide();
            setWmState(ww);
            break;
        case NormalState:
            ww->wstate &= ~StateMinimizedMask;
            if (! ww->mapped) ww->mapWindow();
            else if (ww->desktop_mask &
                     (1L << ww->ws->current_desktop->number))
                ww->show();
            setWmState(ww);
    }
    if (ww->want_focus && ww->mapped && (! ww->hidden))
        ww->meawm_ng->focusNew(ww->id, false);

    ww->want_focus = false;
    
    data[0] = ww->state;
    data[1] = None;

    XChangeProperty(display, ww->id, wm_state, wm_state,
                    32, PropModeReplace, (unsigned char *) data, 2);

    ww->sendConfig();
}

void NetHandler::getWmState(WaWindow *ww) {
    CARD32 *data;
    bool vert = false, horz = false, shaded = false, decor = false;
    unsigned int i;
    long decormask = 0L;
    int status = Success - 1;

    status = XGetWindowProperty(display, ww->id, net_wm_state, 0L, 13L,
                                false, XA_ATOM, &real_type,
                                &real_format, &items_read, &items_left, 
                                (unsigned char **) &data);

    if (status == Success && items_read) {
        for (i = 0; i < items_read; i++) {
            if (data[i] == net_wm_state_sticky)
                ww->wstate |= StateStickyMask;
            else if (data[i] == net_wm_state_shaded) shaded = true;
            else if (data[i] == net_wm_maximized_vert) vert = true;
            else if (data[i] == net_wm_maximized_horz) horz = true;
            else if (data[i] == net_wm_state_hidden)
                ww->wstate |= StateMinimizedMask;
            else if (data[i] == net_wm_state_skip_taskbar)
                ww->wstate &= ~StateTasklistMask;
            else if (data[i] == net_wm_state_above ||
                     data[i] == net_wm_state_stays_on_top)
                ww->alwaysontopOn();
            else if (data[i] == net_wm_state_below ||
                     data[i] == net_wm_state_stays_at_bottom)
                ww->alwaysatbottomOn();
            else if (data[i] == net_wm_state_fullscreen)
                ww->wstate |= StateFullscreenMask;
            else if (data[i] == meawm_ng_net_wm_state_decor) decor = true;
            else if (data[i] == meawm_ng_net_wm_state_decortitle)
                decormask |= DecorTitleMask;
            else if (data[i] == meawm_ng_net_wm_state_decorborder)
                decormask |= DecorBorderMask;
            else if (data[i] == meawm_ng_net_wm_state_decorhandles)
                decormask |= DecorHandlesMask;
        }
        if (decor) ww->setDecor(decormask);
        XFree(data);
    }
    
    if (vert && horz) {
        status = Success - 1;
        status = XGetWindowProperty(display, ww->id,
                                    meawm_ng_net_maximized_restore,
                                    0L, 6L, false, XA_CARDINAL, &real_type,
                                    &real_format, &items_read,
                                    &items_left, (unsigned char **) &data);
        
        if (status == Success && items_read) {
            if (items_read >= 6) {
                ww->_maximize(data[4], data[5]);
                ww->restore_max.x = data[0];
                ww->restore_max.y = data[1];
                ww->restore_max.width = data[2];
                ww->restore_max.height = data[3];
            }
            XFree(data);
        }
    }
    if (shaded) ww->wstate |= StateShadedMask;

    ww->windowStateCheck();
}

void NetHandler::setWmState(WaWindow *ww) {
    int i = 0;
    CARD32 data[16];
    CARD32 data2[6];
    
    if (ww->wstate & StateStickyMask) data[i++] = net_wm_state_sticky;
    if (ww->wstate & StateShadedMask) data[i++] = net_wm_state_shaded;
    if (ww->wstate & StateAlwaysOnTopMask) {
        data[i++] = net_wm_state_above;
        data[i++] = net_wm_state_stays_on_top;
    }
    if (ww->wstate & StateAlwaysAtBottomMask) {
        data[i++] = net_wm_state_below;
        data[i++] = net_wm_state_stays_at_bottom;
    }
    if (ww->wstate & StateMinimizedMask) data[i++] = net_wm_state_hidden;
    if (ww->wstate & StateFullscreenMask) data[i++] = net_wm_state_fullscreen;
    if (ww->wstate & StateMaximizedMask) {
        data[i++] = net_wm_maximized_vert;
        data[i++] = net_wm_maximized_horz;
            
        data2[0] = ww->restore_max.x;
        data2[1] = ww->restore_max.y;
        data2[2] = ww->restore_max.width;
        data2[3] = ww->restore_max.height;
        data2[4] = ww->restore_max.misc0;
        data2[5] = ww->restore_max.misc1;

        XChangeProperty(display, ww->id, meawm_ng_net_maximized_restore,
                        XA_CARDINAL, 32, PropModeReplace,
                        (unsigned char *) data2, 6);
    } else
        XDeleteProperty(display, ww->id, meawm_ng_net_maximized_restore);
    
    data[i++] = meawm_ng_net_wm_state_decor;
    if (ww->wstate & StateDecorTitleMask)
        data[i++] = meawm_ng_net_wm_state_decortitle;
    if (ww->wstate & StateDecorBorderMask)
        data[i++] = meawm_ng_net_wm_state_decorborder;
    if (ww->wstate & StateDecorHandlesMask)
        data[i++] = meawm_ng_net_wm_state_decorhandles;
    
    XChangeProperty(display, ww->id, net_wm_state, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) data, i);
    
    ww->windowStateCheck();
}

void NetHandler::setSupported(WaScreen *ws) {
    CARD32 data[54];
    int i = 0;
    
    data[i++] = net_supported;
    data[i++] = net_supported_wm_check;
    
    data[i++] = net_client_list;
    data[i++] = net_client_list_stacking;
    data[i++] = net_active_window;
    
    data[i++] = net_desktop_viewport;
    data[i++] = net_desktop_geometry;
    data[i++] = net_current_desktop;
    data[i++] = net_number_of_desktops;
    data[i++] = net_desktop_names;
    data[i++] = net_workarea;
    
    data[i++] = net_wm_desktop;
    data[i++] = net_wm_name;
    data[i++] = net_wm_visible_name;
    data[i++] = net_wm_strut;
    data[i++] = net_wm_strut_partial;
    data[i++] = net_wm_pid;
    data[i++] = net_wm_user_time;
    
    data[i++] = net_wm_state;
    data[i++] = net_wm_state_sticky;
    data[i++] = net_wm_state_shaded;
    data[i++] = net_wm_state_hidden;
    data[i++] = net_wm_maximized_vert;
    data[i++] = net_wm_maximized_horz;
    data[i++] = net_wm_state_above;
    data[i++] = net_wm_state_below;
    data[i++] = net_wm_state_stays_on_top;
    data[i++] = net_wm_state_stays_at_bottom;
    data[i++] = net_wm_state_fullscreen;
    data[i++] = net_wm_state_skip_taskbar;

    data[i++] = net_wm_allowed_actions;
    data[i++] = net_wm_action_move;
    data[i++] = net_wm_action_resize;
    data[i++] = net_wm_action_minimize;
    data[i++] = net_wm_action_shade;
    data[i++] = net_wm_action_stick;
    data[i++] = net_wm_action_maximize_horz;
    data[i++] = net_wm_action_maximize_vert;
    data[i++] = net_wm_action_fullscreen;
    data[i++] = net_wm_action_change_desktop;
    data[i++] = net_wm_action_close;
    
    data[i++] = net_wm_window_type;
    data[i++] = net_wm_window_type_desktop;
    data[i++] = net_wm_window_type_dock;
    data[i++] = net_wm_window_type_toolbar;
    data[i++] = net_wm_window_type_menu;
    data[i++] = net_wm_window_type_splash;
    data[i++] = net_wm_window_type_dialog;
    data[i++] = net_wm_window_type_utility;
    data[i++] = net_wm_window_type_normal;
    
    data[i++] = net_close_window;
    data[i++] = net_moveresize_window;
    data[i++] = net_wm_moveresize;
    
    XChangeProperty(display, ws->id, net_supported, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) data, i);
}

void NetHandler::setSupportedWMCheck(WaScreen *ws, Window child) {
    XChangeProperty(display, ws->id, net_supported_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) &child, 1);

    XChangeProperty(display, child, net_supported_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) &child, 1);

    XChangeProperty(display, child, net_wm_name, utf8_string, 8,
                    PropModeReplace, (unsigned char *) PACKAGE,
                    strlen(PACKAGE));
}

void NetHandler::setClientList(WaScreen *ws) {
    CARD32 *data;
    int i = 0;

    data = new CARD32[ws->wawindow_list_map_order.size() + 1];

    list<WaWindow *>::iterator it =
        ws->wawindow_list_map_order.begin();
    for (; it != ws->wawindow_list_map_order.end(); ++it) {
        data[i++] = (*it)->id;
    }

    XChangeProperty(display, ws->id, net_client_list, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) data, i);

    delete [] data;
}

void NetHandler::setClientListStacking(WaScreen *ws) {
    CARD32 *data;
    int i = 0;
    WaFrameWindow *wf;

    data = new CARD32[ws->wawindow_list.size() + 1];

    list<Window>::reverse_iterator it = ws->aab_stacking_list.rbegin();
    for (; it != ws->aab_stacking_list.rend(); ++it) {
        wf = (WaFrameWindow *) meawm_ng->findWin(*it, WindowFrameType);
        if (wf) data[i++] = wf->wa->id;
    }
    it = ws->stacking_list.rbegin();
    for (; it != ws->stacking_list.rend(); ++it) {
        wf = (WaFrameWindow *) meawm_ng->findWin(*it, WindowFrameType);
        if (wf) data[i++] = wf->wa->id;
    }
    it = ws->aot_stacking_list.rbegin();
    for (; it != ws->aot_stacking_list.rend(); ++it) {
        wf = (WaFrameWindow *) meawm_ng->findWin(*it, WindowFrameType);
        if (wf) data[i++] = wf->wa->id;
    }
    
    XChangeProperty(display, ws->id, net_client_list_stacking, XA_WINDOW,
                    32, PropModeReplace, (unsigned char *) data, i);
    
    delete [] data;
}

void NetHandler::getClientListStacking(WaScreen *ws) {
    CARD32 *data;
    unsigned int i;
    
    if (XGetWindowProperty(display, ws->id, net_client_list_stacking,
                           0L, 8192L,
                           false, XA_WINDOW, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) {
        for (i = 0; i < items_read; i++) {
            WaWindow *ww = (WaWindow *) meawm_ng->findWin(data[i], WindowType);
            if (ww) ws->raiseWindow(ww->frame->id, false);
        }
    }
}

void NetHandler::setActiveWindow(WaScreen *ws, WaWindow *ww) {
    CARD32 *data;
    list<WaWindow *>::iterator it;
    int i = 0;

    data = new CARD32[ws->wawindow_list.size() + 1];
    
    if (ww) {
        ws->focused = false;
        ws->wawindow_list.remove(ww);
        ws->wawindow_list.push_front(ww);
    } else
        data[i++] = None;

    it = ws->wawindow_list.begin();
    for (; it != ws->wawindow_list.end(); ++it)
        data[i++] = (*it)->id;
    
    XChangeProperty(display, ws->id, net_active_window, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) data, i);

    delete [] data;

    sendNotify(ws, ActiveWindowChangeNotify, (ww)? ww->id: 0);
}

void NetHandler::getActiveWindow(WaScreen *ws) {
    WaWindow *ww;
    CARD32 *data;
    int i;
    
    if (XGetWindowProperty(display, ws->id, net_active_window, 0L,
                           ws->wawindow_list.size(), 
                           false, XA_WINDOW, &real_type, &real_format, 
                           &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) {
        for (i = items_read - 1; i >= 0; i--) {
            if (i == 0 && data[0] == None) {
                ws->meawm_ng->focusNew(ws->id);
                break;
            }
            ww = (WaWindow *) meawm_ng->findWin(data[i], WindowType);
            if (ww) {
                ws->wawindow_list.remove(ww);
                ws->wawindow_list.push_front(ww);
                if (i == 0)
                    ww->meawm_ng->focusNew(ww->id);
            }
        }
        XFree(data);
    }
}

void NetHandler::getVirtualPos(WaWindow *ww) {
    int *data;
    
    if (XGetWindowProperty(display, ww->id, meawm_ng_net_virtual_pos,
                           0L, 2L, false, XA_INTEGER, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success &&
        items_read) {
        if (items_read >= 2) {
            ww->attrib.x = data[0] - ww->ws->v_x;
            ww->attrib.y = data[1] - ww->ws->v_y;
            if (ww->wstate & StateStickyMask) {
                ww->attrib.x = ww->attrib.x % ww->ws->width;
                ww->attrib.y = ww->attrib.y % ww->ws->height;
            }
        }
        XFree(data);
    }
}

void NetHandler::getXaName(WaWindow *ww) {
    int status = 0, n;
    XTextProperty text_prop;
    char **cl;
    
    status = XGetWMName(display, ww->id, &text_prop);
    
    if (status) {
	    if (text_prop.encoding == XA_STRING) {
            char *newname = wa_locale_to_utf8((char *) text_prop.value);
            if (newname) {
                ww->ws->smartNameRemove(ww);
                if (ww->frame) DWIN_RENDER_GET(ww->frame);
                delete [] ww->name;
                ww->name = newname;
                ww->realnamelen = strlen(ww->name);
                ww->ws->smartName(ww);
                if (ww->frame) DWIN_RENDER_RELEASE(ww->frame);
                setVisibleName(ww);
            }
        } else {

#ifndef   X_HAVE_UTF8_STRING
#  define Xutf8TextPropertyToTextList XmbTextPropertyToTextList
#endif // !X_HAVE_UTF8_STRING

            Xutf8TextPropertyToTextList(display, &text_prop, &cl, &n);
            if (cl) {
                ww->ws->smartNameRemove(ww);
                if (ww->frame) DWIN_RENDER_GET(ww->frame);
                delete [] ww->name;
                ww->name = WA_STRDUP(cl[0]);
                ww->realnamelen = strlen(ww->name);
                ww->ws->smartName(ww);
                if (ww->frame) DWIN_RENDER_RELEASE(ww->frame);
                setVisibleName(ww);
                XFreeStringList(cl);
            }
        }
    } else {
        ww->ws->smartNameRemove(ww);
        if (ww->frame) DWIN_RENDER_GET(ww->frame);
        delete [] ww->name;
        ww->name = WA_STRDUP("");
        ww->realnamelen = 0;
        ww->ws->smartName(ww);
        if (ww->frame) DWIN_RENDER_RELEASE(ww->frame);
        setVisibleName(ww);
    }
}

bool NetHandler::getNetName(WaWindow *ww) {
    char *data;
    int status = Success - 1;
    
    status = XGetWindowProperty(display, ww->id, net_wm_name, 0L, 8192L, 
                                false, utf8_string, &real_type,
                                &real_format, &items_read, &items_left, 
                                (unsigned char **) &data);
    
    if (status == Success && items_read) {
        ww->ws->smartNameRemove(ww);
        if (ww->frame) DWIN_RENDER_GET(ww->frame);
        delete [] ww->name;
        ww->name = WA_STRDUP(data);
        ww->realnamelen = strlen(ww->name);
        ww->ws->smartName(ww);
        if (ww->frame) DWIN_RENDER_RELEASE(ww->frame);
        setVisibleName(ww);
        XFree(data);
        return true;
    }
    return false;
}

void NetHandler::setVisibleName(WaWindow *ww) {
    XChangeProperty(display, ww->id, net_wm_visible_name,
                    utf8_string, 8, PropModeReplace,
                    (unsigned char *) ww->name, strlen(ww->name));
}

void NetHandler::removeVisibleName(WaWindow *ww) {
    XDeleteProperty(display, ww->id, net_wm_visible_name);
}

void NetHandler::setVirtualPos(WaWindow *ww) {
    int data[2];
    
    ww->gravitate(RemoveGravity);
    data[0] = ww->ws->v_x + ww->attrib.x;
    data[1] = ww->ws->v_y + ww->attrib.y;
    ww->gravitate(ApplyGravity);

    XChangeProperty(display, ww->id, meawm_ng_net_virtual_pos, XA_INTEGER,
                    32, PropModeReplace, (unsigned char *) data, 2);
    
    list<WaWindow *>::iterator mit = ww->merged.begin();
    for (; mit != ww->merged.end(); mit++) {
        if ((*mit) != ww) setVirtualPos(*mit);
    }
}

void NetHandler::getWmStrut(WaWindow *ww) {
    CARD32 *data;
    WMstrut *wm_strut;
    bool found = false;
    int status = Success - 1;

    status = XGetWindowProperty(display, ww->id, net_wm_strut_partial,
                                0L, 4L, false, XA_CARDINAL, &real_type,
                                &real_format, &items_read, &items_left, 
                                (unsigned char **) &data);
    
    if (status != Success) {
        status = Success - 1;
        status = XGetWindowProperty(display, ww->id, net_wm_strut, 0L, 4L, 
                                    false, XA_CARDINAL, &real_type,
                                    &real_format, &items_read, &items_left, 
                                    (unsigned char **) &data);
    }
    
    if (status == Success && items_read) {
        if (items_read >= 4) {
            list<WMstrut *>::iterator it = ww->ws->strut_list.begin();
            for (; it != ww->ws->strut_list.end(); ++it) {
                if ((*it)->window == ww->id) {
                    (*it)->left = data[0];
                    (*it)->right = data[1];
                    (*it)->top = data[2];
                    (*it)->bottom = data[3];
                    found = true;
                    ww->ws->updateWorkarea();
                }
            }
            if (! found) {
                wm_strut = new WMstrut;
                wm_strut->window = ww->id;
                wm_strut->left = data[0];
                wm_strut->right = data[1];
                wm_strut->top = data[2];
                wm_strut->bottom = data[3];
                ww->wm_strut = wm_strut;
                ww->ws->strut_list.push_back(wm_strut);
                ww->ws->updateWorkarea();
            }
        }
        XFree(data);
    }
}

void NetHandler::getWmPid(WaWindow *ww) {
    char tmp[32];
    CARD32 *data;
    int status = Success - 1;

    status = XGetWindowProperty(ww->display, ww->id, net_wm_pid, 0L, 1L, 
                                false, XA_CARDINAL, &real_type,
                                &real_format, &items_read, &items_left,
                                (unsigned char **) &data);
    
    if (status == Success && items_read) {
        snprintf(tmp, 32, "%d" , (unsigned int) *data);
        if (ww->frame) DWIN_RENDER_GET(ww->frame);
        delete [] ww->pid;
        ww->pid = WA_STRDUP(tmp);
        if (ww->frame) DWIN_RENDER_RELEASE(ww->frame);
        XFree(data);
    }
}

void NetHandler::getWmUserTime(WaWindow *ww) {
    CARD32 *data;
    int status = Success - 1;

    status = XGetWindowProperty(ww->display, ww->id, net_wm_user_time, 0L, 1L, 
                                false, XA_CARDINAL, &real_type,
                                &real_format, &items_read, &items_left,
                                (unsigned char **) &data);
    
    if (status == Success && items_read) {
        if ((unsigned int) *data)
            ww->wstate |= StateUserTimeMask;
        else
            ww->wstate &= ~StateUserTimeMask;
        
        XFree(data);

        ww->windowStateCheck();
    }
}

void NetHandler::getDesktopViewPort(WaScreen *ws) {
    CARD32 *data;
    
    if (XGetWindowProperty(display, ws->id, net_desktop_viewport, 0L, 32L,
                           false, XA_CARDINAL, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success &&
        items_read) {
        if (items_read >= 2 && ((items_read % 2) == 0)) {
            unsigned int i = 0;
            list<Desktop *>::iterator it = ws->desktop_list.begin();
            for (; (i * 2) < items_read &&
                     it != ws->desktop_list.end(); ++it) {
                (*it)->v_x = data[i++];
                (*it)->v_y = data[i++];
            }
            ws->moveViewportTo(ws->current_desktop->v_x,
                               ws->current_desktop->v_y);
        }
        XFree(data);
    }
}

void NetHandler::setDesktopViewPort(WaScreen *ws) {
    CARD32 data[16 * 2];

    unsigned int i = 0;
    list<Desktop *>::iterator it = ws->desktop_list.begin();
    for (; it != ws->desktop_list.end(); ++it, ++i) {
        data[(i * 2) + 0] = (*it)->v_x;
        data[(i * 2) + 1] = (*it)->v_y;
    }
    XChangeProperty(display, ws->id, net_desktop_viewport, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) data, i * 2);
    
    sendNotify(ws, ViewportChangeNotify, ws->current_desktop->v_x,
               ws->current_desktop->v_y);
}

void NetHandler::setDesktopGeometry(WaScreen *ws) {
    CARD32 data[2];

    data[0] = ws->v_xmax + ws->width;
    data[1] = ws->v_ymax + ws->height;    
    XChangeProperty(display, ws->id, net_desktop_geometry, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) data, 2);
}

void NetHandler::setNumberOfDesktops(WaScreen *ws) {
    CARD32 data[1];

    data[0] = ws->desktop_list.size();
    XChangeProperty(display, ws->id, net_number_of_desktops, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) data, 1);
}
    
void NetHandler::setCurrentDesktop(WaScreen *ws) {
    CARD32 data[1];
    
    data[0] = ws->current_desktop->number;
    XChangeProperty(display, ws->id, net_current_desktop, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) data, 1);

    sendNotify(ws, DesktopChangeNotify, ws->current_desktop->number);
}

void NetHandler::getCurrentDesktop(WaScreen *ws) {
    CARD32 *data;

    if (XGetWindowProperty(display, ws->id, net_current_desktop, 0L, 1L, 
                           false, XA_CARDINAL, &real_type, &real_format, 
                           &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) {
        ws->goToDesktop(data[0]);
        XFree(data);
    }
}

void NetHandler::setDesktopNames(WaScreen *ws, char *names) {
    int i;
    
    for (i = 0; i < 8192; i++) {
        if (names[i] == ',') names[i] = '\0';
        else if (names[i] == '\0') break;
    }
    names[i] = '\0';

    if (i)
        XChangeProperty(display, ws->id, net_desktop_names, utf8_string, 8,
                        PropModeReplace, (unsigned char *) names, i + 1);
}

void NetHandler::setWorkarea(WaScreen *ws) {
    CARD32 data[16 * 4];

    unsigned int i = 0;
    list<Desktop *>::iterator it = ws->desktop_list.begin();
    for (; it != ws->desktop_list.end(); ++it, ++i) {
        data[(i * 4) + 0] = (*it)->workarea.x;
        data[(i * 4) + 1] = (*it)->workarea.y;
        data[(i * 4) + 2] = (*it)->workarea.width;
        data[(i * 4) + 3] = (*it)->workarea.height;
    }
    XChangeProperty(meawm_ng->display, ws->id, net_workarea, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) data, i * 4);
}

void NetHandler::wXDNDMakeAwareness(Window window) {
    long int xdnd_version = 3;

    XChangeProperty(meawm_ng->display, window, xdndaware, XA_ATOM,
                    32, PropModeReplace, (unsigned char *) &xdnd_version, 1);
}

void NetHandler::wXDNDClearAwareness(Window window) {
    XDeleteProperty(meawm_ng->display, window, xdndaware);
}

void NetHandler::deleteSupported(WaScreen *ws) {
    XDeleteProperty(display, ws->id, net_desktop_geometry);
    XDeleteProperty(display, ws->id, net_workarea);
    XDeleteProperty(display, ws->id, net_supported_wm_check);
    XDeleteProperty(display, ws->id, net_supported);
}

void NetHandler::getXRootPMapId(WaScreen *ws) {
    CARD32 *data;
    
    int status = XGetWindowProperty(ws->display, ws->id, xrootpmap_id, 0L, 1L, 
                                    false, XA_PIXMAP, &real_type,
                                    &real_format, &items_read, &items_left, 
                                    (unsigned char **) &data);
    
    if (status == Success && items_read) {
        if ((Pixmap) *data != meawm_ng_xrootpmap_id) {
            unsigned int w, h;
            if (validate_drawable((Pixmap) (*data), &w, &h)) {

                RENDER_GET;

                if (meawm_ng_xrootpmap_id)
                    ws->config.external_bg = true;
                
                if (ws->bg_surface) {
                    if (ws->bg_surface->pixmap != meawm_ng_xrootpmap_id)
                        ws->bg_surface->pixmap = None;
                    
                    ws->bg_surface->unref();
                }
                ws->bg_surface = new WaSurface(ws->display, NULL,
                                               (Pixmap) (*data), None, NULL,
                                               w, h);

                if (ws->meawm_ng->running) {
                    ws->destroySubwindows();
                    ws->forceRenderOfWindows(ANY_DECOR_WINDOW_TYPE);
                }

                RENDER_RELEASE;
                
            }
        }
        XFree(data);
    } else {

        RENDER_GET;

        if (ws->bg_surface) ws->bg_surface->unref();
        ws->bg_surface = NULL;

        RENDER_RELEASE;
        
    }
}

void NetHandler::setXRootPMapId(WaScreen *ws, WaSurface *bg_surface) {
    if (! bg_surface) {
        if (ws->bg_surface && ws->bg_surface->pixmap == meawm_ng_xrootpmap_id) {
            XDeleteProperty(ws->display, ws->id, xrootpmap_id);
        }
        return;
    }
    XChangeProperty(ws->display, ws->id, xrootpmap_id, XA_PIXMAP,
                    32, PropModeReplace,
                    (unsigned char *) &bg_surface->pixmap, 1);
    WaSurface *old_bg = ws->bg_surface;

    RENDER_GET_ONE;
    
    ws->bg_surface = bg_surface->ref();

    RENDER_RELEASE;

    if (old_bg) {
        if (old_bg->pixmap && old_bg->pixmap != meawm_ng_xrootpmap_id) {
            XKillClient(ws->display, old_bg->pixmap);
            old_bg->pixmap = None;
        }
        old_bg->unref();
    }

    meawm_ng_xrootpmap_id = bg_surface->pixmap;

    ws->forceRenderOfWindows(ANY_DECOR_WINDOW_TYPE & ~RootType);
}

void NetHandler::getWmType(WaWindow *ww) {
    CARD32 *data;
    int status = Success - 1;
    
    status = XGetWindowProperty(display, ww->id, net_wm_window_type,
                                0L, 8L, false, XA_ATOM,
                                &real_type, &real_format, &items_read,
                                &items_left, (unsigned char **) &data);
    
    if (status == Success && items_read) {
        for (unsigned int i = 0; i < items_read; ++i) {
            if (data[i] == net_wm_window_type_desktop) {
                ww->desktop_mask = (1L << 16) - 1;
                ww->wstate &= ~StateTasklistMask;
                ww->wstate |= StateStickyMask;
                ww->setDecor(0L);
                ww->size.max_width = ww->ws->width;
                ww->size.min_width = ww->ws->width;
                ww->size.max_height = ww->ws->height;
                ww->size.min_height = ww->ws->height;
                ww->attrib.x = 0;
                ww->attrib.y = 0;
                ww->alwaysatbottomOn();
                ww->functions = FunctionDesktopMask;
            }
            else if (data[i] == net_wm_window_type_toolbar ||
                     data[i] == net_wm_window_type_dock) {
                ww->desktop_mask = (1L << 16) - 1;
                ww->wstate &= ~StateTasklistMask;
                ww->wstate |= StateStickyMask;
                ww->setDecor(0L);
                ww->alwaysontopOn();
                ww->functions = FunctionDesktopMask | FunctionSetStackingMask;
            }
            else if (data[i] == net_wm_window_type_splash ||
                     data[i] == net_wm_window_type_menu) {
                ww->wstate &= ~StateTasklistMask;
                ww->setDecor(0L);
                ww->alwaysontopOn();
                ww->functions = FunctionMoveMask | FunctionSetDecorMask |
                    FunctionSetStackingMask | FunctionDesktopMask;
            }
            else {
                if (ww->attrib.x == 0) {
                    if (ww->ws->current_desktop->workarea.x >
                        ww->attrib.x)
                        ww->attrib.x =
                            ww->ws->current_desktop->workarea.x;
                }
                if (ww->attrib.y == 0) {
                    if (ww->ws->current_desktop->workarea.y >
                        ww->attrib.y)
                        ww->attrib.y =
                            ww->ws->current_desktop->workarea.y;
                }
            }
        }
        XFree(data);
        ww->windowStateCheck();
    } else {
        if (ww->attrib.x == 0) {
            if (ww->ws->current_desktop->workarea.x > ww->attrib.x)
                ww->attrib.x = ww->ws->current_desktop->workarea.x;
        }
        if (ww->attrib.y == 0) {
            if (ww->ws->current_desktop->workarea.y > ww->attrib.y)
                ww->attrib.y = ww->ws->current_desktop->workarea.y;
        }
    }
}

void NetHandler::setDesktop(WaWindow *ww) {
    CARD32 data[1];

    data[0] = 0;
    if (ww->desktop_mask & (1L << ww->ws->current_desktop->number))
        data[0] = ww->ws->current_desktop->number;
    else {
        int i;
        for (i = 0; i < 16; i++)
            if (ww->desktop_mask & (1L << i)) {
                data[0] = i;
                break;
            }
    }
    if (ww->desktop_mask == ((1L << 16) - 1))
        data[0] = 0xffffffff;
    
    XChangeProperty(display, ww->id, net_wm_desktop, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) data, 1);
}

void NetHandler::setDesktopMask(WaWindow *ww) {
    CARD32 data[1];

    data[0] = ww->desktop_mask;

    XChangeProperty(display, ww->id, meawm_ng_net_wm_desktop_mask,
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *) data, 1);
}

void NetHandler::getDesktop(WaWindow *ww) {
    CARD32 *data;

    if (XGetWindowProperty(display, ww->id, net_wm_desktop, 0L, 1L, 
                           false, XA_CARDINAL, &real_type, &real_format, 
                           &items_read, &items_left,
                           (unsigned char **) &data) == Success && 
        items_read) {
        if (data[0] == 0xffffffff || data[0] == 0xfffffffe)
            ww->desktop_mask = ((1L << 16) - 1);
        else if (data[0] < 15) {
            ww->desktop_mask = (1L << data[0]);
        }
        XFree(data);
    }
    if (XGetWindowProperty(display, ww->id, meawm_ng_net_wm_desktop_mask,
                           0L, 1L, false, XA_CARDINAL, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) {
        ww->desktop_mask = data[0];
        XFree(data);
    }
}

bool NetHandler::isSystrayWindow(Window w) {
    CARD32 *data;
    
    items_read = 0;
    if (XGetWindowProperty(display, w, kde_net_wm_system_tray_window_for,
                           0L, 1L, false, XA_WINDOW, &real_type,
                           &real_format, &items_read, &items_left,
                           (unsigned char **) &data) != Success) {
        items_read = 0;
    }
    
    return ((items_read)? true: false);
}

void NetHandler::setSystrayWindows(WaScreen *ws) {
    CARD32 *data;
    int i = 0;

    data = new CARD32[ws->systray_window_list.size() + 1];

    list<Window>::iterator it = ws->systray_window_list.begin();
    for (; it != ws->systray_window_list.end(); it++)
        data[i++] = *it;

    XChangeProperty(display, ws->id, kde_net_system_tray_windows, XA_WINDOW,
                    32, PropModeReplace, (unsigned char *) data, i);

    delete [] data;
}

void NetHandler::getMergedState(WaWindow *ww) {
    CARD32 *data;
    Window mwin = (Window) 0;
    int mtype = NullMergeType;

    if (XGetWindowProperty(display, ww->id, meawm_ng_net_wm_merged_to, 0L,
                           1L, false, XA_WINDOW, &real_type, &real_format, 
                           &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) {
        mwin = *data;
        XFree(data);
        if (XGetWindowProperty(display, ww->id, meawm_ng_net_wm_merged_type,
                               0L, 1L, false, XA_CARDINAL, &real_type,
                               &real_format, &items_read, &items_left, 
                               (unsigned char **) &data) == Success && 
            items_read) {
            mtype = *data;
            XFree(data);
        }
    }
    
    if (mwin) {
        WaWindow *master = (WaWindow *)
            ww->meawm_ng->findWin(mwin, WindowType);
        if (master) {
            if (mtype != VertMergeType && mtype != HorizMergeType)
                master->merge(ww, CloneMergeType);
            else
                master->merge(ww, mtype);
        } else {
            if (! ww->meawm_ng->eh) {
                if (mtype != VertMergeType && mtype != HorizMergeType)
                    ww->ws->mreqs.push_back(new MReq(mwin, ww,
                                                           CloneMergeType));
                else
                    ww->ws->mreqs.push_back(new MReq(mwin, ww, mtype));
            }
        }
    }
}

void NetHandler::setMergedState(WaWindow *ww) {
    CARD32 data[1];

    if (ww->master) {
        data[0] = ww->master->id;
        XChangeProperty(display, ww->id, meawm_ng_net_wm_merged_to,
                        XA_WINDOW, 32, PropModeReplace,
                        (unsigned char *) data, 1);
        data[0] = ww->mergetype;
        XChangeProperty(display, ww->id, meawm_ng_net_wm_merged_type,
                        XA_CARDINAL, 32, PropModeReplace,
                        (unsigned char *) data, 1);
    } else
        XDeleteProperty(display, ww->id, meawm_ng_net_wm_merged_to);
}

void NetHandler::setMergeOrder(WaWindow *ww) {
    CARD32 *data;
    int i = 0;

    data = new CARD32[ww->merged.size()];

    list<WaWindow *>::iterator it = ww->merged.begin();
    for (; it != ww->merged.end(); it++)
        data[i++] = (*it)->id;
    
    if (i) {
        XChangeProperty(display, ww->id, meawm_ng_net_wm_merge_order,
                        XA_WINDOW, 32, PropModeReplace,
                        (unsigned char *) data, i);
    } else
        XDeleteProperty(display, ww->id, meawm_ng_net_wm_merge_order);
}

void NetHandler::getMergeOrder(WaWindow *ww) {
    CARD32 *data;
    int status = Success - 1;
    
    status = XGetWindowProperty(display, ww->id, meawm_ng_net_wm_merge_order,
                                0L, 8192L, false, XA_WINDOW, &real_type,
                                &real_format, &items_read, &items_left,
                                (unsigned char **) &data);
    
    if (status == Success && items_read) {
        int i = items_read;
        for (i--; i >= 0; i--) {
            list<WaWindow *>::iterator it = ww->merged.begin();
            for (; it != ww->merged.end(); it++) {
                if (data[i] == (*it)->id) {
                    ww->merged.erase(it);
                    ww->merged.push_front(*it);
                    break;
                }
            }
        }
        ww->updateAllAttributes();
        XFree(data);
    }
}

void NetHandler::setMergeAtfront(WaWindow *ww, Window win) {
    CARD32 data[1];

    if (ww->ws->shutdown) return;
    
    data[0] = win;
    
    XChangeProperty(display, ww->id, meawm_ng_net_wm_merge_atfront,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *) data, 1);
}

void NetHandler::getMergeAtfront(WaWindow *ww) {
    CARD32 *data;
    int status = Success - 1;
        
    status = XGetWindowProperty(display, ww->id, meawm_ng_net_wm_merge_atfront,
                                0L, 1L, false, XA_WINDOW, &real_type,
                                &real_format, &items_read, &items_left,
                                (unsigned char **) &data);
    
    if (status == Success && items_read) {
        if (*data == ww->id) ww->toFront();
        else {
            list<WaWindow *>::iterator it = ww->merged.begin();
            for (; it != ww->merged.end(); it++) {
                if ((*it)->id == *data)
                    (*it)->toFront();
            }
        }
        XFree(data);
    }
}

void NetHandler::setAllowedActions(WaWindow *ww) {
    CARD32 data[10];
    int i = 0;

    if (ww->functions & FunctionMoveMask)
        data[i++] = net_wm_action_move;

    if (ww->functions & FunctionResizeMask) {
        data[i++] = net_wm_action_resize;
        data[i++] = net_wm_action_maximize_horz;
        data[i++] = net_wm_action_maximize_vert;
        data[i++] = net_wm_action_fullscreen;
    }
    
    if (ww->functions & FunctionDesktopMask)
        data[i++] = net_wm_action_change_desktop;
    
    data[i++] = net_wm_action_close;
    data[i++] = net_wm_action_minimize;
    data[i++] = net_wm_action_shade;
    data[i++] = net_wm_action_stick;
    
    XChangeProperty(display, ww->id, net_wm_allowed_actions,
                    XA_ATOM, 32, PropModeReplace,
                    (unsigned char *) data, i);
}

void NetHandler::removeAllowedActions(WaWindow *ww) {
    XDeleteProperty(display, ww->id, net_wm_allowed_actions);
}

void NetHandler::sendNotify(AWindowObject *awo, long int event,
                            long int value1, long int value2) {
    if (meawm_ng->running) {
        cme.message_type = meawm_ng_net_event_notify;
        cme.window = awo->ws->id;
        cme.data.l[0] = awo->id;
        cme.data.l[1] = event;
        cme.data.l[2] = value1;
        cme.data.l[3] = value2;
        XSendEvent(display, awo->ws->id, false, StructureNotifyMask,
                   (XEvent *) &cme);
    }
}

#define BEST_ICON_MATCH_SIZE 256
#define MAX_ICON_MATCH_DIFF 5000

void NetHandler::getWmIconImage(WaWindow *ww) {
    CARD32 *data;
    int status = Success - 1;

    status = XGetWindowProperty(ww->display, ww->id, net_wm_icon_image, 0L,
                                1048576L, false, XA_CARDINAL, &real_type,
                                &real_format, &items_read, &items_left, 
                                (unsigned char **) &data);

    if (status != Success || items_read == 0) {
        status = XGetWindowProperty(ww->display, ww->id, net_wm_icon, 0L,
                                    1048576L, false, XA_CARDINAL, &real_type,
                                    &real_format, &items_read, &items_left, 
                                    (unsigned char **) &data);
    }
        
    if (status == Success && items_read >= 1048576L) {
        XFree(data);
        return;
    }
    
    if (status == Success && items_read) {
        unsigned int best_diff = INT_MAX;
        unsigned long *best = NULL;
        unsigned long *icon = (unsigned long *) data;
        unsigned int len;
        while (items_read > 3) {
            unsigned int diff;
            len = icon[0] * icon[1];
            if (len > BEST_ICON_MATCH_SIZE) diff = len - BEST_ICON_MATCH_SIZE;
            else diff = BEST_ICON_MATCH_SIZE - len;
            if (diff < best_diff && items_read >= (len + 2)) {
                best_diff = diff;
                best = icon;
            }
            items_read -= (len + 2);
            icon += (len + 2);
        }
        if (! best || best_diff > MAX_ICON_MATCH_DIFF) {
            XFree(data);
            return;
        }
        
        unsigned int w = best[0];
        unsigned int h = best[1];
        len = w * h;
        unsigned long *argb_data = &best[2];
        unsigned char *pix = new unsigned char[len * sizeof(WaPixel)];
      
        for (unsigned int i = 0; i < len * sizeof(WaPixel);
             i += sizeof(WaPixel)) {
            unsigned long *base = argb_data++;
            unsigned char blue = *base;
            unsigned char green = *base >> 8;
            unsigned char red = *base >> 16;
            unsigned char alpha = *base >> 24;
            WaPixel p;
            
            red = (unsigned) red * (unsigned) alpha / 255;
            green = (unsigned) green * (unsigned) alpha / 255;
            blue = (unsigned) blue * (unsigned) alpha / 255;
            p = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
            memcpy(&pix[i], &p, sizeof(WaPixel));
        }
        
        XFree(data);

        RenderGroup *icon_group = new RenderGroup(ww->ws, NULL);
        RenderOpImage *icon_image = new RenderOpImage();
        icon_image->image = ww->ws->rgbaToWaSurface(pix, w, h);
        icon_image->scale = ImageNormalScaleType;
        icon_group->operations.push_back(icon_image);
        icon_group->_w = w;
        icon_group->_h = h;
        icon_group->_x = icon_image->_y = 0;
        icon_group->_xu = icon_group->_yu = icon_group->_wu =
            icon_group->_hu = PXLenghtUnitType;

        if (ww->wm_icon_image) ww->wm_icon_image->unref();
        ww->wm_icon_image = icon_group;
        
        ww->frame->setDynamicGroup(DynamicGroupWmIconImageType,
                                   ww->wm_icon_image);
        
        ww->drawDecor();
    }
}

void NetHandler::getWmIconSvg(WaWindow *ww) {
    CARD32 *data;
    int status = Success - 1;

#ifdef    SVG
    status = XGetWindowProperty(ww->display, ww->id, net_wm_icon_svg, 0L,
                                1048576L, false,  utf8_string, &real_type,
                                &real_format, &items_read, &items_left, 
                                (unsigned char **) &data);
    
    if (status == Success && items_read >= 1048576L) {
        XFree(data);
        return;
    }
    
    if (status == Success && items_read) {
        svg_cairo_t *cairo_svg;
        svg_cairo_status status;
        status = svg_cairo_create(&cairo_svg);
        if (status == SVG_CAIRO_STATUS_SUCCESS) {
            status = svg_cairo_parse_buffer(cairo_svg, (const char *) data,
                                            items_read);
            if (status == SVG_CAIRO_STATUS_SUCCESS) {
                RenderGroup *icon_group = new RenderGroup(ww->ws, NULL);
                RenderOpSvg *icon_svg = new RenderOpSvg();
                icon_svg->cairo_svg = cairo_svg;
                icon_group->operations.push_back(icon_svg);
                icon_group->_w = 16;
                icon_group->_h = 16;
                icon_group->_x = icon_group->_y = 0;
                icon_group->_xu = icon_group->_yu = icon_group->_wu =
                    icon_group->_hu = PXLenghtUnitType;

                if (ww->wm_icon_svg) ww->wm_icon_svg->unref();
                ww->wm_icon_svg = icon_group;
                 
                ww->frame->setDynamicGroup(DynamicGroupWmIconSvgType,
                                           ww->wm_icon_svg);
                 
                ww->drawDecor();
            }
        }
        XFree(data);
    }
#endif // SVG
    
}


char *NetHandler::getDockappHandler(Dockapp *dockapp) {
    char *handlername = NULL;
    CARD32 *data;
    int status = Success - 1;

    status = XGetWindowProperty(display, dockapp->id,
                                meawm_ng_net_dockapp_holder, 0L, 8192L,
                                false, utf8_string, &real_type,
                                &real_format, &items_read, &items_left, 
                                (unsigned char **) &data);
    
    if (status == Success && items_read) {
        handlername = WA_STRDUP((char *) data);
        XFree(data);
    }

    if (! handlername)
        return WA_STRDUP("default");
    else
        return handlername;
}

void NetHandler::setDockappHandler(Dockapp *dockapp) {
    XChangeProperty(display, dockapp->id, meawm_ng_net_dockapp_holder,
                    utf8_string, 8, PropModeReplace,
                    (unsigned char *) dockapp->dh->name,
                    strlen(dockapp->dh->name));
}

void NetHandler::getDockappPrio(Dockapp *dockapp) {
    CARD32 *data;
    int prio = 0;

    if (XGetWindowProperty(display, dockapp->id, meawm_ng_net_dockapp_prio,
                           0L, 1L, false, XA_CARDINAL, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) {
        prio = *data;
        XFree(data);
    }
    
    dockapp->prio = prio;
}

void NetHandler::setDockappPrio(Dockapp *dockapp) {
    if (dockapp->prio_set)
        XChangeProperty(display, dockapp->id, meawm_ng_net_dockapp_prio,
                        XA_CARDINAL, 32, PropModeReplace,
                        (unsigned char *) &dockapp->prio, 1);
}

void NetHandler::getConfig(WaScreen *ws, Window window, Atom atom,
                           unsigned int incsize) {
    CARD32 *data;
    Parser * parser = NULL;
    int status;
    bool failed = false;
    XEvent event;

    XSelectInput(display, window, StructureNotifyMask |
                 PropertyChangeMask);

    RENDER_GET;
    
    do {
        status = Success - 1;
        status = XGetWindowProperty(display, window, atom,
                                    0L, (incsize)? incsize: 819200L, true,
                                    utf8_string, &real_type, &real_format,
                                    &items_read, &items_left,
                                    (unsigned char **) &data);
        if (status == Success && items_read) {
            if (! parser) {
                parser = new Parser(ws);
                parser->pushElementHandler(new CfgElementHandler(parser));
                parser->setFilename("_WAIMEA_NET_CFG");
            }

            if (! failed) {
                if (! parser->parseChunk((char *) data, items_read))
                    failed = true;
            }
            
            XFree(data);
        } else
            break;

        if (incsize && items_read && items_read == incsize) {
            bool new_data = false;

            do {
                XWindowEvent(display, window, StructureNotifyMask |
                             PropertyChangeMask, &event);
                
                if (event.type == PropertyNotify) {
                    if (event.xproperty.state != PropertyDelete) {
                        new_data = true;
                    }
                } else
                    break;
                
            } while (! new_data);

            if (! new_data) break;
        }
    } while (items_read == incsize);
    
    XSelectInput(display, window, NoEventMask);
    
    if (parser) {
        if (! failed)
            parser->parseChunkEnd();
        
        delete parser;
    }

    RENDER_RELEASE;
    
}
