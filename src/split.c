/* Copyright (C) 2000, 2001 Shawn Betts
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
 *
 *
 * Functions for handling window splitting and tiling.
 */

#include "ratpoison.h"

static int
window_fits_in_frame (rp_window *win, rp_window_frame *frame)
{
  /* If the window has minimum size hints, make sure they are smaller
     than the frame. */
  if (win->hints->flags & PMinSize)
    {
      if (win->hints->min_width > frame->width
	  ||
	  win->hints->min_height > frame->height)
	{
	  return 0;
	}
    }

  return 1;
}

/* Search the list of mapped windows for a window that will fit in the
   specified frame. */
rp_window *
find_window_for_frame (rp_window_frame *frame)
{
  int last_access = 0;
  rp_window *most_recent = NULL;
  rp_window *cur;

  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
      if (cur != rp_current_window 
	  && !cur->frame
	  && cur->last_access >= last_access
	  && window_fits_in_frame (cur, frame)) 
	{
	  most_recent = cur;
	  last_access = cur->last_access;
	}
    }
  
  return most_recent;
}

/* Splits the window in 2. if way is != 0 then split it horizontally
   otherwise split it vertically. */
static int
split_window (rp_window *win, int way)
{
  rp_window *other_window;
  rp_window_frame *frame1, *frame2;

  frame1 = xmalloc (sizeof (rp_window_frame));
  frame2 = xmalloc (sizeof (rp_window_frame));

  if (!win->frame)
    {
      frame1->x = PADDING_LEFT;
      frame1->y = PADDING_TOP;
      frame1->width = win->scr->root_attr.width 
	- PADDING_RIGHT - PADDING_LEFT;
      frame1->height = win->scr->root_attr.height 
	- PADDING_BOTTOM - PADDING_TOP;
    }
  else
    {
      frame1->x = win->frame->x;
      frame1->y = win->frame->y;
      frame1->width = win->frame->width;
      frame1->height = win->frame->height;
    }

  if (way)
    {
      frame2->x = frame1->x;
      frame2->y = frame1->y + frame1->height / 2;
      frame2->width = frame1->width;
      frame2->height = frame1->height / 2 + frame1->height % 2;

      frame1->height /= 2;
    }
  else
    {
      frame2->x = frame1->x + frame1->width / 2;
      frame2->y = frame1->y;
      frame2->width = frame1->width / 2 + frame1->width % 2;
      frame2->height = frame1->height;

      frame1->width /= 2;
    }
  
  other_window = find_window_for_frame (frame2);
  if (other_window)
    {
      PRINT_DEBUG ("Split the window!\n");

      if (win->frame) free (win->frame);
      win->frame = frame1;
      other_window->frame = frame2;

      maximize (win);
      maximize (other_window);
      XRaiseWindow (dpy, other_window->w);
      XRaiseWindow (dpy, win->w);
      
      return 1;
    }
  else
    {
      PRINT_DEBUG ("Failed to split.\n");

      free (frame1);
      free (frame2);

      return 0;
    }
}

/* Splits the window vertically in 2. */
int
v_split_window (rp_window *win)
{
  return split_window (win, 0);
}

/* Splits the window horizontally in 2. */
int
h_split_window (rp_window *win)
{
  return split_window (win, 1);
}

void
remove_all_frames ()
{
  rp_window *cur;

  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
      if (cur->frame)
	{
	  free (cur->frame);
	  cur->frame = NULL;
	}
    }
}

static int
frame_is_below (rp_window_frame *src, rp_window_frame *frame)
{
  if (frame->y > src->y) return 1;
  return 0;
}

static int
frame_is_above (rp_window_frame *src, rp_window_frame *frame)
{
  if (frame->y < src->y) return 1;
  return 0;
}

static int
frame_is_left (rp_window_frame *src, rp_window_frame *frame)
{
  if (frame->x < src->x) return 1;
  return 0;
}

