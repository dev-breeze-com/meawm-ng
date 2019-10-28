/* Meawm_NG.cc

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
#include <X11/Xlocale.h>

#include <cairo.h>
    
#ifdef    SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE

#ifdef    XINERAMA
#  include <X11/extensions/Xinerama.h>
#endif // XINERAMA

#ifdef    RANDR
#  include <X11/extensions/Xrandr.h>
#endif // RANDR

#ifdef    RENDER
#  include <X11/extensions/Xrender.h>
#endif // RENDER

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#endif // STDC_HEADERS

#ifdef    HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif // HAVE_SYS_TYPES_H

#ifdef    HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#endif // HAVE_SYS_WAIT_H

#ifdef    HAVE_UNISTD_H
#  include <unistd.h>
#endif // HAVE_UNISTD_H
    
#ifdef    HAVE_ERRNO_H
#  include <errno.h>
#endif // HAVE_ERRNO_H

#ifdef    HAVE_CODESET
#  include <langinfo.h>
#endif // HAVE_CODESET
    
}

#include "Meawm_NG.hh"
#include "Window.hh"

Meawm_NG *meawm_ng = NULL;

char *meawm_ng_pathenv;
int grab_count = 0;
char **argv;
bool hush;
int errors;

XClientMessageEvent cme;

list<DWindowObject *> __render_list;

#ifdef    THREAD
unsigned int __render_thread_count;

pthread_mutex_t __render_mutex;
pthread_cond_t  __render_cond;
unsigned int    __render_count;

pthread_mutex_t __render_list_mutex;
pthread_cond_t  __render_list_cond;
#endif // THREAD

Meawm_NG::Meawm_NG(char **av, char **_options) {
    struct sigaction action;
    int dummy;
    EventDetail ed;
    char render_info[256];
    render_info[0] = '\0';
    config_info[0] = '\0';

    dummy = 0;
    argv = av;
    options = _options;
    running = false;

#ifdef    THREAD
    __render_thread_count = 0;
    if (! XInitThreads()) {
        cerr << "error: xlib is not thread-safe" << endl;
        exit(1);
    }
#endif // THREAD
    
    XSetErrorHandler((XErrorHandler) xerrorhandler);
    if (! (display = XOpenDisplay(options[ARG_DISPLAY]))) {
        cerr << "error: can't open display: " << options[ARG_DISPLAY] << endl;
        exit(1);
    }

#ifdef    DEBUG
    XSynchronize(display, true);
#endif // DEBUG
    
    meawm_ng = this;
    hush = wmerr = false;
    errors = 0;
    eh = NULL;
    timer = NULL;
    quit_signal = restart_signal = unknown_signal = false;
    prefocus = (Window) 0;

    action.sa_handler = signalhandler;
    action.sa_mask = sigset_t();
    action.sa_flags = SA_NOCLDSTOP | SA_NODEFER;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGCHLD, &action, NULL);
    sigaction(SIGHUP, &action, NULL);

    action_create_tsts();
    render_create_tsts();
    parser_create_tsts();
    
    setlocale(LC_ALL, "");
    
#ifdef    HAVE_ICONV

#ifdef    HAVE_CODESET
    char *fromcode = nl_langinfo(CODESET);
#else //  !HAVE_CODESET
    char *fromcode = "ASCII";
#endif // HAVE_CODESET

    utf8conv = iconv_open(INTERNALCODE, fromcode);
    if (utf8conv == (iconv_t) -1) {
        utf8conv = (iconv_t) 0;
        WARNING; perror(NULL);
    }
#endif // HAVE_ICONV
   
    cursor = new WaCursor(display);
    
#ifdef    SHAPE
    shape_event = 0;
    shape = XShapeQueryExtension(display, &shape_event, &dummy);
#endif // SHAPE

#ifdef    XINERAMA
    xinerama = XineramaQueryExtension(display, &dummy, &dummy);
    if (xinerama)
        xinerama = XineramaIsActive(display);
    else
        xinerama = false;

    if (xinerama) {
        xinerama_info = XineramaQueryScreens(display, &xinerama_info_num);
    }
#endif // XINERAMA

#ifdef    RANDR
    randr_event = 0;
    randr = XRRQueryExtension(display, &randr_event, &dummy);
#endif // RANDR

    rh = new ResourceHandler(this, options);
    net = new NetHandler(this);

    rh->loadConfig(this);

    if (! client_side_rendering) {
        if (XRenderQueryExtension(display, &dummy, &dummy)) {
            int major, minor;
            XRenderQueryVersion(display, &major, &minor);
            if (major == 0 && minor < 6) {
                snprintf(render_info, 256,
                         "RENDER extension version on display %s "
                         "is %d.%d. Version 0.6 is required for server-side "
                         "rendering. Client-side rendering forced.",
                         DisplayString(display), major, minor);
                client_side_rendering = true;
            }
        } else {
            snprintf(render_info, 256, "RENDER extension missing on display"
                     "%s. Client-side rendering forced.",
                     DisplayString(display));
            client_side_rendering = true;
        }
    }
    
    eh = new EventHandler(this);
    timer = new Timer(this);

#ifdef    THREAD
    __render_count = 0;
    if (__render_thread_count) {
        pthread_mutex_init(&__render_mutex, NULL);
        pthread_cond_init(&__render_cond, NULL);
        pthread_mutex_init(&__render_list_mutex, NULL);
        pthread_cond_init(&__render_list_cond, NULL);
    }
#endif // THREAD

    XDisplayKeycodes(display, &min_key, &max_key);
    getModifierMappings();

    int i, screens = 0;
    
    for (i = 0; i < ScreenCount(display); ++i) {
        if (screenmask & (1L << i)) {
            WaScreen *ws = new WaScreen(display, i, this);
            if (! wmerr) {
                ws->commonStyleUpdate();
                wascreen_list.push_back(ws);
                screens++;
            } else {
                delete ws;
                wmerr = false;
            }
        }
    }
    if (! screens) {
        ERROR << "no managable screens found on display " <<
            DisplayString(display) << endl;
        exit(1);
    }

    if (*render_info != '\0')
        wascreen_list.front()->showInfoMessage(__FUNCTION__, render_info);

    if (*config_info != '\0')
        wascreen_list.front()->showInfoMessage(__FUNCTION__, config_info);
    
    XrmDestroyDatabase(rh->database);

    running = true;
    
    ed.type = MapRequest;
    ed.detail = 0;
    ed.x11mod = ed.wamod = 0;
    list<WaScreen *>::iterator it = wascreen_list.begin();
    for (; it != wascreen_list.end(); it++)
        eh->evAct(NULL, (*it)->id, &ed);

#ifdef    THREAD
    if (__render_thread_count) {
        pthread_t t;
        pthread_attr_t attr;
        struct sched_param param;
        param.sched_priority = render_thread_prio;
        pthread_attr_init(&attr);
        pthread_attr_setschedparam(&attr, &param);
        for (unsigned int i = 0; i < __render_thread_count; i++)
            pthread_create(&t, &attr, render_thread_func, NULL);
        pthread_attr_destroy(&attr);
    }
#endif // THREAD

}

Meawm_NG::~Meawm_NG(void) {
    running = false;
    
    delete timer;
    
    RENDER_GET;
    
    LISTDEL(wascreen_list);

    XSync(display, false);

    delete rh;
    delete cursor;

#ifndef   RANDR 
    XCloseDisplay(display);  /* XXX: crashes if linked to librandr */
