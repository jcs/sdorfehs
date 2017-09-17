/* Prototypes of all actions that can be performed with keystrokes.
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

#ifndef _RATPOISON_ACTIONS_H
#define _RATPOISON_ACTIONS_H 1

#include "ratpoison.h"

/* The structure returned by a command. */
typedef struct cmdret
{
  char *output;
  int success;
} cmdret;

void clear_frame_undos (void);
rp_keymap *find_keymap (char *name);
void init_user_commands(void);
void initialize_default_keybindings (void);
cmdret *command (int interactive, char *data);
cmdret *cmdret_new (int success, char *fmt, ...);
void cmdret_free (cmdret *ret);
void free_user_commands (void);
void free_aliases (void);
void free_keymaps (void);
char *wingravity_to_string (int g);
rp_action* find_keybinding (KeySym keysym, unsigned int state, rp_keymap *map);
rp_action* find_keybinding_by_action (char *action, rp_keymap *map);


#endif /* ! _RATPOISON_ACTIONS_H */
