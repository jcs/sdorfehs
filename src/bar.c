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
  int x = 0;

  switch (defaults.bar_location)
    {
    case NorthWestGravity:
    case WestGravity:
    case SouthWestGravity:
      x = 0;
      break;
    case NorthGravity:
    case CenterGravity:
    case SouthGravity:
      x = (s->root_attr.width - width - defaults.bar_border_width * 2) / 2;
      break;
    case NorthEastGravity:
    case EastGravity:
    case SouthEastGravity:
      x = s->root_attr.width - width - defaults.bar_border_width * 2;
      break;
    }

  return x;
}

int
bar_y (screen_info *s, int height)
{
  int y = 0;

  switch (defaults.bar_location)
    {
    case NorthEastGravity:
    case NorthGravity:
    case NorthWestGravity:
      y = 0;
      break;
    case EastGravity:
    case CenterGravity:
    case WestGravity:
      y = (s->root_attr.height - height 
	   - defaults.bar_border_width * 2) / 2;
      break;
    case SouthEastGravity:
    case SouthGravity:
    case SouthWestGravity:
      y = (s->root_attr.height - height 
	   - defaults.bar_border_width * 2);
      break;
    }

  return y;
}

void
update_window_names (screen_info *s)
{
  struct sbuf *bar_buffer;
  int mark_start = 0;
  int mark_end = 0;

  if (s->bar_is_raised != BAR_IS_WINDOW_LIST) return;

  bar_buffer = sbuf_new (0);

  if(!defaults.wrap_window_list)
    {
      get_window_list (defaults.window_fmt, NULL, bar_buffer, &mark_start, &mark_end);
      marked_message (sbuf_get (bar_buffer), mark_start, mark_end);
    }
  else
    {
      get_window_list (defaults.window_fmt, "\n", bar_buffer, &mark_start, &mark_end);
      marked_wrapped_message (sbuf_get (bar_buffer), mark_start, mark_end);
    }


/*   marked_message (sbuf_get (bar_buffer), mark_start, mark_end); */
  sbuf_free (bar_buffer);
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

int
count_lines (char* msg, int len)
{
  int ret=1;
  int i=0;
  for(; i<len; ++i) {
    if(msg[i]=='\n') ret++;
  }
  PRINT_DEBUG(("count_lines: %d\n", ret));
  return ret;
}


int
max_line_length (char* msg)
{
  int ret=0;
  int i=0;
  int j=0;
  int len=strlen(msg);
  int current_width=0;
  char tmp_buf[100];
  
  for(; i<=len; ++j, ++i) {
    if(msg[i]=='\n' || msg[i]=='\0') {
      tmp_buf[j]='\0';
      current_width = XTextWidth (defaults.font, tmp_buf, strlen (tmp_buf));
      if(current_width>ret) ret=current_width;
      j=0;
    }
    else tmp_buf[j]=msg[i];
  }
  PRINT_DEBUG(("max_line_length: %d\n", ret));
  return ret;
}

int
pos_in_line (char* msg, int pos)
{
  int i=pos - 1;
  int ret=0;
  if(i>=0) {
    for(; i<=pos && i>=0; ++ret, --i) if(msg[i]=='\n') break;
  }
  PRINT_DEBUG (("pos_in_line(\"%s\", %d) = %d\n", msg, pos, ret));
  return ret;
}

int
line_beginning (char* msg, int pos)
{
  int ret=0;
  int i=pos-1;
  if(i) {
    for(; i>=0; --i) {
      /*      PRINT_DEBUG("pos = %d, i = %d, msg[i] = '%c'\n", pos, i, msg[i]); */
      if (msg[i]=='\n') {
	ret=i+1;
	break;
      }
    }
  }
  PRINT_DEBUG (("line_beginning(\"%s\", %d) = %d\n", msg, pos, ret));
  return ret;

}

void
marked_wrapped_message (char *msg, int mark_start, int mark_end)
{
  XGCValues lgv;
  GC lgc;
  unsigned long mask;
  screen_info *s = current_screen ();
  int i=0;
  int j=0;
  int num_lines;
  int line_no=0;
  char tmp_buf[100];



  int width = defaults.bar_x_padding * 2 + max_line_length(msg);
    /* XTextWidth (defaults.font, msg, strlen (msg)); */
  int line_height = (FONT_HEIGHT (defaults.font) + defaults.bar_y_padding * 2);
  int height;

  PRINT_DEBUG (("msg = %s\n", msg));
  PRINT_DEBUG (("mark_start = %d, mark_end = %d\n", mark_start, mark_end));


  num_lines = count_lines(msg, strlen(msg));
  height = line_height * num_lines;

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
		     bar_x (s, width), bar_y (s, height),
		     width,
		     height);

  XRaiseWindow (dpy, s->bar_window);
  XClearWindow (dpy, s->bar_window);
  XSync (dpy, False);

  /*  if(!defaults.wrap_window_list){
    XDrawString (dpy, s->bar_window, s->normal_gc, 
		 defaults.bar_x_padding, 
		 defaults.bar_y_padding + defaults.font->max_bounds.ascent,
		 msg, strlen (msg));
		 } else { */
  for(i=0; i<=strlen(msg); ++i) {
    if (msg[i]!='\0' && msg[i]!='\n') {
      tmp_buf[j]=msg[i];
      j++;
    }
    else {
      tmp_buf[j]='\0';
      XDrawString (dpy, s->bar_window, s->normal_gc,
		   defaults.bar_x_padding,
		   defaults.bar_y_padding + defaults.font->max_bounds.ascent
		   +  line_no * line_height,
		   tmp_buf, strlen(tmp_buf));
      j=0;
      line_no++; 
    }
  }
 


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

  if (mark_start > mark_end+mark_start)
    {
      int tmp;
      tmp = mark_start;
      mark_start = mark_end;
      mark_end = tmp;
    }

  /* xor the string representing the current window */
  if (mark_end)
    {
      int start;
      int end;
      int width;
      int start_line, 		end_line;
      int start_pos_in_line, 	end_pos_in_line;
      int start_line_beginning,	end_line_beginning;

      start_line=count_lines(msg, mark_start);
      end_line=count_lines(msg, mark_end+mark_start-1);

      start_pos_in_line = pos_in_line(msg, mark_start);
      end_pos_in_line = pos_in_line(msg, mark_end+mark_start-1);

      start_line_beginning = line_beginning(msg, mark_start);
      end_line_beginning = line_beginning(msg, mark_end+mark_start-1);

      PRINT_DEBUG (("start_line = %d, end_line = %d\n", start_line, end_line));

      if (mark_start == 0 || start_pos_in_line == 0)
	start = 0;
      else
	start = XTextWidth (defaults.font, 
			    &msg[start_line_beginning], 
			    start_pos_in_line) + defaults.bar_x_padding;


      end = XTextWidth (defaults.font, &msg[end_line_beginning], 
			end_pos_in_line + (msg[end_line_beginning+end_pos_in_line+1] == '\0'?1:0)  ) 
	+ defaults.bar_x_padding * 2;
      
      if (mark_end != strlen (msg))   	end -= defaults.bar_x_padding;

      width = end - start;

      PRINT_DEBUG (("start = %d, end = %d, width = %d\n", start, end, width));

      lgv.foreground = current_screen()->fg_color;
      lgv.function = GXxor;
      mask = GCForeground | GCFunction;
      lgc = XCreateGC(dpy, s->root, mask, &lgv);

      XFillRectangle (dpy, s->bar_window, lgc, start, (start_line-1)*line_height, width, (end_line-start_line+1)*line_height);

      lgv.foreground = s->bg_color;
      lgc = XCreateGC(dpy, s->root, mask, &lgv);

      XFillRectangle (dpy, s->bar_window, lgc, start, (start_line-1)*line_height, width, (end_line-start_line+1)*line_height);
    }

  /* Keep a record of the message. */
  if (last_msg)
    free (last_msg);
  last_msg = xstrdup (msg);
  last_mark_start = mark_start;
  last_mark_end = mark_end;
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

  PRINT_DEBUG (("%s\n", msg));

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
		     bar_x (s, width), bar_y (s, width),
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
      int width;

      if (mark_start == 0)
	start = 0;
      else
	start = XTextWidth (defaults.font, msg, mark_start) + defaults.bar_x_padding;

      if (mark_end == strlen (msg))
	end = XTextWidth (defaults.font, msg, mark_end) + defaults.bar_x_padding * 2;
      else
	end = XTextWidth (defaults.font, msg, mark_end) + defaults.bar_x_padding;

      width = end - start;

      PRINT_DEBUG (("start = %d, end = %d, width = %d\n", start, end, width));

      lgv.foreground = current_screen()->fg_color;
      lgv.function = GXxor;
      mask = GCForeground | GCFunction;
      lgc = XCreateGC(dpy, s->root, mask, &lgv);

      XFillRectangle (dpy, s->bar_window, lgc, start, 0, width, height);
      XFreeGC (dpy, lgc);

      lgv.foreground = s->bg_color;
      lgc = XCreateGC(dpy, s->root, mask, &lgv);

      XFillRectangle (dpy, s->bar_window, lgc, start, 0, width, height);
      XFreeGC (dpy, lgc);
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

/* Free any memory associated with the bar. */
void
free_bar ()
{
  if (last_msg)
    free (last_msg);
}
