/* Style.cc

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

extern "C" {
#include <X11/Xlib.h>

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_STRINGS_H
#  include <strings.h>
#endif // HAVE_STRINGS_H
    
#ifdef    HAVE_LIBGEN_H
#  include <libgen.h>
#else
    inline char *basename(char *name) {
        int i = strlen(name);
        for (; i >= 0 && name[i] != '/'; i--);
        if (name[i] == '/') i++;
        return &name[i];
    }
#endif // HAVE_LIBGEN_H

#ifdef    LIMITS_H
#  include <limits.h>
#endif // LIMITS_H

#ifdef    HAVE_UNISTD_H
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_MATH_H
#  include <math.h>
#endif // HAVE_MATH_H

}

#include "Style.hh"

Style::Style(WaScreen *_ws, char *_name) : RenderGroup(_ws, _name) {
    translucent = shaped = false;
    focusable = false;
    textops = NULL;
    subs = NULL;
    cursor = (Cursor) 0;
    left_spacing = right_spacing = top_spacing = bottom_spacing =
        grid_spacing = 0;
    left_spacing_u = right_spacing_u = top_spacing_u = bottom_spacing_u =
        grid_spacing_u = PXLenghtUnitType;
    _w = RENDER_WIDTH_DEFAULT;
    _h = RENDER_HEIGHT_DEFAULT;
    _wu = _hu = PXLenghtUnitType;
    is_a_style = true;
    shapemask = NULL;
    orientation = VerticalOrientationType;
    serial = 0;
}

Style::~Style(void) {
    if (shapemask) shapemask->unref();
    clear();
}

void Style::clear(void) {
    if (textops) {
        while (! textops->empty()) {
            textops->back()->unref();
            textops->pop_back();
        }
        delete textops;
        textops = NULL;
    }
    
    if (subs) {
        while (! subs->empty()) {
            subs->back()->unref();
            subs->pop_back();
        }
        delete subs;
        subs = NULL;
    }
    
    ((RenderGroup *) this)->clear();
}

void Style::inheritContent(Style *inherit_style) {
    if (inherit_style->subs) {
        list<SubwinInfo *>::iterator it = inherit_style->subs->begin();
        for (; it != inherit_style->subs->end(); it++) {
            if (! subs) subs = new list<SubwinInfo *>;
            subs->push_back((*it)->ref());
        }
    }
    ((RenderGroup *) this)->inheritContent((RenderGroup *) inherit_style);
}

void Style::inheritAttributes(Style *inherit_style) {
    translucent = inherit_style->translucent;
    focusable = inherit_style->focusable;
    shaped = inherit_style->shaped;
    left_spacing = inherit_style->left_spacing;
    right_spacing = inherit_style->right_spacing;
    top_spacing = inherit_style->top_spacing;
    bottom_spacing = inherit_style->bottom_spacing;
    grid_spacing = inherit_style->grid_spacing;
    left_spacing_u = inherit_style->left_spacing_u;
    right_spacing_u = inherit_style->right_spacing_u;
    top_spacing_u = inherit_style->top_spacing_u;
    bottom_spacing_u = inherit_style->bottom_spacing_u;
    grid_spacing_u = inherit_style->grid_spacing_u;
    _w = inherit_style->_w;
    _h = inherit_style->_h;
    _wu = inherit_style->_wu;
    _hu = inherit_style->_hu;
    cursor = inherit_style->cursor;
    orientation = inherit_style->orientation;
}

void Style::applyAttributes(Parser *parser, Tst<char *> *attr) {
    char *value = parser->attrGetString(attr, "inherit", NULL);
    if (value) {
        Style *inherit_style = ws->getStyleNamed(value, false);
        if (inherit_style == this) {
            parser->warning("style %s cannot inherit itself\n", name);
        } else if (inherit_style) {
            inheritAttributes(inherit_style);
        } else {
            if (! parser->attrGetBool(attr, "ignore_missing", false))
                parser->warning("undefined style: %s\n", value);
        }
    }
    
    ((RenderGroup *) this)->applyAttributes(parser, attr);
    shaped = parser->attrGetBool(attr, "alpha_level_shape", shaped);
    translucent = parser->attrGetBool(attr, "translucent", translucent);
    focusable = parser->attrGetBool(attr, "focusable", focusable);
    left_spacing = parser->attrGetLength(attr, "left_spacing", left_spacing,
                                         &left_spacing_u);
    right_spacing = parser->attrGetLength(attr, "right_spacing", right_spacing,
                                          &right_spacing_u);
    top_spacing = parser->attrGetLength(attr, "top_spacing", top_spacing,
                                        &top_spacing_u);
    bottom_spacing = parser->attrGetLength(attr, "bottom_spacing",
                                           bottom_spacing, &bottom_spacing_u);
    grid_spacing = parser->attrGetLength(attr, "grid_spacing", grid_spacing,
                                         &grid_spacing_u);
    value = parser->attrGetString(attr, "cursor", NULL);
    if (value) {
        
#ifdef    XCURSOR
        char *cursorfile = smartfile(value, parser->filename, false);
        if (cursorfile)
            cursor = ws->meawm_ng->cursor->getCursor(cursorfile);
        else
#endif // XCURSOR
            
            cursor = ws->meawm_ng->cursor->getCursor(value);
    }
    value = parser->attrGetString(attr, "orientation", NULL);
    if (value && (! strcmp(value, "horizontal")))
        orientation = HorizontalOrientationType;
    
}

DWindowObject::DWindowObject(WaScreen *_ws, Window _id, int _type,
                             WaStringMap *_sm, char *_name) :
    AWindowObject(_ws, _id, _type, _sm, _name) {
    style = NULL;
    sb = NULL;
    hidden = force_texture = force_alpha = force_setbg = true;
    mapRequest = false;

#ifdef    THREAD
    if (__render_thread_count)
        pthread_mutex_init(&__win__render_safe_mutex, NULL);
#endif // THREAD

    memset(styles, 0, sizeof(styles));
    memset(default_styles, 0, sizeof(default_styles));

    for (int i = 0; i < WIN_STATE_LAST; i++) {
        style_buffers[i].style = NULL;
        style_buffers[i].surface = NULL;
        style_buffers[i].text_table = NULL;
        style_buffers[i].x = INT_MAX;
        style_buffers[i].y = INT_MAX;
        style_buffers[i].bg = None;
        style_buffers[i].width = style_buffers[i].height = 0;
    }
}

DWindowObject::~DWindowObject(void) {

    DWIN_RENDER_GET(this);

    RENDER_LIST_LOCK;

    list<DWindowObject *>::iterator it = __render_list.begin();
    while (it != __render_list.end()) {
        if (*it == this) it = __render_list.erase(it);
        else it++;
    }
    
    RENDER_LIST_RELEASE;

    DWIN_RENDER_RELEASE(this);
    
#ifdef    THREAD
    if (__render_thread_count)
        pthread_mutex_destroy(&__win__render_safe_mutex);
#endif // THREAD

    MAPUNREFSECOND(dynamic_groups);
    
    for (int i = 0; i < WIN_STATE_LAST; i++) {
        if (style_buffers[i].style) style_buffers[i].style->unref();
        if (style_buffers[i].surface) style_buffers[i].surface->unref();
        MAPUNREFSECOND(style_buffers[i].cache_table);
        if (style_buffers[i].text_table)
            MAPPTRCLEAR(style_buffers[i].text_table);
        MAPUNREFSECOND(style_buffers[i].dynamic_groups);
    }

    for (int i = 0; i < WIN_STATE_LAST; i++) {
        if (styles[i]) styles[i]->unref();
        if (default_styles[i]) default_styles[i]->unref();
    }

    if (style) style->unref();
}

void DWindowObject::resetStyle(void) {
    if (! ws) return;
    
    list<StyleRegex *> *ls = NULL;

    destroySubwindows();

    force_texture = true;

    for (int i = 0; i < WIN_STATE_LAST; i++) {
        if (style_buffers[i].style) style_buffers[i].style->unref();
        style_buffers[i].style = NULL;
        if (style_buffers[i].surface) style_buffers[i].surface->unref();
        style_buffers[i].surface = NULL;
        MAPUNREFSECOND(style_buffers[i].cache_table);
        if (style_buffers[i].text_table)
            MAPPTRCLEAR(style_buffers[i].text_table);
        style_buffers[i].text_table = NULL;
        style_buffers[i].x = INT_MAX;
        style_buffers[i].y = INT_MAX;
        style_buffers[i].bg = None;
        style_buffers[i].width = style_buffers[i].height = 0;
        MAPUNREFSECOND(style_buffers[i].dynamic_groups);
    }    

    for (int i = 0; i < WIN_STATE_LAST; i++) {
        if (styles[i]) styles[i]->unref();
        if (default_styles[i]) default_styles[i]->unref();
    }

    memset(styles, 0, sizeof(styles));
    memset(default_styles, 0, sizeof(default_styles));
 
    switch (decor_root->type) {
        case WindowFrameType:
            ls = &ws->window_styles;
            break;
        case DockHandlerType:
            ls = &ws->dockappholder_styles;
            break;
        case RootType:
            ls = &ws->root_styles;
            break;
        case MenuType:
        case MenuItemType:
            ls = &ws->menu_styles;
            break;
    }    
    
    if (style) style->unref();
    style = NULL;
    sb = NULL;
    
    if (ls) {
        list<StyleRegex *>::iterator it = ls->begin();
        for (; it != ls->end(); it++) {
            if ((styles[WIN_STATE_PASSIVE] =
                 (*it)->match(ids, WIN_STATE_PASSIVE, window_name))) {
                break;
            }
        }
        for (it = ls->begin(); it != ls->end(); it++) {
            if ((styles[WIN_STATE_ACTIVE] =
                 (*it)->match(ids, WIN_STATE_ACTIVE, window_name))) {
                break;
            }
        }
        for (it = ls->begin(); it != ls->end(); it++) {
            if ((styles[WIN_STATE_HOVER] =
                 (*it)->match(ids, WIN_STATE_HOVER, window_name))) {
                break;
            }
        }
        for (it = ls->begin(); it != ls->end(); it++) {
            if ((styles[WIN_STATE_PRESSED] =
                 (*it)->match(ids, WIN_STATE_PRESSED, window_name))) {
                break;
            }
        }
    }

    if (! styles[WIN_STATE_PASSIVE])
        styles[WIN_STATE_PASSIVE] = (Style *) (*ws->styles.begin())->ref();
    
    for (int i = 0; i < WIN_STATE_LAST; i++) {
        if (styles[i]) default_styles[i] = (Style *) styles[i]->ref();
    }

    sb = &style_buffers[WIN_STATE_PASSIVE];
    style = (Style *) styles[WIN_STATE_PASSIVE]->ref();
}

void DWindowObject::updateSubwindows(void) {
    list<SubwinInfo *>::iterator subinfo_it;
    map<int, SubWindowObject *>::iterator sub_it;

    if (type == SubwindowType)
        if (((SubWindowObject *) this)->subwin_level > MAX_SUBWIN_LEVEL)
            return;
    
    if (sb->style->subs) {
        subinfo_it = sb->style->subs->begin();
        for (; subinfo_it != sb->style->subs->end(); subinfo_it++) {
            if ((subs.find((*subinfo_it)->ident) == subs.end())) {
                SubWindowObject *sw;
                bool ancestor_failure = false;

                DWindowObject *ancestor = this;
                while (ancestor->type == SubwindowType) {
                    if (((SubWindowObject *) ancestor)->sub_info->ident ==
                        (*subinfo_it)->ident) {
                        ancestor_failure = true;
                        ws->showWarningMessage(
                            __FUNCTION__, "could not create sub-window, an "
                            "ancestor had the same sub-window ID.");
                        break;
                    }
                    ancestor = ((SubWindowObject *) ancestor)->parent;
                }

                if (! ancestor_failure) {
                    if (type == WindowFrameType)
                        sw = new FrameSubWindowObject(this, *subinfo_it);
                    else
                        sw = new SubWindowObject(this, *subinfo_it);
                    
                    subs.insert(make_pair((*subinfo_it)->ident, sw));

                    sw->commonStyleUpdate();

                    if (sw->decor_root->type == WindowFrameType) {
                        EventDetail ed;
                        ed.x = ed.y = INT_MAX;
                        ed.x11mod = ed.wamod = 0;
                        ed.type = WindowStateChangeNotify;
                        ws->meawm_ng->eh->evAct(NULL, sw->id, &ed);
                    }
                }
            }
        }
    }
        
    sub_it = subs.begin();
    for (; sub_it != subs.end(); sub_it++) {
        SubWindowObject *sw = (*sub_it).second;
        bool shown = false;
        if (sb->style->subs) {
            subinfo_it = sb->style->subs->begin();
            for (; subinfo_it != sb->style->subs->end(); subinfo_it++) {
                if ((*sub_it).first == (*subinfo_it)->ident) {
                    sw->hidden = false;
                    shown = true;

                    MAPUNREFSECOND(sw->dynamic_groups);
                    map<int, RenderGroup *>::iterator dit =
                        dynamic_groups.begin();
                    for (; dit != dynamic_groups.end(); dit++) {
                        sw->dynamic_groups.insert(
                            make_pair((*dit).first,
                                      (RenderGroup *) ((*dit).second)->ref()));
                    }
                    
                    sw->showSubwindow();
                    break;
                }
            }
        }
        if ((! shown) && (! sw->hidden)) {
            sw->hidden = true;
            sw->hideSubwindow();
        }
    }

    if (type == WindowFrameType && sb->style->subs) {
        XLowerWindow(ws->display, ((WaFrameWindow *) this)->wa->id);
    }
}

void DWindowObject::destroySubwindows(void) {
    while (! subs.empty()) {
        ((*subs.begin()).second)->unref();
        subs.erase(subs.begin());
    }
}

void DWindowObject::initTextOpTable(void) {
    if (type == WindowFrameType) return;
        
    if (sb->text_table)
        MAPPTRCLEAR(sb->text_table);
    
    sb->text_table = NULL;
    if (sb->style->textops) {
        list<RenderOpText *>::iterator tit =
            sb->style->textops->begin();
        for (; tit != sb->style->textops->end(); tit++) {
            if (! (*tit)->is_static) {
                if (! sb->text_table)
                    sb->text_table =
                        new map<RenderOpText *, char *>;
                sb->text_table->insert(
                    make_pair(*tit, WA_STRDUP("")));
            }
        }
    }
}

void DWindowObject::setDynamicGroup(int type, RenderGroup *group) {
    map<int, RenderGroup *>::iterator dit =
        dynamic_groups.find(type);
    if (dit != dynamic_groups.end()) {
        (*dit).second->unref();
        dynamic_groups.erase(dit);
    }
    
    if (group)
        dynamic_groups.insert(make_pair(type, (RenderGroup *) group->ref()));
    
    map<int, SubWindowObject *>::iterator it = subs.begin();
    for (; it != subs.end(); it++)
        ((*it).second)->setDynamicGroup(type, group);
}

void DWindowObject::commonStyleUpdate(void) {
    int current_window_state =
        STATE_FROM_MASK_AND_LIST(window_state_mask, styles);
    
    if (style) style->unref();
    style = (Style *) styles[current_window_state]->ref();
    
    if (sb->style != style ||
        (sb->style && sb->style->serial != sb->serial)) {
        sb = &style_buffers[current_window_state];

        force_setbg = true;
        
        if (sb->style == style) {
            force_alpha = true;
            
            if (! sb->surface)
                force_texture = true;
                    
            if (sb->serial != style->serial) {
                force_texture = true;
                if (sb->text_table) {
                    MAPPTRCLEAR(sb->text_table);
                    sb->text_table = NULL;
                }
                
                MAPUNREFSECOND(sb->cache_table);
                
                initTextOpTable();
            }
        } else {
            if (sb->style) sb->style->unref();
            sb->style = (Style *) style->ref();
            if (sb->surface) sb->surface->unref();
            sb->surface = NULL;

            MAPUNREFSECOND(sb->dynamic_groups);
            
            sb->bg = None;
            sb->width = sb->height = 0;
            if (sb->text_table)
                MAPPTRCLEAR(sb->text_table);
            sb->text_table = NULL;
            sb->x = INT_MAX;
            sb->y = INT_MAX;
            
            force_texture = true;
            
            MAPUNREFSECOND(sb->cache_table);
            
            initTextOpTable();
        }

        if (sb->style->cursor)
            XDefineCursor(ws->display, id, sb->style->cursor);
        
        updateSubwindows();
        
        sb->serial = sb->style->serial;
    }
}

void DWindowObject::styleUpdate(bool, bool) {
    pushRenderEvent();
}

void DWindowObject::renderWindow(cairo_t *cr, bool from_bgparent) {
    int x, y;
    unsigned int w, h;
    bool render_texture = false;
    bool render_alpha = false;
    bool update_shape = false;
    Pixmap pixmap = (Pixmap) 0;
    Pixmap bitmap = (Pixmap) 0;

    DWIN_RENDER_SAFE_LOCK(this);

    if (force_texture) {
        render_texture = true;
        force_texture = false;
        force_alpha = false;
    } else if (force_alpha) {
        render_alpha = true;
        force_alpha = false;
    }

    startRender();

    currentPositionAndSize(&x, &y, &w, &h);

    bool pos_change, size_change;
    if (x != sb->x || y != sb->y) pos_change = true;
    else pos_change = false;
    if (w != sb->width || h != sb->height) size_change = true;
    else size_change = false;

    if (size_change)
        render_texture = true;
    else if (pos_change)
        render_alpha = true;
    
    sb->x = x;
    sb->y = y;
    sb->width = w;
    sb->height = h;
    
    evalWhatToRender(pos_change, size_change,
                     &render_texture, &render_alpha, &update_shape);
    
    commonEvalWhatToRender(pos_change, size_change,
                           &render_texture, &render_alpha, &update_shape);
    
    if (type == WindowFrameType) {
        render_texture = false;
        render_alpha = false;
        update_shape = false;
    }

    if (render_texture) {
        render_alpha = true;
        update_shape = true;
    }

    bool alpha = (sb->style->translucent && type != RootType);
    
    bool shape = false;
    
#ifdef    SHAPE
    shape = (sb->style->shaped && type != RootType);
#endif // SHAPE

    /* ARGB visual */
    if (ws->screen_depth == 32) {
      if (alpha) {
        int p_x, p_y;
        DWindowObject *bgparent;
        WaSurface *bgsurface = getBgInfo (&bgparent, &p_x, &p_y);
        if (bgsurface && bgsurface != ws->bg_surface)
        {
          if (ws->meawm_ng->client_side_rendering) {
            if (!bgsurface->data)
              alpha = false;
          } else {
            if (bgsurface->pixmap == None)
              alpha = false;
          }
        } else
          alpha = false;
      }
      
#ifdef    SHAPE
        shape = false;
#endif // SHAPE

    }
    
    if (render_texture || (alpha && render_alpha)) {
        Pixmap root_pixmap = None;
        unsigned char *root_data = NULL;
        cairo_surface_t *root_surface = NULL;
        
        if (alpha || shape) {
            int p_x, p_y;
            DWindowObject *bgparent;
            WaSurface *bgsurface = getBgInfo(&bgparent, &p_x, &p_y);

            if (! from_bgparent) {
                DWIN_RENDER_SAFE_LOCK(bgparent);
            }
              
              if (ws->meawm_ng->client_side_rendering) {
                root_data = ws->getRootBgImage(
                  (bgsurface)? bgsurface->data: NULL,
                  (bgsurface)? bgsurface->width: 0,
                  (bgsurface)? bgsurface->height: 0,
                  p_x, p_y, w, h);
                root_surface =
                  cairo_image_surface_create_for_data(root_data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 w, h, w * sizeof(WaPixel));
              } else {
                root_pixmap = ws->getRootBgPixmap(
                  (bgsurface)? bgsurface->pixmap: None,
                  (bgsurface)? bgsurface->width: 0,
                  (bgsurface)? bgsurface->height: 0,
                  p_x, p_y, w, h);

                  root_surface =
                    cairo_xlib_surface_create(ws->display, root_pixmap,
                                              ws->visual,
                                              (ws->screen_depth == 32)?
                                              CAIRO_FORMAT_ARGB32:
                                              CAIRO_FORMAT_RGB24,
                                              ws->colormap);
              }

              if (! from_bgparent) {
                DWIN_RENDER_SAFE_RELEASE(bgparent);
              }
        }

        CacheSurface *return_surface;
        sb->style->parent_surface = root_surface;
        sb->style->return_surface = &return_surface;
        sb->style->render(this, cr, NULL, w, h);
        sb->style->return_surface = NULL;
        sb->style->parent_surface = NULL;
        
        if (alpha || shape) {
            cairo_destroy(cr);
            cr = cairo_create(root_surface);
            cairo_set_operator(cr, RENDER_OPERATOR_DEFAULT);
            if (sb->style->xrop_set)
                cairo_set_operator(cr, sb->style->xrop);
            cairo_identity_matrix(cr);
            cairo_set_source_surface(cr, return_surface->crsurface, 0., 0.);
            cairo_rectangle(cr, 0., 0., w, h);
            cairo_clip(cr);
            cairo_paint_with_alpha(cr, sb->style->opacity);
            cairo_reset_clip(cr);
        }
        
        if (shape) {
            cairo_surface_t *alpha_surface;
            unsigned char *shape_data;
            if (ws->meawm_ng->client_side_rendering) {
                shape_data = new unsigned char[w * h * sizeof(WaPixel)];
                memset (shape_data, 0, w * h * sizeof(WaPixel));
                alpha_surface =
                    cairo_image_surface_create_for_data(shape_data,
                                                   CAIRO_FORMAT_ARGB32,
                                                   w, h, w * sizeof(WaPixel));
            } else {
                GC gc;
                XGCValues gcv;
                
                bitmap = XCreatePixmap(ws->display, ws->id, w, h, 1);

                gcv.foreground = 0x00000000;
                gc = XCreateGC(ws->display, bitmap, GCForeground, &gcv);
                XFillRectangle(ws->display, bitmap, gc, 0, 0, w, h);
                XFreeGC(ws->display, gc);
                
                alpha_surface =
                    cairo_xlib_surface_create(ws->display, bitmap,
                                              NULL, CAIRO_FORMAT_A1,
                                              ws->colormap);
            }

            cairo_destroy(cr);
            cr = cairo_create(alpha_surface);
            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_surface(cr, return_surface->crsurface, 0., 0.);
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_rectangle(cr, 0, 0, w, h);
            cairo_clip(cr);
            cairo_paint_with_alpha(cr, sb->style->opacity);
            cairo_reset_clip(cr);
            
            if (sb->style->shapemask) {
                cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
                sb->style->shapemask->render(this, cr, alpha_surface, w, h);
            } else {
                cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
                cairo_set_source_surface(cr, return_surface->crsurface, 0., 0.);
                cairo_rectangle(cr, 0.0, 0.0, w, h);
                cairo_clip(cr);
                cairo_paint_with_alpha(cr, sb->style->opacity);
                cairo_reset_clip(cr);
            }
            
            if (ws->meawm_ng->client_side_rendering) {
                /* XXX: can this be done with CAIRO_FORMAT_A1 image data
                   instead? */
                unsigned int *src = (unsigned int *) shape_data;
                unsigned int _x, _y, bit = 0x01;
                unsigned char *shape_map = new unsigned char[(w + 7) / 8 * h];
                unsigned char *dst = shape_map;
                unsigned char value = 0;
                unsigned char src_alpha;
                for (_y = 0; _y < h; _y++) {
                    for (_x = 0; _x < w; _x++) {
                        src_alpha = *src >> 24;
                        if (src_alpha >= 127) value |= bit;
                        if (bit == 0x80) {
                            *dst++ = value;
                            value = 0;
                            bit = 0x01;
                        } else
                            bit <<= 1;
                        
                        src++;
                    }
                    if (bit != 0x01) *dst++ = value;
                    value = 0;
                    bit = 0x01;
                }

                bitmap = XCreateBitmapFromData(ws->display, ws->id,
                                               (char *) shape_map, w, h);
                delete [] shape_data;
                delete [] shape_map;
            }
            
            cairo_surface_destroy(alpha_surface);
        }
            
        CacheSurface *surface;
        if (root_surface) {
            return_surface->unref();
            surface = new CacheSurface(ws->display, root_surface, root_pixmap,
                                       root_data, w, h, &sb->dynamic_groups);
            surface->bitmap = bitmap;
        } else
            surface = return_surface;
        
        if (ws->meawm_ng->client_side_rendering) {
            Visual *visual;
            int depth;
            
            if (type == RootType) {
              XWindowAttributes attr;
                  
              XGetWindowAttributes (ws->display, ws->id, &attr);
              visual = attr.visual;
              depth = attr.depth;
            } else {
              visual = ws->visual;
              depth = ws->screen_depth;
            }
          
            pixmap = XCreatePixmap(ws->display, ws->id, surface->width,
                                   surface->height, depth);
            XImage *xim =
                XCreateImage(ws->display, visual, depth,
                             ZPixmap, 0, (char *) surface->data, w, h, 32, 0);
            
            GC gc = XCreateGC(ws->display, pixmap, 0, NULL);
            XPutImage(ws->display, pixmap, gc, xim, 0, 0, 0, 0, w, h);
            xim->data = NULL;
            XDestroyImage(xim);
            XFreeGC(ws->display, gc);
            surface->pixmap = pixmap;
        }
        
        if (sb->surface) sb->surface->unref();
        sb->surface = surface;
        
        pixmap = sb->surface->pixmap;
    } else if (force_setbg && sb->surface) {
        pixmap = sb->surface->pixmap;
        update_shape = true;
    }

