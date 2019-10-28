/* Resources.hh

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

#ifndef __Resources_hh
#define __Resources_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H
}

#define DEFAULTRCFILE     WAIMEA_PATH "/meawm_ngrc"
#define DEFAULTSCRIPTDIR  WAIMEA_PATH "/scripts"
#define DEFAULTACTIONFILE WAIMEA_PATH \
    "/actions/sloppyfocusopaque/action.conf"
#define DEFAULTMENUFILE   WAIMEA_PATH \
    "/menus/menu.conf"
#define DEFAULTSTYLEFILE  WAIMEA_PATH \
    "/styles/freedesktop/style.conf"

class ResourceHandler;

#include "Meawm_NG.hh"
#include "Regex.hh"
#include "Style.hh"

class ResourceHandler {
public:
    ResourceHandler(Meawm_NG *, char **);

    void loadConfig(Meawm_NG *);
    void loadConfig(WaScreen *);
    
    XrmDatabase database;
    char **options;
    
private:
    Meawm_NG *meawm_ng;
    Display *display;
};

#endif // __Resources_hh
