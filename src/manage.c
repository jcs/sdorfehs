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

  /* Don't overwrite the window name if the user specified one. */
  if (win->named) return 0;

  if (!XGetWMName (dpy, win->w, &text))
    {
      PRINT_DEBUG ("I can't get the WMName.\n");
      return 0;
    }

  if (!XTextPropertyToStringList (&text, &name_list, &list_len))
    {
      PRINT_DEBUG ("Error retrieving TextList.\n");
      return 0;
    }

  /* Sorta sick... */
#ifdef DEBUG
  {
    int i;

    for (i=0; i<list_len; i++)
      {
	PRINT_DEBUG ("WMName: %s\n", name_list[i]);
      }
  }
#endif /* DEBUG */

  /* Set the window's name to the first in the name_list */
  if (list_len > 0)
    {
      char *loc;

      free (win->name);
      if ((win->name = malloc (strlen (name_list[0]) + 1)) == NULL)
	{
	  PRINT_ERROR ("Out of memory!\n");
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
rename_current_window ()
{
  char winname[100];
  
  if (rp_current_window == NULL) return;

  get_input (rp_current_window->scr, "Name: ", winname, 100);
  PRINT_DEBUG ("user entered: %s\n", winname);

  free (rp_current_window->name);
  rp_current_window->name = malloc (sizeof (char) * strlen (winname) + 1);
  if (rp_current_window->name == NULL)
    {
      PRINT_ERROR ("Out of memory\n");
      exit (EXIT_FAILURE);
    }
  strcpy (rp_current_window->name, winname);
  rp_current_window->named = 1;
  
  /* Update the program bar. */
  update_window_names (rp_current_window->scr);
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

      if (attr.override_redirect != True)
	{
	  win = add_to_window_list (s, wins[i]);
	  if (attr.map_state != IsUnmapped) manage (win, s);
	}
    }
  XFree((void *) wins);	/* cast is to shut stoopid compiler up */
}