static int
frame_is_right (rp_window_frame *src, rp_window_frame *frame)
{
  if (frame->x > src->x) return 1;
  return 0;
}

static int
total_frame_area ()
{
  int area = 0;
  rp_window *cur;

  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
      if (cur->frame)
	{
	  area += cur->frame->width * cur->frame->height;
	}
    }

  return area;
}

static int
num_frames ()
{
  int count = 0;
  rp_window *cur;

  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
      if (cur->frame)
	{
	  count++;
	}
    }

  return count;
}

/* Return 1 if frames f1 and f2 overlap */
static int
frames_overlap (rp_window_frame *f1, rp_window_frame *f2)
{
  if (f1->x >= f2->x + f2->width 
      || f1->y >= f2->y + f2->height
      || f2->x >= f1->x + f1->width 
      || f2->y >= f1->y + f1->height)
    {
      return 0;
    }
  return 1;
}

/* Return 1 if w's frame overlaps any other window's frame */
static int
frame_overlaps (rp_window *w)
{
  rp_window *cur;

  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
      if (cur != w
	  && cur->frame
	  && frames_overlap (cur->frame, w->frame))
	{
	  return 1;
	}
    }
  return 0;
}

void
remove_frame (rp_window *w)
{
  int area;
  rp_window *cur;
  rp_window_frame *frame;

  if (w->frame == NULL) return;

  area = total_frame_area();
  PRINT_DEBUG ("Total Area: %d\n", area);

  frame = w->frame;
  w->frame = NULL;

  /* If there is 1 frame then we have no split screen, so get rid
     of the remaining window frame. */
  if (num_frames() <= 1)
    {
      remove_all_frames();
      free (frame);
      return;
    }

  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
      if (cur->frame)
	{
	  rp_window_frame tmp_frame;
	  int fits = 0;

	  /* Backup the frame */
	  memcpy (&tmp_frame, cur->frame, sizeof (rp_window_frame));

	  if (frame_is_below (frame, cur->frame)
	      || frame_is_above (frame, cur->frame))
	    {
	      if (frame_is_below (frame, cur->frame))
		cur->frame->y = frame->y;
	      cur->frame->height += frame->height;
	    }

	  PRINT_DEBUG ("New Total Area: %d\n", total_frame_area());

	  if (total_frame_area() > area || frame_overlaps (cur))
	    {
	      PRINT_DEBUG ("Didn't fit vertically\n");

	      /* Restore the current window's frame */
	      memcpy (cur->frame, &tmp_frame, sizeof (rp_window_frame));
	    }
	  else
	    {
	      PRINT_DEBUG ("It fit vertically!!\n");

	      /* update the frame backup */
	      memcpy (&tmp_frame, cur->frame, sizeof (rp_window_frame));
	      fits = 1;
	    }

	  if (frame_is_left (frame, cur->frame)
	      || frame_is_right (frame, cur->frame))
	    {
	      if (frame_is_right (frame, cur->frame))
		cur->frame->x = frame->x;
	      cur->frame->width += frame->width;
	    }

	  PRINT_DEBUG ("New Total Area: %d\n", total_frame_area());

	  if (total_frame_area() > area || frame_overlaps (cur))
	    {
	      PRINT_DEBUG ("Didn't fit horizontally\n");

	      /* Restore the current window's frame */
	      memcpy (cur->frame, &tmp_frame, sizeof (rp_window_frame));
	    }
	  else
	    {
	      PRINT_DEBUG ("It fit horizontally!!\n");
	      fits = 1;
	    }

	  if (fits)
	    {
	      /* The current frame fits into the new space so keep its
		 new frame parameters and maximize the window to fit
		 the new frame size. */
	      maximize (cur);
	      XRaiseWindow (dpy, cur->w);
	    }
	  else
	    {
	      memcpy (cur->frame, &tmp_frame, sizeof (rp_window_frame));
	    }

	}
    }
  
  free (frame);
}
