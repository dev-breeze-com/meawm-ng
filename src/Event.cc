/* Event.cc

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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <cairo.h>
    
#ifdef    SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE

#ifdef    RANDR
#  include <X11/extensions/Xrandr.h>
#endif // RANDR

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    LIMITS_H
#  include <limits.h>
#endif // LIMITS_H

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif // HAVE_SYS_SELECT_H

#ifdef    HAVE_SYS_WAIT_H
#  include <sys/types.h>
#  include <sys/wait.h>
#endif // HAVE_SYS_WAIT_H

#ifdef    HAVE_SIGNAL_H
#  include <signal.h>
#endif // HAVE_SIGNAL_H
    
}

#include "Event.hh"
#include "Timer.hh"
#include "Action.hh"
#include "Window.hh"

EventHandler::EventHandler(Meawm_NG *wa) {
    meawm_ng = wa;
    focused = last_click_win = action_manager_window = None;
    move_resize = EndMoveResizeType;
    last_button = 0;
    action_manager_screen = NULL;

    moveresize_return_mask.insert(MotionNotify);
    moveresize_return_mask.insert(ButtonPress);
    moveresize_return_mask.insert(ButtonRelease);
    moveresize_return_mask.insert(KeyPress);
    moveresize_return_mask.insert(KeyRelease);
    moveresize_return_mask.insert(MapRequest);
    moveresize_return_mask.insert(UnmapNotify);
    moveresize_return_mask.insert(DestroyNotify);
    moveresize_return_mask.insert(EnterNotify);
    moveresize_return_mask.insert(LeaveNotify);
    moveresize_return_mask.insert(ConfigureRequest);

    menu_viewport_move_return_mask.insert(MotionNotify);
    menu_viewport_move_return_mask.insert(ButtonPress);
    menu_viewport_move_return_mask.insert(ButtonRelease);
    menu_viewport_move_return_mask.insert(KeyPress);
    menu_viewport_move_return_mask.insert(KeyRelease);
    menu_viewport_move_return_mask.insert(MapRequest);
    menu_viewport_move_return_mask.insert(EnterNotify);
    menu_viewport_move_return_mask.insert(LeaveNotify);

    xfd = ConnectionNumber(meawm_ng->display);
}

EventHandler::~EventHandler(void) {
    moveresize_return_mask.clear();
    menu_viewport_move_return_mask.clear();
}

void EventHandler::eventLoop(set<int> *return_mask, XEvent *event) {
   fd_set rfds;

    for (;;) {
        if (XPending(meawm_ng->display)) {
            XNextEvent(meawm_ng->display, event);
            if (return_mask->find(event->type) != return_mask->end()) return;
            handleEvent(event);
        } else if (meawm_ng->quit_signal) {
            quit(EXIT_SUCCESS);
        } else if (meawm_ng->restart_signal) {
            restart(NULL);
        } else if (meawm_ng->unknown_signal) {
            quit(EXIT_FAILURE);
        } else if (meawm_ng->timer->timer_signal) {
            meawm_ng->timer->handleTimeout();
            meawm_ng->timer->timer_signal = 0;
        } else if (! doings.empty()) {
            do {
                doings.front()->envoke();
                delete doings.front();
                doings.pop_front();
            } while (! doings.empty());
        } else if (

#ifdef    THREAD
            (! __render_thread_count) &&
#endif // THREAD

                   (! __render_list.empty())) {
            DWindowObject *dw = __render_list.front();
            if (! dw) continue;
            __render_list.pop_front();

            list<DWindowObject *>::iterator it = __render_list.begin();
            while (it != __render_list.end()) {
                if (*it == dw) it = __render_list.erase(it);
                else it++;
            }
            
            static cairo_t *cr = NULL;
            if (cr == NULL) {
                /* XXX: cairo need a call to this function for text
                   support to be initialized, will probably dissapear soon. */
                cairo_surface_t *surface = 
                    cairo_xlib_surface_create(meawm_ng->display,
                        DefaultRootWindow(meawm_ng->display),
                        DefaultVisual(meawm_ng->display, DefaultScreen(meawm_ng->display)),
                        DisplayWidth(meawm_ng->display, DefaultScreen(meawm_ng->display)),
                        DisplayHeight(meawm_ng->display, DefaultScreen(meawm_ng->display)));
                cr = cairo_create(surface);
            }

            dw->renderWindow(cr);

        } else {
            FD_ZERO(&rfds);
            FD_SET(xfd, &rfds);

            select(xfd + 1, &rfds, 0, 0, NULL);
        }
    }
}

