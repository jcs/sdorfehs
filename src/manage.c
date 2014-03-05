/* Manage windows, such as Mapping them and making sure the proper key
 * Grabs have been put in place.
 *
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
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

static char **unmanaged_window_list = NULL;
static int num_unmanaged_windows = 0;

void
clear_unmanaged_list (void)
{
  if (unmanaged_window_list)
    {
      int i;

      for (i = 0; i < num_unmanaged_windows; i++)
        free(unmanaged_window_list[i]);

      free(unmanaged_window_list);

      unmanaged_window_list = NULL;
    }
  num_unmanaged_windows = 0;
}

char *
list_unmanaged_windows (void)
{
  char *tmp = NULL;

  if (unmanaged_window_list)
    {
      struct sbuf *buf;
      int i;

      buf = sbuf_new (0);

      for (i = 0; i < num_unmanaged_windows; i++)
        {
          sbuf_concat (buf, unmanaged_window_list[i]);
          sbuf_concat (buf, "\n");
        }
      sbuf_chop (buf);
      tmp = sbuf_free_struct (buf);
    }
  return tmp;
}

void
add_unmanaged_window (char *name)
{
  char **tmp;

  if (!name) return;

  tmp = xmalloc((num_unmanaged_windows + 1) * sizeof(char *));

  if (unmanaged_window_list)
    {
      memcpy(tmp, unmanaged_window_list, num_unmanaged_windows * sizeof(char *));
      free(unmanaged_window_list);
    }

  tmp[num_unmanaged_windows] = xstrdup(name);
  num_unmanaged_windows++;

  unmanaged_window_list = tmp;
}

extern Atom wm_state;

void
grab_top_level_keys (Window w)
{
#ifdef HIDE_MOUSE
  XGrabKey(dpy, AnyKey, AnyModifier, w, True,
           GrabModeAsync, GrabModeAsync);
#else
  rp_keymap *map = find_keymap (defaults.top_kmap);
  int i;

  if (map == NULL)
    {
      PRINT_ERROR (("Unable to find %s level keymap\n", defaults.top_kmap));
      return;
    }

  PRINT_DEBUG(("grabbing top level key\n"));
  for (i=0; i<map->actions_last; i++)
    {
      PRINT_DEBUG(("%d\n", i));
      grab_key (map->actions[i].key, map->actions[i].state, w);
    }
#endif
}

void
ungrab_top_level_keys (Window w)
{
  XUngrabKey(dpy, AnyKey, AnyModifier, w);
}

void
ungrab_keys_all_wins (void)
{
  rp_window *cur;

  /* Remove the grab on the current prefix key */
  list_for_each_entry (cur, &rp_mapped_window, node)
    {
      ungrab_top_level_keys (cur->w);
    }
}

void
grab_keys_all_wins (void)
{
  rp_window *cur;

  /* Remove the grab on the current prefix key */
  list_for_each_entry (cur, &rp_mapped_window, node)
    {
      grab_top_level_keys (cur->w);
    }
}

rp_screen*
current_screen (void)
{
  int i;

  for (i=0; i<num_screens; i++)
    {
      if (screens[i].xine_screen_num == rp_current_screen)
        return &screens[i];
    }

  /* This should never happen. */
  return &screens[0];
}

void
update_normal_hints (rp_window *win)
{
  long supplied;

  XGetWMNormalHints (dpy, win->w, win->hints, &supplied);

  /* Print debugging output for window hints. */
#ifdef DEBUG
  if (win->hints->flags & PMinSize)
    PRINT_DEBUG (("minx: %d miny: %d\n", win->hints->min_width, win->hints->min_height));

  if (win->hints->flags & PMaxSize)
    PRINT_DEBUG (("maxx: %d maxy: %d\n", win->hints->max_width, win->hints->max_height));

  if (win->hints->flags & PResizeInc)
    PRINT_DEBUG (("incx: %d incy: %d\n", win->hints->width_inc, win->hints->height_inc));

#endif
}


