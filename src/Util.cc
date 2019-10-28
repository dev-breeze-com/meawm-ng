/* Util.cc

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
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#  include <stdarg.h>
#endif // STDC_HEADERS    

}

#define IS_ENV_CHAR(ch) (isalnum(ch) || ch == '_')

#include "Util.hh"

WaStringMap::WaStringMap(int key, const char *str) :
    RefCounted<WaStringMap>(this) {
    char *newstr;
    if (str) {
        newstr = new char[strlen(str) + 1];
        strcpy(newstr, str);
    } else {
        newstr = new char[1];
        *newstr = '\0';
    }
    str_map.insert(make_pair(key, newstr));
}

WaStringMap::~WaStringMap(void) {
    map<int, char *>::iterator it = str_map.begin();
    for (; it != str_map.end(); it++) {
        delete [] ((*it).second);
    }
    str_map.clear();
}

void WaStringMap::add(int key, const char *str, ...) {
    va_list arglist;
    char buffer[8192];
    
    if (! str) return;    
    
    va_start(arglist, str);
    vsnprintf(buffer, 8192, str, arglist);
    va_end(arglist);

    str_map.insert(make_pair(key, WA_STRDUP(buffer)));
}

char *environment_expansion(char *s) {
    char *tmp, *env, *env_name;
    int i, tmp_char;
    
    for (i = 0; s[i] != '\0'; i++) {
        switch (s[i]) {
            case '\\':
                if (s[i + 1] != '\0') i++;
                break;
            case '$':
                if (IS_ENV_CHAR(s[i + 1])) {
                    s[i] = '\0';
                    env_name = &s[++i];
                    for (; IS_ENV_CHAR(s[i]); i++);
                    tmp_char = s[i];
                    s[i] = '\0';
                    if ((env = getenv(env_name)) == NULL) env = "";
                    s[i] = tmp_char;
                    tmp = new char[strlen(s) + strlen(env) +
                                   strlen(&s[i]) + 1];
                    sprintf(tmp, "%s%s%s", s, env, &s[i]);
                    i = strlen(s) + strlen(env);
                    delete [] s;
                    s = tmp;
                }
                break;
            case '~': 
                s[i] = '\0';
                if ((env = getenv("HOME")) == NULL) env = "~";
                tmp = new char[strlen(s) + strlen(env) +
                               strlen(&s[i + 1]) + 1];
                sprintf(tmp, "%s%s%s", s, env, &s[i + 1]);
                i = strlen(s) + strlen(env);
                delete [] s;
                s = tmp;
                break;
        }
    }
    return s;
}

void commandline_to_argv(char *s, char **tmp_argv, int max_argv) {
    int i;

    for (i = 0; i < (max_argv - 1);) {
        for (; *s == ' ' || *s == '\t'; s++);
        if (*s == '"') {
            tmp_argv[i++] = ++s;
            for (; *s != '"' && *s != '\0'; s++);
            if (*s == '\0') break;
            *s = '\0';
            s++;
            if (*s == '\0') break;
        }
        else {
            tmp_argv[i++] = s;
            for (; *s != ' ' && *s != '\t' && *s != '\0'; s++);
            if (*s == '\0') break;
            *s = '\0';
            s++;
        }
    }
    
    tmp_argv[i] = NULL;
}

char *smartfile(const char *name, char *configfile, bool warn) {
    FILE *fd;
    if (! name) return NULL;

    char *ename = environment_expansion(WA_STRDUP((char *) name));

    if (configfile) {
        bool path_ok = false;
        char *path = strrchr(configfile, '/');
        if (path) {
            path[0] = '\0';
            path_ok = true;
        } else
            configfile = "";
    
        char *configpath_and_file = new char[strlen(configfile) +
                                           strlen(ename) + 2];
        sprintf(configpath_and_file, "%s/%s", configfile, ename);
        if (path_ok) path[0] = '/';
        
        if ((fd = fopen(configpath_and_file, "r"))) {
            fclose(fd);
            delete [] ename;
            return configpath_and_file;
        }
        delete [] configpath_and_file;
    }
    
    if ((fd = fopen(ename, "r"))) {
        fclose(fd);
        return ename;
    } else if (warn)
        WARNING << "unable to open file: `" << ename << "'" << endl;

    delete [] ename;
    return NULL;
}
