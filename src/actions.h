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

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS_LENGTH 100

#include "ratpoison.h"

/* arg_REST and arg_SHELLCMD eat the rest of the input. */
enum argtype { arg_REST,
               arg_NUMBER,
               arg_STRING,
               arg_FRAME,
               arg_WINDOW,
               arg_COMMAND,
               arg_SHELLCMD,
               arg_KEYMAP,
               arg_KEY,
               arg_GRAVITY,
               arg_GROUP,
               arg_HOOK,
               arg_VARIABLE,
               arg_RAW};

union arg_union {
    rp_frame *frame;
    int number;
    float fnumber;
    rp_window *win;
    rp_keymap *keymap;
    rp_group *group;
    struct list_head *hook;
    struct set_var *variable;
    struct rp_key *key;
    int gravity;
  };

struct cmdarg
{
  int type;
  char *string;
  union arg_union arg;
  struct list_head node;
};

struct argspec
{
  int type;
  char *prompt;
};

/* The structure returned by a command. */
typedef struct cmdret cmdret;
struct cmdret
{
  char *output;
  int success;
};

void del_frame_undo (rp_frame_undo *u);

rp_keymap *find_keymap (char *name);
void init_user_commands(void);
void initialize_default_keybindings (void);
cmdret *command (int interactive, char *data);
cmdret *cmdret_new (int success, char *fmt, ...);
void cmdret_free (cmdret *ret);
void keymap_free (rp_keymap *map);
void free_user_commands (void);
void free_aliases (void);
void free_keymaps (void);
char *wingravity_to_string (int g);
rp_action* find_keybinding (KeySym keysym, unsigned int state, rp_keymap *map);
rp_action* find_keybinding_by_action (char *action, rp_keymap *map);


#endif /* ! _RATPOISON_ACTIONS_H */
