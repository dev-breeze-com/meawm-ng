/* Render.cc

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
    
#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS    

#ifdef    HAVE_LIBGEN_H
#  include <libgen.h>
#else
    char *dirname(char *name) {
        int i = strlen(name);
        for (; i >= 0 && name[i] != '/'; i--);
        name[i] = '\0';
        return name;
    }
#endif // HAVE_LIBGEN_H

}

#include "Render.hh"
#include "Screen.hh"

#ifdef    PNG
#  include "Png.hh"
#endif // PNG

Tst<cairo_filter_t>      *filter_tst;
Tst<cairo_extend_t>      *extend_tst;
Tst<PatternType>         *pattern_type_tst;
Tst<cairo_line_cap_t>    *line_cap_tst;
Tst<cairo_line_join_t>   *line_join_tst;
Tst<cairo_fill_rule_t>   *fill_rule_tst;
Tst<DrawingOrder>        *drawing_order_tst;
Tst<cairo_font_weight_t> *weight_tst;
Tst<cairo_font_slant_t>  *slant_tst;
Tst<ImageScaleType>      *image_scale_type_tst;
Tst<int>                 *gravity_type_tst;
Tst<HorizontalAlignment> *horizontal_alignment_tst;
Tst<VerticalAlignment>   *vertical_alignment_tst;
Tst<cairo_operator_t>    *operator_tst;

WaSurface::WaSurface(Display *_display,
                     cairo_surface_t *sp, Pixmap p, Pixmap b,
                     unsigned char *_data, unsigned int w, unsigned int h) :
    RefCounted<WaSurface>(this) {
    display = _display;
    crsurface = sp;
    pixmap = p;
    bitmap = b;
    width = w;
    height = h;
    data = _data;
}

WaSurface::~WaSurface(void) {
    if (crsurface) cairo_surface_destroy(crsurface);
    if (pixmap) XFreePixmap(display, pixmap);
    if (bitmap) XFreePixmap(display, bitmap);
    if (data) delete [] data;
}

CacheSurface::CacheSurface(Display *_display, cairo_surface_t *sp, Pixmap p,
                           unsigned char *_data, unsigned int w,
                           unsigned int h, map<int, RenderGroup *> *dg) :
    WaSurface(_display, sp, p, None, _data, w, h) {
    map<int, RenderGroup *>::iterator it = dg->begin();
    for (; it != dg->end(); it++)
        dynamic_groups.insert(
            make_pair((*it).first, (RenderGroup *) ((*it).second)->ref()));
    group = NULL;
}

CacheSurface::~CacheSurface(void) {
    if (group) {
        group->cache.remove(this);
        group->unref();
    }
    MAPUNREFSECOND(dynamic_groups);
}

bool CacheSurface::dynamicGroupsMatch(DWindowObject *dwo) {
    if (dwo->sb->dynamic_groups.size() != dynamic_groups.size())
        return false;
    
    map<int, RenderGroup *>::iterator it = dynamic_groups.begin();
    for (; it != dynamic_groups.end(); it++) {
        map<int, RenderGroup *>::iterator dwo_it =
            dwo->sb->dynamic_groups.find((*it).first);
        if (dwo_it == dwo->sb->dynamic_groups.end()) return false;
        else if ((*dwo_it).second != (*it).second) return false;
    }
    
    return true;
}

WaColor::WaColor(double r, double g, double b, double a) :
    RefCounted<WaColor>(this) {
    alpha = a; red = r; green = g; blue = b;
}

bool WaColor::parseColor(WaScreen *ws, const char *spec) {
    XColor core_color;
    if (! XParseColor(ws->display, ws->colormap, spec, &core_color))
        return false;
    red = core_color.red / 65535.0;
    green = core_color.green / 65535.0;
    blue = core_color.blue / 65535.0;
    return true;
}

RenderOp::RenderOp(RenderOpType _type) : RefCounted<RenderOp>(this) {
    xrop_set = false;
    rotation = RENDER_ROTATION_DEFAULT;
    type = _type;
    _x = _x2 = RENDER_X_DEFAULT;
    _y = _y2 = RENDER_Y_DEFAULT;
    _xu = _yu = _x2u = _y2u = PXLenghtUnitType;
    _w = _h = 100;
    _wu = _hu = PCTLenghtUnitType;
    halign = RENDER_HALIGN_DEFAULT;
    valign = RENDER_VALIGN_DEFAULT;
    gravity = gravity2 = RENDER_GRAVITY_DEFAULT;
    x2_set = y2_set = false;
}

void RenderOp::inheritAttributes(RenderOp *inherit) {
    xrop_set = inherit->xrop_set;
    rotation = inherit->rotation;
    _x = inherit->_x;
    _x2 = inherit->_x2;
    _y = inherit->_y;
    _y2 = inherit->_y2;
    _xu = inherit->_xu;
    _yu = inherit->_yu;
    _x2u = inherit->_x2u;
    _y2u = inherit->_y2u;
    _w = inherit->_w;
    _h = inherit->_h;
    _wu = inherit->_wu;
    _hu = inherit->_hu;
    halign = inherit->halign;
    valign = inherit->valign;
    gravity = inherit->gravity;
    gravity2 = inherit->gravity2;
    x2_set = inherit->x2_set;
    y2_set = inherit->y2_set;
}

void RenderOp::applyAttributes(Parser *parser, Tst<char *> *attr) {
    attributes_get_render(parser, attr, &xrop, &xrop_set, &rotation);
    attributes_get_area_default(parser, attr, &_x, &_y, &_xu, &_yu, &gravity,
                                &_x2, &_y2, &_x2u, &_y2u,
                                &gravity2, &x2_set, &y2_set,
                                &_w, &_h, &_wu, &_hu, &halign, &valign);
}

void RenderOp::calcWidth(double parent_width, double hdpi,
                         unsigned int *return_width) {
    double rw;
    calc_length(_w, _wu, hdpi, parent_width, &rw);
    *return_width = WA_ROUND_U(rw);
}

void RenderOp::fcalcWidth(double parent_width, double hdpi,
                          double *return_width) {
    calc_length(_w, _wu, hdpi, parent_width, return_width);
}

void RenderOp::calcHeight(double parent_height, double vdpi,
                          unsigned int *return_height) {
    double rh;
    calc_length(_h, _hu, vdpi, parent_height, &rh);
    *return_height = WA_ROUND_U(rh);
}

void RenderOp::calcPositionAndSize(double parent_width,
                                   double parent_height,
                                   double hdpi, double vdpi,
                                   double *return_x, double *return_y,
                                   double *return_width,
                                   double *return_height) {
    calc_position_and_size(halign, valign, gravity,
                           _xu, _yu, _wu, _hu,
                           parent_width, parent_height, hdpi, vdpi,
                           _w, _h, _x, _y, return_x, return_y,
                           return_width, return_height);
    if (x2_set || y2_set) {
        double tmp_x2, tmp_y2;
        calc_position(gravity2, _x2u, _y2u, parent_width, parent_height,
                      hdpi, vdpi, _x2, _y2, &tmp_x2, &tmp_y2);
        if (x2_set) *return_width = tmp_x2 - *return_x;
        if (y2_set) *return_height = tmp_y2 - *return_y;
    }
}

void RenderOp::calcPositionFromSize(double parent_width, double parent_height,
                                    double width, double height,
                                    LenghtUnitType wu, LenghtUnitType hu,
                                    double hdpi, double vdpi,
                                    double *return_x, double *return_y) {
    calc_position_and_size(halign, valign, gravity, _xu, _yu,
                           wu, hu, parent_width, parent_height, hdpi, vdpi,
                           width, height, _x, _y, return_x, return_y,
                           NULL, NULL);
}

RenderGroup::RenderGroup(WaScreen *_ws, char *_name) :
    RenderOp(RenderOpGroupType) {
    opacity = RENDER_OPACITY_DEFAULT;
    has_dynamic_op = is_a_style = false;
    cacheable = true;
    ws = _ws;
    if (_name) name = WA_STRDUP(_name);
    else name = NULL;
    return_surface = NULL;
    parent_surface = NULL;
}

RenderGroup::~RenderGroup(void) {
    clear();
    if (name) delete [] name;
}

void RenderGroup::clear(void) {
    clearCache();
    
    while (! operations.empty()) {
        operations.back()->unref();
        operations.pop_back();
    }
}

void RenderGroup::clearCache(void) {
    while (! cache.empty())
        cache.pop_back();
}

void RenderGroup::inheritContent(RenderGroup *inherit_group) {
    list<RenderOp *>::iterator oit = inherit_group->operations.begin();
    for (; oit != inherit_group->operations.end(); oit++)
        operations.push_back((*oit)->ref());

    if (inherit_group->has_dynamic_op) has_dynamic_op = true;
    if (! inherit_group->cacheable) cacheable = false;
}

void RenderGroup::inheritAttributes(RenderGroup *inherit_group) {
    opacity = inherit_group->opacity;
    if (! inherit_group->cacheable) cacheable = false;

    ((RenderOp *) this)->inheritAttributes((RenderOp *) inherit_group);
}

void RenderGroup::applyAttributes(Parser *parser, Tst<char *> *attr) {
    char *value = parser->attrGetString(attr, "inherit", NULL);
    if (value) {
        RenderGroup *inherit_group = ws->getRenderGroupNamed(value, false);
        if (inherit_group == this) {
            parser->warning("group %s cannot inherit itself\n", name);
        } else if (inherit_group) {
            inheritAttributes(inherit_group);
        } else {
            if (! parser->attrGetBool(attr, "ignore_missing", false))
                parser->warning("undefined group: %s\n", value);
        }
    }
    
    ((RenderOp *) this)->applyAttributes(parser, attr);
    
    opacity = parser->attrGetDouble(attr, "opacity", opacity);
    cacheable = parser->attrGetBool(attr, "cache", cacheable);
}

void RenderGroup::addToCache(CacheSurface *cache_item, DWindowObject *dwo) {
    dwo->sb->cache_table.insert(
        make_pair(this, (CacheSurface *) cache_item->ref()));

    cache.push_front(cache_item);
    cache_item->group = (RenderGroup *) ref();
}

CacheSurface *RenderGroup::findCachedSurface(DWindowObject *dwo,
                                             unsigned int w, unsigned int h) {
    CacheSurface *new_cache_item = NULL;
    list<CacheSurface *>::iterator it = cache.begin();
    for (; it != cache.end(); it++) {
        if ((*it)->width == w && (*it)->height == h &&
            (*it)->dynamicGroupsMatch(dwo)) {
            new_cache_item = (CacheSurface *) (*it)->ref();
            break;
        }
    }
    
    map<RenderGroup *, CacheSurface *>::iterator mit =
        dwo->sb->cache_table.find(this);
    if (mit != dwo->sb->cache_table.end()) {
        ((*mit).second)->unref();
        dwo->sb->cache_table.erase(mit);
    }

    if (new_cache_item) {
        dwo->sb->cache_table.insert(
            make_pair(this, (CacheSurface *) new_cache_item->ref()));
    }
    
    return new_cache_item;
}

void RenderGroup::render(DWindowObject *dwo, cairo_t *cr,
                         cairo_surface_t *crsurface,
                         unsigned int w, unsigned int h) {
    double x, y, _width, _height;
    unsigned int width, height;
    Pixmap pixmap = None;
    unsigned char *data = NULL;

    if (is_a_style) {
        width = w;
        height = h;
        x = y = 0.0;
    } else {
        calcPositionAndSize(w, h, ws->hdpi, ws->vdpi, &x, &y,
                            &_width, &_height);
        width = WA_ROUND_U(_width);
        height = WA_ROUND_U(_height);
    }

    CacheSurface *cache_surface = findCachedSurface(dwo, width, height);
        
    if (! cache_surface) {
        cairo_surface_t *groupsurface;

        if (crsurface) {
            groupsurface =
                cairo_surface_create_similar(crsurface,
                                             CAIRO_CONTENT_COLOR_ALPHA,
                                             width, height);
        } else if (parent_surface) {
          groupsurface =
                cairo_surface_create_similar(parent_surface,
                                             CAIRO_CONTENT_COLOR_ALPHA,
                                             width, height);
        } else {
            if (ws->meawm_ng->client_side_rendering) {
                data = new unsigned char[width * height * sizeof(WaPixel)];
                memset (data, 0, width * height * sizeof(WaPixel));
                groupsurface =
                    cairo_image_surface_create_for_data(data,
                                                   CAIRO_FORMAT_ARGB32,
                                                   width, height, width * 4);
            } else {
                GC gc;
                XGCValues gcv;
                Visual *visual;
                Colormap colormap;
                int depth = ws->screen_depth;

                if (dwo->type == RootType) {
                  XWindowAttributes attr;
                  
                  XGetWindowAttributes (ws->display, ws->id, &attr);
                  visual = attr.visual;
                  depth = attr.depth;
                  colormap = attr.colormap;
                } else {
                  visual = ws->visual;
                  depth = ws->screen_depth;
                  colormap = ws->colormap;
                }
                
                pixmap = XCreatePixmap(ws->display, ws->id,
                                       width, height, depth);

                gcv.foreground = 0x00000000;
                gc = XCreateGC(ws->display, pixmap, GCForeground, &gcv);
                XFillRectangle(ws->display, pixmap, gc, 0, 0, w, h);
                XFreeGC(ws->display, gc);

                groupsurface =
                  cairo_xlib_surface_create(ws->display, pixmap,
                                            visual,
                                            (depth == 32)?
                                            CAIRO_FORMAT_ARGB32:
                                            CAIRO_FORMAT_RGB24,
                                            colormap);
            }
        }
        
		if ( cr ) { cairo_destroy(cr); }
        cr = cairo_create(groupsurface);
        cairo_identity_matrix(cr);
        cairo_set_operator(cr, RENDER_OPERATOR_DEFAULT);
        list<RenderOp *>::iterator oit = operations.begin();
        for (; oit != operations.end(); oit++) {
            cairo_save(cr);
            (*oit)->render(dwo, cr, groupsurface, width, height);
            cairo_restore(cr);
        }

        cache_surface = new CacheSurface(ws->display, groupsurface, pixmap,
                                         data, width, height,
                                         &dwo->sb->dynamic_groups);
        
        if (cacheable)
            addToCache(cache_surface, dwo);
    }

    if (crsurface) {
		if ( cr ) { cairo_destroy(cr); }
        cr = cairo_create(crsurface);
        cairo_set_operator(cr, RENDER_OPERATOR_DEFAULT);
        if (xrop_set) cairo_set_operator(cr, xrop);
        cairo_translate(cr, x, y);
        cairo_rotate(cr, rotation);
        // XXX: I think I may've screwed this up, at least as to whether there
        // should be any clipping. -mjmt
        cairo_set_source_surface(cr, cache_surface->crsurface, 0., 0.);
        cairo_rectangle(cr, 0., 0., width, height);
        cairo_clip(cr);
        cairo_paint_with_alpha(cr, opacity);
    }

    if (return_surface)
        *return_surface = cache_surface;
    else
        cache_surface->unref();
}

void RenderGroup::flattenTextOps(list<RenderOpText *> **textopsptr) {
    if (is_a_style) {
        Style *s = (Style *) this;
        if (s->textops) {
            while (! s->textops->empty()) {
                s->textops->back()->unref();
                s->textops->pop_back();
            }
            delete s->textops;
            s->textops = NULL;
        }
    }
    list<RenderOp *>::iterator oit = operations.begin();
    for (; oit != operations.end(); oit++) {
        if ((*oit)->type == RenderOpTextType) {
            if (! *textopsptr) *textopsptr = new list<RenderOpText *>;
            (*textopsptr)->push_back((RenderOpText *) (*oit)->ref());
        } else if ((*oit)->type == RenderOpGroupType) {
            ((RenderGroup *) *oit)->flattenTextOps(textopsptr);
        }
    }
    if (is_a_style) {
        if (((Style *) this)->shapemask)
            ((Style *) this)->shapemask->flattenTextOps(textopsptr);
    }
}

RenderOpDynamic::RenderOpDynamic(void) : RenderOp(RenderOpDynamicType) {}

void RenderOpDynamic::applyAttributes(Parser *parser, Tst<char *> *attr) {
    rotation =  _x = _y = _w = _h = gravity = gravity2 = INT_MAX;
    halign = HNullAlignType;
    valign = VNullAlignType;
    
    opacity = parser->attrGetDouble(attr, "opacity", INT_MAX);
    ((RenderOp *) this)->applyAttributes(parser, attr);
}

void RenderOpDynamic::render(DWindowObject *dwo, cairo_t *cr,
                             cairo_surface_t *surface,
                             unsigned int w, unsigned int h) {
    RenderGroup saved_attributes;
    RenderGroup set_attributes;

    RenderGroup *dynamic = NULL;

    list<int>::iterator it = dynamic_order.begin();
    for (; it != dynamic_order.end(); it++) {
        map<int, RenderGroup *>::iterator dit =
            dwo->sb->dynamic_groups.find(*it);
        if (dit != dwo->sb->dynamic_groups.end()) {
            dynamic = (*dit).second;
            break;
        }
    }
    if (! dynamic) return;

    saved_attributes.inheritAttributes(dynamic);
    set_attributes.inheritAttributes(dynamic);

    if (_x != INT_MAX) {
        set_attributes._x = _x;
        set_attributes._xu = _xu;
    }
    if (_y != INT_MAX) {
        set_attributes._y = _y;
        set_attributes._yu = _yu;
    }
    if (_w != INT_MAX) {
        set_attributes._w = _w;
        set_attributes._wu = _wu;
    }
    if (_h != INT_MAX) {
        set_attributes._h = _h;
        set_attributes._hu = _hu;
    }
    if (x2_set) {
        set_attributes.x2_set = x2_set;
        set_attributes._x2 = _x2;
        set_attributes._x2u = _x2u;
    }
    if (y2_set) {
        set_attributes.y2_set = y2_set;
        set_attributes._y2 = _y2;
        set_attributes._y2u = _y2u;
    }
    if (gravity != INT_MAX)
        set_attributes.gravity = gravity;
    if (gravity2 != INT_MAX)
        set_attributes.gravity2 = gravity2;
    if (xrop_set) {
        set_attributes.xrop_set = xrop_set;
        set_attributes.xrop = xrop;
    }
    if (valign != VNullAlignType)
        set_attributes.valign = valign;
    if (halign != HNullAlignType)
        set_attributes.halign = halign;
    if (rotation != INT_MAX)
        set_attributes.rotation = rotation;
    if (opacity != INT_MAX)
        set_attributes.opacity = opacity;

    dynamic->inheritAttributes(&set_attributes);

    dynamic->render(dwo, cr, surface, w, h);

    dynamic->inheritAttributes(&saved_attributes);
}

PathOperator::PathOperator(PathOperatorType _type) :
    RefCounted<PathOperator>(this) {
    type = _type;
    if (type == PathOperatorCloseType) size = 0;
    else if (type == PathOperatorCurveToType ||
             type == PathOperatorRelCurveToType ||
             type == PathOperatorArcToType ||
             type == PathOperatorRelArcToType) size = 3;
    else size = 1;
    
    if (size) {
        _a = new double[size];
        _b = new double[size];
        _au = new LenghtUnitType[size];
        _bu = new LenghtUnitType[size];
        gravity = new int[size];
        
        for (int i = 0; i < size; i++) {
            _a[i] = _b[i] = 0;
            _au[i] = _bu[i] = PXLenghtUnitType;
            gravity[i] = RENDER_GRAVITY_DEFAULT;
        }
    }
}

PathOperator::~PathOperator(void) {
    if (size) {
        delete [] _a;
        delete [] _b;
        delete [] _au;
        delete [] _bu;
        delete [] gravity;
    }
}

void PathOperator::applyAttributes(Parser *parser, Tst<char *> *attr) {
    switch (type) {
        case PathOperatorArcToType:
        case PathOperatorCurveToType:
            attributes_get_position(parser, attr, "x1", "y1", "gravity1",
                                    &_a[1], &_b[1], &_au[1], &_bu[1],
                                    &gravity[1]);
            attributes_get_position(parser, attr, "x2", "y2", "gravity2",
                                    &_a[2], &_b[2], &_au[2], &_bu[2],
                                    &gravity[2]);
            if (type == PathOperatorArcToType) {
                attributes_get_size(parser, attr, "radius", "radius",
                                    &_a[0], &_b[0], &_au[0], &_bu[0]);
                break;
            }
        case PathOperatorMoveToType:
        case PathOperatorLineToType:
            attributes_get_position_default(parser, attr, &_a[0], &_b[0],
                                            &_au[0], &_bu[0], &gravity[0]);
            break;
        case PathOperatorRelArcToType:
        case PathOperatorRelCurveToType:
            attributes_get_size(parser, attr, "width1", "height1",
                                &_a[1], &_b[1], &_au[1], &_bu[1]);
            attributes_get_size(parser, attr, "width2", "height2",
                                &_a[2], &_b[2], &_au[2], &_bu[2]);
            if (type == PathOperatorRelArcToType) {
                attributes_get_size(parser, attr, "radius", "radius",
                                    &_a[0], &_b[0], &_au[0], &_bu[0]);
                break;
            }
        case PathOperatorRelLineToType:
        case PathOperatorRelMoveToType:
            attributes_get_size_default(parser, attr, &_a[0], &_b[0],
                                        &_au[0], &_bu[0]);
            break;
            
    }
}

WaColorStop::WaColorStop(double _offset, WaColor *_color)
    : RefCounted<WaColorStop>(this) {
    color = _color;
    offset = _offset;
}

WaColorStop::~WaColorStop(void) {
    color->unref();
}

static const struct FilterMap {
    char *name;
    cairo_filter_t filter;
} filter_map[] = {
    { "fast", CAIRO_FILTER_FAST },
    { "good", CAIRO_FILTER_GOOD },
    { "best", CAIRO_FILTER_BEST },
    { "nearest", CAIRO_FILTER_NEAREST },
    { "bilinear", CAIRO_FILTER_BILINEAR },
    { "gaussian", CAIRO_FILTER_GAUSSIAN }
};

static const struct ExtendMap {
    char *name;
    cairo_extend_t extend;
} extend_map[] = {
    { "none", CAIRO_EXTEND_NONE },
    { "repeat", CAIRO_EXTEND_REPEAT },
    { "reflect", CAIRO_EXTEND_REFLECT }
};

static const struct PatternTypeMap {
    char *name;
    PatternType pattern_type;
} pattern_type_map[] = {
    { "linear", LinearPatternType },
    { "radial", RadialPatternType },
    { "group", GroupPatternType }
};

RenderPattern::RenderPattern(void) : RefCounted<RenderPattern>(this) {
    filter = RENDER_FILTER_DEFAULT;
    extend = RENDER_EXTEND_DEFAULT;

    type = LinearPatternType;

    start_gravity = end_gravity = RENDER_GRAVITY_DEFAULT;
    start_x = start_y = 0;
    end_x = end_y = 100;
    start_x_u = start_y_u = PXLenghtUnitType;
    end_x_u = end_y_u = PCTLenghtUnitType;

    center_gravity = RENDER_GRAVITY_DEFAULT;
    center_x = center_y = 50;
    radius_dx = radius_dy = 50;
    center_x_u = center_y_u = radius_dx_u = radius_dy_u = PCTLenghtUnitType;

    group = NULL;
}

RenderPattern::~RenderPattern(void) {
    clear();
    if (group) group->unref();
}

void RenderPattern::clear(void) {
    while (! color_stops.empty()) {
        color_stops.back()->unref();
        color_stops.pop_back();
    }
}

void RenderPattern::inheritContent(RenderPattern *inherit_pattern) {
    list<WaColorStop *>::iterator it = inherit_pattern->color_stops.begin();
    for (; it != inherit_pattern->color_stops.end(); it++)
        color_stops.push_back((*it)->ref());
}

void RenderPattern::inheritAttributes(RenderPattern *inherit) {
    filter = inherit->filter;
    extend = inherit->extend;
    type = inherit->type;
    start_x = inherit->start_x;
    start_y = inherit->start_y;
    start_gravity = inherit->start_gravity;
    end_x = inherit->end_x;
    end_y = inherit->end_y;
    end_gravity = inherit->end_gravity;
    start_x_u = inherit->start_x_u;
    start_y_u = inherit->start_y_u;
    end_x_u = inherit->end_x_u;
    end_y_u = inherit->end_y_u;
    center_x = inherit->center_x;
    center_y = inherit->center_y;
    center_gravity = inherit->center_gravity;
    radius_dx = inherit->radius_dx;
    radius_dy = inherit->radius_dy;
    center_x_u = inherit->center_x_u;
    center_y_u = inherit->center_y_u;
    radius_dx_u = inherit->radius_dx_u;
    radius_dy_u = inherit->radius_dy_u;
    if (group) group->unref();
    group = NULL;
    if (inherit->group)
        group = (RenderGroup *) inherit->group->ref();
}

void RenderPattern::applyAttributes(Parser *parser, Tst<char *> *attr) {
    char *value = parser->attrGetString(attr, "inherit", NULL);
    if (value) {
        RenderPattern *inherit_pattern =
            parser->ws->getPatternNamed(value, false);
        if (inherit_pattern == this) {
            parser->warning("pattern %s cannot inherit itself\n", value);
        } else if (inherit_pattern) {
            inheritAttributes(inherit_pattern);
        } else {
            if (! parser->attrGetBool(attr, "ignore_missing", false))
                parser->warning("undefined pattern: %s\n", value);
        }
    }

    value = parser->attrGetString(attr, "type", NULL);
    if (value) {
        Tst<PatternType>::iterator it = pattern_type_tst->find(value);
        if (it != pattern_type_tst->end()) type = *it;
    }

    attributes_get_position(parser, attr, "start_x", "start_y",
                            "start_gravity",
                            &start_x, &start_y, &start_x_u, &start_y_u,
                            &start_gravity);
    attributes_get_position(parser, attr, "end_x", "end_y",
                            "end_gravity",
                            &end_x, &end_y, &end_x_u, &end_y_u,
                            &end_gravity);

    attributes_get_position(parser, attr, "center_x", "center_y",
                            "center_gravity",
                            &center_x, &center_y, &center_x_u, &center_y_u,
                            &center_gravity);
    attributes_get_size(parser, attr, "radius_dx", "radius_dy",
                        &radius_dx, &radius_dy, &radius_dx_u, &radius_dy_u);
    
    value = parser->attrGetString(attr, "filter", NULL);
    if (value) {
        Tst<cairo_filter_t>::iterator it = filter_tst->find(value);
        if (it != filter_tst->end()) filter = *it;
    }

    value = parser->attrGetString(attr, "spread", NULL);
    if (value) {
        Tst<cairo_extend_t>::iterator it = extend_tst->find(value);
        if (it != extend_tst->end()) extend = *it;
    }

    value = parser->attrGetString(attr, "group", NULL);
    if (value) {
        group = parser->ws->getRenderGroupNamed(value, false);
        if (! group) {
            parser->warning("undefined group: %s\n", value);
        } else
            group = (RenderGroup *) group->ref();
    }
}

void RenderPattern::setcairo_pattern(DWindowObject *dwo,
                                     cairo_t *cr,
                                     cairo_surface_t *crsurface,
                                     unsigned int w, unsigned int h) {
  cairo_pattern_t *pattern;

  switch (type) {
    case LinearPatternType: {
      double x0, y0, x1, y1;

      calc_position(start_gravity, start_x_u, start_y_u,
                    w, h, dwo->ws->hdpi, dwo->ws->vdpi,
                    start_x, start_y, &x0, &y0);
      calc_position(end_gravity, end_x_u, end_y_u,
                    w, h, dwo->ws->hdpi, dwo->ws->vdpi,
                    end_x, end_y, &x1, &y1);
      
      pattern = cairo_pattern_create_linear (x0, y0, x1, y1);
    } break;
    case RadialPatternType: {
      double cx, cy, rx, ry;

      calc_position(center_gravity, center_x_u, center_y_u,
                    w, h, dwo->ws->hdpi, dwo->ws->vdpi,
                    center_x, center_y, &cx, &cy);
      
      calc_length(radius_dx, radius_dx_u, dwo->ws->hdpi, w, &rx);
      calc_length(radius_dy, radius_dy_u, dwo->ws->vdpi, h, &ry);

      if (rx != ry)
        cx *= (ry / rx);

      pattern = cairo_pattern_create_radial (cx, cy, 0.0, cx, cy, ry);

      if (rx != ry) {
        cairo_matrix_t matrix;
        cairo_matrix_init_scale (&matrix, ry / rx, 1.0);
        cairo_pattern_set_matrix (pattern, &matrix);
      }
    } break;
    case GroupPatternType: {
      if (group) {
        cairo_save(cr);
        CacheSurface *subsurface = NULL;
        
        group->parent_surface = crsurface;
        group->return_surface = &subsurface;
        cairo_save(cr);
        group->render(dwo, cr, NULL, w, h);
        cairo_restore(cr);
        group->return_surface = NULL;
        group->parent_surface = NULL;

        cairo_identity_matrix(cr);
        cairo_move_to(cr, 0.0, 0.0);
        pattern = cairo_pattern_create_for_surface(subsurface->crsurface);
        subsurface->unref();
        cairo_restore(cr);
      } else {
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        return;
      }
    } break;
  }

  int stops = 0;
  list<WaColorStop *>::iterator it = color_stops.begin();
  for (; it != color_stops.end(); ++it) {
    cairo_pattern_add_color_stop_rgba (pattern, (*it)->offset,
                                  (*it)->color->red,
                                  (*it)->color->green,
                                  (*it)->color->blue,
                                  (*it)->color->alpha);
    stops++;
  }

  if (stops == 0) {
    cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0.0, 0.0, 0.0, 1.0);
    cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1.0, 1.0, 1.0, 1.0);
  }
      
  cairo_pattern_set_filter (pattern, filter);
  cairo_pattern_set_extend (pattern, extend);

  cairo_set_source (cr, pattern);

  cairo_pattern_destroy (pattern);
}

static const struct LineCapMap {
    char *name;
    cairo_line_cap_t line_cap;
} line_cap_map[] = {
    { "butt", CAIRO_LINE_CAP_BUTT },
    { "round", CAIRO_LINE_CAP_ROUND },
    { "square", CAIRO_LINE_CAP_SQUARE }
};

static const struct LineJoinMap {
    char *name;
    cairo_line_join_t line_join;
} line_join_map[] = {
    { "miter", CAIRO_LINE_JOIN_MITER },
    { "round", CAIRO_LINE_JOIN_ROUND },
    { "bevel", CAIRO_LINE_JOIN_BEVEL }
};

static const struct FillRuleMap {
    char *name;
    cairo_fill_rule_t fill_rule;
} fill_rule_map[] = {
    { "winding", CAIRO_FILL_RULE_WINDING },
    { "evenodd", CAIRO_FILL_RULE_EVEN_ODD }
};

static const struct DrawingOrderMap {
    char *name;
    DrawingOrder order;
} drawing_order_map[] = {
    { "fillstroke", FillStrokeDrawingOrder },
    { "strokefill", StrokeFillDrawingOrder }
};

RenderOpDraw::RenderOpDraw(RenderOpType _type) : RenderOp(_type) {
    linewidth = RENDER_LINE_WIDTH_DEFAULT;
    linewidth_u = PXLenghtUnitType;
    linejoin = RENDER_LINE_JOIN_DEFAULT;
    linecap = RENDER_LINE_CAP_DEFAULT;
    fillrule = RENDER_FILL_RULE_DEFAULT;
    tolerance = RENDER_TOLERANCE_DEFAULT;
    miterlimit = RENDER_MITER_LIMIT_DEFAULT;
    stroke_pattern = fill_pattern = NULL;
    dashes = NULL;
    dashes_u = NULL;
    dash_offset = 0.0;
    dash_offset_u = PXLenghtUnitType;
    ndash = 0;
    order = RENDER_DRAWING_ORDER_DEFAULT;
    fill = stroke = false;
}

RenderOpDraw::~RenderOpDraw(void) {
    if (fill_pattern) fill_pattern->unref();
    if (stroke_pattern) stroke_pattern->unref();
    if (dashes) delete [] dashes;
    if (dashes_u) delete [] dashes_u;
}

void RenderOpDraw::inheritAttributes(RenderOpDraw *inherit) {
    linewidth = inherit->linewidth;
    linewidth_u = inherit->linewidth_u;
    linejoin = inherit->linejoin;
    linecap = inherit->linecap;
    fillrule = inherit->fillrule;
    tolerance = inherit->tolerance;
    miterlimit = inherit->miterlimit;
    stroke_color = inherit->stroke_color;
    fill_color = inherit->fill_color;
    fill = inherit->fill;
    stroke = inherit->stroke;

    if (fill_pattern) fill_pattern->unref();
    fill_pattern = NULL;
    if (inherit->fill_pattern)
        fill_pattern = inherit->fill_pattern->ref();

    if (stroke_pattern) stroke_pattern->unref();
    stroke_pattern = NULL;
    if (inherit->stroke_pattern)
        stroke_pattern = inherit->stroke_pattern->ref();

    if (dashes) delete [] dashes;
    if (dashes_u) delete [] dashes_u;

    dashes = NULL;
    dashes_u = NULL;
    ndash = inherit->ndash;
    
    if (ndash) {
        dashes = new double[ndash];
        dashes_u = new LenghtUnitType[ndash];
        
        for (int i = 0; i < ndash; i++) {
            dashes[i] = inherit->dashes[i];
            dashes_u[i] = inherit->dashes_u[i];
        }
    }
    dash_offset = inherit->dash_offset;
    dash_offset_u = inherit->dash_offset_u;

    ((RenderOp *) this)->inheritAttributes((RenderOp *) inherit);
}

void RenderOpDraw::applyAttributes(Parser *parser, Tst<char *> *attr) {
    char *value;
    ((RenderOp *) this)->applyAttributes(parser, attr);
    linewidth = parser->attrGetLength(attr, "linewidth", linewidth,
                                      &linewidth_u);
    tolerance = parser->attrGetDouble(attr, "tolerance", tolerance);
    miterlimit = parser->attrGetDouble(attr, "miterlimit", miterlimit);
    value = parser->attrGetString(attr, "linecap", NULL);
    if (value) {
        Tst<cairo_line_cap_t>::iterator it = line_cap_tst->find(value);
        if (it != line_cap_tst->end()) linecap = *it;
    }
    value = parser->attrGetString(attr, "linejoin", NULL);
    if (value) {
        Tst<cairo_line_join_t>::iterator it = line_join_tst->find(value);
        if (it != line_join_tst->end()) linejoin = *it;
    }
    value = parser->attrGetString(attr, "fillrule", NULL);
    if (value) {
        Tst<cairo_fill_rule_t>::iterator it = fill_rule_tst->find(value);
        if (it != fill_rule_tst->end()) fillrule = *it;
    }
    value = parser->attrGetString(attr, "fill_pattern", NULL);
    if (value) {
        fill_pattern = parser->ws->getPatternNamed(value, false);
        if (! fill_pattern) {
            parser->warning("undefined pattern: %s\n", value);
        } else {
            fill = true;
            fill_pattern = (RenderPattern *) fill_pattern->ref();
        }
    }
    value = parser->attrGetString(attr, "stroke_color", NULL);
    if (value) {
        stroke = true;
        if (! stroke_color.parseColor(parser->ws, value))
            parser->warning("error parsing color: %s\n", value);
    }
    stroke_color.setOpacity(
      parser->attrGetDouble(attr, "stroke_opacity",
                            stroke_color.getOpacity()));

    value = parser->attrGetString(attr, "stroke_pattern", NULL);
    if (value) {
        stroke_pattern = parser->ws->getPatternNamed(value, false);
        if (! stroke_pattern) {
            parser->warning("undefined pattern: %s\n", value);
        } else {
            stroke = true;
            stroke_pattern = (RenderPattern *) stroke_pattern->ref();
        }
    }
    value = parser->attrGetString(attr, "fill_color", NULL);
    if (value) {
      fill = true;
      if (! fill_color.parseColor(parser->ws, value))
        parser->warning("error parsing color: %s\n", value);
    }
    
    fill_color.setOpacity(
      parser->attrGetDouble(attr, "fill_opacity",
                            fill_color.getOpacity()));

    value = parser->attrGetString(attr, "stroke_dasharray", NULL);
    if (value) {
        value = WA_STRDUP(value);
        ndash = 1;
        for (int i = 0; value[i] != '\0'; i++) {
            if (value[i] == ',') {
                ndash++;
                value[i] = '\0';
            }
        }

        dashes = new double[ndash];
        dashes_u = new LenghtUnitType[ndash];

        char *dash = value;
        for (int i = 0; i < ndash; i++) {
            dashes[i] = get_double_and_unit(dash, &dashes_u[i]);
            dash += (strlen(dash) + 1);
        }
        
        delete [] value;

        dash_offset = parser->attrGetLength(attr, "stroke_dashoffset",
                                            dash_offset, &dash_offset_u);
    }

    value = parser->attrGetString(attr, "drawing_order", NULL);
    if (value) {
        Tst<DrawingOrder>::iterator it = drawing_order_tst->find(value);
        if (it != drawing_order_tst->end()) order = *it;
    }
}

void RenderOpDraw::draw(DWindowObject *dwo, cairo_t *cr,
                        cairo_surface_t *crsurface,
                        unsigned int w, unsigned int h) {
    cairo_pattern_t *pattern;
    
    if (xrop_set) cairo_set_operator(cr, xrop);
    cairo_rotate(cr, rotation);
    cairo_set_tolerance(cr, tolerance);

    for (int i = 0; i < 2; i++) {
      
      if (order == FillStrokeDrawingOrder && i == 0 ||
          order == StrokeFillDrawingOrder && i == 1) {

        if (fill) {
          cairo_save(cr);
          cairo_set_fill_rule(cr, fillrule);
          fill_color.setcairo_color(cr);
          if (fill_pattern)
            fill_pattern->setcairo_pattern(dwo, cr, NULL, w, h);
          //cairo_set_alpha(cr, fill_color.alpha);
          cairo_set_source_rgba(cr,
                                fill_color.red,
                                fill_color.green,
                                fill_color.blue,
                                fill_color.alpha);
          cairo_fill(cr);
          cairo_restore(cr);
        }
      } else {
        if (stroke) {
          double lwidth;
          calc_length(linewidth, linewidth_u,
                      (dwo->ws->hdpi + dwo->ws->vdpi) / 2, w, &lwidth);
          cairo_set_line_width(cr, lwidth);
          cairo_set_line_join(cr, linejoin);
          cairo_set_line_cap(cr, linecap);
          cairo_set_miter_limit(cr, miterlimit);
          if (dashes) {
            double *pxdashes = new double[ndash];
            double pxdash_offset;
            
            calc_length(dash_offset, dash_offset_u,
                        (dwo->ws->hdpi + dwo->ws->vdpi) / 2, w,
                        &pxdash_offset);
            
            for (int i = 0; i < ndash; i++)
              calc_length(dashes[i], dashes_u[i],
                          (dwo->ws->hdpi + dwo->ws->vdpi) / 2, w,
                          &pxdashes[i]);
            cairo_set_dash(cr, pxdashes, ndash, pxdash_offset);
            delete [] pxdashes;
          }

          cairo_save(cr);
          stroke_color.setcairo_color(cr);
          if (stroke_pattern)
            stroke_pattern->setcairo_pattern(dwo, cr, NULL, w, h);
          cairo_stroke(cr);
          cairo_restore(cr);
        }
      }
    }
}

RenderOpPath::RenderOpPath(char *_name) : RenderOpDraw(RenderOpPathType) {
    if (_name) name = WA_STRDUP(_name);
    else name = NULL;
}

RenderOpPath::~RenderOpPath(void) {
    clear();
}

void RenderOpPath::clear(void) {
    while (! ops.empty()) {
        ops.back()->unref();
        ops.pop_back();
    }
}

void RenderOpPath::inheritContent(RenderOpPath *inherit_path) {
    list<PathOperator *>::iterator it = inherit_path->ops.begin();
    for (; it != inherit_path->ops.end(); it++)
        ops.push_back((*it)->ref());
}

void RenderOpPath::inheritAttributes(RenderOpPath *inherit) {
    ((RenderOpDraw *) this)->inheritAttributes((RenderOpDraw *) inherit);
}

void RenderOpPath::applyAttributes(Parser *parser, Tst<char *> *attr) {
    char *value = parser->attrGetString(attr, "inherit", NULL);
    if (value) {
        RenderOpPath *inherit_path = parser->ws->getPathNamed(value, false);
        if (inherit_path == this) {
            parser->warning("path %s cannot inherit itself\n", name);
        } else if (inherit_path) {
            inheritAttributes(inherit_path);
        } else {
            if (! parser->attrGetBool(attr, "ignore_missing", false))
                parser->warning("undefined path: %s\n", value);
        }
    }
    
    ((RenderOpDraw *) this)->applyAttributes(parser, attr);
}

void RenderOpPath::render(DWindowObject *dwo, cairo_t *cr,
                          cairo_surface_t *crsurface,
                          unsigned int w, unsigned int h) {
    double a[3], b[3], x, y;
    double width, height;
    double vd = dwo->ws->vdpi;
    double hd = dwo->ws->hdpi;

    if (ops.empty()) return;

    calcPositionAndSize(w, h, hd, vd, &x, &y, &width, &height);

    cairo_translate(cr, x, y);
    cairo_new_path(cr);
    list<PathOperator *>::iterator it = ops.begin();
    for (; it != ops.end(); it++) {
        switch ((*it)->type) {
            case PathOperatorMoveToType:
                (*it)->calcPosition(width, height, hd, vd, a, b);
                cairo_move_to(cr, *a, *b);
                break;
            case PathOperatorRelMoveToType:
                (*it)->calcSize(width, height, hd, vd, a, b);
                cairo_rel_move_to(cr, *a, *b);
                break;
            case PathOperatorLineToType:
                (*it)->calcPosition(width, height, hd, vd, a, b);
                cairo_line_to(cr, *a, *b);
                break;
            case PathOperatorRelLineToType:
                (*it)->calcSize(width, height, hd, vd, a, b);
                cairo_rel_line_to(cr, *a, *b);
                break;
            case PathOperatorCurveToType:
                (*it)->calcPosition(width, height, hd, vd, a, b);
                cairo_curve_to(cr, a[1], b[1], a[2], b[2], a[0], b[0]);
                break;
            case PathOperatorRelCurveToType:
                (*it)->calcSize(width, height, hd, vd, a, b);
                cairo_rel_curve_to(cr, a[1], b[1], a[2], b[2], a[0], b[0]);
                break;
            case PathOperatorArcToType:
                (*it)->calcPosition(width, height, hd, vd, a, b);
                /* NYI: not yet implemented in cairo
                   cairo_arc_to(cr, a[1], b[1], a[2], b[2], a[0]);
                */
                break;
            case PathOperatorRelArcToType:
                (*it)->calcSize(width, height, hd, vd, a, b);
                /* NYI: not yet implemented in cairo
                   cairo_rel_arc_to(cr, a[1], b[1], a[2], b[2], a[0]);
                */
                break;
            case PathOperatorCloseType:
                cairo_close_path(cr);
                break;
        }
    }

    cairo_move_to (cr, 0.0, 0.0);
    draw(dwo, cr, crsurface, WA_ROUND_U(width),
         WA_ROUND_U(height));
}

