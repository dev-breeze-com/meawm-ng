/* Timer.hh

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

#ifndef __Timer_hh
#define __Timer_hh

extern "C" {
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

class Timer;
class Interrupt;

#include "Action.hh"
#include "Meawm_NG.hh"

class Timer {
public:
    Timer(Meawm_NG *);
    virtual ~Timer(void);

    void addInterrupt(Interrupt *);
    void start(void);
    void pause(void);
    void removeInterrupt(int);
    bool exitsInterrupt(int);
    void handleTimeout(void);
    
    Meawm_NG *meawm_ng;
    list<Interrupt *> interrupts;
    bool paused;
    int timer_signal;
    
private:
    struct itimerval timerval;
};

class Interrupt {
public:
    Interrupt(Action *, XEvent *, Window);
    ~Interrupt(void);
    
    Window window;
    int ident;
    WaScreen *ws;
    struct timeval delay;
    Action *action;
    XEvent event;
};

void timeout(int);

#endif // __Timer_hh
