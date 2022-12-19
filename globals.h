/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include "data.h"

/* codes used in the cmdret code in actions.c */
#define RET_SUCCESS 1
#define RET_FAILURE 0

#define FONT_HEIGHT(s) ((s)->xft_font->ascent + (s)->xft_font->descent)
#define FONT_ASCENT(s) ((s)->xft_font->ascent)
#define FONT_DESCENT(s) ((s)->xft_font->descent)

#define MAX_FONT_WIDTH(f) (rp_font_width)

#define WIN_EVENTS (StructureNotifyMask | PropertyChangeMask | \
    ColormapChangeMask | FocusChangeMask)

/*
 * EMPTY is used when a frame doesn't contain a window, or a window doesn't
 * have a frame. Any time a field refers to the number of a
 * window/frame/screen/etc, Use EMPTY to denote a lack there of.
 */
#define EMPTY -1

/* Possible values for defaults.window_list_style */
#define STYLE_ROW    0
#define STYLE_COLUMN 1

/* Possible values for defaults.win_name */
#define WIN_NAME_TITLE          0
#define WIN_NAME_RES_CLASS      1
#define WIN_NAME_RES_NAME       2

/* Possible directions to traverse the completions list. */
#define COMPLETION_NEXT         0
#define COMPLETION_PREVIOUS     1

/* Font styles */
#define STYLE_NORMAL  0
#define STYLE_INVERSE 1

/* Whether or not we support xrandr */
extern int rp_have_xrandr;

/*
 * Each child process is stored in this list. spawn, creates a new entry in
 * this list, the SIGCHLD handler sets child.terminated to be true and
 * handle_signals in events.c processes each terminated process by printing a
 * message saying the process ended and displaying it's exit code.
 */
extern struct list_head rp_children;

extern struct rp_defaults defaults;

/* Cached font info. */
extern int rp_font_ascent, rp_font_descent, rp_font_width;

/* The prefix key also known as the command character under screen. */
extern struct rp_key prefix_key;

/*
 * A list of mapped windows. These windows show up in the window list and have
 * a number assigned to them.
 */
extern struct list_head rp_mapped_window;

/*
 * A list of unmapped windows. These windows do not have a number assigned to
 * them and are not visible/active.
 */
extern struct list_head rp_unmapped_window;

/* The list of screens. */
extern struct list_head rp_screens;
extern rp_screen *rp_current_screen;
extern rp_global_screen rp_glob_screen;

extern Display *dpy;

extern XEvent rp_current_event;

extern Atom rp_selection;

extern Atom wm_name;
extern Atom wm_state;
extern Atom wm_change_state;
extern Atom wm_protocols;
extern Atom wm_delete;
extern Atom wm_take_focus;
extern Atom wm_colormaps;

/* TEXT atoms */
extern Atom xa_string;
extern Atom xa_compound_text;
extern Atom xa_utf8_string;

/* netwm atoms. */
extern Atom _net_active_window;
extern Atom _net_client_list;
extern Atom _net_client_list_stacking;
extern Atom _net_current_desktop;
extern Atom _net_number_of_desktops;
extern Atom _net_supported;
extern Atom _net_workarea;
extern Atom _net_wm_name;
extern Atom _net_wm_pid;
extern Atom _net_wm_state;
#define _NET_WM_STATE_REMOVE	0    /* remove/unset property */
#define _NET_WM_STATE_ADD	1    /* add/set property */
#define _NET_WM_STATE_TOGGLE	2    /* toggle property  */
extern Atom _net_wm_state_fullscreen;
extern Atom _net_wm_window_type;
extern Atom _net_wm_window_type_dialog;
extern Atom _net_wm_window_type_dock;
extern Atom _net_wm_window_type_splash;
extern Atom _net_wm_window_type_tooltip;
extern Atom _net_wm_window_type_utility;
extern Atom _net_supporting_wm_check;

/*
 * When unmapping or deleting windows, it is sometimes helpful to ignore a bad
 * window when attempting to clean the window up. This does just that when set
 * to 1
 */
extern int ignore_badwindow;

/* Arguments passed at startup. */
extern char **myargv;

/* Keeps track of which mod mask each modifier is under. */
extern struct modifier_info rp_modifier_info;

/*
 * nonzero if an alarm signal was raised. This means we should hide our
 * popup windows.
 */
extern int alarm_signalled;
extern int kill_signalled;
extern int hup_signalled;
extern int chld_signalled;

/* rudeness levels */
extern int rp_honour_transient_raise;
extern int rp_honour_normal_raise;
extern int rp_honour_transient_map;
extern int rp_honour_normal_map;
extern int rp_honour_vscreen_switch;

/* Keep track of X11 error messages. */
extern char *rp_error_msg;

/* Number sets for windows. */
extern struct numset *rp_window_numset;

extern struct list_head rp_key_hook;
extern struct list_head rp_switch_win_hook;
extern struct list_head rp_switch_frame_hook;
extern struct list_head rp_switch_screen_hook;
extern struct list_head rp_switch_vscreen_hook;
extern struct list_head rp_delete_window_hook;
extern struct list_head rp_quit_hook;
extern struct list_head rp_restart_hook;
extern struct list_head rp_new_window_hook;
extern struct list_head rp_title_changed_hook;

extern struct rp_hook_db_entry rp_hook_db[];

void set_rp_window_focus(rp_window * win);
void set_window_focus(Window window);

extern struct numset *rp_frame_numset;

/* Selection handling globals */
extern rp_xselection selection;
void set_selection(char *txt);
void set_nselection(char *txt, int len);
char *get_selection(void);

/* Wrapper font functions to support Xft */

XftFont *rp_get_font(rp_screen *s, char *font);
void rp_clear_cached_fonts(rp_screen *s);
void rp_draw_string(rp_screen *s, Drawable d, int style, int x, int y,
    char *string, int length, char *font, char *color);
int rp_text_width(rp_screen *s, char *string, int count, char *font);

void check_child_procs(void);
void chld_handler(int signum);
void set_sig_handler(int sig, void (*action)(int));
void set_close_on_exec(int fd);
void read_rc_file(FILE *file);
const char *get_homedir(void);
char *get_config_dir(void);
void clean_up(void);

void register_atom(Atom *a, char *name);
int set_atom(Window w, Atom a, Atom type, unsigned long *val,
    unsigned long nitems);
int append_atom(Window w, Atom a, Atom type, unsigned long *val,
    unsigned long nitems);
unsigned long get_atom(Window w, Atom a, Atom type, unsigned long off,
    unsigned long *ret, unsigned long nitems, unsigned long *left);
void remove_atom(Window w, Atom a, Atom type, unsigned long remove);

#endif
