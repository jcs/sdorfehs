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
  grab_key (XKeysymToKeycode (dpy, prefix_key.sym), prefix_key.state, w);
#endif
}

void
ungrab_prefix_key (Window w)
{
#ifdef HIDE_MOUSE
#else
  XUngrabKey(dpy, XKeysymToKeycode (dpy, prefix_key.sym), AnyModifier, w);
#endif
}

screen_info*
current_screen ()
{
  return &screens[rp_current_screen];
}

void
update_normal_hints (rp_window *win)
{
  long supplied;

  XGetWMNormalHints (dpy, win->w, win->hints, &supplied);

  /* Print debugging output for window hints. */
#ifdef DEBUG
  PRINT_DEBUG ("hints: ");
  if (win->hints->flags & PMinSize)
    PRINT_DEBUG ("minx: %d miny: %d ", win->hints->min_width, win->hints->min_height);

  if (win->hints->flags & PMaxSize)
    PRINT_DEBUG ("maxx: %d maxy: %d ", win->hints->max_width, win->hints->max_height);

  if (win->hints->flags & PResizeInc)
    PRINT_DEBUG ("incx: %d incy: %d", win->hints->width_inc, win->hints->height_inc);

  PRINT_DEBUG ("\n");
#endif
}
		     

static char *
get_wmname (Window w)
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
    }
  else
    {
      name = NULL;
    }

  /* Its our responsibility to free this. */ 
  XFreeStringList (name_list);

  return name;
}

static XClassHint *
get_class_hints (Window w)
{
  XClassHint *class;

  class = XAllocClassHint();

  if (class == NULL)
    {
      PRINT_ERROR ("Not enough memory for WM_CLASS structure.\n");
      exit (EXIT_FAILURE);
    }

  XGetClassHint (dpy, w, class);

  return class;
}

static char *
get_res_name (Window w)
{
  XClassHint *class;
  char *name;

  class = get_class_hints (w);

  if (class->res_name)
    {
      name = (char *)xmalloc (strlen (class->res_name) + 1);
      strcpy (name, class->res_name);
    }
  else
    {
      name = NULL;
    }

  XFree (class->res_name);
  XFree (class->res_class);
  XFree (class);

  return name;
}

static char *
get_res_class (Window w)
{
  XClassHint *class;
  char *name;

  class = get_class_hints (w);

  if (class->res_class)
    {
      name = (char *)xmalloc (strlen (class->res_class) + 1);
      strcpy (name, class->res_class);
    }
  else
    {
      name = NULL;
    }

  XFree (class->res_name);
  XFree (class->res_class);
  XFree (class);

  return name;
}

/* Reget the WM_NAME property for the window and update its name. */
int
update_window_name (rp_window *win)
{
  char *newstr;

  /* Don't overwrite the window name if the user specified one. */
/*   if (win->named) return 0; */

  newstr = get_wmname (win->w);
  if (newstr != NULL)
    {
      free (win->wm_name);
      win->wm_name = newstr;
    }

  newstr = get_res_class (win->w);
  if (newstr != NULL)
    {
      free (win->res_class);
      win->res_class = newstr;
    }

  newstr = get_res_name (win->w);
  if (newstr != NULL)
    {
      free (win->res_name);
      win->res_name = newstr;
    }

  return 1;
}

/* Send an artificial configure event to the window. */ 
void
send_configure (Window w, int x, int y, int width, int height, int border)
{
  XConfigureEvent ce;

  ce.type = ConfigureNotify;
  ce.event = w;
  ce.window = w;
  ce.x = x;
  ce.y = y;
  ce.width = width;
  ce.height = height;
  ce.border_width = border;
  ce.above = None;
  ce.override_redirect = 0;

  XSendEvent (dpy, w, False, StructureNotifyMask, (XEvent*)&ce);
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

  /* Transient status */
  win->transient = XGetTransientForHint (dpy, win->w, &win->transient_for);  

  update_window_gravity (win);
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

      kill_signalled = 1;
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
      if (wins[i] == s->bar_window 
	  || wins[i] == s->key_window 
	  || wins[i] == s->input_window
	  || wins[i] == s->frame_window
	  || wins[i] == s->help_window
	  || attr.override_redirect == True
	  || unmanaged_window (wins[i])) continue;

      win = add_to_window_list (s, wins[i]);

      PRINT_DEBUG ("map_state: %d\n", attr.map_state);
      
      /* Collect mapped and iconized windows. */
      if (attr.map_state == IsViewable
	  || (attr.map_state == IsUnmapped 
	      && get_state (win) == IconicState))
	map_window (win);
    }

  XFree(wins);
}

