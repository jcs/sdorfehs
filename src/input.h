/* Function prototypes.
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

#ifndef _RATPOISON_INPUT_H
#define _RATPOISON_INPUT_H 1

char *keysym_to_string (KeySym keysym, unsigned int modifier);
int cook_keycode (XKeyEvent *ev, KeySym *keysym, unsigned int *mod, char *keysym_name, int len, int ignore_bad_mods);
char *get_input (char *prompt, int history_id, completion_fn fn);
char *get_more_input (char *prompt, char *preinput, int history_id, completion_fn fn);
void read_any_key (void);
int read_single_key (KeySym *keysym, unsigned int *modifiers, char *keysym_name, int len);
int read_key (KeySym *keysym, unsigned int *modifiers, char *keysym_name, int len);
unsigned int x11_mask_to_rp_mask (unsigned int mask);
unsigned int rp_mask_to_x11_mask (unsigned int mask);
void update_modifier_map (void);
void grab_key (KeySym keysym, unsigned int modifiers, Window grab_window);

void ring_bell (void);

void init_xkb (void);

#endif /* ! _RATPOISON_INPUT_H */