#ifdef    SHAPE
    if (shape && update_shape) {
        if (sb->surface)
            bitmap = sb->surface->bitmap;

        if (bitmap)
           setShape(bitmap);
        
    } else if (update_shape)
       unsetShape();
#endif // SHAPE
    
    if (pixmap) {
        force_setbg = false;

        XSetWindowBackgroundPixmap(ws->display, id, pixmap); 
        XClearWindow(ws->display, id);
    }
    
    map<int, SubWindowObject *>::iterator it = subs.begin();
    for (; it != subs.end(); it++) {
        if (! ((*it).second)->hidden) {
            
            DWindowObject *dwin = (*it).second;
            
            RENDER_LIST_LOCK;

            list<DWindowObject *>::iterator it = __render_list.begin();
            while (it != __render_list.end()) {
                if (*it == dwin) it = __render_list.erase(it);
                else it++;
            }
            
            RENDER_LIST_RELEASE;
            
            dwin->renderWindow(cr, (type == WindowFrameType)? false: true);
        }
    }

    if (mapRequest) {
      if (type != RootType)
        XMapWindow (ws->display, id);
      mapRequest = false;
    }

    DWIN_RENDER_SAFE_RELEASE(this);

    endRender(pixmap);
    
}

void DWindowObject::commonEvalWhatToRender(bool, bool, bool *render_texture,
                                           bool *, bool *) {
    if (sb->style->has_dynamic_op) {
        map<int, RenderGroup *>::iterator sbdit;
        map<int, RenderGroup *>::iterator dit = dynamic_groups.begin();
        for (; dit != dynamic_groups.end(); dit++) {
            sbdit = sb->dynamic_groups.find((*dit).first);
            if (sbdit == sb->dynamic_groups.end() ||
                (*sbdit).second != (*dit).second) {
                if (sbdit != sb->dynamic_groups.end()) {
                    ((*sbdit).second)->unref();
                    sb->dynamic_groups.erase(sbdit);
                }
                sb->dynamic_groups.insert(
                    make_pair((*dit).first,
                              (RenderGroup *) ((*dit).second)->ref()));
                *render_texture = true;
            }
        }

        sbdit = sb->dynamic_groups.begin();
        for (; sbdit != sb->dynamic_groups.end(); sbdit++) {
            dit = dynamic_groups.find((*sbdit).first);
            if (dit == dynamic_groups.end()) {
                ((*sbdit).second)->unref();
                sb->dynamic_groups.erase(sbdit);
                *render_texture = true;
            }
        }
    }

    if (sb->serial != sb->style->serial) {
        *render_texture = true;
        initTextOpTable();
    }

    if (sb->text_table) {
        WaWindow *ww = getWindow();
        MenuItem *mi = getMenuItem();
        map<RenderOpText *, char *>::iterator tit = sb->text_table->begin();
        for (; tit != sb->text_table->end(); tit++) {
            char *newstr = expand(((*tit).first)->utf8, ww, mi);
            if (strcmp((*tit).second, newstr)) {
                *render_texture = true;
                delete [] (*tit).second;
                (*tit).second = newstr;
            } else
                delete [] newstr;
        }
    }
}

