/* functions for handling the window list 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ratpoison.h"

rp_window *rp_window_head, *rp_window_tail;
rp_window *rp_current_window;

/* Get the mouse position relative to the root of the specified window */
static void
get_mouse_root_position (rp_window *win, int *mouse_x, int *mouse_y)
{
  Window root_win, child_win;
  int win_x, win_y;
  int mask;
  
  XQueryPointer (dpy, win->scr->root, &root_win, &child_win, &win_x, &win_y, mouse_x, mouse_y, &mask);
}

/* Allocate a new window and add it to the list of managed windows */
rp_window *
add_to_window_list (screen_info *s, Window w)
{
  rp_window *new_window;

  new_window = malloc (sizeof (rp_window));
  if (new_window == NULL)
    {
      PRINT_ERROR ("Out of memory!\n");
      exit (EXIT_FAILURE);
    }
  new_window->w = w;
  new_window->scr = s;
  new_window->last_access = 0;
  new_window->prev = NULL;
  new_window->state = STATE_UNMAPPED;
  new_window->number = -1;	
  new_window->named = 0;
  new_window->hints = XAllocSizeHints ();
  new_window->colormap = DefaultColormap (dpy, s->screen_num);
  new_window->transient = XGetTransientForHint (dpy, new_window->w, &new_window->transient_for);

  get_mouse_root_position (new_window, &new_window->mouse_x, &new_window->mouse_y);

  PRINT_DEBUG ("transient %d\n", new_window->transient);
  
  if ((new_window->name = malloc (strlen ("Unnamed") + 1)) == NULL)
    {
      PRINT_ERROR ("Out of memory!\n");
      exit (EXIT_FAILURE);
    }
  strcpy (new_window->name, "Unnamed");

  if (rp_window_head == NULL)
    {
      /* The list is empty. */
      rp_window_head = new_window;
      rp_window_tail = new_window;
      new_window->next = NULL;
      return new_window;
    }

  /* Add the window to the head of the list. */
  new_window->next = rp_window_head;
  rp_window_head->prev = new_window;
  rp_window_head = new_window;

  return new_window;
}

/* Check to see if the window is already in our list of managed windows. */
rp_window *
find_window (Window w)
{
  rp_window *cur;

  for (cur = rp_window_head; cur; cur = cur->next)
    if (cur->w == w) return cur;

  return NULL;
}
      
void
remove_from_window_list (rp_window *w)
{
  if (rp_window_head == w) rp_window_head = w->next;
  if (rp_window_tail == w) rp_window_tail = w->prev;

  if (w->prev != NULL) w->prev->next = w->next;
  if (w->next != NULL) w->next->prev = w->prev;

  /* set rp_current_window to NULL, so a dangling pointer is not
     left. */
  if (rp_current_window == w) rp_current_window = NULL;

  XFree (w->hints);
  free (w);
  PRINT_DEBUG ("Removed window from list.\n");
}

void
set_current_window (rp_window *win)
{
  rp_current_window = win;  
}

void
init_window_list ()
{
  rp_window_head = rp_window_tail = NULL;
  rp_current_window = NULL;
}

rp_window *
find_window_by_number (int n)
{
  rp_window *cur;

  for (cur=rp_window_head; cur; cur=cur->next)
    {
      if (cur->state == STATE_UNMAPPED) continue;

      if (n == cur->number) return cur;
    }

  return NULL;
}

/* A case insensitive strncmp. */
static int
str_comp (char *s1, char *s2, int len)
{
  int i;

  for (i=0; i<len; i++)
    if (toupper (s1[i]) != toupper (s2[i])) return 0;

  return 1;
}

static rp_window *
find_window_by_name (char *name)
{
  rp_window *cur;

  for (cur=rp_window_head; cur; cur=cur->next)
    {
      if (cur->state == STATE_UNMAPPED) continue;

      if (str_comp (name, cur->name, strlen (name))) return cur;
    }

  return NULL;
}

int
goto_window_name (char *name)
{
  rp_window *win;

  if ((win = find_window_by_name (name)) == NULL)
    {
      return 0;
    }

  set_active_window (win);
  return 1;
}

rp_window *
find_last_accessed_window ()
{
  int last_access = 0;
  rp_window *cur, *most_recent;

  /* Find the first mapped window */
  for (most_recent = rp_window_head; most_recent; most_recent=most_recent->next)
    {
      if (most_recent->state == STATE_MAPPED) break;
    }

  /* If there are no mapped windows, don't bother with the next part */
  if (most_recent == NULL) return NULL;

  for (cur=rp_window_head; cur; cur=cur->next)
    {
      if (cur->last_access >= last_access 
	  && cur != rp_current_window 
	  && cur->state == STATE_MAPPED) 
	{
	  most_recent = cur;
	  last_access = cur->last_access;
	}
    }

  return most_recent;
}

static void
save_mouse_position (rp_window *win)
{
  Window root_win, child_win;
  int win_x, win_y;
  int mask;
  
  /* In the case the XQueryPointer raises a BadWindow error, the
     window is not mapped or has been destroyed so it doesn't matter
     what we store in mouse_x and mouse_y since they will never be
     used again. */

  ignore_badwindow = 1;

  XQueryPointer (dpy, win->w, &root_win, &child_win, 
		 &win_x, &win_y, &win->mouse_x, &win->mouse_y, &mask);

  ignore_badwindow = 0;
}

void
set_active_window (rp_window *rp_w)
{
  static int counter = 1;	/* increments every time this function
                                   is called. This way we can track
                                   which window was last accessed.  */

  if (rp_w == NULL) return;

  counter++;
  rp_w->last_access = counter;

  if (rp_w->scr->bar_is_raised) update_window_names (rp_w->scr);

  if (rp_current_window != NULL)
    {
      save_mouse_position (rp_current_window);
    }

  XWarpPointer (dpy, None, rp_w->w, 0, 0, 0, 0, rp_w->mouse_x, rp_w->mouse_y);
  XSync (dpy, False);

  XSetInputFocus (dpy, rp_w->w, 
		  RevertToPointerRoot, CurrentTime);
  XRaiseWindow (dpy, rp_w->w);
  
  /* Swap colormaps */
  if (rp_current_window != NULL)
    {
      XUninstallColormap (dpy, rp_current_window->colormap);
    }
  XInstallColormap (dpy, rp_w->colormap);

  rp_current_window = rp_w;

      
  /* Make sure the program bar is always on the top */
  update_window_names (rp_w->scr);
}