#endif // RANDR

}

void Meawm_NG::getModifierMappings(void) {
    while (! modmaps.empty()) {
        delete [] modmaps.back()->name;
        delete modmaps.back();
        modmaps.pop_back();
    }
    
    const XModifierKeymap* const modmap = XGetModifierMapping(display);

    if (modmap && modmap->max_keypermod > 0) {
        const int mask_table[] = {
            ShiftMask, LockMask, ControlMask, Mod1Mask,
            Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
        };
        const size_t size = (sizeof(mask_table) / sizeof(mask_table[0])) *
            modmap->max_keypermod;
        
        for (size_t i = 0; i < size; ++i) {
            if (! modmap->modifiermap[i]) continue;
            KeySym ksym = XKeycodeToKeysym(display, modmap->modifiermap[i], 0);
            if (ksym) {
                char *kstring = XKeysymToString(ksym);
                if (kstring) {
                    int modmask = mask_table[i / modmap->max_keypermod];
                    ModifierMap *mm = new ModifierMap;
                    mm->name = WA_STRDUP(kstring);
                    mm->modifier = modmask;
                    modmaps.push_back(mm);
                }
            }
        }
        if (modmap) XFreeModifiermap(const_cast<XModifierKeymap*>(modmap));
    }
}

WindowObject *Meawm_NG::findWin(Window id, long int mask) {
    map<Window, WindowObject *>::iterator it;
    if ((it = window_table.find(id)) != window_table.end()) {
        if (((*it).second)->type & mask)
            return (*it).second;
    }
    return NULL;
}

