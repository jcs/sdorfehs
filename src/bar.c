/* Functionality for a bar across the bottom of the screen listing the
 * windows currently managed. 
 *
 * Copyright (C) 2000, 2001, 2002, 2003 Shawn Betts
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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ratpoison.h"

#include "assert.h"

/* Possible values for bar_is_raised status. */
#define BAR_IS_HIDDEN  0
#define BAR_IS_WINDOW_LIST 1
#define BAR_IS_MESSAGE     2

/* A copy of the last message displayed in the message bar. */
static char *last_msg = NULL;
static int last_mark_start = 0;
static int last_mark_end = 0;

/* Reset the alarm to auto-hide the bar in BAR_TIMEOUT seconds. */
static void
reset_alarm ()
{
  alarm (defaults.bar_timeout);
  alarm_signalled = 0;
}

/* Hide the bar from sight. */
int
hide_bar (rp_screen *s)
{
  if (s->bar_is_raised)
    {
      s->bar_is_raised = 0;
      XUnmapWindow (dpy, s->bar_window);
      return 1;
    }

  return 0;
}

/* Show window listing in bar. */
int
show_bar (rp_screen *s)
{
  if (!s->bar_is_raised)
    {
      s->bar_is_raised = BAR_IS_WINDOW_LIST;
      XMapRaised (dpy, s->bar_window);
      update_window_names (s);
  
      reset_alarm();
      return 1;
    }

  /* If the bar is raised we still need to display the window
     names. */
  update_window_names (s);
  return 0;
}

int
bar_x (rp_screen *s, int width)
{
  int x = 0;

  switch (defaults.bar_location)
    {
    case NorthWestGravity:
    case WestGravity:
    case SouthWestGravity:
      x = s->left;
      break;
    case NorthGravity:
    case CenterGravity:
    case SouthGravity:
      x = s->left + (s->width - width - defaults.bar_border_width * 2) / 2;
      break;
    case NorthEastGravity:
    case EastGravity:
    case SouthEastGravity:
      x = s->left + s->width - width - defaults.bar_border_width * 2;
      break;
    }

  return x;
}

int
bar_y (rp_screen *s, int height)
{
  int y = 0;

  switch (defaults.bar_location)
    {
    case NorthEastGravity:
    case NorthGravity:
    case NorthWestGravity:
      y = s->top;
      break;
    case EastGravity:
    case CenterGravity:
    case WestGravity:
      y = s->top + (s->height - height 
	   - defaults.bar_border_width * 2) / 2;
      break;
    case SouthEastGravity:
    case SouthGravity:
    case SouthWestGravity:
      y = s->top + (s->height - height 
	   - defaults.bar_border_width * 2);
      break;
    }

  return y;
}

void
update_bar (rp_screen *s)
{
  if (s->bar_is_raised == BAR_IS_HIDDEN)
    return;

  if (s->bar_is_raised == BAR_IS_MESSAGE)
    {
      show_last_message();
    }
  else
    {
      /* bar is showing a window list. */
      update_window_names (s);
    }
}

void
update_window_names (rp_screen *s)
{
  struct sbuf *bar_buffer;
  int mark_start = 0;
  int mark_end = 0;

  if (s->bar_is_raised != BAR_IS_WINDOW_LIST) return;

  bar_buffer = sbuf_new (0);

  if(defaults.window_list_style == STYLE_ROW)
    {
      get_window_list (defaults.window_fmt, NULL, bar_buffer, &mark_start, &mark_end);
      marked_message (sbuf_get (bar_buffer), mark_start, mark_end);
    }
  else
    {
      get_window_list (defaults.window_fmt, "\n", bar_buffer, &mark_start, &mark_end);
      marked_message (sbuf_get (bar_buffer), mark_start, mark_end);
    }


/*   marked_message (sbuf_get (bar_buffer), mark_start, mark_end); */
  sbuf_free (bar_buffer);
}

void
message (char *s)
{
  marked_message (s, 0, 0);
}

void
marked_message_printf (int mark_start, int mark_end, char *fmt, ...)
{
  char *buffer;
  va_list ap;

  va_start (ap, fmt);
  buffer = xvsprintf (fmt, ap);
  va_end (ap);

  marked_message (buffer, mark_start, mark_end);
  free (buffer);
}

static int
count_lines (char* msg, int len)
{
  int ret = 1;
  int i;

  if (len < 1)
    return 1;

  for(i=0; i<len; i++) 
    {
      if (msg[i] == '\n') ret++;
    }

  return ret;
}


