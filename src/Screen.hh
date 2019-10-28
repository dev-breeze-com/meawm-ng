/* Screen.hh

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

#ifndef __Screen_hh
#define __Screen_hh

extern "C" {
#include <X11/Xlib.h>

#ifdef    XINERAMA
#  include <X11/extensions/Xinerama.h>
#endif // XINERAMA

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#  include <stdarg.h>
#endif // STDC_HEADERS
    
}

class WaScreen;
class ScreenEdge;
class Regex;

typedef struct {
    Window window;
    unsigned int left;
    unsigned int right;
    unsigned int top;
    unsigned int bottom;
} WMstrut;

#include "Resources.hh"
#include "Dockapp.hh"
#include "Net.hh"
#include "Menu.hh"
#include "Action.hh"
#include "Style.hh"

typedef struct {
    int x;
    int y;
    unsigned int width;
    unsigned int height;
} Workarea;

class Desktop {
public:
    inline Desktop(int _number, int w, int h) {
        number = _number;
        workarea.x = workarea.y = 0;
        workarea.width = w;
        workarea.height = h;
        v_x = v_y = 0;
    }
    unsigned int number;
    Workarea workarea;
    int v_x, v_y;
};

class SystrayWindow : public WindowObject {
public:
    inline SystrayWindow(Window id, WaScreen *_ws) :
        WindowObject(_ws, id, SystrayType) {}
};

class MReq {
public:
    inline MReq(Window m, WaWindow *w, int t) { mid = m; win = w; type = t; }

    Window mid;
    WaWindow *win;
    int type;
};

#define WestDirection  1
#define EastDirection  2
#define NorthDirection 3
#define SouthDirection 4

typedef struct {
    char *style_file;
    char *action_file;
    char *menu_file;
    unsigned int virtual_x;
    unsigned int virtual_y;
    unsigned int desktops;
    StackingType menu_stacking;
    StackingType dock_stacking;
    bool transient_above, revert_to_window;
    bool external_bg;
    char *info_command;
    bool info_command_dynamic;
    char *warning_command;
    bool warning_command_dynamic;
} ScreenConfig;

class WaScreen : public RootWindowObject {
public:
    WaScreen(Display *, int, Meawm_NG *);
    ~WaScreen(void);

#ifdef    RENDER
    Visual *findARGBVisual(void);
  #endif // RENDER
  
    void forceRenderOfWindows(int);
    void propagateActionlistUpdate(ActionList *);
    void propagateStyleUpdate(Style *);
    void clearAllCacheAndRedraw(void);
    void reload(void);
    void raiseTransientWindows(Window, list<Window> *);
    void raiseWindow(Window, bool = true);
    void lowerWindow(Window, bool = true);
    void restackWindows(void);
    void updateCheckboxes(int);
    ActionList *getActionListNamed(char *, bool = true);
    Style *getStyleNamed(char *, bool = true);
    RenderGroup *getRenderGroupNamed(char *, bool = true);
    RenderPattern *getPatternNamed(char *, bool = true);
    RenderOpPath *getPathNamed(char *, bool = true);
    RenderOpText *getTextNamed(char *, bool = true);
    Menu *getMenuNamed(char *, bool = true);
    void moveViewportTo(int, int);
    void moveViewport(int);
    void scrollViewport(int, bool, Action *);
    void mapMenu(char *, AWindowObject *, bool, bool);
    void unmapMenu(char *, bool);
    void updateWorkarea(void);
    void getWorkareaSize(int *, int *, unsigned int *, unsigned int *);
    void addDockapp(Dockapp *, char *);
    void goToDesktop(unsigned int);
    WaWindow *regexMatchWindow(char *, WaWindow * = NULL);
    void smartName(WaWindow *);
    void smartNameRemove(WaWindow *);
    Pixmap getRootBgPixmap(Pixmap, unsigned int, unsigned int, int, int,
                           unsigned int, unsigned int);
    unsigned char *getRootBgImage(unsigned char *, unsigned int, unsigned int,
                                  int, int, unsigned int, unsigned int);

#ifdef    RANDR
    void rrUpdate(void);
#endif // RANDR
    
    void startViewportMove(void);
    void taskSwitcher(void);
    void previousTask(void);
    void nextTask(void);
    void pointerRelativeWarp(char *);
    void pointerFixedWarp(char *);
    void viewportRelativeMove(char *);
    void viewportFixedMove(char *);
    void nextDesktop(void);
    void previousDesktop(void);
    void findClosestWindow(int);
    WaSurface *rgbaToWaSurface(unsigned char *, unsigned int, unsigned int);

    list<ActionRegex *> *getRegexActionList(char *);
    list<StyleRegex *> *getRegexStyleList(char *);
    void getRegexTargets(WindowRegex *, long int, bool,
                         list<AWindowObject *> *);
    void showMessage(char *, bool, const char *, const char *, va_list);
    void showWarningMessage(const char *, const char *, ...)
        __attribute__((format(printf, 3, 4)));
    void showInfoMessage(const char *, const char *, ...)
        __attribute__((format(printf, 3, 4)));
    int getSubwindowId(char *);
    char *getSubwindowName(int);
    void styleUpdate(bool, bool);
    void endRender(Pixmap);
    void styleDiff(Style *, Style *, bool *, bool *);
    
    Display *display;
    unsigned int screen_number, screen_depth, width, height;
    int v_x, v_y, v_xmax, v_ymax;
    Colormap colormap;
    Visual *visual;
    Meawm_NG *meawm_ng;
    NetHandler *net;
    ResourceHandler *rh;
    ScreenConfig config;
    GC black_gc, white_gc, xor_gc;
    Menu *windowlist_menu;

    WaSurface *bg_surface;
    
    char displaystring[256];
    ScreenEdge *west, *east, *north, *south;
    Window wm_check;
    bool focused, shutdown, dont_propagate_cfg_update;

    double vdpi, hdpi;

    list<Desktop *> desktop_list;
    Desktop *current_desktop;

    list<Window> aot_stacking_list, stacking_list, aab_stacking_list;
    list<WaWindow *> wawindow_list;
    list<WaWindow *> wawindow_list_map_order;
    list<WMstrut *> strut_list;
    list<DockappHandler *> docks;
    list<Window> systray_window_list;
    
    list<MReq *> mreqs;
    
    Tst<Menu *> menus;
    Tst<RenderGroup *> rendergroups;
    Tst<RenderOpPath *> paths;
    Tst<RenderPattern *> patterns;
    Tst<RenderOpText *> texts;
    Tst<Style *> styles;
    Tst<ActionList *> actionlists;
    
    list<StyleRegex *> window_styles;
    list<StyleRegex *> root_styles;
    list<StyleRegex *> menu_styles;
    list<StyleRegex *> dockappholder_styles;
    list<ActionRegex *> window_actionlists;
    list<ActionRegex *> screenedge_actionlists;
    list<ActionRegex *> root_actionlists;
    list<ActionRegex *> menu_actionlists;
    list<ActionRegex *> dockappholder_actionlists;
    list<ActionRegex *> dockapp_actionlists;

    Tst<char *> constants;

private:
    void readActionLists(void);
    void readStyles(void);
    void readMenus(void);
    
    int move;
    vector<char *> subwindow_names;
};

class ScreenEdge : public AWindowObject {
public:
    ScreenEdge(WaScreen *, char *, int, int, int, int);
    ~ScreenEdge(void);
};

#endif // __Screen_hh