bool Meawm_NG::focusNew(Window win, bool make_vis) {
    bool status = false;
    map<Window, WindowObject *>::iterator it;
    if ((it = window_table.find(win)) != window_table.end()) {
        switch (((*it).second)->type) {
            case RootType: {
                WaScreen *ws = (WaScreen *) (*it).second;
                if (ws->style && (! ws->style->focusable)) return status;
                ws->focused = true;
                prefocus = win;
                XInstallColormap(display, ws->colormap);
                XSetInputFocus(display, win, RevertToPointerRoot, CurrentTime);
                status = true;
            } break;
            case WindowType: {
                WaWindow *ww = (WaWindow *) (*it).second;
                int newvx, newvy, x, y;
                XEvent e;
                if (ww->master) ww->hidden = ww->master->hidden;
                if ((! (ww->wstate & StateFocusableMask)) ||
                    ((! make_vis) && (ww->hidden ||
                                      (ww->wstate & StateMinimizedMask))))
                    return status;
                
                if (ww->mapped) {
                    if (make_vis) {
                        if (ww->wstate & StateMinimizedMask) ww->unMinimize();
                        if (! (ww->desktop_mask &
                               (1L << ww->ws->current_desktop->number))) {
                            list<Desktop *>::iterator dit =
                                ww->ws->desktop_list.begin();
                            for (; dit != ww->ws->desktop_list.end(); dit++)
                                if (ww->desktop_mask &
                                    (1L << (*dit)->number)) {
                                    ww->ws->goToDesktop((*dit)->number);
                                    break;
                                }
                        }
                        bool x_toolarge = false, y_toolarge = false;
            
                        if ((ww->ws->v_x + ww->attrib.x) >=
                            (ww->ws->v_xmax + (int) ww->ws->width))
                            x_toolarge = true;
            
                        if ((ww->ws->v_y + ww->attrib.y) >=
                            (ww->ws->v_ymax + (int) ww->ws->height))
                            y_toolarge = true;

                        if (x_toolarge || y_toolarge) {
                            int th = ww->attrib.height + ww->bottom_spacing;
                            int tw = ww->attrib.width + ww->right_spacing;
                
                            if (x_toolarge)
                                ww->attrib.x =
                                    (ww->ws->v_xmax + ww->ws->width -
                                     ww->ws->v_x) - tw;
                            if (y_toolarge)
                                ww->attrib.y =
                                    (ww->ws->v_ymax + ww->ws->height -
                                     ww->ws->v_y) - th;
                            ww->redrawWindow();
                        }

                        if (ww->attrib.x >= (int) ww->ws->width ||
                            ww->attrib.y >= (int) ww->ws->height ||
                            (ww->attrib.x + (int) ww->attrib.width) <= 0 ||
                            (ww->attrib.y + (int) ww->attrib.height) <= 0) {
                            x = ww->ws->v_x + ww->attrib.x;
                            y = ww->ws->v_y + ww->attrib.y;
                            newvx = (x / ww->ws->width) * ww->ws->width;
                            newvy = (y / ww->ws->height) * ww->ws->height;
                            ww->ws->moveViewportTo(newvx, newvy);
                            XSync(display, false);
                            while (XCheckTypedEvent(display, EnterNotify, &e));
                        }
                        if (ww->mergedback) ww->toFront();
                    } else if (ww->mergedback) return status;
                    XInstallColormap(display, ww->attrib.colormap);
                    if (ww->input_field) {
                        prefocus = ww->id;
                        XSetInputFocus(display, ww->id,
                                       RevertToPointerRoot,
                                       CurrentTime);
                        status = true;
                    } else if (ww->protocol_mask & TakeFocusProtocalMask) {
                        prefocus = ww->id;
                        
                        e.type = ClientMessage;
                        e.xclient.window = ww->id;
                        e.xclient.message_type = net->wm_protocols;
                        e.xclient.format = 32;
                        e.xclient.data.l[0] = net->wm_take_focus;

                        /* XXX: we shouldn't use CurrentTime here */
                        e.xclient.data.l[1] = CurrentTime;
                        
                        XSendEvent(display, ww->id, false, NoEventMask, &e);
                        status = true;
                    } else {
                        /* XXX: no input window shouldn't get input focus
                           according to ICCCM 2.0. We give it input focus
                           anyway, is this bad? */
                        prefocus = ww->id;
                        XSetInputFocus(display, ww->id,
                                       RevertToPointerRoot,
                                       CurrentTime);
                        status = true;
                    }
                } else
                    ww->want_focus = true;
            } break;
            case MenuItemType: {
                MenuItem *mi = (MenuItem *) (*it).second;
                if (mi->style && (! mi->style->focusable)) return status;
                if (mi->menu->mapped) {
                    mi->focused = true;
                    mi->menu->focus = prefocus = mi->id;
                    XSetInputFocus(display, mi->id, RevertToPointerRoot,
                                   CurrentTime);
                    status = true;
                } break;
            }
        }
    }
    return status;
}

