/* Dockapp.hh

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

#ifndef __Dockapp_hh
#define __Dockapp_hh

class DockappHandler;
class Dockapp;

#include "Regex.hh"
#include "Style.hh"

class DockappHandler : public RootWindowObject {
public:
    DockappHandler(WaScreen *, char *);
    ~DockappHandler(void);

    void update(void);
    void styleUpdate(bool, bool);

    list<Dockapp *> dockapp_list;
    unsigned int desktop_mask;
    bool hidden;
    char *name;
    bool inworkspace;

private:
    int x, y;
    unsigned int width, height;
    WMstrut *wm_strut;
    StackingType stacking;
};

class Dockapp : public AWindowObject {
public:
    Dockapp(WaScreen *, Window);
    ~Dockapp(void);

    Window icon_id, client_id;
    DockappHandler *dh;
    int x, y, prio;
    unsigned int width, height;
    bool deleted, prio_set;
    char *name;
};

#endif // __Dockapp_hh