void EventHandler::handleEvent(XEvent *event) {
    Window w;
    int i, rx, ry;
    EventDetail ed;
    ed.x = ed.y = INT_MAX;
    ed.type = ed.detail = ed.x11mod = ed.wamod = 0;

    switch (event->type) {
        case ConfigureRequest:
            evConfigureRequest(&event->xconfigurerequest); break;
        case PropertyNotify:
            evProperty(&event->xproperty); break;
        case UnmapNotify:
            if (event->xunmap.event != event->xunmap.window) return;
        case ReparentNotify:
        case DestroyNotify:
            evUnmapDestroy(event); break;
        case FocusOut:
        case FocusIn:
            evFocus(&event->xfocus);
            break;
        case LeaveNotify:
        case EnterNotify:
            if (event->xcrossing.mode == NotifyGrab) break;
            ed.type = event->type;
            ed.x11mod = event->xcrossing.state;
            ed.detail = 0;
            evAct(event, event->xcrossing.window, &ed);
            break;
        case KeyPress:
        case KeyRelease:
            ed.type = event->type;
            ed.x11mod = event->xkey.state;
            ed.detail = event->xkey.keycode;
            evAct(event, event->xkey.window, &ed);
            break;
        case ButtonPress:
            ed.type = ButtonPress;
            if (meawm_ng->double_click) {
                if (! meawm_ng->timer->exitsInterrupt(DOUBLECLICK_TIMER_ID)) {
                    Action *double_click_action = new Action;
                    double_click_action->delay = meawm_ng->double_click;
                    double_click_action->timer_id = DOUBLECLICK_TIMER_ID;
                    double_click_action->periodic_timer = false;
                    double_click_action->func = &AWindowObject::nop;
                    Interrupt *i = new Interrupt(double_click_action,
                                                 event, (Window) 0);
                    double_click_action->unref();
                    meawm_ng->timer->addInterrupt(i);
                    last_click_win = event->xbutton.window;
                    last_button = event->xbutton.button;
                } else {
                    if (last_click_win == event->xbutton.window &&
                        last_button == event->xbutton.button) {
                        meawm_ng->timer->removeInterrupt(DOUBLECLICK_TIMER_ID);
                        ed.type = DoubleClick;
                        last_click_win = (Window) 0;
                        last_button = 0;
                    }
                }
            }
        case ButtonRelease:
            if (event->type == ButtonRelease) ed.type = ButtonRelease;
            ed.x11mod = event->xbutton.state;
            ed.detail = event->xbutton.button;
            ed.x = event->xbutton.x;
            ed.y = event->xbutton.y;
            evAct(event, event->xbutton.window, &ed);
            break;
        case ColormapNotify:
            evColormap(&event->xcolormap); break;
        case MapRequest: {
            WindowObject *wo = evMapRequest(&event->xmaprequest);
            if (wo) {
                XQueryPointer(meawm_ng->display, event->xmaprequest.parent,
                              &w, &w, &rx, &ry, &i, &i, &(ed.x11mod));
                ed.detail = 0;
                event->xbutton.x_root = rx;
                event->xbutton.y_root = ry;
                if (wo->type == WindowType) {
                    WaWindow *ww = (WaWindow *) wo;
                    ed.type = event->type;
                    evAct(event, event->xmaprequest.window, &ed);
                    if (! ww->mapped) {
                        ww->net->setState(ww, ww->state);
                        ww->net->setVirtualPos(ww);
                    }
                } else if (wo->type == DockAppType) {
                    ed.type = DockappAddRequest;
                    evAct(event, ((Dockapp *) wo)->id, &ed);
                    if (! ((Dockapp *) wo)->dh)
                        wo->ws->addDockapp((Dockapp *) wo, NULL);
                }
            }
        } break;
        case ClientMessage:
            if (event->xclient.message_type ==
                meawm_ng->net->meawm_ng_net_event_notify) {
                ed.type = event->xclient.data.l[1];
                ed.detail = event->xclient.data.l[2];
                ed.x = event->xclient.data.l[2];
                ed.y = event->xclient.data.l[3];
                XQueryPointer(meawm_ng->display, event->xclient.window,
                              &w, &w, &rx, &ry, &i, &i, &(ed.x11mod));
                evAct(event, event->xclient.data.l[0], &ed);
            } else
                evClientMessage(event, &ed);
            break;
        default:

#ifdef    SHAPE
            if (meawm_ng->shape && event->type == meawm_ng->shape_event) {
                XShapeEvent *e = (XShapeEvent *) event;
                WaWindow *ww = (WaWindow *)
                    meawm_ng->findWin(e->window, WindowType);
                if (ww && meawm_ng->shape)
                    ww->shapeEvent();
            }
#endif // SHAPE

#ifdef    RANDR
            if (meawm_ng->randr && event->type == meawm_ng->randr_event) {
                XRRScreenChangeNotifyEvent *e =
                    (XRRScreenChangeNotifyEvent *) event;
                WaScreen *ws = (WaScreen *)
                    meawm_ng->findWin(e->window, RootType);
                if (ws) {
                    ws->width = e->width;
                    ws->height = e->height;
                    ws->rrUpdate();
                }
            }
#endif // RANDR

            break;
    }
}

