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
      XMapWindow (dpy, s->bar_window);
      update_window_names (s);
  
      /* Set an alarm to auto-hide the bar BAR_TIMEOUT seconds later */
      alarm (BAR_TIMEOUT);
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
  if (BAR_LOCATION >= 2) return s->root_attr.width - width - 2;
  else return 0;
}

int
bar_y (screen_info *s)
{
  if (BAR_LOCATION % 2) return 0;
  else return s->root_attr.height - (FONT_HEIGHT (s->font) + BAR_Y_PADDING * 2) - 2;
}

void
update_window_names (screen_info *s)
{
  static struct sbuf *bar_buffer = NULL;
  rp_window *w;
  rp_window *other_window;

  char dbuf[10];

  int mark_start = 0;
  int mark_end = 0;

  if (!s->bar_is_raised) return;

  if (bar_buffer == NULL)
    bar_buffer = sbuf_new (0);

  sbuf_clear (bar_buffer);

  other_window = find_window_other ();

  for (w = rp_mapped_window_sentinel->next; 
       w != rp_mapped_window_sentinel; 
       w = w->next)
    {
      PRINT_DEBUG ("%d-%s\n", w->number, w->name);

      if (w == rp_current_window)
	mark_start = strlen (sbuf_get (bar_buffer));

      sbuf_concat (bar_buffer, " ");

      sprintf (dbuf, "%d", w->number);
      sbuf_concat (bar_buffer, dbuf);

      if (w == rp_current_window)
	sbuf_concat (bar_buffer, "*");
      else if (w == other_window)
	sbuf_concat (bar_buffer, "+");
      else
	sbuf_concat (bar_buffer, "-");

      sbuf_concat (bar_buffer, w->name);

      sbuf_concat (bar_buffer, " ");

      if (w == rp_current_window)
	mark_end = strlen (sbuf_get (bar_buffer));
    }

  if (!strcmp (sbuf_get (bar_buffer), ""))
    {
      sbuf_copy (bar_buffer, MESSAGE_NO_MANAGED_WINDOWS);
    }

  marked_message (sbuf_get (bar_buffer), mark_start, mark_end);
}

void
marked_message (char *msg, int mark_start, int mark_end)
{
  XGCValues lgv;
  GC lgc;
  unsigned long mask;
  screen_info *s = current_screen ();

  int width = BAR_X_PADDING * 2 + XTextWidth (s->font, msg, strlen (msg));
  int height = (FONT_HEIGHT (s->font) + BAR_Y_PADDING * 2);

  PRINT_DEBUG ("%s\n", msg);

  /* Map the bar if needed */
  if (!s->bar_is_raised)
    {
      s->bar_is_raised = BAR_IS_MESSAGE;
      XMapWindow (dpy, s->bar_window);
    }

  /* Reset the alarm to auto-hide the bar in BAR_TIMEOUT seconds. */
  alarm (BAR_TIMEOUT);

  XMoveResizeWindow (dpy, s->bar_window, 
		     bar_x (s, width), bar_y (s),
		     width,
		     height);

  XClearWindow (dpy, s->bar_window);
  XRaiseWindow (dpy, s->bar_window);

  XDrawString (dpy, s->bar_window, s->normal_gc, 
	       BAR_X_PADDING, 
	       BAR_Y_PADDING + s->font->max_bounds.ascent,
	       msg, strlen (msg));

  /* xor the string representing the current window */
  if (mark_start != mark_end)
    {
      int start;
      int end;

      assert (mark_start <= mark_end); /* FIXME: remove these assertions,
				      make it work in all cases */
      assert (mark_start <= strlen(msg) + 1);
      assert (mark_end <= strlen(msg) + 1);
      
      start = XTextWidth (s->font, msg, mark_start) + BAR_X_PADDING;
      end = XTextWidth (s->font, msg + mark_start, mark_end - mark_start);

      PRINT_DEBUG ("%d %d strlen(%d)==> %d %d\n", mark_start, mark_end, strlen(msg), start, end);

      lgv.foreground = gv.foreground;
      lgv.function = GXxor;
      mask = GCForeground | GCFunction;
      lgc = XCreateGC(dpy, s->root, mask, &lgv);

      XFillRectangle (dpy, s->bar_window, lgc, start, 0, end, height);

      lgv.foreground = gv.background;
      lgc = XCreateGC(dpy, s->root, mask, &lgv);

      XFillRectangle (dpy, s->bar_window, lgc, start, 0, end, height);
    }
}