void Meawm_NG::addToFocusHistory(Window win) {
    if (win == None) return;
    focus_history.remove(win);
    focus_history.push_front(win);
}

void Meawm_NG::removeFromFocusHistory(Window win) {
    if (win == None) return;
    focus_history.remove(win);
}

void Meawm_NG::focusRevertFrom(WaScreen *ws, Window win) {
    list<Window>::iterator it = focus_history.begin();
    for (;it != focus_history.end(); it++) {
        if (ws->config.revert_to_window && *it == ws->id)
            continue;
        if (*it != win) {
            if (focusNew(*it)) break;
        }
    }
    if (it == focus_history.end()) {
        list<WaWindow *>::iterator wit = ws->wawindow_list.begin();
        for (; wit != ws->wawindow_list.end(); wit++)
            if (! (*wit)->hidden &&
                (! ((*wit)->wstate & StateMinimizedMask))) {
                if (focusNew((*wit)->id))
                    break;
            }
        if (wit == ws->wawindow_list.end())
            focusNew(ws->id);
    }
}

void wa_grab_server(void) {
    if (grab_count == 0) {
        grab_count++;
        XGrabServer(meawm_ng->display);
    }
    
#ifdef    DEBUG
    else {
        WARNING << "server allready grabbed" << endl;
        abort();
    }
#endif // DEBUG
    
}

void wa_ungrab_server(void) {
#ifdef    DEBUG
    if (grab_count == 0) {
        WARNING << "server not grabbed" << endl;
        abort();
    } else if (grab_count != 1) {
        WARNING << "server grabbed more than once" << endl;
        abort();
    }
#endif // DEBUG

    if (grab_count > 0) {
        XUngrabServer(meawm_ng->display);
        grab_count = 0;
    }
}

bool validate_drawable(Drawable d, unsigned int *w, unsigned int *h) {
    int ret, _d;
    unsigned int _ud;
    Window _wd;

    XSync(meawm_ng->display, false);
    
    XEvent e;
    if (XCheckTypedWindowEvent(meawm_ng->display, d, DestroyNotify, &e)) {
        XPutBackEvent(meawm_ng->display, &e);
        return false;
    }
    
    errors = 0;
    hush = 1;
    if (w && h)
        XGetGeometry(meawm_ng->display, d, &_wd, &_d, &_d, w, h, &_ud, &_ud);
    else
        XGetGeometry(meawm_ng->display, d, &_wd, &_d, &_d, &_ud, &_ud, &_ud,
                     &_ud);
    
    XSync(meawm_ng->display, false);

    hush = 0;
    ret = ( errors == 0 );
    errors = 0;
    return ret;
}

const bool validate_window_mapped(Window id) {
    XSync(meawm_ng->display, false);
    
    XEvent e;
    if (XCheckTypedWindowEvent(meawm_ng->display, id, DestroyNotify, &e) ||
        XCheckTypedWindowEvent(meawm_ng->display, id, UnmapNotify, &e)) {
        XPutBackEvent(meawm_ng->display, &e);
        return false;
    }
    return true;
}

