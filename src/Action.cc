/* Action.cc

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

#ifdef    LIMITS_H
#  include <limits.h>
#endif // LIMITS_H

#ifdef    HAVE_UNISTD_H
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#endif // HAVE_UNISTD_H
    
}

#include "Action.hh"
#include "Meawm_NG.hh"

Tst<ActionFunc> *actionfunction_tst;
Tst<long int>   *client_window_state_tst;
Tst<int>        *event_tst;
Tst<int>        *window_type_tst;

WindowObject::WindowObject(WaScreen *_ws, Window win_id, int win_type) :
    RefCounted<WindowObject>(this) {
    id = win_id;
    type = win_type;
    ws = _ws;
    stacking = NoStackingType;
}

WindowObject::~WindowObject(void) {
    if (stacking != NoStackingType && ws) {
        switch (stacking) {
            case NormalStackingType:
                ws->stacking_list.remove(id);
                break;
            case AlwaysOnTopStackingType:
                ws->aot_stacking_list.remove(id);
                break;
            case AlwaysAtBottomStackingType:
                ws->aab_stacking_list.remove(id);
                break;
            default:
                break;
        }
    }
}
    
void WindowObject::setStacking(StackingType _stacking) {
    if (! ws || (stacking == _stacking)) return;
    switch (stacking) {
        case NormalStackingType:
            ws->stacking_list.remove(id);
            break;
        case AlwaysOnTopStackingType:
            ws->aot_stacking_list.remove(id);
            break;
        case AlwaysAtBottomStackingType:
            ws->aab_stacking_list.remove(id);
            break;
        default:
            break;
    }
    switch (_stacking) {
        case NormalStackingType:
            if (stacking == AlwaysOnTopStackingType)
                ws->stacking_list.push_front(id);
            else
                ws->stacking_list.push_back(id);
            break;
        case AlwaysOnTopStackingType:
            ws->aot_stacking_list.push_back(id);
            break;
        case AlwaysAtBottomStackingType:
            ws->aab_stacking_list.push_front(id);
            break;
        default:
            break;
    }
    stacking = _stacking;
}

ActionList::ActionList(WaScreen *_ws, char *_name) :
    RefCounted<ActionList>(this) {
    ws = _ws;
    name = WA_STRDUP(_name);
}

ActionList::~ActionList(void) {
    clear();
    if (name) delete [] name;
}

void ActionList::inheritContent(ActionList *inherit) {
    list<Action *>::iterator iit = inherit->actionlist.begin();
    for (; iit != inherit->actionlist.end(); iit++) {
        list<Action *>::iterator it = actionlist.begin();
        for (; it != actionlist.end() &&
                 (*it)->priority <= (*iit)->priority; it++);
        actionlist.insert(it, (*iit)->ref());
    }
}

void ActionList::clear(void) {
    while (! actionlist.empty()) {
        actionlist.back()->unref();
        actionlist.pop_back();
    }
}

Action::Action(void) : RefCounted<Action>(this) {
    func = NULL;
    param = NULL;
    type = detail = x11mod = x11nmod = wamod = wanmod = 0;
    replay = false;
    delay = 0;
    timer_id = 0;
    periodic_timer = false;
    priority = 0;
    ws = NULL;
    memset(&pointer.any, 0, sizeof(pointer));
}

Action::~Action(void) {
    if (param) delete [] param;
}

bool Action::match(EventDetail *ed) {
    if (ed->type == type) {
      if (((!detail) || (!ed->detail)) || (detail == ed->detail)) {
          if ((x11mod & ed->x11mod) != x11mod) return false;
          if ((wamod & ed->wamod) != wamod) return false;
          if (x11nmod & ed->x11mod) return false;
          if (wanmod & ed->wamod) return false;
          return true;
        }
    }
    return false;
}

static const struct EventMap {
    char *name;
    int event;
} event_map[] = {
    { "keypress", KeyPress },
    { "keyrelease", KeyRelease },
    { "buttonpress", ButtonPress },
    { "buttonrelease", ButtonRelease },
    { "enternotify", EnterNotify },
    { "leavenotify", LeaveNotify },
    { "maprequest", MapRequest },
    { "focusin", FocusIn },
    { "focusout", FocusOut },
    { "doubleclick", DoubleClick },
    { "windowstatechangenotify", WindowStateChangeNotify },
    { "stateaddnotify", StateAddNotify },
    { "stateremovenotify", StateRemoveNotify },
    { "viewportchangenotify", ViewportChangeNotify },
    { "desktopchangenotify", DesktopChangeNotify },
    { "restartnotify", RestartNotify },
    { "exitnotify", ExitNotify },
    { "createwindownotify", CreateWindowNotify },
    { "destroywindownotify", DestroyWindowNotify },
    { "raisenotify", RaiseNotify },
    { "lowernotify", LowerNotify },
    { "activewindowchangenotify", ActiveWindowChangeNotify },
    { "menumapnotify", MenuMapNotify },
    { "menuunmapnotify", MenuUnmapNotify },
    { "submenumapnotify", SubmenuMapNotify },
    { "submenuunmapnotify", SubmenuUnmapNotify },
    { "dockappaddrequest", DockappAddRequest }
};

void Action::applyAttributes(Parser *parser, Tst<char *> *attr) {
    char *value = parser->attrGetString(attr, "function", NULL);
    if (value) {
        Tst<ActionFunc>::iterator it = actionfunction_tst->find(value);
        if (it != actionfunction_tst->end()) func = *it;
        
        if (! func)
            parser->warning("unknown action function: %s\n", value);
    } else
        parser->warning("required attribute=function is missing\n");

    value = parser->attrGetString(attr, "parameter", NULL);
    if (value) {
        if (param) delete [] param;
        
        if (func == &AWindowObject::setCursor) {
            
#ifdef    XCURSOR
            char *cursorfile = smartfile(value, parser->filename, false);
            if (cursorfile) {
                pointer.cursor = ws->meawm_ng->cursor->getCursor(cursorfile);
                delete [] cursorfile;
            } else
#endif // XCURSOR
                
                pointer.cursor = ws->meawm_ng->cursor->getCursor(value);
        } else
            param = WA_STRDUP(value);
    }

    value = parser->attrGetString(attr, "event", NULL);
    if (value) {
        Tst<int>::iterator it = event_tst->find(value);
        if (it != event_tst->end()) type = *it;
        else
            parser->warning("unknown event type: %s\n", value);
    } else
        parser->warning("required attribute=event is missing\n");

    bool detail_set = false;
    value = parser->attrGetString(attr, "detail", NULL);
    if (type == ButtonPress || type == ButtonRelease) {
        value = parser->attrGetString(attr, "button", value);
        if (value) {
            if (! strncasecmp(value, "anybutton", 9)) {
                detail = AnyButton;
                detail_set = true;
            }
            else if (! strncasecmp(value, "button", 6)) {
                if (value[6] != '\0') {
                    int button = atoi(&value[6]);
                    if (button > 0 && button < 100) {
                        detail = button;
                        detail_set = true;
                    }
                }
            }
        }   
    } else if (type == KeyPress || type == KeyRelease) {
        value = parser->attrGetString(attr, "key", value);
        int keycode = 0;
        if (value) {
            if (! strncmp(value, "0x", 2))
                keycode = strtol(value, NULL, 16);
            else {
                KeySym keysym = XStringToKeysym(value);
                if (keysym == NoSymbol) {
                    parser->warning("unknown keysymbol: %s\n", value);
                    detail = 0;
                    detail_set = true;
                } else {
                    keycode = XKeysymToKeycode(parser->ws->display, keysym);
                }
            }
        } else {
            value = parser->attrGetString(attr, "keycode", value);
            if (value) {
                keycode = strtol(value, NULL, 16);
            }
        }
        if (keycode) {
            detail_set = true;
            if (keycode < parser->ws->meawm_ng->min_key ||
                keycode > parser->ws->meawm_ng->max_key) {
                parser->warning("bad keycode: 0x%x\n", keycode);
                detail = 0;
            } else
                detail = keycode;
        }
    }

    if (value) {
        if (! detail_set)
            detail = (unsigned int) atoi(value);
    }
    
    priority = parser->attrGetInt(attr, "priority", 0);
    delay = parser->attrGetUint(attr, "delay", 0);
    timer_id = parser->attrGetUint(attr, "timer_id", 0);
    periodic_timer = parser->attrGetBool(attr, "periodic_timer", false);
    replay = parser->attrGetBool(attr, "pass_through", false);
}

AWindowObject::AWindowObject(WaScreen *_ws, Window _id, int _type,
                             WaStringMap *sm, char *_window_name) :
    WindowObject(_ws, _id, _type) {
    if (_window_name)
        window_name = WA_STRDUP(_window_name);
    else
        window_name = NULL;
    
    window_state_mask = 0;
    memset(actionlists, 0, sizeof(actionlists));
    memset(default_actionlists, 0, sizeof(default_actionlists));

    ids = NULL;

    resetActionList(sm);
}

AWindowObject::~AWindowObject(void) {
    if (ids) ids->unref();
    if (window_name) delete [] window_name;
    
    for (int i = 0; i < WIN_STATE_LAST; i++) {
        if (actionlists[i]) actionlists[i]->unref();
        if (default_actionlists[i]) default_actionlists[i]->unref();
    }
}

void AWindowObject::resetActionList(WaStringMap *sm) {
    if (ids) ids->unref();
    ids = NULL;
    
    if (sm) ids = sm;
    resetActionList();
}

void AWindowObject::resetActionList(void) {
    list<ActionRegex *> *ls = NULL;
    
    if (! ws) return;

    switch ((type == SubwindowType)?
            ((DWindowObject *) this)->decor_root->type:
            type) {
        case WindowType:
        case WindowFrameType:
            ls = &ws->window_actionlists;
            break;
        case DockHandlerType:
            ls = &ws->dockappholder_actionlists;
            break;
        case DockAppType:
            ls = &ws->dockapp_actionlists;
            break;
        case MenuItemType:
            ls = &ws->menu_actionlists;
            break;
        case RootType:
            ls = &ws->root_actionlists;
            break;
        case EdgeType:
            ls = &ws->screenedge_actionlists;
            break;
    }

    for (int i = 0; i < WIN_STATE_LAST; i++) {
        if (actionlists[i]) actionlists[i]->unref();
        actionlists[i] = NULL;
        if (default_actionlists[i]) default_actionlists[i]->unref();
        default_actionlists[i] = NULL;
    }

    if (ids && ls) {
        list<ActionRegex *>::iterator it = ls->begin();
        for (; it != ls->end(); it++) {
            if ((actionlists[WIN_STATE_PASSIVE] =
                 (*it)->match(ids, WIN_STATE_PASSIVE, window_name))) {
                break;
            }
        }
        for (it = ls->begin(); it != ls->end(); it++) {
            if ((actionlists[WIN_STATE_ACTIVE] =
                 (*it)->match(ids, WIN_STATE_ACTIVE, window_name))) {
                break;
            }
        }
        for (it = ls->begin(); it != ls->end(); it++) {
            if ((actionlists[WIN_STATE_HOVER] =
                 (*it)->match(ids, WIN_STATE_HOVER, window_name))) {
                break;
            }
        }
        for (it = ls->begin(); it != ls->end(); it++) {
            if ((actionlists[WIN_STATE_PRESSED] =
                 (*it)->match(ids, WIN_STATE_PRESSED, window_name))) {
                break;
            }
        }
    }

    if (! actionlists[WIN_STATE_PASSIVE])
        actionlists[WIN_STATE_PASSIVE] = (*ws->actionlists.begin())->ref();

    for (int i = 0; i < WIN_STATE_LAST; i++) {
        if (actionlists[i])
            default_actionlists[i] = actionlists[i]->ref();
    }

    if (type == EdgeType) {
        if (actionlists[WIN_STATE_PASSIVE]->actionlist.empty())
            XUnmapWindow(ws->display, id);
        else
            XMapWindow(ws->display, id);
    }
}

bool AWindowObject::handleEvent(XEvent *event, EventDetail *ed) {
    bool status = false;
    
    ActionList *al = actionlists[
        STATE_FROM_MASK_AND_LIST(window_state_mask, actionlists)]->ref();
    
    list<Action *>::iterator it = al->actionlist.begin();
    list<Action *>::iterator end_it = al->actionlist.end();
    for (; it != end_it; ++it) {
        if ((*it)->match(ed)) {
            Action *action = (*it)->ref();
            if (! action->replay) status = true;
            if (action->delay) {
                Interrupt *i = new Interrupt(action, event, id);
                ws->meawm_ng->timer->addInterrupt(i);
            } else {
                if (action->timer_id &&
                    (! ws->meawm_ng->timer->exitsInterrupt(action->timer_id)))
                    return status;

                ((*this).*(action->func))(event, action);
            }
            action->unref();
        }
    }
    al->unref();
    return status;
}

void AWindowObject::evalState(EventDetail *ed) {
    DWindowObject *dwo = NULL;
    WaWindow *wa = NULL;
    int tmp_state = window_state_mask;
    if (ed) {
        switch (ed->type) {
            case FocusIn:
                if (type == WindowType) {
                    wa = (WaWindow *) this;
                    wa->frame->evalState(ed);
                    tmp_state |= WIN_STATE_ACTIVE_MASK;
                } else {
                    tmp_state |= WIN_STATE_ACTIVE_MASK;

                    if (type & ANY_DECOR_WINDOW_TYPE) {
                        dwo = (DWindowObject *) this;
                        map<int, SubWindowObject *>::iterator it =
                            dwo->subs.begin();
                        for (; it != dwo->subs.end(); it++)
                            ((*it).second)->evalState(ed);
                    }
                }
                break;
            case FocusOut:
                if (type == WindowType) {
                    wa = (WaWindow *) this;
                    wa->frame->evalState(ed);

                    bool merged_has_focus = false;
                    list<WaWindow *>::iterator it;
                    list<WaWindow *>::iterator it_end;
                    if (wa->master) {
                        it_end = wa->master->merged.end();
                        if (wa->master->id == ws->meawm_ng->prefocus) {
                            merged_has_focus = true;
                            it = wa->master->merged.end();
                        } else
                            it = wa->master->merged.begin();
                    } else {
                        it = wa->merged.begin();
                        it_end = wa->merged.end();
                    }
                    for (; it != it_end; it++)
                        if ((*it)->id == ws->meawm_ng->prefocus) {
                            merged_has_focus = true;
                            break;
                        }
                
                    if (! merged_has_focus) {
                        wa->frame->evalState(ed);
                        if (wa->master)
                            wa->master->frame->evalState(ed);
                    }
                    tmp_state &= ~WIN_STATE_ACTIVE_MASK;
                } else if (type == MenuItemType) {
                    if (((MenuItem *) this)->submenu &&
                        ((MenuItem *) this)->submenu->mapped) {
                        tmp_state |= WIN_STATE_ACTIVE_MASK;
                    } else
                        tmp_state &= ~WIN_STATE_ACTIVE_MASK;
                } else {
                    tmp_state &= ~WIN_STATE_ACTIVE_MASK;

                    if (type & ANY_DECOR_WINDOW_TYPE) {
                        dwo = (DWindowObject *) this;
                        map<int, SubWindowObject *>::iterator it =
                            dwo->subs.begin();
                        for (; it != dwo->subs.end(); it++)
                            ((*it).second)->evalState(ed);
                    }
                }
                break;
            case EnterNotify:
                tmp_state |= WIN_STATE_HOVER_MASK;
                break;
            case LeaveNotify:
                tmp_state &= ~WIN_STATE_HOVER_MASK;
                break;
            case ButtonPress:
                tmp_state |= WIN_STATE_PRESSED_MASK;
                break;
            case ButtonRelease:
                tmp_state &= ~WIN_STATE_PRESSED_MASK;
                break;
            case SubmenuMapNotify:
                if (type == MenuItemType)
                    tmp_state |= WIN_STATE_ACTIVE_MASK;
                break;
            case SubmenuUnmapNotify:
                if (type == MenuItemType)
                    if (! ((MenuItem *) this)->focused)
                        tmp_state &= ~WIN_STATE_ACTIVE_MASK;
                break;
            default:
                break;
        }
    }

    ActionList *old_current_actionlist = NULL;
    wa = NULL;
    if (type == WindowType) {
        wa = (WaWindow *) this;
        old_current_actionlist = wa->actionlists[
            STATE_FROM_MASK_AND_LIST(window_state_mask, wa->actionlists)];
    }

    Style *old_current_style = NULL;
    dwo = NULL;
    if (type & ANY_DECOR_WINDOW_TYPE) {
        dwo = (DWindowObject *) this;
        old_current_style = dwo->styles[
            STATE_FROM_MASK_AND_LIST(window_state_mask, dwo->styles)];
    }

    window_state_mask = tmp_state;

    if (wa) {
        ActionList *new_current_actionlist = wa->actionlists[
            STATE_FROM_MASK_AND_LIST(window_state_mask, wa->actionlists)];

        if (new_current_actionlist != old_current_actionlist)
            wa->updateGrabs();
    }
    
    if (dwo) {
        bool pos_change = false;
        bool size_change = false;
        
        Style *new_current_style = dwo->styles[
            STATE_FROM_MASK_AND_LIST(window_state_mask, dwo->styles)];
        
        if (new_current_style != old_current_style) {

            DWIN_RENDER_GET(dwo);

            if (dwo->style) dwo->style->unref();
            dwo->style = (Style *) new_current_style->ref();
            
            dwo->commonStyleUpdate();
            ws->styleDiff(new_current_style, old_current_style,
                          &pos_change, &size_change);
            dwo->styleUpdate(pos_change, size_change);

            DWIN_RENDER_RELEASE(dwo);
            
        }
    }
}

WaWindow *AWindowObject::getWindow(void) {   
    switch (type) {
        case WindowType:
            return (WaWindow *) this;
            break;
        case WindowFrameType:
            return ((WaFrameWindow *) this)->wa;
            break;
        case SubwindowType:
            switch (((DWindowObject *) this)->decor_root->type) {
                case WindowFrameType:
                    return (WaWindow *)
                        ((WaFrameWindow *)
                         ((DWindowObject *) this)->decor_root)->wa;
                default:
                    return (WaWindow *) 0;
            }
            break;
        case MenuItemType: {
            AWindowObject *m_awo = (AWindowObject *)
                ws->meawm_ng->findWin(((MenuItem *) this)->menu->a_id,
                                    ANY_ACTION_WINDOW_TYPE);
            if (m_awo && m_awo->type != MenuItemType)
                return m_awo->getWindow();
        }
        default:
            return (WaWindow *) 0;
    }
}

WaScreen *AWindowObject::getScreen(void) {   
    switch (type) {
        case RootType:
            return (WaScreen *) this;
            break;
        default:
            return (WaScreen *) 0;
    }
}

MenuItem *AWindowObject::getMenuItem(void) {
    switch (type) {
        case MenuItemType:
            return (MenuItem *) this;
            break;
        default:
            return (MenuItem *) 0;
    }
}

DockappHandler *AWindowObject::getDockHandler(void) {
    switch (type) {
        case DockHandlerType:
            return (DockappHandler *) this;
            break;
        case DockAppType:
            return ((Dockapp *) this)->dh;
            break;
        default:
            return (DockappHandler *) 0;
    }
}

Dockapp *AWindowObject::getDockapp(void) {
    switch (type) {
        case DockAppType:
            return (Dockapp *) this;
            break;
        default:
            return (Dockapp *) 0;
    }
}

void AWindowObject::nop(XEvent *, Action *) {}

void AWindowObject::showInfo(XEvent *, Action *a) {
    ws->showInfoMessage(__FUNCTION__, a->param? a->param: "");
}

void AWindowObject::showWarning(XEvent *, Action *a) {
    ws->showWarningMessage(__FUNCTION__, a->param? a->param: "");
}

void AWindowObject::exec(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    if (! fork()) {
        setsid();
        putenv(ws->displaystring);
        execl("/bin/sh", "/bin/sh", "-c", a->param, NULL);
        exit(0);
    }
}

void AWindowObject::doing(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    
    char *str = WA_STRDUP(a->param);
    Tst<char *> *attr;
    
    attr = short_do_string_to_tst(str);
    
    Doing *doing = new Doing(ws);
    doing->applyAttributes(NULL, attr);
    ws->meawm_ng->eh->doings.push_back(doing);

    delete attr;
        
    delete [] str;
}

void AWindowObject::stopTimer(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    int timer_id = (int) (unsigned int) atoi(a->param);
    ws->meawm_ng->timer->removeInterrupt(timer_id);
}

void AWindowObject::waexit(XEvent *, Action *) {    
    quit(EXIT_SUCCESS);
}

void AWindowObject::warestart(XEvent *, Action *a) {
    restart(a->param);
}

void AWindowObject::setActionFile(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    
    if (ws->config.action_file) delete [] ws->config.action_file;
    ws->config.action_file = WA_STRDUP(a->param);
}

void AWindowObject::setStyleFile(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    
    if (ws->config.style_file) delete [] ws->config.style_file;
    ws->config.style_file = WA_STRDUP(a->param);
}

void AWindowObject::setMenuFile(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    
    if (ws->config.menu_file) delete [] ws->config.menu_file;
    ws->config.menu_file = WA_STRDUP(a->param);
}

void AWindowObject::reloadWithActionFile(XEvent *, Action *a) {
    setActionFile(NULL, a);
    ws->reload();
}

void AWindowObject::reloadWithStyleFile(XEvent *, Action *a) {
    setStyleFile(NULL, a);
    ws->reload();
}

void AWindowObject::reloadWithMenuFile(XEvent *, Action *a) {
    setMenuFile(NULL, a);
    ws->reload();
}

void AWindowObject::reload(XEvent *, Action *) {
    ws->reload();
}

void AWindowObject::taskSwitcher(XEvent *, Action *) {
    ws->taskSwitcher();
}

void AWindowObject::nextTask(XEvent *, Action *) {
    ws->nextTask();
}

void AWindowObject::previousTask(XEvent *, Action *) {
    ws->previousTask();
}

void AWindowObject::goToDesktop(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    ws->goToDesktop((unsigned int) atoi(a->param));
}

void AWindowObject::nextDesktop(XEvent *, Action *) {
    ws->nextDesktop();
}

void AWindowObject::previousDesktop(XEvent *, Action *) {
    ws->previousDesktop();
}

void AWindowObject::pointerRelativeWarp(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    ws->pointerRelativeWarp(a->param);
}

void AWindowObject::pointerFixedWarp(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    ws->pointerFixedWarp(a->param);
}

void AWindowObject::viewportLeft(XEvent *, Action *) {
    ws->moveViewport(WestDirection);
}

void AWindowObject::viewportRight(XEvent *, Action *) {
    ws->moveViewport(EastDirection);
}

void AWindowObject::viewportUp(XEvent *, Action *) {
    ws->moveViewport(NorthDirection);
}

void AWindowObject::viewportDown(XEvent *, Action *) {
    ws->moveViewport(SouthDirection);
}

void AWindowObject::viewportRelativeMove(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    ws->viewportRelativeMove(a->param);
}

void AWindowObject::viewportFixedMove(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    ws->viewportFixedMove(a->param);
}

void AWindowObject::startViewportMove(XEvent *e, Action *) {
    e->xany.window = id;
    ws->startViewportMove();
}

void AWindowObject::mapMenu(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    Menu *menu = ws->getMenuNamed(a->param);
    if (menu) menu->map(this, false, false);
}

void AWindowObject::remapMenu(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    Menu *menu = ws->getMenuNamed(a->param);
    if (menu) menu->map(this, false, true);
}

void AWindowObject::mapMenuFocused(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    Menu *menu = ws->getMenuNamed(a->param);
    if (menu) menu->map(this, true, false);
}

void AWindowObject::remapMenuFocused(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    Menu *menu = ws->getMenuNamed(a->param);
    if (menu) menu->map(this, true, true);
}

void AWindowObject::unmapMenu(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    Menu *menu = ws->getMenuNamed(a->param);
    if (menu) menu->unmap();
}

void AWindowObject::setActionList(int state, Action *a) {
    ActionList *new_al = NULL;
    if ((! a) || (! a->param)) new_al = default_actionlists[state];
    if (! new_al) {
        if (a->pointer.actionlist)
            new_al = a->pointer.actionlist;
        new_al = a->pointer.actionlist = ws->getActionListNamed(a->param);
    }
    if (new_al) {
        if (actionlists[state])
            actionlists[state]->unref();
        actionlists[state] = new_al->ref();
        if (type == WindowType) ((WaWindow *) this)->updateGrabs();
    }
}

void AWindowObject::setPassiveActionList(XEvent *, Action *a) {
    setActionList(WIN_STATE_PASSIVE, a);
}

void AWindowObject::setActiveActionList(XEvent *, Action *a) {
    setActionList(WIN_STATE_ACTIVE, a);
}

void AWindowObject::setHoverActionList(XEvent *, Action *a) {
    setActionList(WIN_STATE_HOVER, a);
}

void AWindowObject::setPressedActionList(XEvent *, Action *a) {
    setActionList(WIN_STATE_PRESSED, a);
}

void AWindowObject::defaultPassiveActionList(XEvent *, Action *) {
    setActionList(WIN_STATE_PASSIVE, NULL);
}

void AWindowObject::defaultActiveActionList(XEvent *, Action *) {
    setActionList(WIN_STATE_ACTIVE, NULL);
}

void AWindowObject::defaultHoverActionList(XEvent *, Action *) {
    setActionList(WIN_STATE_HOVER, NULL);
}

void AWindowObject::defaultPressedActionList(XEvent *, Action *) {
    setActionList(WIN_STATE_PRESSED, NULL);
}

void AWindowObject::setStyle(int state, Action *a) {
    DWindowObject *dwo;
    if (type & ANY_DECOR_WINDOW_TYPE)
        dwo = (DWindowObject *) this;
    else {
        WARNING << "not a decorative window" << endl;
        return;
    }

    Style *new_s = NULL;
    if ((! a) || (! a->param)) new_s = dwo->default_styles[state];
    else {
        if (a->pointer.style)
            new_s = a->pointer.style;
        else
            new_s = a->pointer.style = ws->getStyleNamed(a->param);
    }

    if (new_s) {
        Style *old_style = dwo->styles[state];
        Style *old_current_style = dwo->styles[
            STATE_FROM_MASK_AND_LIST(window_state_mask, dwo->styles)];

        DWIN_RENDER_GET(dwo);
        
        dwo->styles[state] = (Style *) new_s->ref();

        bool pos_change = false;
        bool size_change = false;

        Style *new_current_style = dwo->styles[
            STATE_FROM_MASK_AND_LIST(window_state_mask, dwo->styles)];

        if (dwo->style) dwo->style->unref();
        dwo->style = (Style *) new_current_style->ref();

        if (new_current_style != old_current_style) {
            dwo->commonStyleUpdate();
            ws->styleDiff(new_current_style, old_current_style,
                          &pos_change, &size_change);
            dwo->styleUpdate(pos_change, size_change);
        }
        if (old_style)
          old_style->unref();

        DWIN_RENDER_RELEASE(dwo);
    }
}

void AWindowObject::setPassiveStyle(XEvent *, Action *a) {
    setStyle(WIN_STATE_PASSIVE, a);
}

void AWindowObject::setActiveStyle(XEvent *, Action *a) {
    setStyle(WIN_STATE_ACTIVE, a);
}

void AWindowObject::setHoverStyle(XEvent *, Action *a) {
    setStyle(WIN_STATE_HOVER, a);
}

void AWindowObject::setPressedStyle(XEvent *, Action *a) {
    setStyle(WIN_STATE_PRESSED, a);
}

void AWindowObject::defaultPassiveStyle(XEvent *, Action *) {
    setStyle(WIN_STATE_PASSIVE, NULL);
}

void AWindowObject::defaultActiveStyle(XEvent *, Action *) {
    setStyle(WIN_STATE_ACTIVE, NULL);
}

void AWindowObject::defaultHoverStyle(XEvent *, Action *) {
    setStyle(WIN_STATE_HOVER, NULL);
}

void AWindowObject::defaultPressedStyle(XEvent *, Action *) {
    setStyle(WIN_STATE_PRESSED, NULL);
}

void AWindowObject::setCursor(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    Cursor c;
    if (a->pointer.cursor) c = a->pointer.cursor;
    else c = ws->meawm_ng->cursor->getCursor(a->param);
    if (c) XDefineCursor(ws->display, id, c);
}

void AWindowObject::defaultCursor(XEvent *, Action *) {
    DWindowObject *dwo;
    if (! (type & ANY_DECOR_WINDOW_TYPE)) return;
    dwo = (DWindowObject *) this;
    
    if (dwo->style->cursor)
        XDefineCursor(ws->display, id, dwo->style->cursor);
}

void AWindowObject::endMoveResize(XEvent *, Action *) {
    ws->meawm_ng->eh->move_resize = EndMoveResizeType;
}

void AWindowObject::moveFocusToClosestNorthWindow(XEvent *, Action *) {
    ws->findClosestWindow(NorthDirection);
}

void AWindowObject::moveFocusToClosestSouthWindow(XEvent *, Action *) {
    ws->findClosestWindow(SouthDirection);
}

void AWindowObject::moveFocusToClosestWestWindow(XEvent *, Action *) {
    ws->findClosestWindow(WestDirection);
}

void AWindowObject::moveFocusToClosestEastWindow(XEvent *, Action *) {
    ws->findClosestWindow(EastDirection);
}

void AWindowObject::readAdditionalConfig(XEvent *, Action *a) {
    if (! a->param) PARAM_WARNING;
    
    Parser *parser = new Parser(ws);
    parser->pushElementHandler(new CfgElementHandler(parser));

    RENDER_GET;
    
    parser->parseFile(a->param, false);

    RENDER_RELEASE;
    
}

void AWindowObject::focusRoot(XEvent *, Action *) {
    ws->meawm_ng->focusNew(ws->id, false);
}

void AWindowObject::windowRaise(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->raise();
    else ACTION_WARNING;
}

void AWindowObject::windowLower(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->lower();
    else ACTION_WARNING;
}

void AWindowObject::windowFocus(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) ws->meawm_ng->focusNew(w->id);
    else ACTION_WARNING;
}

void AWindowObject::windowRaiseFocus(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) {
        w->raise();
        ws->meawm_ng->focusNew(w->id, true);
    }
    else ACTION_WARNING;
}

void AWindowObject::windowStartMove(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->move(e);
    else ACTION_WARNING;
}

void AWindowObject::windowStartResizeUpRight(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resize(e, EastResizeTypeMask |
                                     NorthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartResizeDownRight(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resize(e, EastResizeTypeMask |
                                     SouthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartResizeRight(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resize(e, EastResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartResizeUpLeft(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resize(e, WestResizeTypeMask |
                                     NorthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartResizeDownLeft(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resize(e, WestResizeTypeMask |
                                     SouthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartResizeLeft(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resize(e, WestResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartResizeUp(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resize(e, NorthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartResizeDown(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resize(e, SouthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartResizeSmart(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resizeSmart(e);
    else ACTION_WARNING;
}

void AWindowObject::windowStartOpaqueMove(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->moveOpaque(e);
    else ACTION_WARNING;
}

void AWindowObject::windowStartOpaqueResizeUpRight(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resizeOpaque(e, EastResizeTypeMask |
                                           NorthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartOpaqueResizeDownRight(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resizeOpaque(e, EastResizeTypeMask |
                                           SouthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartOpaqueResizeRight(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resizeOpaque(e, EastResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartOpaqueResizeUpLeft(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resizeOpaque(e, WestResizeTypeMask |
                                           NorthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartOpaqueResizeDownLeft(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resizeOpaque(e, WestResizeTypeMask |
                                           SouthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartOpaqueResizeLeft(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resizeOpaque(e, WestResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartOpaqueResizeUp(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resizeOpaque(e, NorthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartOpaqueResizeDown(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resizeOpaque(e, SouthResizeTypeMask);
    else ACTION_WARNING;
}

void AWindowObject::windowStartOpaqueResizeSmart(XEvent *e, Action *) {
    e->xany.window = id;
    WaWindow *w;
    if ((w = getWindow())) w->resizeSmartOpaque(e);
    else ACTION_WARNING;
}

void AWindowObject::windowMoveResize(XEvent *e, Action *a) {
    WaWindow *w;
    if (! a->param) PARAM_WARNING;
    if ((w = getWindow())) w->moveResize(a->param);
    else ACTION_WARNING;
}

void AWindowObject::windowMoveResizeVirtual(XEvent *e, Action *a) {
    WaWindow *w;
    if (! a->param) PARAM_WARNING;
    if ((w = getWindow())) w->moveResizeVirtual(a->param);
    else ACTION_WARNING;
}

void AWindowObject::windowMoveToPointer(XEvent *e, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->moveWindowToPointer(e);
    else ACTION_WARNING;
}

void AWindowObject::windowMoveToSmartPlace(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->moveWindowToSmartPlace();
    else ACTION_WARNING;
}

void AWindowObject::windowDesktopMask(XEvent *, Action *a) {
    WaWindow *w;
    if (! a->param) PARAM_WARNING;
    if ((w = getWindow())) w->desktopMask(a->param);
    else ACTION_WARNING;
}

void AWindowObject::windowJoinDesktop(XEvent *, Action *a) {
    WaWindow *w;
    if (! a->param) PARAM_WARNING;
    if ((w = getWindow())) w->joinDesktop(a->param);
    else ACTION_WARNING;
}

void AWindowObject::windowPartDesktop(XEvent *, Action *a) {
    WaWindow *w;
    if (! a->param) PARAM_WARNING;
    if ((w = getWindow())) w->partDesktop(a->param);
    else ACTION_WARNING;
}

void AWindowObject::windowPartCurrentDesktop(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->partCurrentDesktop();
    else ACTION_WARNING;
}

void AWindowObject::windowJoinAllDesktops(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->joinAllDesktops();
    else ACTION_WARNING;
}

void AWindowObject::windowPartAllDesktopsExceptCurrent(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->partAllDesktopsExceptCurrent();
    else ACTION_WARNING;
}

void AWindowObject::windowPartCurrentJoinDesktop(XEvent *, Action *a) {
    WaWindow *w;
    if (! a->param) PARAM_WARNING;
    if ((w = getWindow())) w->partCurrentJoinDesktop(a->param);
    else ACTION_WARNING;
}

void AWindowObject::windowCloneMergeWithWindow(XEvent *, Action *a) {
    WaWindow *w;
    if (! a->param) PARAM_WARNING;
    if ((w = getWindow())) w->cloneMergeWithWindow(a->param);
    else ACTION_WARNING;
}

void AWindowObject::windowVertMergeWithWindow(XEvent *, Action *a) {
    WaWindow *w;
    if (! a->param) PARAM_WARNING;
    if ((w = getWindow())) w->vertMergeWithWindow(a->param);
    else ACTION_WARNING;
}

void AWindowObject::windowHorizMergeWithWindow(XEvent *, Action *a) {
    WaWindow *w;
    if (! a->param) PARAM_WARNING;
    if ((w = getWindow())) w->horizMergeWithWindow(a->param);
    else ACTION_WARNING;
}

void AWindowObject::windowExplode(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->explode();
    else ACTION_WARNING;
}

void AWindowObject::windowMergedToFront(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->toFront();
    else ACTION_WARNING;
}

void AWindowObject::windowUnMerge(XEvent *e, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->unMergeMaster(e);
    else ACTION_WARNING;
}

void AWindowObject::windowSetMergeMode(XEvent *, Action *a) {
    WaWindow *w;
    if (! a->param) PARAM_WARNING;
    if ((w = getWindow())) w->setMergeMode(a->param);
    else ACTION_WARNING;
}

void AWindowObject::windowNextMergeMode(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->nextMergeMode();
    else ACTION_WARNING;
}

void AWindowObject::windowPrevMergeMode(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->prevMergeMode();
    else ACTION_WARNING;
}

void AWindowObject::windowClose(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->close();
    else ACTION_WARNING;
}

void AWindowObject::windowKill(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->kill();
    else ACTION_WARNING;
}

void AWindowObject::windowCloseKill(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->closeKill();
    else ACTION_WARNING;
}

void AWindowObject::windowShade(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->shade();
    else ACTION_WARNING;
}

void AWindowObject::windowUnShade(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->unShade();
    else ACTION_WARNING;
}

void AWindowObject::windowToggleShade(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->toggleShade();
    else ACTION_WARNING;
}

void AWindowObject::windowMaximize(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->maximize();
    else ACTION_WARNING;
}

void AWindowObject::windowUnMaximize(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->unMaximize();
    else ACTION_WARNING;
}

void AWindowObject::windowToggleMaximize(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->toggleMaximize();
    else ACTION_WARNING;
}

void AWindowObject::windowMinimize(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->minimize();
    else ACTION_WARNING;
}

void AWindowObject::windowUnMinimize(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->unMinimize();
    else ACTION_WARNING;
}

void AWindowObject::windowSticky(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->sticky();
    else ACTION_WARNING;
}

void AWindowObject::windowUnSticky(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->unSticky();
    else ACTION_WARNING;
}

void AWindowObject::windowToggleSticky(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->toggleSticky();
    else ACTION_WARNING;
}

void AWindowObject::windowFullscreenOn(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->fullscreenOn();
    else ACTION_WARNING;
}

void AWindowObject::windowFullscreenOff(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->fullscreenOff();
    else ACTION_WARNING;
}

void AWindowObject::windowFullscreenToggle(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->fullscreenToggle();
    else ACTION_WARNING;
}

void AWindowObject::windowToggleMinimize(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->toggleMinimize();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorTitleOn(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorTitleOn();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorTitleOff(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorTitleOff();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorTitleToggle(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorTitleToggle();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorBorderOn(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorBorderOn();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorBorderOff(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorBorderOff();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorBorderToggle(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorBorderToggle();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorHandlesOn(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorHandlesOn();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorHandlesOff(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorHandlesOff();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorHandlesToggle(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorHandlesToggle();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorAllOn(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorAllOn();
    else ACTION_WARNING;
}

void AWindowObject::windowDecorAllOff(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->decorAllOff();
    else ACTION_WARNING;
}

void AWindowObject::windowAlwaysOnTopOn(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->alwaysontopOn();
    else ACTION_WARNING;
}

void AWindowObject::windowAlwaysOnTopOff(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->alwaysontopOff();
    else ACTION_WARNING;
}

void AWindowObject::windowAlwaysOnTopToggle(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->alwaysontopToggle();
    else ACTION_WARNING;
}

void AWindowObject::windowAlwaysAtBottomOn(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->alwaysatbottomOn();
    else ACTION_WARNING;
}

void AWindowObject::windowAlwaysAtBottomOff(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->alwaysatbottomOff();
    else ACTION_WARNING;
}

void AWindowObject::windowAlwaysAtBottomToggle(XEvent *, Action *) {
    WaWindow *w;
    if ((w = getWindow())) w->alwaysatbottomToggle();
    else ACTION_WARNING;
}


void AWindowObject::menuMapSubmenu(XEvent *, Action *a) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->mapSubmenu(a->param, false);
    else ACTION_WARNING;
}

void AWindowObject::menuUnLink(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->unLink();
    else ACTION_WARNING;
}

void AWindowObject::menuRemapSubmenu(XEvent *, Action *a) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->mapSubmenu(a->param, true);
    else ACTION_WARNING;
}

void AWindowObject::menuUnMap(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->unmap();
    else ACTION_WARNING;
}

void AWindowObject::menuUnMapSubmenus(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->unmapSubmenus();
    else ACTION_WARNING;
}

void AWindowObject::menuUnMapOtherSubmenus(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->unmapOtherSubmenus();
    else ACTION_WARNING;
}
void AWindowObject::menuUnMapTree(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->unmapTree();
    else ACTION_WARNING;
}
void AWindowObject::menuAction(XEvent *e, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->perform(e);
    else ACTION_WARNING;
}

void AWindowObject::menuNextItem(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->nextItem();
    else ACTION_WARNING;
}

void AWindowObject::menuPreviousItem(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->previousItem();
    else ACTION_WARNING;
}

void AWindowObject::menuStartMove(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->move();
    else ACTION_WARNING;
}

void AWindowObject::menuStartOpaqueMove(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) mi->moveOpaque();
    else ACTION_WARNING;
}

void AWindowObject::menuRaise(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) {
        ws->raiseWindow(mi->menu->id, false);
        mi->menu->raiseSubmenus();
        ws->restackWindows();
    }
    else ACTION_WARNING;
}

void AWindowObject::menuLower(XEvent *, Action *) {
    MenuItem *mi;
    if ((mi = getMenuItem())) {
        mi->menu->lowerSubmenus();
        ws->lowerWindow(mi->menu->id, false);
        ws->restackWindows();
    }
    else ACTION_WARNING;
}

void AWindowObject::dockappHandlerSetInWorkspace(XEvent *, Action *) {
    DockappHandler *dh;
    if ((dh = getDockHandler())) {
        dh->inworkspace = true;
        dh->update();
    } else ACTION_WARNING;
}

void AWindowObject::dockappHandlerSetNotInWorkspace(XEvent *, Action *) {
    DockappHandler *dh;
    if ((dh = getDockHandler())) {
        dh->inworkspace = false;
        dh->update();
    } else ACTION_WARNING;
}

void AWindowObject::dockappHandlerRaise(XEvent *, Action *) {
    DockappHandler *d;
    if ((d = getDockHandler())) ws->raiseWindow(d->id);
    else ACTION_WARNING;
}

void AWindowObject::dockappHandlerLower(XEvent *, Action *) {
    DockappHandler *d;
    if ((d = getDockHandler())) ws->lowerWindow(d->id);
    else ACTION_WARNING;
}

void AWindowObject::dockappAddToHandler(XEvent *, Action *a) {
    Dockapp *d;
    if ((d = getDockapp())) {
        ws->addDockapp(d, a->param);
    } else ACTION_WARNING;
}

void AWindowObject::dockappSetPrio(XEvent *, Action *a) {
    Dockapp *d;
    if ((d = getDockapp())) {
        if (! a->param) d->prio = 0;
        else d->prio = atoi(a->param);
        if (d->dh) {
            d->dh->dockapp_list.remove(d);
            list<Dockapp *>::iterator dit = d->dh->dockapp_list.begin();
            for (; dit != d->dh->dockapp_list.end() &&
                     (*dit)->prio <= d->prio; dit++);
            d->dh->dockapp_list.insert(dit, d);
            d->dh->update();
        }
        d->prio_set = true;
        ws->net->setDockappPrio(d);
    } else ACTION_WARNING;
}

void AWindowObject::setStackingLayer(XEvent *, Action *a) {
    MenuItem *mi;
    DockappHandler *d;
    if (! a->param) PARAM_WARNING;

    StackingType st = NormalStackingType;
    if (!strcmp(a->param, "alwaysontop"))
        st = AlwaysOnTopStackingType;
    else if (!strcmp(a->param, "alwaysatbottom"))
        st = AlwaysAtBottomStackingType;
    
    if ((mi = getMenuItem())) {
        mi->menu->setStacking(st);
        ws->restackWindows();
    }
    else if ((d = getDockHandler())) {
        d->setStacking(st);
        ws->restackWindows();
    } else ACTION_WARNING;
}

Doing::Doing(WaScreen *_ws) {
    param = NULL;
    wreg = NULL;
    ws = _ws;
    target_type = WindowType;
    func = NULL;
    multiple = true;
}

Doing::~Doing(void) {
    if (param) delete [] param;
    if (wreg) delete wreg;
}

static const struct WindowTypeMap {
    char *name;
    int type;
} window_type_map[] = {
    { "root", RootType },
    { "window", WindowType },
    { "menu", MenuType },
    { "menuitem", MenuItemType },
    { "dockappholder", DockHandlerType },
    { "dockapp", DockAppType },
    { "screenedge", EdgeType }
};

void Doing::applyAttributes(Parser *parser, Tst<char *> *attr) {
    char *value = parser->attrGetString(attr, "target", NULL);
    if (value) {
        Tst<int>::iterator it = window_type_tst->find(value);
        if (it != window_type_tst->end()) target_type = *it;
    }
    value = parser->attrGetString(attr, "function", NULL);
    if (value) {
        Tst<ActionFunc>::iterator it = actionfunction_tst->find(value);
        if (it != actionfunction_tst->end()) func = *it;
    }

    value = parser->attrGetString(attr, "parameter", NULL);
    if (value) {
        param = WA_STRDUP(value);
    }
    
    value = parser->attrGetString(attr, "window", NULL);
    wreg = new WindowRegex(WIN_STATE_LAST, value);
    parser->attrGetWindowregexContent(attr, wreg);

    value = parser->attrGetString(attr, "applies", NULL);
    if (value)
        if (! strcasecmp(value, "firstfound"))
            multiple = false;
}

void Doing::envoke(void) {
    if (func) {
        list<AWindowObject *> awos;
        ws->getRegexTargets(wreg, target_type, multiple, &awos);
        if (! awos.empty()) {
            Action *action = new Action;
            if (param) action->param = param;
            else action->param = NULL;
            
            list<AWindowObject *>::iterator ait = awos.begin();
            for (; ait != awos.end(); ait++)
                ((*(*ait)).*(func))(NULL, action);

            action->param = NULL;
            action->unref();
            
            awos.clear();
        }
    }
}

Tst<char *> *short_do_string_to_tst(char *s) {
    Tst<char *> *tst = new Tst<char *>;
    
    if (! s || *s == '\0') return tst;

    for (;;) {
        char *name = s;
        char *value = "";
        for (; *s != '\0' && *s != '='; s++);
        if (*s == '\0') break;
        else *s++ = '\0';
        value = s;
        tst->insert(name, value);
        for (; *s != '\0' && *s != ':'; s++);
        if (*s == '\0') break;
        else *s++ = '\0';
    }

    return tst;
}

static const struct ActionFunctionMap {
    char *name;
    ActionFunc function;
} actionfunction_map[] = {  
    { "nop", &AWindowObject::nop },
    { "showinfo", &AWindowObject::showInfo },
    { "showwarning", &AWindowObject::showWarning },
    { "exec", &AWindowObject::exec },
    { "do", &AWindowObject::doing },
    { "stoptimer", &AWindowObject::stopTimer },
    { "exit", &AWindowObject::waexit },
    { "restart", &AWindowObject::warestart },
    { "setactionfile", &AWindowObject::setActionFile },
    { "setstylefile", &AWindowObject::setStyleFile },
    { "setmenufile", &AWindowObject::setMenuFile },
    { "reloadwithactionfile", &AWindowObject::reloadWithActionFile },
    { "reloadwithstylefile", &AWindowObject::reloadWithStyleFile },
    { "reloadwithmenufile", &AWindowObject::reloadWithMenuFile },
    { "reload", &AWindowObject::reload },
    { "taskswitcher", &AWindowObject::taskSwitcher },
    { "nexttask", &AWindowObject::nextTask },
    { "previoustask", &AWindowObject::previousTask },
    { "gotodesktop", &AWindowObject::goToDesktop },
    { "nextdesktop", &AWindowObject::nextDesktop },
    { "previousdesktop", &AWindowObject::previousDesktop },
    { "pointerrelativewarp", &AWindowObject::pointerRelativeWarp },
    { "pointerfixedwarp", &AWindowObject::pointerFixedWarp },
    { "viewportleft", &AWindowObject::viewportLeft },
    { "viewportright", &AWindowObject::viewportRight },
    { "viewportup", &AWindowObject::viewportUp },
    { "viewportdown", &AWindowObject::viewportDown },
    { "viewportrelativemove", &AWindowObject::viewportRelativeMove },
    { "viewportfixedmove", &AWindowObject::viewportFixedMove },
    { "startviewportmove", &AWindowObject::startViewportMove },
    { "setpassiveactionlist", &AWindowObject::setPassiveActionList },
    { "setactiveactionlist", &AWindowObject::setActiveActionList },
    { "sethoveractionlist", &AWindowObject::setHoverActionList },
    { "setpressedactionlist", &AWindowObject::setPressedActionList },
    { "defaultpassiveactionlist", &AWindowObject::defaultPassiveActionList },
    { "defaultactiveactionlist", &AWindowObject::defaultActiveActionList },
    { "defaulthoveractionlist", &AWindowObject::defaultHoverActionList },
    { "defaultpressedactionlist", &AWindowObject::defaultPressedActionList },
    { "setpassivestyle", &AWindowObject::setPassiveStyle },
    { "setactivestyle", &AWindowObject::setActiveStyle },
    { "sethoverstyle", &AWindowObject::setHoverStyle },
    { "setpressedstyle", &AWindowObject::setPressedStyle },
    { "defaultpassivestyle", &AWindowObject::defaultPassiveStyle },
    { "defaultactivestyle", &AWindowObject::defaultActiveStyle },
    { "defaulthoverstyle", &AWindowObject::defaultHoverStyle },
    { "defaultpressedstyle", &AWindowObject::defaultPressedStyle },
    { "setcursor", &AWindowObject::setCursor },
    { "defaultcursor", &AWindowObject::defaultCursor },
    { "mapmenu", &AWindowObject::mapMenu },
    { "remapmenu", &AWindowObject::remapMenu },
    { "mapmenufocused", &AWindowObject::mapMenuFocused },
    { "remapmenufocused", &AWindowObject::remapMenuFocused },
    { "unmapmenu", &AWindowObject::unmapMenu },
    { "endmoveresize", &AWindowObject::endMoveResize },
    { "movefocustoclosestnorthwindow",
      &AWindowObject::moveFocusToClosestNorthWindow },
    { "movefocustoclosestsouthwindow",
      &AWindowObject::moveFocusToClosestSouthWindow },
    { "movefocustoclosestwestwindow",
      &AWindowObject::moveFocusToClosestWestWindow },
    { "movefocustoclosesteastwindow",
      &AWindowObject::moveFocusToClosestEastWindow },
    { "readadditionalconfig", &AWindowObject::readAdditionalConfig },
    { "focusroot", &AWindowObject::focusRoot },

    { "windowraise", &AWindowObject::windowRaise },
    { "windowlower", &AWindowObject::windowLower },
    { "windowfocus", &AWindowObject::windowFocus },
    { "windowraisefocus", &AWindowObject::windowRaiseFocus },
    { "windowstartmove", &AWindowObject::windowStartMove },
    { "windowstartresizedownright",
      &AWindowObject::windowStartResizeDownRight },
    { "windowstartresizeupright", &AWindowObject::windowStartResizeUpRight },
    { "windowstartresizeright", &AWindowObject::windowStartResizeRight },
    { "windowstartresizedownleft", &AWindowObject::windowStartResizeDownLeft },
    { "windowstartresizeupleft", &AWindowObject::windowStartResizeUpLeft },
    { "windowstartresizeleft", &AWindowObject::windowStartResizeLeft },
    { "windowstartresizedown", &AWindowObject::windowStartResizeDown },
    { "windowstartresizeup", &AWindowObject::windowStartResizeUp },
    { "windowstartresizesmart", &AWindowObject::windowStartResizeSmart },
    { "windowstartopaquemove", &AWindowObject::windowStartOpaqueMove },
    { "windowstartopaqueresizedownright",
      &AWindowObject::windowStartOpaqueResizeDownRight },
    { "windowstartopaqueresizeupright",
      &AWindowObject::windowStartOpaqueResizeUpRight },
    { "windowstartopaqueresizeright",
      &AWindowObject::windowStartOpaqueResizeRight },
    { "windowstartopaqueresizedownleft",
      &AWindowObject::windowStartOpaqueResizeDownLeft },
    { "windowstartopaqueresizeupleft",
      &AWindowObject::windowStartOpaqueResizeUpLeft },
    { "windowstartopaqueresizeleft",
      &AWindowObject::windowStartOpaqueResizeLeft },
    { "windowstartopaqueresizedown",
      &AWindowObject::windowStartOpaqueResizeDown },
    { "windowstartopaqueresizeup", &AWindowObject::windowStartOpaqueResizeUp },
    { "windowstartopaqueresizesmart",
      &AWindowObject::windowStartOpaqueResizeSmart },
    { "windowmoveresize", &AWindowObject::windowMoveResize },
    { "windowmoveresizevirtual", &AWindowObject::windowMoveResizeVirtual },
    { "windowmovetopointer", &AWindowObject::windowMoveToPointer },
    { "windowmovetosmartplace", &AWindowObject::windowMoveToSmartPlace },
    { "windowdesktopmask", &AWindowObject::windowDesktopMask },
    { "windowjoindesktop", &AWindowObject::windowJoinDesktop },
    { "windowpartdesktop", &AWindowObject::windowPartDesktop },
    { "windowpartcurrentdesktop", &AWindowObject::windowPartCurrentDesktop },
    { "windowjoinalldesktops", &AWindowObject::windowJoinAllDesktops },
    { "windowpartalldesktopsexceptcurrent",
      &AWindowObject::windowPartAllDesktopsExceptCurrent },
    { "windowpartcurrentjoindesktop",
      &AWindowObject::windowPartCurrentJoinDesktop },
    { "windowclonemergewithwindow",
      &AWindowObject::windowCloneMergeWithWindow },
    { "windowvertmergewithwindow", &AWindowObject::windowVertMergeWithWindow },
    { "windowhorizmergewithwindow",
      &AWindowObject::windowHorizMergeWithWindow },
    { "windowexplode", &AWindowObject::windowExplode },
    { "windowmergedtofront", &AWindowObject::windowMergedToFront },
    { "windowunmerge", &AWindowObject::windowUnMerge },
    { "windowsetmergemode", &AWindowObject::windowSetMergeMode },
    { "windownextmergemode", &AWindowObject::windowNextMergeMode },
    { "windowprevmergemode", &AWindowObject::windowPrevMergeMode },
    { "windowclose", &AWindowObject::windowClose },
    { "windowkill", &AWindowObject::windowKill },
    { "windowclosekill", &AWindowObject::windowCloseKill },
    { "windowshade", &AWindowObject::windowShade },
    { "windowunshade", &AWindowObject::windowUnShade },
    { "windowtoggleshade", &AWindowObject::windowToggleShade },
    { "windowmaximize", &AWindowObject::windowMaximize },
    { "windowunmaximize", &AWindowObject::windowUnMaximize },
    { "windowtogglemaximize", &AWindowObject::windowToggleMaximize },
    { "windowminimize", &AWindowObject::windowMinimize },
    { "windowunminimize", &AWindowObject::windowUnMinimize },
    { "windowsticky", &AWindowObject::windowSticky },
    { "windowunsticky", &AWindowObject::windowUnSticky },
    { "windowtogglesticky", &AWindowObject::windowToggleSticky },
    { "windowfullscreenon", &AWindowObject::windowFullscreenOn },
    { "windowfullscreenoff", &AWindowObject::windowFullscreenOff },
    { "windowfullscreenToggle", &AWindowObject::windowFullscreenToggle },
    { "windowtoggleminimize", &AWindowObject::windowToggleMinimize },
    { "windowdecortitleon", &AWindowObject::windowDecorTitleOn },
    { "windowdecortitleoff", &AWindowObject::windowDecorTitleOff },
    { "windowdecortitletoggle", &AWindowObject::windowDecorTitleToggle },
    { "windowdecorborderon", &AWindowObject::windowDecorBorderOn },
    { "windowdecorborderoff", &AWindowObject::windowDecorBorderOff },
    { "windowdecorbordertoggle", &AWindowObject::windowDecorBorderToggle },
    { "windowdecorhandleson", &AWindowObject::windowDecorHandlesOn },
    { "windowdecorhandlesoff", &AWindowObject::windowDecorHandlesOff },
    { "windowdecorhandlestoggle", &AWindowObject::windowDecorHandlesToggle },
    { "windowdecorallon", &AWindowObject::windowDecorAllOn },
    { "windowdecoralloff", &AWindowObject::windowDecorAllOff },
    { "windowalwaysontopon", &AWindowObject::windowAlwaysOnTopOn },
    { "windowalwaysontopoff", &AWindowObject::windowAlwaysOnTopOff },
    { "windowalwaysontoptoggle", &AWindowObject::windowAlwaysOnTopToggle },
    { "windowalwaysatbottomon", &AWindowObject::windowAlwaysAtBottomOn },
    { "windowalwaysatbottomoff", &AWindowObject::windowAlwaysAtBottomOff },
    { "windowalwaysatbottomtoggle",
      &AWindowObject::windowAlwaysAtBottomToggle },
    
    { "menuunlink", &AWindowObject::menuUnLink },
    { "menumapsubmenu", &AWindowObject::menuMapSubmenu },
    { "menuunlink", &AWindowObject::menuUnLink },
    { "menumapsubmenu", &AWindowObject::menuMapSubmenu },
    { "menuremapsubmenu", &AWindowObject::menuRemapSubmenu },
    { "menuunmap", &AWindowObject::menuUnMap },
    { "menuunmapsubmenus", &AWindowObject::menuUnMapSubmenus },
    { "menuunmapothersubmenus", &AWindowObject::menuUnMapOtherSubmenus },
    { "menuunmaptree", &AWindowObject::menuUnMapTree },
    { "menuaction", &AWindowObject::menuAction },
    { "menunextitem", &AWindowObject::menuNextItem },
    { "menupreviousitem", &AWindowObject::menuPreviousItem },
    { "menuraise", &AWindowObject::menuRaise },
    { "menulower", &AWindowObject::menuLower },
    { "menustartmove", &AWindowObject::menuStartMove },
    { "menustartopaquemove", &AWindowObject::menuStartOpaqueMove },
    { "menusetstacking", &AWindowObject::setStackingLayer },
    
    { "dockappholdersetinworkspace",
      &AWindowObject::dockappHandlerSetInWorkspace },
    { "dockappholdersetnotinworkspace",
      &AWindowObject::dockappHandlerSetNotInWorkspace },
    { "dockappholderraise", &AWindowObject::dockappHandlerRaise },
    { "dockappholderlower", &AWindowObject::dockappHandlerLower },
    { "dockappholdersetstacking", &AWindowObject::setStackingLayer },
    
    { "dockappaddtoholder", &AWindowObject::dockappAddToHandler },
    { "dockappsetpriority", &AWindowObject::dockappSetPrio }
};

static const struct ClientWindowStateMap {
    char *name;
    long int state;
} client_window_state_map[] = {
    { "windowstatemove", StateMoveMask },
    { "windowstateresize", StateResizeMask },
    { "windowstatesticky", StateStickyMask },
    { "windowstateshaded", StateShadedMask },
    { "windowstatemaximized", StateMaximizedMask },
    { "windowstateminimized", StateMinimizedMask },
    { "windowstatefullscreen", StateFullscreenMask },
    { "windowstatedecortitle", StateDecorTitleMask },
    { "windowstatedecorborder", StateDecorBorderMask },
    { "windowstatedecorhandles", StateDecorHandlesMask },
    { "windowstatedecorall", StateDecorAllMask },
    { "windowstatealwaysontop", StateAlwaysOnTopMask },
    { "windowstatealwaysatbottom", StateAlwaysAtBottomMask },
    { "windowstatetransient", StateTransientMask },
    { "windowstategroupleader", StateGroupLeaderMask },
    { "windowstategroupmember", StateGroupMemberMask },
    { "windowstateurgent", StateUrgentMask },
    { "windowstatefocusable", StateFocusableMask },
    { "windowstatetasklist", StateTasklistMask },
    { "windowstateewmhdesktop", StateEwmhDesktopMask },
    { "windowstateewmhdock", StateEwmhDockMask },
    { "windowstateewmhtoolbar", StateEwmhToolbarMask },
    { "windowstateewmhmenu", StateEwmhMenuMask },
    { "windowstateewmhsplash", StateEwmhSplashMask },
    { "windowstateewmhdialog", StateEwmhDialogMask },
    { "windowstateusertime", StateUserTimeMask },
    { "windowstatepositioned", StatePositionedMask }
};

void action_create_tsts(void) {
    int size, i;
    
    actionfunction_tst = new Tst<ActionFunc>;
    size = sizeof(actionfunction_map) / sizeof(ActionFunctionMap);
    for (i = 0; i < size; i++)
        actionfunction_tst->insert(actionfunction_map[i].name,
                                   actionfunction_map[i].function);

    client_window_state_tst = new Tst<long int>;
    size = sizeof(client_window_state_map) / sizeof(ClientWindowStateMap);
    for (i = 0; i < size; i++)
        client_window_state_tst->insert(client_window_state_map[i].name,
                                        client_window_state_map[i].state);

    event_tst = new Tst<int>;
    size = sizeof(event_map) / sizeof(EventMap);
    for (i = 0; i < size; i++)
        event_tst->insert(event_map[i].name, event_map[i].event);

    window_type_tst = new Tst<int>;
    size = sizeof(window_type_map) / sizeof(WindowTypeMap);
    for (i = 0; i < size; i++)
        window_type_tst->insert(window_type_map[i].name,
                                window_type_map[i].type);
}