void RenderOpLine::applyAttributes(Parser *parser, Tst<char *> *attr) {
    ((RenderOpDraw *) this)->applyAttributes(parser, attr);
}

void RenderOpLine::render(DWindowObject *dwo, cairo_t *cr,
                          cairo_surface_t *crsurface,
                          unsigned int w, unsigned int h) {
    double x, y, width, height;

    calcPositionAndSize(w, h, dwo->ws->hdpi, dwo->ws->vdpi, &x, &y,
                        &width, &height);

    cairo_new_path(cr);
    cairo_move_to(cr, x, y);
    cairo_rel_line_to(cr, width, height);
    cairo_move_to (cr, 0.0, 0.0);
    draw(dwo, cr, crsurface, WA_ROUND_U(width), WA_ROUND_U(height));
}

void RenderOpRectangle::applyAttributes(Parser *parser,
                                        Tst<char *> *attr) {
    ((RenderOpDraw *) this)->applyAttributes(parser, attr);
    
    nw_rx = parser->attrGetLength(attr, "rx", RENDER_DEFAULT_RECT_RX,
                                  &nw_rx_u);
    ne_rx = se_rx = sw_rx = nw_rx;
    ne_rx_u = se_rx_u = sw_rx_u = nw_rx_u;
    nw_ry = parser->attrGetLength(attr, "ry", RENDER_DEFAULT_RECT_RY,
                                  &nw_ry_u);
    ne_ry = se_ry = sw_ry = nw_ry;
    ne_ry_u = se_ry_u = sw_ry_u = nw_ry_u;
    
    nw_rx = parser->attrGetLength(attr, "northwest_rx", nw_rx, &nw_rx_u);
    nw_ry = parser->attrGetLength(attr, "northwest_ry", nw_ry, &nw_ry_u);
    
    ne_rx = parser->attrGetLength(attr, "northeast_rx", ne_rx, &ne_rx_u);
    ne_ry = parser->attrGetLength(attr, "northeast_ry", ne_ry, &ne_ry_u);

    sw_rx = parser->attrGetLength(attr, "southwest_rx", sw_rx, &sw_rx_u);
    sw_ry = parser->attrGetLength(attr, "southwest_ry", sw_ry, &sw_ry_u);

    se_rx = parser->attrGetLength(attr, "southeast_rx", se_rx, &se_rx_u);
    se_ry = parser->attrGetLength(attr, "southeast_ry", se_ry, &se_ry_u);
}

