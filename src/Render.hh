/* Render.hh

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

#ifndef __Render_hh
#define __Render_hh

extern "C" {
#include <cairo-xlib.h>

#ifdef    SVG
#include <svg-cairo.h>
#endif // SVG  
}

#include "RefCounted.hh"
#include "Util.hh"
#include "Parser.hh"

#define RENDER_X_DEFAULT             0.0
#define RENDER_Y_DEFAULT             0.0
#define RENDER_WIDTH_DEFAULT         10.0
#define RENDER_HEIGHT_DEFAULT        10.0
#define RENDER_OPERATOR_DEFAULT      CAIRO_OPERATOR_OVER
#define RENDER_ROTATION_DEFAULT      0.0
#define RENDER_TOLERANCE_DEFAULT     0.1
#define RENDER_LINE_WIDTH_DEFAULT    2.0
#define RENDER_LINE_CAP_DEFAULT      CAIRO_LINE_CAP_BUTT
#define RENDER_FILL_RULE_DEFAULT     CAIRO_FILL_RULE_WINDING
#define RENDER_LINE_JOIN_DEFAULT     CAIRO_LINE_JOIN_MITER
#define RENDER_MITER_LIMIT_DEFAULT   10.0
#define RENDER_FONT_KEY_DEFAULT      "sans"
#define RENDER_FONT_SIZE_DEFAULT     12.0
#define RENDER_FONT_WEIGHT_DEFAULT   CAIRO_FONT_WEIGHT_NORMAL
#define RENDER_FONT_SLANT_DEFAULT    CAIRO_FONT_SLANT_NORMAL
#define RENDER_TAB_SPACE_DEFAULT     20.0
#define RENDER_DEFAULT_RECT_RX       0.0
#define RENDER_DEFAULT_RECT_RY       0.0

#define RENDER_COLOR_RED_DEFAULT     0.0
#define RENDER_COLOR_GREEN_DEFAULT   0.0
#define RENDER_COLOR_BLUE_DEFAULT    0.0
#define RENDER_OPACITY_DEFAULT       1.0

#define RENDER_HALIGN_DEFAULT        LeftAlignType
#define RENDER_VALIGN_DEFAULT        TopAlignType
#define RENDER_GRAVITY_DEFAULT       NorthWestGravity

#define RENDER_FILTER_DEFAULT        CAIRO_FILTER_BILINEAR
#define RENDER_EXTEND_DEFAULT        CAIRO_EXTEND_NONE

#define RENDER_LINE_SPACING_DEFAULT  2.0

#define RENDER_IMAGE_SCALE_DEFAULT   ImageNormalScaleType

#define RENDER_DRAWING_ORDER_DEFAULT FillStrokeDrawingOrder

typedef enum {
    LeftAlignType,
    RightAlignType,
    HCenterAlignType,
    HNullAlignType
} HorizontalAlignment;

typedef enum {
    TopAlignType,
    BottomAlignType,
    VCenterAlignType,
    VNullAlignType
} VerticalAlignment;

typedef enum {
    RenderOpGroupType,
    RenderOpDynamicType,
    RenderOpPathType,
    RenderOpLineType,
    RenderOpRectangleType,
    RenderOpTextType,
    RenderOpSolidType,
    RenderOpImageType,
    RenderOpSvgType
} RenderOpType;

typedef enum {
    PathOperatorMoveToType,
    PathOperatorRelMoveToType,
    PathOperatorLineToType,
    PathOperatorRelLineToType,
    PathOperatorCurveToType,
    PathOperatorRelCurveToType,
    PathOperatorArcToType,
    PathOperatorRelArcToType,
    PathOperatorCloseType
} PathOperatorType;

typedef enum {
    LinearPatternType,
    RadialPatternType,
    GroupPatternType
} PatternType;

typedef enum {
    ImageNormalScaleType,
    ImageTileScaleType
} ImageScaleType;

typedef enum {
    FillStrokeDrawingOrder,
    StrokeFillDrawingOrder
} DrawingOrder;

class DWindowObject;
class WaScreen;

class RenderOp;
class RenderOpText;

void calc_length(double, LenghtUnitType, double, double, double *);
void calc_position(int, LenghtUnitType, LenghtUnitType,
                   double, double, double, double, double, double,
                   double *, double *);
void calc_position_and_size(HorizontalAlignment, VerticalAlignment, int,
                            LenghtUnitType, LenghtUnitType,
                            LenghtUnitType, LenghtUnitType,
                            double, double, double, double,
                            double, double, double, double,
                            double *, double *, double *, double *);

unsigned char *read_image_to_rgba(char *, int *, int *);

void attributes_get_position(Parser *, Tst<char *> *,
                             char *, char *, char *,
                             double *, double *,
                             LenghtUnitType *, LenghtUnitType *, int *);
inline void attributes_get_position_default(Parser *parser, Tst<char *> *attr,
                                            double *x, double *y,
                                            LenghtUnitType *xu,
                                            LenghtUnitType *yu, int *g) {
    attributes_get_position(parser, attr, "x", "y", "gravity",
                            x, y, xu, yu, g);
}

void attributes_get_size(Parser *, Tst<char *> *, char *, char *,
                         double *, double *,
                         LenghtUnitType *, LenghtUnitType *);
inline void attributes_get_size_default(Parser *parser, Tst<char *> *attr,
                                        double *w, double *h,
                                        LenghtUnitType *wu,
                                        LenghtUnitType *hu) {
    attributes_get_size(parser, attr, "width", "height", w, h, wu, hu);
}

void attributes_get_alignment(Parser *, Tst<char *> *, char *, char *,
                              HorizontalAlignment *, VerticalAlignment *);
inline void attributes_get_alignment_default(Parser *parser, Tst<char *> *attr,
                                             HorizontalAlignment *ha,
                                             VerticalAlignment *va) {
    attributes_get_alignment(parser, attr, "halign", "valign", ha, va);
}

void attributes_get_area(Parser *, Tst<char *> *,
                         char *, char *, char *, char *, char *, char *,
                         char *, char *, char *, char *,
                         double *, double *,
                         LenghtUnitType *, LenghtUnitType *, int *,
                         double *, double *,
                         LenghtUnitType *, LenghtUnitType *, int *,
                         bool *, bool *, double *, double *,
                         LenghtUnitType *, LenghtUnitType *,
                         HorizontalAlignment *, VerticalAlignment *);
inline void attributes_get_area_default(Parser *parser, Tst<char *> *attr,
                                        double *x, double *y,
                                        LenghtUnitType *xu,
                                        LenghtUnitType *yu,
                                        int *g,
                                        double *x2, double *y2,
                                        LenghtUnitType *x2u,
                                        LenghtUnitType *y2u, int *g2,
                                        bool *x2set, bool *y2set,
                                        double *w, double *h,
                                        LenghtUnitType *wu,
                                        LenghtUnitType *hu,
                                        HorizontalAlignment *ha,
                                        VerticalAlignment *va) {
    attributes_get_area(parser, attr, "x", "y", "gravity", "x2", "y2",
                        "gravity2", "width", "height", "halign", "valign",
                        x, y, xu, yu, g, x2, y2, x2u, y2u, g2, x2set, y2set,
                        w, h, wu, hu, ha, va);
}
    
void attributes_get_render(Parser*, Tst<char*> *, cairo_operator_t *, bool *,
                           double *);

class WaSurface : public RefCounted<WaSurface> {
public:
    WaSurface(Display *, cairo_surface_t *, Pixmap, Pixmap,
              unsigned char *, unsigned int, unsigned int);
    virtual ~WaSurface(void);

    Display *display;
    cairo_surface_t *crsurface;
    Pixmap pixmap, bitmap;
    unsigned char *data;
    unsigned int width, height;
};

class CacheSurface : public WaSurface {
public:
    CacheSurface(Display *, cairo_surface_t *, Pixmap,
                 unsigned char *, unsigned int, unsigned int,
                 map<int, RenderGroup *> *);
    ~CacheSurface(void);

    bool dynamicGroupsMatch(DWindowObject *);
    
    map<int, RenderGroup *> dynamic_groups;
    RenderGroup *group;
};

class WaColor : public RefCounted<WaColor> {
public:
    WaColor(double = RENDER_COLOR_RED_DEFAULT,
            double = RENDER_COLOR_GREEN_DEFAULT,
            double = RENDER_COLOR_BLUE_DEFAULT,
            double = RENDER_OPACITY_DEFAULT);
    
    bool parseColor(WaScreen *, const char *);
    void setOpacity(double o) { alpha = o; }
    double getOpacity(void) { return alpha; }

    inline void setcairo_color(cairo_t *cr) {
        cairo_set_source_rgba(cr, red, green, blue, alpha);
    }    

    double red, green, blue, alpha;
};

class RenderOp : public RefCounted<RenderOp> {
public:
    RenderOp(RenderOpType);
    virtual ~RenderOp(void) {}

    void inheritAttributes(RenderOp *);
    void applyAttributes(Parser *, Tst<char *> *);
    virtual void render(DWindowObject *, cairo_t *, cairo_surface_t *,
                        unsigned int, unsigned int) = 0;
    void calcWidth(double, double, unsigned int *);
    void fcalcWidth(double, double, double *);
    void calcHeight(double, double, unsigned int *);
    void calcPositionAndSize(double, double, double, double,
                             double *, double *, double *, double *);
    void calcPositionFromSize(double, double, double, double,
                              LenghtUnitType, LenghtUnitType,
                              double, double, double *, double *);
    int type;
    bool xrop_set;
    cairo_operator_t xrop;
    int gravity;
    double rotation, _x, _y, _x2, _y2, _w, _h;
    LenghtUnitType _xu, _yu, _x2u, _y2u, _wu, _hu;
    bool x2_set, y2_set;
    HorizontalAlignment halign;
    VerticalAlignment valign;
    int gravity2;
};

class RenderGroup : public RenderOp {
public:
    RenderGroup(WaScreen * = NULL, char * = NULL);
    virtual ~RenderGroup(void);
    
    void clear(void);
    void clearCache(void);
    void inheritContent(RenderGroup *);
    void inheritAttributes(RenderGroup *);
    void applyAttributes(Parser *, Tst<char *> *);
    void render(DWindowObject *, cairo_t *, cairo_surface_t *,
                unsigned int, unsigned int);
    void flattenTextOps(list<RenderOpText *> **);
    void addToCache(CacheSurface *, DWindowObject *);
    CacheSurface *findCachedSurface(DWindowObject *,
                                    unsigned int, unsigned int);
    char *name;
    list<RenderOp *> operations;
    list<CacheSurface *> cache;

    CacheSurface **return_surface;
    cairo_surface_t *parent_surface;
    bool has_dynamic_op;
    bool cacheable, is_a_style;
    double opacity;

protected:
    WaScreen *ws;
};

#define DynamicGroupStaticType            (1 << 0)
#define DynamicGroupMenuItemIconImageType (1 << 1)
#define DynamicGroupMenuItemIconSvgType   (1 << 2)
#define DynamicGroupWmIconImageType       (1 << 3)
#define DynamicGroupWmIconSvgType         (1 << 4)

class RenderOpDynamic : public RenderOp {
public:
    RenderOpDynamic(void);
    
    void applyAttributes(Parser *, Tst<char *> *);
    void render(DWindowObject *, cairo_t *, cairo_surface_t *,
                unsigned int, unsigned int);

    double opacity;
    list<int> dynamic_order;
};

class WaColorStop : public RefCounted<WaColorStop> {
public:
  WaColorStop(double, WaColor *);
  ~WaColorStop(void);
  
  double offset;
  WaColor *color;
};

class RenderPattern : public RefCounted<RenderPattern> {
public:
    RenderPattern();
    ~RenderPattern(void);

    void clear(void);
    void inheritContent(RenderPattern *);
    void inheritAttributes(RenderPattern *);
    void applyAttributes(Parser *, Tst<char *> *);

    void setcairo_pattern(DWindowObject *, cairo_t *, cairo_surface_t *,
                          unsigned int, unsigned int);
  
    list<WaColorStop *> color_stops;
    cairo_filter_t filter;
    cairo_extend_t extend;

    PatternType type;

    int start_gravity, end_gravity;
    double start_x, start_y, end_x, end_y;
    LenghtUnitType start_x_u, start_y_u, end_x_u, end_y_u;

    int center_gravity;
    double center_x, center_y, radius_dx, radius_dy;
    LenghtUnitType center_x_u, center_y_u, radius_dx_u, radius_dy_u;

    RenderGroup *group;
};

class RenderOpDraw : public RenderOp {
public:
    RenderOpDraw(RenderOpType);
    virtual ~RenderOpDraw(void);

    void inheritAttributes(RenderOpDraw *);
    void applyAttributes(Parser *, Tst<char *> *);
    void draw(DWindowObject *, cairo_t *, cairo_surface_t *,
              unsigned int, unsigned int);
    
protected:
    WaColor stroke_color, fill_color;
    RenderPattern *stroke_pattern, *fill_pattern;
    double linewidth, tolerance, miterlimit;
    LenghtUnitType linewidth_u;
    cairo_line_cap_t linecap;
    cairo_line_join_t linejoin;
    cairo_fill_rule_t fillrule;
    double *dashes, dash_offset;
    LenghtUnitType *dashes_u, dash_offset_u;
    int ndash;
    DrawingOrder order;
    bool stroke, fill;
};

class PathOperator : public RefCounted<PathOperator> {
public:
    PathOperator(PathOperatorType);
    ~PathOperator(void);
    
    void applyAttributes(Parser *, Tst<char *> *);
    inline void calcPosition(double w, double h, double hdpi, double vdpi,
                             double *return_x, double *return_y) {
        for (int i = 0; i < size; i++) {
            calc_position(gravity[i], _au[i], _bu[i],
                          (unsigned int) w, (unsigned int) h, hdpi, vdpi,
                          _a[i], _b[i], &return_x[i], &return_y[i]);
        }
    }
    inline void calcSize(double w, double h, double hdpi, double vdpi,
                         double *return_w, double *return_h) {
        for (int i = 0; i < size; i++) {
            calc_length(_a[i], _au[i], hdpi, w, &return_w[i]);
            calc_length(_b[i], _bu[i], vdpi, h, &return_h[i]);
        }
    }
    
    int type;
    
private:
    int size;
    double *_a, *_b;
    LenghtUnitType *_au, *_bu;
    int *gravity;
};

class RenderOpPath : public RenderOpDraw {
public:
    RenderOpPath(char *);
    ~RenderOpPath(void);

    void clear(void);
    void inheritContent(RenderOpPath *);
    void inheritAttributes(RenderOpPath *);
    void applyAttributes(Parser *, Tst<char *> *);
    void render(DWindowObject *, cairo_t *, cairo_surface_t *,
                unsigned int, unsigned int);
    
    list<PathOperator *> ops;
    char *name;
};

class RenderOpLine : public RenderOpDraw {
public:
    RenderOpLine(void) : RenderOpDraw(RenderOpLineType) {}
    ~RenderOpLine(void) {}
    
    void applyAttributes(Parser *, Tst<char *> *);
    void render(DWindowObject *, cairo_t *, cairo_surface_t *,
                unsigned int, unsigned int);
};

class RenderOpRectangle : public RenderOpDraw {
public:
    RenderOpRectangle(void) : RenderOpDraw(RenderOpRectangleType) {}
    ~RenderOpRectangle(void) {}
    
    void applyAttributes(Parser *, Tst<char *> *);
    void render(DWindowObject *, cairo_t *, cairo_surface_t *,
                unsigned int, unsigned int);

    double nw_rx, nw_ry, ne_rx, ne_ry, sw_rx, sw_ry, se_rx, se_ry;
    LenghtUnitType nw_rx_u, nw_ry_u, ne_rx_u, ne_ry_u, sw_rx_u, sw_ry_u,
        se_rx_u, se_ry_u;
};

class RenderStringInfo {
public:
    char *text;
    double x;
};

class RenderLineInfo {
public:
    list<RenderStringInfo *> strings;
    double width;
};

class RenderOpText : public RenderOpDraw {
public:
    RenderOpText(char *);
    ~RenderOpText(void);

    void clear(void);
    void applyAttributes(Parser *, Tst<char *> *);
    void inheritAttributes(RenderOpText *);
    void render(DWindowObject *, cairo_t *, cairo_surface_t *,
                unsigned int, unsigned int);
    bool is_static;
    char *utf8;
    char *name;
    
private:
    void calcLinePosition(double, unsigned int, unsigned int, double *);
    
    double size, tab_spacing, shadow_x_offset, shadow_y_offset, line_spacing,
        left_spacing, right_spacing, top_spacing, bottom_spacing;
    LenghtUnitType size_u, tab_spacing_u, line_spacing_u, left_spacing_u,
        right_spacing_u, top_spacing_u, bottom_spacing_u;
    bool shadow, dynamic_area_width, dynamic_area_height;
    HorizontalAlignment text_halign;
    WaColor shadow_color;
    char *family;
    cairo_font_face_t *font;
    RenderGroup *bg_group;
    cairo_font_weight_t weight;
    cairo_font_slant_t slant;
};

class RenderOpFill : public RenderOp {
public:
    RenderOpFill(RenderOpType);
    virtual ~RenderOpFill(void) {}
    
    void applyAttributes(Parser *, Tst<char *> *);

protected:
    cairo_fill_rule_t fillrule;
};

class RenderOpSolid : public RenderOpFill {
public:
    RenderOpSolid(void) : RenderOpFill(RenderOpSolidType) {}
    ~RenderOpSolid(void) {}

    void applyAttributes(Parser *, Tst<char *> *);
    void render(DWindowObject *, cairo_t *, cairo_surface_t *,
                unsigned int, unsigned int);
    
private:
    WaColor color;
};

class RenderOpImage : public RenderOpFill {
public:
    RenderOpImage(void);
    ~RenderOpImage(void);

    bool applyAttributes(Parser *, Tst<char *> *);
    void render(DWindowObject *, cairo_t *, cairo_surface_t *,
                unsigned int, unsigned int);

    ImageScaleType scale;
    cairo_filter_t filter;
    WaSurface *image;
};

#ifdef    SVG
class RenderOpSvg : public RenderOpFill {
public:
    RenderOpSvg(void);
    ~RenderOpSvg(void);

    bool applyAttributes(Parser *, Tst<char *> *);
    void render(DWindowObject *, cairo_t *, cairo_surface_t *,
                unsigned int, unsigned int);
    
    svg_cairo_t *cairo_svg;
};
#endif // SVG

void render_create_tsts(void);

#endif // __Render_hh
