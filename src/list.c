/* functions for handling the window list 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ratpoison.h"

rp_window *rp_unmapped_window_sentinel;
rp_window *rp_mapped_window_sentinel;

/* Get the mouse position relative to the root of the specified window */
static void
get_mouse_root_position (rp_window *win, int *mouse_x, int *mouse_y)
{
  Window root_win, child_win;
  int win_x, win_y;
  unsigned int mask;
  
  XQueryPointer (dpy, win->scr->root, &root_win, &child_win, &win_x, &win_y, mouse_x, mouse_y, &mask);
}

void
free_window (rp_window *w)
{
  if (w == NULL) return;

  free (w->name);
  XFree (w->hints);
  
  free (w);
}


/* Allocate a new window and add it to the list of managed windows */
rp_window *
add_to_window_list (screen_info *s, Window w)
{
  rp_window *new_window;

  new_window = xmalloc (sizeof (rp_window));

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
  PRINT_DEBUG ("transient %d\n", new_window->transient);

  get_mouse_root_position (new_window, &new_window->mouse_x, &new_window->mouse_y);

  XSelectInput (dpy, new_window->w, 
		PropertyChangeMask | ColormapChangeMask | FocusChangeMask);

  new_window->name = xmalloc (strlen ("Unnamed") + 1);

  strcpy (new_window->name, "Unnamed");

  /* Add the window to the end of the unmapped list. */
  append_to_list (new_window, rp_unmapped_window_sentinel);

  return new_window;
}

/* Check to see if the window is in the list of windows. */
rp_window *
find_window_in_list (Window w, rp_window *sentinel)
{
  rp_window *cur;

  for (cur = sentinel->next; cur != sentinel; cur = cur->next)
    {
      if (cur->w == w) return cur;
    }

  return NULL;
}

/* Check to see if the window is in any of the lists of windows. */
rp_window *
find_window (Window w)
{
  rp_window *win = NULL;

  win = find_window_in_list (w, rp_mapped_window_sentinel);

  if (!win) 
    {
      win = find_window_in_list (w, rp_unmapped_window_sentinel);
    }

  return win;
}

void
set_current_window (rp_window *win)
{
  rp_current_frame->win = win;  
}

void
init_window_list ()
{
  rp_mapped_window_sentinel = xmalloc (sizeof (rp_window));
  rp_unmapped_window_sentinel = xmalloc (sizeof (rp_window));

  rp_mapped_window_sentinel->next = rp_mapped_window_sentinel;
  rp_mapped_window_sentinel->prev = rp_mapped_window_sentinel;

  rp_unmapped_window_sentinel->next = rp_unmapped_window_sentinel;
  rp_unmapped_window_sentinel->prev = rp_unmapped_window_sentinel;
}

rp_window *
find_window_number (int n)
{
  rp_window *cur;

  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
/*       if (cur->state == STATE_UNMAPPED) continue; */

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

/* return a window by name */
rp_window *
find_window_name (char *name)
{
  rp_window *w;

  for (w = rp_mapped_window_sentinel->next; 
       w != rp_mapped_window_sentinel; 
       w = w->next)
    {
/*       if (w->state == STATE_UNMAPPED)  */
/* 	continue; */

      if (str_comp (name, w->name, strlen (name))) 
	return w;
    }

  /* didn't find it */
  return NULL;
}

/* Return the previous window in the list. Assumes window is in the
   mapped window list. */
rp_window*
find_window_prev (rp_window *w)
{
  rp_window *cur;

  if (!w) return NULL;

  for (cur = w->prev; 
       cur != w;
       cur = cur->prev)
    {
      if (cur == rp_mapped_window_sentinel) continue;

      if (!find_windows_frame (cur))
	{
	  return cur;
	}
    }

  return NULL;
}

/* Return the next window in the list. Assumes window is in the mapped
   window list. */
rp_window*
find_window_next (rp_window *w)
{
  rp_window *cur;

  if (!w) return NULL;

  for (cur = w->next; 
       cur != w;
       cur = cur->next)
    {
      if (cur == rp_mapped_window_sentinel) continue;

      if (!find_windows_frame (cur))
	{
	  return cur;
	}
    }

  return NULL;
}

rp_window *
find_window_other ()
{
  int last_access = 0;
  rp_window *most_recent = NULL;
  rp_window *cur;

  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
      if (cur->last_access >= last_access 
	  && cur != current_window()
	  && !find_windows_frame (cur))
	{
	  most_recent = cur;
	  last_access = cur->last_access;
	}
    }

  return most_recent;
}