int xerrorhandler(Display *d, XErrorEvent *e) {
    char buff[128];
    map<Window, WindowObject *>::iterator it;

    errors++;

    if (! hush) {
        bool error_output;
        switch (e->request_code) {
            case X_CreateWindow:
            case X_DestroyWindow:
            case X_CreatePixmap:
            case X_FreePixmap:
                error_output = true;
                break;
            default:
                error_output = false;
                break;
        }
        
#ifdef    DEBUG
        bool error_abort = error_output;
        error_output = true;
#endif // DEBUG

        if (error_output) {
            XGetErrorDatabaseText(d, "XlibMessage", "XError", "", buff, 128);
            cerr << buff;
            XGetErrorText(d, e->error_code, buff, 128);
            cerr << ":  " << buff << endl;
            XGetErrorDatabaseText(d, "XlibMessage", "MajorCode", "%d",
                                  buff, 128);
            cerr << "  ";
            fprintf(stderr, buff, e->request_code);
            sprintf(buff, "%d", e->request_code);
            XGetErrorDatabaseText(d, "XRequest", buff, "%d", buff, 128);
            cerr << " (" << buff << ")" << endl;
            XGetErrorDatabaseText(d, "XlibMessage", "MinorCode", "%d",
                                  buff, 128);
            cerr << "  ";
            fprintf(stderr, buff, e->minor_code);
            cerr << endl;
            XGetErrorDatabaseText(d, "XlibMessage", "ResourceID", "%d",
                                  buff, 128);
            cerr << "  ";
            fprintf(stderr, buff, e->resourceid);
        }
        
        if ((it = meawm_ng->window_table.find(e->resourceid)) !=
            meawm_ng->window_table.end()) {
            if (((*it).second)->type == WindowType) {
                if (error_output) {
                    cerr << " (" << ((WaWindow *) (*it).second)->name << ")";
                }
                ((WaWindow *) (*it).second)->deleted = true;
            } else if (((*it).second)->type == DockAppType) {
                if (error_output) {
                    cerr << " (" << ((Dockapp *) (*it).second)->name << ")";
                }
                ((Dockapp *) (*it).second)->deleted = true;
            }
        }

        if (error_output) cerr << endl;
            
#ifdef    DEBUG
            //if (error_abort) abort();
#endif // DEBUG
        
    }
    return 0;
}

int wmrunningerror(Display *, XErrorEvent *) {
    meawm_ng->wmerr = true;
    return 0;
}

void signalhandler(int sig) {
    int status;

    switch (sig) {
        case SIGCHLD:
            waitpid(-1, &status, WNOHANG | WUNTRACED);
            break;
        case SIGINT:
            meawm_ng->quit_signal = true;
            break;
        case SIGHUP:
            meawm_ng->restart_signal = true;
            break;
        default:
            meawm_ng->unknown_signal = true;
            break;
    }
}

void restart(char *command) {
    char *tmp_argv[128];

    cme.type = ClientMessage;
    cme.display = meawm_ng->display;
    cme.format = 32;
    cme.window = meawm_ng->wascreen_list.front()->id;
    cme.data.l[0] = meawm_ng->wascreen_list.front()->id;
    cme.data.l[1] = RestartNotify;
    cme.data.l[2] = cme.data.l[3] = cme.data.l[4] = 0l;
    meawm_ng->eh->handleEvent((XEvent *) &cme);

    if (command) {
        commandline_to_argv(WA_STRDUP(command), tmp_argv, 128);
        meawm_ng->running = false;
        delete meawm_ng;
        execvp(*tmp_argv, tmp_argv);
        WARNING;
        perror(*tmp_argv);
        exit(EXIT_FAILURE);
    } else {
        delete meawm_ng;
	}

    execvp(argv[0], argv);
    perror(argv[0]);
    WARNING;
    exit(EXIT_FAILURE);
}

void quit(int status) {
    cme.type = ClientMessage;
    cme.display = meawm_ng->display;
    cme.format = 32;
    cme.window = meawm_ng->wascreen_list.front()->id;
    cme.data.l[0] = meawm_ng->wascreen_list.front()->id;
    cme.data.l[1] = RestartNotify;
    cme.data.l[2] = cme.data.l[3] = cme.data.l[4] = 0l;
    meawm_ng->eh->handleEvent((XEvent *) &cme);

    meawm_ng->running = false;
    delete meawm_ng;
    exit(status);
}

