/* Manage windows, such as Mapping them and making sure the proper key
 * Grabs have been put in place.
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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ratpoison.h"

const char *unmanaged_window_list[] = {
#ifdef UNMANAGED_WINDOW_LIST
  UNMANAGED_WINDOW_LIST,
#endif
  NULL
};

extern Atom wm_state;

void
grab_prefix_key (Window w)
{
#ifdef HIDE_MOUSE
  XGrabKey(dpy, AnyKey, AnyModifier, w, True, 
	   GrabModeAsync, GrabModeAsync);
#else
  XGrabKey(dpy, XKeysymToKeycode (dpy, prefix_key.sym ), prefix_key.state, 
	   w, True, GrabModeAsync, GrabModeAsync);
#endif
}

void
ungrab_prefix_key (Window w)
{
#ifdef HIDE_MOUSE
#else
  XUngrabKey(dpy, XKeysymToKeycode (dpy, prefix_key.sym ), prefix_key.state, w);
#endif
}

screen_info*
current_screen ()
{
  if (rp_current_window)
    return rp_current_window->scr;
  else
    return &screens[0];
}

void
update_normal_hints (rp_window *win)
{
  long supplied;

  XGetWMNormalHints (dpy, win->w, win->hints, &supplied);

  PRINT_DEBUG ("hints: minx: %d miny: %d maxx: %d maxy: %d incx: %d incy: %d\n", 
	       win->hints->min_width, win->hints->min_height,
	       win->hints->max_width, win->hints->max_height,
	       win->hints->width_inc, win->hints->height_inc);
}
		     

char *
get_window_name (Window w)
{
  char *name;
  XTextProperty text;
  char **name_list;
  int list_len;

  if (!XGetWMName (dpy, w, &text))
    {
      PRINT_DEBUG ("I can't get the WMName.\n");
      return NULL;
    }

  if (!XTextPropertyToStringList (&text, &name_list, &list_len))
    {
      PRINT_DEBUG ("Error retrieving TextList.\n");
      return NULL;
    }

  if (list_len > 0)
    {
      name = xmalloc (strlen (name_list[0]) + 1);

      strcpy (name, name_list[0]);

      /* Its our responsibility to free this. */ 
      XFreeStringList (name_list);

      return name;
    }

  /* Its our responsibility to free this. */ 
  XFreeStringList (name_list);

  return NULL;
}

/* Reget the WM_NAME property for the window and update its name. */
int
update_window_name (rp_window *win)
{
  char *newstr;
  char *loc;

  /* Don't overwrite the window name if the user specified one. */
  if (win->named) return 0;

  newstr = get_window_name (win->w);
  if (newstr != NULL)
    {
      free (win->name);
      win->name = newstr;
    }

  /* A bit of a hack. If there's a : in the string, crop the
     string off there. This is mostly brought on by netscape's
     disgusting tendency to put its current URL in the WMName!!
     arg! */
  loc = strchr (win->name, ':');
  if (loc) loc[0] = '\0';

  return 1;
}

/* Send an artificial configure event to the window. */ 
void
send_configure (rp_window *win)
{
  XConfigureEvent ce;

  ce.type = ConfigureNotify;
  ce.event = win->w;
  ce.window = win->w;
  ce.x = win->x;
  ce.y = win->y;
  ce.width = win->width;
  ce.height = win->height;
  ce.border_width = win->border;
  ce.above = None;
  ce.override_redirect = 0;

  XSendEvent (dpy, win->w, False, StructureNotifyMask, (XEvent*)&ce);
}

void
update_window_information (rp_window *win)
{
  XWindowAttributes attr;

  update_window_name (win);

  /* Get the WM Hints */
  update_normal_hints (win);

  /* Get the colormap */
  XGetWindowAttributes (dpy, win->w, &attr);
  win->colormap = attr.colormap;
  win->x = attr.x;
  win->y = attr.y;
  win->width = attr.width;
  win->height = attr.height;
  win->border = attr.border_width;
}