void EventHandler::evProperty(XPropertyEvent *e) {
    WaWindow *ww;

    if (e->atom == XA_WM_NAME) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getXaName(ww);
            ww->drawDecor();
        }
    }
    else if (e->atom == XA_WM_CLASS) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getClassHint(ww);
            ww->drawDecor();
        }
    }
    else if (e->atom == meawm_ng->net->net_wm_pid) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getWmPid(ww);
            ww->drawDecor();
        }
    }
    else if (e->atom == meawm_ng->net->net_wm_user_time) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getWmUserTime(ww);
        }
    }
    else if (e->atom == XA_WM_CLIENT_MACHINE) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getWMClientMachineHint(ww);
            ww->drawDecor();
        }
    }
    else if (e->atom == XA_WM_TRANSIENT_FOR) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getTransientForHint(ww);
        }
    }
    else if (e->atom == XA_WM_HINTS) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getWMHints(ww);
        }
    }
    else if (e->atom == XA_WM_NORMAL_HINTS) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            unsigned int w, h;
            meawm_ng->net->getWMNormalHints(ww);
            ww->incSizeCheck(ww->attrib.width, ww->attrib.height, &w, &h);
            if (w != ww->attrib.width || h != ww->attrib.height) {
                ww->attrib.width = w;
                ww->attrib.height = h;
                ww->redrawWindow();
            }
        }
    }
    else if (e->atom == meawm_ng->net->wm_protocols) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getWMProtocols(ww);
        }
    }
    else if (e->atom == meawm_ng->net->mwm_hints) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getMWMHints(ww);
        }
    }
    else if (e->atom == meawm_ng->net->net_wm_name) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            if (! meawm_ng->net->getNetName(ww)) meawm_ng->net->getXaName(ww);
            ww->drawDecor();
        }
    }
    else if (e->atom == meawm_ng->net->net_wm_icon ||
             e->atom == meawm_ng->net->net_wm_icon_image) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getWmIconImage(ww);
        }
    }
    else if (e->atom == meawm_ng->net->net_wm_icon_svg) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
            meawm_ng->net->getWmIconSvg(ww);
        }
    }
    else if (e->atom == meawm_ng->net->net_wm_strut) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType)))
            meawm_ng->net->getWmStrut(ww);
    }
    else if (e->atom == meawm_ng->net->xrootpmap_id) {
        if (WaScreen *ws = (WaScreen *) meawm_ng->findWin(e->window, RootType))
            meawm_ng->net->getXRootPMapId(ws);
    }
}