char *expand(char *org, WaWindow *w, MenuItem *m,
             const char *warning_function, char *warning_message,
             MenuItem *m2) {
    int i;
    char *insert, *expanded, *tmp;
    bool cont, found = false;
    char insert_buf[1024];
    int offset;

    if (! org) return NULL;

    expanded = org;
    for (i = 0; expanded[i] != '\0';) {
        for (; expanded[i] != '\0' && expanded[i] != '%'; i++);
        if (expanded[i] == '\0') break;
        insert = "";
        offset = 2;
        cont = true;
        if (w) {
            cont = false;
            switch (expanded[i + 1]) {
                case 'r': {
                    insert = w->rid;
                } break;
                case 't':
                    insert = w->name;
                    break;
                case 'P':
                    insert = w->pid;
                    break;
                case 's':
                    insert = w->host;
                    break;
                case 'c':
                    insert = w->wclass;
                    break;
                case 'C':
                    insert = w->wclassname;
                    break;
                case 'x':
                    snprintf(insert_buf, 1024, "%d", w->frame->attrib.x);
                    insert = insert_buf;
                    break;
                case 'y':
                    snprintf(insert_buf, 1024, "%d", w->frame->attrib.y);
                    insert = insert_buf;
                    break;
                case 'X':
                    snprintf(insert_buf, 1024, "%d",
                             w->ws->v_x + w->frame->attrib.x);
                    insert = insert_buf;
                    break;
                case 'Y':
                    snprintf(insert_buf, 1024, "%d",
                             w->ws->v_y + w->frame->attrib.y);
                    insert = insert_buf;
                    break;
                case 'w':
                    snprintf(insert_buf, 1024, "%d", w->attrib.width /
                             w->size.width_inc);
                    insert = insert_buf;
                    break;
                case 'h':
                    snprintf(insert_buf, 1024, "%d", w->attrib.height /
                             w->size.height_inc);
                    insert = insert_buf;
                    break;
                case 'W':
                    snprintf(insert_buf, 1024, "%d", w->attrib.width);
                    insert = insert_buf;
                    break;
                case 'H':
                    snprintf(insert_buf, 1024, "%d", w->attrib.height);
                    insert = insert_buf;
                    break;
                default:
                    cont = true;
            }
        }
        if (m && cont) {
            cont = false;
            char *exp2;
            switch (expanded[i + 1]) {
                case 'm':
                    if (m->str_dynamic) {
                        exp2 = expand(m->str, w, NULL, NULL, NULL, m);
                        strncpy(insert_buf, exp2, 1023);
                        insert_buf[1023] = '\0';
                        delete [] exp2;
                        insert = insert_buf;
                    } else
                        insert = m->str;
                    break;
                case 'M':
                    if (m->str2_dynamic) {
                        exp2 = expand(m->str2, w, NULL, NULL, NULL, m);
                        strncpy(insert_buf, exp2, 1023);
                        insert_buf[1023] = '\0';
                        delete [] exp2;
                        insert = insert_buf;
                    } else
                        insert = m->str2;
                    break;
                default:
                    cont = true;
            }
        }
        if (m2 && cont) {
            cont = false;
            switch (expanded[i + 1]) {
                case 'S':
                    if (m2->monitor_state) {
                        if (m2->mstate)
                            insert = "true";
                        else
                            insert = "false";
                    }
                    break;
                default:
                    cont = true;
            }
        }
        if (cont) {
            cont = false;
            switch (expanded[i + 1]) {
                case 'd': {
                    char *format = NULL;
                    char tmp_char = expanded[i + 2];
                    if (expanded[i + 2] == '(') {
                        format = &expanded[i + 3];
                        for (offset = 3; expanded[i + offset] != '\0' &&
                                 expanded[i + offset] != ')'; offset++);
                        tmp_char = expanded[i + offset];
                        if (tmp_char == ')')
                            expanded[i + offset] = '\0';
                    }
                    
#ifdef    HAVE_STRFTIME
                    time_t ttmp = time(NULL);
                    if (ttmp != -1) {
#ifdef    THREAD
#  ifdef    HAVE_LOCALTIME_R
                        struct tm tt_r;
                        localtime_r(&ttmp, &tt_r);
                        struct tm *tt = &tt_r;
#  else  // !HAVE_LOCALTIME_R
                        struct tm *tt = localtime(&ttmp);
#  endif // HAVE_LOCALTIME_R
#else  // !THREAD
                        struct tm *tt = localtime(&ttmp);
#endif // THREAD
                        
                        if (tt) {
                            int len = 0;
                            if (format)
                                len = strftime(insert_buf, 1024, format, tt);
                            if (len == 0) {
                                char def[3];
                                def[0] = '%';
                                def[1] = 'c';
                                def[2] = '\0';
                                strftime(insert_buf, 1024, def, tt);
                            }
                            insert = insert_buf;
                        }
                    }
#else // !HAVE_STRFTIME
                    insert = "00:00";
#endif // HAVE_STRFTIME

                    expanded[i + offset] = tmp_char;
                    if (tmp_char != '\0') offset += 1;
                } break;
                case 'f':
                    if (warning_function)
                        insert = (char *) warning_function;
                    break;
                case 'm':
                    if (warning_message)
                        insert = warning_message;
                    break;
                default:
                    cont = true;
            }
        }
        
        int ilen = strlen(insert);
        tmp = new char[strlen(expanded) + ilen + 1];
        expanded[i] = '\0';
        sprintf(tmp, "%s%s%s", expanded, insert, &expanded[i + offset]);
        if (found) delete [] expanded;
        else expanded[i] = '%';
        expanded = tmp;
        found = true;
        i += ilen;
    }
    if (found) return expanded;
    else return WA_STRDUP(org);
}

