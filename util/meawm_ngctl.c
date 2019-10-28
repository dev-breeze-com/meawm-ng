/*
 * Copyright © 2003 David Reveman
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Reveman not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Reveman makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * DAVID REVEMAN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID REVEMAN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
    
#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif /* HAVE_STDIO_H */

#ifdef    HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif /* STDC_HEADERS */

void usage(void);
void help(void);
void version(void);

static const struct argument_def {
    char *name;
    char *short_name;
    char *short_description;
    char *description;
    void (*function)(void);
    char *default_value;
} argument_map[] = {
    { "--display", NULL, "[--display=DISPLAYNAME] ",
      "      --display=DISPLAYNAME    X server to contact", NULL, NULL },
    { "--usage", NULL, "[--usage] ",
      "      --usage                  Display brief usage message", usage,
      NULL },
    { "--help", NULL, "[--help] ",
      "      --help                   Show this help message", help, NULL },
    { "--version", NULL, "[--version]",
      "      --version                Output version information and exit",
      version, NULL }
};

char *program_name;

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
                            values[j] = argument_map[j].default_value;
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

int screen_handled_by_meawm_ng(Window);

Display *display;
Atom utf8_string, net_supporting_wm_check, net_wm_name;

#define INC_SIZE 8192

int main(int argc, char **argv) {
    char data[INC_SIZE];
    int i, initial = 1;
    Window window, root;
    int screen;
    Atom meawm_ng_net_cfg, meawm_ng_net_cfg_data;
    char *arg_values[sizeof(argument_map) / sizeof(struct argument_def)];

    program_name = argv[0];
    memset(&arg_values, 0, sizeof(argument_map) / sizeof(struct argument_def) *
           sizeof(char *));

    parse_arguments(argc, argv, arg_values);

    /* we have to open a connection to the display to be able to communicate
       with meawm_ng. */
    if (! (display = XOpenDisplay(arg_values[0]))) {
        fprintf(stderr, "error: can't open display: %s\n", arg_values[0]);
        exit(1);
    }
    
    /* data must be in UTF-8 encoding, this program leaves it to the user to
       make sure that data is in the right encoding. */
    utf8_string = XInternAtom(display, "UTF8_STRING", 0);

    /* atoms used for checking that meawm_ng is handling the screen. */
    net_supporting_wm_check =
        XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", 0);
    net_wm_name =
        XInternAtom(display, "_NET_WM_NAME", 0);

    /* client message atom. */
    meawm_ng_net_cfg = XInternAtom(display, "_WAIMEA_NET_CFG", 0);

    /* meawm_ng doesn't care about the name of this hint, the one we tell
       meawm_ng to use will be used. */
    meawm_ng_net_cfg_data = XInternAtom(display, "_WAIMEA_NET_CFG_DATA", 0);

    /* screen that we which to control. */
    screen = DefaultScreen(display);

    /* and root window of the screen. */
    root = RootWindow(display, screen);

    /* so that can be notified when meawm_ng is starting to handle screen. */
    XSelectInput(display, root, PropertyChangeMask);

    /* create window used for sending data. */
    window = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, 0, 0);

    /* so that we receive notify events when meawm_ng deletes the property. */
    XSelectInput(display, window, PropertyChangeMask);

    /* ok lets read some data from stdin and send it to meawm_ng.
       meawm_ng support incremental transfer of data which is good because
       the data we are about to send may be to large to store in a single
       property, we don't have to buffer all the data that we want to send.
       if you don't which to send data incrementally just set the
       data.l[2] field in the client message to zero. */
    do {
        XEvent event;

        /* read some data to our buffer. */
        i = fread(data, sizeof(char), INC_SIZE, stdin);

        /* set the property the we use for data transfers to the data
           we just read. */
        XChangeProperty(display, window, meawm_ng_net_cfg_data,
                        utf8_string, 8, PropModeAppend,
                        (unsigned char *) data, i);

        /* is this the first chunk we are about to transfer, then we must
           send the client message event to the root screen so that meawm_ng
           starts receiving the data. */
        while (initial) {
            XClientMessageEvent cme;

            /* this loop makes sure that meawm_ng is handling the screen
               the are about to send configuration data to */
            while (! screen_handled_by_meawm_ng(root)) {
                XWindowEvent(display, root, PropertyChangeMask, &event);
            }
            
            cme.type = ClientMessage;
            cme.display = display;
            cme.format = 32;

            /* important that this hint is really right, otherwise meawm_ng
               wont do a thing. */
            cme.message_type = meawm_ng_net_cfg;

            /* must be the root window. */
            cme.window = root;
            
            /* this must be the window that we are to use for the transfer. */
            cme.data.l[0] = window;

            /* our hint. */
            cme.data.l[1] = meawm_ng_net_cfg_data;

            /* and the incremental transfer size. */
            cme.data.l[2] = INC_SIZE;

            /* (reserved). */
            cme.data.l[3] = cme.data.l[4] = 0;

            /* send it to the root window. */
            XSendEvent(display, root, 0, StructureNotifyMask,
                       (XEvent *) &cme);

            /* make sure meawm_ng is still running. otherwise we do another run
               in this loop */
            if (screen_handled_by_meawm_ng(root))
                initial = 0;
        }

        do {
            /* wait for meawm_ng to respond, by deleting the property.
               the deletion event is our ack it tells us that meawm_ng
               has received the data. */
            XWindowEvent(display, window, PropertyChangeMask, &event);
        } while (event.xproperty.state != PropertyDelete);

        /* more data to transfer? */
    } while (i == INC_SIZE);

    /* end the incremental transfer by sending zero length data. */
    XChangeProperty(display, window, meawm_ng_net_cfg_data,
                    utf8_string, 8, PropModeAppend,
                    (unsigned char *) data, 0);

    /* ok we are done lets close the display connection and exit. */
    XCloseDisplay(display);
    exit(0);
}

/* checks that meawm_ng is really running on the given screen. */
int screen_handled_by_meawm_ng(Window root) {
    int real_format;
    Atom real_type;
    unsigned long items_read, items_left;
    CARD32 *data;

    int screen_handled_by_meawm_ng = 0;

    if (XGetWindowProperty(display, root, net_supporting_wm_check,
                           0L, 1L, 0, XA_WINDOW, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) {
        Window check_window = *data;
        XFree(data);

        if (XGetWindowProperty(display, check_window, net_wm_name,
                               0L, 6L, 0, utf8_string, &real_type,
                               &real_format, &items_read, &items_left, 
                               (unsigned char **) &data) == Success && 
            items_read == 6) {

            if (! strncmp((char *) data, "meawm_ng", 6)) 
                screen_handled_by_meawm_ng = 1;
            
            XFree(data);
        }
    }
    
    return screen_handled_by_meawm_ng;
}

void usage(void) {
    unsigned int i, size = sizeof(argument_map) / sizeof(struct 
                                                         argument_def);

    printf("Usage: %s ", program_name);
    for (i = 0; i < size; i++) {
        printf("%s", argument_map[i].short_description);
    }
    printf("\n\nType %s --help for a full description.\n\n", program_name);
}

void help(void) {
    unsigned int i, size = sizeof(argument_map) / sizeof(struct 
                                                         argument_def);

    printf("Usage: %s [OPTION...]\n", program_name);
    printf("meawm_ng-control - sends configuration data to meawm_ng\n\n");
    for (i = 0; i < size; i++) {
        printf("%s\n", argument_map[i].description);
    }
    printf("\nReport bugs to <david@meawm_ng.org>.\n");
}

void version(void) {
    printf("meawm_ng-control %s\n", VERSION);
}