static int
max_line_length (char* msg)
{
  int i;
  int start;
  int ret = 0;
  
  /* Count each line and keep the length of the longest one. */
  for(start=0, i=0; i <= strlen(msg); i++) 
    {
      if(msg[i] == '\n' || msg[i] == '\0') 
	{
	  int current_width;

	  /* Check if this line is the longest so far. */
	  current_width = XTextWidth (defaults.font, msg + start, i - start);
	  if(current_width > ret)
	    {
	      ret = current_width;
	    }

	  /* Update the start of the new line. */
	  start = i + 1;
	}
    }

  return ret;
}

static int
pos_in_line (char* msg, int pos)
{
  int ret;
  int i;

  if(pos <= 0)
    return 0;

  /* Go backwards until we hit the beginning of the string or a new
     line. */
  ret = 0;
  for(i=pos-1; i>=0; ret++, i--)
    {
      if(msg[i]=='\n') 
	break;
    }

  return ret;
}

static int
line_beginning (char* msg, int pos)
{
  int ret = 0;
  int i;

  if(pos <= 0) 
    return 0;

  /* Go backwards until we hit a new line or the beginning of the
     string. */
  for(i=pos-1; i>=0; --i) 
    {
      if (msg[i]=='\n')
	{
	  ret = i + 1;
	  break;
	}
    }

  return ret;
}

static void
draw_string (rp_screen *s, char *msg)
{
  int i;
  int line_no;
  int start;
  int line_height = FONT_HEIGHT (defaults.font);

  /* Walk through the string, print each line. */
  start = 0;
  line_no = 0;
  for(i=0; i < strlen(msg); ++i) 
    {
      /* When we encounter a new line, print the text up to the new
	 line, and move down one line. */
      if (msg[i] == '\n')
	{
	  XDrawString (dpy, s->bar_window, s->normal_gc,
		       defaults.bar_x_padding,
		       defaults.bar_y_padding + defaults.font->max_bounds.ascent
		       +  line_no * line_height,
		       msg + start, i - start);
	  line_no++;
	  start = i + 1;
	}
    }
    
  /* Print the last line. */
  XDrawString (dpy, s->bar_window, s->normal_gc,
	       defaults.bar_x_padding,
	       defaults.bar_y_padding + defaults.font->max_bounds.ascent
	       +  line_no * line_height,
	       msg + start, strlen (msg) - start);

  XSync (dpy, False);
}

/* Move the marks if they are outside the string or if the start is
   after the end. */
static void
correct_mark (int msg_len, int *mark_start, int *mark_end)
{
  /* Make sure the marks are inside the string. */
  if (*mark_start < 0)
    *mark_start = 0;

  if (*mark_end < 0)
    *mark_end = 0;

  if (*mark_start > msg_len)
    *mark_start = msg_len;

  if (*mark_end > msg_len)
    *mark_end = msg_len;

  /* Make sure the marks aren't reversed. */
  if (*mark_start > *mark_end)
    {
      int tmp;
      tmp = *mark_start;
      *mark_start = *mark_end;
      *mark_end = tmp;
    }

}

/* Raise the bar and put it in the right spot */
static void
prepare_bar (rp_screen *s, int width, int height)
{
  XMoveResizeWindow (dpy, s->bar_window, 
		     bar_x (s, width), bar_y (s, height),
		     width, height);

  /* Map the bar if needed */
  if (!s->bar_is_raised)
    {
      s->bar_is_raised = BAR_IS_MESSAGE;
      XMapRaised (dpy, s->bar_window);
    }

  XRaiseWindow (dpy, s->bar_window);
  XClearWindow (dpy, s->bar_window);
  XSync (dpy, False);
}

