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
  char * (*func)(int, void *);
  int argtype;
};

void spawn(void *data);
char * command (int interactive, char *data);

char * cmd_newwm(int interactive, void *which);
char * cmd_generate (int interactive, void *data);
char * cmd_abort (int interactive, void *data);
char * cmd_exec (int interactive, void *data);
char * cmd_colon (int interactive, void *data);
char * cmd_kill (int interactive, void *data);
char * cmd_delete (int interactive, void *data);
char * cmd_rename (int interactive, void *data);
char * cmd_select (int interactive, void *data);
char * cmd_last (int interactive, void *data);
char * cmd_next (int interactive, void *data);
char * cmd_next_frame (int interactive, void *data);
char * cmd_prev (int interactive, void *data);
char * cmd_prev_frame (int interactive, void *data);
char * cmd_windows (int interactive, void *data);
char * cmd_other (int interactive, void *data);
char * cmd_clock (int interactive, void *data);
char * cmd_version (int interactive, void *data);
char * cmd_unimplemented (int interactive, void *data);
char * cmd_bind (int interactive, void* data);
char * cmd_source (int interactive, void* data);
char * cmd_maximize (int interactive, void *data);
char * cmd_escape (int interactive, void *data);
char * cmd_echo (int interactive, void *data);
char * cmd_h_split (int interactive, void *data);
char * cmd_v_split (int interactive, void *data);
char * cmd_only (int interactive, void *data);
char * cmd_remove (int interactive, void *data);
char * cmd_banish (int interactive, void *data);
char * cmd_curframe (int interactive, void *data);
char * cmd_help (int interactive, void *data);
char * cmd_quit(int interactive, void *data);
char * cmd_number (int interactive, void *data);
char * cmd_rudeness (int interactive, void *data);

/* void cmd_xterm (void *data); */

void initialize_default_keybindings (void);
rp_action* find_keybinding (int keysym, int state);

#endif /* ! _RATPOISON_ACTIONS_H */
