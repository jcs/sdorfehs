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

void switch_to(void *which);
void bye(void *dummy);
void generate_prefix (void *data);
void generate_key_event (void *data);
void abort_keypress (void *data);
void goto_window_number (void* data);
void spawn(void *data);
void shell_command (void *data);
void command (void *data);
void command (void *data);
void kill_window (void *data);
void delete_window (void *data);
void rename_current_window (void *data);
void goto_win_by_name (void *data);
void last_window (void *data);
/* void next_window (void *data); */
/* void prev_window (void *data); */
void toggle_bar (void *data);
void maximize (void *data);
void show_clock (void *data);
void show_version (void *data);
void xterm_command (void *data);

extern rp_action key_actions[];

#endif /* ! _RATPOISON_ACTIONS_H */
