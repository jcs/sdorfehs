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
  new_window->state = WithdrawnState;
  new_window->number = -1;	
  new_window->named = 0;
  new_window->hints = XAllocSizeHints ();
  new_window->colormap = DefaultColormap (dpy, s->screen_num);
  new_window->transient = XGetTransientForHint (dpy, new_window->w, &new_window->transient_for);
  PRINT_DEBUG ("transient %d\n", new_window->transient);

  get_mouse_root_position (new_window, &new_window->mouse_x, &new_window->mouse_y);

  XSelectInput (dpy, new_window->w, WIN_EVENTS);

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

/* Takes focus away from last_win and gives focus to win */
void
give_window_focus (rp_window *win, rp_window *last_win)
{
  /* counter increments every time this function is called. This way
     we can track which window was last accessed. */
  static int counter = 1;

  if (win == NULL) return;

  counter++;
  win->last_access = counter;

  unhide_window (win);

  /* Warp the cursor to the window's saved position. */
  if (last_win != NULL) save_mouse_position (last_win);
  XWarpPointer (dpy, None, win->scr->root, 
		0, 0, 0, 0, win->mouse_x, win->mouse_y);
  
  /* Swap colormaps */
  if (last_win != NULL) XUninstallColormap (dpy, last_win->colormap);
  XInstallColormap (dpy, win->colormap);

  /* Finally, give the window focus */
  XSetInputFocus (dpy, win->w, 
		  RevertToPointerRoot, CurrentTime);

  XSync (dpy, False);
}

void
unhide_transient_for (rp_window *win)
{
  rp_window_frame *frame;
  rp_window *transient_for;

  if (win == NULL) return;
  if (!win->transient) return;
  frame = find_windows_frame (win);

  transient_for = find_window (win->transient_for);
  if (transient_for == NULL) 
    {
      PRINT_DEBUG ("Can't find transient_for for '%s'", win->name );
      return;
    }

  if (find_windows_frame (transient_for) == NULL)
    {
      frame->win = transient_for;
      maximize (transient_for);

      PRINT_DEBUG ("unhide transient window: %s\n", transient_for->name);
      
      unhide_below_window (transient_for);

      if (transient_for->transient) 
	{
	  unhide_transient_for (transient_for);
	}

      frame->win = win;
    }
  else if (transient_for->transient) 
    {
      unhide_transient_for (transient_for);
    }
}

void
hide_transient_for (rp_window *win)
{
  rp_window *transient_for;

  if (win == NULL) return;
  if (!win->transient) return;

  transient_for = find_window (win->transient_for);
  if (transient_for == NULL) 
    {
      PRINT_DEBUG ("Can't find transient_for for '%s'", win->name );
      return;
    }

  if (find_windows_frame (transient_for) == NULL)
    {
      PRINT_DEBUG ("hide transient window: %s\n", transient_for->name);
      hide_window (transient_for);
    }
  
  if (transient_for->transient) 
    {
      hide_transient_for (transient_for);
    }
}


void
set_active_window (rp_window *win)
{
  rp_window *last_win;

  if (win == NULL) return;

  last_win = rp_current_frame->win;
  rp_current_frame->win = win;

  if (last_win) PRINT_DEBUG ("last window: %s\n", last_win->name);
  PRINT_DEBUG ("new window: %s\n", win->name);

  if (win->transient
      && find_window (win->transient_for) == last_win)
    {
      /* Do nothing if the transient is already in the frame */
    }
  else
    {
      hide_window (last_win);
    }

  hide_transient_for (last_win);

  /* Make sure the window comes up full screen */
  unhide_transient_for (win);
  maximize (win);
  unhide_window (win);

  give_window_focus (win, last_win);

  /* Make sure the program bar is always on the top */
  update_window_names (win->scr);

  XSync (dpy, False);
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

void
print_window_information (rp_window *win)
{
  char number[3];
  char *str;

  snprintf (number, 3, "%d", win->number);

  /* There is a bit of extra memory being allocated from the
     formatting tags in MESSAGE_WINDOW_INFORMATION, but it is not a
     couple bytes. */
  str = xmalloc (strlen (number) + strlen (win->name) + strlen (MESSAGE_WINDOW_INFORMATION) + 1);

  sprintf (str, MESSAGE_WINDOW_INFORMATION, number, win->name);
  message (str);
  free (str);

  return;
}