void
unmanage (rp_window *w)
{
  return_window_number (w->number);
  remove_from_list (w);

  free_window (w);

#ifdef AUTO_CLOSE
  if (rp_mapped_window_sentinel->next == rp_mapped_window_sentinel
      && rp_mapped_window_sentinel->prev == rp_mapped_window_sentinel)
    {
      /* If the mapped window list is empty then we have run out of
 	 managed windows, so kill ratpoison. */

      /* FIXME: The unmapped window list may also have to be checked
	 in the case that the only mapped window in unmapped and
	 shortly after another window is mapped most likely by the
	 same app. */

      send_kill();
    }
#endif
}

/* When starting up scan existing windows and start managing them. */
void
scanwins(screen_info *s)
{
  rp_window *win;
  XWindowAttributes attr;
  unsigned int i, nwins;
  Window dw1, dw2, *wins;

  XQueryTree(dpy, s->root, &dw1, &dw2, &wins, &nwins);
  PRINT_DEBUG ("windows: %d\n", nwins);

  for (i = 0; i < nwins; i++) 
    {
      XGetWindowAttributes(dpy, wins[i], &attr);
      if (wins[i] == s->bar_window || wins[i] == s->key_window || wins[i] == s->input_window) continue;

      if (attr.override_redirect != True && !unmanaged_window (wins[i]))
	{
	  win = add_to_window_list (s, wins[i]);

	  PRINT_DEBUG ("map_state: %d\n", attr.map_state);
	  if (attr.map_state == IsViewable) 
	    {
	      map_window (win);
	    }
	      
	}
    }
  XFree((void *) wins);	/* cast is to shut stoopid compiler up */
}

int
unmanaged_window (Window w)
{
  char *wname;
  int i;

  for (i = 0; unmanaged_window_list[i]; i++) 
    {
      wname = get_window_name (w);
      if (!wname)
	return 0;
      if (!strcmp (unmanaged_window_list[i], wname))
	{
	  free (wname);
	  return 1;
	}
    }

  return 0;
}

/* Set the state of the window.

   FIXME: This is sort of broken. We should really record the state in
   win->state and mimic the X states NormalState, WithdrawnState,
   IconicState */
void
set_state (rp_window *win, int state)
{
  long data[2];
  
  data[0] = (long)state;
  data[1] = (long)None;

  XChangeProperty (dpy, win->w, wm_state, wm_state, 32,
		   PropModeReplace, (unsigned char *)data, 2);
}

/* Set a transient window's x,y,width,height fields to maximize the
   window. */
static void
maximize_transient (rp_window *win)
{
  int maxx, maxy;

  /* Set the window's border */
  win->border = WINDOW_BORDER_WIDTH;

  /* Honour the window's maximum size */
  if (win->hints->flags & PMaxSize)
    {
      maxx = win->hints->max_width;
      maxy = win->hints->max_height;
    }
  else
    {
      maxx = win->width;
      maxy = win->height;
    }

  /* Make sure we maximize to the nearest Resize Increment specified
     by the window */
  if (win->hints->flags & PResizeInc)
    {
      int amount;
      int delta;

      amount = maxx - win->width;
      delta = amount % win->hints->width_inc;
      amount -= delta;
      if (amount < 0 && delta) amount -= win->hints->width_inc;
      maxx = amount + win->width;

      amount = maxy - win->height;
      delta = amount % win->hints->height_inc;
      amount -= delta;
      if (amount < 0 && delta) amount -= win->hints->height_inc;
      maxy = amount + win->height;
    }

  PRINT_DEBUG ("maxsize: %d %d\n", maxx, maxy);

  /* Fit the window inside its frame (if it has one) */
  if (win->frame)
    {
      win->x = win->frame->x - win->width / 2 
	+ (win->frame->width - win->border * 2) / 2;
      win->y = win->frame->y - win->height / 2
	+ (win->frame->height - win->border * 2) / 2;
    }
  else
    {
      win->x = PADDING_LEFT - win->width / 2 
	+ (win->scr->root_attr.width - PADDING_LEFT - PADDING_RIGHT - win->border * 2) / 2;
      win->y = PADDING_TOP - win->height / 2 
	+ (win->scr->root_attr.height - PADDING_TOP - PADDING_BOTTOM - win->border * 2) / 2;
    }

  win->width = maxx;
  win->height = maxy;
}

