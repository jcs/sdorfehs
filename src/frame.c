/* functions that manipulate the frame structure.
 * Copyright (C) 2000-2003 Shawn Betts
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

int
frame_left (rp_window_frame *frame)
{
  return frame->x;
}

int
frame_top (rp_window_frame *frame)
{
  return frame->y;
}

int
frame_right (rp_window_frame *frame)
{
  return frame->x + frame->width;
}

int
frame_bottom (rp_window_frame *frame)
{
  return frame->y + frame->height;
}

int
frame_width(rp_window_frame *frame)
{
  return frame->width;
}

int
frame_height(rp_window_frame *frame)
{
  return frame->height;
}

void
frame_resize_left (rp_window_frame *frame, int amount)
{
  frame->x -= amount;
  frame->width += amount;
}

void
frame_resize_right (rp_window_frame *frame, int amount)
{
  frame->width += amount;
}

void
frame_resize_up (rp_window_frame *frame, int amount)
{
  frame->y -= amount;
  frame->height += amount;
}

void
frame_resize_down (rp_window_frame *frame, int amount)
{
  frame->height += amount;
}

void
frame_move_left (rp_window_frame *frame, int amount)
{
  frame->x -= amount;
}

void
frame_move_right (rp_window_frame *frame, int amount)
{
  frame->x += amount;
}

void
frame_move_up (rp_window_frame *frame, int amount)
{
  frame->y -= amount;
}

void
frame_move_down (rp_window_frame *frame, int amount)
{
  frame->y += amount;
}

rp_window_frame *
frame_new (screen_info *s)
{
  rp_window_frame *f;

  f = xmalloc (sizeof (rp_window_frame));
  f->number = numset_request (s->frames_numset);

  return f;
}

void
frame_free (screen_info *s, rp_window_frame *f)
{
  numset_release (s->frames_numset, f->number);
  free (f);
}


rp_window_frame *
frame_copy (rp_window_frame *frame)
{
  rp_window_frame *copy;

  copy = xmalloc (sizeof (rp_window_frame));

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
frame_dump (rp_window_frame *frame)
{
  char *tmp;
  struct sbuf *s;

  s = sbuf_new (0);
  sbuf_printf (s, "%d %d %d %d %d %d %d", 
	       frame->number,
	       frame->x,
	       frame->y,
	       frame->width,
	       frame->height,
	       frame->win_number,
	       frame->last_access);

  /* Extract the string and return it, and don't forget to free s. */
  tmp = sbuf_get (s);
  free (s);
  return tmp;
}