static void
get_mark_box (char *msg, int mark_start, int mark_end, 
	      int *x, int *y, int *width, int *height)
{
  int start, end;
  int mark_end_is_new_line = 0;
  int start_line;
  int end_line;
  int start_pos_in_line;
  int end_pos_in_line;
  int start_line_beginning;
  int end_line_beginning;

  /* If the mark_end is on a new line or the end of the string, then
     back it up one character. */
  if (msg[mark_end-1] == '\n' || mark_end == strlen (msg))
    {
      mark_end--;
      mark_end_is_new_line = 1;
    }

  start_line = count_lines(msg, mark_start);
  end_line = count_lines(msg, mark_end);

  start_pos_in_line = pos_in_line(msg, mark_start);
  end_pos_in_line = pos_in_line(msg, mark_end);

  start_line_beginning = line_beginning(msg, mark_start);
  end_line_beginning = line_beginning(msg, mark_end);

  PRINT_DEBUG (("start_line = %d, end_line = %d\n", start_line, end_line));
  PRINT_DEBUG (("start_line_beginning = %d, end_line_beginning = %d\n", 
		start_line_beginning, end_line_beginning));

  if (mark_start == 0 || start_pos_in_line == 0)
    start = defaults.bar_x_padding;
  else
    start = XTextWidth (defaults.font, 
			&msg[start_line_beginning], 
			start_pos_in_line) + defaults.bar_x_padding;

  end = XTextWidth (defaults.font, 
		    &msg[end_line_beginning], 
		    end_pos_in_line) + defaults.bar_x_padding * 2;
      
  if (mark_end != strlen (msg))
    end -= defaults.bar_x_padding;

  /* A little hack to highlight to the end of the line, if the
     mark_end is at the end of a line. */
  if (mark_end_is_new_line)
    {
      *width = max_line_length(msg);
    }
  else
    {
      *width = end - start;
    }

  *x = start;
  *y = (start_line - 1) * FONT_HEIGHT (defaults.font) + defaults.bar_y_padding;
  *height = (end_line - start_line + 1) * FONT_HEIGHT (defaults.font);
}

static void
draw_inverse_box (rp_screen *s, int x, int y, int width, int height)
{
  XGCValues lgv;
  GC lgc;
  unsigned long mask;

  lgv.foreground = current_screen()->fg_color;
  lgv.function = GXxor;
  mask = GCForeground | GCFunction;
  lgc = XCreateGC(dpy, s->root, mask, &lgv);

  XFillRectangle (dpy, s->bar_window, lgc, 
		  x, y, width, height);
  XFreeGC (dpy, lgc);

  lgv.foreground = s->bg_color;
  lgc = XCreateGC(dpy, s->root, mask, &lgv);

  XFillRectangle (dpy, s->bar_window, lgc, 
		  x, y, width, height);
  XFreeGC (dpy, lgc);
}

static void
draw_mark (rp_screen *s, char *msg, int mark_start, int mark_end)
{
  int x, y, width, height;

  /* when this happens, there is no mark. */
  if (mark_end == 0 || mark_start == mark_end)
    return;

  get_mark_box (msg, mark_start, mark_end, 
		&x, &y, &width, &height);
  draw_inverse_box (s, x, y, width, height);
}

static void
update_last_message (char *msg, int mark_start, int mark_end)
{
  if (last_msg)
    free (last_msg);
  last_msg = xstrdup (msg);
  last_mark_start = mark_start;
  last_mark_end = mark_end;
}

void
marked_message (char *msg, int mark_start, int mark_end)
{
  rp_screen *s = current_screen ();
  int num_lines;
  int width;
  int height;

  PRINT_DEBUG (("msg = %s\n", msg));
  PRINT_DEBUG (("mark_start = %d, mark_end = %d\n", mark_start, mark_end));

  /* Schedule the bar to be hidden after some amount of time. */
  reset_alarm ();

  /* Calculate the width and height of the window. */
  num_lines = count_lines (msg, strlen(msg));
  width = defaults.bar_x_padding * 2 + max_line_length(msg);
  height = FONT_HEIGHT (defaults.font) * num_lines + defaults.bar_y_padding * 2;

  /* Display the string. */
  prepare_bar (s, width, height);
  draw_string (s, msg);

  /* Draw the mark over the designated part of the string. */
  correct_mark (strlen (msg), &mark_start, &mark_end);
  draw_mark (s, msg, mark_start, mark_end);

  /* Keep a record of the message. */
  update_last_message (msg, mark_start, mark_end);
}

void
show_last_message ()
{
  char *msg;

  if (last_msg == NULL) return;

  /* A little kludge to avoid last_msg in marked_message from being
     strdup'd right after freeing the pointer. Note: in this case
     marked_message's msg arg would have been the same as
     last_msg.  */
  msg = xstrdup (last_msg);
  marked_message (msg, last_mark_start, last_mark_end);
  free (msg);
}

/* Free any memory associated with the bar. */
void
free_bar ()
{
  if (last_msg)
    free (last_msg);
}