void EventHandler::evFocus(XFocusChangeEvent *e) {
    WindowObject *wo;
    WaScreen *ws = NULL;
    EventDetail ed;
    Window w;
    int i, rx, ry;
    bool newfocus = false;

    if (e->type == FocusIn && e->window != focused) {
        if ((wo = meawm_ng->findWin(e->window,
                                  WindowType | MenuItemType | RootType))) {
            if (wo->type == RootType) {
                ws = (WaScreen *) wo;
                ws->focused = true;
                meawm_ng->net->setActiveWindow(ws, NULL);
            }
            else if (wo->type == MenuItemType) {
                ((MenuItem *) wo)->focused = true;
                ws = ((MenuItem *) wo)->ws;
                meawm_ng->net->setActiveWindow(ws, NULL);
            }
            else if (wo->type == WindowType) {
                WaWindow *wa = ((WaWindow *) wo);
                wa->has_focus = true;
                ws = wa->ws;
                meawm_ng->net->setActiveWindow(ws, wa);
            }
            
            meawm_ng->addToFocusHistory(e->window);
            
            newfocus = true;
            XQueryPointer(meawm_ng->display, ws->id, &w, &w, &rx, &ry, &i, &i,
                          &(ed.x11mod));

            ed.type = FocusIn;
            ed.detail = 0;
            ed.x = ed.y = INT_MAX;
            evAct((XEvent *) e, wo->id, &ed);
        }
        if ((wo = meawm_ng->findWin(focused,
                                  WindowType | MenuItemType | RootType))) {
            if (wo->type == RootType) {
                ws = (WaScreen *) wo;
                ws->focused = false;
            }
            else if (wo->type == MenuItemType) {
                ((MenuItem *) wo)->focused = false;
                if (((MenuItem *) wo)->menu->focus == ((MenuItem *) wo)->id)
                    ((MenuItem *) wo)->menu->focus = 0;
                ws = ((MenuItem *) wo)->ws;
            }
            else if (wo->type == WindowType) {
                WaWindow *wa = ((WaWindow *) wo);
                wa->has_focus = false;
                ws = wa->ws;
            }

            if (! newfocus)
                XQueryPointer(meawm_ng->display, ws->id, &w, &w, &rx, &ry, &i,
                              &i, &(ed.x11mod));

            ed.type = FocusOut;
            ed.detail = 0;
            ed.x = ed.y = INT_MAX;
            evAct((XEvent *) e, wo->id, &ed);
        }
        focused = e->window;
    }
}

void EventHandler::evConfigureRequest(XConfigureRequestEvent *e) {
    WindowObject *wo;
    WaWindow *ww;
    Dockapp *da;
    XWindowChanges wc;

    wc.x = e->x;
    wc.y = e->y;
    wc.width = e->width;
    wc.height = e->height;
    wc.sibling = e->above;
    wc.stack_mode = e->detail;
    wc.border_width = e->border_width;

    if ((wo = meawm_ng->findWin(e->window, WindowType | DockAppType))) {
        if (wo->type == WindowType) {
            ww = (WaWindow *) wo;
            if (e->value_mask & CWBorderWidth) ww->old_bw = e->border_width;
            if (e->value_mask & (CWX | CWY)) {
                if (e->value_mask & CWX) ww->attrib.x = e->x;
                if (e->value_mask & CWY) ww->attrib.y = e->y;
                ww->gravitate(ApplyGravity);
            }
            if (e->value_mask & CWWidth)
                ww->attrib.width = e->width;
            if (e->value_mask & CWHeight)
                ww->attrib.height = e->height;
            if (e->value_mask & CWWidth)
                ww->old_attrib.width = ww->attrib.width + 1;
            if (e->value_mask & CWHeight)
                ww->old_attrib.height = ww->attrib.height + 1;
            ww->redrawWindow();
            if (e->value_mask & CWStackMode) {
                switch (e->detail) {
                    case Above:
                        ww->raise();
                        break;
                    case Below:
                        ww->lower();
                        break;
                    case TopIf:
                        ww->alwaysontopOn();
                        break;
                    case BottomIf:
                        ww->alwaysatbottomOn();
                        break;
                    case Opposite:
                        if (ww->wstate & StateAlwaysOnTopMask)
                            ww->alwaysatbottomOn();
                        else if (ww->wstate & StateAlwaysAtBottomMask)
                            ww->alwaysontopOn();
                }
            }
            return;
        }
        else if (wo->type == DockAppType) {
            da = (Dockapp *) wo;
            if (e->value_mask & CWWidth) da->width = e->width;
            if (e->value_mask & CWHeight) da->height = e->height;
            XConfigureWindow(e->display, da->id, e->value_mask, &wc);
            if (da->dh) da->dh->update();
            return;
        }
    }
    XConfigureWindow(e->display, e->window, e->value_mask, &wc);
}

void EventHandler::evColormap(XColormapEvent *e) {
    XInstallColormap(e->display, e->colormap);
}