#ifdef    SHAPE
void DWindowObject::commonSetShape(Pixmap bitmap) {
    XShapeCombineMask(ws->display, id,
                      ShapeBounding, 0, 0, bitmap, ShapeSet);
}

void DWindowObject::commonUnsetShape(void) {
    XShapeCombineMask(ws->display, id,
                      ShapeBounding, 0, 0, None, ShapeSet);
}

void DWindowObject::setShape(Pixmap bitmap) {
    commonSetShape(bitmap);
}

void DWindowObject::unsetShape(void) {
    commonUnsetShape();
}
#endif // SHAPE

void DWindowObject::pushRenderEvent(void) {
    if (type == RootType && ws->config.external_bg) return;
    if (hidden) return;
    
    RENDER_LIST_LOCK;
    __render_list.push_back(this);
    RENDER_LIST_SIGNAL;
}

SubWindowObject::SubWindowObject(DWindowObject *_parent, SubwinInfo *sinfo) :
    DWindowObject(NULL, (Window) 0, SubwindowType, NULL, NULL) {
    parent = _parent;
    ws = parent->ws;
    sub_info = sinfo;

    if (parent->type == SubwindowType)
        subwin_level = ((SubWindowObject *) parent)->subwin_level + 1;
    else
        subwin_level = 1;
    
    decor_root = parent->decor_root;

    char *name = ws->getSubwindowName(sinfo->ident);

    if (window_name) delete [] window_name;
    if (parent->window_name) {
        window_name = new char[strlen(parent->window_name) +
                              strlen(name) + 2];
        sprintf(window_name, "%s.%s", parent->window_name, name);
    } else
        window_name = WA_STRDUP(name);
    
    XSetWindowAttributes attrib_set;
    int create_mask = CWOverrideRedirect | CWEventMask;

    attrib_set.override_redirect = true;
    attrib_set.event_mask = ButtonPressMask | ButtonReleaseMask |
        EnterWindowMask | LeaveWindowMask;
    id = XCreateWindow(ws->display, parent->id, 0, 0, 1, 1, 0,
                       CopyFromParent, CopyFromParent, CopyFromParent,
                       create_mask, &attrib_set);

    ws->meawm_ng->window_table.insert(make_pair(id, this));

    if (parent->window_state_mask & WIN_STATE_ACTIVE_MASK)
        window_state_mask = WIN_STATE_ACTIVE_MASK;
    else
        window_state_mask = 0L;
    
    if (sinfo->raised && (! (parent->type == RootType)))
        XRaiseWindow(ws->display, id);
    else XLowerWindow(ws->display, id);

    resetActionList(parent->ids->ref());
    resetStyle();
}