#define M_KAPPA 0.5522847498

void RenderOpRectangle::render(DWindowObject *dwo, cairo_t *cr,
                               cairo_surface_t *crsurface,
                               unsigned int w, unsigned int h) {
    double x, y, width, height;
    double vd = dwo->ws->vdpi;
    double hd = dwo->ws->hdpi;

    calcPositionAndSize(w, h, dwo->ws->hdpi, dwo->ws->vdpi, &x, &y,
                        &width, &height);

    if (nw_rx && nw_ry || ne_rx && ne_ry || sw_rx && sw_ry || se_rx && se_ry) {
        double rx, ry, go_w, go_h;
        cairo_new_path(cr);
        cairo_move_to(cr, x, y);

        go_w = width;
            
        if (nw_rx && nw_ry) {
            calc_length(nw_rx, nw_rx_u, hd, width, &rx);
            calc_length(nw_ry, nw_ry_u, vd, height, &ry);

            cairo_rel_move_to(cr, 0.0, ry);
            cairo_rel_curve_to(cr, 0.0, -ry * M_KAPPA,
                               rx - rx * M_KAPPA, -ry,
                               rx, -ry);
            go_w -= rx;
        }

        go_h = height;
        
        if (ne_rx && ne_ry) {
            calc_length(ne_rx, ne_rx_u, hd, width, &rx);
            calc_length(ne_ry, ne_ry_u, vd, height, &ry);
            
            cairo_rel_line_to(cr, go_w - rx, 0.0);
            cairo_rel_curve_to(cr, rx * M_KAPPA, 0.0,
                               rx, ry - ry * M_KAPPA,
                               rx, ry);
            go_h -= ry;
        } else
            cairo_rel_line_to(cr, go_w, 0.0);

        go_w = width;

        if (se_rx && se_ry) {
            calc_length(se_rx, se_rx_u, hd, width, &rx);
            calc_length(se_ry, se_ry_u, vd, height, &ry);
            
            cairo_rel_line_to(cr, 0.0, go_h - ry);
            cairo_rel_curve_to(cr, 0.0, ry * M_KAPPA,
                               -rx + rx * M_KAPPA, ry,
                               -rx, ry);
            go_w -= rx;
        } else
            cairo_rel_line_to(cr, 0.0, go_h);

        if (sw_rx && sw_ry) {
            calc_length(sw_rx, sw_rx_u, hd, width, &rx);
            calc_length(sw_ry, sw_ry_u, vd, height, &ry);
            
            cairo_rel_line_to(cr, -(go_w - rx), 0.0);
            cairo_rel_curve_to(cr, -rx * M_KAPPA, 0.0,
                               -rx, -ry + ry * M_KAPPA,
                               -rx, -ry);
        } else
            cairo_rel_line_to(cr, -go_w, 0.0);

        cairo_close_path(cr);
        
    } else
        cairo_rectangle(cr, x, y, width, height);

    cairo_translate(cr, x, y);
    cairo_move_to (cr, 0.0, 0.0);
    draw(dwo, cr, crsurface, WA_ROUND_U(width),
         WA_ROUND_U(height));
}