static char *
get_wmname (Window w)
{
  char *name = NULL;
  XTextProperty text_prop;
  int ret = None, n;
  char** cl;

  /* If current encoding is UTF-8, try to use the window's _NET_WM_NAME ewmh
     property */
  if (defaults.utf8_locale)
    {
      Atom type = None;
      unsigned long nitems, bytes_after;
      int format;
      unsigned char *val = NULL;

      ret = XGetWindowProperty (dpy, w, _net_wm_name, 0, 40, False,
				xa_utf8_string, &type, &format, &nitems,
				&bytes_after, &val);
      /* We have a valid UTF-8 string */
      if (ret == Success && type == xa_utf8_string
	  && format == 8 && nitems > 0)
	{
	  name = xstrdup ((char *)val);
	  XFree (val);
          PRINT_DEBUG (("Fetching window name using _NET_WM_NAME succeeded\n"));
	  PRINT_DEBUG (("WM_NAME: %s\n", name));
	  return name;
	}
      /* Something went wrong for whatever reason */
      if (ret == Success && val)
	XFree (val);
      PRINT_DEBUG (("Could not fetch window name using _NET_WM_NAME\n"));
    }

  if (XGetWMName (dpy, w, &text_prop) == 0)
    {
      PRINT_DEBUG (("XGetWMName failed\n"));
      return NULL;
    }

  PRINT_DEBUG (("WM_NAME encoding: "));
  if (text_prop.encoding == xa_string)
    PRINT_DEBUG  (("STRING\n"));
  else if (text_prop.encoding == xa_compound_text)
    PRINT_DEBUG (("COMPOUND_TEXT\n"));
  else if (text_prop.encoding == xa_utf8_string)
    PRINT_DEBUG (("UTF8_STRING\n"));
  else
    PRINT_DEBUG (("unknown (%d)\n", (int) text_prop.encoding));

#ifdef X_HAVE_UTF8_STRING
  /* It seems that most applications supporting UTF8_STRING and
     _NET_WM_NAME don't bother making their WM_NAME available as
     UTF8_STRING (but only as either STRING or COMPOUND_TEXT).
     Let's try anyway.  */
  if (defaults.utf8_locale && text_prop.encoding == xa_utf8_string)
    {
      ret = Xutf8TextPropertyToTextList (dpy, &text_prop, &cl, &n);
      PRINT_DEBUG (("Xutf8TextPropertyToTextList: %s\n",
		    ret == Success ? "success" : "error"));
    }
  else
#endif
    {
      /* XmbTextPropertyToTextList should be fine for all cases,
	 even UTF8_STRING encoded WM_NAME */
      ret = XmbTextPropertyToTextList (dpy, &text_prop, &cl, &n);
      PRINT_DEBUG (("XmbTextPropertyToTextList: %s\n",
		    ret == Success ? "success" : "error"));
    }

  if (ret == Success && cl && n > 0)
    {
      name = xstrdup (cl[0]);
      XFreeStringList (cl);
    }
  else if (text_prop.value)
    {
      /* Convertion failed, try to get the raw string */
      name = xstrdup ((char *) text_prop.value);
      XFree (text_prop.value);
    }

  if (name == NULL) {
    PRINT_DEBUG (("I can't get the WMName.\n"));
  } else {
    PRINT_DEBUG (("WM_NAME: '%s'\n", name));
  }

  return name;
}

static XClassHint *
get_class_hints (Window w)
{
  XClassHint *class;

  class = XAllocClassHint();

  if (class == NULL)
    {
      PRINT_ERROR (("Not enough memory for WM_CLASS structure.\n"));
      exit (EXIT_FAILURE);
    }

  XGetClassHint (dpy, w, class);

  return class;
}

/* Reget the WM_NAME property for the window and update its
   name. Return 1 if the name changed. */