SubWindowObject::~SubWindowObject(void) {
    ws->meawm_ng->window_table.erase(id);
    destroySubwindows();
    XDestroyWindow(ws->display, id);
}

void SubWindowObject::showSubwindow(void) {
    XMapWindow(ws->display, id);
}

void SubWindowObject::hideSubwindow(void) {
    XUnmapWindow(ws->display, id);
}

void SubWindowObject::currentPositionAndSize(int *x, int *y,
                                             unsigned int *w,
                                             unsigned int *h) {
    double nx, ny, nw, nh;
    int ox, oy;
    unsigned int ow, oh, ud;
    Window dw;
        
    style->calcPositionAndSize(parent->sb->width, parent->sb->height,
                               ws->hdpi, ws->vdpi, &nx, &ny, &nw, &nh);
    
    *x = WA_ROUND(nx);
    *y = WA_ROUND(ny);
    *w = WA_ROUND_U(nw);
    *h = WA_ROUND_U(nh);
    if (*w == 0) *w = 1;
    if (*h == 0) *h = 1;

    XGetGeometry(ws->display, id, &dw, &ox, &oy, &ow, &oh, &ud, &ud);

    if (ow != *w || oh != *h) {
        XMoveResizeWindow(ws->display, id, *x, *y, *w,* h);
    } else if (ox != *x || oy != *y) {
        XMoveWindow(ws->display, id, *x, *y);
    }
}

