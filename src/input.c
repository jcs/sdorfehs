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

/* Variables to keep track of input history. */
static char *input_history[INPUT_MAX_HISTORY];
static int input_num_history_entries = 0;

/* Convert an X11 modifier mask to the rp modifier mask equivalent, as
   best it can (the X server may not have a hyper key defined, for
   instance). */
unsigned int
x11_mask_to_rp_mask (unsigned int mask)
{
  unsigned int result = 0;

  PRINT_DEBUG (("x11 mask = %x\n", mask));

  result |= mask & ControlMask ? RP_CONTROL_MASK:0;
  result |= mask & rp_modifier_info.meta_mod_mask ? RP_META_MASK:0;
  result |= mask & rp_modifier_info.alt_mod_mask ? RP_ALT_MASK:0;
  result |= mask & rp_modifier_info.hyper_mod_mask ? RP_HYPER_MASK:0;
  result |= mask & rp_modifier_info.super_mod_mask ? RP_SUPER_MASK:0;

  PRINT_DEBUG (("rp mask = %x\n", mask));

  return result;
}

/* Convert an rp modifier mask to the x11 modifier mask equivalent, as
   best it can (the X server may not have a hyper key defined, for
   instance). */
unsigned int
rp_mask_to_x11_mask (unsigned int mask)
{
  unsigned int result = 0;

  PRINT_DEBUG (("rp mask = %x\n", mask));

  result |= mask & RP_CONTROL_MASK ? ControlMask:0;
  result |= mask & RP_META_MASK ? rp_modifier_info.meta_mod_mask:0;
  result |= mask & RP_ALT_MASK ? rp_modifier_info.alt_mod_mask:0;
  result |= mask & RP_HYPER_MASK ? rp_modifier_info.hyper_mod_mask:0;
  result |= mask & RP_SUPER_MASK ? rp_modifier_info.super_mod_mask:0;

  PRINT_DEBUG (("x11 mask = %x\n", mask));

  return result;
}


