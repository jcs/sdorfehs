/* Copyright (C) 2000-2004 Shawn Betts <sabetts@vcn.bc.ca>
 *
 * This file is part of ratpoison.
 *
 * ratpoison is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * ratpoison is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include "data.h"

#define FONT_HEIGHT(f) ((f)->max_bounds.ascent + (f)->max_bounds.descent)
#define MAX_FONT_WIDTH(f) ((f)->max_bounds.width)

#define WIN_EVENTS (StructureNotifyMask | PropertyChangeMask | ColormapChangeMask | FocusChangeMask)
/* EMPTY is used when a frame doesn't contain a window, or a window
   doesn't have a frame. Any time a field refers to the number of a
   window/frame/screen/etc, Use EMPTY to denote a lack there of. */
#define EMPTY -1

/* Possible values for defaults.window_list_style */
#define STYLE_ROW    0 
#define STYLE_COLUMN 1

/* Possible values for defaults.win_name */
#define WIN_NAME_TITLE 		0
#define WIN_NAME_RES_CLASS 	1
#define WIN_NAME_RES_NAME 	2

/* Possible directions to traverse the completions list. */
#define COMPLETION_NEXT		0
#define COMPLETION_PREVIOUS 	1

/* Error codes for group_delete_group() */
#define GROUP_DELETE_GROUP_OK         0
#define GROUP_DELETE_GROUP_NONEMPTY   1

/* The list of groups. */
extern struct list_head rp_groups;

extern rp_group *rp_current_group;

/* Each child process is stored in this list. spawn, creates a new
   entry in this list, the SIGCHLD handler sets child.terminated to be
   true and handle_signals in events.c processes each terminated
   process by printing a message saying the process ended and
   displaying it's exit code. */
extern struct list_head rp_children;

extern struct rp_defaults defaults;

/* The prefix key also known as the command character under screen. */
extern struct rp_key prefix_key;

/* A list of mapped windows. These windows show up in the window
   list and have a number assigned to them. */
extern struct list_head rp_mapped_window;

/* A list of unmapped windows. These windows do not have a number
   assigned to them and are not visible/active. */
extern struct list_head rp_unmapped_window;

extern int rp_current_screen;
extern rp_screen *screens;
extern int num_screens;

extern XEvent rp_current_event;

extern Display *dpy;
extern Atom rp_command;
extern Atom rp_command_request;
extern Atom rp_command_result;
extern Atom rp_selection;

extern Atom wm_name;
extern Atom wm_state;
extern Atom wm_change_state;
extern Atom wm_protocols;
extern Atom wm_delete;
extern Atom wm_take_focus;
extern Atom wm_colormaps;

/* mouse properties */
extern int rat_x;
extern int rat_y;
extern int rat_visible;

/* When unmapping or deleting windows, it is sometimes helpful to
   ignore a bad window when attempting to clean the window up. This
   does just that when set to 1 */
extern int ignore_badwindow;

/* Arguments passed to ratpoison. */
extern char **myargv;

/* Keeps track of which mod mask each modifier is under. */
extern struct modifier_info rp_modifier_info;

/* nonzero if an alarm signal was raised. This means ratpoison should
   hide its popup windows. */
extern int alarm_signalled;
extern int kill_signalled;
extern int hup_signalled;
extern int chld_signalled;

/* rudeness levels */
extern int rp_honour_transient_raise;
extern int rp_honour_normal_raise;
extern int rp_honour_transient_map;
extern int rp_honour_normal_map;

/* Keep track of X11 error messages. */
extern char *rp_error_msg;

/* Number sets for windows. */
extern struct numset *rp_window_numset;

extern struct list_head rp_key_hook;
extern struct list_head rp_switch_win_hook;
extern struct list_head rp_switch_frame_hook;
extern struct list_head rp_switch_group_hook;
extern struct list_head rp_quit_hook;
extern struct list_head rp_restart_hook;

extern struct rp_hook_db_entry rp_hook_db[];

void set_rp_window_focus (rp_window *win);
void set_window_focus (Window window);

extern int rp_have_xinerama;
extern int xine_screen_count;

extern struct numset *rp_frame_numset;

extern struct list_head rp_frame_undos;
extern int rp_num_frame_undos;
#endif