void SubWindowObject::evalWhatToRender(bool, bool size_change,
                                       bool *render_texture,
                                       bool *render_alpha,
                                       bool *update_shape) {
    Window dw;
    int x, y;
    XTranslateCoordinates(ws->display, id, ws->id, 0, 0, &x, &y, &dw);

    if ((! size_change) &&
        (x > (int) ws->width || y > (int) ws->height ||
         x + (int) sb->width < 0 || y + (int) sb->height < 0)) {
        *render_texture = false;
        *render_alpha = false;
        *update_shape = false;
        return;
    }

    Pixmap newbg = (parent->sb->surface)? parent->sb->surface->pixmap: None;
    
    if (sb->bg != newbg) {
        *render_alpha = true;
        sb->bg = newbg;
    }
}

WaSurface *SubWindowObject::getBgInfo(DWindowObject **dwo, int *x, int *y) {
    *x = sb->x;
    *y = sb->y;
    *dwo = parent;
    return parent->sb->surface;
}

FrameSubWindowObject::FrameSubWindowObject(DWindowObject *_parent,
                                           SubwinInfo *sinfo) :
    SubWindowObject(_parent, sinfo) {
    last_x = last_y = current_x = current_y = 0;
    frame = (WaFrameWindow *) parent;
    
#ifdef    SHAPE
    shapeinfo = new ShapeInfo(id);
#endif // SHAPE
    
}

