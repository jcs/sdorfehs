/* for reading kdb input from the user. Currently only used to read in
   the name of a window to jump to. */

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "ratpoison.h"

/* Cooks a keycode + modifier into a keysym + modifier. This should be
   used anytime meaningful key information is to be extracted from a
   KeyPress or KeyRelease event. */
void
cook_keycode (KeyCode keycode, KeySym *keysym, int *mod)
{
  KeySym normal, shifted;

  if (*mod & ShiftMask)
    {
      normal = XKeycodeToKeysym(dpy, keycode, 0);
      shifted = XKeycodeToKeysym(dpy, keycode, 1);

      /* if the shifted code is not defined, then we use the normal keysym and keep the shift mask */
      if (shifted == NoSymbol)
	{
	  *keysym = normal;
	}
      /* But if the shifted code is defined, we use it and remove the shift mask */
      else
	{
	  *keysym = shifted;
	  *mod &= ~ShiftMask;
	}
    }
  else
    {
      *keysym = XKeycodeToKeysym(dpy, keycode, 0);
    }

  PRINT_DEBUG ("cooked keysym: %ld '%c' mask: %d\n", *keysym, *keysym, *mod);
}

static KeySym
read_key ()
{
  KeySym keysym;
  int mod;
  XEvent ev;

  XMaskEvent (dpy, KeyPressMask, &ev);
  mod = ev.xkey.state;
  cook_keycode (ev.xkey.keycode, &keysym, &mod);

  return keysym;
}

/* pass in a pointer a string and how much room we have, and fill it
   in with text from the user. */
void
get_input (screen_info *s, char *prompt, char *str, int len)
{
  int cur_len;			/* Current length of the string. */
  KeySym ch;
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
      PRINT_DEBUG ("key %d\n", ch);
      if (ch == XK_BackSpace)
	{
	  if (cur_len > 0) cur_len--;
	  XClearWindow (dpy, s->input_window);
	  XDrawString (dpy, s->input_window, s->bold_gc, 5, 
		       BAR_PADDING + s->font->max_bounds.ascent, prompt, strlen (prompt));
	  XDrawString (dpy, s->input_window, s->bold_gc, 5 + prompt_width,
		       BAR_PADDING + s->font->max_bounds.ascent, str, cur_len);
	}
      else if (!IsModifierKey (ch))
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
      