static const struct WeightMap {
    char *name;
    cairo_font_weight_t weight;
} weight_map[] = {
    { "normal", CAIRO_FONT_WEIGHT_NORMAL },
    { "bold", CAIRO_FONT_WEIGHT_BOLD }
};

static const struct SlantMap {
    char *name;
    cairo_font_slant_t slant;
} slant_map[] = {
    { "normal", CAIRO_FONT_SLANT_NORMAL },
    { "italic", CAIRO_FONT_SLANT_ITALIC },
    { "oblique", CAIRO_FONT_SLANT_OBLIQUE }
};

RenderOpText::RenderOpText(char *_name) : RenderOpDraw(RenderOpTextType) {
    shadow_x_offset = shadow_y_offset = 0.0;
    size = RENDER_FONT_SIZE_DEFAULT;
    size_u = _wu = _hu = PXLenghtUnitType;
    shadow = dynamic_area_width = dynamic_area_height = false;
    _w = _h = 0.0;
    line_spacing = RENDER_LINE_SPACING_DEFAULT;
    line_spacing_u = PXLenghtUnitType;
    tab_spacing = RENDER_TAB_SPACE_DEFAULT;
    tab_spacing_u = PXLenghtUnitType;
    left_spacing = right_spacing = top_spacing = bottom_spacing = 0.0;
    left_spacing_u = right_spacing_u = top_spacing_u =
        bottom_spacing_u = PXLenghtUnitType;
    text_halign = LeftAlignType;
    utf8 = NULL;
    is_static = false;
    family = WA_STRDUP(RENDER_FONT_KEY_DEFAULT);
    font = NULL;
    slant = RENDER_FONT_SLANT_DEFAULT;
    weight = RENDER_FONT_WEIGHT_DEFAULT;
    bg_group = NULL;
    if (_name) name = WA_STRDUP(_name);
    else name = NULL;
}

