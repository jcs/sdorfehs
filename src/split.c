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

#include <unistd.h>

#include "ratpoison.h"

rp_window_frame *rp_window_frame_sentinel;
rp_window_frame *rp_current_frame;

static void
delete_frame_from_list (rp_window_frame *frame)
{
  frame->next->prev = frame->prev;
  frame->prev->next = frame->next;
}

/* Make the frame occupy the entire screen */ 
static void
maximize_frame (rp_window_frame *frame)
{
  frame->x = PADDING_LEFT;
  frame->y = PADDING_TOP;

  /* FIXME: what about multiple screens? */
  frame->width = DisplayWidth (dpy, 0) - PADDING_RIGHT - PADDING_LEFT;
  frame->height = DisplayHeight (dpy, 0) - PADDING_BOTTOM - PADDING_TOP;
}

/* Create a full screen frame */
static void
create_initial_frame ()
{
  rp_current_frame = xmalloc (sizeof (rp_window_frame));

  rp_window_frame_sentinel->next = rp_current_frame;
  rp_window_frame_sentinel->prev = rp_current_frame;
  rp_current_frame->next = rp_window_frame_sentinel;
  rp_current_frame->prev = rp_window_frame_sentinel;

  maximize_frame (rp_current_frame);
  
  rp_current_frame->win = NULL;
}

void
init_frame_list ()
{
  rp_window_frame_sentinel = xmalloc (sizeof (rp_window_frame));

  rp_window_frame_sentinel->next = rp_window_frame_sentinel;
  rp_window_frame_sentinel->prev = rp_window_frame_sentinel;

  create_initial_frame();
}

/* Return the frame that contains the window. */
rp_window_frame *
find_windows_frame (rp_window *win)
{
  rp_window_frame *cur;

  for (cur = rp_window_frame_sentinel->next; 
       cur != rp_window_frame_sentinel;
       cur = cur->next)
    {
      if (cur->win == win) return cur;
    }

  return NULL;
}

rp_window_frame *
find_frame_next (rp_window_frame *frame)
{
  rp_window_frame *cur;

  if (frame == NULL) return NULL;

  cur = frame;
  if (cur->next == rp_window_frame_sentinel)
    {
      cur = cur->next;
      if (cur->next == frame) return NULL;
    }

  return cur->next;
}

rp_window_frame *
find_frame_prev (rp_window_frame *frame)
{
  rp_window_frame *cur;

  if (frame == NULL) return NULL;

  cur = frame;
  if (cur->prev == rp_window_frame_sentinel)
    {
      cur = cur->prev;
      if (cur->prev == frame) return NULL;
    }

  return cur->prev;
}

rp_window *
current_window ()
{
  if (rp_current_frame) return rp_current_frame->win;

  PRINT_ERROR ("BUG: There should always be a current frame\n");
  return NULL;
}

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
      if (cur != current_window() 
	  && !find_windows_frame (cur)
	  && cur->last_access >= last_access
	  && window_fits_in_frame (cur, frame)) 
	{
	  most_recent = cur;
	  last_access = cur->last_access;
	}
    }
  
  return most_recent;
}

/* Splits the frame in 2. if way is 0 then split vertically otherwise
   split it horizontally. */
static void
split_frame (rp_window_frame *frame, int way)
{
  rp_window *win;
  rp_window_frame *new_frame;

  new_frame = xmalloc (sizeof (rp_window_frame));

  /* append the new frame to the list */
  new_frame->prev = rp_window_frame_sentinel->prev;
  rp_window_frame_sentinel->prev->next = new_frame;
  rp_window_frame_sentinel->prev = new_frame;
  new_frame->next = rp_window_frame_sentinel;

  new_frame->win = NULL;

  if (way)
    {
      new_frame->x = frame->x;
      new_frame->y = frame->y + frame->height / 2;
      new_frame->width = frame->width;
      new_frame->height = frame->height / 2 + frame->height % 2;

      frame->height /= 2;
    }
  else
    {
      new_frame->x = frame->x + frame->width / 2;
      new_frame->y = frame->y;
      new_frame->width = frame->width / 2 + frame->width % 2;
      new_frame->height = frame->height;

      frame->width /= 2;
    }

  win = find_window_for_frame (new_frame);
  if (win)
    {
      PRINT_DEBUG ("Found a window for the frame!\n");

      new_frame->win = win;

      maximize (win);
      unhide_window (win);
      XRaiseWindow (dpy, win->w);
    }
  else
    {
      PRINT_DEBUG ("No window fits the frame.\n");

      new_frame->win = NULL;
    }

  /* resize the existing frame */
  if (frame->win)
    {
      maximize (frame->win);
      XRaiseWindow (dpy, frame->win->w);
    }

  show_frame_indicator();
}

/* Splits the window vertically in 2. */
void
v_split_frame (rp_window_frame *frame)
{
  split_frame (frame, 0);
}

/* Splits the window horizontally in 2. */
void
h_split_frame (rp_window_frame *frame)
{
  split_frame (frame, 1);
}

void
remove_all_splits ()
{
  rp_window *cur_window;
  rp_window_frame *cur;

  cur_window = current_window();

  while (rp_window_frame_sentinel->next != rp_window_frame_sentinel)
    {
      cur = rp_window_frame_sentinel->next;
      delete_frame_from_list (cur);
      if (cur != rp_current_frame) hide_window (cur->win);
      free (cur);
    }

  create_initial_frame ();
  rp_current_frame->win = cur_window;
  maximize (cur_window);
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
  rp_window_frame *cur;

  for (cur = rp_window_frame_sentinel->next; 
       cur != rp_window_frame_sentinel; 
       cur = cur->next)
    {
      area += cur->width * cur->height;
    }

  return area;
}

