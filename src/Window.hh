/* Window.hh

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

#ifndef __Window_hh
#define __Window_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>

#ifdef    SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE
}

class WaWindow;
class WaFrameWindow;
class FlagMonitor;

#ifdef    SHAPE
class ShapeInfo;
#endif // SHAPE

#define DeleteWindowProtocalMask (1L << 0)
#define TakeFocusProtocalMask    (1L << 1)

#define FunctionMoveMask        (1L << 0)
#define FunctionResizeMask      (1L << 1)
#define FunctionSetDecorMask    (1L << 2)
#define FunctionSetStackingMask (1L << 3)
#define FunctionDesktopMask     (1L << 4)
#define FunctionMergeMask       (1L << 5)
#define FunctionAllMask         ((1L << 6) - 1)

#include "Style.hh"
#include "Event.hh"
#include "Net.hh"

#define MERGED_LOOP \
    list<WaWindow *>::iterator __mw_it = merged.begin(); \
    for (WaWindow *_mw = NULL; _mw != this && \
             (_mw = (__mw_it == merged.end())? this: *__mw_it); __mw_it++)

#define ApplyGravity   1
#define RemoveGravity -1

#define NullMergeType  1
#define CloneMergeType 2
#define HorizMergeType 3
#define VertMergeType  4

#define EastResizeTypeMask  (1L << 0)
#define WestResizeTypeMask  (1L << 1)
#define NorthResizeTypeMask (1L << 2)
#define SouthResizeTypeMask (1L << 3)

#define DecorTitleMask   (1L << 0)
#define DecorBorderMask  (1L << 1)
#define DecorHandlesMask (1L << 2)
#define DecorAllMask     ((1L << 3) - 1)

class FlagMonitor {
public:
    FlagMonitor(AWindowObject *_monitor, long int _type) {
        monitor = _monitor;
        type = _type;
    }
    
    AWindowObject *monitor;
    long int type;
};

typedef struct {
    unsigned int max_width, max_height;
    unsigned int min_width, min_height;
    unsigned int width_inc, height_inc;
    unsigned int base_width, base_height;
    bool base_size;
    double min_aspect, max_aspect;
    bool aspect;
    int win_gravity;
} SizeStruct;

typedef struct {
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    int misc0;
    int misc1;
    Colormap colormap;
} WaWindowAttributes;

class WaWindow : public AWindowObject {
public:
    WaWindow(Window, WaScreen *);
    ~WaWindow(void);

    void withdrawTransient(void);
    void mapWindow(void);
    void drawDecor(void);
    void show(void);
    void hide(void);
    void updateAllAttributes(void);
    void redrawWindow(bool = false);
    void sendConfig(void);
    void gravitate(int);
    void updateGrabs(void);
    bool incSizeCheck(int, int, unsigned int *, unsigned int *);
    void _maximize(int, int);
    void merge(WaWindow *, int);
    void unmerge(WaWindow *);
    bool checkMoveMerge(int, int, int = 0, int = 0);    
    void raise(void);
    void lower(void);
    void move(XEvent *);
    void moveOpaque(XEvent *);
    void resize(XEvent *, int);
    void resizeOpaque(XEvent *, int);
    void resizeSmart(XEvent *);
    void resizeSmartOpaque(XEvent *);
    void maximize(void) { _maximize(-1, -1); } 
    void unMaximize(void);
    void toggleMaximize(void);
    void close(void);
    void kill(void);
    void closeKill(void);
    void shade(void);
    void unShade(void);
    void toggleShade(void);
    void sticky(void);
    void unSticky(void);
    void toggleSticky(void);
    void minimize(void);
    void unMinimize(void);
    void toggleMinimize(void);
    void fullscreenOn(void);
    void fullscreenOff(void);
    void fullscreenToggle(void);
    void setDecor(long);
    void decorTitleOn(void) { setDecor(decormask | DecorTitleMask); }
    void decorBorderOn(void) { setDecor(decormask | DecorBorderMask); }
    void decorHandlesOn(void) { setDecor(decormask | DecorHandlesMask); }
    void decorAllOn(void) { setDecor(DecorAllMask); }
    void decorTitleOff(void) { setDecor(decormask & ~DecorTitleMask); }
    void decorBorderOff(void) { setDecor(decormask & ~DecorBorderMask); }
    void decorHandlesOff(void) { setDecor(decormask & ~DecorHandlesMask); }
    void decorAllOff(void) { setDecor(0L); }
    void decorTitleToggle(void) {
        setDecor((decormask & DecorTitleMask)?
                 decormask & ~DecorTitleMask: decormask | DecorTitleMask);
    }
    void decorBorderToggle(void) {
        setDecor((decormask & DecorBorderMask)?
                 decormask & ~DecorBorderMask: decormask | DecorBorderMask);
    }
    void decorHandlesToggle(void) {
        setDecor((decormask & DecorHandlesMask)?
                 decormask & ~DecorHandlesMask: decormask | DecorHandlesMask);
    }
    void alwaysontopOn(void);
    void alwaysatbottomOn(void);
    void alwaysontopOff(void);
    void alwaysatbottomOff(void);
    void alwaysontopToggle(void);
    void alwaysatbottomToggle(void);
    void moveResize(char *);
    void moveResizeVirtual(char *);
    void moveWindowToPointer(XEvent *);
    void moveWindowToSmartPlace(void);
    void findClosestWindow(int);
    void desktopMask(char *);
    void joinDesktop(char *);
    void partCurrentJoinDesktop(char *);
    void partDesktop(char *);
    void partCurrentDesktop(void);
    void joinCurrentDesktop(void);
    void joinAllDesktops(void);
    void partAllDesktopsExceptCurrent(void);
    void mergeWithWindow(char *, int);
    void cloneMergeWithWindow(char *s) { mergeWithWindow(s, CloneMergeType); }
    void vertMergeWithWindow(char *s) { mergeWithWindow(s, VertMergeType); }
    void horizMergeWithWindow(char *s) { mergeWithWindow(s, HorizMergeType); }
    void explode(void);
    void toFront(void);
    void mergeTo(XEvent *, int);
    void cloneMergeTo(XEvent *e) { mergeTo(e, CloneMergeType); }
    void vertMergeTo(XEvent *e) { mergeTo(e, VertMergeType); }
    void horizMergeTo(XEvent *e) { mergeTo(e, HorizMergeType); }
    void unMergeMaster(XEvent *) { if (master) master->unmerge(this); }
    void setMergeMode(char *);
    void nextMergeMode(void);
    void prevMergeMode(void);
    void windowStateCheck(bool = false);
    void setWindowState(long int);
    void addMonitor(AWindowObject *, long int);
    void removeMonitor(AWindowObject *);

#ifdef    SHAPE
    void shapeEvent(void);
    
    ShapeInfo *shapeinfo;
    bool shaped;
#endif // SHAPE


    char *name, *wclass, *wclassname, *host, *pid, *rid;
    int realnamelen;
    bool has_focus, want_focus, dontsend, deleted, hidden, init_done, remap,
      mapped;
    Display *display;
    Meawm_NG *meawm_ng;
    int screen_number, state, restore_shade, old_bw;
    unsigned int top_spacing, bottom_spacing, left_spacing, right_spacing;
    WaFrameWindow *frame;
    WaWindowAttributes attrib, old_attrib, restore_max;
    long int wstate, old_wstate;
    SizeStruct size;
    long int protocol_mask;
    bool input_field;
    NetHandler *net;
    WMstrut *wm_strut;
    Window transient_for;
    bool urgent;
    list<Window> transients;
    unsigned int desktop_mask;
    list<WaWindow *> merged;
    WaWindow *master;
    int mergetype, mergemode;
    bool mergedback;
    Window window_group;
    list<FlagMonitor *> monitors;
    RenderGroup *wm_icon_image, *wm_icon_svg;
    long decormask;
    long int functions;
    int pending_unmaps;
    
private:
    void reparentWin(void);
    void initPosition(void);
    void calcSpacing(void);
    void drawOutline(int, int, int, int);
    void clearOutline(void);
    bool _moveOpaque(XEvent *, int, int, int, int, list<XEvent *> *);
    void signalMonitors(void);
    
    bool move_resize, sendcf;
    bool outline_state;
    int outl_x, outl_y;
    unsigned int outl_w, outl_h;
};

class WaFrameWindow : public RootWindowObject {
public:
    WaFrameWindow(WaWindow *, WaStringMap *);
    ~WaFrameWindow(void);

    void styleUpdate(bool, bool);

    WaWindow *wa;
    WaWindowAttributes attrib;

#ifdef    SHAPE
    void startRender(void);
    void endRender(Pixmap);
    void addShapeInfo(ShapeInfo *);
    void removeShapeInfo(ShapeInfo *);
    void shapeUpdateNotify(void);
    void applyShape(void);

    Window apply_shape_buffer;
    bool apply_shape;
    unsigned shape_count;
    list<ShapeInfo *> shape_infos;
#endif // SHAPE
    
};

#ifdef    SHAPE
class ShapeInfo {
public:
    ShapeInfo(Window);

    void setShapeOffset(int, int);
    
    Window window;
    int xoff;
    int yoff;
    bool in_list;
};
#endif // SHAPE

#endif // __Window_hh
