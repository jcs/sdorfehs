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

/* Figure out what keysyms are attached to what modifiers */
void
update_modifier_map ()
{
  unsigned int modmasks[] = 
    { Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask };
  int row, col;	/* The row and column in the modifier table.  */
  XModifierKeymap *mods;

/*   rp_modifier_info.mode_switch_mask = 0; */
  rp_modifier_info.meta_mod_mask = 0;
  rp_modifier_info.alt_mod_mask = 0;
  rp_modifier_info.super_mod_mask = 0;
  rp_modifier_info.hyper_mod_mask = 0;

  mods = XGetModifierMapping (dpy);

  for (row=3; row < 8; row++)
    for (col=0; col < mods->max_keypermod; col++)
      {
	KeyCode code = mods->modifiermap[(row * mods->max_keypermod) + col];
	
	if (code == 0) continue;
	
	switch (XKeycodeToKeysym(dpy, code, 0))
	  {
	    case XK_Meta_L:
	    case XK_Meta_R:
	      rp_modifier_info.meta_mod_mask |= modmasks[row - 3];
	      PRINT_DEBUG ("Found Meta on %d\n",
			   rp_modifier_info.meta_mod_mask);
	      break;

	    case XK_Alt_L:
	    case XK_Alt_R:
	      rp_modifier_info.alt_mod_mask |= modmasks[row - 3];
	      PRINT_DEBUG ("Found Alt on %d\n",
			   rp_modifier_info.alt_mod_mask);
	      break;

	    case XK_Super_L:
	    case XK_Super_R:
	      rp_modifier_info.super_mod_mask |= modmasks[row - 3];
	      PRINT_DEBUG ("Found Super on %d\n",
			   rp_modifier_info.super_mod_mask);
	      break;

	    case XK_Hyper_L:
	    case XK_Hyper_R:
	      rp_modifier_info.hyper_mod_mask |= modmasks[row - 3];
	      PRINT_DEBUG ("Found Hyper on %d\n",
			   rp_modifier_info.hyper_mod_mask);
	      break;

/* 	  case XK_Mode_switch: */
/* 	    rp_modifier_info.mode_switch_mask |= modmasks[row - 3]; */
/* 	      PRINT_DEBUG ("Found Mode_switch on %d\n", */
/* 			   rp_modifier_info.mode_switch_mask); */
/* 	    break; */

	  default:
	    break;
	  }
      }
  
  /* Stolen from Emacs 21.0.90 - xterm.c */
  /* If we couldn't find any meta keys, accept any alt keys as meta keys.  */
  if (! rp_modifier_info.meta_mod_mask)
    {
      rp_modifier_info.meta_mod_mask = rp_modifier_info.alt_mod_mask;
      rp_modifier_info.alt_mod_mask = 0;
    }

  /* If some keys are both alt and meta,
     make them just meta, not alt.  */
  if (rp_modifier_info.alt_mod_mask & rp_modifier_info.meta_mod_mask)
    {
      rp_modifier_info.alt_mod_mask &= ~rp_modifier_info.meta_mod_mask;
    }

  XFreeModifiermap (mods);
}

/* Return the name of the keysym. caller must free returned pointer */
char *
keysym_to_string (KeySym keysym, unsigned int modifier)
{
  struct sbuf *name;
  char *tmp;

  name = sbuf_new (0);

  if (modifier & ControlMask) sbuf_concat (name, "C-");
  if (modifier & rp_modifier_info.meta_mod_mask) sbuf_concat (name, "M-");
  if (modifier & rp_modifier_info.alt_mod_mask) sbuf_concat (name, "A-");
  if (modifier & rp_modifier_info.hyper_mod_mask) sbuf_concat (name, "H-");
  if (modifier & rp_modifier_info.super_mod_mask) sbuf_concat (name, "S-");
    
  sbuf_concat (name, XKeysymToString (keysym));

  /* Eat the nut and throw away the shells. */
  tmp = sbuf_get (name);
  free (name);

  return tmp;
}

/* Cooks a keycode + modifier into a keysym + modifier. This should be
   used anytime meaningful key information is to be extracted from a
   KeyPress or KeyRelease event. 

   returns the number of bytes in keysym_name. If you are not
   interested in the keysym name pass in NULL for keysym_name and 0
   for len. */
int
cook_keycode (XKeyEvent *ev, KeySym *keysym, unsigned int *mod, char *keysym_name, int len)
{
int nbytes;

 nbytes =  XLookupString (ev, keysym_name, len, keysym, NULL);

  *mod = ev->state;
  *mod &= (rp_modifier_info.meta_mod_mask
	   | rp_modifier_info.alt_mod_mask
	   | rp_modifier_info.hyper_mod_mask
	   | rp_modifier_info.super_mod_mask
	   | ControlMask );

  return nbytes;
}

/* void */
/* cook_keycode (KeyCode keycode, KeySym *keysym, unsigned int *mod) */
/* { */
/*   KeySym normal, shifted; */

/*   /\* FIXME: Although this should theoretically work, the mod that */
/*      mode_switch is on doesn't seem to get activated. Instead the */
/*      2<<13 bit gets set! It doesn't seem to matter which mod I put */
/*      Mode_switch on. So if this doesn't work try uncommented the line */
/*      below and commented the current one. *\/ */

/* /\*   if (*mod & 8192) *\/ */
/*   if (*mod & rp_modifier_info.mode_switch_mask) */
/*     { */
/*       normal = XKeycodeToKeysym(dpy, keycode, 2); */
/*       if (normal == NoSymbol) normal = XKeycodeToKeysym(dpy, keycode, 0); */
/*       shifted = XKeycodeToKeysym(dpy, keycode, 3); */
/*       if (shifted == NoSymbol) shifted = XKeycodeToKeysym(dpy, keycode, 1); */