/* static int */
/* num_frames () */
/* { */
/*   int count = 0; */
/*   rp_window_frame *cur; */

/*   for (cur = rp_window_frame_sentinel->next;  */
/*        cur != rp_window_frame_sentinel;  */
/*        cur = cur->next) */
/*     { */
/*       count++; */
/*     } */

/*   return count; */
/* } */

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
frame_overlaps (rp_window_frame *frame)
{
  rp_window_frame *cur;

  for (cur = rp_window_frame_sentinel->next; 
       cur != rp_window_frame_sentinel; 
       cur = cur->next)
    {
      if (cur != frame && frames_overlap (cur, frame))
	{
	  return 1;
	}
    }
  return 0;
}

void
remove_frame (rp_window_frame *frame)
{
  int area;
  rp_window_frame *cur;

  if (frame == NULL) return;

  area = total_frame_area();
  PRINT_DEBUG ("Total Area: %d\n", area);

  delete_frame_from_list (frame);
  hide_window (frame->win);

  for (cur = rp_window_frame_sentinel->next; 
       cur != rp_window_frame_sentinel; 
       cur = cur->next)
    {
      rp_window_frame tmp_frame;
      int fits = 0;

      /* Backup the frame */
      memcpy (&tmp_frame, cur, sizeof (rp_window_frame));

      if (frame_is_below (frame, cur)
	  || frame_is_above (frame, cur))
	{
	  if (frame_is_below (frame, cur))
	    cur->y = frame->y;
	  cur->height += frame->height;
	}

      PRINT_DEBUG ("New Total Area: %d\n", total_frame_area());

      if (total_frame_area() > area || frame_overlaps (cur))
	{
	  PRINT_DEBUG ("Didn't fit vertically\n");

	  /* Restore the current window's frame */
	  memcpy (cur, &tmp_frame, sizeof (rp_window_frame));
	}
      else
	{
	  PRINT_DEBUG ("It fit vertically!!\n");

	  /* update the frame backup */
	  memcpy (&tmp_frame, cur, sizeof (rp_window_frame));
	  fits = 1;
	}

      if (frame_is_left (frame, cur)
	  || frame_is_right (frame, cur))
	{
	  if (frame_is_right (frame, cur))
	    cur->x = frame->x;
	  cur->width += frame->width;
	}

      PRINT_DEBUG ("New Total Area: %d\n", total_frame_area());

      if (total_frame_area() > area || frame_overlaps (cur))
	{
	  PRINT_DEBUG ("Didn't fit horizontally\n");

	  /* Restore the current window's frame */
	  memcpy (cur, &tmp_frame, sizeof (rp_window_frame));
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
	  if (cur->win)
	    {
	      maximize (cur->win);
	      XRaiseWindow (dpy, cur->win->w);
	    }
	}
      else
	{
	  memcpy (cur, &tmp_frame, sizeof (rp_window_frame));
	}
    }
 
  free (frame);
}

void
set_active_frame (rp_window_frame *frame)
{
  rp_window_frame *old = rp_current_frame;

  give_window_focus (frame->win, rp_current_frame->win);
  rp_current_frame = frame;

  if (!frame->win || old != rp_current_frame) 
    {
      show_frame_indicator();
    }

  /* If the frame has no window to give focus to, give the frame
     indicator focus. */
  if( !frame->win )
    {
      XSetInputFocus (dpy, current_screen()->frame_window, 
		  RevertToPointerRoot, CurrentTime);
    }
}

void
blank_frame (rp_window_frame *frame)
{
  if (frame->win == NULL) return;

  hide_transient_for (frame->win);
  hide_window (frame->win);
  frame->win = NULL;

  if (frame == rp_current_frame)
    {
      show_frame_indicator();  
      XSetInputFocus (dpy, current_screen()->frame_window, 
		      RevertToPointerRoot, CurrentTime);
    }
}

void
hide_frame_indicator ()
{
  /* Only hide the frame indicator if a window occupies the frame */
  if (rp_current_frame->win)
    {
      XUnmapWindow (dpy, current_screen()->frame_window);
    }
}

void
show_frame_indicator ()
{
  screen_info *s = current_screen ();
  int width, height;

  width = BAR_X_PADDING * 2 + XTextWidth (s->font, FRAME_STRING, strlen (FRAME_STRING));
  height = (FONT_HEIGHT (s->font) + BAR_Y_PADDING * 2);

  XMoveResizeWindow (dpy, current_screen()->frame_window,
		     rp_current_frame->x + rp_current_frame->width / 2 - width / 2, 
		     rp_current_frame->y + rp_current_frame->height / 2 - height / 2,
		     width, height);

  XMapRaised (dpy, current_screen()->frame_window);

  XClearWindow (dpy, s->frame_window);
  XDrawString (dpy, s->frame_window, s->normal_gc, 
	       BAR_X_PADDING, 
	       BAR_Y_PADDING + s->font->max_bounds.ascent,
	       FRAME_STRING, strlen (FRAME_STRING));

  alarm (FRAME_INDICATOR_TIMEOUT);
}