FrameSubWindowObject::~FrameSubWindowObject(void) {
    
#ifdef    SHAPE
    if (shapeinfo->in_list)
        frame->removeShapeInfo(shapeinfo);
    delete shapeinfo;
#endif // SHAPE
    
}

void FrameSubWindowObject::showSubwindow(void) {
    
#ifdef    SHAPE
    frame->addShapeInfo(shapeinfo);
    shapeinfo->in_list = true;
#endif // SHAPE
    
    XMapWindow(ws->display, id);
}

void FrameSubWindowObject::hideSubwindow(void) {

#ifdef    SHAPE
    if (shapeinfo->in_list) {
        frame->removeShapeInfo(shapeinfo);
        shapeinfo->in_list = false;
    }
#endif // SHAPE
    
    XUnmapWindow(ws->display, id);
}


void FrameSubWindowObject::currentPositionAndSize(int *x, int *y,
                                                  unsigned int *w,
                                                  unsigned int *h) {
    double nx, ny, nw, nh;
    int ox, oy;
    unsigned int ow, oh, ud;
    Window dw;
        
    style->calcPositionAndSize(parent->sb->width, parent->sb->height,
                               ws->hdpi, ws->vdpi, &nx, &ny, &nw, &nh);
    
    current_x = WA_ROUND(nx);
    current_y = WA_ROUND(ny);
    *w = WA_ROUND_U(nw);
    *h = WA_ROUND_U(nh);
    if (*w == 0) *w = 1;
    if (*h == 0) *h = 1;

    XGetGeometry(ws->display, id, &dw, &ox, &oy, &ow, &oh, &ud, &ud);

    if (ow != *w || oh != *h) {
        XMoveResizeWindow(ws->display, id, current_x, current_y, *w, *h);
    } else if (ox != current_x || oy != current_y) {
        XMoveWindow(ws->display, id, current_x, current_y);
    }

    XTranslateCoordinates(ws->display, id, ws->id, 0, 0, x, y, &dw);
}


