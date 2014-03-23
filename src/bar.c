/* Functionality for a bar listing the windows currently managed.
 *
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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef USE_XFT_FONT
#include <X11/Xft/Xft.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ratpoison.h"

/* Possible values for bar_is_raised status. */
#define BAR_IS_HIDDEN  0
#define BAR_IS_WINDOW_LIST 1
#define BAR_IS_GROUP_LIST  2
#define BAR_IS_MESSAGE     3

/* A copy of the last message displayed in the message bar. */
static char *last_msg = NULL;
static int last_mark_start = 0;
static int last_mark_end = 0;

static void marked_message_internal (char *msg, int mark_start, int mark_end);

/* Reset the alarm to auto-hide the bar in BAR_TIMEOUT seconds. */
static void
reset_alarm (void)
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

      /* Possibly restore colormap. */
      if (current_window())
	{
	  XUninstallColormap (dpy, s->def_cmap);
	  XInstallColormap (dpy, current_window()->colormap);
	}

      return 1;
    }

  return 0;
}

/* Show window listing in bar. */
int
show_bar (rp_screen *s, char *fmt)
{
  if (!s->bar_is_raised)
    {
      s->bar_is_raised = BAR_IS_WINDOW_LIST;
      XMapRaised (dpy, s->bar_window);
      update_window_names (s, fmt);

      /* Switch to the default colormap */
      if (current_window())
	XUninstallColormap (dpy, current_window()->colormap);
      XInstallColormap (dpy, s->def_cmap);

      reset_alarm();
      return 1;
    }

  /* If the bar is raised we still need to display the window
     names. */
  update_window_names (s, fmt);
  return 0;
}

/* Show group listing in bar. */
int
show_group_bar (rp_screen *s)
{
  if (!s->bar_is_raised)
    {
      s->bar_is_raised = BAR_IS_GROUP_LIST;
      XMapRaised (dpy, s->bar_window);
      update_group_names (s);

      /* Switch to the default colormap */
      if (current_window())
	XUninstallColormap (dpy, current_window()->colormap);
      XInstallColormap (dpy, s->def_cmap);

      reset_alarm();
      return 1;
    }

  /* If the bar is raised we still need to display the window
     names. */
  update_group_names (s);
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
      x = s->left + (defaults.bar_in_padding ? 0 : defaults.padding_left);
      break;
    case NorthGravity:
    case CenterGravity:
    case SouthGravity:
      x = s->left + (s->width - width - defaults.bar_border_width * 2) / 2
          - (defaults.bar_in_padding ? 0 : defaults.padding_left);
      break;
    case NorthEastGravity:
    case EastGravity:
    case SouthEastGravity:
      x = s->left + s->width - width - defaults.bar_border_width * 2
          - (defaults.bar_in_padding ? 0 : defaults.padding_right);
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
      y = s->top + (defaults.bar_in_padding ? 0 : defaults.padding_top);
      break;
    case EastGravity:
    case CenterGravity:
    case WestGravity:
      y = s->top + (s->height - height
           - defaults.bar_border_width * 2) / 2
	   - (defaults.bar_in_padding ? 0 : defaults.padding_top);
      break;
    case SouthEastGravity:
    case SouthGravity:
    case SouthWestGravity:
      y = s->top + (s->height - height
           - defaults.bar_border_width * 2)
 	   - (defaults.bar_in_padding ? 0 : defaults.padding_top);
      break;
    }

  return y;
}

void
update_bar (rp_screen *s)
{
  if (s->bar_is_raised == BAR_IS_WINDOW_LIST) {
    update_window_names (s, defaults.window_fmt);
    return;
  }

  if (s->bar_is_raised == BAR_IS_GROUP_LIST) {
    update_group_names (s);
    return;
  }

  if (s->bar_is_raised == BAR_IS_HIDDEN)
    return;

  redraw_last_message();
}

/* Note that we use marked_message_internal to avoid resetting the
   alarm. */
