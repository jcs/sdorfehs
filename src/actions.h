/* Prototypes of all actions that can be performed with keystrokes.
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

#ifndef _RATPOISON_ACTIONS_H
#define _RATPOISON_ACTIONS_H 1

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS_LENGTH 100

typedef struct user_command user_command;

enum argtype { arg_VOID, arg_STRING, arg_NUMBER };

struct
user_command
{
  char *name;
  void (*func)(void *);
  int argtype;
};

void bye(void *dummy);
void spawn(void *data);
void command (char *data);
void maximize (void *data);
void force_maximize (void *data);

void cmd_newwm(void *which);
void cmd_generate (void *data);
void cmd_abort (void *data);
void cmd_exec (void *data);
void cmd_colon (void *data);
void cmd_kill (void *data);
void cmd_delete (void *data);
void cmd_rename (void *data);
void cmd_select (void *data);
void cmd_last (void *data);
void cmd_next (void *data);
void cmd_prev (void *data);
void cmd_windows (void *data);
void cmd_other (void *data);
void cmd_clock (void *data);
void cmd_version (void *data);
void cmd_unimplemented (void *data);
void cmd_bind (void* data);
void cmd_source (void* data);

/* void cmd_xterm (void *data); */

void initialize_default_keybindings (void);
rp_action* find_keybinding (int keysym, int state);

struct key
{
  long sym;
  long state;
};

#endif /* ! _RATPOISON_ACTIONS_H */