/* set a good standard window's x,y,width,height fields to maximize
   the window. */
static void
maximize_normal (rp_window *win)
{
  int maxx, maxy;

  int off_x = 0;
  int off_y = 0;

  /* Set the window's border */
  win->border = WINDOW_BORDER_WIDTH;

  /* Honour the window's maximum size */
  if (win->hints->flags & PMaxSize)
    {
      maxx = win->hints->max_width;
      maxy = win->hints->max_height;
    }
  else
    {
      maxx = win->scr->root_attr.width 
	- PADDING_LEFT - PADDING_RIGHT - win->border * 2;
      maxy = win->scr->root_attr.height 
	- PADDING_TOP - PADDING_BOTTOM - win->border * 2;
    }

  /* Fit the window inside its frame (if it has one) */
  if (win->frame)
    {
      PRINT_DEBUG ("frame width=%d height=%d\n", 
		   win->frame->width, win->frame->height);

      if (maxx > win->frame->width) maxx = win->frame->width 
				      - win->border * 2;
      if (maxy > win->frame->height) maxy = win->frame->height 
				       - win->border * 2;
    }

  /* Make sure we maximize to the nearest Resize Increment specified
     by the window */
  if (win->hints->flags & PResizeInc)
    {
      int amount;
      int delta;

      amount = maxx - win->width;
      delta = amount % win->hints->width_inc;
      amount -= delta;
      if (amount < 0 && delta) amount -= win->hints->width_inc;
      maxx = amount + win->width;

      amount = maxy - win->height;
      delta = amount % win->hints->height_inc;
      amount -= delta;
      if (amount < 0 && delta) amount -= win->hints->height_inc;
      maxy = amount + win->height;
    }

  PRINT_DEBUG ("maxsize: %d %d\n", maxx, maxy);

  /* Fit the window inside its frame (if it has one) */
  if (win->frame)
    {
      win->x = win->frame->x;
      win->y = win->frame->y;
    }
  else
    {
      win->x = PADDING_LEFT + off_x;
      win->y = PADDING_TOP + off_y;
    }

  win->width = maxx;
  win->height = maxy;
}

/* Maximize the current window if data = 0, otherwise assume it is a
   pointer to a window that should be maximized */
void
maximize (rp_window *win)
{
  if (!win) win = rp_current_window;
  if (!win) return;

  /* Handle maximizing transient windows differently */
  if (win->transient) 
    {
      maximize_transient (win);
    }
  else
    {
      maximize_normal (win);
    }

  /* Actually do the maximizing */
  XMoveResizeWindow (dpy, win->w, win->x, win->y, win->width, win->height);
  XSetWindowBorderWidth (dpy, win->w, win->border);
  XSync (dpy, False);
}

/* Maximize the current window but don't treat transient windows
   differently. */
void
force_maximize (rp_window *win)
{
  if (!win) win = rp_current_window;
  if (!win) return;

  maximize_normal(win);

  /* Actually do the maximizing */
  XMoveResizeWindow (dpy, win->w, win->x, win->y, win->width+1, win->height+1);
  XMoveResizeWindow (dpy, win->w, win->x, win->y, win->width, win->height);
  XSetWindowBorderWidth (dpy, win->w, win->border);
  XSync (dpy, False);
}

/* map the unmapped window win */
void
map_window (rp_window *win)
{
  PRINT_DEBUG ("Mapping the unmapped window %s\n", win->name);

  update_window_information (win);
  grab_prefix_key (win->w);

  maximize (win);

  XMapWindow (dpy, win->w);
  set_state (win, NormalState);
  set_active_window (win);

  win->state = STATE_MAPPED;
  win->number = get_unique_window_number ();

  /* Put win in the mapped window list */
  remove_from_list (win);
  insert_into_list (win, rp_mapped_window_sentinel);
}
