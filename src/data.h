/* our datatypes and global variables 
 * Copyright (C) 2000, 2001 Shawn Betts
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

#ifndef _RATPOISON_DATA_H
#define _RATPOISON_DATA_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define FONT_HEIGHT(f) ((f)->max_bounds.ascent + (f)->max_bounds.descent)

#define STATE_UNMAPPED 0
#define STATE_MAPPED   1


typedef struct rp_window rp_window;
typedef struct screen_info screen_info;
typedef struct rp_action rp_action;
typedef struct rp_window_frame rp_window_frame;

struct rp_window_frame
{
  int x, y, width, height;
  rp_window *win;

  rp_window_frame *prev, *next;
};


struct rp_window
{
  screen_info *scr;
  Window w;
  int number;
  char *name;
  int state;
  int last_access;		
  int named;
  
  /* Dimensions */
  int x, y, width, height, border;

  /* WM Hints */
  XSizeHints *hints;

  /* Colormap */
  Colormap colormap;

  /* Is this a transient window? */
  int transient;
  Window transient_for;

  /* Saved mouse position */
  int mouse_x, mouse_y;

  rp_window *next, *prev;  
};

struct screen_info
{
  GC normal_gc;
  XFontStruct *font;		/* The font we want to use. */
  XWindowAttributes root_attr;
  Window root, bar_window, key_window, input_window, frame_window;
  int bar_is_raised;
  int screen_num;		/* Our screen number as dictated my X */
  Colormap def_cmap;
};

struct rp_action
{
  KeySym key;
  unsigned int state;
  void *data;			/* misc data to be passed to the function */
/*   void (*func)(void *); */
};

struct rp_key
{
  KeySym sym;
  unsigned int state;
};

/* The prefix key also known as the command character under screen. */
extern struct rp_key prefix_key;

/* These _sentinel pointers point to a special rp_window whose next
   pointer points to the head of the list and whose prev pointer
   points to the tail. This is done to simplify removing an element
   from the list: since there are no head and tail global variable
   pointers that need to be updated, functions that manipulate
   elements in a list don't need to know what list they belong to. The
   last element's next pointer points to the sentinel as does the
   first element's prev pointer. An empty list is represented by the
   sentinel's prev and next pointers pointing to itself. */

/* A list of mapped windows. These windows show up in the window
   list and have a number assigned to them. */
extern rp_window *rp_mapped_window_sentinel;

/* A list of unmapped windows. These windows do not have a number
   assigned to them and are not visible/active. */
extern rp_window *rp_unmapped_window_sentinel;

/* A list of frames that may or may not contain windows. There should
   always be one in the list. */
extern rp_window_frame *rp_window_frame_sentinel;

/* Pointer to the currently focused frame. */
extern rp_window_frame *rp_current_frame;

extern screen_info *screens;
extern int num_screens;

extern XEvent *rp_current_event;

extern Display *dpy;
extern Atom rp_restart;
extern Atom rp_kill;
extern Atom rp_command;
extern Atom rp_command_request;
extern Atom rp_command_result;


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

struct modifier_info
{
/*   unsigned int mode_switch_mask; */
  unsigned int meta_mod_mask;
  unsigned int alt_mod_mask;
  unsigned int super_mod_mask;
  unsigned int hyper_mod_mask;
};

/* Keeps track of which mod mask each modifier is under. */
extern struct modifier_info rp_modifier_info;

#endif /* _RATPOISON_DATA_H */