RenderOpText::~RenderOpText(void) {
    delete [] family;
    if (name) delete [] name;
    if (bg_group) bg_group->unref();
    clear();
    if (font) cairo_font_face_destroy(font);
}

void RenderOpText::clear(void) {
    if (utf8) delete [] utf8;
    utf8 = NULL;
}

void RenderOpText::inheritAttributes(RenderOpText *inherit) {
    shadow_x_offset = inherit->shadow_x_offset;
    shadow_y_offset = inherit->shadow_y_offset;
    size = inherit->size;
    size_u = inherit->size_u;
    _wu = inherit->_wu;
    _hu = inherit->_hu;
    shadow = inherit->shadow;
    dynamic_area_width = inherit->dynamic_area_width;
    dynamic_area_height = inherit->dynamic_area_height;
    _w = inherit->_w;
    _h = inherit->_h;
    line_spacing = inherit->line_spacing;
    line_spacing_u = inherit->line_spacing_u;
    tab_spacing = inherit->tab_spacing;
    tab_spacing_u = inherit->tab_spacing_u;
    left_spacing = inherit->left_spacing;
    right_spacing = inherit->right_spacing;
    top_spacing = inherit->top_spacing;
    bottom_spacing = inherit->bottom_spacing;
    left_spacing_u = inherit->left_spacing_u;
    right_spacing_u = inherit->right_spacing_u;
    top_spacing_u = inherit->top_spacing_u;
    bottom_spacing_u = inherit->bottom_spacing_u;
    text_halign = inherit->text_halign;
    family = WA_STRDUP((char *) inherit->family);
    slant = inherit->slant;
    weight = inherit->weight;
    if (bg_group) bg_group->unref();
    bg_group = NULL;
    if (inherit->bg_group)
        bg_group = (RenderGroup *) inherit->bg_group->ref();

    ((RenderOpDraw *) this)->inheritAttributes((RenderOpDraw *) inherit);
}

void RenderOpText::applyAttributes(Parser *parser, Tst<char *> *attr) {
    char *value = parser->attrGetString(attr, "inherit", NULL);
    if (value) {
        RenderOpText *inherit_text = parser->ws->getTextNamed(value, false);
        if (inherit_text == this) {
            parser->warning("text object %s cannot inherit itself\n", name);
        } else if (inherit_text) {
            inheritAttributes(inherit_text);
        } else {
            if (! parser->attrGetBool(attr, "ignore_missing", false))
                parser->warning("undefined text object: %s\n", value);
        }
    }
    
    attributes_get_alignment(parser, attr, "text_halign", NULL,
                             &text_halign, NULL);

    value = parser->attrGetString(attr, "bg_group", NULL);
    if (value) {
        bg_group = parser->ws->getRenderGroupNamed(value, false);
        if (! bg_group) {
            parser->warning("undefined group: %s\n", value);
        } else
            bg_group = (RenderGroup *) bg_group->ref();
    }

    is_static = parser->attrGetBool(attr, "static", is_static);

    dynamic_area_width = parser->attrGetBool(attr, "dynamic_area_width",
                                             dynamic_area_width);
    dynamic_area_height = parser->attrGetBool(attr, "dynamic_area_height",
                                              dynamic_area_height);
    
    size = parser->attrGetLength(attr, "size", size, &size_u);
    tab_spacing = parser->attrGetLength(attr, "tab_spacing", tab_spacing,
                                        &tab_spacing_u);
    line_spacing = parser->attrGetLength(attr, "line_spacing", line_spacing,
                                         &line_spacing_u);
    left_spacing = parser->attrGetLength(attr, "left_spacing", left_spacing,
                                         &left_spacing_u);
    right_spacing = parser->attrGetLength(attr, "right_spacing", right_spacing,
                                          &right_spacing_u);
    top_spacing = parser->attrGetLength(attr, "top_spacing", top_spacing,
                                        &top_spacing_u);
    bottom_spacing = parser->attrGetLength(attr, "bottom_spacing",
                                           bottom_spacing, &bottom_spacing_u);
    
    shadow = parser->attrGetBool(attr, "shadow", shadow);
    if (shadow) {
        shadow_x_offset = parser->attrGetDouble(attr, "shadow_hoffset",
                                                shadow_x_offset);
        shadow_y_offset = parser->attrGetDouble(attr, "shadow_voffset",
                                                shadow_y_offset);
    
        value = parser->attrGetString(attr, "shadow_color", NULL);
        if (value) {
            if (! shadow_color.parseColor(parser->ws, value))
                parser->warning("error parsing color: %s\n", value);
        }
        shadow_color.setOpacity(
            parser->attrGetDouble(attr, "shadow_opacity",
                                  shadow_color.getOpacity()));
    }

    value = parser->attrGetString(attr, "weight", NULL);
    if (value) {
        Tst<cairo_font_weight_t>::iterator it = weight_tst->find(value);
        if (it != weight_tst->end()) weight = *it;
    }

    value = parser->attrGetString(attr, "slant", NULL);
    if (value) {
        Tst<cairo_font_slant_t>::iterator it = slant_tst->find(value);
        if (it != slant_tst->end()) slant = *it;
    }
    
    value = parser->attrGetString(attr, "font", NULL);
    if (value) {
        delete [] family;
        family = WA_STRDUP(value);
    }
    ((RenderOpDraw *) this)->applyAttributes(parser, attr);
}

