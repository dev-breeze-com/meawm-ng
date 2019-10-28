/* Util.hh

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

#ifndef __Util_hh
#define __Util_hh

extern "C" {

#ifdef HAVE_STDIO_H
#  include <stdio.h>
#  include <stdint.h>
#  include <unistd.h>
#  include <values.h>
#  include <malloc.h>
#endif // HAVE_STDIO_H

#ifdef HAVE_MATH_H
#  include <math.h>
#endif // HAVE_MATH_H

}

/* If we're not using GNU C, elide __attribute__ */
#ifndef __GNUC__
#  define  __attribute__(x)  /* NOTHING */
#endif

#include <cctype>
#include <cstring>
#include <cstdlib>

#include <list>
using std::list;

#include <map>
using std::map;
using std::make_pair;

#include <vector>
using std::vector;

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

#define UNUSED_VARIABLE(x) ((void) (x)) 

#define WA_ROUND_U(value) __wa_round_to_unsigned(value)
#define WA_ROUND(value) ((int) (value + 0.5))

#define WA_STRDUP(str) __wa_strdup(str)

#define WARNING      cerr << "meawm_ng: warning: "
#define ERROR        cerr << "meawm_ng: error: "
#define WARNING_FUNC cerr << "meawm_ng: warning: " << __FUNCTION__ << ": "
#define ERROR_FUNC   cerr << "meawm_ng: error: "   << __FUNCTION__ << ": "

#define LISTDEL(list) \
    while (! list.empty()) { \
        delete list.back(); \
        list.pop_back(); \
    }

#define LISTUNREF(list) \
    while (! list.empty()) { \
        list.back()->unref(); \
        list.pop_back(); \
    }

#define LISTDELITEMS(list) \
    while (! list.empty()) { \
        delete list.back(); \
    }

#define LISTPTRDEL(list) \
    while (! list->empty()) { \
        delete list->back(); \
        list->pop_back(); \
    }

#define LISTPTRDELSET(list) \
    while (! list->empty()) { \
        delete [] list->back(); \
        list->pop_back(); \
    }

#define LISTCLEAR(list) \
    while (! list.empty()) { \
        list.pop_back(); \
    }

#define LISTPTRCLEAR(list) \
    while (! list->empty()) { \
        list->pop_back(); \
    } \
    delete list;

#define LISTPTRDELITEMS(list) \
    while (! list->empty()) { \
        delete list->back(); \
    }

#define MAPCLEAR(map) \
    while (! map.empty()) { \
        map.erase(map.begin()); \
    }

#define MAPUNREFSECOND(map) \
    while (! map.empty()) { \
        ((*map.begin()).second)->unref(); \
        map.erase(map.begin()); \
    }

#define MAPPTRCLEAR(map) \
    while (! map->empty()) { \
        map->erase(map->begin()); \
    } \
    delete map;

typedef unsigned long WaPixel;

#define RootType        (1L << 0)
#define WindowFrameType (1L << 1)
#define MenuType        (1L << 2)
#define MenuItemType    (1L << 3)
#define DockHandlerType (1L << 4)

#define ANY_ROOTDECOR_WINDOW_TYPE ((1L << 5) - 1)

#define SubwindowType   (1L << 5)

#define ANY_DECOR_WINDOW_TYPE ((1L << 6) - 1)

#define WindowType      (1L << 6)
#define EdgeType        (1L << 7)
#define DockAppType     (1L << 8)

#define ANY_ACTION_WINDOW_TYPE ((1L << 9) - 1)

#define SystrayType     (1L << 9)

typedef enum {
    MoveType,
    MoveOpaqueType,
    ResizeType,
    ResizeOpaqueType,
    EndMoveResizeType
} MoveResizeType;

#define WIN_STATE_ACTIVE_MASK  (1L << 0)
#define WIN_STATE_HOVER_MASK   (1L << 1)
#define WIN_STATE_PRESSED_MASK (1L << 2)

enum {
    WIN_STATE_PASSIVE = 0,
    WIN_STATE_ACTIVE,
    WIN_STATE_HOVER,
    WIN_STATE_PRESSED,
    WIN_STATE_LAST
};

#define STATE_FROM_MASK_AND_LIST(mask, list) \
    ((mask & WIN_STATE_HOVER_MASK)? ( \
        ((mask & WIN_STATE_PRESSED_MASK) && list[WIN_STATE_PRESSED])? ( \
            WIN_STATE_PRESSED \
            ) \
        : ( \
            (list[WIN_STATE_HOVER])? ( \
                WIN_STATE_HOVER \
                ) \
            : ( \
                ((mask & WIN_STATE_ACTIVE_MASK) && list[WIN_STATE_ACTIVE])? ( \
                    WIN_STATE_ACTIVE \
                    ) : ( \
                        WIN_STATE_PASSIVE \
                        ) \
                ) \
            ) \
        ) \
    : ( \
        ((mask & WIN_STATE_ACTIVE_MASK) && list[WIN_STATE_ACTIVE])? ( \
            WIN_STATE_ACTIVE \
            ) : ( \
                WIN_STATE_PASSIVE \
                ) \
        ))


#include "RefCounted.hh"

class WaStringMap : public RefCounted<WaStringMap> {
public:
    inline WaStringMap(void) : RefCounted<WaStringMap>(this) {}
    WaStringMap(int, const char *);
    ~WaStringMap(void);
    
    void add(int, const char *, ...) __attribute__((format(printf, 3, 4)));
    inline map<int, char *>::iterator begin(void) { return str_map.begin(); }
    inline map<int, char *>::iterator end(void) { return str_map.end(); }
    inline map<int, char *>::iterator find(int key) {
        return str_map.find(key);
    }

private:
    map<int, char *> str_map;
};

inline unsigned int __wa_round_to_unsigned(double uv) {
    int tmp = (int) (uv + 0.5);
    return (tmp > 0)? (unsigned int) tmp: 0;
}

inline char *__wa_strdup(char *s) {
    char *tmp = new char[std::strlen(s) + 1];
    std::strcpy(tmp, s);
    return tmp;
}

char *environment_expansion(char *);
void commandline_to_argv(char *, char **, int);
char *smartfile(const char *, char *, bool = true);

#endif // __Util_hh