int
update_window_name (rp_window *win)
{
  char *newstr;
  int changed = 0;
  XClassHint *class;

  newstr = get_wmname (win->w);
  if (newstr != NULL)
    {
      changed = changed || win->wm_name == NULL || strcmp (newstr, win->wm_name);
      free (win->wm_name);
      win->wm_name = newstr;
    }

  class = get_class_hints (win->w);

  if (class->res_class != NULL
      && (win->res_class == NULL || strcmp (class->res_class, win->res_class)))
    {
      changed = 1;
      free (win->res_class);
      win->res_class = xstrdup(class->res_class);
    }

  if (class->res_name != NULL
      && (win->res_name == NULL || strcmp (class->res_name, win->res_name)))
    {
      changed = 1;
      free (win->res_name);
      win->res_name = xstrdup(class->res_name);
    }

  XFree (class->res_name);
  XFree (class->res_class);
  XFree (class);
  return changed;
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

/* This function is used to determine if the window should be treated
   as a transient. */
int
window_is_transient (rp_window *win)
{
  return win->transient
#ifdef ASPECT_WINDOWS_ARE_TRANSIENTS
 || win->hints->flags & PAspect
#endif
#ifdef MAXSIZE_WINDOWS_ARE_TRANSIENTS
|| (win->hints->flags & PMaxSize
    && (win->hints->max_width < win->scr->width
        || win->hints->max_height < win->scr->height))
#endif
    ;
}

static Atom
get_net_wm_window_type (rp_window *win)
{
  Atom type, window_type = None;
  int format;
  unsigned long nitems;
  unsigned long bytes_left;
  unsigned char *data;

  if (win == NULL)
    return None;

  if (XGetWindowProperty (dpy, win->w, _net_wm_window_type, 0, 1L,
                          False, XA_ATOM, &type, &format,
                          &nitems, &bytes_left,
                          &data) == Success && nitems > 0)
    {
      window_type = *(Atom *)data;
      XFree (data);
      PRINT_DEBUG(("hey ya %ld %ld\n", window_type, _net_wm_window_type_dialog));
    }

  return window_type;
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

  if (get_net_wm_window_type(win) == _net_wm_window_type_dialog)
    win->transient = 1;

  update_window_gravity (win);
}

void
unmanage (rp_window *w)
{
  list_del (&w->node);
  groups_del_window (w);

  free_window (w);

#ifdef AUTO_CLOSE
  if (rp_mapped_window.next == &rp_mapped_window
      && rp_mapped_window.prev == &rp_mapped_window)
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
scanwins(rp_screen *s)
{
  rp_window *win;
  XWindowAttributes attr;
  unsigned int i, nwins;
  Window dw1, dw2, *wins;

  XQueryTree(dpy, s->root, &dw1, &dw2, &wins, &nwins);
  PRINT_DEBUG (("windows: %d\n", nwins));

  for (i = 0; i < nwins; i++)
    {
      XGetWindowAttributes(dpy, wins[i], &attr);
      if (is_rp_window_for_screen(wins[i], s)
          || attr.override_redirect == True
          || unmanaged_window (wins[i])) continue;

      /* FIXME - with this code, windows which are entirely off-screen
       * when RP starts won't ever be managed when Xinerama is enabled.
       */
      {
        XWindowAttributes root_attr;

        XGetWindowAttributes (dpy, s->root, &root_attr);
      PRINT_DEBUG (("attrs: %d %d %d %d %d %d\n", root_attr.x, root_attr.y,
                    s->left, s->top, s->left + s->width, s->top + s->height));}

      if (rp_have_xinerama
          && ((attr.x > s->left + s->width)
               || (attr.x < s->left)
               || (attr.y > s->top + s->height)
               || (attr.y < s->top))) continue;

      win = add_to_window_list (s, wins[i]);

      PRINT_DEBUG (("map_state: %s\n",
                    attr.map_state == IsViewable ? "IsViewable":
                    attr.map_state == IsUnviewable ? "IsUnviewable" : "IsUnmapped"));
      PRINT_DEBUG (("state: %s\n",
                    get_state(win) == IconicState ? "Iconic":
                    get_state(win) == NormalState ? "Normal" : "Other"));

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

  if (!unmanaged_window_list)
    return 0;

  wname = get_wmname (w);
  if (!wname)
    return 0;

  for (i = 0; i < num_unmanaged_windows; i++)
    {
      if (!strcmp (unmanaged_window_list[i], wname))
        {
          free (wname);
          return 1;
        }
    }

  free (wname);
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
  unsigned char *data;

  if (win == NULL)
    return state;

  if (XGetWindowProperty (dpy, win->w, wm_state, 0L, 2L,
                          False, wm_state, &type, &format,
                          &nitems, &bytes_left,
                          &data) == Success && nitems > 0)
    {
      state = *(long *)data;
      XFree (data);
    }

  return state;
}

static void
move_window (rp_window *win)
{
  rp_frame *frame;

  if (win->frame_number == EMPTY)
    return;

  frame = win_get_frame (win);

  /* X coord. */
  switch (win->gravity)
    {
    case NorthWestGravity:
    case WestGravity:
    case SouthWestGravity:
      win->x = frame->x;
      break;
    case NorthGravity:
    case CenterGravity:
    case SouthGravity:
      win->x = frame->x + (frame->width - win->border * 2) / 2 - win->width / 2;
      break;
    case NorthEastGravity:
    case EastGravity:
    case SouthEastGravity:
      win->x = frame->x + frame->width - win->width - win->border;
      break;
    }

  /* Y coord. */
  switch (win->gravity)
    {
    case NorthEastGravity:
    case NorthGravity:
    case NorthWestGravity:
      win->y = frame->y;
      break;
    case EastGravity:
    case CenterGravity:
    case WestGravity:
      win->y = frame->y + (frame->height - win->border * 2) / 2 - win->height / 2;
      break;
    case SouthEastGravity:
    case SouthGravity:
    case SouthWestGravity:
      win->y = frame->y + frame->height - win->height - win->border;
      break;
    }
}

/* Set a transient window's x,y,width,height fields to maximize the
   window. */
static void
maximize_transient (rp_window *win)
{
  rp_frame *frame;
  int maxx, maxy;

  frame = win_get_frame (win);

  /* We can't maximize a window if it has no frame. */
  if (frame == NULL)
    return;

  /* Set the window's border */
  win->border = defaults.window_border_width;

  /* Always use the window's current width and height for
     transients. */
  maxx = win->width;
  maxy = win->height;

  /* Fit the window inside its frame (if it has one) */
  if (frame)
    {
      PRINT_DEBUG (("frame width=%d height=%d\n",
                   frame->width, frame->height));

      if (maxx + win->border * 2 > frame->width) maxx = frame->width - win->border * 2;
      if (maxy + win->border * 2 > frame->height) maxy = frame->height - win->border * 2;
    }

  /* Make sure we maximize to the nearest Resize Increment specified
     by the window */
  if (win->hints->flags & PResizeInc)
    {
      int amount;
      int delta;

      /* Avoid a divide by zero if width/height_inc is 0. */
      if (win->hints->width_inc)
        {
          amount = maxx - win->width;
          delta = amount % win->hints->width_inc;
          amount -= delta;
          if (amount < 0 && delta) amount -= win->hints->width_inc;
          maxx = amount + win->width;
        }

      if (win->hints->height_inc)
        {
          amount = maxy - win->height;
          delta = amount % win->hints->height_inc;
          amount -= delta;
          if (amount < 0 && delta) amount -= win->hints->height_inc;
          maxy = amount + win->height;
        }
    }

  PRINT_DEBUG (("maxsize: %d %d\n", maxx, maxy));

  win->width = maxx;
  win->height = maxy;
}

/* set a good standard window's x,y,width,height fields to maximize
   the window. */
static void
maximize_normal (rp_window *win)
{
  rp_frame *frame;
  int maxx, maxy;

  frame = win_get_frame (win);

  /* We can't maximize a window if it has no frame. */
  if (frame == NULL)
    return;

  /* Set the window's border */
  win->border = defaults.window_border_width;

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

  /* Honour the window's aspect ratio. */
  PRINT_DEBUG (("aspect: %ld\n", win->hints->flags & PAspect));
  if (win->hints->flags & PAspect)
    {
      float ratio = (float)maxx / maxy;
      float min_ratio = (float)win->hints->min_aspect.x / win->hints->min_aspect.y;
      float max_ratio = (float)win->hints->max_aspect.x / win->hints->max_aspect.y;
      PRINT_DEBUG (("ratio=%f min_ratio=%f max_ratio=%f\n",
                    ratio,min_ratio,max_ratio));
      if (ratio < min_ratio)
        {
          maxy = (int) (maxx / min_ratio);
        }
      else if (ratio > max_ratio)
        {
          maxx = (int) (maxy * max_ratio);
        }
    }

  /* Fit the window inside its frame (if it has one) */
  if (frame)
    {
      PRINT_DEBUG (("frame width=%d height=%d\n",
                   frame->width, frame->height));

      if (maxx > frame->width) maxx = frame->width - win->border * 2;
      if (maxy > frame->height) maxy = frame->height - win->border * 2;
    }

  /* Make sure we maximize to the nearest Resize Increment specified
     by the window */
  if (win->hints->flags & PResizeInc)
    {
      int amount;
      int delta;

      if (win->hints->width_inc)
        {
          amount = maxx - win->width;
          delta = amount % win->hints->width_inc;
          if (amount < 0 && delta) amount -= win->hints->width_inc;
          amount -= delta;
          maxx = amount + win->width;
        }

      if (win->hints->height_inc)
        {
          amount = maxy - win->height;
          delta = amount % win->hints->height_inc;
          if (amount < 0 && delta) amount -= win->hints->height_inc;
          amount -= delta;
          maxy = amount + win->height;
        }
    }

  PRINT_DEBUG (("maxsize: %d %d\n", maxx, maxy));

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

  PRINT_DEBUG (("Resizing window '%s' to x:%d y:%d w:%d h:%d\n", window_name (win),
               win->x, win->y, win->width, win->height));


  /* Actually do the maximizing. */
  XMoveResizeWindow (dpy, win->w, win->scr->left + win->x, win->scr->top + win->y, win->width, win->height);
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
      XMoveResizeWindow (dpy, win->w, win->scr->left + win->x, win->scr->top + win->y,
                         win->width + win->hints->width_inc,
                         win->height + win->hints->height_inc);
    }
  else
    {
      XResizeWindow (dpy, win->w, win->width + 1, win->height + 1);
    }

  XSync (dpy, False);

  /* Resize the window to its proper maximum size. */
  XMoveResizeWindow (dpy, win->w, win->scr->left + win->x, win->scr->top + win->y, win->width, win->height);
  XSetWindowBorderWidth (dpy, win->w, win->border);

  XSync (dpy, False);
}

/* map the unmapped window win */
void
map_window (rp_window *win)
{
  PRINT_DEBUG (("Mapping the unmapped window %s\n", window_name (win)));

  /* Fill in the necessary data about the window */
  update_window_information (win);
  win->number = numset_request (rp_window_numset);
  grab_top_level_keys (win->w);

  /* Put win in the mapped window list */
  list_del (&win->node);
  insert_into_list (win, &rp_mapped_window);

  /* Update all groups. */
  groups_map_window (win);

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
    show_rudeness_msg (win, 0);

  hook_run (&rp_new_window_hook);
}

void
hide_window (rp_window *win)
{
  if (win == NULL) return;

  /* An unmapped window is not inside a frame. */
  win->frame_number = EMPTY;

  /* Ignore the unmap_notify event. */
  XSelectInput(dpy, win->w, WIN_EVENTS&~(StructureNotifyMask));
  XUnmapWindow (dpy, win->w);
  XSelectInput (dpy, win->w, WIN_EVENTS);
  /* Ensure that the window doesn't have the focused border
     color. This is needed by remove_frame and possibly others. */
  XSetWindowBorder (dpy, win->w, win->scr->bw_color);
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

void
unhide_all_windows (void)
{
  struct list_head *tmp, *iter;
  rp_window *win;

  list_for_each_safe_entry (win, iter, tmp, &rp_mapped_window, node)
    unhide_window (win);
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

  PRINT_DEBUG (("withdraw_window on '%s'\n", window_name (win)));

  /* Give back the window number. the window will get another one,
     if it is remapped. */
  if (win->number == -1)
    PRINT_ERROR(("Attempting to withdraw '%s' with number -1!\n", window_name(win)));

  numset_release (rp_window_numset, win->number);
  win->number = -1;

  list_move_tail(&win->node, &rp_unmapped_window);

  /* Update the groups. */
  groups_unmap_window (win);

  ignore_badwindow++;

  XRemoveFromSaveSet (dpy, win->w);
  set_state (win, WithdrawnState);
  XSync (dpy, False);

  ignore_badwindow--;

  /* Call our hook */
  hook_run (&rp_delete_window_hook);
}

/* Hide all other mapped windows except for win in win's frame. */
void
hide_others (rp_window *win)
{
  rp_frame *frame;
  rp_window *cur;

  if (win == NULL) return;
  frame = find_windows_frame (win);
  if (frame == NULL) return;

  list_for_each_entry (cur, &rp_mapped_window, node)
    {
      if (find_windows_frame (cur)
          || cur->state != NormalState
          || cur->frame_number != frame->number)
        continue;

      hide_window (cur);
    }
}
