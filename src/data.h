/* our datatypes and global variables 
 * Copyright (C) 2000, 2001, 2002, 2003 Shawn Betts
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

#include "linkedlist.h"
#include "number.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct rp_window rp_window;
typedef struct rp_screen rp_screen;
typedef struct rp_action rp_action;
typedef struct rp_keymap rp_keymap;
typedef struct rp_frame rp_frame;
typedef struct rp_child_info rp_child_info;
typedef struct rp_group rp_group;
typedef struct rp_window_elem rp_window_elem;
typedef struct rp_completions rp_completions;
typedef struct rp_input_line rp_input_line;

struct rp_frame
{
  int number;
  int x, y, width, height;

  /* The number of the window that is focused in this frame. */
  int win_number;

  /* For determining the last frame. */
  int last_access;
  
  struct list_head node;
};

struct rp_window
{
  rp_screen *scr;
  Window w;
  int state;
  int last_access;
  int named;

  /* A number uniquely identifying this window. This is a different
     number than the one given to it by the group it is in. This
     number is used for internal purposes, whereas the group number is
     what the user sees. */
  int number;

  /* Window name hints. */
  char *user_name;
  char *wm_name;
  char *res_name;
  char *res_class;

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

  /* The alignment of the window. Decides to what side or corner the
     window sticks to. */
  int gravity;

  /* A window can be visible inside a frame but not the frame's
     current window. This keeps track of what frame the window was
     mapped into. */
  int frame_number;

  struct list_head node;
};

struct rp_window_elem
{
  rp_window *win;
  int number;
  struct list_head node;
};

/* An rp_group is a group of windows. By default all windows are added
   to the same group. But a new group can be created. All new windows
   will be part of this new current group. The windows of any other
   group may be visible in another frame, but will not show up in the
   window list and will not be accessible with select, next, or
   prev. These window navigation commands only navigate the current
   group. */
struct rp_group
{
  /* The name and number of this group. This is to allow the user to
     quickly jump to the desired group. */
  char *name;
  int number;

  /* The list of windows participating in this group. */
  struct list_head mapped_windows, unmapped_windows;

  /* This numset is responsible for giving out numbers for each window
     in the group. */
  struct numset *numset;

  /* This structure can exist in a list. */
  struct list_head node;
};

struct rp_screen
{
  GC normal_gc;
  Window root, bar_window, key_window, input_window, frame_window, help_window;
  int bar_is_raised;
  int screen_num;		/* Our screen number as dictated my X */
  int xine_screen_num;		/* Our screen number for the Xinerama extension */
  Colormap def_cmap;
  Cursor rat;
  unsigned long fg_color, bg_color; /* The pixel color. */
  
  /* Here to abstract over the Xinerama vs X screens difference */
  int left, top, width, height;
  
  char *display_string;

  /* A list of frames that may or may not contain windows. There should
     always be one in the list. */
  struct list_head frames;

  /* Keep track of which numbers have been given to frames. */
  struct numset *frames_numset;

  /* The number of the currently focused frame. One for each screen so
     when you switch screens the focus doesn't get frobbed. */
  int current_frame;
};

struct rp_action
{
  KeySym key;
  unsigned int state;
  void *data;			/* misc data to be passed to the function */
/*   void (*func)(void *); */
};

struct rp_keymap
{
  char *name;
  rp_action *actions;
  int actions_last;
  int actions_size;

  /* This structure can be part of a list. */
  struct list_head node;
};

struct rp_key
{
  KeySym sym;
  unsigned int state;
};

struct rp_defaults
{
  /* Default positions for new normal windows, transient windows, and
     normal windows with maxsize hints. */
  int win_gravity;
  int trans_gravity;
  int maxsize_gravity;

  int input_window_size;
  int window_border_width;

  int bar_x_padding;
  int bar_y_padding;
  int bar_location;
  int bar_timeout;
  int bar_border_width;
  
  int frame_indicator_timeout;
  int frame_resize_unit;

  int padding_left;
  int padding_right;
  int padding_top;
  int padding_bottom;

  XFontStruct *font;
  char *font_string;

  char *fgcolor_string;
  char *bgcolor_string;

  int wait_for_key_cursor;

  char *window_fmt;

  /* Which name to use: wm_name, res_name, res_class. */
  int win_name;

  int startup_message;

  /* Decides whether the window list is displayed in a row or a
     column. */
  int window_list_style;

  /* Pointer warping toggle. */
  int warp;

  int history_size;

  char *frame_selectors;
};

/* Information about a child process. */
struct rp_child_info
{
  /* The command that was executed. */
  char *cmd;

  /* PID of the process. */
  int pid;

  /* Return status when the child process finished. */
  int status;

  /* When this is != 0 then the process finished. */
  int terminated;

  /* This structure can exist in a list. */
  struct list_head node;
};

/* These defines should be used to specify the modifier mask for keys
   and they are translated into the X11 modifier mask when the time
   comes to compare modifier masks. */
#define RP_SHIFT_MASK   1
#define RP_CONTROL_MASK 2
#define RP_META_MASK 	4
#define RP_ALT_MASK 	8
#define RP_SUPER_MASK 	16
#define RP_HYPER_MASK 	32

struct modifier_info
{
/*   unsigned int mode_switch_mask; */
  unsigned int meta_mod_mask;
  unsigned int alt_mod_mask;
  unsigned int super_mod_mask;
  unsigned int hyper_mod_mask;

  /* Keep track of these because they mess up the grab and should be
     ignored. */  
  unsigned int num_lock_mask;
  unsigned int scroll_lock_mask; 
};

typedef struct list_head *(*completion_fn)(char *string);

struct rp_completions
{
  /* A pointer to the partial string that is being completed. We need
     to store this so that the user can cycle through all possible
     completions. */
  char *partial;

  /* A pointer to the string that was last matched string. Used to
     keep track of where we are in the completion list. */
  struct sbuf *last_match;

  /* A list of sbuf's which are possible completions. */
  struct list_head completion_list;

  /* The function that generates the completions. */
  completion_fn complete_fn;

  /* virgin = 1 means no completions have been attempted on the input
     string. */
  unsigned short int virgin;
};

struct rp_input_line
{
  char *buffer;
  char *prompt;
  char *saved;
  int   position;
  int   length;
  int   size;
  rp_completions *compl;
  Atom  selection;
};

/* The hook dictionary. */
struct rp_hook_db_entry
{
  char *name;
  struct list_head *hook;
};

#endif /* _RATPOISON_DATA_H */
