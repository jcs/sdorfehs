/* our datatypes and global variables 
 *  
 * Copyright (C) 2000 Shawn Betts
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA */

#ifndef _DATA_H
#define _DATA_H

#include <X11/X.h>
#include <X11/Xlib.h>

#define FONT_HEIGHT(f) ((f)->max_bounds.ascent + (f)->max_bounds.descent)

#define STATE_UNMAPPED 0
#define STATE_MAPPED   1


typedef struct rp_window rp_window;
typedef struct screen_info screen_info;

struct rp_window
{
  screen_info *scr;
  Window w;
  int number;
  char *name;
  int state;
  int last_access;		
  int named;
  rp_window *next, *prev;  
};

struct screen_info
{
  GC normal_gc;
  GC bold_gc;
  XFontStruct *font;		/* The font we want to use. */
  XWindowAttributes root_attr;
  Window root, bar_window, key_window, input_window;
  int bar_is_raised;
  int screen_num;		/* Our screen number as dictated my X */
  Colormap def_cmap;
};

extern  rp_window *rp_window_head, *rp_window_tail;
extern  rp_window *rp_current_window;
extern screen_info *screens;
extern int num_screens;

extern Display *dpy;
extern Atom rp_restart;
extern Atom rp_kill;

extern Atom wm_state;
extern Atom wm_change_state;
extern Atom wm_protocols;
extern Atom wm_delete;
extern Atom wm_take_focus;
extern Atom wm_colormaps;

/* Set to 1 to indicate that the WM should exit at it's earliest
   convenience. */
extern int exit_signal;

/* Arguments passed to ratpoison. */
extern char **myargv;

#endif /* _DATA_H */
