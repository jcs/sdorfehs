/* functions that manipulate the frame structure.
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

#include "ratpoison.h"

#include <string.h>

int
frame_left (rp_frame *frame)
{
  return frame->x;
}

int
frame_top (rp_frame *frame)
{
  return frame->y;
}

int
frame_right (rp_frame *frame)
{
  return frame->x + frame->width;
}

int
frame_bottom (rp_frame *frame)
{
  return frame->y + frame->height;
}

int
frame_width(rp_frame *frame)
{
  return frame->width;
}

int
frame_height(rp_frame *frame)
{
  return frame->height;
}

void
frame_resize_left (rp_frame *frame, int amount)
{
  frame->x -= amount;
  frame->width += amount;
}

void
frame_resize_right (rp_frame *frame, int amount)
{
  frame->width += amount;
}

void
frame_resize_up (rp_frame *frame, int amount)
{
  frame->y -= amount;
  frame->height += amount;
}

void
frame_resize_down (rp_frame *frame, int amount)
{
  frame->height += amount;
}

void
frame_move_left (rp_frame *frame, int amount)
{
  frame->x -= amount;
}

void
frame_move_right (rp_frame *frame, int amount)
{
  frame->x += amount;
}

void
frame_move_up (rp_frame *frame, int amount)
{
  frame->y -= amount;
}

void
frame_move_down (rp_frame *frame, int amount)
{
  frame->y += amount;
}

static void
init_frame (rp_frame *f)
{
  f->number = 0;
  f->x = 0;
  f->y = 0;
  f->width = 0;
  f->height = 0;
  f->win_number = 0;
  f->last_access = 0;
  f->dedicated = 0;
}

rp_frame *
frame_new (rp_screen *s)
{
  rp_frame *f;

  f = xmalloc (sizeof (rp_frame));
  init_frame(f);
  f->number = numset_request (s->frames_numset);

  return f;
}

void
frame_free (rp_screen *s, rp_frame *f)
{
  numset_release (s->frames_numset, f->number);
  free (f);
}


rp_frame *
frame_copy (rp_frame *frame)
{
  rp_frame *copy;

  copy = xmalloc (sizeof (rp_frame));

  copy->number = frame->number;
  copy->x = frame->x;
  copy->y = frame->y;
  copy->width = frame->width;
  copy->height = frame->height;
  copy->win_number = frame->win_number;
  copy->last_access = frame->last_access;

  return copy;
}

char *
frame_dump (rp_frame *frame, rp_screen *screen)
{
  rp_window *win;
  char *tmp;
  struct sbuf *s;

  /* rather than use win_number, use the X11 window ID. */
  win = find_window_number (frame->win_number);

  s = sbuf_new (0);
  sbuf_printf (s, "(frame :number %d :x %d :y %d :width %d :height %d :screenw %d :screenh %d :window %ld :last-access %d :dedicated %d)", 
               frame->number,
               frame->x,
               frame->y,
               frame->width,
               frame->height,
	       screen->width,
	       screen->height,
               win ? win->w:0,
               frame->last_access,
               frame->dedicated);

  /* Extract the string and return it, and don't forget to free s. */
  tmp = sbuf_get (s);
  free (s);
  return tmp;
}

/* Used only by frame_read */
#define read_slot(x) do { tmp = strtok_ws (NULL); x = strtol(tmp,NULL,10); } while(0)

rp_frame *
frame_read (char *str, rp_screen *screen)
{
  Window w = 0L;
  rp_window *win;
  rp_frame *f;
  char *tmp, *d;
  int s_width = -1;
  int s_height = -1;

  /* Create a blank frame. */
  f = xmalloc (sizeof (rp_frame));
  init_frame(f);

  PRINT_DEBUG(("parsing '%s'\n", str));

  d = xstrdup(str);
  tmp = strtok_ws (d);

  /* Verify it starts with '(frame ' */
  if (strcmp(tmp, "(frame"))
    {
      PRINT_DEBUG(("Doesn't start with '(frame '\n"));
      free (d);
      free (f);
      return NULL;
    }
  /* NOTE: there is no check to make sure each field was filled in. */
  tmp = strtok_ws(NULL);
  while (tmp)
    {
      if (!strcmp(tmp, ":number"))
        read_slot(f->number);
      else if (!strcmp(tmp, ":x"))
        read_slot(f->x);
      else if (!strcmp(tmp, ":y"))
        read_slot(f->y);
      else if (!strcmp(tmp, ":width"))
        read_slot(f->width);
      else if (!strcmp(tmp, ":height"))
        read_slot(f->height);
      else if (!strcmp(tmp, ":screenw"))
	read_slot(s_width);
      else if (!strcmp(tmp, ":screenh"))
	read_slot(s_height);
      else if (!strcmp(tmp, ":window"))
        read_slot(w);
      else if (!strcmp(tmp, ":last-access"))
        read_slot(f->last_access);
      else if (!strcmp(tmp, ":dedicated")) {
  	/* f->dedicated is unsigned, so read into local variable. */
        long dedicated;

	read_slot(dedicated);
        if (dedicated <= 0)
          f->dedicated = 0;
        else
          f->dedicated = 1;
      }
      else if (!strcmp(tmp, ")"))
        break;
      else
        PRINT_ERROR(("Unknown slot %s\n", tmp));
      /* Read the next token. */
      tmp = strtok_ws(NULL);
    }
  if (tmp)
    PRINT_ERROR(("Frame has trailing garbage\n"));
  free (d);

  /* adjust x, y, width and height to a possible screen size change */
  if (s_width > 0)
    {
      f->x = (f->x*screen->width)/s_width;
      f->width = (f->width*screen->width)/s_width;
    }
  if (s_height > 0)
    {
      f->y = (f->y*screen->height)/s_height;
      f->height = (f->height*screen->height)/s_height;
    }

  /* Perform some integrity checks on what we got and fix any
     problems. */
  if (f->number <= 0)
    f->number = 0;
  if (f->x <= 0)
    f->x = 0;
  if (f->y <= 0)
    f->y = 0;
  if (f->width <= defaults.window_border_width*2)
    f->width = defaults.window_border_width*2 + 1;
  if (f->height <= defaults.window_border_width*2)
    f->height = defaults.window_border_width*2 + 1;
  if (f->last_access < 0)
    f->last_access = 0;

  /* Find the window with the X11 window ID. */
  win = find_window_in_list (w, &rp_mapped_window);
  if (win)
    f->win_number = win->number;
  else
    f->win_number = EMPTY;

  return f;
}

#undef read_slot