int
unmanaged_window (Window w)
{
  char *wname;
  int i;

  for (i = 0; unmanaged_window_list[i]; i++) 
    {
      wname = get_wmname (w);
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

/* Set the state of the window. */
void
set_state (rp_window *win, int state)
{
  long data[2];
  
  win->state = state;

  data[0] = (long)win->state;
  data[1] = (long)None;

  XChangeProperty (dpy, win->w, wm_state, wm_state, 32,
		   PropModeReplace, (unsigned char *)data, 2);
}

/* Get the WM state of the window. */
long
get_state (rp_window *win)
{
  long state = WithdrawnState;
  Atom type;
  int format;
  unsigned long nitems;
  unsigned long bytes_left;
  long *data;

  if (win == NULL) 
    return state;

  if (XGetWindowProperty (dpy, win->w, wm_state, 0L, 2L, 
			  False, wm_state, &type, &format, 
			  &nitems, &bytes_left, 
			  (unsigned char **)&data) == Success && nitems > 0)
    {
      state = *data;
      XFree (data);
    }

  return state;
}

static void
move_window (rp_window *win)
{
  if (win->frame == NULL) 
    return;

  /* X coord. */
  switch (win->gravity)
    {
    case NorthWestGravity:
    case WestGravity:
    case SouthWestGravity:
      win->x = win->frame->x;
      break;
    case NorthGravity:
    case CenterGravity:
    case SouthGravity:
      win->x = win->frame->x + (win->frame->width - win->border * 2) / 2 - win->width / 2;
      break;
    case NorthEastGravity:
    case EastGravity:
    case SouthEastGravity:
      win->x = win->frame->x + win->frame->width - win->width - win->border;
      break;
    }

  /* Y coord. */
  switch (win->gravity)
    {
    case NorthEastGravity:
    case NorthGravity:
    case NorthWestGravity:
      win->y = win->frame->y;
      break;
    case EastGravity:
    case CenterGravity:
    case WestGravity:
      win->y = win->frame->y + (win->frame->height - win->border * 2) / 2 - win->height / 2;
      break;
    case SouthEastGravity:
    case SouthGravity:
    case SouthWestGravity:
      win->y = win->frame->y + win->frame->height - win->height - win->border;
      break;
    }
}

/* Set a transient window's x,y,width,height fields to maximize the
   window. */
static void
maximize_transient (rp_window *win)
{
  rp_window_frame *frame;
  int maxx, maxy;

  /* We can't maximize a window if it has no frame. */
  if (win->frame == NULL)
    return;

  /* Set the window's border */
  win->border = defaults.window_border_width;

  frame = win->frame;

  /* Always use the window's current width and height for
     transients. */
  maxx = win->width;
  maxy = win->height;

  /* Fit the window inside its frame (if it has one) */
  if (frame)
    {
      PRINT_DEBUG ("frame width=%d height=%d\n", 
		   frame->width, frame->height);

      if (maxx + win->border * 2 > frame->width) maxx = frame->width - win->border * 2;
      if (maxy + win->border * 2 > frame->height) maxy = frame->height - win->border * 2;
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

  win->width = maxx;
  win->height = maxy;
}

/* set a good standard window's x,y,width,height fields to maximize
   the window. */
static void
maximize_normal (rp_window *win)
{
  rp_window_frame *frame;
  int maxx, maxy;

  /* We can't maximize a window if it has no frame. */
  if (win->frame == NULL)
    return;

  /* Set the window's border */
  win->border = defaults.window_border_width;

  frame = win->frame;

  /* Honour the window's maximum size */
  if (win->hints->flags & PMaxSize)
    {
      maxx = win->hints->max_width;
      maxy = win->hints->max_height;
    }
  else
    {
      maxx = frame->width - win->border * 2;
      maxy = frame->height - win->border * 2;
    }

  /* Fit the window inside its frame (if it has one) */
  if (frame)
    {
      PRINT_DEBUG ("frame width=%d height=%d\n", 
		   frame->width, frame->height);

      if (maxx > frame->width) maxx = frame->width - win->border * 2;
      if (maxy > frame->height) maxy = frame->height - win->border * 2;
    }

  /* Make sure we maximize to the nearest Resize Increment specified
     by the window */
  if (win->hints->flags & PResizeInc)
    {
      int amount;
      int delta;

      amount = maxx - win->width;
      delta = amount % win->hints->width_inc;
      if (amount < 0 && delta) amount -= win->hints->width_inc;
      amount -= delta;
      maxx = amount + win->width;

      amount = maxy - win->height;
      delta = amount % win->hints->height_inc;
      if (amount < 0 && delta) amount -= win->hints->height_inc;
      amount -= delta;
      maxy = amount + win->height;
    }

  PRINT_DEBUG ("maxsize: %d %d\n", maxx, maxy);

  win->width = maxx;
  win->height = maxy;
}

/* Maximize the current window if data = 0, otherwise assume it is a
   pointer to a window that should be maximized */
void
maximize (rp_window *win)
{
  if (!win) win = current_window();
  if (!win) return;

  /* Handle maximizing transient windows differently. */
  if (win->transient)
    maximize_transient (win);
  else
    maximize_normal (win);

  /* Reposition the window. */
  move_window (win);

  PRINT_DEBUG ("Resizing window '%s' to x:%d y:%d w:%d h:%d\n", window_name (win), 
	       win->x, win->y, win->width, win->height);


  /* Actually do the maximizing. */
  XMoveResizeWindow (dpy, win->w, win->x, win->y, win->width, win->height);
  XSetWindowBorderWidth (dpy, win->w, win->border);

  XSync (dpy, False);
}

/* Maximize the current window but don't treat transient windows
   differently. */
void
force_maximize (rp_window *win)
{
  if (!win) win = current_window();
  if (!win) return;

  maximize_normal(win);

  /* Reposition the window. */
  move_window (win);

  /* This little dance is to force a maximize event. If the window is
     already "maximized" X11 will optimize away the event since to
     geometry changes were made. This initial resize solves the
     problem. */
  if (win->hints->flags & PResizeInc)
    {
      XMoveResizeWindow (dpy, win->w, win->x, win->y,
			 win->width + win->hints->width_inc, 
			 win->height + win->hints->height_inc);
    }
  else
    {
      XResizeWindow (dpy, win->w, win->width + 1, win->height + 1);
    }

  /* Resize the window to its proper maximum size. */
  XMoveResizeWindow (dpy, win->w, win->x, win->y, win->width, win->height);
  XSetWindowBorderWidth (dpy, win->w, win->border);

  XSync (dpy, False);
}

/* map the unmapped window win */
void
map_window (rp_window *win)
{
  PRINT_DEBUG ("Mapping the unmapped window %s\n", window_name (win));

  /* Fill in the necessary data about the window */
  update_window_information (win);
  win->number = get_unique_window_number ();
  grab_prefix_key (win->w);

  /* Put win in the mapped window list */
  remove_from_list (win);
  insert_into_list (win, rp_mapped_window_sentinel);

  /* The window has never been accessed since it was brought back from
     the Withdrawn state. */
  win->last_access = 0;

  /* It is now considered iconic and set_active_window can handle the rest. */
  set_state (win, IconicState);

  /* Depending on the rudeness level, actually map the window. */
  if ((rp_honour_transient_map && win->transient)
      || (rp_honour_normal_map && !win->transient))
    set_active_window (win);
  else
    {
      if (win->transient)
	marked_message_printf (0, 0, MESSAGE_MAP_TRANSIENT, 
			       win->number, window_name (win));
      else
	marked_message_printf (0, 0, MESSAGE_MAP_WINDOW,
			       win->number, window_name (win));
    }
}

void
hide_window (rp_window *win)
{
  if (win == NULL) return;

  /* An unmapped window is not inside a frame. */
  win->frame = NULL;

  /* Ignore the unmap_notify event. */
  XSelectInput(dpy, win->w, WIN_EVENTS&~(StructureNotifyMask));
  XUnmapWindow (dpy, win->w);
  XSelectInput (dpy, win->w, WIN_EVENTS);
  set_state (win, IconicState);
}

void
unhide_window (rp_window *win)
{
  if (win == NULL) return;

  /* Always raise the window. */
  XRaiseWindow (dpy, win->w);

  if (win->state != IconicState) return;

  XMapWindow (dpy, win->w);
  set_state (win, NormalState);
}

/* same as unhide_window except that it makes sure the window is mapped
   on the bottom of the window stack. */
void
unhide_window_below (rp_window *win)
{
  if (win == NULL) return;

  /* Always lower the window, but if its not iconic we don't need to
     map it since it already is mapped. */
  XLowerWindow (dpy, win->w);

  if (win->state != IconicState) return;

  XMapWindow (dpy, win->w);
  set_state (win, NormalState);
}

void
withdraw_window (rp_window *win)
{
  if (win == NULL) return;

  PRINT_DEBUG ("withdraw_window on '%s'\n", window_name (win));

  /* Give back the window number. the window will get another one,
     if it is remapped. */
  return_window_number (win->number);
  win->number = -1;

  remove_from_list (win);
  append_to_list (win, rp_unmapped_window_sentinel);

  ignore_badwindow++;

  XRemoveFromSaveSet (dpy, win->w);
  set_state (win, WithdrawnState);
  XSync (dpy, False);

  ignore_badwindow--;
}

/* Hide all other mapped windows except for win in win's frame. */
void
hide_others (rp_window *win)
{
  rp_window_frame *frame;
  rp_window *cur;

  if (win == NULL) return;
  frame = find_windows_frame (win);
  if (frame == NULL) return;

  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
      if (find_windows_frame (cur) 
	  || cur->state != NormalState 
	  || cur->frame != frame)
	continue;

      hide_window (cur);
    }
}