/* This somewhat CPU intensive (memcpy) swap function sorta defeats
   the purpose of a linear linked list. */
/* static void */
/* swap_list_elements (rp_window *a, rp_window *b) */
/* { */
/*   rp_window tmp; */
/*   rp_window *tmp_a_next, *tmp_a_prev; */
/*   rp_window *tmp_b_next, *tmp_b_prev; */

/*   if (a == NULL || b == NULL || a == b) return; */

/*   tmp_a_next = a->next; */
/*   tmp_a_prev = a->prev; */

/*   tmp_b_next = b->next; */
/*   tmp_b_prev = b->prev; */

/*   memcpy (&tmp, a, sizeof (rp_window)); */
/*   memcpy (a, b, sizeof (rp_window)); */
/*   memcpy (b, &tmp, sizeof (rp_window)); */

/*   a->next = tmp_a_next; */
/*   a->prev = tmp_a_prev; */

/*   b->next = tmp_b_next; */
/*   b->prev = tmp_b_prev; */
/* } */

/* Can you say b-b-b-b-bubble sort? */
/* void */
/* sort_window_list_by_number () */
/* { */
/*   rp_window *i, *j, *smallest; */

/*   for (i=rp_window_head; i; i=i->next) */
/*     { */
/*       if (i->state == STATE_UNMAPPED) continue; */

/*       smallest = i; */
/*       for (j=i->next; j; j=j->next) */
/* 	{ */
/* 	  if (j->state == STATE_UNMAPPED) continue; */

/* 	  if (j->number < smallest->number) */
/* 	    { */
/* 	      smallest = j; */
/* 	    } */
/* 	} */

/*       swap_list_elements (i, smallest); */
/*     } */
/* } */

void
append_to_list (rp_window *win, rp_window *sentinel)
{
  win->prev = sentinel->prev;
  sentinel->prev->next = win;
  sentinel->prev = win;
  win->next = sentinel;
}

/* Assumes the list is sorted by increasing number. Inserts win into
   to Right place to keep the list sorted. */
void
insert_into_list (rp_window *win, rp_window *sentinel)
{
  rp_window *cur;

  for (cur = sentinel->next; cur != sentinel; cur = cur->next)
    {
      if (cur->number > win->number)
	{
	  win->next = cur;
	  win->prev = cur->prev;

	  cur->prev->next = win;
	  cur->prev = win;

	  return;
	}
    }

  append_to_list (win, sentinel);
}

void
remove_from_list (rp_window *win)
{
  win->next->prev = win->prev;
  win->prev->next = win->next;
}

static void
save_mouse_position (rp_window *win)
{
  Window root_win, child_win;
  int win_x, win_y;
  unsigned int mask;
  
  /* In the case the XQueryPointer raises a BadWindow error, the
     window is not mapped or has been destroyed so it doesn't matter
     what we store in mouse_x and mouse_y since they will never be
     used again. */

  ignore_badwindow++;

  XQueryPointer (dpy, win->w, &root_win, &child_win, 
		 &win->mouse_x, &win->mouse_y, &win_x, &win_y, &mask);

  ignore_badwindow--;
}

void
give_window_focus (rp_window *win)
{
  static int counter = 1;	/* increments every time this function
                                   is called. This way we can track
                                   which window was last accessed.  */

  if (win == NULL) return;

  counter++;
  win->last_access = counter;

/*   if (win->scr->bar_is_raised) update_window_names (win->scr); */

  if (current_window() != NULL)
    {
      save_mouse_position (current_window());
    }

  XWarpPointer (dpy, None, win->scr->root, 
		0, 0, 0, 0, win->mouse_x, win->mouse_y);
  XSync (dpy, False);

  XSetInputFocus (dpy, win->w, 
		  RevertToPointerRoot, CurrentTime);
  
  /* Swap colormaps */
  if (current_window() != NULL)
    {
      XUninstallColormap (dpy, current_window()->colormap);
    }
  XInstallColormap (dpy, win->colormap);
}

void
set_active_window (rp_window *win)
{
  if (win == NULL) return;

  give_window_focus (win);
  rp_current_frame->win = win;

  /* Make sure the window comes up full screen */
  maximize (current_window());
  XRaiseWindow (dpy, win->w);

  /* Make sure the program bar is always on the top */
  update_window_names (win->scr);
}

/* Go to the window, switching frames if the window is already in a
   frame. */
void
goto_window (rp_window *win)
{
  rp_window_frame *frame;

  frame = find_windows_frame (win);
  if (frame)
    {
      set_active_frame (frame);
    }
  else
    {
      set_active_window (win);
    }
}
