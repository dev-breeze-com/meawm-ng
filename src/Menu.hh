/* Menu.hh

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

#ifndef __Menu_hh
#define __Menu_hh

extern "C" {
#include <X11/Xlib.h>
}

class Menu;
class MenuItem;

#include "Screen.hh"
#include "Window.hh"
#include "Style.hh"

typedef enum {
    NoSpecialMenuType = 0,
    WindowListMenuType,
    CloneMergeListMenuType,
    VertMergeListMenuType,
    HorizMergeListMenuType
} SpecialMenuType;

class Menu : public RootWindowObject {
public:
    Menu(WaScreen *, char *);
    ~Menu(void);

    void inheritContent(Menu *);
    void inheritAttributes(Menu *);
    void applyAttributes(Parser *, Tst<char *> *);
    void clear(void);
    void update(void);
    void draw(void);
    void move(int, int);
    void map(int, int);
    void map(AWindowObject *_awo, bool focus, bool force);
    void unmap(void);
    void unmapSubmenus(MenuItem *);
    void raiseSubmenus(void);
    void lowerSubmenus(void);
    void unmapTree(void);
    void unLink(void);
    void focusFirst(void);
    void drawOutline(int, int);
    void clearOutline(void);
    void removeSpecialItems(void);
    void addWindowListItems(void);
    void addMergeListItems(WaWindow *, SpecialMenuType);
    void styleUpdate(bool, bool);

    list<MenuItem *> items;
    char *name;
    Window a_id;
    unsigned int width, height;
    double width_factor;
    int x, y;
    bool mapped;
    Window rootitem_id, rootitemlnk_id;
    int special_items;
    SpecialMenuType special_menu_type;
    bool outline_state;
    int outl_x, outl_y;
    Window focus;
};

class MenuItemAction {
public:
    MenuItemAction(void);
    ~MenuItemAction(void);

    bool applyAttributes(Parser *, Tst<char *> *);
    
    ActionFunc func;
    char *param;
};

class MenuItem : public DWindowObject {
public:
    MenuItem(WaScreen *, Menu *, char *);
    ~MenuItem(void);

    void applyAttributes(Parser *, Tst<char *> *);
    void perform(XEvent *);
    void mapSubmenu(char *, bool);
    inline void unmap(void) { menu->unmap(); }
    inline void unmapSubmenus(void) { menu->unmapSubmenus(NULL); }
    inline void unmapOtherSubmenus(void) { menu->unmapSubmenus(this); }
    inline void unmapTree(void) { menu->unmapTree(); }
    inline void unLink(void) { menu->unLink(); }
    void nextItem(void);
    void previousItem(void);
    void move(void);
    void moveOpaque(void);
    void currentPositionAndSize(int *, int *,
                                unsigned int *, unsigned int *);
    void evalWhatToRender(bool, bool, bool *, bool *, bool *);
    WaSurface *getBgInfo(DWindowObject **, int *, int *);
 
    unsigned int width, height;
    int y, x, newy, newx;
    char *str, *str2;
    bool str_dynamic, str2_dynamic;
    char *submenuname;
    Menu *menu, *submenu;
    bool focused, mstate, hilited;
    long int monitor_state;
    Window a_id;
    RenderGroup *icon_image, *icon_svg;
    list<MenuItemAction *> actions;
};

#endif // __Menu_hh
