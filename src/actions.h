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

#include <ratpoison.h>

typedef struct user_command user_command;

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

struct
user_command
{
  char *name;
  cmdret * (*func)(int, struct cmdarg **);
  struct argspec *args;
  int num_args;
  /* The number of required arguments. Any arguments after that are
     optional and won't be filled in when called
     interactively. ni_required_args is used when called non-interactively,
     i_required_args when called interactively. */
  int ni_required_args, i_required_args;

  struct list_head node;
};

int spawn(char *data, int raw);
cmdret *command (int interactive, char *data);

/* command function prototypes. */
#define RP_CMD(cmd) cmdret *cmd_ ## cmd (int interactive, struct cmdarg **args)
RP_CMD (abort);
RP_CMD (addhook);
RP_CMD (alias);
RP_CMD (banish);
RP_CMD (bind);
RP_CMD (compat);
RP_CMD (chdir);
RP_CMD (clrunmanaged);
RP_CMD (colon);
RP_CMD (curframe);
RP_CMD (delete);
RP_CMD (echo);
RP_CMD (escape);
RP_CMD (exec);
RP_CMD (fdump);
RP_CMD (focusdown);
RP_CMD (focuslast);
RP_CMD (focusleft);
RP_CMD (focusright);
RP_CMD (focusup);
RP_CMD (frestore);
RP_CMD (fselect);
RP_CMD (gdelete);
RP_CMD (getenv);
RP_CMD (gmerge);
RP_CMD (gmove);
RP_CMD (gnew);
RP_CMD (gnewbg);
RP_CMD (gnext);
RP_CMD (gprev);
RP_CMD (gravity);
RP_CMD (groups);
RP_CMD (gselect);
RP_CMD (h_split);
RP_CMD (help);
RP_CMD (info);
RP_CMD (kill);
RP_CMD (lastmsg);
RP_CMD (license);
RP_CMD (link);
RP_CMD (listhook);
RP_CMD (meta);
RP_CMD (msgwait);
RP_CMD (newwm);
RP_CMD (next);
RP_CMD (next_frame);
RP_CMD (nextscreen);
RP_CMD (number);
RP_CMD (only);
RP_CMD (other);
RP_CMD (prev);
RP_CMD (prev_frame);
RP_CMD (prevscreen);
RP_CMD (quit);
RP_CMD (redisplay);
RP_CMD (remhook);
RP_CMD (remove);
RP_CMD (rename);
RP_CMD (resize);
RP_CMD (restart);
RP_CMD (rudeness);
RP_CMD (select);
RP_CMD (setenv);
RP_CMD (shrink);
RP_CMD (source);
RP_CMD (startup_message);
RP_CMD (time);
RP_CMD (tmpwm);
RP_CMD (unalias);
RP_CMD (unbind);
RP_CMD (unimplemented);
RP_CMD (unmanage);
RP_CMD (unsetenv);
RP_CMD (v_split);
RP_CMD (verbexec);
RP_CMD (version);
RP_CMD (warp);
RP_CMD (windows);
RP_CMD (readkey);
RP_CMD (newkmap);
RP_CMD (delkmap);
RP_CMD (definekey);
RP_CMD (undefinekey);
RP_CMD (set);
RP_CMD (sselect);
RP_CMD (ratwarp);
RP_CMD (ratclick);
RP_CMD (ratrelwarp);
RP_CMD (rathold);
RP_CMD (cnext);
RP_CMD (cother);
RP_CMD (cprev);
RP_CMD (dedicate);
RP_CMD (describekey);
RP_CMD (inext);
RP_CMD (iother);
RP_CMD (iprev);
RP_CMD (prompt);
RP_CMD (sdump);
RP_CMD (sfdump);
RP_CMD (undo);
RP_CMD (redo);
RP_CMD (putsel);
RP_CMD (getsel);

void del_frame_undo (rp_frame_undo *u);

rp_keymap *find_keymap (char *name);
void init_user_commands();
void initialize_default_keybindings (void);
void cmdret_free (cmdret *ret);
void keymap_free (rp_keymap *map);
void free_aliases ();
void free_keymaps ();
char *wingravity_to_string (int g);
rp_action* find_keybinding (KeySym keysym, int state, rp_keymap *map);
rp_action* find_keybinding_by_action (char *action, rp_keymap *map);


#endif /* ! _RATPOISON_ACTIONS_H */
