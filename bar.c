/* Functionality for a bar across the bottom of the screen listing the
   windows currently managed. */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ratpoison.h"

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

int
show_bar (screen_info *s)
{
  if (!s->bar_is_raised)
    {
      s->bar_is_raised = 1;
      XMapWindow (dpy, s->bar_window);
      update_window_names (s);
  
      /* Set an alarm to auto-hide the bar BAR_TIMEOUT seconds later */
      alarm (BAR_TIMEOUT);
      return 1;
    }

  return 0;
}

/* Toggle the display of the program bar */
void
toggle_bar (screen_info *s)
{
  if (!hide_bar (s)) show_bar (s);
}

static int
calc_bar_width (XFontStruct *font)
{
  char str[100];		/* window names are capped at 99 chars */
  int i;
  int size = 1;
  rp_window *cur;

  for (i=0, cur = rp_window_head; cur; cur = cur->next)
    {
      if (cur->state == STATE_UNMAPPED) continue;
      
      sprintf (str, "%d-%s", i, cur->name);
      size += 10 + XTextWidth (font, str, strlen (str));
      i++;
    }

  return size;
}

static int
bar_x (screen_info *s, int width)
{
  if (BAR_LOCATION >= 2) return s->root_attr.width - width;
  else return 0;
}

static int
bar_y (screen_info *s)
{
  if (BAR_LOCATION % 2) return 0;
  else return s->root_attr.height - (FONT_HEIGHT (s->font) + BAR_PADDING * 2) - 2;
}

void
update_window_names (screen_info *s)
{
  char str[100];		/* window names are capped at 99 chars */
  int i;
  int width = calc_bar_width (s->font);
  rp_window *cur;
  int cur_x = 5;

  if (!s->bar_is_raised) return;

  XMoveResizeWindow (dpy, s->bar_window, 
		     bar_x (s, width), bar_y (s),
		     width,
		     (FONT_HEIGHT (s->font) + BAR_PADDING * 2));
  XClearWindow (dpy, s->bar_window);
  XRaiseWindow (dpy, s->bar_window);

  if (rp_window_head == NULL) return;
  for (i=0, cur = rp_window_head; cur; cur = cur->next)
    {
      if (cur->state == STATE_UNMAPPED) continue;

      sprintf (str, "%d-%s", i, cur->name);
      if ( rp_current_window == cur) 
	{
	  XDrawString (dpy, s->bar_window, s->bold_gc, cur_x, 
		       BAR_PADDING + s->font->max_bounds.ascent, str, strlen (str));
	}
      else
	{
	  XDrawString (dpy, s->bar_window, s->normal_gc, cur_x, 
		       BAR_PADDING + s->font->max_bounds.ascent, str, strlen (str));
	}

      cur_x += 10 + XTextWidth (s->font, str, strlen (str));
      i++;
    }
}