/* Figure out what keysyms are attached to what modifiers */
void
update_modifier_map ()
{
  unsigned int modmasks[] = 
    { Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask };
  int row, col;	/* The row and column in the modifier table.  */
  XModifierKeymap *mods;

  rp_modifier_info.meta_mod_mask = 0;
  rp_modifier_info.alt_mod_mask = 0;
  rp_modifier_info.super_mod_mask = 0;
  rp_modifier_info.hyper_mod_mask = 0;
  rp_modifier_info.num_lock_mask = 0;
  rp_modifier_info.scroll_lock_mask = 0;

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
	    PRINT_DEBUG (("Found Meta on %d\n",
			 rp_modifier_info.meta_mod_mask));
	    break;

	  case XK_Alt_L:
	  case XK_Alt_R:
	    rp_modifier_info.alt_mod_mask |= modmasks[row - 3];
	    PRINT_DEBUG (("Found Alt on %d\n",
			 rp_modifier_info.alt_mod_mask));
	    break;

	  case XK_Super_L:
	  case XK_Super_R:
	    rp_modifier_info.super_mod_mask |= modmasks[row - 3];
	    PRINT_DEBUG (("Found Super on %d\n",
			 rp_modifier_info.super_mod_mask));
	    break;

	  case XK_Hyper_L:
	  case XK_Hyper_R:
	    rp_modifier_info.hyper_mod_mask |= modmasks[row - 3];
	    PRINT_DEBUG (("Found Hyper on %d\n",
			 rp_modifier_info.hyper_mod_mask));
	    break;

	  case XK_Num_Lock:
	    rp_modifier_info.num_lock_mask |= modmasks[row - 3];
	    PRINT_DEBUG (("Found NumLock on %d\n", 
			 rp_modifier_info.num_lock_mask));
	    break;

	  case XK_Scroll_Lock:
	    rp_modifier_info.scroll_lock_mask |= modmasks[row - 3];
	    PRINT_DEBUG (("Found ScrollLock on %d\n", 
			 rp_modifier_info.scroll_lock_mask));
	    break;
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

/* Grab the key while ignoring annoying modifier keys including
   caps lock, num lock, and scroll lock. */
void
grab_key (int keycode, unsigned int modifiers, Window grab_window)
{
  unsigned int mod_list[8];
  int i;

  /* Convert to a modifier mask that X Windows will understand. */
  modifiers = rp_mask_to_x11_mask (modifiers);

  /* Create a list of all possible combinations of ignored
     modifiers. Assumes there are only 3 ignored modifiers. */
  mod_list[0] = 0;
  mod_list[1] = LockMask;
  mod_list[2] = rp_modifier_info.num_lock_mask;
  mod_list[3] = mod_list[1] | mod_list[2];
  mod_list[4] = rp_modifier_info.scroll_lock_mask;
  mod_list[5] = mod_list[1] | mod_list[4];
  mod_list[6] = mod_list[2] | mod_list[4];
  mod_list[7] = mod_list[1] | mod_list[2] | mod_list[4];

  /* Grab every combination of ignored modifiers. */
  for (i=0; i<8; i++)
    {
      XGrabKey(dpy, keycode, modifiers | mod_list[i],
	       grab_window, True, GrabModeAsync, GrabModeAsync);
    }
}


/* Return the name of the keysym. caller must free returned pointer */
char *
keysym_to_string (KeySym keysym, unsigned int modifier)
{
  static char *null_string = "NULL"; /* A NULL string. */
  struct sbuf *name;
  char *tmp;

  name = sbuf_new (0);

  if (modifier & RP_CONTROL_MASK) sbuf_concat (name, "C-");
  if (modifier & RP_META_MASK) sbuf_concat (name, "M-");
  if (modifier & RP_ALT_MASK) sbuf_concat (name, "A-");
  if (modifier & RP_HYPER_MASK) sbuf_concat (name, "H-");
  if (modifier & RP_SUPER_MASK) sbuf_concat (name, "S-");
    
  /* On solaris machines (perhaps other machines as well) this call
     can return NULL. In this case use the "NULL" string. */
  tmp = XKeysymToString (keysym);
  if (tmp == NULL)
    tmp = null_string;

  sbuf_concat (name, tmp);

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
cook_keycode (XKeyEvent *ev, KeySym *keysym, unsigned int *mod, char *keysym_name, int len, int ignore_bad_mods)
{
  int nbytes;
 
  if (ignore_bad_mods)
    {
      ev->state &= ~(LockMask
		     | rp_modifier_info.num_lock_mask
		     | rp_modifier_info.scroll_lock_mask);
    }

  nbytes =  XLookupString (ev, keysym_name, len, keysym, NULL);

  *mod = ev->state;
  *mod &= (rp_modifier_info.meta_mod_mask
	   | rp_modifier_info.alt_mod_mask
	   | rp_modifier_info.hyper_mod_mask
	   | rp_modifier_info.super_mod_mask
	   | ControlMask );

  return nbytes;
}

int
read_key (KeySym *keysym, unsigned int *modifiers, char *keysym_name, int len, int gobble_rel)
{
  int key_presses = 0;
  XEvent ev;
  int nbytes;
  unsigned int keycode;

  /* Read a key from the keyboard. */
  do
    {
      XMaskEvent (dpy, KeyPressMask, &ev);
      /* Store the keycode so we can wait for it's corresponding key
	 release event. */
      keycode = ev.xkey.keycode;
      *modifiers = ev.xkey.state;
      nbytes = cook_keycode (&ev.xkey, keysym, modifiers, keysym_name, len, 0);
    } while (IsModifierKey (*keysym));

  PRINT_DEBUG (("key press events: %d\n", key_presses));

  /* Gobble the release event for the key we pressed. */
  if (gobble_rel)
    do
      {
	XMaskEvent (dpy, KeyReleaseMask, &ev);
      } while (ev.xkey.keycode != keycode);

  return nbytes;
}

static void
update_input_window (screen_info *s, char *prompt, char *input, int input_len)
{
  int 	prompt_width = XTextWidth (defaults.font, prompt, strlen (prompt));
  int 	input_width  = XTextWidth (defaults.font, input, input_len);
  int 	width, height;

  width = defaults.bar_x_padding * 2 + prompt_width + input_width;
  height = (FONT_HEIGHT (defaults.font) + defaults.bar_y_padding * 2);

  if (width < defaults.input_window_size + prompt_width)
    {
      width = defaults.input_window_size + prompt_width;
    }

  XMoveResizeWindow (dpy, s->input_window, 
		     bar_x (s, width), bar_y (s, height), width, height);

  XClearWindow (dpy, s->input_window);
  XSync (dpy, False);

  XDrawString (dpy, s->input_window, s->normal_gc, 
	       defaults.bar_x_padding, 
	       defaults.bar_y_padding + defaults.font->max_bounds.ascent, prompt, 
	       strlen (prompt));

  XDrawString (dpy, s->input_window, s->normal_gc, 
	       defaults.bar_x_padding + prompt_width,
	       defaults.bar_y_padding + defaults.font->max_bounds.ascent, input, 
	       input_len);

  /* Draw a cheap-o cursor. */
  XDrawLine (dpy, s->input_window, s->normal_gc, 
	     defaults.bar_x_padding + prompt_width + input_width + 2, 
	     defaults.bar_y_padding + 1, 
	     defaults.bar_x_padding + prompt_width + input_width + 2,
	     defaults.bar_y_padding + FONT_HEIGHT (defaults.font) - 1);
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
  int history_index = input_num_history_entries;

  /* Allocate some memory to start with. */
  str = (char *) xmalloc ( allocated_len );

  /* load in the preinput */
  strcpy (str, preinput);
  cur_len = strlen (preinput);

  /* We don't want to draw overtop of the program bar. */
  hide_bar (s);

  XMapWindow (dpy, s->input_window);
  XRaiseWindow (dpy, s->input_window);
  XClearWindow (dpy, s->input_window);
  XSync (dpy, False);

  update_input_window (s, prompt, str, cur_len);

  XGetInputFocus (dpy, &fwin, &revert);
  XSetInputFocus (dpy, s->input_window, RevertToPointerRoot, CurrentTime);
  /* XSync (dpy, False); */


  nbytes = read_key (&ch, &modifier, keysym_buf, keysym_bufsize, 0);
  while (ch != XK_Return) 
    {
      PRINT_DEBUG (("key %ld\n", ch));
      if (ch == XK_BackSpace)
	{
	  if (cur_len > 0) cur_len--;
	  update_input_window(s, prompt, str, cur_len);
	}
      else if (ch == INPUT_PREV_HISTORY_KEY 
	       && modifier == INPUT_PREV_HISTORY_MODIFIER)
	{
	  /* Cycle through the history. */
	  if (input_num_history_entries > 0)
	    {
	      history_index--;
	      if (history_index < 0)
		{
		  history_index = input_num_history_entries - 1;
		}

	      free (str);
	      str = xstrdup (input_history[history_index]);
	      allocated_len = strlen (str) + 1;
	      cur_len = allocated_len - 1;

	      update_input_window (s, prompt, str, cur_len);
	    }
	}
      else if (ch == INPUT_NEXT_HISTORY_KEY 
	       && modifier == INPUT_NEXT_HISTORY_MODIFIER)
	{
	  /* Cycle through the history. */
	  if (input_num_history_entries > 0)
	    {
	      history_index++;
	      if (history_index >= input_num_history_entries)
		{
		  history_index = 0;
		}

	      free (str);
	      str = xstrdup (input_history[history_index]);
	      allocated_len = strlen (str) + 1;
	      cur_len = allocated_len - 1;

	      update_input_window (s, prompt, str, cur_len);
	    }
	}
      else if (ch == INPUT_ABORT_KEY && modifier == INPUT_ABORT_MODIFIER)
	{
	  /* User aborted. */
	  free (str);
	  XSetInputFocus (dpy, fwin, RevertToPointerRoot, CurrentTime);
	  XUnmapWindow (dpy, s->input_window);
	  return NULL;
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

      nbytes = read_key (&ch, &modifier, keysym_buf, keysym_bufsize, 0);
    }

  str[cur_len] = 0;

  /* Push the history entries down. */
  if (input_num_history_entries >= INPUT_MAX_HISTORY)
    {
      int i;
      free (input_history[0]);
      for (i=0; i<INPUT_MAX_HISTORY-1; i++)
	{
	  input_history[i] = input_history[i+1];
	}

      input_num_history_entries--;
    }

  /* Store the string in the history. */
  input_history[input_num_history_entries] = xstrdup (str);
  input_num_history_entries++;

  XSetInputFocus (dpy, fwin, RevertToPointerRoot, CurrentTime);
  XUnmapWindow (dpy, s->input_window);
  return str;
}  

void
free_history ()
{
  int i;

  for (i=0; i<input_num_history_entries; i++)
    free (input_history[i]);
}