WindowObject *EventHandler::evMapRequest(XMapRequestEvent *e) {
    XWindowAttributes attr;
    WindowObject *wo = NULL;
    WaScreen *ws = NULL;
    XWMHints *wm_hints = NULL;
    WaWindow *ww = NULL;

    if ((ww = (WaWindow *) meawm_ng->findWin(e->window, WindowType))) {
        if (ww->wstate & StateMinimizedMask) ww->unMinimize();
    } else {
        int status = 0;
        int state = -1;
        ws = (WaScreen *) meawm_ng->findWin(e->parent, RootType);
        status = XGetWindowAttributes(e->display, e->window, &attr);
        if (! status) return NULL;
        
        wm_hints = XGetWMHints(e->display, e->window);
        
        if (wm_hints) {
            if (wm_hints->flags & StateHint)
                state = wm_hints->initial_state;
            XFree(wm_hints);
        }

        if (! ws) {
            ws = (WaScreen *) meawm_ng->findWin(attr.root, RootType);
            if (! ws) return NULL;
        }

        if (status && attr.screen && (! attr.override_redirect)) {
            if (ws->net->isSystrayWindow(e->window)) {
                if (! meawm_ng->findWin(e->window, SystrayType)) {
                    XSelectInput(ws->display, e->window,
                                 StructureNotifyMask);
                    SystrayWindow *stw = new SystrayWindow(e->window, ws);
                    meawm_ng->window_table.insert(make_pair(e->window, stw));
                    ws->systray_window_list.push_back(e->window);
                    ws->net->setSystrayWindows(ws);
                }
            } else {
                if (state == WithdrawnState) {
                    Dockapp *d = new Dockapp(ws, e->window);
                    wo = d;
                } else {
                    ww = new WaWindow(e->window, ws);
                    ws->net->setClientList(ws);
                    ws->net->setClientListStacking(ws);
                    wo = ww;
                }
            }
        }
    }
    return wo;
}

void EventHandler::evUnmapDestroy(XEvent *e) {
    DockappHandler *dh;
    WindowObject *wo;

    if ((wo = meawm_ng->findWin((e->type == UnmapNotify)?
                              e->xunmap.window:
                              (e->type == DestroyNotify)?
                              e->xdestroywindow.window:
                              e->xreparent.window,
                              WindowType | DockAppType | SystrayType))) {
        if (wo->type == WindowType) {
          if (e->type == UnmapNotify && ((WaWindow *) wo)->pending_unmaps) {
            ((WaWindow *) wo)->pending_unmaps -= 1;
            return;
          }
          
          ((WaWindow *) wo)->deleted = false;
          if (e->type == DestroyNotify)
            ((WaWindow *) wo)->deleted = true;
          else if (e->type == ReparentNotify) {
                if (e->xreparent.window != ((WaWindow *) wo)->id ||
                    e->xreparent.parent == ((WaWindow *) wo)->frame->id)
                    return;
                XEvent ev;
                ev.xreparent = e->xreparent;
                XPutBackEvent(meawm_ng->display, &ev);
                ((WaWindow *) wo)->remap = true;
            }
            delete ((WaWindow *) wo);
        }
        else if (wo->type == DockAppType) {
            if (e->type == DestroyNotify)
                ((Dockapp *) wo)->deleted = true;
            dh = ((Dockapp *) wo)->dh;
            if (dh) dh->dockapp_list.remove(((Dockapp *) wo));
            ((Dockapp *) wo)->unref();
            dh->update();
        }
        else if (wo->type == SystrayType) {
            SystrayWindow *stw = (SystrayWindow *) wo;
            meawm_ng->window_table.erase(stw->id);
            XSelectInput(stw->ws->display, stw->id, NoEventMask);
            stw->ws->systray_window_list.remove(stw->id);
            stw->ws->net->setSystrayWindows(stw->ws);
            delete stw;
        }
    }
}

