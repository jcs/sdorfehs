/* Manage windows, such as Mapping them and making sure the proper key
 * Grabs have been put in place.
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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ratpoison.h"

extern Atom wm_state;

void
grab_keys (screen_info *s)
{
  int i;

  for (i='0'; i<='9'; i++)
    XGrabKey(dpy, XKeysymToKeycode (dpy, i ), AnyModifier, s->key_window, True, 
	     GrabModeAsync, GrabModeAsync);  

  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_XTERM ), AnyModifier, s->key_window, True, 
	   GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_EMACS ), AnyModifier, s->key_window, True, 
	   GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_PREVWINDOW ), AnyModifier, s->key_window, True, 
	   GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_NEXTWINDOW ), AnyModifier, s->key_window, True, 
	   GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_TOGGLEBAR ), AnyModifier, s->key_window, True, 
	   GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_LASTWINDOW ), AnyModifier, s->key_window, True, 
	   GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_DELETE ), AnyModifier, s->key_window, True, 
	   GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_PREFIX ), AnyModifier, s->key_window, True, 
	   GrabModeAsync, GrabModeAsync);
  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_WINBYNAME ), AnyModifier, s->key_window, True, 
	   GrabModeAsync, GrabModeAsync);
}

static void
grab_prefix_key (Window w)
{
  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_PREFIX ), MODIFIER_PREFIX, w, True, 
	   GrabModeAsync, GrabModeAsync);
}

/* Reget the WM_NAME property for the window and update its name. */
int
update_window_name (rp_window *win)
{
  XTextProperty text;
  char **name_list;
  int list_len;
  int i;

  if (!XGetWMName (dpy, win->w, &text))
    {
      fprintf (stderr, "ratpoison:manage.c: I can't get the WMName.\n");
      return 0;
    }

  if (!XTextPropertyToStringList (&text, &name_list, &list_len))
    {
      fprintf (stderr, "ratpoison:manage.c:Error retrieving TextList.\n");
      return 0;
    }

  for (i=0; i<list_len; i++)
    {
      printf ("WMName: %s\n", name_list[i]);
    }

  /* Set the window's name to the first in the name_list */
  if (list_len > 0)
    {
      char *loc;

      free (win->name);
      if ((win->name = malloc (strlen (name_list[0]) + 1)) == NULL)
	{
	  fprintf (stderr, "manage.c:update_window_name():Out of memory!\n");
	  exit (EXIT_FAILURE);
	}    
      strcpy (win->name, name_list[0]);

      /* A bit of a hack. If there's a : in the string, crop the
         string off there. This is mostly brought on by netscape's
         disgusting tendency to put its current URL in the WMName!!
         arg! */
      loc = strchr (win->name, ':');
      if (loc) loc[0] = '\0';
    }

  /* Its our responsibility to free this. */ 
  XFreeStringList (name_list);

  return 1;
}

void
manage (rp_window *win, screen_info *s)
{
  if (!update_window_name (win)) return;

  /* We successfully got the name, which means we can start managing! */
  XMapWindow (dpy, win->w);
  XMoveResizeWindow (dpy, win->w, 0, 0, s->root_attr.width, s->root_attr.height);
  XSelectInput (dpy, win->w, PropertyChangeMask);
  XAddToSaveSet(dpy, win->w);
  grab_prefix_key (win->w);

  win->state = STATE_MAPPED;
  win->number = get_unique_window_number ();

#ifdef DEBUG
  printf ("window '%s' managed.\n", win->name);
#endif
}

void
unmanage (rp_window *w)
{
  return_window_number (w->number);
  remove_from_window_list (w);
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
#ifdef DEBUG
  printf ("windows: %d\n", nwins);
#endif

  for (i = 0; i < nwins; i++) 
    {
      XGetWindowAttributes(dpy, wins[i], &attr);
      if (wins[i] == s->bar_window || wins[i] == s->key_window || wins[i] == s->input_window) continue;

      if (attr.override_redirect != True)
	{
	  win = add_to_window_list (s, wins[i]);
	  if (attr.map_state != IsUnmapped) manage (win, s);
	}
    }
  XFree((void *) wins);	/* cast is to shut stoopid compiler up */
}
