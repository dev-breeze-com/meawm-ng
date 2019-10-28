/* Parser.hh

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

#ifndef __Parser_hh
#define __Parser_hh

extern "C" {

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>


#ifdef    HAVE_STDARG_H
#include <stdarg.h>
#endif // HAVE_STDARG_H

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_XMLPARSE_H
#  include <xmlparse.h>
#else // !HAVE_XMLPARSE_H
#  include <expat.h>
#endif // HAVE_XMLPARSE_H
    
}

#include "Util.hh"
#include "RefCounted.hh"
#include "Regex.hh"
#include "Tst.hh"

class Action;
class ActionList;
class Menu;
class MenuItem;
class RenderGroup;
class RenderOpDynamic;
class RenderPattern;
class RenderOpPath;
class RenderOpLine;
class RenderOpRectangle;
class RenderOpEllipse;
class RenderOpText;
class Style;
class WaScreen;
class Variable;

typedef enum {
    PXLenghtUnitType,
    CMLenghtUnitType,
    MMLenghtUnitType,
    INLenghtUnitType,
    PTLenghtUnitType,
    PCLenghtUnitType,
    PCTLenghtUnitType
} LenghtUnitType;

#define PARSE_BUFFER_SIZE 8192
#define MAX_INCLUDE_DEPTH 128

typedef enum {
    ParserElementNone,

    ParserElementConstant,
    ParserElementInclude,
    ParserElementInherit,
    ParserElementMatch,
    ParserElementDo,
    
    ParserElementActionList,
    ParserElementAction,
    ParserElementModifier,
    
    ParserElementMenu,
    ParserElementMenuItem,
    ParserElementMenuItemAction,
    
    ParserElementStyle,
    ParserElementShapeMask,
    ParserElementRenderGroup,
    ParserElementRenderDynamic,
    ParserElementRenderDynamicTry,
    ParserElementRenderWindow,
    ParserElementRenderColorStop,
    ParserElementRenderPath,
    ParserElementRenderPathMoveTo,
    ParserElementRenderPathRelMoveTo,
    ParserElementRenderPathLineTo,
    ParserElementRenderPathRelLineTo,
    ParserElementRenderPathCurveTo,
    ParserElementRenderPathRelCurveTo,
    ParserElementRenderPathArcTo,
    ParserElementRenderPathRelArcTo,
    ParserElementRenderPathClose,
    ParserElementRenderPattern,
    ParserElementRenderLine,
    ParserElementRenderRectangle,
    ParserElementRenderEllipse,
    ParserElementRenderSolid,
    ParserElementRenderImage,
    ParserElementRenderSvg,
    ParserElementRenderText
} ParserElement;

typedef struct {
    char *name;
    ParserElement element;
} ParserElementMap;

class StringBuffer {
public:
    StringBuffer(void) { strbuf_length = 0; }
    ~StringBuffer(void);

    void addString(char *, int);
    char *getString();

    list<char *> strbuf;
    int strbuf_length;
};

class ElementHandler;

class Parser {
public:
    Parser(WaScreen *, int = 0, bool = true);
    ~Parser(void);
    
    void pushElementHandler(ElementHandler *);
    void popElementHandler(void);
    void setFilename(const char *);
    bool parseChunk(const char *, int);
    bool parseChunkEnd(void);
    bool parseFile(const char *, bool);
    bool parseCommand(const char *, bool);
    void warning(const char *, ...) __attribute__((format(printf, 2, 3)));
    char *attrGetString(Tst<char *> *, char *, char *);
    bool attrGetBool(Tst<char *> *, char *, bool);
    double attrGetDouble(Tst<char *> *, char *, double);
    int attrGetInt(Tst<char *> *, char *, int);
    unsigned int attrGetUint(Tst<char *> *, char *, unsigned int);
    double attrGetLength(Tst<char *> *, char *, double, LenghtUnitType *);
    void attrGetWindowregexContent(Tst<char *> *, WindowRegex *);
    char *attrValue(char *);
    
    list<ElementHandler *> element_handler_stack;
    XML_Parser xml_parser;
    int include_depth;
    unsigned int unknown_element_depth;
    char *filename;
    bool meawm_ng_element_found;
    WaScreen *ws;
    StringBuffer strbuf;
    Tst<char *> constants;
};

class ElementHandler : public RefCounted<ElementHandler> {
public:
    ElementHandler(Parser *);
    inline virtual ~ElementHandler(void) {}

    virtual inline ParserElement elementMap(const XML_Char *) {
        return ParserElementNone;
    }
    virtual inline bool startElement(ParserElement, const XML_Char *,
                                     Tst<char *> *) {
        return false;
    }
    virtual inline void endElement(const XML_Char *) {};
    virtual inline void characters(const XML_Char *, int) {};
    
    Parser *parser;
};

class CfgElementHandler : public ElementHandler {
public:
    inline CfgElementHandler(Parser *p) : ElementHandler(p) {}
    
    ParserElement elementMap(const XML_Char *);
    bool startElement(ParserElement, const XML_Char *, Tst<char *> *);
};

class IncludeElementHandler : public ElementHandler {
public:
    IncludeElementHandler(Parser *, ElementHandler *, bool);
    
    void endElement(const XML_Char *);
    void characters(const XML_Char *, int);

    ElementHandler *previous_handler;
    bool ignore_missing;
    StringBuffer strbuf;
};

class InheritElementHandler : public ElementHandler {
public:
    InheritElementHandler(CfgElementHandler *, Tst<char *> *);
    
    void characters(const XML_Char *, int);

protected:
    bool ignore_missing;
    StringBuffer strbuf;
};

class ActionListElementHandler : public ElementHandler {
public:
    ActionListElementHandler(CfgElementHandler *, ActionList *);
    
    ParserElement elementMap(const XML_Char *);
    bool startElement(ParserElement, const XML_Char *, Tst<char *> *);
    void endElement(const XML_Char *);
    
private:
    CfgElementHandler *cfg_handler;
    ActionList *actionlist;
};

class ActionListInheritElementHandler : public InheritElementHandler {
public:
    ActionListInheritElementHandler(CfgElementHandler *, ActionList *,
                                    Tst<char *> *);
    
    void endElement(const XML_Char *);
    
private:
    ActionList *actionlist;
};

class ActionElementHandler : public ElementHandler {
public:
    ActionElementHandler(CfgElementHandler *, ActionList *, Action *);
    
    ParserElement elementMap(const XML_Char *);
    bool startElement(ParserElement, const XML_Char *, Tst<char *> *);
    void endElement(const XML_Char *);
    
private:
    CfgElementHandler *cfg_handler;
    ActionList *actionlist;
    Action *action;
};

class MenuElementHandler : public ElementHandler {
public:
    MenuElementHandler(CfgElementHandler *, Menu *);
    
    ParserElement elementMap(const XML_Char *);
    bool startElement(ParserElement, const XML_Char *, Tst<char *> *);
    void endElement(const XML_Char *);
    
private:
    CfgElementHandler *cfg_handler;
    Menu *menu;
};

class MenuInheritElementHandler : public InheritElementHandler {
public:
    MenuInheritElementHandler(CfgElementHandler *, Menu *, Tst<char *> *);
    
    void endElement(const XML_Char *);
    
private:
    Menu *menu;
};

class MenuItemElementHandler : public ElementHandler {
public:
    MenuItemElementHandler(CfgElementHandler *, MenuItem *);
    
    ParserElement elementMap(const XML_Char *);
    bool startElement(ParserElement, const XML_Char *, Tst<char *> *);
    
private:
    CfgElementHandler *cfg_handler;
    MenuItem *menuitem;
};

class RenderGroupElementHandler : public ElementHandler {
public:
    RenderGroupElementHandler(CfgElementHandler *, RenderGroup *,
                              RenderGroup *);
    
    ParserElement elementMap(const XML_Char *);
    bool startElement(ParserElement, const XML_Char *, Tst<char *> *);
    void endElement(const XML_Char *);
    
private:
    CfgElementHandler *cfg_handler;
    RenderGroup *rendergroup, *parentgroup;
};

class RenderGroupInheritElementHandler : public InheritElementHandler {
public:
    RenderGroupInheritElementHandler(CfgElementHandler *, RenderGroup *,
                                     Tst<char *> *);
    
    void endElement(const XML_Char *);
    
private:
    RenderGroup *rendergroup;
};

class WindowElementHandler : public ElementHandler {
public:
    WindowElementHandler(CfgElementHandler *, Style *, bool);
    
    void endElement(const XML_Char *);
    void characters(const XML_Char *, int);
    
private:
    CfgElementHandler *cfg_handler;
    Style *style;
    StringBuffer strbuf;
    bool raised;
};

class RenderOpDynamicElementHandler : public ElementHandler {
public:
    RenderOpDynamicElementHandler(CfgElementHandler *, RenderGroup *,
                                  RenderOpDynamic *);
    
    ParserElement elementMap(const XML_Char *);
    bool startElement(ParserElement, const XML_Char *, Tst<char *> *);
    void endElement(const XML_Char *);
    
private:
    CfgElementHandler *cfg_handler;
    RenderGroup *rendergroup;
    RenderOpDynamic *dynamic;
};

class RenderOpDynamicTryElementHandler : public InheritElementHandler {
public:
    RenderOpDynamicTryElementHandler(CfgElementHandler *, RenderOpDynamic *,
                                     Tst<char *> *);
    
    void endElement(const XML_Char *);
    
private:
    RenderOpDynamic *dynamic;
};

class RenderPatternElementHandler : public ElementHandler {
public:
    RenderPatternElementHandler(CfgElementHandler *, RenderPattern *);
    
    ParserElement elementMap(const XML_Char *);
    bool startElement(ParserElement, const XML_Char *, Tst<char *> *);
    void endElement(const XML_Char *) {}
    
private:
    CfgElementHandler *cfg_handler;
    RenderPattern *pattern;
};

class RenderPatternInheritElementHandler : public InheritElementHandler {
public:
    RenderPatternInheritElementHandler(CfgElementHandler *, RenderPattern *,
                                       Tst<char *> *);
    
    void endElement(const XML_Char *);
    
private:
    RenderPattern *pattern;
};

class RenderPathElementHandler : public ElementHandler {
public:
    RenderPathElementHandler(CfgElementHandler *, RenderOpPath *,
                             RenderGroup *);
    
    ParserElement elementMap(const XML_Char *);
    bool startElement(ParserElement, const XML_Char *, Tst<char *> *);
    void endElement(const XML_Char *);
    
private:
    CfgElementHandler *cfg_handler;
    RenderOpPath *path;
    RenderGroup *rendergroup;
};

class RenderOpPathInheritElementHandler : public InheritElementHandler {
public:
    RenderOpPathInheritElementHandler(CfgElementHandler *, RenderOpPath *,
                                      Tst<char *> *);
    
    void endElement(const XML_Char *);
    
private:
    RenderOpPath *path;
};

class RenderOpTextElementHandler : public ElementHandler {
public:
    RenderOpTextElementHandler(CfgElementHandler *, RenderOpText *,
                               RenderGroup *);
    
    ParserElement elementMap(const XML_Char *);
    bool startElement(ParserElement, const XML_Char *, Tst<char *> *);
    void endElement(const XML_Char *);
    void characters(const XML_Char *, int);
    
private:
    CfgElementHandler *cfg_handler;
    RenderOpText *text;
    RenderGroup *rendergroup;
    StringBuffer strbuf;
};

class RenderOpTextInheritElementHandler : public InheritElementHandler {
public:
    RenderOpTextInheritElementHandler(CfgElementHandler *, StringBuffer *,
                                      Tst<char *> *);
    
    void endElement(const XML_Char *);
    
private:
    StringBuffer *textstrbuf;
};

double get_double_and_unit(char *, LenghtUnitType *);
void parser_create_tsts(void);

#endif // __Parser_hh