void EventHandler::evClientMessage(XEvent *e, EventDetail *ed) {
    Window w;
    int i, rx, ry;
    WaWindow *ww;

    if (e->xclient.message_type == meawm_ng->net->net_active_window) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->xclient.window,
                                               WindowType))) {
            ww->raise();
            meawm_ng->focusNew(ww->id);
        }
    }
    else if (e->xclient.message_type == meawm_ng->net->meawm_ng_net_restart) {
        restart(NULL);
    }
    else if (e->xclient.message_type == meawm_ng->net->meawm_ng_net_shutdown) {
        quit(EXIT_SUCCESS);
    }
    else if (e->xclient.message_type == meawm_ng->net->net_wm_name) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->xclient.window,
                                               WindowType))) {
            meawm_ng->net->getNetName(ww);
            ww->drawDecor();
        }
    }
    else if (e->xclient.message_type == meawm_ng->net->wm_change_state) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->xclient.window,
                                               WindowType))) {
            if ((unsigned int) e->xclient.data.l[0] == IconicState)
                ww->minimize();
            else if ((unsigned int) e->xclient.data.l[0] == NormalState)
                ww->unMinimize();
            else if ((unsigned int) e->xclient.data.l[0] == WithdrawnState)
                delete ww;
        }
    }
    else if (e->xclient.message_type == meawm_ng->net->net_wm_desktop) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->xclient.window,
                                               WindowType))) {
            if ((unsigned int) e->xclient.data.l[0] == 0xffffffff ||
                (unsigned int) e->xclient.data.l[0] == 0xfffffffe) {
                ww->desktop_mask = (1L << 16) - 1;
                ww->show();
                ww->net->setDesktop(ww);
                ww->net->setDesktopMask(ww);
            }
            else if ((unsigned int) e->xclient.data.l[0] <
                ww->ws->config.desktops) {
                ww->desktop_mask =
                    (1L << (unsigned int) e->xclient.data.l[0]);
                if (ww->desktop_mask &
                    (1L << ww->ws->current_desktop->number))
                    ww->show();
                else
                    ww->hide();
                ww->net->setDesktop(ww);
                ww->net->setDesktopMask(ww);
            }
        }
    }
    else if (e->xclient.message_type ==
             meawm_ng->net->meawm_ng_net_wm_desktop_mask) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->xclient.window,
                                               WindowType))) {
            if (e->xclient.data.l[0] <
                ((1L << ww->ws->config.desktops) - 1) &&
                e->xclient.data.l[0] >= 0) {
                ww->desktop_mask = e->xclient.data.l[0];
                if (ww->desktop_mask &
                    (1L << ww->ws->current_desktop->number))
                    ww->show();
                else
                    ww->hide();
                ww->net->setDesktop(ww);
                ww->net->setDesktopMask(ww);
            }
        }
    }
    else if (e->xclient.message_type == meawm_ng->net->net_wm_state) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->xclient.window,
                                               WindowType))) {
            bool max_done = false;
            for (int i = 1; i < 3; i++) {
                if ((unsigned long) e->xclient.data.l[i] ==
                    meawm_ng->net->net_wm_state_sticky) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->unSticky(); break;
                        case _NET_WM_STATE_ADD:
                            ww->sticky(); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->toggleSticky(); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->net_wm_state_shaded) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->unShade(); break;
                        case _NET_WM_STATE_ADD:
                            ww->shade(); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->toggleShade(); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->net_wm_state_hidden) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->unMinimize(); break;
                        case _NET_WM_STATE_ADD:
                            ww->minimize(); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->toggleMinimize(); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->net_wm_maximized_vert ||
                           (unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->net_wm_maximized_horz) {
                    if (max_done) break;
                    max_done = true;
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->unMaximize(); break;
                        case _NET_WM_STATE_ADD:
                            ww->maximize(); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->toggleMaximize(); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->net_wm_state_above ||
                           (unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->net_wm_state_stays_on_top) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->alwaysontopOff(); break;
                        case _NET_WM_STATE_ADD:
                            ww->alwaysontopOn(); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->alwaysontopToggle(); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->net_wm_state_below ||
                           (unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->net_wm_state_stays_at_bottom) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->alwaysatbottomOff(); break;
                        case _NET_WM_STATE_ADD:
                            ww->alwaysatbottomOn(); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->alwaysatbottomToggle(); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->net_wm_state_skip_taskbar) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->wstate &= ~StateTasklistMask; break;
                        case _NET_WM_STATE_ADD:
                            ww->wstate |= StateTasklistMask; break;
                        case _NET_WM_STATE_TOGGLE:
                            if (ww->wstate & StateTasklistMask)
                                ww->wstate &= ~StateTasklistMask;
                            else
                                ww->wstate |= StateTasklistMask;
                            break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->net_wm_state_fullscreen) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->fullscreenOff();
                            ww->unMaximize();
                            ww->alwaysontopOff();
                            ww->decorAllOn();
                            break;
                        case _NET_WM_STATE_ADD:
                            ww->decorAllOff();
                            ww->alwaysontopOn();
                            ww->raise();
                            ww->fullscreenOn();
                            ww->maximize();
                            break;
                        case _NET_WM_STATE_TOGGLE:
                            if (ww->wstate & StateFullscreenMask) {
                                ww->fullscreenOff();
                                ww->unMaximize();
                                ww->alwaysontopOff();
                                ww->decorAllOn();
                            } else {
                                ww->decorAllOff();
                                ww->alwaysontopOn();
                                ww->raise();
                                ww->fullscreenOn();
                                ww->maximize();
                            }
                            break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->meawm_ng_net_wm_state_decor) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->decorAllOff(); break;
                        case _NET_WM_STATE_ADD:
                            ww->decorAllOn(); break;
                        case _NET_WM_STATE_TOGGLE:
                            if (ww->wstate & StateDecorAllMask)
                                ww->decorAllOff();
                            else ww->decorAllOn();
                            break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->meawm_ng_net_wm_state_decortitle) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->decorTitleOff(); break;
                        case _NET_WM_STATE_ADD:
                            ww->decorTitleOn(); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->decorTitleToggle(); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->meawm_ng_net_wm_state_decorborder) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->decorBorderOff(); break;
                        case _NET_WM_STATE_ADD:
                            ww->decorBorderOn(); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->decorBorderToggle(); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           meawm_ng->net->meawm_ng_net_wm_state_decorhandles) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->decorHandlesOff(); break;
                        case _NET_WM_STATE_ADD:
                            ww->decorHandlesOn(); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->decorHandlesToggle(); break;
                    }
                }   
            }
        }
    }
    else if (e->xclient.message_type == meawm_ng->net->xdndenter ||
             e->xclient.message_type == meawm_ng->net->xdndleave) {
        if (e->xclient.message_type == meawm_ng->net->xdndenter) {
            e->type = EnterNotify;
            ed->type = EnterNotify;
        } else {
            e->type = LeaveNotify;
            ed->type = LeaveNotify;
        }

        if (WaScreen *ws = (WaScreen *) meawm_ng->findWin(e->xclient.window,
                                                        RootType)) {
            XQueryPointer(ws->display, ws->id, &w, &w, &rx, &ry, &i, &i,
                          &(ed->x11mod));
        } else {
            rx = 0;
            ry = 0;
        }
        ed->detail = 0;
        e->xcrossing.x_root = rx;
        e->xcrossing.y_root = ry;

        evAct(e, e->xclient.window, ed);
    }
    else if (e->xclient.message_type == meawm_ng->net->net_desktop_viewport) {
        if (WaScreen *ws = (WaScreen *) meawm_ng->findWin(e->xclient.window,
                                                        RootType))
            ws->moveViewportTo(e->xclient.data.l[0], e->xclient.data.l[1]);
    }
    else if (e->xclient.message_type == meawm_ng->net->net_close_window) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->xclient.window, WindowType)))
            ww->close();
    }
    else if (e->xclient.message_type == meawm_ng->net->net_current_desktop) {
        if (WaScreen *ws = (WaScreen *) meawm_ng->findWin(e->xclient.window,
                                                        RootType))
            ws->goToDesktop(e->xclient.data.l[0]);
    }
    else if (e->xclient.message_type == meawm_ng->net->net_moveresize_window) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->xclient.window,
                                               WindowType))) {

            char gravity = (char) e->xclient.data.l[0];
            if (gravity == 0) gravity = (char) ww->size.win_gravity;

            int x = ww->attrib.x;
            int y = ww->attrib.y;
            int width = ww->attrib.width;
            int height = ww->attrib.height;

            if (e->xclient.data.l[0] & (1L << 8)) x = e->xclient.data.l[1];
            if (e->xclient.data.l[0] & (1L << 9)) y = e->xclient.data.l[2];
            if (e->xclient.data.l[0] & (1L << 10))
                width = e->xclient.data.l[3];
            if (e->xclient.data.l[0] & (1L << 11))
                height = e->xclient.data.l[4];

            ww->incSizeCheck(width, height, &ww->attrib.width,
                             &ww->attrib.height);

            if (gravity != StaticGravity)
                ww->gravitate(RemoveGravity);
            if (gravity == NorthEastGravity ||
                gravity == EastGravity ||
                gravity == SouthEastGravity)
                ww->attrib.x = ww->ws->width - x - ww->attrib.width;
            else ww->attrib.x = x;

            if (gravity == SouthWestGravity ||
                gravity == SouthGravity ||
                gravity == SouthEastGravity)
                ww->attrib.y = ww->ws->height - y - ww->attrib.height;
            else ww->attrib.y = y;

            if (gravity == NorthGravity ||
                gravity == SouthGravity ||
                gravity == CenterGravity)
                ww->attrib.x -= ww->attrib.width / 2;

            if (gravity == EastGravity ||
                gravity == WestGravity ||
                gravity == CenterGravity)
                ww->attrib.y -= ww->attrib.height / 2;

            if (gravity != StaticGravity)
                ww->gravitate(ApplyGravity);

            ww->redrawWindow();
            ww->checkMoveMerge(ww->attrib.x, ww->attrib.y);
        }
    }
    else if (e->xclient.message_type == meawm_ng->net->net_wm_moveresize) {
        if ((ww = (WaWindow *) meawm_ng->findWin(e->xclient.window,
                                               WindowType))) {
            if (e->xclient.data.l[2] == _NET_WM_MOVERESIZE_MOVE ||
                e->xclient.data.l[2] == _NET_WM_MOVERESIZE_MOVE_KEYBOARD)
                ww->moveOpaque(e);
            else if (e->xclient.data.l[2] == _NET_WM_MOVERESIZE_SIZE_TOPLEFT)
                ww->resizeOpaque(e, WestResizeTypeMask | NorthResizeTypeMask);
            else if (e->xclient.data.l[2] ==
                     _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT)
                ww->resizeOpaque(e, WestResizeTypeMask | SouthResizeTypeMask);
            else if (e->xclient.data.l[2] ==
                     _NET_WM_MOVERESIZE_SIZE_LEFT)
                ww->resizeOpaque(e, WestResizeTypeMask);
            else if (e->xclient.data.l[2] ==
                     _NET_WM_MOVERESIZE_SIZE_TOPRIGHT)
                ww->resizeOpaque(e, EastResizeTypeMask | NorthResizeTypeMask);
            else if (e->xclient.data.l[2] ==
                     _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT)
                ww->resizeOpaque(e, EastResizeTypeMask | SouthResizeTypeMask);
            else if (e->xclient.data.l[2] ==
                     _NET_WM_MOVERESIZE_SIZE_RIGHT)
                ww->resizeOpaque(e, EastResizeTypeMask);
            else if (e->xclient.data.l[2] ==
                     _NET_WM_MOVERESIZE_SIZE_TOP)
                ww->resizeOpaque(e, NorthResizeTypeMask);
            else if (e->xclient.data.l[2] ==
                     _NET_WM_MOVERESIZE_SIZE_BOTTOM)
                ww->resizeOpaque(e, SouthResizeTypeMask);
        }  
    }
    else if (e->xclient.message_type == meawm_ng->net->meawm_ng_net_cfg) {
        WaScreen *ws = (WaScreen *) meawm_ng->findWin(e->xclient.window,
                                                    RootType);
        if (ws)
            meawm_ng->net->getConfig(ws, e->xclient.data.l[0],
                                   e->xclient.data.l[1], e->xclient.data.l[2]);
    }
}

