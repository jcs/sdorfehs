/* our datatypes and global variables */

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
  char *name;
  int state;
  int last_access;		
  rp_window *next, *prev;  
};

struct screen_info
{
  GC normal_gc;
  GC bold_gc;
  XFontStruct *font;		/* The font we want to use. */
  XWindowAttributes root_attr;
  Window root, bar_window, key_window;
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

/* Set to 1 to indicate that the WM should exit at it's earliest
   convenience. */
extern int exit_signal;

#endif /* _DATA_H */
