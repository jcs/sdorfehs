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
#include <string.h>

#include "ratpoison.h"

#define VERTICALLY 0
#define HORIZONTALLY 1

static void
update_last_access (rp_window_frame *frame)
{
  static int counter = 0;

  frame->last_access = counter;
  counter++;
}

int
num_frames (screen_info *s)
{
 int count = 0;
 rp_window_frame *cur;

 list_for_each_entry (cur, &s->rp_window_frames, node)
   {
     count++;
   }

 return count;
}

rp_window *
set_frames_window (rp_window_frame *frame, rp_window *win)
{
  rp_window *last_win;

  last_win = frame->win;
  frame->win = win;
  if (win)
    win->frame = frame;

  return last_win;
}

static screen_info *
frames_screen (rp_window_frame *frame)
{
  int i;
  rp_window_frame *cur;

  for (i=0; i<num_screens; i++)
    list_for_each_entry (cur, &screens[i].rp_window_frames, node)
      {
	if (frame == cur)
	  return &screens[i];
      }

  /* This SHOULD be impossible to get to. FIXME: It'll crash higher up if we
     return NULL. */
  return NULL;
}

void
maximize_all_windows_in_frame (rp_window_frame *frame)
{
  rp_window *win;

  list_for_each_entry (win, &rp_mapped_window, node)
    {
      if (win->frame == frame)
	{
	  maximize (win);
	}
    }
}

/* Make the frame occupy the entire screen */ 
static void
maximize_frame (rp_window_frame *frame)
{
  screen_info *s = frames_screen (frame);

  frame->x = defaults.padding_left;
  frame->y = defaults.padding_top;

  frame->width = DisplayWidth (dpy, s->screen_num) - defaults.padding_right - defaults.padding_left;
  frame->height = DisplayHeight (dpy, s->screen_num) - defaults.padding_bottom - defaults.padding_top;
}

/* Create a full screen frame */
static void
create_initial_frame (screen_info *screen)
{
  screen->rp_current_frame = xmalloc (sizeof (rp_window_frame));

  list_add_tail (&screen->rp_current_frame->node, &screen->rp_window_frames);

  update_last_access (screen->rp_current_frame);
  maximize_frame (screen->rp_current_frame);
  set_frames_window (screen->rp_current_frame, NULL);
}

void
init_frame_lists ()
{
  int i;

  for (i=0; i<num_screens; i++)
    init_frame_list (&screens[i]);
}

void
init_frame_list (screen_info *screen)
{
  INIT_LIST_HEAD (&screen->rp_window_frames);

  create_initial_frame(screen);
}

rp_window_frame *
find_last_frame (screen_info *s)
{
  rp_window_frame *cur, *last = NULL;
  int last_access = -1;

  list_for_each_entry (cur, &s->rp_window_frames, node)
    {
      if (cur != s->rp_current_frame
	  && cur->last_access > last_access)
	{
	  last_access = cur->last_access;
	  last = cur;
	}
    }

  return last;
}

/* Return the frame that contains the window. */
rp_window_frame *
find_windows_frame (rp_window *win)
{
  screen_info *s;
  rp_window_frame *cur;

  s = win->scr;

  list_for_each_entry (cur, &s->rp_window_frames, node)
    {
      if (cur->win == win) return cur;
    }

  return NULL;
}

rp_window_frame *
find_frame_next (rp_window_frame *frame)
{
  if (frame == NULL) return NULL;
  return list_next_entry (frame, &frames_screen (frame)->rp_window_frames, node);
}

rp_window_frame *
find_frame_prev (rp_window_frame *frame)
{
  if (frame == NULL) return NULL;
  return list_prev_entry (frame, &frames_screen (frame)->rp_window_frames, node);
}