void RenderOpText::calcLinePosition(double width,
                                    unsigned int region_width,
                                    unsigned int region_height,
                                    double *return_x) {
    int textgravity = NorthWestGravity;
    switch (text_halign) {
        case RightAlignType:
            textgravity = NorthEastGravity;
            break;
        case HCenterAlignType:
            textgravity = CenterGravity;
            break;
        default:
            break;
    }
    double d;
    calc_position_and_size(text_halign, TopAlignType, textgravity,
                           PXLenghtUnitType, PXLenghtUnitType,
                           PXLenghtUnitType, PXLenghtUnitType,
                           region_width, region_height, 1.0, 1.0,
                           WA_ROUND_U(width), 1.0,
                           0.0, 0.0, return_x, &d, NULL, NULL);
    if (*return_x < 0.0) *return_x = 0.0;
}

void RenderOpText::render(DWindowObject *dwo, cairo_t *cr,
                          cairo_surface_t *crsurface,
                          unsigned int w, unsigned int h) {
    char *text;
    double x, y, width = 0.0, height = 0.0, font_size, tab_space, line_space,
        left_space, right_space, top_space, bottom_space, stroke_size;
    LenghtUnitType width_u = PXLenghtUnitType, height_u = PXLenghtUnitType;
    list<RenderLineInfo *> lines;
    CacheSurface *subsurface = NULL;
    
    if (! utf8) return;

    calc_length(size, size_u, dwo->ws->hdpi, w, &font_size);
    calc_length(tab_spacing, tab_spacing_u, dwo->ws->hdpi, w, &tab_space);
    calc_length(line_spacing, line_spacing_u, dwo->ws->vdpi, h, &line_space);
    calc_length(left_spacing, left_spacing_u, dwo->ws->hdpi, w, &left_space);
    calc_length(right_spacing, right_spacing_u, dwo->ws->hdpi, w,
                &right_space);
    calc_length(top_spacing, top_spacing_u, dwo->ws->vdpi, h, &top_space);
    calc_length(bottom_spacing, bottom_spacing_u, dwo->ws->vdpi, h,
                &bottom_space);

    cairo_identity_matrix(cr);

    if (!font) {
        cairo_select_font_face (cr, family, slant, weight);
        /* font = cairo_current_font(cr);
        if (!font)
          return;
        
        cairo_font_reference (font);
        */
    } else
        cairo_set_font_face (cr, font);
    
    cairo_set_font_size (cr, font_size);    
    
    if (is_static)
        text = utf8;
    else {
        map<RenderOpText *, char *>::iterator it =
            dwo->sb->text_table->find(this);
        if (it != dwo->sb->text_table->end())
            text = (*it).second;
        else
            text = utf8;
    }

    double space_width = -1.0;
    double line_height = 0.0, y_pos = 0.0, y_off;
    char *textptr = text;
    unsigned int end_offset = 0;
    unsigned int linenr = 0;
    cairo_surface_t *text_region = NULL;
    bool end_of_lines = false;
    do {        
        RenderLineInfo *linfo = new RenderLineInfo;
        linfo->width = 0.0;
        linenr++;
        
        do {
            cairo_text_extents_t extents;
            char *str;
            double str_width, str_height, str_y_pos;
            RenderStringInfo *sinfo = new RenderStringInfo;
                
            textptr += end_offset;
            for (end_offset = 0; textptr[end_offset] != '\n' &&
                     textptr[end_offset] != '\t' &&
                     textptr[end_offset] != '\0'; end_offset++);
            str = new char[end_offset + 1];
            strncpy(str, textptr, end_offset);
            str[end_offset] = '\0';
            sinfo->text = str;
            
            cairo_text_extents(cr, sinfo->text, &extents);
            str_y_pos = extents.height - extents.y_bearing;
            str_width = extents.width;
            str_height = extents.height;
            
            if (str_height > line_height) line_height = str_height;
            if (str_y_pos > y_pos) y_pos = str_y_pos;
            sinfo->x = linfo->width;
            linfo->width += str_width;
            linfo->strings.push_back(sinfo);
            if (textptr[end_offset] == '\t') {
                double tab, with_space;
                if (space_width < 0.0) {
                    cairo_text_extents(cr, "1 1", &extents);
                    with_space = extents.width;
                    cairo_text_extents(cr, "11", &extents);
                    space_width = with_space - extents.width;
                }
                tab = ceil((linfo->width + space_width) / tab_space);
                if (tab == ((linfo->width + space_width) / tab_space))
                    tab += 1.0;
                linfo->width = tab * tab_space;
            }
            if (textptr[end_offset] == '\0') end_of_lines = true;
        } while (textptr[end_offset++] == '\t');

        lines.push_back(linfo);
    } while (! end_of_lines);

    list<RenderLineInfo *>::iterator it = lines.begin();
    for (; it != lines.end(); it++)
        if ((*it)->width > width) width = (*it)->width;

    height = y_pos + y_pos + top_space + bottom_space + 1.0;
    height += (linenr - 1) * line_height + (linenr - 1) * line_space;
    width += (left_space + right_space + 2.0);
    if (shadow) {
        width += shadow_x_offset;
        height += shadow_y_offset;
    }

    if (_w) {
        if (! dynamic_area_width) {
            width = _w;
            width_u = _wu;
        } else {
            double max_w;
            calc_length(_w, _wu, dwo->ws->hdpi, w, &max_w);
            if (width > max_w) {
                width = _w;
                width_u = _wu;
            }
        }
    }
    if (_h)  {
        if (! dynamic_area_height) {
            height = _h;
            height_u = _hu;
        } else {
            double max_h;
            calc_length(_h, _hu, dwo->ws->vdpi, h, &max_h);
            if (height > max_h) {
                height = _h;
                height_u = _hu;
            }
        }
    }

    calc_position_and_size(halign, valign, gravity,
                           _xu, _yu, width_u, height_u, w, h,
                           dwo->ws->hdpi, dwo->ws->vdpi,
                           width, height, _x, _y, &x, &y,
                           &width, &height);
    if (x2_set || y2_set) {
        double tmp_x2, tmp_y2;
        calc_position(gravity2, _x2u, _y2u, w, h, dwo->ws->hdpi, dwo->ws->vdpi,
                      _x2, _y2, &tmp_x2, &tmp_y2);
        if (x2_set) width = tmp_x2 - x;
        if (y2_set) height = tmp_y2 - y;
    }

    if (width <= 0 || height <= 0) goto bail;

    cairo_save(cr);

    text_region = cairo_surface_create_similar(
        cairo_get_target(cr), CAIRO_CONTENT_COLOR_ALPHA,
        (unsigned int) ceil(width), (unsigned int) ceil(height));

	if ( cr ) { cairo_destroy(cr); }
    cr = cairo_create(crsurface);
    
    if (bg_group) {
        bg_group->parent_surface = cairo_get_target(cr);
        bg_group->return_surface = &subsurface;
        cairo_save(cr);
        bg_group->render(dwo, cr, NULL, (unsigned int) ceil(width),
                         (unsigned int) ceil(height));
        cairo_restore(cr);
        bg_group->return_surface = NULL;
        bg_group->parent_surface = NULL;

        if (subsurface) {
            cairo_pattern_t *pattern;
            
            cairo_save(cr);
            pattern = cairo_pattern_create_for_surface(subsurface->crsurface);
            cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
            cairo_set_source(cr, pattern);
            cairo_pattern_destroy(pattern);
            cairo_rectangle(cr, 0.0, 0.0, ceil(width), ceil(height));
            cairo_fill(cr);
            cairo_restore(cr);
            subsurface->unref();
        }
    }
    
    y_off = -line_spacing;
    y_off += top_space;

    it = lines.begin();
    for (; it != lines.end(); it++) {
        double pos_x;

        y_off += line_spacing;
        calcLinePosition((*it)->width, (unsigned int) ceil(width),
                         (unsigned int) ceil(height), &pos_x);

        list<RenderStringInfo *>::iterator sit = (*it)->strings.begin();
        for (; sit != (*it)->strings.end(); sit++) {
            if (stroke || fill_pattern) {
              cairo_move_to(cr, (int) (left_space + (*sit)->x + pos_x),
                            (int) (y_off + y_pos));
              cairo_text_path(cr, (*sit)->text);
              cairo_move_to(cr, 0.0, 0.0);
              draw(dwo, cr, crsurface, WA_ROUND_U(width),
                   WA_ROUND_U(height));
            } else {
              if (shadow) {
                shadow_color.setcairo_color(cr);
                cairo_move_to(cr, (int) (left_space + (*sit)->x + pos_x +
                                         shadow_x_offset),
                              (int) (y_off + y_pos + shadow_y_offset));
                cairo_show_text(cr, (*sit)->text);
              }
              fill_color.setcairo_color(cr);
              cairo_move_to(cr, (int) (left_space + (*sit)->x + pos_x),
                            (int) (y_off + y_pos));
              cairo_show_text(cr, (*sit)->text);
            }
        }
            
        y_off += line_height;
    }

    cairo_restore(cr);

    if (x || y) cairo_translate(cr, x, y);
    if (xrop_set) cairo_set_operator(cr, xrop);
    if (rotation) cairo_rotate(cr, rotation);
    cairo_set_source_surface(cr, text_region, 0., 0.);
    cairo_rectangle(cr, 0., 0., ceil(width), ceil(height));
    cairo_clip(cr);
    cairo_paint(cr);
    
    cairo_surface_destroy(text_region);

 bail:
    while (! lines.empty()) {
        while (! lines.back()->strings.empty()) {
            delete [] lines.back()->strings.back()->text;
            delete lines.back()->strings.back();
            lines.back()->strings.pop_back();
        }
        delete lines.back();
        lines.pop_back();
    }
}

RenderOpFill::RenderOpFill(RenderOpType _type) : RenderOp(_type) {
    fillrule = RENDER_FILL_RULE_DEFAULT;
}

void RenderOpFill::applyAttributes(Parser *parser, Tst<char *> *attr) {
    ((RenderOp *) this)->applyAttributes(parser, attr);
}

void RenderOpSolid::applyAttributes(Parser *parser, Tst<char *> *attr) {
    ((RenderOpFill *) this)->applyAttributes(parser, attr);
    
    char *value = parser->attrGetString(attr, "color", NULL);
    if (value) {
        if (! color.parseColor(parser->ws, value))
            parser->warning("error parsing color: %s\n", value);
    }
    color.setOpacity(parser->attrGetDouble(attr, "opacity",
                                           color.getOpacity()));
}

