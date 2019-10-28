/* Regex.hh

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

#ifndef __Regex_hh
#define __Regex_hh

extern "C" {
#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif // HAVE_SYS_TYPES_H

#ifdef    HAVE_REGEX_H
#  include <regex.h>
#endif // HAVE_REGEX_H
}

#include "Util.hh"

class Regex {
public:
    Regex(char *);
    inline ~Regex(void) { if (comp_ok) regfree(&regexp); }

    bool match(char *);

    regex_t regexp;
    bool comp_ok;
};

typedef enum {
    WindowIDName,
    WindowIDClass,
    WindowIDClassName,
    WindowIDPID,
    WindowIDHost,
    WindowIDWinID
} WindowIDType;

class WindowRegex {
public:
    WindowRegex(int, char *);
    virtual ~WindowRegex(void);

    void addIDRegex(WindowIDType, char *);
    bool match(WaStringMap *, int, char *);

    int state;
    
private:
    map<WindowIDType, Regex *> id_regex_map;
    Regex *window_regex;
};

#endif // __Regex_hh
