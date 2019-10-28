/* Style.hh

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

#ifndef __Style_hh
#define __Style_hh

extern "C" {
#include <X11/Xlib.h>

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H
    
}

class SubwinInfo;
class Style;
class StyleRegex;

#include "Action.hh"
#include "Render.hh"

#define MAX_SUBWIN_LEVEL 16

class WaScreen;
class ShapeInfo;
class WaFrameWindow;
class SubWindowObject;
class FrameSubWindowObject;
class RootWindowObject;

class StyleBuffer {
public:
    Style *style;
    
    Pixmap bg;
    unsigned int width, height;
    int x, y;
    map<RenderOpText *, char *> *text_table;
    map<RenderGroup *, CacheSurface *> cache_table;
    map<int, RenderGroup *> dynamic_groups;
    CacheSurface *surface;
    long int serial;
};

class DWindowObject : public AWindowObject {
public:
    DWindowObject(WaScreen *, Window, int, WaStringMap *, char *);
    virtual ~DWindowObject(void);
    
    void updateSubwindows(void);
    void destroySubwindows(void);
    void resetStyle(void);
    void initTextOpTable(void);
    void setDynamicGroup(int, RenderGroup *);
    void pushRenderEvent(void);
    void renderWindow(cairo_t *cr, bool = false);
    
    virtual void styleUpdate(bool, bool);
    virtual void currentPositionAndSize(int *, int *,
                                        unsigned int *, unsigned int *) = 0;
    virtual WaSurface *getBgInfo(DWindowObject **, int *, int *) = 0;
    virtual void evalWhatToRender(bool, bool, bool *, bool *, bool *) = 0;
    virtual void startRender(void) {}
    virtual void endRender(Pixmap) {}

    void commonStyleUpdate(void);
    void commonEvalWhatToRender(bool, bool, bool *, bool *, bool *);

#ifdef    SHAPE
    virtual void setShape(Pixmap);
    virtual void unsetShape(void);

    void commonSetShape(Pixmap);
    void commonUnsetShape(void);
#endif // SHAPE
    
    bool hidden, force_texture, force_alpha, force_setbg;
    map<int, SubWindowObject *> subs;
    map<int, RenderGroup *> dynamic_groups;
    RootWindowObject *decor_root;
    
    Style *styles[WIN_STATE_LAST];
    Style *default_styles[WIN_STATE_LAST];

    StyleBuffer style_buffers[WIN_STATE_LAST];

    StyleBuffer *sb;
    Style *style;

    bool mapRequest;

#ifdef    THREAD
    pthread_mutex_t __win__render_safe_mutex;
#endif // THREAD
    
};

class SubWindowObject : public DWindowObject {
public:
    SubWindowObject(DWindowObject *, SubwinInfo *);
    virtual ~SubWindowObject(void);

    virtual void showSubwindow(void);
    virtual void hideSubwindow(void);
    
    void currentPositionAndSize(int *, int *,
                                unsigned int *, unsigned int *);
    void evalWhatToRender(bool, bool, bool *, bool *, bool *);
    WaSurface *getBgInfo(DWindowObject **, int *, int *);

    DWindowObject *parent;
    SubwinInfo *sub_info;
    
    unsigned int subwin_level;
};

class FrameSubWindowObject : public SubWindowObject {
public:
    FrameSubWindowObject(DWindowObject *, SubwinInfo *);
    ~FrameSubWindowObject(void);

    void showSubwindow(void);
    void hideSubwindow(void);
    void currentPositionAndSize(int *, int *,
                                unsigned int *, unsigned int *);
    void evalWhatToRender(bool, bool, bool *, bool *, bool *);
    WaSurface *getBgInfo(DWindowObject **, int *, int *);

    WaFrameWindow *frame;
    int last_x, last_y, current_x, current_y;

#ifdef    SHAPE
    void setShape(Pixmap);
    void unsetShape(void);

    ShapeInfo *shapeinfo;
#endif // SHAPE
    
};

class RootWindowObject : public DWindowObject {
public:
    RootWindowObject(WaScreen *, Window, int, WaStringMap *, char *);
    virtual ~RootWindowObject(void);

    void currentPositionAndSize(int *, int *,
                                unsigned int *, unsigned int *);
    void evalWhatToRender(bool, bool, bool *, bool *, bool *);
    WaSurface *getBgInfo(DWindowObject **, int *, int *);

#ifdef    THREAD
    pthread_mutex_t __win__render_mutex;
    pthread_cond_t  __win__render_cond;
    unsigned int    __win__render_count;
    unsigned int    __win__render_get_count;
#endif // THREAD
    
};

#include "Regex.hh"
#include "Screen.hh"

typedef enum {
    VerticalOrientationType,
    HorizontalOrientationType
} OrientationType;

class SubwinInfo : public RefCounted<SubwinInfo> {
public:
    SubwinInfo(int _ident, bool _raised) : RefCounted<SubwinInfo>(this) {
        ident = _ident;
        raised = _raised;
    }
    
    int ident;
    bool raised;
};

class Style : public RenderGroup {
public:
    Style(WaScreen *, char *);
    ~Style(void);

    void clear(void);
    void inheritContent(Style *);
    void inheritAttributes(Style *);
    void applyAttributes(Parser *, Tst<char *> *);

    bool translucent, shaped, focusable;
    Cursor cursor;
    list<SubwinInfo *> *subs;
    list<RenderOpText *> *textops;
    double left_spacing, right_spacing, top_spacing, bottom_spacing,
        grid_spacing;
    LenghtUnitType left_spacing_u, right_spacing_u, top_spacing_u,
        bottom_spacing_u, grid_spacing_u;
    OrientationType orientation;
    RenderGroup *shapemask;
    long int serial;
};

class StyleRegex : public WindowRegex {
public:
    StyleRegex(int s, char *wr) : WindowRegex(s, wr) { style = NULL; }

    inline Style *match(WaStringMap *m, int s, char *w = NULL) {
        if (((WindowRegex *) this)->match(m, s, w))
            return (Style *) style->ref();
        else
            return NULL;
    }
    
    Style *style;
};

#ifdef    THREAD
#  define DWIN_RENDER_GET(win) { \
      if (__render_thread_count) { \
          win->decor_root->__win__render_get_count++; \
          if (win->decor_root->__win__render_get_count == 1) { \
              pthread_mutex_lock(&win->decor_root->__win__render_mutex); \
              while (win->decor_root->__win__render_count) \
                  pthread_cond_wait(&win->decor_root->__win__render_cond, \
                                    &win->decor_root->__win__render_mutex); \
          } \
      } \
  }
#  define DWIN_RENDER_RELEASE(win) { \
      if (__render_thread_count) { \
          win->decor_root->__win__render_get_count--; \
          if (win->decor_root->__win__render_get_count == 0) { \
              pthread_mutex_unlock(&win->decor_root->__win__render_mutex); \
          } \
      } \
  }
#  define DWIN_RENDER_RELEASE_BY_THREAD(win) { \
      if (__render_thread_count) { \
          pthread_mutex_unlock(&win->decor_root->__win__render_mutex); \
      } \
  }
#  define DWIN_RENDER_LOCK(win) { \
      if (__render_thread_count) { \
          pthread_mutex_lock(&win->decor_root->__win__render_mutex); \
      } \
  }
#  define DWIN_RENDER_BROADCAST(win) { \
      if (__render_thread_count) { \
          pthread_mutex_unlock(&win->decor_root->__win__render_mutex); \
          pthread_cond_broadcast(&win->decor_root->__win__render_cond); \
      } \
  }
#  define DWIN_RENDER_SAFE_RELEASE(win) { \
      if (__render_thread_count) { \
          pthread_mutex_unlock(&win->__win__render_safe_mutex); \
      } \
  }
#  define DWIN_RENDER_SAFE_LOCK(win) { \
      if (__render_thread_count) { \
          pthread_mutex_lock(&win->__win__render_safe_mutex); \
      } \
  }
#else  // !THREAD
#  define DWIN_RENDER_GET(win)
#  define DWIN_RENDER_RELEASE(win)
#  define DWIN_RENDER_RELEASE_BY_THREAD(win)
#  define DWIN_RENDER_LOCK(win)
#  define DWIN_RENDER_BROADCAST(win)
#  define DWIN_RENDER_SAFE_RELEASE(win)
#  define DWIN_RENDER_SAFE_LOCK(win)
#endif // THREAD

#endif // __Style_hh
