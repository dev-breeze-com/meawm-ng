/* Menu.cc

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
    
#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H
    
}

#include "Menu.hh"

Menu::Menu(WaScreen *ws, char *n) :
    RootWindowObject(ws, 0, MenuType,
                     new WaStringMap(WindowIDName, n), "frame") {
    XSetWindowAttributes attrib_set;
    name = (*(ids->begin())).second;
    width = height = 0;
    x = y = 0;
    a_id = focus = (Window) 0;
    rootitem_id = rootitemlnk_id = (Window) 0;
    special_items = 0;
    special_menu_type = NoSpecialMenuType;
    width_factor = 1.0;

    if ((! ws->windowlist_menu) && (! strcmp(name, "windowlist"))) {
        ws->windowlist_menu = this;
        special_menu_type = WindowListMenuType;
    } else if (! strcmp(name, "clonemergelist")) {
        special_menu_type = CloneMergeListMenuType;
    } else if (! strcmp(name, "vertmergelist")) {
        special_menu_type = VertMergeListMenuType;
    } else if (! strcmp(name, "horizmergelist")) {
        special_menu_type = HorizMergeListMenuType;
    } else
        special_menu_type = NoSpecialMenuType;

    int create_mask = CWOverrideRedirect | CWEventMask | CWColormap |
        CWDontPropagate;

    if (ws->screen_depth == 32) {
        attrib_set.background_pixmap = None;
        attrib_set.background_pixel = 0x00000000;
        attrib_set.border_pixel = 0x00000000;
        create_mask |= CWBackPixel | CWBorderPixel;
    } else {
        attrib_set.background_pixmap = ParentRelative;
    }
    create_mask |= CWBackPixmap;
    
    attrib_set.colormap = ws->colormap;
    attrib_set.override_redirect = true;
    attrib_set.event_mask = NoEventMask;
    attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
        ButtonMotionMask;
    
    id = XCreateWindow(ws->display, ws->id, x, y, 1, 1, 0,
                       ws->screen_depth, CopyFromParent, ws->visual,
                       create_mask, &attrib_set);
    ws->meawm_ng->window_table.insert(make_pair(id, this));

    setStacking(ws->config.menu_stacking);
}

Menu::~Menu(void) {
    clear();
}

void Menu::applyAttributes(Parser *parser, Tst<char *> *attr) {
    char *value;
    value = parser->attrGetString(attr, "inherit", NULL);
    if (value) {
        Menu *inherit_menu = ws->getMenuNamed(value, false);
        if (inherit_menu == this) {
            parser->warning("menu %s cannot inherit itself\n", name);
        } else if (inherit_menu) {
            inheritAttributes(inherit_menu);
        } else {
            if (! parser->attrGetBool(attr, "ignore_missing", false))
                parser->warning("undefined menu: %s\n", value);
        }
    }
        
    width_factor = parser->attrGetDouble(attr, "width_factor", width_factor);
}

void Menu::clear(void) {
    while (! items.empty()) {
        items.back()->unref();
        items.pop_back();
    }
}

void Menu::inheritContent(Menu *inherit) {
    list<MenuItem *>::iterator it = inherit->items.begin();
    for (; it != inherit->items.end(); ++it) {
        items.push_back((MenuItem *) (*it)->ref());
    }
}

void Menu::inheritAttributes(Menu *inherit) {
    width_factor = inherit->width_factor;
}

void Menu::update(void) {
    unsigned int newwidth = 1;
    unsigned int newheight = 0;
    unsigned int calc_tmp;
    double _width, _top_spacing, _bottom_spacing,_left_spacing, _right_spacing;
    unsigned int top_spacing, bottom_spacing, left_spacing, right_spacing;

    style->fcalcWidth(ws->width, ws->hdpi, &_width);
    calc_length(style->top_spacing, style->top_spacing_u,
                ws->vdpi, ws->height, &_top_spacing);
    calc_length(style->bottom_spacing, style->bottom_spacing_u,
                ws->vdpi, ws->height, &_bottom_spacing);
    calc_length(style->left_spacing, style->left_spacing_u,
                ws->hdpi, ws->width, &_left_spacing);
    calc_length(style->right_spacing, style->right_spacing_u,
                ws->hdpi, ws->width, &_right_spacing);

    top_spacing = WA_ROUND_U(_top_spacing);
    bottom_spacing = WA_ROUND_U(_bottom_spacing);
    left_spacing = WA_ROUND_U(_left_spacing);
    right_spacing = WA_ROUND_U(_right_spacing);

    newheight = top_spacing;
    
    newwidth = WA_ROUND_U(_width * width_factor);

    list<MenuItem *>::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        if ((*it)->icon_image)
            (*it)->setDynamicGroup(DynamicGroupMenuItemIconImageType,
                                   (*it)->icon_image);
        if ((*it)->icon_svg)
            (*it)->setDynamicGroup(DynamicGroupMenuItemIconSvgType,
                                   (*it)->icon_svg);
        (*it)->style->calcWidth(10, ws->hdpi, &calc_tmp);
        if (calc_tmp > newwidth) newwidth = calc_tmp;
        (*it)->newy = newheight;
        (*it)->newx = left_spacing;
        (*it)->style->calcHeight(10, ws->vdpi, &calc_tmp);
        newheight += calc_tmp;
    }
    
    it = items.begin();
    for (; it != items.end(); ++it) {
        (*it)->style->calcHeight(10, ws->vdpi, &calc_tmp);
        if ((*it)->width != newwidth ||
            (*it)->height != calc_tmp ||
            (*it)->newy != (*it)->y || (*it)->newx != (*it)->x) {
            (*it)->y = (*it)->newy;
            (*it)->x = (*it)->newx;
            (*it)->width = newwidth;
            (*it)->height = calc_tmp;
            XMoveResizeWindow(ws->display, (*it)->id, (*it)->x, (*it)->y,
                              ((*it)->width)? (*it)->width: 1,
                              ((*it)->height)? (*it)->height: 1);
        }
    }
    
    newwidth += left_spacing + right_spacing;
    newheight += bottom_spacing;

    if (newwidth != width || newheight != height) {
        width = newwidth;
        height = newheight;
        XResizeWindow(ws->display, id, (width)? width: 1, (height)? height: 1);
    }

    draw();
}

void Menu::draw(void) {
    pushRenderEvent();
    list<MenuItem *>::iterator it = items.begin();
    for (; it != items.end(); ++it)
        (*it)->pushRenderEvent();
}

void Menu::move(int newx, int newy) {
    int diff_x = newx - x;
    int diff_y = newy - y;
    if (newx != x || newy != y) {
        x = newx;
        y = newy;
        XMoveWindow(ws->display, id, x, y);
        pushRenderEvent();
        list<MenuItem *>::iterator it = items.begin();
        for (; it != items.end(); ++it) {
            (*it)->pushRenderEvent();
            if ((*it)->submenu &&
                (*it)->submenu->rootitem_id == (*it)->id) {
                (*it)->submenu->move((*it)->submenu->x + diff_x,
                                     (*it)->submenu->y + diff_y);
            }
        }
    }
}

void Menu::map(AWindowObject *_awo, bool focus, bool force) {
    Window w;
    int i, px, py;
    unsigned int ui;
    int workx, worky;
    unsigned int workw, workh;

    if ((! force) && mapped) return;
    if (ws->meawm_ng->eh->move_resize != EndMoveResizeType) return;
    
    a_id = _awo->id;
    ws->getWorkareaSize(&workx, &worky, &workw, &workh);

    if (XQueryPointer(ws->display, ws->id, &w, &w, &px, &py, &i, &i, &ui)) {
        if ((py + (int) height) > ((int) workh + worky)) py -= height;
        if ((px + (int)width) > ((int) workw + workx)) px -= (width);
    }
    map(px, py);
    if (focus) focusFirst();
}

void Menu::map(int mapx, int mapy) {
    if (items.empty()) {
        ws->showWarningMessage("menu=%s does not contain any items.", name);
        return;
    }
    ws->raiseWindow(id);
    move(mapx, mapy);
    XUngrabPointer(ws->display, CurrentTime);
    if (! mapped) {
        WaWindow *ww = NULL;
        EventDetail ed;
        focus = (Window) 0;
        if (a_id) {
            AWindowObject *awo = (AWindowObject *)
                ws->meawm_ng->findWin(a_id, ANY_ACTION_WINDOW_TYPE);
            if (awo) ww = awo->getWindow();
        }
        if (special_menu_type != NoSpecialMenuType) {
            switch (special_menu_type) {
                case WindowListMenuType:
                    addWindowListItems();
                    break;
                case CloneMergeListMenuType:
                case VertMergeListMenuType:
                case HorizMergeListMenuType:
                    if (ww) addMergeListItems(ww, special_menu_type);
                    break;
                default:
                    break;
            }
        }
        XMapWindow (ws->display, id);
        if (ww) {
            list<MenuItem *>::iterator it = items.begin();
            for (; it != items.end(); ++it) {
                ed.x = ed.y = INT_MAX;
                ed.x11mod = ed.wamod = 0;
                ed.type = MenuMapNotify;
                ws->meawm_ng->eh->evAct(NULL, (*it)->id, &ed);
                ww->addMonitor(*it, (*it)->monitor_state);
                if (ww->wm_icon_image)
                    (*it)->setDynamicGroup(DynamicGroupWmIconImageType,
                                           ww->wm_icon_image);
                if (ww->wm_icon_svg)
                    (*it)->setDynamicGroup(DynamicGroupWmIconSvgType,
                                           ww->wm_icon_svg);
                (*it)->pushRenderEvent();
            }
        }
        if (rootitemlnk_id) {
            ed.x = ed.y = INT_MAX;
            ed.x11mod = ed.wamod = 0;
            ed.type = SubmenuMapNotify;
            ws->meawm_ng->eh->evAct(NULL, rootitemlnk_id, &ed);
        }
    }
    mapped = true;
}

void Menu::unmap(void) {
    bool was_mapped = mapped;
    unmapSubmenus(NULL);

    mapped = false;

    list<MenuItem *>::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        if ((*it)->focused && (*it)->id == ws->meawm_ng->prefocus) {
            ws->meawm_ng->focusRevertFrom(ws, (*it)->id);
            break;
        }
    }
    rootitem_id = (Window) 0;
    
    if (was_mapped) {
        EventDetail ed;
        WaWindow *ww = NULL;
        XUnmapWindow(ws->display, id);
        if (a_id) {
            AWindowObject *awo = (AWindowObject *)
                ws->meawm_ng->findWin(a_id, WindowType);
            if (awo) ww = awo->getWindow();
        }
        if (special_items) removeSpecialItems();
        it = items.begin();
        for (; it != items.end(); ++it) {
            ed.x = ed.y = INT_MAX;
            ed.x11mod = ed.wamod = 0;
            ed.type = MenuUnmapNotify;
            ws->meawm_ng->eh->evAct(NULL, (*it)->id, &ed);
            if (ww) ww->removeMonitor(*it);
        }
        if (rootitemlnk_id) {
            ed.x = ed.y = INT_MAX;
            ed.x11mod = ed.wamod = 0;
            ed.type = SubmenuUnmapNotify;
            ws->meawm_ng->eh->evAct(NULL, rootitemlnk_id, &ed);
        }
    }
    a_id = (Window) 0;
}

void Menu::unLink(void) {
    rootitem_id = (Window) 0;
}

void Menu::unmapSubmenus(MenuItem *mi) {
    list<MenuItem *>::iterator it = items.begin();
    for (; it != items.end(); ++it)
        if (mi != *it) {
            if ((*it)->submenu &&
                (*it)->submenu->rootitem_id == (*it)->id)
                (*it)->submenu->unmap();
        }
}

void Menu::raiseSubmenus(void) {
    list<MenuItem *>::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        if ((*it)->submenu &&
            (*it)->submenu->rootitem_id == (*it)->id) {
            ws->raiseWindow((*it)->submenu->id, false);
            (*it)->submenu->raiseSubmenus();
        }
    }
}

void Menu::lowerSubmenus(void) {
    list<MenuItem *>::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        if ((*it)->submenu &&
            (*it)->submenu->rootitem_id == (*it)->id) {
            (*it)->submenu->lowerSubmenus();
            ws->lowerWindow((*it)->submenu->id, false);
        }
    }
}


void Menu::unmapTree(void) {
    if (rootitem_id) {
        MenuItem *rootitem = (MenuItem *)
            ws->meawm_ng->findWin(rootitem_id, MenuItemType);
        if (rootitem) rootitem->menu->unmapTree();
        else unmap();
    }
    else unmap();
}

void Menu::focusFirst(void) {
    if (! mapped) return;
    list<MenuItem *>::iterator it = items.begin();
    for (; it != items.end(); ++it)
        if ((*it)->style->focusable) {
            ws->meawm_ng->focusNew((*it)->id);
            break;
        }
}

void Menu::drawOutline(int ox, int oy) {
    if (outline_state) clearOutline();
    outline_state = true;

    int diff_x = ox - x;
    int diff_y = oy - y;
    outl_x = ox;
    outl_y = oy;

    XDrawRectangle(ws->display, ws->id, ws->xor_gc,
                   outl_x, outl_y, width, height);

    list<MenuItem *>::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        if ((*it)->submenu &&
            (*it)->submenu->rootitem_id == (*it)->id) {
            (*it)->submenu->drawOutline((*it)->submenu->x + diff_x,
                                        (*it)->submenu->y + diff_y);
        }
    }
}

void Menu::clearOutline(void) {
    if (! outline_state) return;
    outline_state = false;

    XDrawRectangle(ws->display, ws->id, ws->xor_gc,
                   outl_x, outl_y, width, height);
    
    list<MenuItem *>::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        if ((*it)->submenu &&
            (*it)->submenu->rootitem_id == (*it)->id) {
            (*it)->submenu->clearOutline();
        }
    }
}

void Menu::removeSpecialItems(void) {
    while (special_items) {
        items.back()->unref();
        items.pop_back();
        special_items--;
    }
}

void Menu::addWindowListItems(void) {
    list<WaWindow *>::iterator it = ws->wawindow_list.begin();
    for (; it != ws->wawindow_list.end() &&
             (! ((*it)->wstate & StateTasklistMask)); ++it);
    
    if (it == ws->wawindow_list.end()) return;

    WaWindow *next = NULL, *last = *it;
    while (next != last) {
        it++;
        if (it == ws->wawindow_list.end())
            next = last;
        else
            next = *it;
        if (! (next->wstate & StateTasklistMask)) continue;
        MenuItem *mi = new MenuItem(ws, this, "item");
        mi->commonStyleUpdate();
        delete [] mi->str;
        mi->str = WA_STRDUP(next->name);
        MenuItemAction *mia = new MenuItemAction();
        mia->func = &AWindowObject::windowRaiseFocus;
        mi->actions.push_back(mia);
        
        mi->a_id = next->id;
        
        if (next->wm_icon_image)
            mi->setDynamicGroup(DynamicGroupWmIconImageType,
                                next->wm_icon_image);

        if (next->wm_icon_svg)
            mi->setDynamicGroup(DynamicGroupWmIconSvgType,
                                next->wm_icon_svg);
        
        items.push_back(mi);
        special_items++;
    }
    update();
}

void Menu::addMergeListItems(WaWindow *ww, SpecialMenuType smtype) {
    list<WaWindow *>::iterator it = ws->wawindow_list.begin();
    for (; it != ws->wawindow_list.end() &&
             (! ((*it)->wstate & StateTasklistMask)) &&
             (*it)->id != ww->id && (! (*it)->master); ++it);
    
    if (it == ws->wawindow_list.end()) return;

    WaWindow *next = NULL, *last = *it;
    while (next != last) {
        it++;
        if (it == ws->wawindow_list.end())
            next = last;
        else
            next = *it;
        if (! (next->functions & FunctionMergeMask)) continue;
        if (next->id == ww->id) continue;
        if (next->master) continue;

        MenuItem *mi;
        switch (smtype) {
            case CloneMergeListMenuType:
                mi = new MenuItem(ws, this, "item");
                break;
            case VertMergeListMenuType:
                mi = new MenuItem(ws, this, "item");
                break;
            default:
                mi = new MenuItem(ws, this, "item");
        }
        mi->commonStyleUpdate();
        delete [] mi->str;
        mi->str = WA_STRDUP(next->name);
        mi->a_id = next->id;

        if (next->wm_icon_image)
            mi->setDynamicGroup(DynamicGroupWmIconImageType,
                                next->wm_icon_image);

        if (next->wm_icon_svg)
            mi->setDynamicGroup(DynamicGroupWmIconSvgType,
                                next->wm_icon_svg);
        
        items.push_back(mi);
        special_items++;
    }
    update();
}

void Menu::styleUpdate(bool, bool) {
    update();
}

MenuItemAction::MenuItemAction(void) {
    func = NULL;
    param = NULL;
}

MenuItemAction::~MenuItemAction(void) {
    if (param) delete [] param;
}

bool MenuItemAction::applyAttributes(Parser *parser, Tst<char *> *attr) {
    char *value = parser->attrGetString(attr, "function", NULL);
    if (value) {
        Tst<ActionFunc>::iterator it = actionfunction_tst->find(value);
        if (it != actionfunction_tst->end()) func = *it;
        
        if (! func) {
            parser->warning("unknown action function `%s'\n", value);
            return false;
        }
    } else
        return false;
    
    value = parser->attrGetString(attr, "parameter", NULL);
    if (value) {
        if (param) delete [] param;
        param = WA_STRDUP(value);
    }

    return true;
}

MenuItem::MenuItem(WaScreen *_ws, Menu *m, char *n) :
    DWindowObject(_ws, 0, MenuItemType, m->ids->ref(), n) {
    XSetWindowAttributes attrib_set;
    str = WA_STRDUP("");
    str2 = WA_STRDUP("");
    str_dynamic = str2_dynamic = false;
    submenuname = NULL;
    menu = (Menu *) m->ref();
    focused = mstate = hilited = false;
    submenu = NULL;
    height = width = 0;
    x = y = -1;
    monitor_state = 0;
    a_id = (Window) 0;
    icon_image = NULL;
    icon_svg = NULL;

    int create_mask = CWOverrideRedirect | CWEventMask;

    attrib_set.override_redirect = true;
    attrib_set.event_mask = ButtonPressMask | ButtonReleaseMask |
        EnterWindowMask | LeaveWindowMask | KeyPressMask |
        KeyReleaseMask | FocusChangeMask;
    id = XCreateWindow(ws->display, menu->id, 0, 0, 1, 1, 0,
                       CopyFromParent, CopyFromParent, CopyFromParent,
                       create_mask, &attrib_set);
    ws->meawm_ng->window_table.insert(make_pair(id, this));
    XMapWindow(ws->display, id);

    hidden = false;
    decor_root = menu;
    
    resetStyle();
}

MenuItem::~MenuItem(void) {
    LISTDEL(actions);
    delete [] str;
    delete [] str2;
    if (submenuname) delete [] submenuname;
    if (icon_image) icon_image->unref();
    if (icon_svg) icon_svg->unref();
    ws->meawm_ng->window_table.erase(id);
    ws->meawm_ng->removeFromFocusHistory(id);

    destroySubwindows();
    XDestroyWindow(ws->display, id);
    menu->unref();
}

void MenuItem::applyAttributes(Parser *parser, Tst<char *> *attr) {
    MenuItemAction *mia = new MenuItemAction();
    if (mia->applyAttributes(parser, attr))
        actions.push_back(mia);
    else
        delete mia;    

    char *value = parser->attrGetString(attr, "string", NULL);
    if (value) {
        delete [] str;
        str = preexpand(value, &str_dynamic);
    }
    value = parser->attrGetString(attr, "string2", NULL);
    if (value) {
        delete [] str2;
        str2 = preexpand(value, &str2_dynamic);
    }
    value = parser->attrGetString(attr, "image", NULL);
    if (value) {
        int image_width, image_height;
        unsigned char *rgba = NULL;
        char *filename = smartfile(value, parser->filename);
        if (filename) {
            rgba = read_image_to_rgba(filename, &image_width, &image_height);
            if (rgba) {
                WaSurface *image = ws->rgbaToWaSurface(rgba, image_width,
                                                       image_height);
                if (image) {
                    icon_image = new RenderGroup(ws, NULL);
                    RenderOpImage *image_op = new RenderOpImage();
                    image_op->image = image;
                    image_op->scale = ImageNormalScaleType;
                    icon_image->operations.push_back(image_op);
                    icon_image->_w = image_width;
                    icon_image->_h = image_height;
                    icon_image->_x = icon_image->_y = 0;
                    icon_image->_xu = icon_image->_yu = icon_image->_wu =
                        icon_image->_hu = PXLenghtUnitType;
                }
            }
            delete [] filename;
        }
        
        if (! icon_image)
            parser->warning("failed loading image=%s\n", value);
    }

#ifdef    SVG
    value = parser->attrGetString(attr, "svg", NULL);
    if (value) {
        char *filename = smartfile(value, parser->filename);
        if (filename) {
            FILE *file = fopen(filename, "r");
            if (file) {
                svg_cairo_t *cairo_svg;
                svg_cairo_status status;
                status = svg_cairo_create(&cairo_svg);
                if (status == SVG_CAIRO_STATUS_SUCCESS) {
                    status = svg_cairo_parse_file(cairo_svg, file);
                    if (status == SVG_CAIRO_STATUS_SUCCESS) {
                        RenderOpSvg *svg_op = new RenderOpSvg();
                        svg_op->cairo_svg = cairo_svg; 
                        icon_svg = new RenderGroup(ws, NULL);
                        icon_svg->operations.push_back(svg_op);
                        icon_svg->_w = 16;
                        icon_svg->_h = 16;
                        icon_svg->_x = icon_svg->_y = 0;
                        icon_svg->_xu = icon_svg->_yu = icon_svg->_wu =
                            icon_svg->_hu = PXLenghtUnitType;
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
    }
#endif // SVG
    
    value = parser->attrGetString(attr, "submenu", NULL);
    if (value) {
        submenu = ws->getMenuNamed(submenuname, false);
        if (! submenu) submenuname = WA_STRDUP(value);
    }
    
    value = parser->attrGetString(attr, "monitor_window_state", NULL);
    if (value) {
        Tst<long int>::iterator it = client_window_state_tst->find(value);
        if (it != client_window_state_tst->end())
            monitor_state = *it;
        else
            parser->warning("unknown window state=%s\n", value);
    }
}

void MenuItem::mapSubmenu(char *param, bool force) {
    int workx, worky;
    unsigned int workw, workh;
    int xoff = 0;
    int yoff = 0;
    
    if (! submenu) {
        submenu = ws->getMenuNamed(submenuname, true);
        if (! submenu) return;
    }
    
    if ((! force) && submenu->mapped) return;
    
    ws->getWorkareaSize(&workx, &worky, &workw, &workh);

    if (param) {
        double value, value_return;
        LenghtUnitType unit;
        char *yoff_str = strchr(param, ',');
        if (yoff_str) {
            value = get_double_and_unit(yoff_str + 1, &unit);
            calc_length(value, unit, ws->vdpi, menu->height, &value_return);
            yoff = WA_ROUND(value_return);
            *yoff_str = '\0';
        }
        value = get_double_and_unit(param, &unit);
        calc_length(value, unit, ws->hdpi, menu->width, &value_return);
        xoff = WA_ROUND(value_return);
        if (yoff_str) *yoff_str = ',';
        if (xoff > (int) menu->width) xoff = menu->width;
        if (xoff < - (int) menu->width) xoff = - (int) menu->width;
        if (yoff > (int) menu->height) yoff = menu->height;
        if (yoff < - (int) menu->height) yoff = - (int) menu->height;
    }
    
    int mapx = menu->x + menu->width + xoff;
    int mapy = menu->y + y;

    list<MenuItem *>::iterator it = submenu->items.begin();
    for (; it != submenu->items.end(); ++it)
        if ((*it)->style->focusable) {
            mapy -= (*it)->y;
            break;
        }

    mapy += yoff;
    
    int diff = (mapy + submenu->height) - (workh + worky);
    if (diff > 0) mapy -= diff;
    if (mapy < 0) mapy = 0;
    if ((mapx + (int) submenu->width) > ((int) workw + workx))
        mapx = menu->x - submenu->width - xoff;

    submenu->rootitem_id = submenu->rootitemlnk_id = id;
    submenu->a_id = menu->a_id;
    submenu->map(mapx, mapy);
    if (focused) submenu->focusFirst();
}

void MenuItem::perform(XEvent *e) {
    AWindowObject *awo = NULL;
    if (a_id) {
        awo = (AWindowObject *) ws->meawm_ng->findWin(a_id,
                                                    ANY_ACTION_WINDOW_TYPE);
        if (! awo) return;
    } else if (menu->a_id) {
        awo = (AWindowObject *) ws->meawm_ng->findWin(menu->a_id,
                                                    ANY_ACTION_WINDOW_TYPE);
        if (! awo) return;
    }
    
    Action *action = new Action;
    action->param = NULL;

    if (! actions.empty()) {
        list<MenuItemAction *>::iterator it = actions.begin();
        for (; it != actions.end(); it++) {
            if (action->param) delete [] action->param;
            if ((*it)->param) action->param = WA_STRDUP((*it)->param);
            else action->param = NULL;

            if (awo)
                ((*(awo)).*((*it)->func))(e, action);
            else
                ((*ws).*((*it)->func))(e, action);
        }
    }
    else if (menu->special_menu_type != NoSpecialMenuType &&
             awo && awo->type == WindowType) {
        WaWindow *master = (WaWindow *) awo;
        WaWindow *child = NULL;
        AWindowObject *m_awo = (AWindowObject *)
            ws->meawm_ng->findWin(menu->a_id, ANY_ACTION_WINDOW_TYPE);
        if (m_awo) child = m_awo->getWindow();
        if (child) {
            switch (menu->special_menu_type) {
                case CloneMergeListMenuType:
                    master->merge(child, CloneMergeType);
                    break;
                case VertMergeListMenuType:
                    master->merge(child, VertMergeType);
                    break;
                case HorizMergeListMenuType:
                    master->merge(child, HorizMergeType);
                    break;
                default:
                    break;
            }
        }
    }
    action->unref();
}

void MenuItem::nextItem(void) {
    MenuItem *first = NULL;
    MenuItem *next = NULL;
    bool find_next = false;
    list<MenuItem *>::iterator it = menu->items.begin();
    for (; it != menu->items.end(); ++it) {
        if (*it == this) find_next = true;
        else if ((! find_next) && (! first) && (*it)->style->focusable)
            first = *it;
        else if (find_next && (*it)->style->focusable) {
            next = *it;
            break;
        }
    }
    if (next) ws->meawm_ng->focusNew(next->id);
    else if (first) ws->meawm_ng->focusNew(first->id);
}


void MenuItem::previousItem(void) {
    MenuItem *last = NULL;
    MenuItem *prev = NULL;
    bool find_prev = true;
    list<MenuItem *>::iterator it = menu->items.begin();
    for (; it != menu->items.end(); ++it) {
        if (*it == this) find_prev = false;
        else if (find_prev && (*it)->style->focusable) prev = *it;
        else if ((*it)->style->focusable) last = *it;
    }
    if (prev) ws->meawm_ng->focusNew(prev->id);
    else if (last) ws->meawm_ng->focusNew(last->id);
}

void MenuItem::move(void) {
    XEvent event, *map_ev;
    int px, py, i;
    list<XEvent *> *maprequest_list;
    int nx = menu->x;
    int ny = menu->y;
    Window w;
    unsigned int ui;

    if (ws->meawm_ng->eh->move_resize != EndMoveResizeType) return;
    ws->meawm_ng->eh->move_resize = MoveType;
    
    XQueryPointer(ws->display, ws->id, &w, &w, &px, &py, &i, &i, &ui);
    
    maprequest_list = new list<XEvent *>;
    if (XGrabPointer(ws->display, id, true, ButtonReleaseMask |
                     ButtonPressMask | PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask, GrabModeAsync, GrabModeAsync,
                     menu->ws->id, None, CurrentTime) != GrabSuccess) {
        ws->meawm_ng->eh->move_resize = EndMoveResizeType;
        delete maprequest_list;
        return;
    }
    if (XGrabKeyboard(ws->display, id, true, GrabModeAsync, GrabModeAsync,
                      CurrentTime) != GrabSuccess) {
        ws->meawm_ng->eh->move_resize = EndMoveResizeType;
        delete maprequest_list;
        return;
    }
    for (;;) {
        ws->meawm_ng->eh->eventLoop(
            &ws->meawm_ng->eh->menu_viewport_move_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                while (XCheckTypedWindowEvent(ws->display,
                                              event.xmotion.window,
                                              MotionNotify, &event));
                nx += event.xmotion.x_root - px;
                ny += event.xmotion.y_root - py;
                px = event.xmotion.x_root;
                py = event.xmotion.y_root;
                menu->drawOutline(nx, ny);
                break;
            case LeaveNotify:
            case EnterNotify:
                if (ws->west->id == event.xcrossing.window ||
                    ws->east->id == event.xcrossing.window ||
                    ws->north->id == event.xcrossing.window ||
                    ws->south->id == event.xcrossing.window) {
                    menu->clearOutline();
                    ws->meawm_ng->eh->handleEvent(&event);
                    menu->drawOutline(nx, ny);
                } else if (event.type == EnterNotify &&
                           event.xany.window != id) {
                    int cx, cy;
                    XQueryPointer(ws->display, ws->id, &w, &w, &cx,
                                  &cy, &i, &i, &ui);
                    nx += cx - px;
                    ny += cy - py;
                    px = cx;
                    py = cy;
                    menu->drawOutline(nx, ny);
                }
                break;
            case MapRequest:
                map_ev = new XEvent;
                *map_ev = event;
                maprequest_list->push_front(map_ev); break;
            case ButtonPress:
            case ButtonRelease:
                event.xbutton.window = id;
            case KeyPress:
            case KeyRelease:
                if (event.type == KeyPress || event.type == KeyRelease)
                    event.xkey.window = id;
                menu->clearOutline();
                ws->meawm_ng->eh->handleEvent(&event);
                if (ws->meawm_ng->eh->move_resize != EndMoveResizeType) {
                    menu->drawOutline(nx, ny);
                    break;
                }
                menu->move(nx, ny);
                while (! maprequest_list->empty()) {
                    XPutBackEvent(ws->display, maprequest_list->front());
                    delete maprequest_list->front();
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                XUngrabKeyboard(ws->display, CurrentTime);
                XUngrabPointer(ws->display, CurrentTime);
                return;
        }
    }
}

void MenuItem::moveOpaque(void) {
    XEvent event, *map_ev;
    int px, py, i;
    list<XEvent *> *maprequest_list;
    int nx = menu->x;
    int ny = menu->y;
    Window w;
    unsigned int ui;

    if (ws->meawm_ng->eh->move_resize != EndMoveResizeType) return;
    ws->meawm_ng->eh->move_resize = MoveType;
    
    XQueryPointer(ws->display, ws->id, &w, &w, &px, &py, &i, &i, &ui);
    
    maprequest_list = new list<XEvent *>;
    if (XGrabPointer(ws->display, id, true, ButtonReleaseMask |
                     ButtonPressMask | PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask, GrabModeAsync, GrabModeAsync,
                     menu->ws->id, None, CurrentTime) != GrabSuccess) {
        ws->meawm_ng->eh->move_resize = EndMoveResizeType;
        delete maprequest_list;
        return;
    }
    if (XGrabKeyboard(ws->display, id, true, GrabModeAsync, GrabModeAsync,
                      CurrentTime) != GrabSuccess) {
        ws->meawm_ng->eh->move_resize = EndMoveResizeType;
        delete maprequest_list;
        return;
    }
    for (;;) {
        ws->meawm_ng->eh->eventLoop(
            &ws->meawm_ng->eh->menu_viewport_move_return_mask, &event);
        switch (event.type) {
            case MotionNotify:
                while (XCheckTypedWindowEvent(ws->display,
                                              event.xmotion.window,
                                              MotionNotify, &event));
                nx += event.xmotion.x_root - px;
                ny += event.xmotion.y_root - py;
                px = event.xmotion.x_root;
                py = event.xmotion.y_root;
                menu->move(nx, ny);
                break;
            case LeaveNotify:
            case EnterNotify:
                if (ws->west->id == event.xcrossing.window ||
                    ws->east->id == event.xcrossing.window ||
                    ws->north->id == event.xcrossing.window ||
                    ws->south->id == event.xcrossing.window) {
                    ws->meawm_ng->eh->handleEvent(&event);
                } else if (event.type == EnterNotify &&
                           event.xany.window != id) {
                    int cx, cy;
                    XQueryPointer(ws->display, ws->id, &w, &w, &cx,
                                  &cy, &i, &i, &ui);
                    nx += cx - px;
                    ny += cy - py;
                    px = cx;
                    py = cy;
                    menu->move(nx, ny);
                }
                break;
            case MapRequest:
                map_ev = new XEvent;
                *map_ev = event;
                maprequest_list->push_front(map_ev); break;
            case ButtonPress:
            case ButtonRelease:
                event.xbutton.window = id;
            case KeyPress:
            case KeyRelease:
                if (event.type == KeyPress || event.type == KeyRelease)
                    event.xkey.window = id;
                ws->meawm_ng->eh->handleEvent(&event);
                if (ws->meawm_ng->eh->move_resize != EndMoveResizeType) break;
                while (! maprequest_list->empty()) {
                    XPutBackEvent(ws->display, maprequest_list->front());
                    delete maprequest_list->front();
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                XUngrabKeyboard(ws->display, CurrentTime);
                XUngrabPointer(ws->display, CurrentTime);
                return;
        }
    }
}

void MenuItem::currentPositionAndSize(int *ret_x, int *ret_y,
                                      unsigned int *ret_w,
                                      unsigned int *ret_h) {
    *ret_x = x;
    *ret_y = y;
    *ret_w = width;
    *ret_h = height;
}

void MenuItem::evalWhatToRender(bool, bool size_change,
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

    Pixmap newbg = (menu->sb->surface)? menu->sb->surface->pixmap: None;

    if (sb->bg != newbg) {
        *render_alpha = true;
        sb->bg = newbg;
    }
}

WaSurface *MenuItem::getBgInfo(DWindowObject **return_dwo,
                               int *return_x, int *return_y) {
    *return_x = x;
    *return_y = y;
    *return_dwo = menu;
    return menu->sb->surface;
}