void FrameSubWindowObject::evalWhatToRender(bool, bool size_change,
                                            bool *render_texture,
                                            bool *render_alpha,
                                            bool *update_shape) {
    Window dw;
    int x, y;
    XTranslateCoordinates(ws->display, id, ws->id, 0, 0, &x, &y, &dw);

    if ((! size_change) &&
        (x > (int) ws->width || y > (int) ws->height ||
         x + (int) sb->width < 0 || y + (int) sb->height < 0)) {
        *render_texture = false;
        *render_alpha = false;
        *update_shape = false;
        return;
    }

    if (current_x != last_x || current_y != last_y) {
       *update_shape = true;
    }

    Pixmap newbg = (ws->bg_surface)? ws->bg_surface->pixmap: None;

    if (sb->bg != newbg) {
        *render_alpha = true;
        sb->bg = newbg;
    }
}

WaSurface *FrameSubWindowObject::getBgInfo(DWindowObject **dwo,
                                           int *x, int *y) {
    *x = sb->x;
    *y = sb->y;
    *dwo = ws;
    return ws->bg_surface;
}

#ifdef    SHAPE
void FrameSubWindowObject::setShape(Pixmap bitmap) {
    commonSetShape(bitmap);
    shapeinfo->setShapeOffset(current_x, current_y);
    frame->shapeUpdateNotify();
    
    last_x = current_x;
    last_y = current_y;
}

