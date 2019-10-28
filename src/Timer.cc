/* Timer.cc

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

#include "Timer.hh"

extern "C" {
#ifdef    HAVE_SIGNAL_H
#  include <signal.h>
#endif // HAVE_SIGNAL_H

#ifdef    HAVE_UNISTD_H
#  include <unistd.h>
#endif // HAVE_UNISTD_H
}
    
Timer *timer;

Timer::Timer(Meawm_NG *wa) {
    struct sigaction action;

    timer = this;
    meawm_ng = wa;
    timerval.it_interval.tv_sec = 0;
    timerval.it_interval.tv_usec = 0;
    timerval.it_value.tv_sec = 0;
    timerval.it_value.tv_usec = 0;
    action.sa_handler = timeout;
    action.sa_mask = sigset_t();
    action.sa_flags = 0;
    sigaction(SIGALRM, &action, NULL);
    paused = true;
    timer_signal = 0;

    start();
}

Timer::~Timer(void) {
    struct sigaction action;

    action.sa_handler = SIG_DFL;
    action.sa_mask = sigset_t();
    action.sa_flags = 0;
    timerval.it_value.tv_sec = 0;
    timerval.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timerval, NULL);
    sigaction(SIGALRM, &action, NULL);
    LISTDEL(interrupts);
}

void Timer::addInterrupt(Interrupt *i) {
    pause();
    
    if (interrupts.empty())
        interrupts.push_back(i);
    else {
        list<Interrupt *>::iterator it = interrupts.begin();
        for (; it != interrupts.end(); ++it) {
            if (i->delay.tv_sec < (*it)->delay.tv_sec ||
                (i->delay.tv_sec == (*it)->delay.tv_sec &&
                 i->delay.tv_usec < (*it)->delay.tv_usec)) {
                interrupts.insert(it, i);
                break;
            }
        }
        if (it == interrupts.end())
            interrupts.push_back(i);
    }
    start();
}

void Timer::start(void) {
    if (! paused) pause();
    if (! interrupts.empty()) {
        timerval.it_value.tv_sec = interrupts.front()->delay.tv_sec;
        timerval.it_value.tv_usec = interrupts.front()->delay.tv_usec;
        if (timerval.it_value.tv_sec < 0) timerval.it_value.tv_sec = 0;
        if (timerval.it_value.tv_usec < 0) timerval.it_value.tv_usec = 0;
        if (timerval.it_value.tv_sec == 0 && timerval.it_value.tv_usec == 0)
            timerval.it_value.tv_usec = 1;
        paused = false;
        setitimer(ITIMER_REAL, &timerval, NULL);
    }    
}

void Timer::pause(void) {
    struct itimerval remainval;
    struct timeval elipsedval;

    if (interrupts.empty() || paused) {
        paused = true;
        return;
    }
    
    timerval.it_value.tv_sec = 0;
    timerval.it_value.tv_usec = 0;
    getitimer(ITIMER_REAL, &remainval);
    setitimer(ITIMER_REAL, &timerval, NULL);
   
    elipsedval.tv_sec = interrupts.front()->delay.tv_sec - 
        remainval.it_value.tv_sec;
    elipsedval.tv_usec = interrupts.front()->delay.tv_usec -
        remainval.it_value.tv_usec;
    
    if (elipsedval.tv_usec < 0) {
        elipsedval.tv_sec--;
        elipsedval.tv_usec += 1000000;
    }

    list<Interrupt *>::iterator it = interrupts.begin();
    for (; it != interrupts.end(); ++it) {
        (*it)->delay.tv_sec -= elipsedval.tv_sec;
        (*it)->delay.tv_usec -= elipsedval.tv_usec;

        if ((*it)->delay.tv_usec < 0) {
            (*it)->delay.tv_sec--;
            (*it)->delay.tv_usec += 1000000;
        }
    }
}

void Timer::removeInterrupt(int id) {
    pause();
    
    if (interrupts.empty()) return;
    
    list<Interrupt *>::iterator it = interrupts.begin();
    for (; it != interrupts.end(); ++it) {
        if ((*it)->ident == id) {
            delete *it;
            it = interrupts.erase(it);
        }
    }
    
    start();
}

bool Timer::exitsInterrupt(int id) {
    bool exits = false;
    
    pause();
    
    if (interrupts.empty()) return false;
    
    list<Interrupt *>::iterator it = interrupts.begin();
    for (; it != interrupts.end(); ++it) {
        if ((*it)->ident == id) {
            exits = true;
            break;
        }
    }
    
    start();

    return exits;
}

Interrupt::Interrupt(Action *ac, XEvent *e, Window win) {
    memcpy(&event, e, sizeof(XEvent));
    action = ac->ref();
    delay.tv_sec = ac->delay / 1000;
    delay.tv_usec = (ac->delay % 1000) * 1000;
    window = win;
    ident = ac->timer_id;
}

Interrupt::~Interrupt(void) {
    action->unref();
}

void Timer::handleTimeout(void) {
    if (interrupts.empty()) return;
    Interrupt *i = interrupts.front();
    if (! i) return;
    interrupts.pop_front();

    list<Interrupt *>::iterator it = interrupts.begin();
    for (; it != interrupts.end(); ++it) {
        (*it)->delay.tv_sec -= i->delay.tv_sec;
        (*it)->delay.tv_usec -= i->delay.tv_usec;

        if ((*it)->delay.tv_usec < 0) {
            (*it)->delay.tv_sec--;
            (*it)->delay.tv_usec += 1000000;
        }
    }

    map<Window, WindowObject *>::iterator wit;
    if ((wit = meawm_ng->window_table.find(i->window)) !=
        meawm_ng->window_table.end()) {
        AWindowObject *awo = (AWindowObject *) (*wit).second;
        
        ((*awo).*(i->action->func))(&i->event, i->action);
    }
    if (i->action->periodic_timer) {
        i->delay.tv_sec = i->action->delay / 1000;
        i->delay.tv_usec = (i->action->delay % 1000) * 1000;
        addInterrupt(i);
    } else
        delete i;
    
    start();
}

void timeout(int signal) {
    timer->timer_signal = signal;
}
