/* Prototypes of all actions that can be performed with keystrokes.
 * Copyright (C) 2000, 2001, 2002, 2003 Shawn Betts
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
  char * (*func)(int, char *);
  int argtype;
};

int spawn(char *data);
char * command (int interactive, char *data);

char *cmd_abort (int interactive, char *data);
char *cmd_addhook (int interactive, char *data);
char *cmd_alias (int interactive, char *data);
char *cmd_banish (int interactive, char *data);
char *cmd_bind (int interactive, char *data);
char *cmd_chdir (int interactive, char *data);
char *cmd_clrunmanaged (int interactive, char *data);
char *cmd_colon (int interactive, char *data);
char *cmd_curframe (int interactive, char *data);
char *cmd_defbarborder (int interactive, char *data);
char *cmd_defbargravity (int interactive, char *data);
char *cmd_defbarpadding (int interactive, char *data);
char *cmd_defbgcolor (int interactive, char *data);
char *cmd_defborder (int interactive, char *data);
char *cmd_deffgcolor (int interactive, char *data);
char *cmd_deffont (int interactive, char *data);
char *cmd_definputwidth (int interactive, char *data);
char *cmd_defmaxsizegravity (int interactive, char *data);
char *cmd_defpadding (int interactive, char *data);
char *cmd_defresizeunit (int interactive, char *data);
char *cmd_deftransgravity (int interactive, char *data);
char *cmd_defwaitcursor (int interactive, char *data);
char *cmd_defwinfmt (int interactive, char *data);
char *cmd_defwingravity (int interactive, char *data);
char *cmd_defwinliststyle (int interactive, char *data);
char *cmd_defwinname (int interactive, char *data);
char *cmd_delete (int interactive, char *data);
char *cmd_echo (int interactive, char *data);
char *cmd_escape (int interactive, char *data);
char *cmd_exec (int interactive, char *data);
char *cmd_fdump (int interactively, char *data);
char *cmd_focusdown (int interactive, char *data);
char *cmd_focuslast (int interactive, char *data);
char *cmd_focusleft (int interactive, char *data);
char *cmd_focusright (int interactive, char *data);
char *cmd_focusup (int interactive, char *data);
char *cmd_frestore (int interactively, char *data);
char *cmd_fselect (int interactive, char *data);
char *cmd_gdelete (int interactive, char *data);
char *cmd_getenv (int interactive, char *data);
char *cmd_gmerge (int interactive, char *data);
char *cmd_gmove (int interactive, char *data);
char *cmd_gnew (int interactive, char *data);
char *cmd_gnewbg (int interactive, char *data);
char *cmd_gnext (int interactive, char *data);
char *cmd_gprev (int interactive, char *data);
char *cmd_gravity (int interactive, char *data);
char *cmd_groups (int interactive, char *data);
char *cmd_gselect (int interactive, char *data);
char *cmd_h_split (int interactive, char *data);
char *cmd_help (int interactive, char *data);
char *cmd_info (int interactive, char *data);
char *cmd_kill (int interactive, char *data);
char *cmd_last (int interactive, char *data);
char *cmd_lastmsg (int interactive, char *data);
char *cmd_license (int interactive, char *data);
char *cmd_link (int interactive, char *data);
char *cmd_listhook (int interactive, char *data);
char *cmd_meta (int interactive, char *data);
char *cmd_msgwait (int interactive, char *data);
char *cmd_newwm(int interactive, char *which);
char *cmd_next (int interactive, char *data);
char *cmd_next_frame (int interactive, char *data);
char *cmd_nextscreen (int interactive, char *data);
char *cmd_number (int interactive, char *data);
char *cmd_only (int interactive, char *data);
char *cmd_other (int interactive, char *data);
char *cmd_prev (int interactive, char *data);
char *cmd_prev_frame (int interactive, char *data);
char *cmd_prevscreen (int interactive, char *data);
char *cmd_quit(int interactive, char *data);
char *cmd_redisplay (int interactive, char *data);
char *cmd_remhook (int interactive, char *data);
char *cmd_remove (int interactive, char *data);
char *cmd_rename (int interactive, char *data);
char *cmd_resize (int interactive, char *data);
char *cmd_restart (int interactive, char *data);
char *cmd_rudeness (int interactive, char *data);
char *cmd_select (int interactive, char *data);
char *cmd_setenv (int interactive, char *data);
char *cmd_shrink (int interactive, char *data);
char *cmd_source (int interactive, char *data);
char *cmd_startup_message (int interactive, char *data);
char *cmd_time (int interactive, char *data);
char *cmd_tmpwm (int interactive, char *data);
char *cmd_togglewrapwinlist ();
char *cmd_unalias (int interactive, char *data);
char *cmd_unbind (int interactive, char *data);
char *cmd_unimplemented (int interactive, char *data);
char *cmd_unmanage (int interactive, char *data);
char *cmd_unsetenv (int interactive, char *data);
char *cmd_v_split (int interactive, char *data);
char *cmd_verbexec (int interactive, char *data);
char *cmd_version (int interactive, char *data);
char *cmd_warp(int interactive, char *data);
char *cmd_windows (int interactive, char *data);
char *cmd_readkey (int interactive, char *data);
char *cmd_newkmap (int interactive, char *data);
char *cmd_delkmap (int interactive, char *data);
char *cmd_definekey (int interactive, char *data);
char *cmd_defframesels (int interactive, char *data);

rp_keymap *find_keymap (char *name);
void initialize_default_keybindings (void);
void keymap_free (rp_keymap *map);
void free_aliases ();
void free_keymaps ();
rp_action* find_keybinding (KeySym keysym, int state, rp_keymap *map);
rp_action* find_keybinding_by_action (char *action, rp_keymap *map);


#endif /* ! _RATPOISON_ACTIONS_H */
