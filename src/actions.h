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
char * cmd_meta (int interactive, void *data);
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
char * cmd_time (int interactive, void *data);
char * cmd_version (int interactive, void *data);
char * cmd_unimplemented (int interactive, void *data);
char * cmd_bind (int interactive, void* data);
char * cmd_source (int interactive, void* data);
char * cmd_redisplay (int interactive, void *data);
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
char * cmd_unbind (int interactive, void *data);
char * cmd_gravity (int interactive, void *data);
char * cmd_defwingravity (int interactive, void *data);
char * cmd_deftransgravity (int interactive, void *data);
char * cmd_defmaxsizegravity (int interactive, void *data);
char * cmd_msgwait (int interactive, void *data);
char * cmd_defbarloc (int interactive, void *data);
char * cmd_deffont (int interactive, void *data);
char * cmd_defpadding (int interactive, void *data);
char * cmd_defborder (int interactive, void *data);
char * cmd_definputwidth (int interactive, void *data);
char * cmd_defwaitcursor (int interactive, void *data);
char * cmd_defwinfmt (int interactive, void *data);
char * cmd_defwinname (int interactive, void *data);
char * cmd_deffgcolor (int interactive, void *data);
char * cmd_defbgcolor (int interactive, void *data);
char * cmd_setenv (int interactive, void *data);
char * cmd_chdir (int interactive, void *data);
char * cmd_unsetenv (int interactive, void *data);
char * cmd_info (int interactive, void *data);
char * cmd_lastmsg (int interactive, void *data);
char * cmd_focusup (int interactive, void *data);
char * cmd_focusdown (int interactive, void *data);
char * cmd_focusleft (int interactive, void *data);
char * cmd_focusright (int interactive, void *data);
char * cmd_restart (int interactive, void *data);
char * cmd_startup_message (int interactive, void *data);

/* void cmd_xterm (void *data); */

void initialize_default_keybindings (void);
rp_action* find_keybinding (KeySym keysym, int state);
rp_action* find_keybinding_by_action (char *action);

#endif /* ! _RATPOISON_ACTIONS_H */
