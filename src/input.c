/* Read kdb input from the user.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "ratpoison.h"

/* Return the name of the keysym. caller must free returned pointer */
char *
keysym_to_string (KeySym keysym, unsigned int modifier)
{
  const unsigned int mod_table[] = {ControlMask, 
				    Mod1Mask, 
				    Mod2Mask, 
				    Mod3Mask, 
				    Mod4Mask, 
				    Mod5Mask};
  const unsigned char str_table[] = "CM2345";

  unsigned char *name;
  int pos, i;

  name = malloc (15);
  if (name == NULL)
    {
      PRINT_ERROR ("Out of memory!\n");
      exit (EXIT_FAILURE);
    }

  for (pos = 0, i = 0; i < 6; i++)
    {
      if (modifier & mod_table[i])
	{
	  name[pos] = str_table[i];
	  name[pos+1] = '-';
	  pos += 2;
	}
    }

  name[pos] = keysym;
  name[pos+1] = '\0';

  return name;
}

/* Cooks a keycode + modifier into a keysym + modifier. This should be
   used anytime meaningful key information is to be extracted from a
   KeyPress or KeyRelease event. */
void
cook_keycode (KeyCode keycode, KeySym *keysym, unsigned int *mod)
{
  KeySym normal, shifted;

  normal = XKeycodeToKeysym(dpy, keycode, 0);
  shifted = XKeycodeToKeysym(dpy, keycode, 1);

  /* FIXME: eew, this looks gross. */
  if (*mod & (ShiftMask | LockMask))
    {
      /* if the shifted code is not defined, then we use the normal
	 keysym and keep the shift mask */
      if (shifted == NoSymbol)
	{
	  *keysym = normal;
	}
      /* But if the shifted code is defined, we use it and remove the
	 shift mask */
      else if (*mod & ShiftMask)
	{
	  *keysym = shifted;
	  *mod &= ~(ShiftMask | LockMask);
	}
      /* If caps lock is on, use shifted for alpha keys */
      else if (normal >= XK_a 
	       && normal <= XK_z
	       && *mod & LockMask)
	{
	  *keysym = shifted;
	}
      else
	{
	  *keysym = normal;
	}
    }
  else
    {
      *keysym = normal;
    }

  PRINT_DEBUG ("cooked keysym: %ld '%c' mask: %d\n", 
	       *keysym, (char)*keysym, *mod);
}

void
read_key (KeySym *keysym, unsigned int *modifiers)
{
  XEvent ev;

  do
    {
      XMaskEvent (dpy, KeyPressMask, &ev);
      *modifiers = ev.xkey.state;
      cook_keycode (ev.xkey.keycode, keysym, modifiers);
    } while (IsModifierKey (*keysym));
}

char *
get_input (char *prompt)
{
  screen_info *s = current_screen ();
  int cur_len;			/* Current length of the string. */
  int allocated_len=100;		/* The ammount of memory we allocated for str */
  KeySym ch;
  unsigned int modifier;
  int revert;
  Window fwin;
  int prompt_width = XTextWidth (s->font, prompt, strlen (prompt));
  int width = 200 + prompt_width;
  char *str;


  /* We don't want to draw overtop of the program bar. */
  hide_bar (s);

  XMapWindow (dpy, s->input_window);
  XMoveResizeWindow (dpy, s->input_window, 
		     bar_x (s, width), bar_y (s), width, 
		     (FONT_HEIGHT (s->font) + BAR_Y_PADDING * 2));
  XClearWindow (dpy, s->input_window);
  XRaiseWindow (dpy, s->input_window);

  /* draw the window prompt. */
  XDrawString (dpy, s->input_window, s->normal_gc, BAR_X_PADDING, 
	       BAR_Y_PADDING + s->font->max_bounds.ascent, prompt, 
	       strlen (prompt));

  XGetInputFocus (dpy, &fwin, &revert);
  XSetInputFocus (dpy, s->input_window, RevertToPointerRoot, CurrentTime);
  /* XSync (dpy, False); */

  cur_len = 0;

  /* Allocate some memory */
  str = (char *) malloc ( 100 ); /* Most of the time people
  				 won't type more than 100 chars, will they ?*/
  if ( str == NULL )
    {
      PRINT_ERROR ("Out of memory\n");
      exit (EXIT_FAILURE);
    }

  read_key (&ch, &modifier);
  while (ch != XK_Return) 
    {
      PRINT_DEBUG ("key %ld\n", ch);
      if (ch == XK_BackSpace)
	{
	  if (cur_len > 0) cur_len--;
	  XClearWindow (dpy, s->input_window);
	  XDrawString (dpy, s->input_window, s->normal_gc, BAR_X_PADDING, 
		       BAR_Y_PADDING + s->font->max_bounds.ascent, prompt, 
		       strlen (prompt));
	  XDrawString (dpy, s->input_window, s->normal_gc, 
		       BAR_X_PADDING + prompt_width,
		       BAR_Y_PADDING + s->font->max_bounds.ascent, str, 
		       cur_len);
	}
      else
	{
	  if (cur_len > allocated_len - 1)
	    {
	      str = realloc ( str, allocated_len + 100 );
	      /** FIXME: realloc(3) is unlcear to me, how should I check for
	      its success ? - algernon **/
	      allocated_len += 100;
	    }
	  str[cur_len] = ch;
	  cur_len++;

	  XDrawString (dpy, s->input_window, s->normal_gc, 
		       BAR_X_PADDING + prompt_width,
		       BAR_Y_PADDING + s->font->max_bounds.ascent, str, cur_len);
	}

      read_key (&ch, &modifier);
    }

  str[cur_len] = 0;
  XSetInputFocus (dpy, fwin, RevertToPointerRoot, CurrentTime);
  XUnmapWindow (dpy, s->input_window);
  return str;
}  