char *preexpand(char *org, bool *dynamic) {
    int i;
    char *insert, *expanded, *tmp;
    bool cont, found = false;
    int offset;
    char insert_buf[1024];
    
    *dynamic = false;
    if (! org) return NULL;

    expanded = org;
    for (i = 0; expanded[i] != '\0';) {
        cont = false;
        for (; expanded[i] != '\0' && expanded[i] != '%'; i++);
        if (expanded[i] == '\0') break;
        if (expanded[i + 1] == '\0') break;
        insert = "";
        offset = 2;
        switch (expanded[i + 1]) {
            case 'v':
                insert = VERSION;
                break;
            case 'p':
                insert = PACKAGE;
                break;
            case 'D':
                insert = __DATE__;
                break;
            case 'T':
                insert = __TIME__;
                break;
            case 'E': {
                char *extstr = insert_buf;
                extstr[0] = '\0';

                if (! meawm_ng->client_side_rendering)
                    extstr += sprintf(extstr, "RENDER ");
                
#ifdef    SHAPE
                if (meawm_ng->shape)
                    extstr += sprintf(extstr, "SHAPE ");
#endif // SHAPE

#ifdef    XINERAMA
                if (meawm_ng->xinerama)
                    extstr += sprintf(extstr, "XINERAMA ");
#endif // XINERAMA

#ifdef    RANDR
                if (meawm_ng->randr)
                    extstr += sprintf(extstr, "RANDR ");
#endif // RANDR
                
                insert = insert_buf;
            } break;
            case 'F':
                insert = ""
                    
#ifdef   THREAD
    "thread " 
#endif // THREAD

#ifdef    SHAPE
    "shape "
#endif // SHAPE

#ifdef    XINERAMA
    "xinerama "
#endif // XINERAMA

#ifdef    RANDR
    "randr "
#endif // RANDR

#ifdef    COMPOSITE
    "composite "
#endif // COMPOSITE

#ifdef    PNG
    "png "
#endif // PNG

#ifdef    SVG
    "svg "
#endif // SVG

#ifdef    XCURSOR
    "xcursor "
#endif // XCURSOR

                    ;
                break;
            case 'e': {
                char *env_var = NULL;
                char tmp_char = expanded[i + 2];
                if (expanded[i + 2] == '(') {
                    env_var = &expanded[i + 3];
                    for (offset = 3; expanded[i + offset] != '\0' &&
                             expanded[i + offset] != ')'; offset++);
                    tmp_char = expanded[i + offset];
                    if (tmp_char == ')')
                        expanded[i + offset] = '\0';
                }

                insert = NULL;
                if (env_var) insert = getenv(env_var);
                
                if (! insert) insert = "";
                
                expanded[i + offset] = tmp_char;
                if (tmp_char != '\0') offset += 1;
            } break;
            case 'u':
                insert = getenv("USER");
                if (! insert) insert = "";
                break;
            case 'n':
                insert = "\n";
                break;
            case 'd':
                if (expanded[i + 2] == '(') {
                    for (offset = 3; expanded[i + offset] != '\0' &&
                             expanded[i + offset] != ')'; offset++);
                    i += offset;
                } else
                    i += 2;
                *dynamic = true;
                cont = true;
                break;
            case '%':
                insert = "%";
                break;
            default:
                if (expanded[i + 1] == 'r' || expanded[i + 1] == 't' ||
                    expanded[i + 1] == 'P' || expanded[i + 1] == 's' ||
                    expanded[i + 1] == 'T' || expanded[i + 1] == 'c' ||
                    expanded[i + 1] == 'C' || expanded[i + 1] == 'x' ||
                    expanded[i + 1] == 'y' || expanded[i + 1] == 'X' ||
                    expanded[i + 1] == 'Y' || expanded[i + 1] == 'w' ||
                    expanded[i + 1] == 'h' || expanded[i + 1] == 'W' ||
                    expanded[i + 1] == 'H' || expanded[i + 1] == 'm' ||
                    expanded[i + 1] == 'M' || expanded[i + 1] == 'S' ||
                    expanded[i + 1] == 'f') {
                    *dynamic = true;
                    i += 2;
                    cont = true;
                }
        }
        if (cont) continue;
        int ilen = strlen(insert);
        tmp = new char[strlen(expanded) + ilen + 1];
        expanded[i] = '\0';
        sprintf(tmp, "%s%s%s", expanded, insert, &expanded[i + offset]);
        if (found) delete [] expanded;
        else expanded[i] = '%';
        expanded = tmp;
        found = true;
        i += ilen;
    }
    if (found) return expanded;
    else return WA_STRDUP(org);
}

