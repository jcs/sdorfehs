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

rp_window *
add_to_window_list (screen_info *s, Window w)
{
  rp_window *new_window;

  new_window = malloc (sizeof (rp_window));
  if (new_window == NULL)
    {
      fprintf (stderr, "list.c:add_to_window_list():Out of memory!\n");
      exit (EXIT_FAILURE);
    }
  new_window->w = w;
  new_window->scr = s;
  new_window->last_access = 0;
  new_window->prev = NULL;
  new_window->state = STATE_UNMAPPED;
  new_window->number = -1;	
  if ((new_window->name = malloc (strlen ("Unnamed") + 1)) == NULL)
    {
      fprintf (stderr, "list.c:add_to_window_list():Out of memory.\n");
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

  free (w);
#ifdef DEBUG
  printf ("Removed window from list.\n");
#endif
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

void
next_window ()
{
  if (rp_current_window != NULL)
    {
      rp_current_window = rp_current_window->next;
      if (rp_current_window == NULL) 
	{
	  rp_current_window = rp_window_head;
	}
      if (rp_current_window->state == STATE_UNMAPPED) next_window ();
      set_active_window (rp_current_window);
    }
}

void
prev_window ()
{
  if (rp_current_window != NULL)
    {
      set_current_window (rp_current_window->prev);
      if (rp_current_window == NULL) 
	{
	  rp_current_window = rp_window_tail;
	}
      if (rp_current_window->state == STATE_UNMAPPED) prev_window ();
      set_active_window (rp_current_window);
    }
}

rp_window *
find_window_by_number (int n)
{
  rp_window *cur;

  for (cur=rp_window_head; cur; cur=cur->next)
    {
      if (cur->state != STATE_MAPPED) continue;

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
      if (str_comp (name, cur->name, strlen (name))) return cur;
    }

  return NULL;
}

void
goto_window_name (char *name)
{
  rp_window *win;

  if ((win = find_window_by_name (name)) == NULL)
    {
      return;
    }

  rp_current_window = win;
  set_active_window (rp_current_window);
}

void
goto_window_number (int n)
{
  rp_window *win;

  if ((win = find_window_by_number (n)) == NULL)
    {
      return;
    }

  rp_current_window = win;
  set_active_window (rp_current_window);
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

void
last_window ()
{
  rp_current_window = find_last_accessed_window ();
  set_active_window (rp_current_window);
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

  XSetInputFocus (dpy, rp_w->w, 
		  RevertToPointerRoot, CurrentTime);
  XRaiseWindow (dpy, rp_w->w);
      
  /* Make sure the program bar is always on the top */
  update_window_names (rp_w->scr);
}