void RenderOpSolid::render(DWindowObject *dwo, cairo_t *cr, cairo_surface_t *,
                           unsigned int w, unsigned int h) {
    double x, y, width, height;
    
    calcPositionAndSize(w, h, dwo->ws->hdpi, dwo->ws->vdpi, &x, &y,
                        &width, &height);
    
    if (color.alpha == 1.0) cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    if (xrop_set) cairo_set_operator(cr, xrop);
    color.setcairo_color(cr);
    cairo_rectangle(cr, x, y, width, height);
    cairo_rotate(cr, rotation);
    cairo_set_fill_rule(cr, fillrule);
    cairo_fill(cr);
}

RenderOpImage::RenderOpImage(void) : RenderOpFill(RenderOpImageType) {
    scale = RENDER_IMAGE_SCALE_DEFAULT;
    image = NULL;
    filter = RENDER_FILTER_DEFAULT;
}

RenderOpImage::~RenderOpImage(void) {
    if (image) image->unref();
}

static const struct ImageScaleTypeMap {
    char *name;
    ImageScaleType type;
} image_scale_type_map[] = {
    { "scaled", ImageNormalScaleType },
    { "tiled", ImageTileScaleType }
};

bool RenderOpImage::applyAttributes(Parser *parser, Tst<char *> *attr) {
    int image_width, image_height;
    unsigned char *rgba = NULL;

    ((RenderOpFill *) this)->applyAttributes(parser, attr);

    char *value = parser->attrGetString(attr, "src", NULL);
    if (value) {
        char *filename = smartfile(value, parser->filename, false);
        if (filename) {
            rgba = read_image_to_rgba(filename, &image_width, &image_height);
            if (rgba)
                image = parser->ws->rgbaToWaSurface(rgba, image_width,
                                                    image_height);
            delete [] filename;
        } else {
            parser->warning("unable to open image file: %s\n", value);
            return false;
        }
            
        if (! image) {
            parser->warning("failed loading image: %s\n", value);
            return false;
        }
    } else {
        parser->warning("required attribute=src is missing\n");
        return false;
    }

    value = parser->attrGetString(attr, "scaletype", NULL);
    if (value) {
        Tst<ImageScaleType>::iterator it = image_scale_type_tst->find(value);
        if (it != image_scale_type_tst->end()) scale = *it;
    }

    value = parser->attrGetString(attr, "filter", NULL);
    if (value) {
        Tst<cairo_filter_t>::iterator it = filter_tst->find(value);
        if (it != filter_tst->end()) filter = *it;
    }

    return true;
}

void RenderOpImage::render(DWindowObject *dwo, cairo_t *cr, cairo_surface_t *,
                           unsigned int w, unsigned int h) {
    double x, y, width, height;
    WaSurface *img;
    cairo_matrix_t *matrix;
    cairo_pattern_t *pattern;

    if (image) img = image->ref();
    else return;
    
    calcPositionAndSize(w, h, dwo->ws->hdpi, dwo->ws->vdpi,
                        &x, &y, &width, &height);
    
    switch (scale) {
        case ImageNormalScaleType:
            cairo_identity_matrix(cr);
            
            if (width != img->width || height != img->height) {
                if (xrop_set) cairo_set_operator(cr, xrop);
                cairo_operator_t op = cairo_get_operator(cr);
            
                cairo_surface_t *target = cairo_get_target(cr);
                cairo_surface_t *scaled =
                    cairo_surface_create_similar(target, CAIRO_CONTENT_COLOR_ALPHA,
                                                 (int) width + 2,
                                                 (int) height + 2);
                cairo_destroy(cr);
                cr = cairo_create(scaled);
                cairo_scale(cr, (double) width / (img->width -
                                                  ((img->width > 1)? 1: 0)),
                            (double) height / (img->height -
                                               ((img->height > 1)? 1: 0)));
                cairo_pattern_t *img_pattern =
                    cairo_pattern_create_for_surface(img->crsurface);
                cairo_pattern_set_filter(img_pattern, filter);
                cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
                cairo_set_source(cr, img_pattern);
                cairo_rectangle(cr, 0., 0., img->width + 1, img->height + 1);
                cairo_fill(cr);
                cairo_identity_matrix(cr);
                cairo_pattern_destroy(img_pattern);

                cairo_destroy(cr);
                cr = cairo_create(target);
                cairo_translate(cr, x, y);
                cairo_set_operator(cr, op);
                cairo_set_source_surface(cr, scaled, 0., 0.);
                cairo_rectangle(cr, 0., 0., width + 2., height + 2.);
                cairo_fill(cr);                
                cairo_surface_destroy(scaled);
            } else {
                if (xrop_set) cairo_set_operator(cr, xrop);
                cairo_translate(cr, x, y);
                cairo_rotate(cr, rotation);
                cairo_set_source_surface(cr, img->crsurface, 0., 0.);
                cairo_rectangle(cr, 0., 0., img->width + 1., img->height + 1.);
                cairo_fill(cr);
            }
            break;
        case ImageTileScaleType: {
            cairo_identity_matrix(cr);
            cairo_translate(cr, x, y);
            cairo_rectangle(cr, 0, 0, width, height);
            cairo_move_to(cr, 0.0, 0.0);
            cairo_identity_matrix(cr);
            pattern = cairo_pattern_create_for_surface(img->crsurface);
            cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
            cairo_set_source(cr, pattern);
            cairo_pattern_destroy(pattern);
            if (xrop_set) cairo_set_operator(cr, xrop);
            cairo_fill(cr);
            //cairo_surface_set_repeat(img->crsurface, false);
        } break;
    }
    
    img->unref();
}

#ifdef    SVG
RenderOpSvg::RenderOpSvg(void) : RenderOpFill(RenderOpSvgType) {
    cairo_svg = NULL;
}

RenderOpSvg::~RenderOpSvg(void) {
    if (cairo_svg) svg_cairo_destroy(cairo_svg);
}

bool RenderOpSvg::applyAttributes(Parser *parser,
                                  Tst<char *> *attr) {
    ((RenderOpFill *) this)->applyAttributes(parser, attr);

    char *value = parser->attrGetString(attr, "file", NULL);
    if (value) {
        char *filename = smartfile(value, parser->filename, false);
        if (filename) {
            FILE *file = fopen(filename, "r");
            if (file) {
                svg_cairo_status_t status;
                status = svg_cairo_create(&cairo_svg);
                if (status == SVG_CAIRO_STATUS_SUCCESS) {
                    status = svg_cairo_parse_file(cairo_svg, file);
                    if (status == SVG_CAIRO_STATUS_SUCCESS) {
                        return true;
                    } else {
                        parser->warning("parsing of svg file %s failed\n",
                                        value);
                    }
                } else {
                    parser->warning("creation of svg context failed\n");
                }
                fclose(file);
            }
            delete [] filename;
        } else
            parser->warning("unable to open svg file: %s\n", value);
    } else
        parser->warning("missing required attribute=file'\n");

    return false;
}

void RenderOpSvg::render(DWindowObject *dwo, cairo_t *cr,
                         cairo_surface_t *,
                         unsigned int w, unsigned int h) {
    double x, y, width, height;
    unsigned int svg_w, svg_h;
    
    calcPositionAndSize(w, h, dwo->ws->hdpi, dwo->ws->vdpi,
                        &x, &y, &width, &height);
    
    cairo_save(cr);
    if (xrop_set) cairo_set_operator(cr, xrop);
    if (rotation) cairo_rotate(cr, rotation);
    cairo_translate(cr, x, y);

    svg_cairo_get_size(cairo_svg, &svg_w, &svg_h);
    cairo_scale(cr, width / (double) svg_w, height / (double) svg_h);

    svg_cairo_set_viewport_dimension(cairo_svg, (unsigned int) width,
                                     (unsigned int) height);

    if (svg_cairo_render(cairo_svg, cr) != SVG_CAIRO_STATUS_SUCCESS) {
        dwo->ws->showWarningMessage(__FUNCTION__, "svg rendering failed");
    }
    
    cairo_restore(cr);
}
#endif // SVG

void calc_length(double value, LenghtUnitType unit, double dpi,
                 double parent_length, double *return_value) {
    switch (unit) {
        case PXLenghtUnitType:
            *return_value = value;
            break;
        case CMLenghtUnitType:
            *return_value = (value / 2.54) * dpi;
            break;
        case MMLenghtUnitType:
            *return_value = (value / 25.4) * dpi;
            break;
        case INLenghtUnitType:
            *return_value = value * dpi;
            break;
        case PTLenghtUnitType:
            *return_value = (value / 72.0) * dpi;
            break;
        case PCLenghtUnitType:
            *return_value = (value / 6.0) * dpi;
            break;
        case PCTLenghtUnitType:
            *return_value = (value * parent_length) / 100.0;
            break;
    }
}

void calc_position(int gravity, LenghtUnitType _xu, LenghtUnitType _yu,
                   double parent_w, double parent_h, double hdpi, double vdpi,
                   double _x, double _y, double *return_x, double *return_y) {
    double x, y;

    calc_length(_x, _xu, hdpi, parent_w, &x);
    calc_length(_y, _yu, vdpi, parent_h, &y);
    
    switch (gravity) {
        case NorthEastGravity:
            x = parent_w - x;
            break;
        case NorthGravity:
            x = parent_w / 2 + x;
            break;
        case SouthEastGravity:
            x = parent_w - x;
            y = parent_h - y;
            break;
        case SouthWestGravity:
            y = parent_h - y;
            break;
        case SouthGravity:
            x = parent_w / 2 + x;
            y = parent_h - y;
            break;
        case EastGravity:
            x = parent_w - x;
            y = parent_h / 2 + y;
            break;
        case WestGravity:
            y = parent_h / 2 + y;
            break;
        default:
            break;
    }

    *return_x = x;
    *return_y = y;
}

void calc_position_and_size(HorizontalAlignment halign,
                            VerticalAlignment valign, int gravity,
                            LenghtUnitType _xu, LenghtUnitType _yu,
                            LenghtUnitType _wu, LenghtUnitType _hu,
                            double parent_w, double parent_h,
                            double hdpi, double vdpi,
                            double _w, double _h, double _x, double _y,
                            double *return_x, double *return_y,
                            double *return_w, double *return_h) {
    double x, y, w, h;

    calc_length(_w, _wu, hdpi, parent_w, &w);
    calc_length(_h, _hu, vdpi, parent_h, &h);
    calc_length(_x, _xu, hdpi, parent_w, &x);
    calc_length(_y, _yu, vdpi, parent_h, &y);
    
    switch (gravity) {
        case NorthEastGravity:
            x = parent_w - x;
            break;
        case NorthGravity:
            x = parent_w / 2 + x;
            break;
        case SouthEastGravity:
            x = parent_w - x;
            y = parent_h - y;
            break;
        case SouthWestGravity:
            y = parent_h - y;
            break;
        case SouthGravity:
            x = parent_w / 2 + x;
            y = parent_h - y;
            break;
        case EastGravity:
            x = parent_w - x;
            y = parent_h / 2 + y;
            break;
        case WestGravity:
            y = parent_h / 2 + y;
            break;
        case CenterGravity:
            x = parent_w / 2 + x;
            y = parent_h / 2 + y;
            break;
        default:
            break;
    }
    
    switch (halign) {
        case RightAlignType:
            x -= w;
            break;
        case HCenterAlignType:
            x -= w / 2;
            break;
        default:
            break;
    }

    switch (valign) {
        case BottomAlignType:
            y -= h;
            break;
        case VCenterAlignType:
            y -= h / 2;
            break;
        default:
            break;
    }
    
    *return_x = x;
    *return_y = y;

    if (return_w) *return_w = w;
    if (return_h) *return_h = h;
}

