/* Event.hh

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

#ifndef __Event_hh
#define __Event_hh

extern "C" {
#include <X11/Xlib.h>

#ifdef    TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else // !TIME_WITH_SYS_TIME
#  ifdef    HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else // !HAVE_SYS_TIME_H
#    include <time.h>
#  endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME
}

#include <set>
using std::set;

class EventHandler;

#include "Meawm_NG.hh"
#include "Action.hh"

#define DOUBLECLICK_TIMER_ID -1

class EventHandler {
public:
    EventHandler(Meawm_NG *);
    virtual ~EventHandler(void);

    void eventLoop(set<int> *, XEvent *);
    void handleEvent(XEvent *);
    void evFocus(XFocusChangeEvent *);
    void evUnmapDestroy(XEvent *);
    void evConfigureRequest(XConfigureRequestEvent *);
    void evAct(XEvent *, Window, EventDetail *);

    void getActionManager(Window);
    
    XEvent *event;
    set<int> empty_return_mask;
    set<int> moveresize_return_mask;
    set<int> menu_viewport_move_return_mask;

    list<Doing *> doings;
    
    int move_resize;
    Window focused;

private:
    void evProperty(XPropertyEvent *);
    void evColormap(XColormapEvent *);
    WindowObject *evMapRequest(XMapRequestEvent *);
    void evClientMessage(XEvent *, EventDetail *);
    
    Meawm_NG *meawm_ng;
    Window last_click_win;
    unsigned int last_button;
    XWMHints *wm_hints;
    int xfd;
    Window action_manager_window;
    WaScreen *action_manager_screen;
};

#endif // __EventHandler_hh