void EventHandler::evAct(XEvent *e, Window win, EventDetail *ed) {
    AWindowObject *awo;

    map<Window, WindowObject *>::iterator it;
    if ((it = meawm_ng->window_table.find(win)) !=
        meawm_ng->window_table.end()) {
        if (((*it).second)->type & ~ANY_ACTION_WINDOW_TYPE) return;
        awo = (AWindowObject *) (*it).second;
        
        if (meawm_ng->eh->move_resize != EndMoveResizeType)
            ed->wamod |= MoveResizeMask;

        awo->evalState(ed);
        
        if (awo->type == MenuItemType) {
            if (ed->type == StateAddNotify) {
                ((MenuItem *) awo)->mstate = true;
                ((MenuItem *) awo)->pushRenderEvent();
            }
            else if (ed->type == StateRemoveNotify) {
                ((MenuItem *) awo)->mstate = false;
                ((MenuItem *) awo)->pushRenderEvent();
            }
            if (((MenuItem *) awo)->mstate)
                ed->wamod |= StateTrueMask;
        }

        WaWindow *wawin = awo->getWindow();
        if (wawin) ed->wamod |= wawin->wstate;

        awo->ref();

        bool status = awo->handleEvent(e, ed);

        if (awo->type == WindowType) {
            if (! status) {
                XAllowEvents(meawm_ng->display, ReplayPointer, CurrentTime);
                XAllowEvents(meawm_ng->display, ReplayKeyboard, CurrentTime);
            } else {
                XAllowEvents(meawm_ng->display, AsyncPointer, CurrentTime);
                XAllowEvents(meawm_ng->display, AsyncKeyboard, CurrentTime);
            }
        }

        if (ed->type == WindowStateChangeNotify &&
            (awo->type & ANY_DECOR_WINDOW_TYPE)) {
            DWindowObject *dwo = (DWindowObject *) awo;
            map<int, SubWindowObject *>::iterator it = dwo->subs.begin();
            for (; it != dwo->subs.end(); it++)
                evAct(NULL, ((*it).second)->id, ed);
        }

        awo->unref();
    }
}
