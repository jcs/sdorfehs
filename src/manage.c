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

const char *unmanaged_window_list[] = {
#ifdef UNMANAGED_WINDOW_LIST
  UNMANAGED_WINDOW_LIST,
#endif
  NULL
};

extern Atom wm_state;

static void
grab_prefix_key (Window w)
{
  XGrabKey(dpy, XKeysymToKeycode (dpy, KEY_PREFIX ), MODIFIER_PREFIX, w, True, 
	   GrabModeAsync, GrabModeAsync);
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
      if ((name = malloc (strlen (name_list[0]) + 1)) == NULL)
	{
	  PRINT_ERROR ("Out of memory!\n");
	  exit (EXIT_FAILURE);
	}    
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
  char *loc;

  /* Don't overwrite the window name if the user specified one. */
  if (win->named) return 0;

  win->name = get_window_name (win->w);

  /* A bit of a hack. If there's a : in the string, crop the
     string off there. This is mostly brought on by netscape's
     disgusting tendency to put its current URL in the WMName!!
     arg! */
  loc = strchr (win->name, ':');
  if (loc) loc[0] = '\0';

  return 1;
}


void
manage (rp_window *win, screen_info *s)
{
  if (!update_window_name (win)) return;

  /* We successfully got the name, which means we can start managing! */
  XMapWindow (dpy, win->w);
  XMoveResizeWindow (dpy, win->w, 
		     PADDING_LEFT, 
		     PADDING_TOP, 
		     s->root_attr.width - PADDING_LEFT - PADDING_RIGHT, 
		     s->root_attr.height - PADDING_TOP - PADDING_BOTTOM);
  XSelectInput (dpy, win->w, PropertyChangeMask);
  XAddToSaveSet(dpy, win->w);
  grab_prefix_key (win->w);

  win->state = STATE_MAPPED;
  win->number = get_unique_window_number ();

  PRINT_DEBUG ("window '%s' managed.\n", win->name);
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
  PRINT_DEBUG ("windows: %d\n", nwins);

  for (i = 0; i < nwins; i++) 
    {
      XGetWindowAttributes(dpy, wins[i], &attr);
      if (wins[i] == s->bar_window || wins[i] == s->key_window || wins[i] == s->input_window) continue;

      if (attr.override_redirect != True && !unmanaged_window (wins[i]))
	{
	  
	  win = add_to_window_list (s, wins[i]);
	  PRINT_DEBUG ("map_state: %d\n", attr.map_state);
	  if (attr.map_state == IsViewable) manage (win, s);
	}
    }
  XFree((void *) wins);	/* cast is to shut stoopid compiler up */
}

int
unmanaged_window (Window w)
{
  int i;

  for (i = 0; unmanaged_window_list[i]; i++) 
    if (!strcmp (unmanaged_window_list[i], get_window_name (w)))
      return 1;

  return 0;
}

