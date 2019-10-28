/* Parser.cc

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
#  include <stdarg.h>
#endif // STDC_HEADERS

#ifdef    HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif // HAVE_SYS_TYPES_H    
    
#ifdef    HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif // HAVE_SYS_STAT_H
    
#ifdef    HAVE_UNISTD_H
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#endif // HAVE_SYS_WAIT_H

#ifdef    HAVE_FCNTL_H
#  include <fcntl.h>
#endif // HAVE_FCNTL_H
    
}

#include "Parser.hh"
#include "Util.hh"
#include "Action.hh"
#include "Style.hh"
#include "Screen.hh"

Tst<int>           *win_state_tst;
Tst<long int>      *x11_modifier_tst;
Tst<long int>      *wa_modifier_tst;
Tst<int>           *dynamic_try_tst;
Tst<ParserElement> *cfg_element_tst;
Tst<ParserElement> *actionlist_element_tst;
Tst<ParserElement> *action_element_tst;
Tst<ParserElement> *menu_element_tst;
Tst<ParserElement> *menuitem_element_tst;
Tst<ParserElement> *rendergroup_element_tst;
Tst<ParserElement> *renderopdynamic_element_tst;
Tst<ParserElement> *renderpattern_element_tst;
Tst<ParserElement> *renderpath_element_tst;
Tst<ParserElement> *rendertext_element_tst;

static void parser_start_element(void *userdata, const XML_Char *name,
                                 const XML_Char **attr) {
    Parser *parser;
    ElementHandler *handler;
    ParserElement element;

    parser = (Parser *) userdata;

    if (parser->unknown_element_depth) {
        parser->unknown_element_depth++;
        return;
    }

    if (! parser->meawm_ng_element_found) {
        if ( ! strcmp((char *) name, "meawm_ng")) {
            Tst<char *> *attr_tst = new Tst<char *>(attr);
            char *version = parser->attrGetString(attr_tst, "version", NULL);
            if (version) {
                if (strcmp(version, VERSION) < 0)
                    parser->warning("detected an old configuration file "
                                    "version %s. meawm_ng version is " VERSION
                                    "\n", version);
                else if (strcmp(version, VERSION) > 0)
                    parser->warning("detected an old version of meawm_ng "
                                    VERSION ". configuration file version is "
                                    "%s\n", version);
            }
            parser->meawm_ng_element_found = true;
            delete attr_tst;
            return;
        }
    }

    handler = parser->element_handler_stack.back();
    if (handler) {
        element = handler->elementMap(name);
        if (element == ParserElementNone) {
            parser->unknown_element_depth++;
        } else {
            Tst<char *> *attr_trie = new Tst<char *>(attr);
            if (! handler->startElement(element, name, attr_trie))
                parser->unknown_element_depth++;
            delete attr_trie;
        }
    }
}

static void parser_end_element(void *userdata, const XML_Char *name) {
    Parser *parser;
    ElementHandler *handler;

    parser = (Parser *) userdata;

    if (parser->unknown_element_depth) {
        parser->unknown_element_depth--;
        return;
    }

    handler = parser->element_handler_stack.back();
    if (handler) {
        handler->endElement(name);
        parser->popElementHandler();
    }
}

static void parser_characters(void *userdata, const XML_Char *chars, int len) {
    Parser *parser;
    ElementHandler *handler;

    parser = (Parser *) userdata;
    
    handler = parser->element_handler_stack.back();

    if (! len || ! handler) return;

    handler->characters(chars, len);
}

static void parser_start_doctype_decl(void *userdata,
                                      const XML_Char *doctypename,
                                      const XML_Char *,
                                      const XML_Char *, int) {
    Parser *parser = (Parser *) userdata;

    if (strcmp((char *) doctypename, "meawm_ng") != 0)
        parser->warning("invalid doctype: %s", doctypename);
}

static void parser_end_doctype_decl(void *) {}

StringBuffer::~StringBuffer(void) {
    while (! strbuf.empty()) {
        delete [] strbuf.back();
        strbuf.pop_back();
    }
}

void StringBuffer::addString(char *chars, int len) {
    if (len < 1) return;
    strbuf_length += (len);
    char *substr = new char[len + 1];
    strncpy(substr, chars, len);
    substr[len] = '\0';
    strbuf.push_back(substr);
}

char *StringBuffer::getString(void) {
    char *str = new char[strbuf_length + 1];
    char *str_ptr = str;
    str[strbuf_length] = '\0';
    list<char *>::iterator it = strbuf.begin();
    for (; it != strbuf.end(); it++)
        str_ptr += sprintf(str_ptr, "%s", *it);
    return str;
}

Parser::Parser(WaScreen *_ws, int _include_depth, bool cr_xml_parser) {
    if (cr_xml_parser) {
        xml_parser = XML_ParserCreate("UTF-8");
        if (! xml_parser) {
            ERROR_FUNC << "failed to create xml parser context" << endl;
            exit(1);
        }
        XML_SetDoctypeDeclHandler(xml_parser,
                                  parser_start_doctype_decl,
                                  parser_end_doctype_decl);
        XML_SetElementHandler(xml_parser,
                              parser_start_element,
                              parser_end_element);
        XML_SetCharacterDataHandler(xml_parser, parser_characters);
        XML_SetUserData(xml_parser, this);
    } else
        xml_parser = NULL;
    
    include_depth = _include_depth;
    unknown_element_depth = 0;
    filename = WA_STRDUP("");
    meawm_ng_element_found = false;
    ws = _ws;
}

Parser::~Parser(void) {
    char *warnings = strbuf.getString();
    if (*warnings != '\0')
        ws->showWarningMessage(__FUNCTION__, warnings);

    delete [] warnings;
    
    if (xml_parser)
        XML_ParserFree(xml_parser);
    
    while (! element_handler_stack.empty()) {
        element_handler_stack.back()->unref();
        element_handler_stack.pop_back();
    }
    delete [] filename;

    Tst<char *>::iterator it = constants.begin();
    for (; it != constants.end(); it++)
        delete [] *it;
}

void Parser::pushElementHandler(ElementHandler *handler) {
    element_handler_stack.push_back(handler);
}

void Parser::popElementHandler(void) {
    if (! element_handler_stack.empty()) {
        element_handler_stack.back()->unref();
        element_handler_stack.pop_back();
    }
}

void Parser::setFilename(const char *name) {
    delete [] filename;
    filename = WA_STRDUP((char *) name);
}

bool Parser::parseChunk(const char *data, int len) {
    if (! xml_parser) return false;    
    
    if (! XML_Parse(xml_parser, data, len, len == 0)) {
        warning("%s", XML_ErrorString(XML_GetErrorCode(xml_parser)));
        return false;
    }
        
    return true;
}

bool Parser::parseChunkEnd(void) {
    if (! xml_parser) return false;

    if (! XML_Parse(xml_parser, NULL, 0, true)) {
        warning("%s", XML_ErrorString(XML_GetErrorCode(xml_parser)));
        return false;
    }
    
    return true;
}

bool Parser::parseFile(const char *file, bool ignore_missing) {
    if (! xml_parser) return false;
    
    FILE *fd = fopen(file, "r");   
    if (fd == NULL) {
        if (! ignore_missing) {
            warning("%s: %s", file, strerror(errno));
        }
    } else {
        int len;
        delete [] filename;
        filename = WA_STRDUP((char *) file);
        do {
            void *buf = XML_GetBuffer(xml_parser, PARSE_BUFFER_SIZE);
            if (! buf) {
                WARNING_FUNC << "cannot get parse buffer" << endl;
                return false;
            }
            len = fread(buf, 1, PARSE_BUFFER_SIZE, fd);
            if (len < 0) {
                WARNING_FUNC << "failed reading config file" << endl;
                return false;
            }
            if (! XML_ParseBuffer(xml_parser, len, len == 0)) {
                warning("%s", XML_ErrorString(XML_GetErrorCode(xml_parser)));
                return false;
            }
        } while (len != 0);

        fclose(fd);
        
        return true;
    }
    return false;
}

bool Parser::parseCommand(const char *command, bool ignore_missing) {
    int m_pipe[2];
    int pid, waitstatus;
    struct sigaction action;
    FILE *fd;
    bool status = false;
    
    if (! xml_parser) return false;    

    if (pipe(m_pipe) < 0) {
        WARNING_FUNC; perror("pipe");
        goto bail;
    }
        
    action.sa_handler = SIG_DFL;
    action.sa_mask = sigset_t();
    action.sa_flags = 0;
    sigaction(SIGCHLD, &action, NULL);
    pid = fork();
    if (pid == 0) {
        dup2(m_pipe[1], STDOUT_FILENO);
        close(m_pipe[0]);
        close(m_pipe[1]);
        putenv(meawm_ng_pathenv);
        if (execl("/bin/sh", "/bin/sh", "-c", command, NULL) < 0)
            WARNING_FUNC; perror("/bin/sh");
        close(STDOUT_FILENO);
        exit(127);
    }
    close(m_pipe[1]);

    fd = fdopen(m_pipe[0], "r");
        
    if (fd == NULL) {
        WARNING_FUNC; perror("fdopen");
    } else {
        int len;
            
        if (filename) delete [] filename;
        filename = WA_STRDUP("STDOUT");
        
        do {
            void *buf = XML_GetBuffer(xml_parser, PARSE_BUFFER_SIZE);
            if (! buf) {
                WARNING_FUNC << "cannot get parse buffer" << endl;
                goto bail;
            }
            len = fread(buf, 1, PARSE_BUFFER_SIZE, fd);
            if (len < 0) {
                WARNING_FUNC << "failed reading command output" << endl;
                goto bail;
            }
            if (! XML_ParseBuffer(xml_parser, len, len == 0)) {
                warning("%s", XML_ErrorString(XML_GetErrorCode(xml_parser)));
                goto bail;
            }
        } while (len != 0);
        
        action.sa_handler = signalhandler;
        action.sa_flags = SA_NOCLDSTOP | SA_NODEFER;
        sigaction(SIGCHLD, &action, NULL);
        
        if (waitpid(pid, &waitstatus, 0) == -1) {
            WARNING_FUNC; perror("waitpid");
        } else if (waitstatus) {
            if (! ignore_missing) {
                warning("execution failed");
            }
        }
        
        status = true;
    }
    
 bail:
    return status;
}

void Parser::warning(const char *msg, ...) {
    va_list args;
    char message[8192];
    char linenr[32];
    char *line;
    int len;
    
    va_start(args, msg);
    vsnprintf(message, 8192, msg, args);
    va_end(args);

    snprintf(linenr, 32, "%d", XML_GetCurrentLineNumber(xml_parser));
    len = strlen(filename) + strlen(linenr) + strlen(message) + 5;
    line = new char[len + 1];
    sprintf(line, "%s: %s: %s", filename, linenr, message);
    if (strbuf.strbuf.empty()) {
        char *error_head = "The following parsing errors occured:\n";
        strbuf.addString(error_head, strlen(error_head));
    }
    strbuf.addString(line, len);
    
    delete [] line;
}

char *Parser::attrGetString(Tst<char *> *attr, char *name,
                            char *default_value) {
    if (attr) {
        Tst<char *>::iterator it = attr->find(name);
        if (it != attr->end()) return attrValue(*it);
    }
    return default_value;
}

bool Parser::attrGetBool(Tst<char *> *attr, char *name, bool default_value) {
    if (attr) {
        Tst<char *>::iterator it = attr->find(name);
        if (it != attr->end()) {
            if (! strcasecmp(attrValue(*it), "true")) return true;
            else return false;
        }
    }
    return default_value;
}

double Parser::attrGetDouble(Tst<char *> *attr, char *name,
                             double default_value) {
    if (attr) {
        Tst<char *>::iterator it = attr->find(name);
        if (it != attr->end())
            return strtod(attrValue(*it), NULL);
    }
    return default_value;
}

int Parser::attrGetInt(Tst<char *> *attr, char *name, int default_value) {
    if (attr) {
        Tst<char *>::iterator it = attr->find(name);
        if (it != attr->end())
            return (int) strtol(attrValue(*it), NULL, 0);
    }
    return default_value;
}

unsigned int Parser::attrGetUint(Tst<char *> *attr, char *name,
                                 unsigned int default_value) {
    if (attr) {
        Tst<char *>::iterator it = attr->find(name);
        if (it != attr->end())    
            return (unsigned int) strtol(attrValue(*it), NULL, 0);
    }
    return default_value;
}

double Parser::attrGetLength(Tst<char *> *attr, char *name,
                             double default_value, LenghtUnitType *unit) {
    if (attr) {
        Tst<char *>::iterator it = attr->find(name);
        if (it != attr->end())
            return get_double_and_unit(attrValue(*it), unit);
    }
    return default_value;
}

void Parser::attrGetWindowregexContent(Tst<char *> *attr, WindowRegex *wreg) {
    char *value = attrGetString(attr, "name", NULL);
    if (value) wreg->addIDRegex(WindowIDName, value);
    value = attrGetString(attr, "class", NULL);
    if (value) wreg->addIDRegex(WindowIDClass, value);
    value = attrGetString(attr, "classname", NULL);
    if (value) wreg->addIDRegex(WindowIDClassName, value);
    value = attrGetString(attr, "pid", NULL);
    if (value) wreg->addIDRegex(WindowIDPID, value);
    value = attrGetString(attr, "host", NULL);
    if (value) wreg->addIDRegex(WindowIDHost, value);
    value = attrGetString(attr, "resourceid", NULL);
    if (value) wreg->addIDRegex(WindowIDWinID, value);
}

char *Parser::attrValue(char *value) {
    Tst<char *>::iterator it = constants.find(value);
    if (it != constants.end()) {
        return *it;
    } else {
        it = ws->constants.find(value);
        if (it != ws->constants.end())
            return *it;
    }
    return value;
}

ElementHandler::ElementHandler(Parser *_parser) :
    RefCounted<ElementHandler>(this) {
    parser = _parser;
}

IncludeElementHandler::IncludeElementHandler(Parser *_parser,
                                             ElementHandler *_handler,
                                             bool _ignore_missing) :
    ElementHandler(_parser) {
    previous_handler = _handler;
    ignore_missing = _ignore_missing;
}

void IncludeElementHandler::characters(const XML_Char *chars, int len) {
    strbuf.addString((char *) chars, len);
}

void IncludeElementHandler::endElement(const XML_Char *) {
    char *fname = strbuf.getString();

    if (parser->include_depth >= MAX_INCLUDE_DEPTH) {
        parser->warning("%s: cannot be included because maximum include "
                        "depth of %d reached", fname, MAX_INCLUDE_DEPTH);
        delete [] fname;
        return;
    }

    if (*fname == '|') {
        Parser *command_parser = new Parser(parser->ws,
                                            parser->include_depth + 1);
        previous_handler->parser = command_parser;
        command_parser->pushElementHandler(previous_handler->ref());
        command_parser->parseCommand(fname + 1, ignore_missing);
        previous_handler->parser = parser;
        
        delete command_parser;
    } else {
        char *filename = smartfile(fname, parser->filename, false);

        if (filename) {
            Parser *include_parser = new Parser(parser->ws,
                                                parser->include_depth + 1);
            previous_handler->parser = include_parser;
            include_parser->pushElementHandler(previous_handler->ref());
            include_parser->parseFile(filename, ignore_missing);
            previous_handler->parser = parser;
            
            delete include_parser;
            delete [] filename;
        } else {
            if (! ignore_missing)
                parser->warning("unable to open file: %s\n", fname);
        }
    }

    if (fname) delete [] fname;
}

InheritElementHandler::InheritElementHandler(CfgElementHandler *_cfg_handler,
                                             Tst<char *> *attr) :
    ElementHandler(_cfg_handler->parser) {
    ignore_missing = parser->attrGetBool(attr, "ignore_missing", false);
}

void InheritElementHandler::characters(const XML_Char *chars, int len) {
    strbuf.addString((char *) chars, len);
}

static const ParserElementMap cfg_element_map[] = {
    { "constant", ParserElementConstant },
    { "include", ParserElementInclude },
    { "do", ParserElementDo },
    { "actionlist", ParserElementActionList },
    { "match", ParserElementMatch },
    { "menu", ParserElementMenu },
    { "style", ParserElementStyle },
    { "group", ParserElementRenderGroup },
    { "path", ParserElementRenderPath },
    { "pattern", ParserElementRenderPattern },
    { "text", ParserElementRenderText }
};

ParserElement CfgElementHandler::elementMap(const XML_Char *name) {
    Tst<ParserElement>::iterator it = cfg_element_tst->find((char *) name);
    if (it != cfg_element_tst->end()) return *it;
    else return ParserElementNone;
}

static const struct WindowStateMap {
    char *name;
    int state;
} win_state_map[] = {
    { "passive", WIN_STATE_PASSIVE },
    { "active", WIN_STATE_ACTIVE },
    { "hover", WIN_STATE_HOVER },
    { "pressed", WIN_STATE_PRESSED }
};

bool CfgElementHandler::startElement(ParserElement element,
                                     const XML_Char *, Tst<char *> *attr) {
    switch (element) {
        case ParserElementConstant: {
            char *cname = parser->attrGetString(attr, "name", NULL);
            char *cvalue = parser->attrGetString(attr, "value", NULL);
            if (cname && cvalue) {
                Tst<char *> *cons = &parser->ws->constants;
                char *storage = parser->attrGetString(attr, "scope", NULL);
                if (storage && ((! strcmp(storage, "local"))))
                    cons = &parser->constants;
                
                Tst<char *>::iterator it = cons->find(cname);
                if (it == cons->end())
                    cons->insert(cname, WA_STRDUP(cvalue));
                
            } else {
                parser->warning("required attribute=name or attribute=value "
                                "is missing\n");
            }
        } break;
        case ParserElementInclude: {
            bool ignore_missing = parser->attrGetBool(attr, "ignore_missing",
                                                      false);
            IncludeElementHandler *handler = new
                IncludeElementHandler(parser, this, ignore_missing);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementDo: {
            Doing *doing = new Doing(parser->ws);
            doing->applyAttributes(parser, attr);
            parser->ws->meawm_ng->eh->doings.push_back(doing);
        } break;
        case ParserElementActionList: {
            ActionList *actionlist = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                actionlist = parser->ws->getActionListNamed(name, false);
                if (actionlist) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        actionlist->clear();
                } else {
                    actionlist = new ActionList(parser->ws, name);
                    parser->ws->actionlists.insert(actionlist->name,
                                                   actionlist);
                }
            } else {
                parser->warning("required attribute=name is missing\n");
                return false;
            }
            
            ActionListElementHandler *handler =
                new ActionListElementHandler(this, actionlist);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementMenu: {
            Menu *menu = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                menu = parser->ws->getMenuNamed(name, false);
                if (menu) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        menu->clear();
                } else {
                    menu = new Menu(parser->ws, name);
                    menu->commonStyleUpdate();
                    parser->ws->menus.insert(menu->name, menu);
                }
                menu->applyAttributes(parser, attr);
            } else {
                parser->warning("required attribute=name is missing\n");
                return false;
            }
            
            MenuElementHandler *handler = new MenuElementHandler(this, menu);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementRenderGroup: {
            RenderGroup *rendergroup = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                rendergroup = parser->ws->getRenderGroupNamed(name, false);
                if (rendergroup) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        rendergroup->clear();
                } else {
                    rendergroup = new RenderGroup(parser->ws, name);
                    parser->ws->rendergroups.insert(rendergroup->name,
                                                    rendergroup);
                }
            } else {
                parser->warning("group without attribute=name outside of a "
                                "style tag does not make any sense\n");
                return false;
            }

            rendergroup->applyAttributes(parser, attr);
            
            RenderGroupElementHandler *handler =
                new RenderGroupElementHandler(this, rendergroup, NULL);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementStyle: {
            Style *style = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                style = parser->ws->getStyleNamed(name, false);
                if (style) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        style->clear();
                } else {
                    style = new Style(parser->ws, name);
                    parser->ws->styles.insert(style->name, style);
                    parser->ws->rendergroups.insert(style->name,
                                                    (Style *) style->ref());
                }
            } else {
                parser->warning("required attribute=name is missing\n");
                return false;
            }
            
            style->applyAttributes(parser, attr);
            
            RenderGroupElementHandler *handler =
                new RenderGroupElementHandler(this, (RenderGroup *) style,
                                              NULL);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementRenderPattern: {
            RenderPattern *pattern = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                pattern = parser->ws->getPatternNamed(name, false);
                if (pattern) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        pattern->clear();
                } else {
                    pattern = new RenderPattern();
                    parser->ws->patterns.insert(name, pattern);
                }
            } else {
                parser->warning("path without attribute=name\n");
                return false;
            }

            pattern->applyAttributes(parser, attr);
            
            RenderPatternElementHandler *handler =
                new RenderPatternElementHandler(this, pattern);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementRenderPath: {
            RenderOpPath *path = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                path = parser->ws->getPathNamed(name, false);
                if (path) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        path->clear();
                } else {
                    path = new RenderOpPath(name);
                    parser->ws->paths.insert(path->name, path);
                }
            } else {
                parser->warning("path without attribute=name outside of a "
                                "group or style tag does not make any "
                                "sense\n");
                return false;
            }

            path->applyAttributes(parser, attr);
            
            RenderPathElementHandler *handler =
                new RenderPathElementHandler(this, path, NULL);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementRenderText: {
            RenderOpText *text = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                text = parser->ws->getTextNamed(name, false);
                if (text) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        text->clear();
                } else {
                    text = new RenderOpText(name);
                    parser->ws->texts.insert(text->name,
                                             (RenderOpText *) text->ref());
                }
            } else {
                parser->warning("text object without attribute=name "
                                "outside of a group or style tag does not "
                                "make any sense\n");
                return false;
            }
            
            text->applyAttributes(parser, attr);
            
            RenderOpTextElementHandler *handler =
                new RenderOpTextElementHandler(this, text, NULL);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementMatch: {
            char *a_name = parser->attrGetString(attr, "actionlist", NULL);
            char *s_name = parser->attrGetString(attr, "style", NULL);
            if (a_name || s_name) {
                list<ActionRegex *> *a_tolist = NULL;
                list<StyleRegex *> *s_tolist = NULL;
                char *t = parser->attrGetString(attr, "target", "window");
                if (t) {
                    if (a_name) a_tolist = parser->ws->getRegexActionList(t);
                    if (s_name) s_tolist = parser->ws->getRegexStyleList(t);
                    if (a_tolist || s_tolist) {
                        int state = WIN_STATE_PASSIVE;
                        char *s = parser->attrGetString(attr, "state", NULL);
                        if (s) {
                            Tst<int>::iterator it = win_state_tst->find(s);
                            if (it != win_state_tst->end()) state = *it;
                        }

                        char *w = parser->attrGetString(attr, "window", NULL);
                        
                        if (a_tolist) {
                            ActionRegex *areg = new ActionRegex(state, w);
                            parser->attrGetWindowregexContent(attr, areg);
                            areg->actionlist =
                                parser->ws->getActionListNamed(a_name, false);
                            if (! areg->actionlist) {
                                parser->warning("undefined actionlist: %s\n",
                                                a_name);
                                delete areg;
                            } else
                                a_tolist->push_front(areg);
                        }
                        if (s_tolist) {
                            StyleRegex *sreg = new StyleRegex(state, w);
                            parser->attrGetWindowregexContent(attr, sreg);
                            sreg->style =
                                parser->ws->getStyleNamed(s_name, false);
                            if (! sreg->style) {
                                parser->warning("undefined style: %s\n",
                                                s_name);
                                delete sreg;
                            } else
                                s_tolist->push_front(sreg);
                        }                        
                    }
                } else
                    parser->warning("missing required attribute=target\n");
            }
        } break;
        default:
            break;
    }
    
    return false;
}

ActionListElementHandler::ActionListElementHandler(
    CfgElementHandler *_cfg_handler, ActionList *_actionlist)
    : ElementHandler(_cfg_handler->parser) {
    actionlist = _actionlist;
    cfg_handler = _cfg_handler;
}

static const ParserElementMap actionlist_element_map[] = {
    { "inherit", ParserElementInherit },
    { "action", ParserElementAction }
};

ParserElement ActionListElementHandler::elementMap(const XML_Char *name) {
    if (! actionlist) return ParserElementNone;
    Tst<ParserElement>::iterator it =
        actionlist_element_tst->find((char *) name);
    if (it != actionlist_element_tst->end()) return *it;
    else return ParserElementNone;
}

bool ActionListElementHandler::startElement(ParserElement element,
                                            const XML_Char *,
                                            Tst<char *> *attr) {
    switch (element) {
        case ParserElementAction: {
            Action *action = new Action();
            action->applyAttributes(parser, attr);
            ActionElementHandler *handler =
                new ActionElementHandler(cfg_handler, actionlist, action);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementInherit: {
            ActionListInheritElementHandler *handler = new
                ActionListInheritElementHandler(cfg_handler, actionlist, attr);
            parser->pushElementHandler(handler);
            return true;
        } break;
        default:
            break;
    }
    return false;
}

void ActionListElementHandler::endElement(const XML_Char *) {
    parser->ws->propagateActionlistUpdate(actionlist);
}

ActionListInheritElementHandler::ActionListInheritElementHandler(
    CfgElementHandler *_cfg_handler, ActionList *_actionlist,
    Tst<char *> *attr) : InheritElementHandler(_cfg_handler, attr) {
    actionlist = _actionlist;
}

void ActionListInheritElementHandler::endElement(const XML_Char *) {
    char *actionlistname = strbuf.getString();
    
    ActionList *inherit_list = parser->ws->getActionListNamed(actionlistname,
                                                              false);

    if (inherit_list == actionlist) {
        parser->warning("actionlist %s cannot inherit itself\n",
                        actionlist->name);
    } else if (inherit_list) {
        actionlist->inheritContent(inherit_list);
    } else {
        if (! ignore_missing)
            parser->warning("undefined actionlist: %s\n", actionlistname);
    }
    
    delete [] actionlistname;
}

ActionElementHandler::ActionElementHandler(
    CfgElementHandler *_cfg_handler, ActionList *_actionlist,
    Action *_action) :
    ElementHandler(_cfg_handler->parser) {
    cfg_handler = _cfg_handler;
    actionlist = _actionlist;
    action = _action;
}

static const ParserElementMap action_element_map[] = {
    { "modifier", ParserElementModifier }
};

ParserElement ActionElementHandler::elementMap(const XML_Char *name) {
    Tst<ParserElement>::iterator it = action_element_tst->find((char *) name);
    if (it != action_element_tst->end()) return *it;
    else return ParserElementNone;
}

static ModifierMap x11_modifier_map[] = {
    { "shift", ShiftMask },
    { "lock", LockMask },
    { "control", ControlMask },
    { "mod1", Mod1Mask },
    { "mod2", Mod2Mask },
    { "mod3", Mod3Mask },
    { "mod4", Mod4Mask },
    { "mod5", Mod5Mask },
    { "button1", Button1Mask },
    { "button2", Button2Mask },
    { "button3", Button3Mask },
    { "button4", Button4Mask },
    { "button5", Button5Mask }
};

static ModifierMap wa_modifier_map[] = {
    { "moveresize", MoveResizeMask },
    { "statetrue", StateTrueMask }
};

bool ActionElementHandler::startElement(ParserElement element,
                                        const XML_Char *,
                                        Tst<char *> *attr) {
    switch (element) {
        case ParserElementModifier: {
            unsigned int *x11_modifier_value = &action->x11mod;
            unsigned int *wa_modifier_value = &action->wamod;
            char *value = parser->attrGetString(attr, "constraint", NULL);
            if (value && (! strcasecmp(value, "must_not_exist"))) {
                x11_modifier_value = &action->x11nmod;
                wa_modifier_value = &action->wanmod;
            }

            value = parser->attrGetString(attr, "name", NULL);
            if (value) {
                bool mod_set = false;
                Tst<long int>::iterator it = x11_modifier_tst->find(value);
                if (it != x11_modifier_tst->end()) {
                    *x11_modifier_value = *it;
                    mod_set = true;
                }                    
                if (! mod_set) {
                    it = wa_modifier_tst->find(value);
                    if (it != wa_modifier_tst->end()) {
                        *wa_modifier_value = *it;
                        mod_set = true;
                    }
                }
                if (! mod_set) {
                    it = client_window_state_tst->find(value);
                    if (it != client_window_state_tst->end()) {
                        *wa_modifier_value = *it;
                        mod_set = true;
                    }
                }
                if (! mod_set) {
                    parser->warning("modifier=%s doesn't exist\n", value);
                }
            } else {
                value = parser->attrGetString(attr, "key", NULL);
                if (value) {
                    list<ModifierMap *>::iterator it =
                        parser->ws->meawm_ng->modmaps.begin();
                    for (; it != parser->ws->meawm_ng->modmaps.end(); it++) {
                        if (! strcasecmp(value, (*it)->name)) {
                            *x11_modifier_value |= (*it)->modifier;
                            break;
                        }
                    } if (it == parser->ws->meawm_ng->modmaps.end())
                        parser->warning("no modifier mapping to key: %s\n",
                                        value);
                } else
                    parser->warning("required attribute missing, neither "
                                    "name or key specified\n");
            }
        } break;
        default:
            break;
    }
    return false;
}

void ActionElementHandler::endElement(const XML_Char *) {
    if (! action) return;
        
    if (action->func && action->type) {
        actionlist->actionlist.push_back(action);
    } else {
        delete action;
        action = NULL;
    }
}

MenuInheritElementHandler::MenuInheritElementHandler(
    CfgElementHandler *_cfg_handler, Menu *_menu, Tst<char *> *attr) :
    InheritElementHandler(_cfg_handler, attr) {
    menu = _menu;
}

void MenuInheritElementHandler::endElement(const XML_Char *) {
    char *menuname = strbuf.getString();
    
    Menu *inherit_menu = parser->ws->getMenuNamed(menuname, false);

    if (inherit_menu == menu) {
        parser->warning("menu %s cannot inherit itself\n", menu->name);
    } else if (inherit_menu) {
        menu->inheritContent(inherit_menu);
    } else {
        if (! ignore_missing)
            parser->warning("undefined menu: %s\n", menuname);
    }

    delete [] menuname;
}

MenuElementHandler::MenuElementHandler(
    CfgElementHandler *_cfg_handler, Menu *_menu)
    : ElementHandler(_cfg_handler->parser) {
    menu = _menu;
    cfg_handler = _cfg_handler;
}

static const ParserElementMap menu_element_map[] = {
    { "inherit", ParserElementInherit },
    { "item", ParserElementMenuItem }
};

ParserElement MenuElementHandler::elementMap(const XML_Char *name) {
    if (! menu) return ParserElementNone;
    Tst<ParserElement>::iterator it = menu_element_tst->find((char *) name);
    if (it != menu_element_tst->end()) return *it;
    else return ParserElementNone;
}

bool MenuElementHandler::startElement(ParserElement element,
                                      const XML_Char *,
                                      Tst<char *> *attr) {
    switch (element) {
        case ParserElementMenuItem: {
            char *name = parser->attrGetString(attr, "name", "item");
            MenuItem *menuitem = new MenuItem(parser->ws, menu, name);
            menuitem->commonStyleUpdate();
            menu->items.push_back(menuitem);
            menuitem->applyAttributes(parser, attr);

            MenuItemElementHandler *handler = new
                MenuItemElementHandler(cfg_handler, menuitem);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementInherit: {
            MenuInheritElementHandler *handler = new
                MenuInheritElementHandler(cfg_handler, menu, attr);
            parser->pushElementHandler(handler);
            return true;
        } break;
        default:
            break;
    }
    return false;
}

void MenuElementHandler::endElement(const XML_Char *) {
    if (menu) menu->update();
}

MenuItemElementHandler::MenuItemElementHandler(
    CfgElementHandler *_cfg_handler, MenuItem *_menuitem)
    : ElementHandler(_cfg_handler->parser) {
    menuitem = _menuitem;
    cfg_handler = _cfg_handler;
}

static const ParserElementMap menuitem_element_map[] = {
    { "menu", ParserElementMenu },
    { "itemaction", ParserElementMenuItemAction }
};

ParserElement MenuItemElementHandler::elementMap(const XML_Char *name) {
    Tst<ParserElement>::iterator it =
        menuitem_element_tst->find((char *) name);
    if (it != menuitem_element_tst->end()) return *it;
    else return ParserElementNone;
}

bool MenuItemElementHandler::startElement(ParserElement element,
                                          const XML_Char *,
                                          Tst<char *> *attr) {
    switch (element) {
        case ParserElementMenu: {
            Menu *menu = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                menu = parser->ws->getMenuNamed(name, false);
                if (menu) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        menu->clear();
                } else {
                    menu = new Menu(parser->ws, name);
                    menu->commonStyleUpdate();
                    parser->ws->menus.insert(menu->name, menu);
                }
            } else {
                menu = new Menu(parser->ws, NULL);
                menu->commonStyleUpdate();
                parser->ws->menus.insert(menu->name, menu);
            }
            menu->applyAttributes(parser, attr);

            menuitem->submenu = (Menu *) menu->ref();
            
            MenuElementHandler *handler =
                new MenuElementHandler(cfg_handler, menu);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementMenuItemAction: {
            MenuItemAction *itemaction = new MenuItemAction();
            if (itemaction->applyAttributes(parser, attr))
                menuitem->actions.push_back(itemaction);
            else
                delete itemaction;
                
        } break;
        default:
            break;
    }
    return false;
}

RenderGroupElementHandler::RenderGroupElementHandler(
    CfgElementHandler *_cfg_handler, RenderGroup *_rendergroup,
    RenderGroup *_parentgroup) :
    ElementHandler(_cfg_handler->parser) {
    rendergroup = _rendergroup;
    parentgroup = _parentgroup;
    cfg_handler = _cfg_handler;
}

static const ParserElementMap rendergroup_element_map[] = {
    { "inherit", ParserElementInherit },
    { "group", ParserElementRenderGroup },
    { "dynamic", ParserElementRenderDynamic },
    { "shape_mask", ParserElementShapeMask },
    { "path", ParserElementRenderPath },
    { "line", ParserElementRenderLine },
    { "rectangle", ParserElementRenderRectangle },
    { "ellipse", ParserElementRenderEllipse },
    { "solid", ParserElementRenderSolid },
    { "image", ParserElementRenderImage },
    
#ifdef    SVG
    { "svg", ParserElementRenderSvg },
#endif // SVG
    
    { "text", ParserElementRenderText },
    { "window", ParserElementRenderWindow }
};

ParserElement RenderGroupElementHandler::elementMap(const XML_Char *name) {
    Tst<ParserElement>::iterator it =
        rendergroup_element_tst->find((char *) name);
    if (it != rendergroup_element_tst->end()) {
        if (*it == ParserElementRenderWindow) {
            if (rendergroup->is_a_style) return *it;
            else return ParserElementNone;
        } else
            return *it;
    } else
        return ParserElementNone;
}

bool RenderGroupElementHandler::startElement(ParserElement element,
                                             const XML_Char *,
                                             Tst<char *> *attr) {
    switch (element) {
        case ParserElementInherit: {
            RenderGroupInheritElementHandler *handler = new
                RenderGroupInheritElementHandler(cfg_handler, rendergroup,
                                                 attr);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementRenderWindow: {
            bool raised = parser->attrGetBool(attr, "raised", false);
            WindowElementHandler *handler =
                new WindowElementHandler(cfg_handler, (Style *) rendergroup,
                                         raised);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementShapeMask: {
            if (! rendergroup->is_a_style) return false;

            if (((Style *) rendergroup)->shapemask) {
                parser->warning("shape mask allready set\n");
                return false;
            }
            
            RenderGroup *subgroup = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                subgroup = parser->ws->getRenderGroupNamed(name, false);
                if (subgroup) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        subgroup->clear();
                } else {
                    subgroup = new RenderGroup(parser->ws, name);
                    parser->ws->rendergroups.insert(
                        subgroup->name, (RenderGroup *) subgroup->ref());
                }
            } else
                subgroup = new RenderGroup(parser->ws, NULL);

            subgroup->applyAttributes(parser, attr);
            
            ((Style *) rendergroup)->shapemask = subgroup;
            ((Style *) rendergroup)->shaped = true;
            
            RenderGroupElementHandler *handler =
                new RenderGroupElementHandler(cfg_handler, subgroup,
                                              rendergroup);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementRenderGroup: {
            RenderGroup *subgroup = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                subgroup = parser->ws->getRenderGroupNamed(name, false);
                if (subgroup) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        subgroup->clear();
                } else {
                    subgroup = new RenderGroup(parser->ws, name);
                    parser->ws->rendergroups.insert(
                        subgroup->name, (RenderGroup *) subgroup->ref());
                }
            } else
                subgroup = new RenderGroup(parser->ws, NULL);

            subgroup->applyAttributes(parser, attr);
            
            rendergroup->operations.push_back(subgroup);
            
            RenderGroupElementHandler *handler =
                new RenderGroupElementHandler(cfg_handler, subgroup,
                                              rendergroup);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementRenderDynamic: {
            RenderOpDynamic *dynamic = new RenderOpDynamic();
            dynamic->applyAttributes(parser, attr);
            RenderOpDynamicElementHandler *handler =
                new RenderOpDynamicElementHandler(cfg_handler, rendergroup,
                                                  dynamic);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementRenderPath: {
            RenderOpPath *path = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                path = parser->ws->getPathNamed(name, false);
                if (path) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        path->clear();
                } else {
                    path = new RenderOpPath(name);
                    parser->ws->paths.insert(path->name,
                                             (RenderOpPath *) path->ref());
                }
            } else
                path = new RenderOpPath(NULL);

            path->applyAttributes(parser, attr);

            rendergroup->operations.push_back(path);
            
            RenderPathElementHandler *handler =
                new RenderPathElementHandler(cfg_handler, path, rendergroup);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementRenderLine: {
            RenderOpLine *line = new RenderOpLine();            
            line->applyAttributes(parser, attr);
            rendergroup->operations.push_back(line);
        } break;
        case ParserElementRenderRectangle:
        case ParserElementRenderEllipse: {
            RenderOpRectangle *rectangle = new RenderOpRectangle();
            rectangle->applyAttributes(parser, attr);
            if (element == ParserElementRenderEllipse) {
                rectangle->nw_rx = rectangle->nw_ry = rectangle->ne_rx =
                    rectangle->ne_ry = rectangle->sw_rx = rectangle->sw_ry =
                    rectangle->se_rx = rectangle->se_ry = 50.0;
                rectangle->nw_rx_u = rectangle->nw_ry_u = rectangle->ne_rx_u =
                    rectangle->ne_ry_u = rectangle->sw_rx_u =
                    rectangle->sw_ry_u = rectangle->se_rx_u =
                    rectangle->se_ry_u = PCTLenghtUnitType;
            }
            rendergroup->operations.push_back(rectangle);
        } break;
        case ParserElementRenderSolid: {
            RenderOpSolid *solid = new RenderOpSolid();
            solid->applyAttributes(parser, attr);
            rendergroup->operations.push_back(solid);
        } break;
        case ParserElementRenderImage: {
            RenderOpImage *image = new RenderOpImage();
            if (image->applyAttributes(parser, attr)) {
                rendergroup->operations.push_back(image);
            } else
                delete image;
        } break;
        
#ifdef    SVG
        case ParserElementRenderSvg: {
            RenderOpSvg *svg = new RenderOpSvg();
            if (svg->applyAttributes(parser, attr)) {
                rendergroup->operations.push_back(svg);
            } else
                delete svg;
        } break;
#endif // SVG
        
        case ParserElementRenderText: {
            RenderOpText *text = NULL;
            char *name = parser->attrGetString(attr, "name", NULL);
            if (name) {
                text = parser->ws->getTextNamed(name, false);
                if (text) {
                    if (! parser->attrGetBool(attr, "extend", false))
                        text->clear();
                } else {
                    text = new RenderOpText(name);
                    parser->ws->texts.insert(text->name,
                                             (RenderOpText *) text->ref());
                }
            } else
                text = new RenderOpText(NULL);            

            text->applyAttributes(parser, attr);
            
            RenderOpTextElementHandler *handler =
                new RenderOpTextElementHandler(cfg_handler, text, rendergroup);
            parser->pushElementHandler(handler);
            return true;
        } break;
        default:
            break;
    }
    return false;
}

void RenderGroupElementHandler::endElement(const XML_Char *) {
    if (rendergroup->is_a_style) {
        ((Style *) rendergroup)->flattenTextOps(
            &((Style *) rendergroup)->textops);
        ((Style *) rendergroup)->serial++;
        parser->ws->propagateStyleUpdate((Style *) rendergroup);
    }
    if (parentgroup) {
        if (rendergroup->has_dynamic_op)
            parentgroup->has_dynamic_op = true;
        
        if (! rendergroup->cacheable)
            parentgroup->cacheable = false;
    }
}

RenderGroupInheritElementHandler::RenderGroupInheritElementHandler(
    CfgElementHandler *_cfg_handler, RenderGroup *_rendergroup,
    Tst<char *> *attr) : InheritElementHandler(_cfg_handler, attr) {
    rendergroup = _rendergroup;
}

void RenderGroupInheritElementHandler::endElement(const XML_Char *) {
    char *rendergroupname = strbuf.getString();
    
    RenderGroup *inherit_rendergroup =
        parser->ws->getRenderGroupNamed(rendergroupname, false);
    
    if (inherit_rendergroup == rendergroup) {
        parser->warning("group/style %s cannot inherit itself\n",
                        rendergroup->name);
    } else if (inherit_rendergroup) {
        if (inherit_rendergroup->is_a_style && rendergroup->is_a_style)
            ((Style *) rendergroup)->inheritContent(
                (Style *) inherit_rendergroup);
        else
            rendergroup->inheritContent(inherit_rendergroup);
    } else {
        if (! ignore_missing)
            parser->warning("undefined group/style: %s\n", rendergroupname);
    }
    
    delete [] rendergroupname;
}

WindowElementHandler::WindowElementHandler(
    CfgElementHandler *_cfg_handler, Style *_style, bool _raised) :
    ElementHandler(_cfg_handler->parser) {
    style = _style;
    cfg_handler = _cfg_handler;
    raised = _raised;
}

void WindowElementHandler::characters(const XML_Char *chars, int len) {
    strbuf.addString((char *) chars, len);
}

void WindowElementHandler::endElement(const XML_Char *) {
    char *windowname = strbuf.getString();
    
    if (*windowname != '\0') {
        if (! style->subs) style->subs = new list<SubwinInfo *>;
        SubwinInfo *sinfo =
            new SubwinInfo(parser->ws->getSubwindowId(windowname), raised);
        style->subs->push_back(sinfo);
    } else {
        parser->warning("missing character data for window name");
    }

    delete [] windowname;
}

static const ParserElementMap renderopdynamic_element_map[] = {
    { "try", ParserElementRenderDynamicTry }
};

RenderOpDynamicElementHandler::RenderOpDynamicElementHandler(
    CfgElementHandler *_cfg_handler, RenderGroup *_rendergroup,
    RenderOpDynamic *_dynamic) :
    ElementHandler(_cfg_handler->parser) {
    dynamic = _dynamic;
    cfg_handler = _cfg_handler;
    rendergroup = _rendergroup;
}

ParserElement RenderOpDynamicElementHandler::elementMap(const XML_Char *name) {
    Tst<ParserElement>::iterator it =
        renderopdynamic_element_tst->find((char *) name);
    if (it != renderopdynamic_element_tst->end())
        return *it;
    else
        return ParserElementNone;
}

static const struct DynamicTryMap {
    char *name;
    int type;
} dynamic_try_map[] = {
    { "static", DynamicGroupStaticType },
    { "menuitemiconimage", DynamicGroupMenuItemIconImageType },
    { "menuitemiconsvg", DynamicGroupMenuItemIconSvgType },
    { "wmiconimage", DynamicGroupWmIconImageType },
    { "wmiconsvg", DynamicGroupWmIconSvgType }
};

bool RenderOpDynamicElementHandler::startElement(ParserElement element,
                                                 const XML_Char *,
                                                 Tst<char *> *attr) {
    switch (element) {
        case ParserElementRenderDynamicTry: {
            char *value = parser->attrGetString(attr, "type", NULL);
            if (value) {
                Tst<int>::iterator it = dynamic_try_tst->find((char *) value);
                if (it != dynamic_try_tst->end()) {
                    if (*it == DynamicGroupStaticType) {
                        value = parser->attrGetString(attr, "name", NULL);
                        if (value) {
                            RenderGroup *rendergroup =
                                parser->ws->getRenderGroupNamed(value, false);
                            if (rendergroup)
                                dynamic->dynamic_order.push_back(*it);
                            else
                                parser->warning("unknown static group: %s",
                                                value);
                        } else
                            parser->warning("static group name not defined");
                    } else
                        dynamic->dynamic_order.push_back(*it);
                }
            }
        }
        default:
            break;
    }

    return false;
}

void RenderOpDynamicElementHandler::endElement(const XML_Char *) {
    if (! dynamic->dynamic_order.empty()) {
        rendergroup->operations.push_back(dynamic);
        rendergroup->has_dynamic_op = true;
    } else
        cfg_handler->parser->warning("missing group in dynamic operation");
}

RenderPatternElementHandler::RenderPatternElementHandler(
    CfgElementHandler *_cfg_handler, RenderPattern *_pattern) :
    ElementHandler(_cfg_handler->parser) {
    pattern = _pattern;
    cfg_handler = _cfg_handler;
}

static const ParserElementMap renderpattern_element_map[] = {
    { "colorstop", ParserElementRenderColorStop }
};

ParserElement RenderPatternElementHandler::elementMap(const XML_Char *name) {
    Tst<ParserElement>::iterator it =
        renderpattern_element_tst->find((char *) name);
    if (it != renderpattern_element_tst->end()) return *it;
    else return ParserElementNone;
}

bool RenderPatternElementHandler::startElement(ParserElement element,
                                               const XML_Char *,
                                               Tst<char *> *attr) {
    WaColor *color = new WaColor();
    double offset = 0.0;
    char *value = parser->attrGetString(attr, "color", NULL);
    if (value) {
        if (! color->parseColor(parser->ws, value))
            parser->warning("error parsing color: %s\n", value);
    }
    if (color)
        color->setOpacity(
            parser->attrGetDouble(attr, "opacity",
                                  color->getOpacity()));

    offset = parser->attrGetDouble(attr, "offset", offset);

    pattern->color_stops.push_back(new WaColorStop(offset, color));
    
    return false;
}

RenderPatternInheritElementHandler::RenderPatternInheritElementHandler(
    CfgElementHandler *_cfg_handler, RenderPattern *_pattern,
    Tst<char *> *attr) : InheritElementHandler(_cfg_handler, attr) {
    pattern = _pattern;
}

void RenderPatternInheritElementHandler::endElement(const XML_Char *) {
    char *patternname = strbuf.getString();
    
    RenderPattern *inherit_pattern =
        parser->ws->getPatternNamed(patternname, false);
    
    if (inherit_pattern == pattern) {
        parser->warning("pattern %s cannot inherit itself\n", patternname);
    } else if (inherit_pattern) {
        pattern->inheritContent(inherit_pattern);
    } else {
        if (! ignore_missing)
            parser->warning("undefined pattern: %s\n", patternname);
    }
    
    delete [] patternname;
}

RenderPathElementHandler::RenderPathElementHandler(
    CfgElementHandler *_cfg_handler, RenderOpPath *_path,
    RenderGroup *_rendergroup) : ElementHandler(_cfg_handler->parser) {
    path = _path;
    cfg_handler = _cfg_handler;
    rendergroup = _rendergroup;
}

static const ParserElementMap renderpath_element_map[] = {
    { "inherit", ParserElementInherit },
    { "moveto", ParserElementRenderPathMoveTo },
    { "relmoveto", ParserElementRenderPathRelMoveTo },
    { "lineto", ParserElementRenderPathLineTo },
    { "rellineto", ParserElementRenderPathRelLineTo },
    { "curveto", ParserElementRenderPathCurveTo },
    { "relcurveto", ParserElementRenderPathRelCurveTo },
    { "arcto", ParserElementRenderPathArcTo },
    { "close", ParserElementRenderPathClose }
};

ParserElement RenderPathElementHandler::elementMap(const XML_Char *name) {
    Tst<ParserElement>::iterator it =
        renderpath_element_tst->find((char *) name);
    if (it != renderpath_element_tst->end()) return *it;
    else return ParserElementNone;
}

bool RenderPathElementHandler::startElement(ParserElement element,
                                            const XML_Char *,
                                            Tst<char *> *attr) {
    PathOperator *op = NULL;
    switch (element) {
        case ParserElementInherit: {
            RenderOpPathInheritElementHandler *handler = new
                RenderOpPathInheritElementHandler(cfg_handler, path, attr);
            parser->pushElementHandler(handler);
            return true;
        } break;
        case ParserElementRenderPathMoveTo:
            op = new PathOperator(PathOperatorMoveToType);
            op->applyAttributes(parser, attr);
            break;
        case ParserElementRenderPathRelMoveTo:
            op = new PathOperator(PathOperatorRelMoveToType);
            op->applyAttributes(parser, attr);
            break;
        case ParserElementRenderPathLineTo:
            op = new PathOperator(PathOperatorLineToType);
            op->applyAttributes(parser, attr);
            break;
        case ParserElementRenderPathRelLineTo:
            op = new PathOperator(PathOperatorRelLineToType);
            op->applyAttributes(parser, attr);
            break;
        case ParserElementRenderPathCurveTo:
            op = new PathOperator(PathOperatorCurveToType);
            op->applyAttributes(parser, attr);
            break;
        case ParserElementRenderPathRelCurveTo:
            op = new PathOperator(PathOperatorRelCurveToType);
            op->applyAttributes(parser, attr);
            break;
        case ParserElementRenderPathArcTo:
            op = new PathOperator(PathOperatorArcToType);
            op->applyAttributes(parser, attr);
            break;
        case ParserElementRenderPathRelArcTo:
            op = new PathOperator(PathOperatorRelArcToType);
            op->applyAttributes(parser, attr);
            break;
        case ParserElementRenderPathClose:
            op = new PathOperator(PathOperatorCloseType);
            op->applyAttributes(parser, attr);
            break;
        default:
            break;
    }
    if (op) path->ops.push_back(op);
    
    return false;
}

void RenderPathElementHandler::endElement(const XML_Char *) {
    if (path->ops.empty())
        parser->warning("path element does not contain any operations\n");
}

RenderOpPathInheritElementHandler::RenderOpPathInheritElementHandler(
    CfgElementHandler *_cfg_handler, RenderOpPath *_path,
    Tst<char *> *attr) : InheritElementHandler(_cfg_handler, attr) {
    path = _path;
}

void RenderOpPathInheritElementHandler::endElement(const XML_Char *) {
    char *pathname = strbuf.getString();
    
    RenderOpPath *inherit_path = parser->ws->getPathNamed(pathname, false);
    
    if (inherit_path == path) {
        parser->warning("path %s cannot inherit itself\n", path->name);
    } else if (inherit_path) {
        path->inheritContent(inherit_path);
    } else {
        if (! ignore_missing)
            parser->warning("undefined path: %s\n", pathname);
    }
    
    delete [] pathname;
}

static const ParserElementMap rendertext_element_map[] = {
    { "inherit", ParserElementInherit }
};

ParserElement RenderOpTextElementHandler::elementMap(const XML_Char *name) {
    Tst<ParserElement>::iterator it =
        rendertext_element_tst->find((char *) name);
    if (it != rendertext_element_tst->end()) return *it;
    else return ParserElementNone;
}

RenderOpTextElementHandler::RenderOpTextElementHandler(
    CfgElementHandler *_cfg_handler, RenderOpText *_text,
    RenderGroup *_rendergroup) : ElementHandler(_cfg_handler->parser) {
    text = _text;
    rendergroup = _rendergroup;
    cfg_handler = _cfg_handler;
}

bool RenderOpTextElementHandler::startElement(ParserElement element,
                                              const XML_Char *,
                                              Tst<char *> *attr) {
    switch (element) {
        case ParserElementInherit: {
            RenderOpTextInheritElementHandler *handler = new
                RenderOpTextInheritElementHandler(cfg_handler, &strbuf, attr);
            parser->pushElementHandler(handler);
            return true;
        } break;
        default:
            break;
    }
    return false;
}

void RenderOpTextElementHandler::characters(const XML_Char *chars, int len) {
    strbuf.addString((char *) chars, len);
}

void RenderOpTextElementHandler::endElement(const XML_Char *) {
    char *utf8 = strbuf.getString();
    bool dynamic;

    if (! text->is_static) {
        text->is_static = true;
        text->utf8 = preexpand(utf8, &dynamic);
        if (dynamic) {
            text->is_static = false;
            if (rendergroup) rendergroup->cacheable = false;
        }
    } else {
        text->is_static = true;
        text->utf8 = WA_STRDUP(utf8);
    }

    if (rendergroup) rendergroup->operations.push_back(text);
    
    delete [] utf8;
}

RenderOpTextInheritElementHandler::RenderOpTextInheritElementHandler(
    CfgElementHandler *_cfg_handler, StringBuffer *_textstrbuf,
    Tst<char *> *attr) : InheritElementHandler(_cfg_handler, attr) {
    textstrbuf = _textstrbuf;
}

void RenderOpTextInheritElementHandler::endElement(const XML_Char *) {
    char *textname = strbuf.getString();
    
    RenderOpText *inherit_text = parser->ws->getTextNamed(textname, false);
    
    if (inherit_text) {
        if (inherit_text->utf8)
            textstrbuf->addString((char *) inherit_text->utf8,
                                  strlen(inherit_text->utf8));
    } else {
        if (! ignore_missing)
            parser->warning("undefined text object: %s\n", textname);
    }
    
    delete [] textname;
}

double get_double_and_unit(char *s, LenghtUnitType *unit) {
    char *invalid_char;
    double value = strtod(s, &invalid_char);
    if (unit)
        if (invalid_char) {
            if (! strcasecmp(invalid_char, "%")) {
                *unit = PCTLenghtUnitType;
            } else if (! strcasecmp(invalid_char, "px")) {
                *unit = PXLenghtUnitType;
            } else if (! strcasecmp(invalid_char, "cm")) {
                *unit = CMLenghtUnitType;
            } else if (! strcasecmp(invalid_char, "mm")) {
                *unit = MMLenghtUnitType;
            } else if (! strcasecmp(invalid_char, "in")) {
                *unit = INLenghtUnitType;
            } else if (! strcasecmp(invalid_char, "pt")) {
                *unit = PTLenghtUnitType;
            } else if (! strcasecmp(invalid_char, "pc")) {
                *unit = PCLenghtUnitType;
            } else {
                *unit = PXLenghtUnitType;
            }
        } else
            *unit = PXLenghtUnitType;
    
    return value;
} 

void parser_create_tsts(void) {
    int size, i;

    win_state_tst = new Tst<int>;
    size = sizeof(win_state_map) / sizeof(WindowStateMap);
    for (i = 0; i < size; i++)
        win_state_tst->insert(win_state_map[i].name, win_state_map[i].state);
    
    x11_modifier_tst = new Tst<long int>;
    size = sizeof(x11_modifier_map) / sizeof(ModifierMap);
    for (i = 0; i < size; i++)
        x11_modifier_tst->insert(x11_modifier_map[i].name,
                                 x11_modifier_map[i].modifier);

    wa_modifier_tst = new Tst<long int>;
    size = sizeof(wa_modifier_map) / sizeof(ModifierMap);
    for (i = 0; i < size; i++)
        wa_modifier_tst->insert(wa_modifier_map[i].name,
                                wa_modifier_map[i].modifier);

    dynamic_try_tst = new Tst<int>;
    size = sizeof(dynamic_try_map) / sizeof(DynamicTryMap);
    for (i = 0; i < size; i++)
        dynamic_try_tst->insert(dynamic_try_map[i].name,
                                dynamic_try_map[i].type);
    
    cfg_element_tst = new Tst<ParserElement>;
    size = sizeof(cfg_element_map) / sizeof(ParserElementMap);
    for (i = 0; i < size; i++)
        cfg_element_tst->insert(cfg_element_map[i].name,
                                cfg_element_map[i].element);

    actionlist_element_tst = new Tst<ParserElement>;
    size = sizeof(actionlist_element_map) / sizeof(ParserElementMap);
    for (i = 0; i < size; i++)
        actionlist_element_tst->insert(actionlist_element_map[i].name,
                                       actionlist_element_map[i].element);

    action_element_tst = new Tst<ParserElement>;
    size = sizeof(action_element_map) / sizeof(ParserElementMap);
    for (i = 0; i < size; i++)
        action_element_tst->insert(action_element_map[i].name,
                                   action_element_map[i].element);

    menu_element_tst = new Tst<ParserElement>;
    size = sizeof(menu_element_map) / sizeof(ParserElementMap);
    for (i = 0; i < size; i++)
        menu_element_tst->insert(menu_element_map[i].name,
                                 menu_element_map[i].element);

    menuitem_element_tst = new Tst<ParserElement>;
    size = sizeof(menuitem_element_map) / sizeof(ParserElementMap);
    for (i = 0; i < size; i++)
        menuitem_element_tst->insert(menuitem_element_map[i].name,
                                     menuitem_element_map[i].element);

    rendergroup_element_tst = new Tst<ParserElement>;
    size = sizeof(rendergroup_element_map) / sizeof(ParserElementMap);
    for (i = 0; i < size; i++)
        rendergroup_element_tst->insert(rendergroup_element_map[i].name,
                                        rendergroup_element_map[i].element);

    renderopdynamic_element_tst = new Tst<ParserElement>;
    size = sizeof(renderopdynamic_element_map) / sizeof(ParserElementMap);
    for (i = 0; i < size; i++)
        renderopdynamic_element_tst->insert(
            renderopdynamic_element_map[i].name,
            renderopdynamic_element_map[i].element);

    renderpath_element_tst = new Tst<ParserElement>;
    size = sizeof(renderpath_element_map) / sizeof(ParserElementMap);
    for (i = 0; i < size; i++)
        renderpath_element_tst->insert(renderpath_element_map[i].name,
                                       renderpath_element_map[i].element);

    renderpattern_element_tst = new Tst<ParserElement>;
    size = sizeof(renderpattern_element_map) / sizeof(ParserElementMap);
    for (i = 0; i < size; i++)
        renderpattern_element_tst->insert
          (renderpattern_element_map[i].name,
           renderpattern_element_map[i].element);

    rendertext_element_tst = new Tst<ParserElement>;
    size = sizeof(rendertext_element_map) / sizeof(ParserElementMap);
    for (i = 0; i < size; i++)
        rendertext_element_tst->insert(rendertext_element_map[i].name,
                                       rendertext_element_map[i].element);
}
