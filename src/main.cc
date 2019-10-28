/* main.cc

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

}

#include "Meawm_NG.hh"

void usage(void);
void help(void);
void version(void);

char *program_name;

static const struct argument_def {
    const char* const name;
    const char* const short_name;
    const char* const short_description;
    const char* const description;
    void (*function)(void);
    const char* const default_value;
} argument_map[] = {
    { "--display", 0L, "[--display=DISPLAYNAME] ",
      "      --display=DISPLAYNAME    X server to contact", 0L, 0L },
    { "--rcfile", 0L, "[--rcfile=FILE]\n    ",
      "      --rcfile=RCFILE          Resource-file to use", 0L, 0L },
    { "--screen-mask", 0L, "[--screen-mask=SCREENS] ",
      "      --screen-mask=SCREENS    Screens to manage", 0L, 0L },
    { "--script-dir", 0L, "[--script-dir=PATH] ",
      "      --script-dir=PATH        Path to script directory", 0L, 0L },
    { "--double-click", 0L, "[--double-click=INTERVAL]\n    ",
      "      --double-click=INTERVAL  Double-click interval in milliseconds",
      0L, 0L },
    { "--client-side", "-c", "[-c] [--client-side] ",
      "  -c, --client-side            Force client-side rendering", 0L,
      "true" },

#ifdef    RENDER
    { "--argb-visual", "-a", "[-a] [--argb-visual] ",
      "  -a, --argb-visual            Use ARGB visual if available", 0L,
      "true" },
#endif // RENDER
    
#ifdef    THREAD
    { "--thread", "-t", "[-t] [--thread] ",
      "   -t, --thread               Use threading", 0L, "false" },
    { "--thread-priority", 0L, "[--thread-priority=VALUE]\n    ",
      "      --thread-priority=VALUE  Rendering thread priority value",
      0L, 0L },
#endif // THREAD
    
    { "--actionfile", 0L, "[--actionfile=FILE]\n    ",
      "      --actionfile=FILE        Action-file to use", 0L, 0L },
    { "--stylefile", 0L, "[--stylefile=FILE] ",
      "      --stylefile=FILE         Style-file to use", 0L, 0L },
    { "--menufile", 0L, "[--menufile=FILE] ",
      "      --menufile=FILE          Menu-file to use", 0L, 0L },
    { "--desktops", 0L, "[--desktops=NUMBER]\n    ",
      "      --desktops=NUMBER        Number of desktops", 0L, 0L },
    { "--desktop-names", 0L, "[--desktop-names=LIST] ",
      "      --desktop-names=LIST     Comma seperated list of desktops",
      0L, 0L },
    { "--virtual-size", 0L, "[--virtual-size=SIZE] ",
      "      --virtual-size=SIZE      Virtual desktop size, e.g. '3x3'",
      0L, 0L },
    { "--menu-stacking", 0L, "[--menu-stacking=TYPE]\n    ",
      "      --menu-stacking=TYPE     Menu stacking type, e.g. 'AlwaysOnTop'",
      0L, 0L },
    { "--dh-stacking", 0L, "[--dh-stacking=TYPE] ",
      "      --dh-stacking=TYPE       Dockapp-holder stacking type",
      0L, 0L },
    { "--transient-not-above", "-t", "[-t] [--transient-not-above] ",
      "  -t, --transient-not-above    Do not keep transient windows above",
      0L, "true" },
    { "--revert-to-root", "-r", "[-r] [--revert-to-root]\n    ",
      "  -r, --revert-to-root         Revert focus to window",
      0L, "true" },
    { "--external-bg", "-e", "[-e] [--external-bg] ",
      "  -e, --external-bg            Use external background",
      0L, "true" },
    { "--info-command", 0L, "[--info-command=COMMAND] ",
      "      --info-command           Info command",
      0L, 0L },
    { "--warning-command", 0L, "[--warning-command=COMMAND]\n    ",
      "      --warning-command        Warning command",
      0L, 0L },
    { "--usage", 0L, "[--usage] ",
      "      --usage                  Display brief usage message", usage,
      0L },
    { "--help", 0L, "[--help] ",
      "      --help                   Show this help message", help, 0L },
    { "--version", 0L, "[--version]",
      "      --version                Output version information and exit",
      version, 0L }
};

void parse_arguments(int argc, char **argv, char **values) {

    int i, j, size = sizeof(argument_map) /
        sizeof(struct argument_def);

    for (i = 1; i < argc; i++) {
        for (j = 0; j < size; j++) {
            int match = 0;
            int len = strlen(argument_map[j].name);
            if (! strncmp(argv[i], argument_map[j].name, len))
                match = 1;
            else if (argument_map[j].short_name) {
                len = strlen(argument_map[j].short_name);
                if (! strncmp(argv[i], argument_map[j].short_name, len))
                    match = 1;
            }

            if (match) {
                if (argument_map[j].function) {
                    (argument_map[j].function)();
                    exit(0);
                } else {
                    if (*(argv[i] + len) == '\0') {
                        if (argument_map[j].default_value)
                            values[j] = (char*) argument_map[j].default_value;
                        else {
                            if (i + 1 < argc) values[j] = argv[++i];
                            else {
                                fprintf(stderr, "%s: option `%s' requires "
                                        "an argument\n",
                                        program_name, argv[i]);
                                exit(1);
                            }
                        }
                    } else if (*(argv[i] + len) == '=' &&
                               (int) strlen(argv[i]) >= (len + 1)) {
                        values[j] = argv[i] + len + 1;
                    } else
                        continue;
                }
                break;
            }
        }
        if (j == size) {
            fprintf(stderr, "%s: unrecognized option `%s'\n", program_name,
                    argv[i]); usage(); exit(1);
        }
    }
}

int main(int argc, char **argv) {
    XEvent e;
    char *arg_values[sizeof(argument_map) / sizeof(struct argument_def)];
    memset(arg_values, 0, sizeof(arg_values));

    program_name = WA_STRDUP(argv[0]);

    parse_arguments(argc, argv, arg_values);
    
    delete [] program_name;

    Meawm_NG *meawm_ng = new Meawm_NG(argv, arg_values);
    meawm_ng->eh->eventLoop(&meawm_ng->eh->empty_return_mask, &e);
    
    exit(1);
}

void usage(void) {
    unsigned int i, size = sizeof(argument_map) / sizeof(struct argument_def);
    cout << "Usage: " << program_name << " ";
    for (i = 0; i < size; i++)
        cout << argument_map[i].short_description;

    cout << endl << endl << "Type " << program_name <<
        " --help for a full description." << endl << endl;
}

void help(void) {
    unsigned int i, size = sizeof(argument_map) / sizeof(struct argument_def);

    cout << "Usage: " << program_name << " [OPTION...]" << endl;
    cout << "meawm_ng - an X11 window manager" << endl <<
        endl;
    
    for (i = 0; i < size; i++)
        cout << argument_map[i].description << endl;

    cout << endl << "Features compiled in:" << endl;
    cout << "   " <<

#ifdef   THREAD
    "thread " 
#endif // THREAD

#ifdef    SHAPE
    "shape "
#endif // SHAPE

#ifdef    XINERAMA
    "xinerama "
#endif // XINERAMA

#ifdef    RENDER
    "render "
#endif // RENDER

#ifdef    RANDR
    "randr "
#endif // RANDR

#ifdef    PNG
    "png "
#endif // PNG

#ifdef    SVG
    "svg "
#endif // SVG

#ifdef    XCURSOR
    "xcursor "
#endif // XCURSOR

        "" << endl << endl;
    
    cout << "Report bugs to <david@meawm_ng.org>." << endl;
}

void version(void) {
    cout << PACKAGE << " " << VERSION << endl;
}