#ifdef    HAVE_ICONV
#ifndef   ICONV_CONST
#  define ICONV_CONST
#endif // !ICONV_CONST
char *wa_locale_to_utf8(const char *str) {
    char *dest;
    char *outp;
    const char *p;
    size_t len;
    size_t inbytes_remaining;
    size_t outbytes_remaining;
    size_t err;
    size_t outbuf_size;
    bool have_error = false;
    
    len = strlen(str);
    
    p = str;
    inbytes_remaining = len;
    outbuf_size = len + 1;
    
    outbytes_remaining = outbuf_size - 1;
    outp = dest = new char[outbuf_size];
    
again:
    err = iconv(meawm_ng->utf8conv, (ICONV_CONST char **) &p,
                &inbytes_remaining, &outp, &outbytes_remaining);

    if (err == (size_t) -1) {
        switch (errno) {
            case EINVAL:
                break;
            case E2BIG: {
                size_t used = outp - dest;
                size_t newsize = outbuf_size * 2;
                char *newdest = new char[newsize];
                memcpy(newdest, dest, outbuf_size);
                delete [] dest;
                outbuf_size = newsize;
                dest = newdest;
                outp = dest + used;
                outbytes_remaining = outbuf_size - used - 1;
                goto again;
            }
            case EILSEQ:
                WARNING_FUNC <<
                    "invalid byte sequence in conversion input" << endl;
                have_error = true;
                break;   
            default:
                WARNING_FUNC << "error during conversion: ";
                perror(NULL);
                have_error = true;
                break;
        }
    }

    *outp = '\0';

    if (have_error) {
        delete [] dest;
        return NULL;
    } else
        return dest;
}
#else
char *wa_locale_to_utf8(const char *str) {
  return WA_STRDUP((char *) str);
}
#endif // HAVE_ICONV


#ifdef    THREAD
void *render_thread_func(void *) {
    sigset_t sigset;
    cairo_state_t *cr = cairo_create();

    /* XXX: cairo need a call to this function for text support to be
       initialized, will probably dissapear soon. */
    cairo_set_target_drawable(cr, meawm_ng->display, DefaultRootWindow(meawm_ng->display));

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    for (;;) {
        RENDER_LIST_GET;

        DWindowObject *dw = __render_list.front();
        if (! dw) {
            RENDER_LIST_RELEASE;
            continue;
        }
        __render_list.pop_front();
        
        list<DWindowObject *>::iterator it = __render_list.begin();
        while (it != __render_list.end()) {
            if (*it == dw) it = __render_list.erase(it);
            else it++;
        }
        
        RENDER_LIST_RELEASE;
        
        RENDER_LOCK;
        __render_count++;
        RENDER_RELEASE;

        DWIN_RENDER_LOCK(dw);
        dw->decor_root->__win__render_count++;
        DWIN_RENDER_RELEASE_BY_THREAD(dw);

        dw->renderWindow(cr);

        XSync(meawm_ng->display, false);

        DWIN_RENDER_LOCK(dw);
        dw->decor_root->__win__render_count--;
        if (dw->decor_root->__win__render_count == 0) {
            DWIN_RENDER_BROADCAST(dw);
        } else
            DWIN_RENDER_RELEASE_BY_THREAD(dw);

        RENDER_LOCK;
        __render_count--;
        if (__render_count == 0) { RENDER_BROADCAST; }
        else RENDER_RELEASE;
    }
}
#endif // THREAD