void
update_window_names (rp_screen *s, char *fmt)
{
  struct sbuf *bar_buffer;
  int mark_start = 0;
  int mark_end = 0;
  char *delimiter;

  if (s->bar_is_raised != BAR_IS_WINDOW_LIST) return;

  delimiter = (defaults.window_list_style == STYLE_ROW) ? " " : "\n";

  bar_buffer = sbuf_new (0);

  get_window_list (fmt, delimiter, bar_buffer, &mark_start, &mark_end);
  marked_message (sbuf_get (bar_buffer), mark_start, mark_end);

  sbuf_free (bar_buffer);
}

/* Note that we use marked_message_internal to avoid resetting the
   alarm. */
void
update_group_names (rp_screen *s)
{
  struct sbuf *bar_buffer;
  int mark_start = 0;
  int mark_end = 0;
  char *delimiter;

  if (s->bar_is_raised != BAR_IS_GROUP_LIST) return;

  delimiter = (defaults.window_list_style == STYLE_ROW) ? " " : "\n";

  bar_buffer = sbuf_new (0);

  get_group_list (delimiter, bar_buffer, &mark_start, &mark_end);
  marked_message_internal (sbuf_get (bar_buffer), mark_start, mark_end);

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
  rp_screen *s = current_screen ();
  size_t i;
  size_t start;
  int ret = 0;

  /* Count each line and keep the length of the longest one. */
  for(start=0, i=0; i <= strlen(msg); i++)
    {
      if(msg[i] == '\n' || msg[i] == '\0')
        {
          int current_width;

          /* Check if this line is the longest so far. */
          current_width = rp_text_width (s, msg + start, i - start);
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
draw_partial_string (rp_screen *s, char *msg, int len,
                     int x_offset, int y_offset, int style)
{
  rp_draw_string (s, s->bar_window, style,
                  defaults.bar_x_padding + x_offset,
                  defaults.bar_y_padding + FONT_ASCENT(s)
                  + y_offset * FONT_HEIGHT (s),
                  msg, len + 1);
}

#define REASON_NONE    0x00
#define REASON_STYLE   0x01
#define REASON_NEWLINE 0x02
static void
draw_string (rp_screen *s, char *msg, int mark_start, int mark_end)
{
  int i, start;
  int x_offset, y_offset;          /* Base coordinates where to print. */
  int print_reason = REASON_NONE;  /* Should we print something? */
  int style = STYLE_NORMAL, next_style = STYLE_NORMAL;
  int msg_len, part_len;

  start = 0;
  x_offset = y_offset = 0;
  msg_len = strlen (msg);

  /* Walk through the string, print each part. */
  for (i = 0; i < msg_len; ++i)
    {

      /* Should we ignore style hints? */
      if (mark_start != mark_end)
        {
          if (i == mark_start)
            {
              next_style = STYLE_INVERSE;
              if (i > start)
                print_reason |= REASON_STYLE;
            }
          else if (i == mark_end)
            {
              next_style = STYLE_NORMAL;
              if (i > start)
                print_reason |= REASON_STYLE;
            }
        }

      if (msg[i] == '\n')
          print_reason |= REASON_NEWLINE;

      if (print_reason != REASON_NONE)
        {
          /* Strip the trailing newline if necessary. */
          part_len = i - start - ((print_reason & REASON_NEWLINE) ? 1 : 0);

          draw_partial_string (s, msg + start, part_len,
                               x_offset, y_offset, style);

          /* Adjust coordinates. */
          if (print_reason & REASON_NEWLINE)
            {
              x_offset = 0;
              y_offset++;
              /* Skip newline. */
              start = i + 1;
            }
          else
            {
              x_offset += rp_text_width (s, msg + start, part_len);
              start = i;
            }

          print_reason = REASON_NONE;
        }
      style = next_style;
    }

  part_len = i - start - 1;

  /* Print the last line. */
  draw_partial_string (s, msg + start, part_len, x_offset, y_offset, style);

  XSync (dpy, False);
}
#undef REASON_NONE
#undef REASON_STYLE
#undef REASON_NEWLINE

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
  width = width < s->width ? width : s->width;
  height = height < s->height ? height : s->height;
  XMoveResizeWindow (dpy, s->bar_window,
                     bar_x (s, width), bar_y (s, height),
                     width, height);

  /* Map the bar if needed */
  if (!s->bar_is_raised)
    {
      s->bar_is_raised = BAR_IS_MESSAGE;
      XMapRaised (dpy, s->bar_window);

      /* Switch to the default colormap */
      if (current_window())
	XUninstallColormap (dpy, current_window()->colormap);
      XInstallColormap (dpy, s->def_cmap);
    }

  XRaiseWindow (dpy, s->bar_window);
  XClearWindow (dpy, s->bar_window);
  XSync (dpy, False);
}

static void
get_mark_box (char *msg, size_t mark_start, size_t mark_end,
              int *x, int *y, int *width, int *height)
{
  rp_screen *s = current_screen ();
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
    start = 0;
  else
    start = rp_text_width (s, &msg[start_line_beginning],
                           start_pos_in_line) + defaults.bar_x_padding;

  end = rp_text_width (s, &msg[end_line_beginning],
                       end_pos_in_line) + defaults.bar_x_padding * 2;

  if (mark_end != strlen (msg))
    end -= defaults.bar_x_padding;

  /* A little hack to highlight to the end of the line, if the
     mark_end is at the end of a line. */
  if (mark_end_is_new_line)
    {
      *width = max_line_length(msg) + defaults.bar_x_padding * 2;
    }
  else
    {
      *width = end - start;
    }

  *x = start;
  *y = (start_line - 1) * FONT_HEIGHT (s) + defaults.bar_y_padding;
  *height = (end_line - start_line + 1) * FONT_HEIGHT (s);
}

static void
draw_box (rp_screen *s, int x, int y, int width, int height)
{
  XGCValues lgv;
  GC lgc;
  unsigned long mask;

  lgv.foreground = s->fg_color;
  mask = GCForeground;
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
  draw_box (s, x, y, width, height);
}

static void
update_last_message (char *msg, int mark_start, int mark_end)
{
  free (last_msg);
  last_msg = xstrdup (msg);
  last_mark_start = mark_start;
  last_mark_end = mark_end;
}

void
marked_message (char *msg, int mark_start, int mark_end)
{
  /* Schedule the bar to be hidden after some amount of time. */
  reset_alarm ();
  marked_message_internal (msg, mark_start, mark_end);
}

static void
marked_message_internal (char *msg, int mark_start, int mark_end)
{
  rp_screen *s = current_screen ();
  int num_lines;
  int width;
  int height;

  PRINT_DEBUG (("msg = %s\n", msg?msg:"NULL"));
  PRINT_DEBUG (("mark_start = %d, mark_end = %d\n", mark_start, mark_end));

  /* Calculate the width and height of the window. */
  num_lines = count_lines (msg, strlen(msg));
  width = defaults.bar_x_padding * 2 + max_line_length(msg);
  height = FONT_HEIGHT (s) * num_lines + defaults.bar_y_padding * 2;

  prepare_bar (s, width, height);

  /* Draw the mark over the designated part of the string. */
  correct_mark (strlen (msg), &mark_start, &mark_end);
  draw_mark (s, msg, mark_start, mark_end);

  draw_string (s, msg, mark_start, mark_end);

  /* Keep a record of the message. */
  update_last_message (msg, mark_start, mark_end);
}

/* Use this just to update the bar. show_last_message will draw it and
   leave it up for a period of time. */
void
redraw_last_message (void)
{
  char *msg;

  if (last_msg == NULL) return;

  /* A little kludge to avoid last_msg in marked_message from being
     strdup'd right after freeing the pointer. Note: in this case
     marked_message's msg arg would have been the same as
     last_msg.  */
  msg = xstrdup (last_msg);
  marked_message_internal (msg, last_mark_start, last_mark_end);
  free (msg);
}

void
show_last_message (void)
{
  redraw_last_message();
  reset_alarm();
}

/* Free any memory associated with the bar. */
void
free_bar (void)
{
  free (last_msg);
  last_msg = NULL;
}
