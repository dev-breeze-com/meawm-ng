/* Regex.cc

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

#include "Regex.hh"
#include "Meawm_NG.hh"

Regex::Regex(char *_pattern) {
    int status = 0;
    char *err_msg = NULL;

    if (_pattern == NULL) {
        comp_ok = false;
        return;
    }
    
    comp_ok = true;

    char *pattern = WA_STRDUP(_pattern);
    
    for (int i = 0; pattern[i] != '\0'; i++) {
        int n;
        if (pattern[i] == '\\' && pattern[i + 1] == '/') {
            for (n = 1; pattern[i + n] != '\0'; n++)
                pattern[i + n - 1] = pattern[i + n];
            pattern[i + n - 1] = '\0';
        }
    }
    if ((status = regcomp(&regexp, pattern, REG_NOSUB)) != 0) {
        if (status == REG_ESPACE)
            WARNING << "memory allocation error" << endl;
        else {
            int err_msg_sz = regerror(status, &regexp, NULL, (size_t) 0);
            if ((err_msg = (char *) malloc(err_msg_sz)) != NULL) {
                regerror(status, &regexp, err_msg, err_msg_sz );
                WARNING << err_msg << " = " << pattern << endl;
                free(err_msg);
            } else {
                WARNING << "invalid regular expression = " << pattern << endl;
            }
        }
        comp_ok = false;
    }

    delete [] pattern;
}

bool Regex::match(char *str) {
    int status = 0;
    char *err_msg = NULL;

    if (! comp_ok) return false;
    
    status = regexec(&regexp, str, (size_t) 0, NULL, 0);

    if (status == REG_NOMATCH)
        return false;
    else if (status != 0) {
        if (status == REG_ESPACE)
            WARNING << "memory allocation error" << endl;
        else {
            int err_msg_sz = regerror(status, &regexp, NULL, (size_t) 0);
            if ((err_msg = (char *) malloc(err_msg_sz)) != NULL) {
                regerror(status, &regexp, err_msg, err_msg_sz );
                WARNING << err_msg << endl;
                free(err_msg);
            } else {
                WARNING << "regexec error " << endl;
            }
        }
        return false;
    }
    return true;
}

WindowRegex::WindowRegex(int _state, char *_window_regex) {
    if (_window_regex) window_regex = new Regex(_window_regex);
    else window_regex = NULL;
    state = _state;
}

WindowRegex::~WindowRegex(void) {
    map<WindowIDType, Regex *>::iterator it = id_regex_map.begin();
    for (; it != id_regex_map.end(); it++) {
        delete ((*it).second);
    }
    id_regex_map.clear();
    if (window_regex) delete window_regex;
}

void WindowRegex::addIDRegex(WindowIDType type, char *_regex) {
    if (_regex) {
        Regex *r = new Regex(_regex);
        if (r) id_regex_map.insert(make_pair(type, r));
    }
}

bool WindowRegex::match(WaStringMap *ids, int _state, char *window) {
    if (_state != state) return false;
    bool ismatching = true;
    map<WindowIDType, Regex *>::iterator r_it = id_regex_map.begin();
    for (; r_it != id_regex_map.end(); r_it++) {
        map<int, char *>::iterator ids_it;
        if ((ids_it = ids->find((WindowIDType) (*r_it).first)) != ids->end()) {
            if (! ((*r_it).second)->match(((*ids_it).second)))
                ismatching = false;
        } else
            ismatching = false;
    }
    if (ismatching) {
        if (window_regex) {
            if (window) {
                if (window_regex->match(window)) {
                    return true;
                }
            }
            return false;
        } else
            return true;
    }
    return false;
}