rp_window *
current_window ()
{
  return screens[rp_current_screen].rp_current_frame->win;
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
  screen_info *s = frames_screen (frame);
  int last_access = 0;
  rp_window *most_recent = NULL;
  rp_window *cur;

  list_for_each_entry (cur, &rp_mapped_window, node)
    {
      if (cur->scr == s
	  && cur != current_window() 
	  && !find_windows_frame (cur)
	  && cur->last_access >= last_access
	  && window_fits_in_frame (cur, frame)
	  && !cur->frame)
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
split_frame (rp_window_frame *frame, int way, int pixels)
{
  screen_info *s;
  rp_window *win;
  rp_window_frame *new_frame;

  s = frames_screen (frame);

  new_frame = xmalloc (sizeof (rp_window_frame));

  /* It seems intuitive to make the last frame the newly created
     frame. */
  update_last_access (new_frame);

  /* TODO: don't put the new frame at the end of the list, put it
     after the existing frame. Then cycling frames cycles in the order
     they were created. */
  list_add_tail (&new_frame->node, &s->rp_window_frames);

  set_frames_window (new_frame, NULL);

  if (way == HORIZONTALLY)
    {
      new_frame->x = frame->x;
      new_frame->y = frame->y + pixels;
      new_frame->width = frame->width;
      new_frame->height = frame->height - pixels;

      frame->height = pixels;
    }
  else
    {
      new_frame->x = frame->x + pixels;
      new_frame->y = frame->y;
      new_frame->width = frame->width - pixels;
      new_frame->height = frame->height;

      frame->width = pixels;
    }

  win = find_window_for_frame (new_frame);
  if (win)
    {
      PRINT_DEBUG (("Found a window for the frame!\n"));

      set_frames_window (new_frame, win);

      maximize (win);
      unhide_window (win);
      XRaiseWindow (dpy, win->w);
    }
  else
    {
      PRINT_DEBUG (("No window fits the frame.\n"));

      set_frames_window (new_frame, NULL);
    }

  /* resize the existing frame */
  if (frame->win)
    {
      maximize_all_windows_in_frame (frame);
      XRaiseWindow (dpy, frame->win->w);
    }

  show_frame_indicator();
}

/* Splits the window vertically leaving the original with 'pixels'
   pixels . */
void
v_split_frame (rp_window_frame *frame, int pixels)
{
  split_frame (frame, VERTICALLY, pixels);
}

/* Splits the frame horizontally leaving the original with 'pixels'
   pixels . */
void
h_split_frame (rp_window_frame *frame, int pixels)
{
  split_frame (frame, HORIZONTALLY, pixels);
}

void
remove_all_splits ()
{
  struct list_head *tmp, *iter;
  screen_info *s = &screens[rp_current_screen];
  rp_window_frame *frame = NULL;
  rp_window *win = NULL;

  /* Hide all the windows not in the current frame. */
  list_for_each_entry (win, &rp_mapped_window, node)
    {
      if (win->frame == frame)
	hide_window (win);
    }

  /* Delete all the frames except the current one. */
  list_for_each_safe_entry (frame, iter, tmp, &s->rp_window_frames, node)
    {
      if (frame != s->rp_current_frame)
	{
	  list_del (&frame->node);
	  free (frame);
	}
    }

  /* Maximize the frame and the windows in the frame. */
  maximize_frame (s->rp_current_frame);
  maximize_all_windows_in_frame (s->rp_current_frame);
}

/* Shrink the size of the frame to fit it's current window. */
void
resize_shrink_to_window (rp_window_frame *frame)
{
  if (frame->win == NULL) return;

  resize_frame_horizontally (frame, frame->win->width + frame->win->border*2 - frame->width);
  resize_frame_vertically (frame, frame->win->height + frame->win->border*2 - frame->height);
}

/* Resize FRAME vertically by diff pixels. if diff is negative
   then shrink the frame, otherwise enlarge it. */
void
resize_frame_vertically (rp_window_frame *frame, int diff)
{
  screen_info *s = frames_screen (frame);
  int found_adjacent_frames = 0;
  int max_bound, min_bound;
  int orig_y;
  rp_window_frame *cur;

  if (num_frames (s) < 2 || diff == 0)
    return;

  max_bound = frame->x + frame->width;
  min_bound = frame->x;
  orig_y = frame->y;

  /* Look for frames below that needs to be resized.  */
  list_for_each_entry (cur, &s->rp_window_frames, node)
    {
      if (cur->y == (frame->y + frame->height)
      	  && (cur->x + cur->width) > frame->x
      	  && cur->x < (frame->x + frame->width))
	{
	  cur->height -= diff;
	  cur->y += diff;

	  if ((cur->x + cur->width) > max_bound)
	    max_bound = cur->x + cur->width;

	  if (cur->x < min_bound)
	    min_bound = cur->x;

	  if (cur->win)
	    maximize_all_windows_in_frame (cur);

	  found_adjacent_frames = 1;
	}
    }

  /* Found no frames below, look for some above.  */
  if (!found_adjacent_frames)
    {
      list_for_each_entry (cur, &s->rp_window_frames, node)
	{
	    if (cur->y == (frame->y - cur->height)
		&& (cur->x + cur->width) > frame->x
		&& cur->x < (frame->x + frame->width))
	    {
	      cur->height -= diff;

	      if ((cur->x + cur->width) > max_bound)
		max_bound = cur->x + cur->width;

	      if (cur->x < min_bound)
		min_bound = cur->x;

	      if (cur->win)
		maximize_all_windows_in_frame (cur);

	      found_adjacent_frames = 1;
	    }
	}

      /* If we found any frames, move the current frame.  */
      if (found_adjacent_frames)
	{
	  frame->y -= diff;
	}
    }

  /* Resize current frame.  */
  if (found_adjacent_frames)
    {
      frame->height += diff;

      /* If we left any gaps, take care of them too.  */
      list_for_each_entry (cur, &s->rp_window_frames, node)
	{
	  if (cur->y == orig_y && cur->x >= min_bound
	      && cur->x + cur->width <= max_bound
	      && cur != frame)
	    {
	      cur->height = frame->height;
	      cur->y = frame->y;

	      if (cur->win)
		maximize_all_windows_in_frame (cur);
	    }
	}

      if (frame->win)
	{
	  maximize_all_windows_in_frame (frame);
	  XRaiseWindow (dpy, frame->win->w);
	}
    }
}

/* Resize FRAME horizontally by diff pixels. if diff is negative
   then shrink the frame, otherwise enlarge it. */
void
resize_frame_horizontally (rp_window_frame *frame, int diff)
{
  screen_info *s = frames_screen (frame);
  int found_adjacent_frames = 0;
  int max_bound, min_bound;
  int orig_x;
  rp_window_frame *cur;

  if (num_frames (s) < 2 || diff == 0)
    return;

  max_bound = frame->y + frame->height;
  min_bound = frame->y;
  orig_x = frame->x;

  /* Look for frames on the right that needs to be resized.  */
  list_for_each_entry (cur, &s->rp_window_frames, node)
    {
      if (cur->x == (frame->x + frame->width)
      	  && (cur->y + cur->height) > frame->y
      	  && cur->y < (frame->y + frame->height))
	{
	  cur->width -= diff;
	  cur->x += diff;

	  if ((cur->y + cur->height) > max_bound)
	    max_bound = cur->y + cur->height;

	  if (cur->y < min_bound)
	    min_bound = cur->y;

	  if (cur->win)
	    maximize_all_windows_in_frame (cur);

	  found_adjacent_frames = 1;
	}
    }

  /* Found no frames to the right, look for some to the left.  */
  if (!found_adjacent_frames)
    {
      list_for_each_entry (cur, &s->rp_window_frames, node)
	{
	  if (cur->x == (frame->x - cur->width)
	      && (cur->y + cur->height) > frame->y
	      && cur->y < (frame->y + frame->height))
	    {
	      cur->width -= diff;

	      if ((cur->y + cur->height) > max_bound)
		max_bound = cur->y + cur->height;

	      if (cur->y < min_bound)
		min_bound = cur->y;

	      if (cur->win)
		maximize_all_windows_in_frame (cur);

	      found_adjacent_frames = 1;
	    }
	}

      /* If we found any frames, move the current frame.  */
      if (found_adjacent_frames)
	{
	    frame->x -= diff;
	}
    }

  /* Resize current frame.  */
  if (found_adjacent_frames)
    {
      frame->width += diff;

      /* If we left any gaps, take care of them too.  */
      list_for_each_entry (cur, &s->rp_window_frames, node)
	{
	  if (cur->x == orig_x && cur->y >= min_bound
	      && cur->y + cur->height <= max_bound
	      && cur != frame)
	    {
	      cur->width = frame->width;
	      cur->x = frame->x;

	      if (cur->win)
		maximize_all_windows_in_frame (cur);
	    }
	}

      if (frame->win)
	{
	  maximize_all_windows_in_frame (frame);
	  XRaiseWindow (dpy, frame->win->w);
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
total_frame_area (screen_info *s)
{
  int area = 0;
  rp_window_frame *cur;

  list_for_each_entry (cur, &s->rp_window_frames, node)
    {
      area += cur->width * cur->height;
    }

  return area;
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
frame_overlaps (rp_window_frame *frame)
{
  screen_info *s;
  rp_window_frame *cur;

  s = frames_screen (frame);

  list_for_each_entry (cur, &s->rp_window_frames, node)
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
  screen_info *s;
  int area;
  rp_window_frame *cur;

  if (frame == NULL) return;

  s = frames_screen (frame);

  area = total_frame_area(s);
  PRINT_DEBUG (("Total Area: %d\n", area));

  list_del (&frame->node);
  hide_window (frame->win);
  hide_others (frame->win);

  list_for_each_entry (cur, &s->rp_window_frames, node)
    {
      rp_window_frame tmp_frame;
      int fits = 0;

      if (cur->win)
	{
	  PRINT_DEBUG (("Trying frame containing window '%s'\n", window_name (cur->win)));
	}
      else
	{
	  PRINT_DEBUG (("Trying some empty frame\n"));
	}

      /* Backup the frame */
      memcpy (&tmp_frame, cur, sizeof (rp_window_frame));

      if (frame_is_below (frame, cur)
	  || frame_is_above (frame, cur))
	{
	  if (frame_is_below (frame, cur))
	    cur->y = frame->y;
	  cur->height += frame->height;
	}

      PRINT_DEBUG (("Attempting vertical Frame y=%d height=%d\n", cur->y, cur->height));
      PRINT_DEBUG (("New Total Area: %d\n", total_frame_area(s)));

      /* If the area is bigger than before, the frame takes up too
	 much space. If the current frame and the deleted frame DON'T
	 overlap then the current window took up just the right amount
	 of space but didn't take up the space left behind by the
	 deleted window. If any active frames overlap, it could have
	 taken up the right amount of space, overlaps with the deleted
	 frame but obviously didn't fit. */
      if (total_frame_area(s) > area || !frames_overlap (cur, frame) || frame_overlaps (cur))
	{
	  PRINT_DEBUG (("Didn't fit vertically\n"));

	  /* Restore the current window's frame */
	  memcpy (cur, &tmp_frame, sizeof (rp_window_frame));
	}
      else
	{
	  PRINT_DEBUG (("It fit vertically!!\n"));

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

      PRINT_DEBUG (("Attempting horizontal Frame x=%d width=%d\n", cur->x, cur->width));
      PRINT_DEBUG (("New Total Area: %d\n", total_frame_area(s)));

      /* Same test as the vertical test, above. */
      if (total_frame_area(s) > area || !frames_overlap (cur, frame) || frame_overlaps (cur))
	{
	  PRINT_DEBUG (("Didn't fit horizontally\n"));

	  /* Restore the current window's frame */
	  memcpy (cur, &tmp_frame, sizeof (rp_window_frame));
	}
      else
	{
	  PRINT_DEBUG (("It fit horizontally!!\n"));
	  fits = 1;
	}

      if (fits)
	{
	  /* The current frame fits into the new space so keep its
	     new frame parameters and maximize the window to fit
	     the new frame size. */
	  if (cur->win)
	    {
	      maximize_all_windows_in_frame (cur);
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

/* Switch the input focus to another frame, and therefore a different
   window. */
void
set_active_frame (rp_window_frame *frame)
{
  screen_info *old_s = current_screen();
  screen_info *s = frames_screen (frame);
  rp_window_frame *old = current_screen()->rp_current_frame;

  /* Make the switch */
  give_window_focus (frame->win, old->win);
  update_last_access (frame);
  s->rp_current_frame = frame;

  /* If frame->win == NULL, then rp_current_screen is not updated. */
  rp_current_screen = s->screen_num;

  /* Possibly show the frame indicator. */
  if ((old != s->rp_current_frame && num_frames(s) > 1) 
      || s != old_s)
    {
      show_frame_indicator();
    }

  /* If the frame has no window to give focus to, give the key window
     focus. */
  if( !frame->win )
    {
      XSetInputFocus (dpy, s->key_window, 
		      RevertToPointerRoot, CurrentTime);
    }  
}

void
blank_frame (rp_window_frame *frame)
{
  if (frame->win == NULL) return;
  
  hide_window (frame->win);
  hide_others (frame->win);

  set_frames_window (frame, NULL);

  /* Give the key window focus. */
  XSetInputFocus (dpy, current_screen()->key_window, 
		  RevertToPointerRoot, CurrentTime);
}

void
hide_frame_indicator ()
{
  XUnmapWindow (dpy, current_screen()->frame_window);
}

void
show_frame_indicator ()
{
  screen_info *s = current_screen ();
  int width, height;

  width = defaults.bar_x_padding * 2 + XTextWidth (defaults.font, MESSAGE_FRAME_STRING, strlen (MESSAGE_FRAME_STRING));
  height = (FONT_HEIGHT (defaults.font) + defaults.bar_y_padding * 2);

  XMoveResizeWindow (dpy, current_screen()->frame_window,
		     current_screen()->rp_current_frame->x
		     + current_screen()->rp_current_frame->width / 2 - width / 2, 
		     current_screen()->rp_current_frame->y
		     + current_screen()->rp_current_frame->height / 2 - height / 2,
		     width, height);

  XMapRaised (dpy, current_screen()->frame_window);
  XClearWindow (dpy, s->frame_window);
  XSync (dpy, False);

  XDrawString (dpy, s->frame_window, s->normal_gc, 
	       defaults.bar_x_padding, 
	       defaults.bar_y_padding + defaults.font->max_bounds.ascent,
	       MESSAGE_FRAME_STRING, strlen (MESSAGE_FRAME_STRING));

  alarm (defaults.frame_indicator_timeout);
}

rp_window_frame *
find_frame_up (rp_window_frame *frame)
{
  screen_info *s = frames_screen (frame);
  rp_window_frame *cur;

  list_for_each_entry (cur, &s->rp_window_frames, node)
    {
      if (frame->y == cur->y + cur->height)
	{
	  if (frame->x >= cur->x && frame->x < cur->x + cur->width)
	    return cur;
	}
    }

  return NULL;
}

rp_window_frame *
find_frame_down (rp_window_frame *frame)
{
  screen_info *s = frames_screen (frame);
  rp_window_frame *cur;

  list_for_each_entry (cur, &s->rp_window_frames, node)
    {
      if (frame->y + frame->height == cur->y)
	{
	  if (frame->x >= cur->x && frame->x < cur->x + cur->width)
	    return cur;
	}
    }

  return NULL;
}

rp_window_frame *
find_frame_left (rp_window_frame *frame)
{
  screen_info *s = frames_screen (frame);
  rp_window_frame *cur;

  list_for_each_entry (cur, &s->rp_window_frames, node)
    {
      if (frame->x == cur->x + cur->width)
	{
	  if (frame->y >= cur->y && frame->y < cur->y + cur->height)
	    return cur;
	}
    }

  return NULL;
}

rp_window_frame *
find_frame_right (rp_window_frame *frame)
{
  screen_info *s = frames_screen (frame);
  rp_window_frame *cur;

  list_for_each_entry (cur, &s->rp_window_frames, node)
    {
      if (frame->x + frame->width == cur->x)
	{
	  if (frame->y >= cur->y && frame->y < cur->y + cur->height)
	    return cur;
	}
    }

  return NULL;
}