/*       /\* Remove the mode switch modifier since we have dealt with it. *\/ */
/*       *mod &= ~rp_modifier_info.mode_switch_mask; */
/*     } */
/*   else */
/*     { */
/*       normal = XKeycodeToKeysym(dpy, keycode, 0); */
/*       shifted = XKeycodeToKeysym(dpy, keycode, 1); */
/*     } */

/*   /\* FIXME: eew, this looks gross. *\/ */
/*   if (*mod & (ShiftMask | LockMask)) */
/*     { */
/*       /\* if the shifted code is not defined, then we use the normal */
/* 	 keysym and keep the shift mask *\/ */
/*       if (shifted == NoSymbol) */
/* 	{ */
/* 	  *keysym = normal; */
/* 	} */
/*       /\* But if the shifted code is defined, we use it and remove the */
/* 	 shift mask *\/ */
/*       else if (*mod & ShiftMask) */
/* 	{ */
/* 	  *keysym = shifted; */
/* 	  *mod &= ~(ShiftMask | LockMask); */
/* 	} */
/*       /\* If caps lock is on, use shifted for alpha keys *\/ */
/*       else if (normal >= XK_a  */
/* 	       && normal <= XK_z */
/* 	       && *mod & LockMask) */
/* 	{ */
/* 	  *keysym = shifted; */
/* 	} */
/*       else */
/* 	{ */
/* 	  *keysym = normal; */
/* 	} */
/*     } */
/*   else */
/*     { */
/*       *keysym = normal; */
/*     } */

/*   PRINT_DEBUG ("cooked keysym: %ld '%c' mask: %d\n",  */
/* 	       *keysym, (char)*keysym, *mod); */
/* } */

int
read_key (KeySym *keysym, unsigned int *modifiers, char *keysym_name, int len)
{
  XEvent ev;
  int nbytes;

  do
    {
      XMaskEvent (dpy, KeyPressMask, &ev);
      *modifiers = ev.xkey.state;
      nbytes = cook_keycode (&ev.xkey, keysym, modifiers, keysym_name, len);
    } while (IsModifierKey (*keysym));

  return nbytes;
}

static void
update_input_window (screen_info *s, char *prompt, char *input, int input_len)
{
  int 	prompt_width = XTextWidth (s->font, prompt, strlen (prompt));
  int 	input_width  = XTextWidth (s->font, input, input_len);
  int 	width;

  width = BAR_X_PADDING * 2 + prompt_width + input_width;

  if (width < INPUT_WINDOW_SIZE + prompt_width)
    {
      width = INPUT_WINDOW_SIZE + prompt_width;
    }

  XMoveResizeWindow (dpy, s->input_window, 
		     bar_x (s, width), bar_y (s), width,
		     (FONT_HEIGHT (s->font) + BAR_Y_PADDING * 2));

  XClearWindow (dpy, s->input_window);

  XDrawString (dpy, s->input_window, s->normal_gc, 
	       BAR_X_PADDING, 
	       BAR_Y_PADDING + s->font->max_bounds.ascent, prompt, 
	       strlen (prompt));

  XDrawString (dpy, s->input_window, s->normal_gc, 
	       BAR_X_PADDING + prompt_width,
	       BAR_Y_PADDING + s->font->max_bounds.ascent, input, 
	       input_len);
}

char *
get_input (char *prompt)
{
  return get_more_input (prompt, "");
}

char *
get_more_input (char *prompt, char *preinput)
{
  /* Emacs 21 uses a 513 byte string to store the keysym name. */
  char keysym_buf[513];	
  int keysym_bufsize = sizeof (keysym_buf);
  int nbytes;
  screen_info *s = current_screen ();
  int cur_len = 0;		/* Current length of the string. */
  int allocated_len=100; /* The amount of memory we allocated for str */
  KeySym ch;
  unsigned int modifier;
  int revert;
  Window fwin;
  char *str;

  /* Allocate some memory to start with */
  str = (char *) xmalloc ( allocated_len );

  /* load in the preinput */
  strcpy (str, preinput);
  cur_len = strlen (preinput);

  /* We don't want to draw overtop of the program bar. */
  hide_bar (s);

  XMapWindow (dpy, s->input_window);
  XClearWindow (dpy, s->input_window);
  XRaiseWindow (dpy, s->input_window);

  update_input_window (s, prompt, str, cur_len);

  XGetInputFocus (dpy, &fwin, &revert);
  XSetInputFocus (dpy, s->input_window, RevertToPointerRoot, CurrentTime);
  /* XSync (dpy, False); */


  nbytes = read_key (&ch, &modifier, keysym_buf, keysym_bufsize);
  while (ch != XK_Return) 
    {
      PRINT_DEBUG ("key %ld\n", ch);
      if (ch == XK_BackSpace)
	{
	  if (cur_len > 0) cur_len--;
	  update_input_window(s, prompt, str, cur_len);
	}
      else
	{
	  if (cur_len + nbytes > allocated_len - 1)
	    {
	      allocated_len += nbytes + 100;
	      str = xrealloc ( str, allocated_len );
	    }

	  strncpy (&str[cur_len], keysym_buf, nbytes);
/* 	  str[cur_len] = ch; */
	  cur_len+=nbytes;

	  update_input_window(s, prompt, str, cur_len);
	}

      nbytes = read_key (&ch, &modifier, keysym_buf, keysym_bufsize);
    }

  str[cur_len] = 0;
  XSetInputFocus (dpy, fwin, RevertToPointerRoot, CurrentTime);
  XUnmapWindow (dpy, s->input_window);
  return str;
}  