unsigned char *read_image_to_rgba(char *filename,
                                  int *image_width, int *image_height) {
    FILE *fp;
    bool status = false;
    unsigned char *rgba = NULL;

    UNUSED_VARIABLE(image_width);
    UNUSED_VARIABLE(image_height);
    
    if ((fp = fopen(filename, "rb")) == NULL) {
        
        WARNING << "`" << filename << "' "; perror(NULL);
        return NULL;
    }
    
#ifdef    PNG
    if (! status) {
        status = check_if_png(fp);
        rewind(fp);
        if (status)
            rgba = read_png_to_rgba(fp, image_width, image_height);
    }
#endif // PNG
    
    if (! status)
        WARNING << "`" << filename << "' unsupported image format" << endl;
    
    fclose(fp);
    
    return rgba;
}

static const struct GravityTypeMap {
    char *name;
    int gravity;
} gravity_type_map[] = {
    { "north", NorthGravity },
    { "northwest", NorthWestGravity },
    { "northeast", NorthEastGravity },
    { "south", SouthGravity },
    { "southwest", SouthWestGravity },
    { "southeast", SouthEastGravity },
    { "west", WestGravity },
    { "east", SouthGravity },
    { "center", CenterGravity }
};

static const struct HorizontalAlignmentMap {
    char *name;
    HorizontalAlignment align;
} horizontal_alignment_map[] = {
    { "left", LeftAlignType },
    { "right", RightAlignType },
    { "center", HCenterAlignType }
};

static const struct VerticalAlignmentMap {
    char *name;
    VerticalAlignment align;
} vertical_alignment_map[] = {
    { "top", TopAlignType },
    { "bottom", BottomAlignType },
    { "center", VCenterAlignType }
};

static const struct OperatorMap {
    char *name;
    cairo_operator_t crop;
} operator_map[] = {
    { "clear", CAIRO_OPERATOR_CLEAR },
    { "src", CAIRO_OPERATOR_SOURCE },
    { "dst", CAIRO_OPERATOR_DEST },
    { "dst_over", CAIRO_OPERATOR_DEST_OVER },
    { "dst_in", CAIRO_OPERATOR_DEST_IN },
    { "dst_out", CAIRO_OPERATOR_DEST_OUT },
    { "dst_atop", CAIRO_OPERATOR_DEST_ATOP },
    { "over", CAIRO_OPERATOR_OVER },
    { "in", CAIRO_OPERATOR_IN },
    { "out", CAIRO_OPERATOR_OUT },
    { "atop", CAIRO_OPERATOR_ATOP },
    { "xor", CAIRO_OPERATOR_XOR },
    { "add", CAIRO_OPERATOR_ADD },
    { "saturate", CAIRO_OPERATOR_SATURATE }
};

void attributes_get_position(Parser *parser, Tst<char *> *attr,
                             char *xname, char *yname, char *gname,
                             double *x, double *y,
                             LenghtUnitType *_xu, LenghtUnitType *_yu,
                             int *g) {
    double tmp = parser->attrGetLength(attr, xname, INT_MIN, _xu);
    if (tmp != INT_MIN) *x = tmp;
    tmp = parser->attrGetLength(attr, yname, INT_MIN, _yu);
    if (tmp != INT_MIN) *y = tmp;
    char *value = parser->attrGetString(attr, gname, NULL);
    if (value) {
        Tst<int>::iterator it = gravity_type_tst->find(value);
        if (it != gravity_type_tst->end()) *g = *it;
    }
}

void attributes_get_size(Parser *parser, Tst<char *> *attr,
                         char *wname, char *hname,
                         double *w, double *h,
                         LenghtUnitType *_wu, LenghtUnitType *_hu) {
    double tmp = parser->attrGetLength(attr, wname, INT_MIN, _wu);
    if (tmp != INT_MIN) *w = tmp;
    tmp = parser->attrGetLength(attr, hname, INT_MIN, _hu);
    if (tmp != INT_MIN) *h = tmp;
}

void attributes_get_alignment(Parser *parser, Tst<char *> *attr,
                              char *haname, char *vaname,
                              HorizontalAlignment *ha, VerticalAlignment *va) {
    
    char *value;

    if (ha) {
        value = parser->attrGetString(attr, haname, NULL);
        if (value) {
            Tst<HorizontalAlignment>::iterator it =
                horizontal_alignment_tst->find(value);
            if (it != horizontal_alignment_tst->end()) *ha = *it;
        }
    }

    if (va) {
        value = parser->attrGetString(attr, vaname, NULL);
        if (value) {
            Tst<VerticalAlignment>::iterator it =
                vertical_alignment_tst->find(value);
            if (it != vertical_alignment_tst->end()) *va = *it;
        }
    }
}

void attributes_get_area(Parser *parser, Tst<char *> *attr,
                         char *xname, char *yname, char *gname,
                         char *x2name, char *y2name, char *g2name,
                         char *wname, char *hname,
                         char *haname, char *vaname,
                         double *x, double *y,
                         LenghtUnitType *_xu, LenghtUnitType *_yu, int *g,
                         double *x2, double *y2,
                         LenghtUnitType *_x2u, LenghtUnitType *_y2u, int *g2,
                         bool *x2set, bool *y2set,
                         double *w, double *h,
                         LenghtUnitType *_wu, LenghtUnitType *_hu,
                         HorizontalAlignment *ha, VerticalAlignment *va) {
    double tmp_x2 = INT_MIN, tmp_y2 = INT_MIN;
    attributes_get_position(parser, attr, xname, yname, gname, x, y,
                            _xu, _yu, g);
    attributes_get_position(parser, attr, x2name, y2name, g2name, &tmp_x2,
                            &tmp_y2, _x2u, _y2u, g2);
    if (tmp_x2 != INT_MIN) {
        *x2 = tmp_x2;
        *x2set = true;
    }
    if (tmp_y2 != INT_MIN) {
        *y2 = tmp_y2;
        *y2set = true;
    }
    attributes_get_size(parser, attr, wname, hname, w, h, _wu, _hu);
    attributes_get_alignment(parser, attr, haname, vaname, ha, va);
}

void attributes_get_render(Parser *parser, Tst<char *> *attr,
                           cairo_operator_t *o, bool *opset, double *r) {
    char *value = parser->attrGetString(attr, "operator", NULL);
    if (value) {
        Tst<cairo_operator_t>::iterator it = operator_tst->find(value);
        if (it != operator_tst->end()) {
            *o = *it;
            *opset = true;
        }
    }
    double angle = parser->attrGetDouble(attr, "rotation", (double) -INT_MAX);
    if (angle != (double) -INT_MAX) *r = angle / (M_PI * 2.0);
}

void render_create_tsts(void) {
    int size, i;

    filter_tst = new Tst<cairo_filter_t>;
    size = sizeof(filter_map) / sizeof(FilterMap);
    for (i = 0; i < size; i++)
        filter_tst->insert(filter_map[i].name, filter_map[i].filter);

    extend_tst = new Tst<cairo_extend_t>;
    size = sizeof(extend_map) / sizeof(ExtendMap);
    for (i = 0; i < size; i++)
        extend_tst->insert(extend_map[i].name, extend_map[i].extend);

    pattern_type_tst = new Tst<PatternType>;
    size = sizeof(pattern_type_map) / sizeof(PatternTypeMap);
    for (i = 0; i < size; i++)
        pattern_type_tst->insert(pattern_type_map[i].name,
                                 pattern_type_map[i].pattern_type); 
    
    line_cap_tst = new Tst<cairo_line_cap_t>;
    size = sizeof(line_cap_map) / sizeof(LineCapMap);
    for (i = 0; i < size; i++)
        line_cap_tst->insert(line_cap_map[i].name, line_cap_map[i].line_cap);

    line_join_tst = new Tst<cairo_line_join_t>;
    size = sizeof(line_join_map) / sizeof(LineJoinMap);
    for (i = 0; i < size; i++)
        line_join_tst->insert(line_join_map[i].name,
                              line_join_map[i].line_join);

    fill_rule_tst = new Tst<cairo_fill_rule_t>;
    size = sizeof(fill_rule_map) / sizeof(FillRuleMap);
    for (i = 0; i < size; i++)
        fill_rule_tst->insert(fill_rule_map[i].name,
                              fill_rule_map[i].fill_rule);

    drawing_order_tst = new Tst<DrawingOrder>;
    size = sizeof(drawing_order_map) / sizeof(DrawingOrderMap);
    for (i = 0; i < size; i++)
        drawing_order_tst->insert(drawing_order_map[i].name,
                                  drawing_order_map[i].order);
   
    weight_tst = new Tst<cairo_font_weight_t>;
    size = sizeof(weight_map) / sizeof(WeightMap);
    for (i = 0; i < size; i++)
        weight_tst->insert(weight_map[i].name, weight_map[i].weight);

    slant_tst = new Tst<cairo_font_slant_t>;
    size = sizeof(slant_map) / sizeof(SlantMap);
    for (i = 0; i < size; i++)
        slant_tst->insert(slant_map[i].name, slant_map[i].slant);

    image_scale_type_tst = new Tst<ImageScaleType>;
    size = sizeof(image_scale_type_map) / sizeof(ImageScaleTypeMap);
    for (i = 0; i < size; i++)
        image_scale_type_tst->insert(image_scale_type_map[i].name,
                                     image_scale_type_map[i].type);

    gravity_type_tst = new Tst<int>;
    size = sizeof(gravity_type_map) / sizeof(GravityTypeMap);
    for (i = 0; i < size; i++)
        gravity_type_tst->insert(gravity_type_map[i].name,
                                 gravity_type_map[i].gravity);

    horizontal_alignment_tst = new Tst<HorizontalAlignment>;
    size = sizeof(horizontal_alignment_map) / sizeof(HorizontalAlignmentMap);
    for (i = 0; i < size; i++)
        horizontal_alignment_tst->insert(horizontal_alignment_map[i].name,
                                         horizontal_alignment_map[i].align);
    
    vertical_alignment_tst = new Tst<VerticalAlignment>;
    size = sizeof(vertical_alignment_map) / sizeof(VerticalAlignmentMap);
    for (i = 0; i < size; i++)
        vertical_alignment_tst->insert(vertical_alignment_map[i].name,
                                       vertical_alignment_map[i].align);

    operator_tst = new Tst<cairo_operator_t>;
    size = sizeof(operator_map) / sizeof(OperatorMap);
    for (i = 0; i < size; i++)
        operator_tst->insert(operator_map[i].name,
                             operator_map[i].crop);    
}