void FrameSubWindowObject::unsetShape(void) {
    commonUnsetShape();
    shapeinfo->setShapeOffset(current_x, current_y);
    frame->shapeUpdateNotify();
    
    last_x = current_x;
    last_y = current_y;
}
#endif // SHAPE

RootWindowObject::RootWindowObject(WaScreen *_ws, Window _id, int _type,
                                   WaStringMap *sm, char *_wname) :
    DWindowObject(_ws, _id, _type, sm, _wname) {
    
#ifdef    THREAD
    if (__render_thread_count) {
        pthread_mutex_init(&__win__render_mutex, NULL);
        pthread_cond_init(&__win__render_cond, NULL);
        __win__render_count = 0;
        __win__render_get_count = 0;
    }
#endif // THREAD

    hidden = false;

    decor_root = this;

    resetStyle();
}

RootWindowObject::~RootWindowObject(void) {

    DWIN_RENDER_GET(this);

    destroySubwindows();
    
#ifdef    THREAD
    if (__render_thread_count) {
        pthread_mutex_destroy(&__win__render_mutex);
        pthread_cond_destroy(&__win__render_cond);
    }
#endif // THREAD

    if (type != RootType)
        XDestroyWindow(ws->display, id);

}

void RootWindowObject::currentPositionAndSize(int *x, int *y,
                                              unsigned int *w,
                                              unsigned int *h) {
    Window dw;
    unsigned int ud;
    XGetGeometry(ws->display, id, &dw, x, y, w, h, &ud, &ud);
    XTranslateCoordinates(ws->display, id, ws->id, 0, 0, x, y, &dw);
}

void RootWindowObject::evalWhatToRender(bool, bool size_change,
                                        bool *render_texture,
                                        bool *render_alpha,
                                        bool *update_shape) {

    if ((! size_change) &&
        (sb->x > (int) ws->width || sb->y > (int) ws->height ||
         sb->x + (int) sb->width < 0 || sb->y + (int) sb->height < 0)) {
        *render_texture = false;
        *render_alpha = false;
        *update_shape = false;
        return;
    }

    Pixmap newbg = (ws->bg_surface)? ws->bg_surface->pixmap: None;

    if (sb->bg != newbg) {
        *render_alpha = true;
        sb->bg = newbg;
    }
}

WaSurface *RootWindowObject::getBgInfo(DWindowObject **dwo, int *x, int *y) {
    *x = sb->x;
    *y = sb->y;
    *dwo = ws;
    return ws->bg_surface;
}
