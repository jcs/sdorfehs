/* for reading kdb input from the user. Currently only used to read in
   the name of a window to jump to. */

#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>

#include "ratpoison.h"


static int
read_key ()
{
  XEvent ev;

  XMaskEvent (dpy, KeyPressMask, &ev);
  return XLookupKeysym ((XKeyEvent *)&ev, 0);
}

/* pass in a pointer a string and how much room we have, and fill it
   in with text from the user. */
void
get_input (screen_info *s, char *prompt, char *str, int len)
{
  int cur_len;			/* Current length of the string. */
  int ch;
  int revert;
  Window fwin;
  int prompt_width = XTextWidth (s->font, prompt, strlen (prompt));
  int width = 100 + prompt_width;

  /* We don't want to draw overtop of the program bar. */
  hide_bar (s);

  XMapWindow (dpy, s->input_window);
  XMoveResizeWindow (dpy, s->input_window, 
		     bar_x (s, width), bar_y (s), width, (FONT_HEIGHT (s->font) + BAR_PADDING * 2));
  XClearWindow (dpy, s->input_window);
  XRaiseWindow (dpy, s->input_window);

  /* draw the window prompt. */
  XDrawString (dpy, s->input_window, s->bold_gc, 5, 
	       BAR_PADDING + s->font->max_bounds.ascent, prompt, strlen (prompt));

  XGetInputFocus (dpy, &fwin, &revert);
  XSetInputFocus (dpy, s->input_window, RevertToPointerRoot, CurrentTime);

  cur_len = 0;
  while ((ch = read_key ()) != XK_Return)
    {
      printf ("key %d\n", ch);
      if (ch == XK_BackSpace)
	{
	  if (cur_len > 0) cur_len--;
	  XClearWindow (dpy, s->input_window);
	  XDrawString (dpy, s->input_window, s->bold_gc, 5, 
		       BAR_PADDING + s->font->max_bounds.ascent, prompt, strlen (prompt));
	  XDrawString (dpy, s->input_window, s->bold_gc, 5 + prompt_width,
		       BAR_PADDING + s->font->max_bounds.ascent, str, cur_len);
	}
      else if (ch >= ' ')
	{
	  str[cur_len] = ch;
	  if (cur_len < len - 1) cur_len++;

	  XDrawString (dpy, s->input_window, s->bold_gc, 5 + prompt_width,
		       BAR_PADDING + s->font->max_bounds.ascent, str, cur_len);
	}
    }

  str[cur_len] = 0;
  XSetInputFocus (dpy, fwin, RevertToPointerRoot, CurrentTime);
  XUnmapWindow (dpy, s->input_window);
}  
      
