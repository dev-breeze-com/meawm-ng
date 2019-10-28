/* Action.hh

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

#ifndef __Action_hh
#define __Action_hh

extern "C" {
#include <X11/Xlib.h>

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H
}

#include <list>
using std::list;

class AWindowObject;
class Action;
class ActionList;
class WindowObject;
class WaStringList;

#include "Parser.hh"
#include "RefCounted.hh"
#include "Regex.hh"

class WaScreen;
class WaWindow;
class Style;
class Menu;
class MenuItem;
class DockappHandler;
class Dockapp;

typedef struct {
    unsigned int type, x11mod, wamod, detail;
    int x, y;
} EventDetail;

typedef enum {
    NoStackingType,
    NormalStackingType,
    AlwaysOnTopStackingType,
    AlwaysAtBottomStackingType
} StackingType;

class WindowObject : public RefCounted<WindowObject> {
public:
    WindowObject(WaScreen *, Window, int);
    virtual ~WindowObject(void);
    
    void setStacking(StackingType);

    Window id;
    int type;
    StackingType stacking;
    WaScreen *ws;
};

#define ACTION_WARNING { \
    ws->showWarningMessage(__FUNCTION__, "Tried to trigger action " \
                           "function %s on a window that " \
                           "does not support that action.", __FUNCTION__); }

#define PARAM_WARNING { \
    ws->showWarningMessage(__FUNCTION__, "Missing parameter, action " \
                           "function %s requires a " \
                           "parameter.", __FUNCTION__); \
    return; }

typedef void (AWindowObject::*ActionFunc)(XEvent *, Action *);

class AWindowObject : public WindowObject {
public:
    AWindowObject(WaScreen *, Window, int, WaStringMap *, char *);
    virtual ~AWindowObject(void);
    void resetActionList(WaStringMap *);
    void resetActionList(void);
    void evalState(EventDetail *);

    WaStringMap *ids;
    char *window_name;

    int window_state_mask;
    
    ActionList *actionlists[WIN_STATE_LAST];
    ActionList *default_actionlists[WIN_STATE_LAST];

    WaWindow       *getWindow(void);
    WaScreen       *getScreen(void);
    MenuItem       *getMenuItem(void);
    DockappHandler *getDockHandler(void);
    Dockapp        *getDockapp(void);

    bool handleEvent(XEvent *, EventDetail *);

    void nop(XEvent *, Action *);
    void showInfo(XEvent *, Action *);
    void showWarning(XEvent *, Action *);
    void exec(XEvent *, Action *);
    void doing(XEvent *, Action *);
    void stopTimer(XEvent *, Action *);
    void waexit(XEvent *, Action *);
    void warestart(XEvent *, Action *);
    void setActionFile(XEvent *, Action *);
    void setStyleFile(XEvent *, Action *);
    void setMenuFile(XEvent *, Action *);
    void reloadWithActionFile(XEvent *, Action *);
    void reloadWithStyleFile(XEvent *, Action *);
    void reloadWithMenuFile(XEvent *, Action *);
    void reload(XEvent *, Action *);
    void taskSwitcher(XEvent *, Action *);
    void nextTask(XEvent *, Action *);
    void previousTask(XEvent *, Action *);
    void goToDesktop(XEvent *, Action *);
    void nextDesktop(XEvent *, Action *);
    void previousDesktop(XEvent *, Action *);
    void pointerRelativeWarp(XEvent *, Action *);
    void pointerFixedWarp(XEvent *, Action *);
    void viewportLeft(XEvent *, Action *);
    void viewportRight(XEvent *, Action *);
    void viewportUp(XEvent *, Action *);
    void viewportDown(XEvent *, Action *);
    void viewportRelativeMove(XEvent *, Action *);
    void viewportFixedMove(XEvent *, Action *);
    void startViewportMove(XEvent *, Action *);
    void setActionList(int, Action *);
    void setPassiveActionList(XEvent *, Action *);
    void setActiveActionList(XEvent *, Action *);
    void setHoverActionList(XEvent *, Action *);
    void setPressedActionList(XEvent *, Action *);
    void defaultPassiveActionList(XEvent *, Action *);
    void defaultActiveActionList(XEvent *, Action *);
    void defaultHoverActionList(XEvent *, Action *);
    void defaultPressedActionList(XEvent *, Action *);
    void setStyle(int, Action *);
    void setPassiveStyle(XEvent *, Action *);
    void setActiveStyle(XEvent *, Action *);
    void setHoverStyle(XEvent *, Action *);
    void setPressedStyle(XEvent *, Action *);
    void defaultPassiveStyle(XEvent *, Action *);
    void defaultActiveStyle(XEvent *, Action *);
    void defaultHoverStyle(XEvent *, Action *);
    void defaultPressedStyle(XEvent *, Action *);
    void setCursor(XEvent *, Action *);
    void defaultCursor(XEvent *, Action *);
    void mapMenu(XEvent *, Action *);
    void remapMenu(XEvent *, Action *);
    void mapMenuFocused(XEvent *, Action *);
    void remapMenuFocused(XEvent *, Action *);
    void unmapMenu(XEvent *, Action *);
    void endMoveResize(XEvent *, Action *);
    void moveFocusToClosestNorthWindow(XEvent *, Action *);
    void moveFocusToClosestSouthWindow(XEvent *, Action *);
    void moveFocusToClosestWestWindow(XEvent *, Action *);
    void moveFocusToClosestEastWindow(XEvent *, Action *);
    void readAdditionalConfig(XEvent *, Action *);
    void focusRoot(XEvent *, Action *);

    void windowRaise(XEvent *, Action *);
    void windowLower(XEvent *, Action *);
    void windowFocus(XEvent *, Action *);
    void windowRaiseFocus(XEvent *, Action *);
    void windowStartMove(XEvent *, Action *);
    void windowStartResizeRight(XEvent *, Action *);
    void windowStartResizeUpRight(XEvent *, Action *);
    void windowStartResizeDownRight(XEvent *, Action *);
    void windowStartResizeUpLeft(XEvent *, Action *);
    void windowStartResizeDownLeft(XEvent *, Action *);
    void windowStartResizeUp(XEvent *, Action *);
    void windowStartResizeDown(XEvent *, Action *);
    void windowStartResizeLeft(XEvent *, Action *);
    void windowStartResizeSmart(XEvent *, Action *);
    void windowStartOpaqueMove(XEvent *, Action *);
    void windowStartOpaqueResizeUpRight(XEvent *, Action *);
    void windowStartOpaqueResizeDownRight(XEvent *, Action *);
    void windowStartOpaqueResizeRight(XEvent *, Action *);
    void windowStartOpaqueResizeUpLeft(XEvent *, Action *);
    void windowStartOpaqueResizeDownLeft(XEvent *, Action *);
    void windowStartOpaqueResizeLeft(XEvent *, Action *);
    void windowStartOpaqueResizeUp(XEvent *, Action *);
    void windowStartOpaqueResizeDown(XEvent *, Action *);
    void windowStartOpaqueResizeSmart(XEvent *, Action *);
    void windowMoveResize(XEvent *, Action *);
    void windowMoveResizeVirtual(XEvent *, Action *);
    void windowMoveToPointer(XEvent *, Action *);
    void windowMoveToSmartPlace(XEvent *, Action *);
    void windowDesktopMask(XEvent *, Action *);
    void windowJoinDesktop(XEvent *, Action *);
    void windowPartDesktop(XEvent *, Action *);
    void windowPartCurrentDesktop(XEvent *, Action *);
    void windowJoinAllDesktops(XEvent *, Action *);
    void windowPartAllDesktopsExceptCurrent(XEvent *, Action *);
    void windowPartCurrentJoinDesktop(XEvent *, Action *);
    void windowCloneMergeWithWindow(XEvent *, Action *);
    void windowVertMergeWithWindow(XEvent *, Action *);
    void windowHorizMergeWithWindow(XEvent *, Action *);
    void windowExplode(XEvent *, Action *);
    void windowMergedToFront(XEvent *, Action *);
    void windowUnMerge(XEvent *, Action *);
    void windowSetMergeMode(XEvent *, Action *);
    void windowNextMergeMode(XEvent *, Action *);
    void windowPrevMergeMode(XEvent *, Action *);
    void windowClose(XEvent *, Action *);
    void windowKill(XEvent *, Action *);
    void windowCloseKill(XEvent *, Action *);
    void windowShade(XEvent *, Action *);
    void windowUnShade(XEvent *, Action *);
    void windowToggleShade(XEvent *, Action *);
    void windowMaximize(XEvent *, Action *);
    void windowUnMaximize(XEvent *, Action *);
    void windowToggleMaximize(XEvent *, Action *);
    void windowMinimize(XEvent *, Action *);
    void windowUnMinimize(XEvent *, Action *);
    void windowSticky(XEvent *, Action *);
    void windowUnSticky(XEvent *, Action *);
    void windowToggleSticky(XEvent *, Action *);
    void windowFullscreenOn(XEvent *, Action *);
    void windowFullscreenOff(XEvent *, Action *);
    void windowFullscreenToggle(XEvent *, Action *);
    void windowToggleMinimize(XEvent *, Action *);
    void windowDecorTitleOn(XEvent *, Action *);
    void windowDecorTitleOff(XEvent *, Action *);
    void windowDecorTitleToggle(XEvent *, Action *);
    void windowDecorBorderOn(XEvent *, Action *);
    void windowDecorBorderOff(XEvent *, Action *);
    void windowDecorBorderToggle(XEvent *, Action *);
    void windowDecorHandlesOn(XEvent *, Action *);
    void windowDecorHandlesOff(XEvent *, Action *);
    void windowDecorHandlesToggle(XEvent *, Action *);
    void windowDecorAllOn(XEvent *, Action *);
    void windowDecorAllOff(XEvent *, Action *);
    void windowAlwaysOnTopOn(XEvent *, Action *);
    void windowAlwaysOnTopOff(XEvent *, Action *);
    void windowAlwaysOnTopToggle(XEvent *, Action *);
    void windowAlwaysAtBottomOn(XEvent *, Action *);
    void windowAlwaysAtBottomOff(XEvent *, Action *);
    void windowAlwaysAtBottomToggle(XEvent *, Action *);
    
    void menuUnLink(XEvent *, Action *);
    void menuMapSubmenu(XEvent *, Action *);
    void menuRemapSubmenu(XEvent *, Action *);
    void menuUnMap(XEvent *, Action *);
    void menuUnMapSubmenus(XEvent *, Action *);
    void menuUnMapOtherSubmenus(XEvent *, Action *);
    void menuUnMapTree(XEvent *, Action *);
    void menuAction(XEvent *, Action *);
    void menuNextItem(XEvent *, Action *);
    void menuPreviousItem(XEvent *, Action *);
    void menuStartMove(XEvent *, Action *);
    void menuStartOpaqueMove(XEvent *, Action *);
    void menuRaise(XEvent *, Action *);
    void menuLower(XEvent *, Action *);
    
    void dockappHandlerSetInWorkspace(XEvent *, Action *);
    void dockappHandlerSetNotInWorkspace(XEvent *, Action *);
    void dockappHandlerRaise(XEvent *, Action *);
    void dockappHandlerLower(XEvent *, Action *);
    
    void setStackingLayer(XEvent *, Action *);
    
    void dockappAddToHandler(XEvent *, Action *);
    void dockappSetPrio(XEvent *, Action *);
};

class Action;
class ActionRegex;

#define MoveResizeMask          (1L <<  0)
#define StateTrueMask           (1L <<  1)

#define StateMoveMask           (1L <<  2)
#define StateResizeMask         (1L <<  3)
#define StateStickyMask         (1L <<  4)
#define StateShadedMask         (1L <<  5)
#define StateMaximizedMask      (1L <<  6)
#define StateMinimizedMask      (1L <<  7)
#define StateFullscreenMask     (1L <<  8)
#define StateDecorTitleMask     (1L <<  9)
#define StateDecorBorderMask    (1L << 10)
#define StateDecorHandlesMask   (1L << 11)
#define StateDecorAllMask       (1L << 12)
#define StateAlwaysOnTopMask    (1L << 13)
#define StateAlwaysAtBottomMask (1L << 14)
#define StateTransientMask      (1L << 15)
#define StateGroupLeaderMask    (1L << 16)
#define StateGroupMemberMask    (1L << 17)
#define StateUrgentMask         (1L << 18)
#define StateFocusableMask      (1L << 19)
#define StateTasklistMask       (1L << 20)
#define StateEwmhDesktopMask    (1L << 21)
#define StateEwmhDockMask       (1L << 22)
#define StateEwmhToolbarMask    (1L << 23)
#define StateEwmhMenuMask       (1L << 24)
#define StateEwmhSplashMask     (1L << 25)
#define StateEwmhDialogMask     (1L << 26)
#define StateUserTimeMask       (1L << 27)
#define StatePositionedMask     (1L << 28)

enum {
    DoubleClick = 36,
    WindowStateChangeNotify,
    ViewportChangeNotify,
    DesktopChangeNotify,
    RestartNotify,
    ExitNotify,
    CreateWindowNotify,
    DestroyWindowNotify,
    RaiseNotify,
    LowerNotify,
    ActiveWindowChangeNotify,
    MenuMapNotify,
    MenuUnmapNotify,
    SubmenuMapNotify,
    SubmenuUnmapNotify,
    StateAddNotify,
    StateRemoveNotify,
    DockappAddRequest
};

class ActionList : public RefCounted<ActionList> {
public:
    ActionList(WaScreen *, char *);
    ~ActionList(void);

    void clear(void);
    void inheritContent(ActionList *);

    WaScreen *ws;
    char *name;
    list<Action *> actionlist;
};

class ActionRegex : public WindowRegex {
public:
    ActionRegex(int s, char *wr) : WindowRegex(s, wr) { actionlist = NULL; }

    inline ActionList *match(WaStringMap *m, int s, char *w = NULL) {
        if (((WindowRegex *) this)->match(m, s, w))
            return (ActionList *) actionlist->ref();
        else
            return NULL;
    }
    
    ActionList *actionlist;
};

class Action : public RefCounted<Action> {
public:
    Action(void);
    ~Action(void);

    void applyAttributes(Parser *, Tst<char *> *);
    bool match(EventDetail *);

    WaScreen *ws;
    ActionFunc func;
    char *param;
    unsigned int type, detail, x11mod, x11nmod, wamod, wanmod;
    bool replay;
    unsigned int delay;
    int timer_id;
    bool periodic_timer;
    int priority;
    union {
        long int any;
        Menu *menu;
        ActionList *actionlist;
        Style *style;
        Cursor cursor;
    } pointer;
};

class Doing {
public:
    Doing(WaScreen *);
    ~Doing(void);

    void applyAttributes(Parser *, Tst<char *> *);
    void envoke(void);
    
    WindowRegex *wreg;
    bool multiple;
    ActionFunc func;
    int target_type;
    char *param;
    WaScreen *ws;
};

Tst<char *> *short_do_string_to_tst(char *);

extern Tst<ActionFunc> *actionfunction_tst;
extern Tst<long int> *client_window_state_tst;

void action_create_tsts(void);

#endif // __Action_hh
