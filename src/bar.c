/* Functionality for a bar across the bottom of the screen listing the
 * windows currently managed. 
 *
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
#define BAR_IS_WINDOW_LIST 1
#define BAR_IS_MESSAGE     2

/* A copy of the last message displayed in the message bar. */
static char *last_msg = NULL;
static int last_mark_start = 0;
static int last_mark_end = 0;

/* Hide the bar from sight. */
int
hide_bar (screen_info *s)
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
show_bar (screen_info *s)
{
  if (!s->bar_is_raised)
    {
      s->bar_is_raised = BAR_IS_WINDOW_LIST;
      XMapRaised (dpy, s->bar_window);
      update_window_names (s);
  
      /* Set an alarm to auto-hide the bar BAR_TIMEOUT seconds later */
      alarm (defaults.bar_timeout);
      alarm_signalled = 0;
      return 1;
    }

  /* If the bar is raised we still need to display the window
     names. */
  update_window_names (s);
  return 0;
}

int
bar_x (screen_info *s, int width)
{
  if (defaults.bar_location == SouthEastGravity
      || defaults.bar_location == NorthEastGravity)
    return s->root_attr.width - width - defaults.bar_border_width * 2;
  else
    return 0;
}

int
bar_y (screen_info *s)
{
  if (defaults.bar_location == NorthWestGravity
      || defaults.bar_location == NorthEastGravity ) 
    return 0;
  else
    return s->root_attr.height - (FONT_HEIGHT (defaults.font) + defaults.bar_y_padding * 2) - defaults.bar_border_width * 2;
}

void
update_window_names (screen_info *s)
{
  static struct sbuf *bar_buffer = NULL;

  int mark_start = 0;
  int mark_end = 0;

  if (s->bar_is_raised != BAR_IS_WINDOW_LIST) return;

  if (bar_buffer == NULL)
    bar_buffer = sbuf_new (0);

  get_window_list (defaults.window_fmt, NULL, bar_buffer, &mark_start, &mark_end);

  marked_message (sbuf_get (bar_buffer), mark_start, mark_end);
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

void
marked_message (char *msg, int mark_start, int mark_end)
{
  XGCValues lgv;
  GC lgc;
  unsigned long mask;
  screen_info *s = current_screen ();

  int width = defaults.bar_x_padding * 2 + XTextWidth (defaults.font, msg, strlen (msg));
  int height = (FONT_HEIGHT (defaults.font) + defaults.bar_y_padding * 2);

  PRINT_DEBUG ("%s\n", msg);

  /* Map the bar if needed */
  if (!s->bar_is_raised)
    {
      s->bar_is_raised = BAR_IS_MESSAGE;
      XMapRaised (dpy, s->bar_window);
    }

  /* Reset the alarm to auto-hide the bar in BAR_TIMEOUT seconds. */
  alarm (defaults.bar_timeout);
  alarm_signalled = 0;

  XMoveResizeWindow (dpy, s->bar_window, 
		     bar_x (s, width), bar_y (s),
		     width,
		     height);

  XRaiseWindow (dpy, s->bar_window);
  XClearWindow (dpy, s->bar_window);
  XSync (dpy, False);
  XDrawString (dpy, s->bar_window, s->normal_gc, 
	       defaults.bar_x_padding, 
	       defaults.bar_y_padding + defaults.font->max_bounds.ascent,
	       msg, strlen (msg));
  XSync (dpy, False);
  
  /* Crop to boundary conditions. */
  if (mark_start < 0)
    mark_start = 0;

  if (mark_end < 0)
    mark_end = 0;

  if (mark_start > strlen (msg))
    mark_start = strlen (msg);

  if (mark_end > strlen (msg))
    mark_end = strlen (msg);

  if (mark_start > mark_end)
    {
      int tmp;
      tmp = mark_start;
      mark_start = mark_end;
      mark_end = tmp;
    }

  /* xor the string representing the current window */
  if (mark_start != mark_end)
    {
      int start;
      int end;

      start = XTextWidth (defaults.font, msg, mark_start) + defaults.bar_x_padding;
      end = XTextWidth (defaults.font, msg + mark_start, mark_end - mark_start) + defaults.bar_x_padding;

      PRINT_DEBUG ("%d %d strlen(%d)==> %d %d\n", mark_start, mark_end, strlen(msg), start, end);

      lgv.foreground = current_screen()->fg_color;
      lgv.function = GXxor;
      mask = GCForeground | GCFunction;
      lgc = XCreateGC(dpy, s->root, mask, &lgv);

      XFillRectangle (dpy, s->bar_window, lgc, start, 0, end, height);

      lgv.foreground = s->bg_color;
      lgc = XCreateGC(dpy, s->root, mask, &lgv);

      XFillRectangle (dpy, s->bar_window, lgc, start, 0, end, height);
    }

  /* Keep a record of the message. */
  if (last_msg)
    free (last_msg);
  last_msg = xstrdup (msg);
  last_mark_start = mark_start;
  last_mark_end = mark_end;
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
