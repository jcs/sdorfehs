/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <X11/keysym.h>
#include <string.h>
#include <strings.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <X11/Xproto.h>
#include <X11/extensions/XTest.h>
#include <sys/ioctl.h>

#include "sdorfehs.h"

/* arg_REST and arg_SHELLCMD eat the rest of the input. */
enum argtype {
	arg_REST,
	arg_NUMBER,
	arg_STRING,
	arg_FRAME,
	arg_WINDOW,
	arg_COMMAND,
	arg_SHELLCMD,
	arg_KEYMAP,
	arg_KEY,
	arg_GRAVITY,
	arg_VSCREEN,
	arg_HOOK,
	arg_VARIABLE,
	arg_RAW,
};

union arg_union {
	rp_frame *frame;
	int number;
	float fnumber;
	rp_window *win;
	rp_keymap *keymap;
	rp_vscreen *vscreen;
	struct list_head *hook;
	struct set_var *variable;
	struct rp_key *key;
	int gravity;
};

struct cmdarg {
	int type;
	char *string;
	union arg_union arg;
	struct list_head node;
};
#define ARG_STRING(elt) args[elt]->string
#define ARG(elt, type)  args[elt]->arg.type

struct argspec {
	int type;
	char *prompt;
};

struct set_var {
	char *var;
	cmdret *(*set_fn)(struct cmdarg **);
	int nargs;
	struct argspec *args;
	struct list_head node;
};

struct user_command {
	char *name;
	cmdret *(*func)(int, struct cmdarg **);
	struct argspec *args;
	int num_args;
	/*
	 * The number of required arguments. Any arguments after that are
	 * optional and won't be filled in when called interactively.
	 * ni_required_args is used when called non-interactively,
	 * i_required_args when called interactively.
	 */
	int ni_required_args, i_required_args;

	struct list_head node;
};

typedef struct {
	char *name;
	char *alias;
} alias_t;

typedef struct rp_frame_undo {
	char *frames;
	rp_vscreen *vscreen;
	struct list_head node;
} rp_frame_undo;

static LIST_HEAD(user_commands);
static LIST_HEAD(rp_keymaps);
static LIST_HEAD(set_vars);
static LIST_HEAD(rp_frame_undos);
static LIST_HEAD(rp_frame_redos);

static alias_t *alias_list;
static int alias_list_size;
static int alias_list_last;

static const char invalid_negative_arg[] = "invalid negative argument";

/* setter function prototypes */
static cmdret *set_barborder(struct cmdarg **args);
static cmdret *set_barbordercolor(struct cmdarg **args);
static cmdret *set_bargravity(struct cmdarg **args);
static cmdret *set_barinpadding(struct cmdarg **args);
static cmdret *set_barpadding(struct cmdarg **args);
static cmdret *set_barsticky(struct cmdarg **args);
static cmdret *set_bgcolor(struct cmdarg **args);
static cmdret *set_border(struct cmdarg **args);
static cmdret *set_bwcolor(struct cmdarg **args);
static cmdret *set_fgcolor(struct cmdarg **args);
static cmdret *set_font(struct cmdarg **args);
static cmdret *set_framefmt(struct cmdarg **args);
static cmdret *set_framemsgwait(struct cmdarg **args);
static cmdret *set_framesels(struct cmdarg **args);
static cmdret *set_fwcolor(struct cmdarg **args);
static cmdret *set_gap(struct cmdarg **args);
static cmdret *set_historysize(struct cmdarg **args);
static cmdret *set_ignoreresizehints(struct cmdarg **args);
static cmdret *set_infofmt(struct cmdarg **args);
static cmdret *set_inputwidth(struct cmdarg **args);
static cmdret *set_maxsizegravity(struct cmdarg **args);
static cmdret *set_maxundos(struct cmdarg **args);
static cmdret *set_msgwait(struct cmdarg **args);
static cmdret *set_onlyborder(struct cmdarg **args);
static cmdret *set_padding(struct cmdarg **args);
static cmdret *set_resizefmt(struct cmdarg **args);
static cmdret *set_resizeunit(struct cmdarg **args);
static cmdret *set_rudeness(struct cmdarg **args);
static cmdret *set_startupmessage(struct cmdarg **args);
static cmdret *set_stickyfmt(struct cmdarg **args);
static cmdret *set_topkmap(struct cmdarg **args);
static cmdret *set_transgravity(struct cmdarg **args);
static cmdret *set_vscreens(struct cmdarg **args);
static cmdret *set_waitcursor(struct cmdarg **args);
static cmdret *set_warp(struct cmdarg **args);
static cmdret *set_winfmt(struct cmdarg **args);
static cmdret *set_wingravity(struct cmdarg **args);
static cmdret *set_winliststyle(struct cmdarg **args);
static cmdret *set_winname(struct cmdarg **args);
static cmdret *set_winaddcurvscreen(struct cmdarg **args);

/* command function prototypes. */
static cmdret *cmd_abort(int interactive, struct cmdarg **args);
static cmdret *cmd_addhook(int interactive, struct cmdarg **args);
static cmdret *cmd_alias(int interactive, struct cmdarg **args);
static cmdret *cmd_banish(int interactive, struct cmdarg **args);
static cmdret *cmd_banishrel(int interactive, struct cmdarg **args);
static cmdret *cmd_chdir(int interactive, struct cmdarg **args);
static cmdret *cmd_clrunmanaged(int interactive, struct cmdarg **args);
static cmdret *cmd_cnext(int interactive, struct cmdarg **args);
static cmdret *cmd_colon(int interactive, struct cmdarg **args);
static cmdret *cmd_commands(int interactive, struct cmdarg **args);
static cmdret *cmd_cother(int interactive, struct cmdarg **args);
static cmdret *cmd_cprev(int interactive, struct cmdarg **args);
static cmdret *cmd_curframe(int interactive, struct cmdarg **args);
static cmdret *cmd_dedicate(int interactive, struct cmdarg **args);
static cmdret *cmd_definekey(int interactive, struct cmdarg **args);
static cmdret *cmd_delete(int interactive, struct cmdarg **args);
static cmdret *cmd_delkmap(int interactive, struct cmdarg **args);
static cmdret *cmd_describekey(int interactive, struct cmdarg **args);
static cmdret *cmd_echo(int interactive, struct cmdarg **args);
static cmdret *cmd_escape(int interactive, struct cmdarg **args);
static cmdret *cmd_exchangedown(int interactive, struct cmdarg **args);
static cmdret *cmd_exchangeleft(int interactive, struct cmdarg **args);
static cmdret *cmd_exchangeright(int interactive, struct cmdarg **args);
static cmdret *cmd_exchangeup(int interactive, struct cmdarg **args);
static cmdret *cmd_exec(int interactive, struct cmdarg **args);
static cmdret *cmd_execa(int interactive, struct cmdarg **args);
static cmdret *cmd_execf(int interactive, struct cmdarg **args);
static cmdret *cmd_fdump(int interactive, struct cmdarg **args);
static cmdret *cmd_focusdown(int interactive, struct cmdarg **args);
static cmdret *cmd_focuslast(int interactive, struct cmdarg **args);
static cmdret *cmd_focusleft(int interactive, struct cmdarg **args);
static cmdret *cmd_focusright(int interactive, struct cmdarg **args);
static cmdret *cmd_focusup(int interactive, struct cmdarg **args);
static cmdret *cmd_frestore(int interactive, struct cmdarg **args);
static cmdret *cmd_fselect(int interactive, struct cmdarg **args);
static cmdret *cmd_getenv(int interactive, struct cmdarg **args);
static cmdret *cmd_getsel(int interactive, struct cmdarg **args);
static cmdret *cmd_gravity(int interactive, struct cmdarg **args);
static cmdret *cmd_hsplit(int interactive, struct cmdarg **args);
static cmdret *cmd_help(int interactive, struct cmdarg **args);
static cmdret *cmd_inext(int interactive, struct cmdarg **args);
static cmdret *cmd_info(int interactive, struct cmdarg **args);
static cmdret *cmd_iother(int interactive, struct cmdarg **args);
static cmdret *cmd_iprev(int interactive, struct cmdarg **args);
static cmdret *cmd_kill(int interactive, struct cmdarg **args);
static cmdret *cmd_lastmsg(int interactive, struct cmdarg **args);
static cmdret *cmd_link(int interactive, struct cmdarg **args);
static cmdret *cmd_listhook(int interactive, struct cmdarg **args);
static cmdret *cmd_meta(int interactive, struct cmdarg **args);
static cmdret *cmd_newkmap(int interactive, struct cmdarg **args);
static cmdret *cmd_next(int interactive, struct cmdarg **args);
static cmdret *cmd_nextframe(int interactive, struct cmdarg **args);
static cmdret *cmd_nextscreen(int interactive, struct cmdarg **args);
static cmdret *cmd_number(int interactive, struct cmdarg **args);
static cmdret *cmd_only(int interactive, struct cmdarg **args);
static cmdret *cmd_other(int interactive, struct cmdarg **args);
static cmdret *cmd_prev(int interactive, struct cmdarg **args);
static cmdret *cmd_prevframe(int interactive, struct cmdarg **args);
static cmdret *cmd_prevscreen(int interactive, struct cmdarg **args);
static cmdret *cmd_prompt(int interactive, struct cmdarg **args);
static cmdret *cmd_putsel(int interactive, struct cmdarg **args);
static cmdret *cmd_quit(int interactive, struct cmdarg **args);
static cmdret *cmd_ratclick(int interactive, struct cmdarg **args);
static cmdret *cmd_rathold(int interactive, struct cmdarg **args);
static cmdret *cmd_ratinfo(int interactive, struct cmdarg **args);
static cmdret *cmd_ratrelinfo(int interactive, struct cmdarg **args);
static cmdret *cmd_ratrelwarp(int interactive, struct cmdarg **args);
static cmdret *cmd_ratwarp(int interactive, struct cmdarg **args);
static cmdret *cmd_readkey(int interactive, struct cmdarg **args);
static cmdret *cmd_redisplay(int interactive, struct cmdarg **args);
static cmdret *cmd_redo(int interactive, struct cmdarg **args);
static cmdret *cmd_remhook(int interactive, struct cmdarg **args);
static cmdret *cmd_remove(int interactive, struct cmdarg **args);
static cmdret *cmd_rename(int interactive, struct cmdarg **args);
static cmdret *cmd_resize(int interactive, struct cmdarg **args);
static cmdret *cmd_restart(int interactive, struct cmdarg **args);
static cmdret *cmd_sdump(int interactive, struct cmdarg **args);
static cmdret *cmd_select(int interactive, struct cmdarg **args);
static cmdret *cmd_set(int interactive, struct cmdarg **args);
static cmdret *cmd_setenv(int interactive, struct cmdarg **args);
static cmdret *cmd_sfdump(int interactive, struct cmdarg **args);
static cmdret *cmd_sfrestore(int interactive, struct cmdarg **args);
static cmdret *cmd_shrink(int interactive, struct cmdarg **args);
static cmdret *cmd_smove(int interactive, struct cmdarg **args);
static cmdret *cmd_source(int interactive, struct cmdarg **args);
static cmdret *cmd_sselect(int interactive, struct cmdarg **args);
static cmdret *cmd_stick(int interactive, struct cmdarg **args);
static cmdret *cmd_swap(int interactive, struct cmdarg **args);
static cmdret *cmd_unalias(int interactive, struct cmdarg **args);
static cmdret *cmd_undefinekey(int interactive, struct cmdarg **args);
static cmdret *cmd_undo(int interactive, struct cmdarg **args);
static cmdret *cmd_unmanage(int interactive, struct cmdarg **args);
static cmdret *cmd_unsetenv(int interactive, struct cmdarg **args);
static cmdret *cmd_unstick(int interactive, struct cmdarg **args);
static cmdret *cmd_vsplit(int interactive, struct cmdarg **args);
static cmdret *cmd_verbexec(int interactive, struct cmdarg **args);
static cmdret *cmd_version(int interactive, struct cmdarg **args);
static cmdret *cmd_vmove(int interactive, struct cmdarg **args);
static cmdret *cmd_vnext(int interactive, struct cmdarg **args);
static cmdret *cmd_vother(int interactive, struct cmdarg **args);
static cmdret *cmd_vprev(int interactive, struct cmdarg **args);
static cmdret *cmd_vrename(int interactive, struct cmdarg **args);
static cmdret *cmd_vscreens(int interactive, struct cmdarg **args);
static cmdret *cmd_vselect(int interactive, struct cmdarg **args);
static cmdret *cmd_windows(int interactive, struct cmdarg **args);

static void
add_set_var(char *name, cmdret *(*fn)(struct cmdarg **), int nargs, ...)
{
	int i = 0;
	struct set_var *var;
	va_list va;

	var = xmalloc(sizeof(struct set_var));
	var->var = name;
	var->set_fn = fn;
	var->nargs = nargs;
	var->args = xmalloc(sizeof(struct argspec) * nargs);

	/* Fill var->args */
	va_start(va, nargs);
	for (i = 0; i < nargs; i++) {
		var->args[i].prompt = va_arg(va, char *);
		var->args[i].type = va_arg(va, int);
	}
	va_end(va);

	list_add_tail(&var->node, &set_vars);
}

static void
set_var_free(struct set_var *var)
{
	if (var == NULL)
		return;

	free(var->args);
	free(var);
}

static void
init_set_vars(void)
{
	/* Keep this sorted alphabetically. */
	add_set_var("barborder", set_barborder, 1, "", arg_NUMBER);
	add_set_var("barbordercolor", set_barbordercolor, 1, "", arg_STRING);
	add_set_var("bargravity", set_bargravity, 1, "", arg_GRAVITY);
	add_set_var("barinpadding", set_barinpadding, 1, "", arg_NUMBER);
	add_set_var("barpadding", set_barpadding, 2, "", arg_NUMBER, "",
	    arg_NUMBER);
	add_set_var("barsticky", set_barsticky, 1, "", arg_NUMBER);
	add_set_var("bgcolor", set_bgcolor, 1, "", arg_STRING);
	add_set_var("border", set_border, 1, "", arg_NUMBER);
	add_set_var("bwcolor", set_bwcolor, 1, "", arg_STRING);
	add_set_var("fgcolor", set_fgcolor, 1, "", arg_STRING);
	add_set_var("font", set_font, 1, "", arg_STRING);
	add_set_var("framefmt", set_framefmt, 1, "", arg_REST);
	add_set_var("framemsgwait", set_framemsgwait, 1, "", arg_NUMBER);
	add_set_var("framesels", set_framesels, 1, "", arg_STRING);
	add_set_var("fwcolor", set_fwcolor, 1, "", arg_STRING);
	add_set_var("gap", set_gap, 1, "", arg_NUMBER);
	add_set_var("historysize", set_historysize, 1, "", arg_NUMBER);
	add_set_var("ignoreresizehints", set_ignoreresizehints, 1, "",
	    arg_NUMBER);
	add_set_var("infofmt", set_infofmt, 1, "", arg_REST);
	add_set_var("inputwidth", set_inputwidth, 1, "", arg_NUMBER);
	add_set_var("maxsizegravity", set_maxsizegravity, 1, "", arg_GRAVITY);
	add_set_var("maxundos", set_maxundos, 1, "", arg_NUMBER);
	add_set_var("msgwait", set_msgwait, 1, "", arg_NUMBER);
	add_set_var("onlyborder", set_onlyborder, 1, "", arg_NUMBER);
	add_set_var("padding", set_padding, 4, "", arg_NUMBER, "", arg_NUMBER,
	    "", arg_NUMBER, "", arg_NUMBER);
	add_set_var("resizefmt", set_resizefmt, 1, "", arg_REST);
	add_set_var("resizeunit", set_resizeunit, 1, "", arg_NUMBER);
	add_set_var("rudeness", set_rudeness, 1, "", arg_NUMBER);
	add_set_var("startupmessage", set_startupmessage, 1, "", arg_NUMBER);
	add_set_var("stickyfmt", set_stickyfmt, 1, "", arg_REST);
	add_set_var("topkmap", set_topkmap, 1, "", arg_STRING);
	add_set_var("transgravity", set_transgravity, 1, "", arg_GRAVITY);
	add_set_var("vscreens", set_vscreens, 1, "", arg_NUMBER);
	add_set_var("waitcursor", set_waitcursor, 1, "", arg_NUMBER);
	add_set_var("warp", set_warp, 1, "", arg_NUMBER);
	add_set_var("winfmt", set_winfmt, 1, "", arg_REST);
	add_set_var("wingravity", set_wingravity, 1, "", arg_GRAVITY);
	add_set_var("winliststyle", set_winliststyle, 1, "", arg_STRING);
	add_set_var("winname", set_winname, 1, "", arg_STRING);
	add_set_var("winaddcurvscreen", set_winaddcurvscreen, 1, "", arg_NUMBER);
}

/*
 * i_nrequired is the number required when called interactively. ni_nrequired
 * is when called non-interactively.
 */
static void
add_command(char *name, cmdret *(*fn)(int, struct cmdarg **), int nargs,
    int i_nrequired, int ni_nrequired, ...)
{
	int i = 0;
	struct user_command *cmd;
	va_list va;

	cmd = xmalloc(sizeof(struct user_command));
	cmd->name = name;
	cmd->func = fn;
	cmd->num_args = nargs;
	cmd->ni_required_args = ni_nrequired;
	cmd->i_required_args = i_nrequired;
	cmd->args = nargs ? xmalloc(nargs * sizeof(struct argspec)) : NULL;

	/* Fill cmd->args */
	va_start(va, ni_nrequired);
	for (i = 0; i < nargs; i++) {
		cmd->args[i].prompt = va_arg(va, char *);
		cmd->args[i].type = va_arg(va, int);
	}
	va_end(va);

	list_add(&cmd->node, &user_commands);
}

static void
user_command_free(struct user_command *cmd)
{
	if (cmd == NULL)
		return;

	free(cmd->args);
	free(cmd);
}

void
init_user_commands(void)
{
	/* @begin (tag required for genrpbindings) */
	add_command("abort",		cmd_abort,	0, 0, 0);
	add_command("addhook",		cmd_addhook,	2, 2, 2,
	            "Hook: ", arg_HOOK,
	            "Command: ", arg_COMMAND);
	add_command("alias",		cmd_alias,	2, 2, 2,
	            "Alias: ", arg_STRING,
	            "Command: ", arg_COMMAND);
	add_command("banish",		cmd_banish,	0, 0, 0);
	add_command("chdir",		cmd_chdir,	1, 0, 0,
	            "Dir: ", arg_REST);
	add_command("clrunmanaged",	cmd_clrunmanaged, 0, 0, 0);
	add_command("cnext",		cmd_cnext,	0, 0, 0);
	add_command("colon",		cmd_colon,	1, 0, 0,
	            "", arg_REST);
	add_command("commands",		cmd_commands,	0, 0, 0);
	add_command("cother",		cmd_cother,	0, 0, 0);
	add_command("cprev",		cmd_cprev,	0, 0, 0);
	add_command("curframe",		cmd_curframe,	0, 0, 0);
	add_command("dedicate",		cmd_dedicate,	1, 0, 0,
                    "", arg_NUMBER);
	add_command("definekey",	cmd_definekey,	3, 3, 3,
	            "Keymap: ", arg_KEYMAP,
	            "Key: ", arg_KEY,
	            "Command: ", arg_COMMAND);
	add_command("delete",		cmd_delete,	0, 0, 0);
	add_command("delkmap",		cmd_delkmap,	1, 1, 1,
	            "Keymap: ", arg_KEYMAP);
	add_command("describekey",	cmd_describekey, 1, 1, 1,
                    "Keymap: ", arg_KEYMAP);
	add_command("echo",		cmd_echo,	1, 1, 1,
	            "Echo: ", arg_RAW);
	add_command("escape",		cmd_escape,	1, 1, 1,
	            "Key: ", arg_KEY);
	add_command("exchangedown",	cmd_exchangedown, 0, 0, 0);
	add_command("exchangeleft",	cmd_exchangeleft, 0, 0, 0);
	add_command("exchangeright",	cmd_exchangeright, 0, 0, 0);
	add_command("exchangeup",	cmd_exchangeup,	0, 0, 0);
	add_command("exec",		cmd_exec,	1, 1, 1,
	            "/bin/sh -c ", arg_SHELLCMD);
	add_command("execa",		cmd_execa,	1, 1, 1,
	            "/bin/sh -c ", arg_SHELLCMD);
	add_command("execf",		cmd_execf,	2, 2, 2,
	            "frame to execute in:", arg_FRAME,
	            "/bin/sh -c ", arg_SHELLCMD);
	add_command("fdump",		cmd_fdump,	1, 0, 0,
	            "", arg_NUMBER);
	add_command("focus",		cmd_nextframe,	0, 0, 0);
	add_command("focusdown",	cmd_focusdown,	0, 0, 0);
	add_command("focusprev",	cmd_prevframe,	0, 0, 0);
	add_command("focuslast",	cmd_focuslast,	0, 0, 0);
	add_command("focusleft",	cmd_focusleft,	0, 0, 0);
	add_command("focusright",	cmd_focusright,	0, 0, 0);
	add_command("focusup",		cmd_focusup,	0, 0, 0);
	add_command("frestore",		cmd_frestore,	1, 1, 1,
	            "Frames: ", arg_REST);
	add_command("fselect",		cmd_fselect,	1, 1, 1,
	            "", arg_FRAME);
	add_command("getenv",		cmd_getenv,	1, 1, 1,
	            "Variable: ", arg_STRING);
	add_command("getsel",		cmd_getsel,	0, 0, 0);
	add_command("gravity",		cmd_gravity,	1, 0, 0,
	            "Gravity: ", arg_GRAVITY);
	add_command("help",		cmd_help,	1, 0, 0,
	            "Keymap: ", arg_KEYMAP);
	add_command("hsplit",		cmd_hsplit,	1, 0, 0,
	            "Split: ", arg_STRING);
	add_command("inext",		cmd_inext,	0, 0, 0);
	add_command("info",		cmd_info,	1, 0, 0,
	            "Format: ", arg_REST);
	add_command("iother",		cmd_iother,	0, 0, 0);
	add_command("iprev",		cmd_iprev,	0, 0, 0);
	add_command("kill",		cmd_kill,	0, 0, 0);
	add_command("lastmsg",		cmd_lastmsg,	0, 0, 0);
	add_command("link",		cmd_link,	2, 1, 1,
	            "Key: ", arg_STRING,
	            "Keymap: ", arg_KEYMAP);
	add_command("listhook",		cmd_listhook,	1, 1, 1,
                    "Hook: ", arg_HOOK);
	add_command("meta",		cmd_meta,	1, 0, 0,
                    "key: ", arg_KEY);
	add_command("newkmap",		cmd_newkmap,	1, 1, 1,
                    "Keymap: ", arg_STRING);
	add_command("next",		cmd_next,	0, 0, 0);
	add_command("nextscreen",	cmd_nextscreen,	0, 0, 0);
	add_command("number",		cmd_number,	2, 1, 1,
                    "Number: ", arg_NUMBER,
                    "Number: ", arg_NUMBER);
	add_command("only",		cmd_only,	0, 0, 0);
	add_command("other",		cmd_other,	0, 0, 0);
	add_command("prev",		cmd_prev,	0, 0, 0);
	add_command("prevscreen",	cmd_prevscreen,	0, 0, 0);
	add_command("prompt",		cmd_prompt,	1, 0, 0,
                    "", arg_REST);
	add_command("putsel",		cmd_putsel,	1, 1, 1,
                    "Text: ", arg_RAW);
	add_command("quit",		cmd_quit,	0, 0, 0);
	add_command("ratinfo",		cmd_ratinfo,	0, 0, 0);
	add_command("ratrelinfo",	cmd_ratrelinfo,	0, 0, 0);
	add_command("banishrel",	cmd_banishrel,	0, 0, 0);
	add_command("ratwarp",		cmd_ratwarp,	2, 2, 2,
                    "X: ", arg_NUMBER,
                    "Y: ", arg_NUMBER);
	add_command("ratrelwarp",	cmd_ratrelwarp,	2, 2, 2,
                    "X: ", arg_NUMBER,
                    "Y: ", arg_NUMBER);
	add_command("ratclick",		cmd_ratclick,	1, 0, 0,
                    "Button: ", arg_NUMBER);
	add_command("rathold",		cmd_rathold,	2, 1, 1,
                    "State: ", arg_STRING,
                    "Button: ", arg_NUMBER);
	add_command("readkey",		cmd_readkey,	1, 1, 1,
                    "Keymap: ", arg_KEYMAP);
	add_command("redisplay",	cmd_redisplay,	0, 0, 0);
	add_command("redo",		cmd_redo,	0, 0, 0);
	add_command("remhook",		cmd_remhook,	2, 2, 2,
                    "Hook: ", arg_HOOK,
                    "Command: ", arg_COMMAND);
	add_command("remove",		cmd_remove,	0, 0, 0);
	add_command("resize",		cmd_resize,	2, 0, 2,
                    "", arg_NUMBER,
                    "", arg_NUMBER);
	add_command("restart",		cmd_restart,	0, 0, 0);
	add_command("sdump",		cmd_sdump,	0, 0, 0);
	add_command("select",		cmd_select,	1, 0, 1,
                    "Select window: ", arg_REST);
	add_command("set",		cmd_set,	2, 0, 0,
                    "", arg_VARIABLE,
                    "", arg_REST);
	add_command("setenv",		cmd_setenv,	2, 2, 2,
                    "Variable: ", arg_STRING,
                    "Value: ", arg_REST);
	add_command("sfdump",		cmd_sfdump,	0, 0, 0);
	add_command("sfrestore",	cmd_sfrestore,	1, 1, 1,
                    "Frames: ", arg_REST);
	add_command("shrink",		cmd_shrink,	0, 0, 0);
	add_command("source",		cmd_source,	1, 1, 1,
                    "File: ", arg_REST);
	add_command("smove",		cmd_smove,	1, 1, 1,
                    "Screen: ", arg_NUMBER);
	add_command("sselect",		cmd_sselect,	1, 1, 1,
                    "Screen: ", arg_NUMBER);
	add_command("stick",		cmd_stick,	0, 0, 0);
	add_command("swap",		cmd_swap,	2, 1, 1,
	            "destination frame: ", arg_FRAME,
	            "source frame: ", arg_FRAME);
	add_command("title",		cmd_rename,	1, 1, 1,
                    "Set window's title to: ", arg_REST);
	add_command("unalias",		cmd_unalias,	1, 1, 1,
                    "Alias: ", arg_STRING);
	add_command("undefinekey",	cmd_undefinekey, 2, 2, 2,
	            "Keymap: ", arg_KEYMAP,
	            "Key: ", arg_KEY);
	add_command("undo",		cmd_undo,	0, 0, 0);
	add_command("unmanage",		cmd_unmanage,	1, 1, 0,
                    "Unmanage: ", arg_REST);
	add_command("unsetenv",		cmd_unsetenv,	1, 1, 1,
                    "Variable: ", arg_STRING);
	add_command("unstick",		cmd_unstick,	0, 0, 0);
	add_command("verbexec",		cmd_verbexec,	1, 1, 1,
                    "/bin/sh -c ", arg_SHELLCMD);
	add_command("version",		cmd_version,	0, 0, 0);
	add_command("vmove",		cmd_vmove,	1, 1, 1,
	            "Virtual Screen: ", arg_VSCREEN);
	add_command("vnext",		cmd_vnext,	0, 0, 0);
	add_command("vother",		cmd_vother,	0, 0, 0);
	add_command("vprev",		cmd_vprev,	0, 0, 0);
	add_command("vrename",		cmd_vrename,	1, 1, 1,
	            "Change virtual screen name to: ", arg_REST);
	add_command("vscreens",		cmd_vscreens,	0, 0, 0);
	add_command("vselect",		cmd_vselect,	1, 1, 1,
	            "Virtual Screen: ", arg_VSCREEN);
	add_command("vsplit",		cmd_vsplit,	1, 0, 0,
                    "Split: ", arg_STRING);
	add_command("windows",		cmd_windows,	1, 0, 0,
                    "", arg_REST);
	/* @end (tag required for genrpbindings) */

	init_set_vars();
}

/* Delete all entries in the redo list. */
static void
clear_frame_redos(void)
{
	rp_frame_undo *cur;
	struct list_head *tmp, *iter;

	list_for_each_safe_entry(cur, iter, tmp, &rp_frame_redos, node) {
		free(cur->frames);
		list_del(&(cur->node));
	}
}

static void
del_frame_undo(rp_frame_undo *u)
{
	if (!u)
		return;

	free(u->frames);
	list_del(&(u->node));
	free(u);
}

void
clear_frame_undos(void)
{
	while (list_size(&rp_frame_undos) > 0) {
		/* Delete the oldest node */
		rp_frame_undo *cur;
		list_last(cur, &rp_frame_undos, node);
		del_frame_undo(cur);
	}
}

static void
push_frame_undo(rp_vscreen *vscreen)
{
	rp_frame_undo *cur;

	if (list_size(&rp_frame_undos) > defaults.maxundos) {
		/* Delete the oldest node */
		list_last(cur, &rp_frame_undos, node);
		del_frame_undo(cur);
	}
	cur = xmalloc(sizeof(rp_frame_undo));
	cur->frames = fdump(vscreen);
	cur->vscreen = vscreen;
	list_add(&cur->node, &rp_frame_undos);
	/*
	 * Since we're creating new frames the redo list is now invalid, so
	 * clear it.
	 */
	clear_frame_redos();
}

static rp_frame_undo *
pop_frame_list(struct list_head *undo_list, struct list_head *redo_list)
{
	rp_vscreen *vscreen = rp_current_vscreen;
	rp_frame_undo *first, *new;

	/* Is there something to restore? */
	list_first(first, undo_list, node);
	if (!first)
		return NULL;

	/* First save the current layout into undo */
	new = xmalloc(sizeof(rp_frame_undo));
	new->frames = fdump(vscreen);
	new->vscreen = vscreen;
	list_add(&new->node, redo_list);

	list_del(&(first->node));
	return first;
}

/* Pop the head of the frame undo list off and put it in the redo list. */
static rp_frame_undo *
pop_frame_undo(void)
{
	return pop_frame_list(&rp_frame_undos, &rp_frame_redos);
}

/* Pop the head of the frame redo list off and put it in the undo list. */
static rp_frame_undo *
pop_frame_redo(void)
{
	return pop_frame_list(&rp_frame_redos, &rp_frame_undos);
}

rp_action *
find_keybinding_by_action(char *action, rp_keymap *map)
{
	int i;

	for (i = 0; i < map->actions_last; i++) {
		if (!strcmp(map->actions[i].data, action)) {
			return &map->actions[i];
		}
	}

	return NULL;
}

rp_action *
find_keybinding(KeySym keysym, unsigned int state, rp_keymap *map)
{
	int i;
	for (i = 0; i < map->actions_last; i++) {
		if (map->actions[i].key == keysym &&
		    map->actions[i].state == state)
			return &map->actions[i];
	}
	return NULL;
}

static char *
find_command_by_keydesc(char *desc, rp_keymap *map)
{
	int i = 0;
	char *keysym_name;

	while (i < map->actions_last) {
		keysym_name = keysym_to_string(map->actions[i].key,
		    map->actions[i].state);
		if (!strcmp(keysym_name, desc)) {
			free(keysym_name);
			return map->actions[i].data;
		}
		free(keysym_name);
		i++;
	}

	return NULL;
}

static char *
resolve_command_from_keydesc(char *desc, int depth, rp_keymap *map)
{
	char *cmd, *c;

	c = find_command_by_keydesc(desc, map);
	if (!c)
		return NULL;

	/* is it a link? */
	if (strncmp(c, "link", 4) || depth > MAX_LINK_DEPTH)
		/* it is not */
		return c;

	cmd = resolve_command_from_keydesc(&c[5], depth + 1, map);
	return (cmd != NULL) ? cmd : c;
}

static void
add_keybinding(KeySym keysym, int state, char *cmd, rp_keymap *map)
{
	if (map->actions_last >= map->actions_size) {
		/* double the key table size */
		map->actions_size *= 2;
		map->actions = xrealloc(map->actions,
		    sizeof(rp_action) * map->actions_size);
		PRINT_DEBUG(("realloc()ed key_table %d\n", map->actions_size));
	}
	map->actions[map->actions_last].key = keysym;
	map->actions[map->actions_last].state = state;
	/* free this on shutdown, or re/unbinding */
	map->actions[map->actions_last].data = xstrdup(cmd);

	map->actions_last++;
}

static void
replace_keybinding(rp_action * key_action, char *newcmd)
{
	free(key_action->data);
	key_action->data = xstrdup(newcmd);
}

static int
remove_keybinding(KeySym keysym, unsigned int state, rp_keymap *map)
{
	int i;
	int found = -1;

	for (i = 0; i < map->actions_last; i++) {
		if (map->actions[i].key == keysym &&
		    map->actions[i].state == state) {
			found = i;
			break;
		}
	}

	if (found >= 0) {
		free(map->actions[found].data);

		memmove(&map->actions[found], &map->actions[found + 1],
		    sizeof(rp_action) * (map->actions_last - found - 1));
		map->actions_last--;

		return 1;
	}

	return 0;
}

static rp_keymap *
keymap_new(char *name)
{
	rp_keymap *map;

	/* All keymaps must have a name. */
	if (name == NULL)
		return NULL;

	map = xmalloc(sizeof(rp_keymap));
	map->name = xstrdup(name);
	map->actions_size = 1;
	map->actions = xmalloc(sizeof(rp_action) * map->actions_size);
	map->actions_last = 0;

	return map;
}

rp_keymap *
find_keymap(char *name)
{
	rp_keymap *cur;

	list_for_each_entry(cur, &rp_keymaps, node) {
		if (!strcmp(name, cur->name)) {
			return cur;
		}
	}

	return NULL;
}

/*
 * Search the alias table for a match. If a match is found, return its index
 * into the table. Otherwise return -1.
 */
static int
find_alias_index(char *name)
{
	int i;

	for (i = 0; i < alias_list_last; i++)
		if (!strcmp(name, alias_list[i].name))
			return i;

	return -1;
}

static void
add_alias(char *name, char *alias)
{
	int i;

	/* Are we updating an existing alias, or creating a new one? */
	i = find_alias_index(name);
	if (i >= 0) {
		free(alias_list[i].alias);
		alias_list[i].alias = xstrdup(alias);
	} else {
		if (alias_list_last >= alias_list_size) {
			alias_list_size *= 2;
			alias_list = xrealloc(alias_list,
			    sizeof(alias_t) * alias_list_size);
		}
		alias_list[alias_list_last].name = xstrdup(name);
		alias_list[alias_list_last].alias = xstrdup(alias);
		alias_list_last++;
	}
}

void
initialize_default_keybindings(void)
{
	rp_keymap *map, *top;

	map = keymap_new(ROOT_KEYMAP);
	list_add(&map->node, &rp_keymaps);

	top = keymap_new(defaults.top_kmap);
	list_add(&top->node, &rp_keymaps);

	/* Initialize the alias list. */
	alias_list_size = 5;
	alias_list_last = 0;
	alias_list = xmalloc(sizeof(alias_t) * alias_list_size);

	prefix_key.sym = KEY_PREFIX;
	prefix_key.state = MODIFIER_PREFIX;

	/* Add the prefix key to the top-level map. */
	add_keybinding(prefix_key.sym, prefix_key.state, "readkey " ROOT_KEYMAP,
	    top);

	add_keybinding(prefix_key.sym, prefix_key.state, "other", map);
	add_keybinding(prefix_key.sym, 0, "meta", map);
	add_keybinding(XK_g, RP_CONTROL_MASK, "abort", map);
	add_keybinding(XK_0, 0, "select 0", map);
	add_keybinding(XK_1, 0, "select 1", map);
	add_keybinding(XK_2, 0, "select 2", map);
	add_keybinding(XK_3, 0, "select 3", map);
	add_keybinding(XK_4, 0, "select 4", map);
	add_keybinding(XK_5, 0, "select 5", map);
	add_keybinding(XK_6, 0, "select 6", map);
	add_keybinding(XK_7, 0, "select 7", map);
	add_keybinding(XK_8, 0, "select 8", map);
	add_keybinding(XK_9, 0, "select 9", map);
	add_keybinding(XK_minus, 0, "select -", map);
	add_keybinding(XK_A, 0, "title", map);
	add_keybinding(XK_A, RP_CONTROL_MASK, "title", map);
	add_keybinding(XK_K, 0, "kill", map);
	add_keybinding(XK_K, RP_CONTROL_MASK, "kill", map);
	add_keybinding(XK_Return, 0, "next", map);
	add_keybinding(XK_Return, RP_CONTROL_MASK, "next", map);
	add_keybinding(XK_b, 0, "banish", map);
	add_keybinding(XK_b, RP_CONTROL_MASK, "banish", map);
	add_keybinding(XK_c, 0, "exec " TERM_PROG, map);
	add_keybinding(XK_c, RP_CONTROL_MASK, "exec " TERM_PROG, map);
	add_keybinding(XK_colon, 0, "colon", map);
	add_keybinding(XK_exclam, 0, "exec", map);
	add_keybinding(XK_exclam, RP_CONTROL_MASK,
	    "colon exec " TERM_PROG " -e ", map);
	add_keybinding(XK_i, 0, "info", map);
	add_keybinding(XK_i, RP_CONTROL_MASK, "info", map);
	add_keybinding(XK_k, 0, "delete", map);
	add_keybinding(XK_k, RP_CONTROL_MASK, "delete", map);
	add_keybinding(XK_l, 0, "redisplay", map);
	add_keybinding(XK_l, RP_CONTROL_MASK, "redisplay", map);
	add_keybinding(XK_m, 0, "lastmsg", map);
	add_keybinding(XK_m, RP_CONTROL_MASK, "lastmsg", map);
	add_keybinding(XK_n, 0, "next", map);
	add_keybinding(XK_n, RP_CONTROL_MASK, "next", map);
	add_keybinding(XK_p, 0, "prev", map);
	add_keybinding(XK_p, RP_CONTROL_MASK, "prev", map);
	add_keybinding(XK_quoteright, 0, "select", map);
	add_keybinding(XK_quoteright, RP_CONTROL_MASK, "select", map);
	add_keybinding(XK_space, 0, "next", map);
	add_keybinding(XK_space, RP_CONTROL_MASK, "next", map);
	add_keybinding(XK_v, 0, "version", map);
	add_keybinding(XK_v, RP_CONTROL_MASK, "version", map);
	add_keybinding(XK_w, 0, "windows", map);
	add_keybinding(XK_w, RP_CONTROL_MASK, "windows", map);
	add_keybinding(XK_s, 0, "split", map);
	add_keybinding(XK_s, RP_CONTROL_MASK, "split", map);
	add_keybinding(XK_S, 0, "hsplit", map);
	add_keybinding(XK_S, RP_CONTROL_MASK, "hsplit", map);
	add_keybinding(XK_Tab, 0, "focus", map);
	add_keybinding(XK_Tab, RP_META_MASK, "focuslast", map);
	add_keybinding(XK_Left, RP_CONTROL_MASK, "exchangeleft", map);
	add_keybinding(XK_Right, RP_CONTROL_MASK, "exchangeright", map);
	add_keybinding(XK_Up, RP_CONTROL_MASK, "exchangeup", map);
	add_keybinding(XK_Down, RP_CONTROL_MASK, "exchangedown", map);
	add_keybinding(XK_Left, 0, "focusleft", map);
	add_keybinding(XK_Right, 0, "focusright", map);
	add_keybinding(XK_Up, 0, "focusup", map);
	add_keybinding(XK_Down, 0, "focusdown", map);
	add_keybinding(XK_Q, 0, "only", map);
	add_keybinding(XK_R, 0, "remove", map);
	add_keybinding(XK_f, 0, "fselect", map);
	add_keybinding(XK_f, RP_CONTROL_MASK, "fselect", map);
	add_keybinding(XK_F, 0, "curframe", map);
	add_keybinding(XK_r, 0, "resize", map);
	add_keybinding(XK_r, RP_CONTROL_MASK, "resize", map);
	add_keybinding(XK_question, 0, "help " ROOT_KEYMAP, map);
	add_keybinding(XK_underscore, RP_CONTROL_MASK, "undo", map);
	add_keybinding(XK_u, 0, "undo", map);
	add_keybinding(XK_u, RP_CONTROL_MASK, "undo", map);
	add_keybinding(XK_U, 0, "redo", map);
	add_keybinding(XK_x, 0, "swap", map);
	add_keybinding(XK_x, RP_CONTROL_MASK, "swap", map);
	add_keybinding(XK_N, 0, "nextscreen", map);
	add_keybinding(XK_P, 0, "prevscreen", map);
	add_keybinding(XK_F1, 0, "vselect 0", map);
	add_keybinding(XK_F2, 0, "vselect 1", map);
	add_keybinding(XK_F3, 0, "vselect 2", map);
	add_keybinding(XK_F4, 0, "vselect 3", map);
	add_keybinding(XK_F5, 0, "vselect 4", map);
	add_keybinding(XK_F6, 0, "vselect 5", map);
	add_keybinding(XK_F7, 0, "vselect 6", map);
	add_keybinding(XK_F8, 0, "vselect 7", map);
	add_keybinding(XK_F9, 0, "vselect 8", map);
	add_keybinding(XK_F10, 0, "vselect 9", map);
	add_keybinding(XK_F11, 0, "vselect 10", map);
	add_keybinding(XK_F12, 0, "vselect 11", map);

	add_alias("unbind", "undefinekey " ROOT_KEYMAP);
	add_alias("bind", "definekey " ROOT_KEYMAP);
	add_alias("split", "vsplit");
}

cmdret *
cmdret_new(int success, char *fmt,...)
{
	cmdret *ret;
	va_list ap;

	ret = xmalloc(sizeof(cmdret));
	ret->success = success;

	if (fmt) {
		va_start(ap, fmt);
		ret->output = xvsprintf(fmt, ap);
		va_end(ap);
	} else
		ret->output = NULL;

	return ret;
}

void
cmdret_free(cmdret *ret)
{
	free(ret->output);
	free(ret);
}

static void
keymap_free(rp_keymap *map)
{
	int i;

	/* Free the data in the actions. */
	for (i = 0; i < map->actions_last; i++) {
		free(map->actions[i].data);
	}

	/* Free the map data. */
	free(map->actions);
	free(map->name);

	/* ...and the map itself. */
	free(map);
}

void
free_keymaps(void)
{
	rp_keymap *cur;
	struct list_head *tmp, *iter;

	list_for_each_safe_entry(cur, iter, tmp, &rp_keymaps, node) {
		list_del(&cur->node);
		keymap_free(cur);
	}
}

void
free_aliases(void)
{
	int i;

	/* Free the alias data. */
	for (i = 0; i < alias_list_last; i++) {
		free(alias_list[i].name);
		free(alias_list[i].alias);
	}

	/* Free the alias list. */
	free(alias_list);
}

void
free_user_commands(void)
{
	struct user_command *cur;
	struct set_var *var;
	struct list_head *tmp, *iter;

	list_for_each_safe_entry(cur, iter, tmp, &user_commands, node) {
		list_del(&cur->node);
		user_command_free(cur);
	}
	list_for_each_safe_entry(var, iter, tmp, &set_vars, node) {
		list_del(&var->node);
		set_var_free(var);
	}
}

/*
 * return a KeySym from a string that contains either a hex value or an X
 * keysym description
 */
static int
string_to_keysym(char *str)
{
	int retval;
	int keysym;

	retval = sscanf(str, "0x%x", &keysym);

	if (!retval || retval == EOF)
		keysym = XStringToKeysym(str);

	return keysym;
}

static cmdret *
parse_keydesc(char *keydesc, struct rp_key *key)
{
	char *token, *next_token;

	if (keydesc == NULL)
		return NULL;

	key->state = 0;
	key->sym = 0;

	if (!strchr(keydesc, '-')) {
		/* Its got no hyphens in it, so just grab the keysym */
		key->sym = string_to_keysym(keydesc);

		if (key->sym == NoSymbol)
			return cmdret_new(RET_FAILURE,
			    "parse_keydesc: Unknown key '%s'", keydesc);
	} else if (keydesc[strlen(keydesc) - 1] == '-') {
		/* A key description can't end in a -. */
		return cmdret_new(RET_FAILURE,
		    "parse_keydesc: Can't parse key '%s'", keydesc);
	} else {
		/* Its got hyphens, so parse out the modifiers and keysym */
		char *copy;

		copy = xstrdup(keydesc);

		token = strtok(copy, "-");
		if (token == NULL) {
			/* It was nothing but hyphens */
			free(copy);
			return cmdret_new(RET_FAILURE,
			    "parse_keydesc: Can't parse key '%s'", keydesc);
		}
		do {
			next_token = strtok(NULL, "-");

			if (next_token == NULL) {
				/*
				 * There is nothing more to parse and token
				 * contains the keysym name.
				 */
				key->sym = string_to_keysym(token);

				if (key->sym == NoSymbol) {
					cmdret *ret = cmdret_new(RET_FAILURE,
					    "parse_keydesc: Unknown key '%s'",
					    token);
					free(copy);
					return ret;
				}
			} else {
				/*
				 * Which modifier is it? Only accept modifiers
				 * that are present. ie don't accept a hyper
				 * modifier if the keymap has no hyper key.
				 */
				if (!strcmp(token, "C")) {
					key->state |= RP_CONTROL_MASK;
				} else if (!strcmp(token, "M")) {
					key->state |= RP_META_MASK;
				} else if (!strcmp(token, "A")) {
					key->state |= RP_ALT_MASK;
				} else if (!strcmp(token, "S")) {
					key->state |= RP_SHIFT_MASK;
				} else if (!strcmp(token, "s")) {
					key->state |= RP_SUPER_MASK;
				} else if (!strcmp(token, "H")) {
					key->state |= RP_HYPER_MASK;
				} else {
					free(copy);
					return cmdret_new(RET_FAILURE,
					    "parse_keydesc: Unknown modifier '%s'",
					    token);
				}
			}

			token = next_token;
		} while (next_token != NULL);

		free(copy);
	}

	/* Successfully parsed the key. */
	return NULL;
}

static void
grab_rat(void)
{
	XGrabPointer(dpy, rp_current_screen->root, True, 0, GrabModeAsync,
	    GrabModeAsync, None, rp_current_screen->rat, CurrentTime);
}

static void
ungrab_rat(void)
{
	XUngrabPointer(dpy, CurrentTime);
}

/* Unmanage window */
cmdret *
cmd_unmanage(int interactive, struct cmdarg **args)
{
	if (args[0] == NULL && !interactive) {
		cmdret *ret;
		char *s = list_unmanaged_windows();

		if (s)
			ret = cmdret_new(RET_SUCCESS, "%s", s);
		else
			ret = cmdret_new(RET_SUCCESS, NULL);

		free(s);
		return ret;
	}

	if (!args[0])
		return cmdret_new(RET_FAILURE,
		    "unmanage: at least one argument required");

	add_unmanaged_window(ARG_STRING(0));

	return cmdret_new(RET_SUCCESS, NULL);
}

/* Clear the unmanaged window list */
cmdret *
cmd_clrunmanaged(int interactive, struct cmdarg **args)
{
	clear_unmanaged_list();
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_undefinekey(int interactive, struct cmdarg **args)
{
	cmdret *ret = NULL;
	rp_keymap *map;
	struct rp_key *key;

	map = ARG(0, keymap);
	key = ARG(1, key);

	/*
	 * If we're updating the top level map, we'll need to update the keys
	 * grabbed.
	 */
	if (map == find_keymap(defaults.top_kmap))
		ungrab_keys_all_wins();

	/* If no comand is specified, then unbind the key. */
	if (!remove_keybinding(key->sym, key->state, map))
		ret = cmdret_new(RET_FAILURE,
		    "undefinekey: key '%s' is not bound", ARG_STRING(1));

	/* Update the grabbed keys. */
	if (map == find_keymap(defaults.top_kmap))
		grab_keys_all_wins();
	XSync(dpy, False);

	if (ret)
		return ret;

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_definekey(int interactive, struct cmdarg **args)
{
	cmdret *ret = NULL;
	rp_keymap *map;
	struct rp_key *key;
	char *cmd;
	rp_action *key_action;

	map = ARG(0, keymap);
	key = ARG(1, key);
	cmd = ARG_STRING(2);

	/*
	 * If we're updating the top level map, we'll need to update the keys
	 * grabbed.
	 */
	if (map == find_keymap(defaults.top_kmap))
		ungrab_keys_all_wins();

	if ((key_action = find_keybinding(key->sym, key->state, map)))
		replace_keybinding(key_action, cmd);
	else
		add_keybinding(key->sym, key->state, cmd, map);

	/* Update the grabbed keys. */
	if (map == find_keymap(defaults.top_kmap))
		grab_keys_all_wins();
	XSync(dpy, False);

	if (ret)
		return ret;

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_source(int interactive, struct cmdarg **args)
{
	FILE *fileptr;

	if ((fileptr = fopen(ARG_STRING(0), "r")) == NULL)
		return cmdret_new(RET_FAILURE, "source: %s: %s", ARG_STRING(0),
		    strerror(errno));

	set_close_on_exec(fileno(fileptr));
	read_rc_file(fileptr);
	fclose(fileptr);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_meta(int interactive, struct cmdarg **args)
{
	XEvent ev;

	memset(&ev, 0, sizeof(ev));
	/*
	 * Redundant with the line above, but points out that passing some
	 * garbage time value trips up some clients
	 */
	ev.xkey.time = CurrentTime;

	if (current_window() == NULL)
		return cmdret_new(RET_FAILURE, NULL);

	ev.xkey.type = KeyPress;
	ev.xkey.display = dpy;
	ev.xkey.window = current_window()->w;

	if (args[0]) {
		struct rp_key key;
		cmdret *ret;

		memset(&key, 0, sizeof(key));
		ret = parse_keydesc(ARG_STRING(0), &key);
		if (ret != NULL)
			return ret;

		ev.xkey.state = rp_mask_to_x11_mask(key.state);
		ev.xkey.keycode = XKeysymToKeycode(dpy, key.sym);
		if (ev.xkey.keycode == NoSymbol)
			return cmdret_new(RET_FAILURE,
			    "meta: Couldn't convert keysym to keycode");
	} else {
		ev.xkey.state = rp_mask_to_x11_mask(prefix_key.state);
		ev.xkey.keycode = XKeysymToKeycode(dpy, prefix_key.sym);
	}

	XSendEvent(dpy, current_window()->w, False, KeyPressMask, &ev);
	XSync(dpy, False);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_prev(int interactive, struct cmdarg **args)
{
	rp_window *cur, *win;
	cur = current_window();
	win = vscreen_prev_window(rp_current_vscreen, cur);

	if (win)
		set_active_window(win);
	else if (cur)
		return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_WINDOW);
	else
		return cmdret_new(RET_FAILURE, "%s",
		    MESSAGE_NO_MANAGED_WINDOWS);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_prevframe(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	frame = find_frame_prev(current_frame(rp_current_vscreen));
	if (!frame)
		return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_FRAME);

	set_active_frame(frame, 0);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_next(int interactive, struct cmdarg **args)
{
	rp_window *cur, *win;
	cur = current_window();
	win = vscreen_next_window(rp_current_vscreen, cur);

	if (win)
		set_active_window(win);
	else if (cur)
		return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_WINDOW);
	else
		return cmdret_new(RET_FAILURE, "%s",
		    MESSAGE_NO_MANAGED_WINDOWS);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_nextframe(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	frame = find_frame_next(current_frame(rp_current_vscreen));
	if (!frame)
		return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_FRAME);

	set_active_frame(frame, 0);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_other(int interactive, struct cmdarg **args)
{
	rp_window *w;

	/* w = find_window_other (); */
	w = vscreen_last_window(rp_current_vscreen);
	if (!w)
		return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_WINDOW);

	set_active_window_force(w);

	return cmdret_new(RET_SUCCESS, NULL);
}

/* Parse a positive or null number, returns -1 on failure. */
static int
string_to_positive_int(char *str)
{
	char *ep;
	long lval;

	errno = 0;
	lval = strtol(str, &ep, 10);
	if (str[0] == '\0' || *ep != '\0')
		return -1;
	if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
	    (lval > INT_MAX || lval < 0))
		return -1;

	return (int) lval;
}

static struct list_head *
trivial_completions(char *str)
{
	struct list_head *list;

	/* Initialize our list. */
	list = xmalloc(sizeof(struct list_head));
	INIT_LIST_HEAD(list);

	return list;
}

static struct list_head *
keymap_completions(char *str)
{
	rp_keymap *cur;
	struct list_head *list;

	/* Initialize our list. */
	list = xmalloc(sizeof(struct list_head));
	INIT_LIST_HEAD(list);

	list_for_each_entry(cur, &rp_keymaps, node) {
		struct sbuf *name;

		name = sbuf_new(0);
		sbuf_copy(name, cur->name);
		list_add_tail(&name->node, list);
	}

	return list;
}

static struct list_head *
window_completions(char *str)
{
	rp_window_elem *cur;
	struct list_head *list;

	/* Initialize our list. */
	list = xmalloc(sizeof(struct list_head));
	INIT_LIST_HEAD(list);

	/* Gather the names of all the windows. */
	list_for_each_entry(cur, &rp_current_vscreen->mapped_windows, node) {
		struct sbuf *name;

		name = sbuf_new(0);
		sbuf_copy(name, window_name(cur->win));
		list_add_tail(&name->node, list);
	}

	return list;
}

/* switch to window number or name */
cmdret *
cmd_select(int interactive, struct cmdarg **args)
{
	cmdret *ret = NULL;
	char *str;
	int n;

	/*
	 * FIXME: This is manually done because of the kinds of things select
	 * accepts.
	 */
	if (args[0] == NULL)
		str = get_more_input(MESSAGE_PROMPT_SWITCH_TO_WINDOW, "",
		    hist_SELECT, SUBSTRING, window_completions);
	else
		str = xstrdup(ARG_STRING(0));

	/* User aborted. */
	if (str == NULL)
		return cmdret_new(RET_FAILURE, NULL);

	/* Only search if the string contains something to search for. */
	if (strlen(str) > 0) {
		if (strlen(str) == 1 && str[0] == '-') {
			blank_frame(current_frame(rp_current_vscreen));
			ret = cmdret_new(RET_SUCCESS, NULL);
		} else if ((n = string_to_positive_int(str)) >= 0) {
			/* try by number */
			rp_window_elem *elem = vscreen_find_window_by_number(
			    rp_current_vscreen, n);

			if (elem) {
				goto_window(elem->win);
				ret = cmdret_new(RET_SUCCESS, NULL);
			} else {
				if (interactive) {
					/* show the window list as feedback */
					show_bar(rp_current_screen,
					    defaults.window_fmt);
					ret = cmdret_new(RET_SUCCESS, NULL);
				} else {
					ret = cmdret_new(RET_FAILURE,
					    "select: unknown window number '%d'",
					    n);
				}
			}
		} else {
			/* try by name */
			rp_window *win = find_window_name(str, 1);

			if (!win)
				win = find_window_name(str, 0);

			if (win) {
				goto_window(win);
				ret = cmdret_new(RET_SUCCESS, NULL);
			} else
				ret = cmdret_new(RET_FAILURE,
				    "select: unknown window '%s'", str);
		}
	} else
		/* Silently fail, since the user didn't provide a window spec */
		ret = cmdret_new(RET_SUCCESS, NULL);

	free(str);

	return ret;
}

cmdret *
cmd_rename(int interactive, struct cmdarg **args)
{
	if (current_window() == NULL)
		return cmdret_new(RET_FAILURE, NULL);

	free(current_window()->user_name);
	current_window()->user_name = xstrdup(ARG_STRING(0));
	current_window()->named = 1;
	hook_run(&rp_title_changed_hook);

	/* Update the program bar. */
	update_window_names(rp_current_screen, defaults.window_fmt);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_delete(int interactive, struct cmdarg **args)
{
	XEvent ev;
	int status;

	if (current_window() == NULL)
		return cmdret_new(RET_FAILURE, NULL);

	ev.xclient.type = ClientMessage;
	ev.xclient.window = current_window()->w;
	ev.xclient.message_type = wm_protocols;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = wm_delete;
	ev.xclient.data.l[1] = CurrentTime;

	status = XSendEvent(dpy, current_window()->w, False, 0, &ev);
	if (status == 0)
		PRINT_DEBUG(("Delete window failed\n"));

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_kill(int interactive, struct cmdarg **args)
{
	if (current_window() == NULL)
		return cmdret_new(RET_FAILURE, NULL);

	if (XKillClient(dpy, current_window()->w) == BadValue)
		return cmdret_new(RET_FAILURE,
		    "kill failed (got BadValue, this may be a bug)");

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_version(int interactive, struct cmdarg **args)
{
	return cmdret_new(RET_SUCCESS, "%s %s", PROGNAME, VERSION);
}

static char *
frame_selector(unsigned int n)
{
	if (n < strlen(defaults.frame_selectors)) {
		return xsprintf(" %c ", defaults.frame_selectors[n]);
	} else {
		return xsprintf(" %d ", n);
	}
}

/* Return true if ch is nth frame selector. */
static int
frame_selector_match(char ch)
{
	size_t i;

	/* Is it in the frame selector string? */
	for (i = 0; i < strlen(defaults.frame_selectors); i++) {
		if (ch == defaults.frame_selectors[i])
			return i;
	}

	/*
	 * Maybe it's a number less than 9 and the frame selector doesn't
	 * define that many selectors.
	 */
	if (ch >= '0' && ch <= '9'
	    && (size_t) (ch - '0') >= strlen(defaults.frame_selectors)) {
		return ch - '0';
	}
	return -1;
}

static cmdret *
read_string(struct argspec *spec, struct sbuf *s, int history_id,
    completion_fn fn, struct cmdarg ** arg)
{
	char *input;

	if (s)
		input = xstrdup(sbuf_get(s));
	else
		input = get_input(spec->prompt, history_id, fn);

	if (input) {
		*arg = xmalloc(sizeof(struct cmdarg));
		(*arg)->type = spec->type;
		(*arg)->string = input;
		return NULL;
	}

	*arg = NULL;
	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
read_keymap(struct argspec *spec, struct sbuf *s, struct cmdarg **arg)
{
	char *input;

	if (s)
		input = xstrdup(sbuf_get(s));
	else
		input = get_input(spec->prompt, hist_KEYMAP, keymap_completions);

	if (input) {
		rp_keymap *map;

		map = find_keymap(input);
		if (map == NULL) {
			cmdret *ret = cmdret_new(RET_FAILURE,
			    "unknown keymap '%s'", input);
			free(input);
			return ret;
		}

		*arg = xmalloc(sizeof(struct cmdarg));
		(*arg)->type = spec->type;
		(*arg)->arg.keymap = map;
		(*arg)->string = input;

		return NULL;
	}

	*arg = NULL;
	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
read_keydesc(struct argspec *spec, struct sbuf *s, struct cmdarg **arg)
{
	char *input;

	if (s)
		input = xstrdup(sbuf_get(s));
	else
		input = get_input(spec->prompt, hist_KEY, trivial_completions);

	if (input) {
		cmdret *ret;
		struct rp_key *key = xmalloc(sizeof(struct rp_key));

		ret = parse_keydesc(input, key);
		if (ret) {
			free(key);
			return ret;
		}

		*arg = xmalloc(sizeof(struct cmdarg));
		(*arg)->type = spec->type;
		(*arg)->arg.key = key;
		(*arg)->string = input;

		return NULL;
	}

	*arg = NULL;
	return cmdret_new(RET_SUCCESS, NULL);
}

static struct list_head *
vscreen_completions(char *str)
{
       struct list_head *list;
       rp_vscreen *cur;

       /* Initialize our list. */
       list = xmalloc(sizeof(struct list_head));
       INIT_LIST_HEAD(list);

       /* Grab all the vscreen names. */
       list_for_each_entry(cur, &(rp_current_screen)->vscreens, node) {
	       struct sbuf *s;

	       s = sbuf_new(0);
	       if (cur->name) {
		       sbuf_copy(s, cur->name);
	       } else {
		       sbuf_printf(s, "%d", cur->number);
	       }

	       list_add_tail(&s->node, list);
       }

       return list;
}

static struct list_head *
colon_completions(char *str)
{
	int i;
	struct user_command *uc;
	struct sbuf *s;
	struct list_head *list;

	/* Initialize our list. */
	list = xmalloc(sizeof(struct list_head));
	INIT_LIST_HEAD(list);

	/* Put all the aliases in our list. */
	for (i = 0; i < alias_list_last; ++i) {
		s = sbuf_new(0);
		sbuf_copy(s, alias_list[i].name);
		/*
		 * The space is so when the user completes a space is
		 * conveniently inserted after the command.
		 */
		sbuf_concat(s, " ");
		list_add_tail(&s->node, list);
	}

	/* Put all the commands in our list. */
	list_for_each_entry(uc, &user_commands, node) {
		s = sbuf_new(0);
		sbuf_copy(s, uc->name);
		/*
		 * The space is so when the user completes a space is
		 * conveniently inserted after the command.
		 */
		sbuf_concat(s, " ");
		list_add_tail(&s->node, list);
	}

	return list;
}

static cmdret *
read_command(struct argspec *spec, struct sbuf *s, struct cmdarg **arg)
{
	return read_string(spec, s, hist_COMMAND, colon_completions, arg);
}

static struct list_head *
exec_completions(char *str)
{
	size_t n = 256;
	char *partial;
	struct sbuf *line;
	FILE *file;
	struct list_head *head;
	char *completion_string;

	/* Initialize our list. */
	head = xmalloc(sizeof(struct list_head));
	INIT_LIST_HEAD(head);

	/* FIXME: A Bash dependancy?? */
	completion_string = xsprintf("bash -c \"compgen -ac %s|sort\"", str);
	file = popen(completion_string, "r");
	free(completion_string);
	if (!file) {
		warn("popen failed");
		return head;
	}
	partial = xmalloc(n);

	/*
	 * Read data from the file, split it into lines and store it in a list.
	 */
	line = sbuf_new(0);
	while (fgets(partial, n, file) != NULL) {
		/* Read a chunk from the file into our line accumulator. */
		sbuf_concat(line, partial);

		if (feof(file) ||
		    (*(sbuf_get(line) + strlen(sbuf_get(line)) - 1) == '\n')) {
			char *s;
			struct sbuf *elem;

			s = sbuf_get(line);

			/* Frob the newline into */
			if (*(s + strlen(s) - 1) == '\n')
				*(s + strlen(s) - 1) = '\0';

			/* Add our line to the list. */
			elem = sbuf_new(0);
			sbuf_copy(elem, s);
			list_add_tail(&elem->node, head);

			sbuf_clear(line);
		}
	}

	sbuf_free(line);

	free(partial);
	pclose(file);

	return head;
}

static cmdret *
read_shellcmd(struct argspec *spec, struct sbuf *s, struct cmdarg **arg,
    const char *command_name)
{
	cmdret *ret;

	ret = read_string(spec, s, hist_SHELLCMD, exec_completions, arg);
	if (command_name && !s && !ret && (*arg)->string) {
		/* store for command history */
		struct sbuf *buf;

		buf = sbuf_new(0);
		sbuf_printf(buf, "%s %s", command_name, (*arg)->string);
		history_add(hist_COMMAND, sbuf_get(buf));
		sbuf_free(buf);
	}

	return ret;
}

static cmdret *
read_frame(struct sbuf *s, struct cmdarg **arg)
{
	rp_frame *frame;
	XSetWindowAttributes attr;
	int fnum = -1;
	KeySym c;
	char keysym_buf[513];
	int keysym_bufsize = sizeof(keysym_buf);
	unsigned int mod;
	Window *wins;
	int i = 0;
	rp_frame *cur_frame;
	rp_screen *cur_screen = rp_current_screen;
	int frames;

	if (s == NULL) {
		frames = num_frames(rp_current_vscreen);
		wins = xmalloc(sizeof(Window) * frames);

		/*
		 * Loop through each frame and display its number in it's top
		 * left corner.
		 */
		attr.border_pixel = rp_glob_screen.fg_color;
		attr.background_pixel = rp_glob_screen.bg_color;
		attr.override_redirect = True;

		list_for_each_entry(cur_frame, &rp_current_vscreen->frames,
		    node) {
			int width, height;
			char *num;

			/*
			 * Create the string to be displayed in the
			 * window and determine the height and width of
			 * the window.
			 */
			num = frame_selector(cur_frame->number);
			width = defaults.bar_x_padding * 2 +
			    rp_text_width(cur_screen, num, -1, NULL);
			height = (FONT_HEIGHT(cur_screen) +
			    defaults.bar_y_padding * 2);

			/* Create and map the window. */
			wins[i] = XCreateWindow(dpy, cur_screen->root,
			    cur_frame->x,
			    cur_frame->y, width,
			    height, 1, CopyFromParent, CopyFromParent,
			    CopyFromParent,
			    CWOverrideRedirect|CWBorderPixel|CWBackPixel,
			    &attr);
			XMapWindow(dpy, wins[i]);
			XClearWindow(dpy, wins[i]);

			/* Display the frame's number inside the window. */
			rp_draw_string(cur_screen, wins[i], STYLE_NORMAL,
			    defaults.bar_x_padding,
			    defaults.bar_y_padding +
			    FONT_ASCENT(cur_screen), num, -1,
			    NULL, NULL);

			free(num);
			i++;
		}
		XSync(dpy, False);

		/* Read a key. */
		read_single_key(&c, &mod, keysym_buf, keysym_bufsize);

		/* Destroy our number windows and free the array. */
		for (i = 0; i < frames; i++)
			XDestroyWindow(dpy, wins[i]);

		free(wins);

		/* FIXME: We only handle one character long keysym names. */
		if (strlen(keysym_buf) == 1) {
			fnum = frame_selector_match(keysym_buf[0]);
			if (fnum == -1)
				return cmdret_new(RET_FAILURE,
				    "unknown frame selector `%s'", keysym_buf);
		} else {
			return cmdret_new(RET_FAILURE,
			    "frame selector too long `%s'", keysym_buf);
		}
	} else {
		fnum = string_to_positive_int(sbuf_get(s));
		if (fnum == -1)
			return cmdret_new(RET_FAILURE,
			    "invalid frame selector `%s', negative or too big",
			    sbuf_get(s));
	}

	/*
	 * Now that we have a frame number to go to, let's try to jump to it.
	 */
	frame = find_frame_number(fnum);
	if (frame) {
		/*
		 * We have to return a string, because commands get lists of
		 * strings.  Sucky, yes. The command is simply going to parse
		 * it back into an rp_frame.
		 */
		*arg = xmalloc(sizeof(struct cmdarg));
		(*arg)->type = arg_FRAME;
		(*arg)->string = NULL;
		(*arg)->arg.frame = frame;
		return NULL;
	}

	return cmdret_new(RET_FAILURE, "frame not found");
}

static cmdret *
read_window(struct argspec *spec, struct sbuf *s, struct cmdarg **arg)
{
	rp_window *win = NULL;
	char *name;
	int n;

	if (s)
		name = xstrdup(sbuf_get(s));
	else
		name = get_input(spec->prompt, hist_WINDOW, window_completions);

	if (name) {
		/* try by number */
		if ((n = string_to_positive_int(name)) >= 0) {
			rp_window_elem *elem = vscreen_find_window_by_number(
			    rp_current_vscreen, n);
			if (elem)
				win = elem->win;
		} else {
			/* try by name */
			win = find_window_name(name, 1);
			if (win == NULL)
				win = find_window_name(name, 0);
		}

		if (win) {
			*arg = xmalloc(sizeof(struct cmdarg));
			(*arg)->type = arg_WINDOW;
			(*arg)->arg.win = win;
			(*arg)->string = name;
			return NULL;
		}

		free(name);
		*arg = NULL;
		return cmdret_new(RET_SUCCESS, NULL);
	}

	/* user abort. */
	*arg = NULL;
	return cmdret_new(RET_SUCCESS, NULL);
}

static int
parse_wingravity(char *data)
{
	int ret = -1;

	if (!strcasecmp(data, "northwest") || !strcasecmp(data, "nw") || !strcmp(data, "7"))
		ret = NorthWestGravity;
	if (!strcasecmp(data, "north") || !strcasecmp(data, "n") || !strcmp(data, "8"))
		ret = NorthGravity;
	if (!strcasecmp(data, "northeast") || !strcasecmp(data, "ne") || !strcmp(data, "9"))
		ret = NorthEastGravity;
	if (!strcasecmp(data, "west") || !strcasecmp(data, "w") || !strcmp(data, "4"))
		ret = WestGravity;
	if (!strcasecmp(data, "center") || !strcasecmp(data, "c") || !strcmp(data, "5"))
		ret = CenterGravity;
	if (!strcasecmp(data, "east") || !strcasecmp(data, "e") || !strcmp(data, "6"))
		ret = EastGravity;
	if (!strcasecmp(data, "southwest") || !strcasecmp(data, "sw") || !strcmp(data, "1"))
		ret = SouthWestGravity;
	if (!strcasecmp(data, "south") || !strcasecmp(data, "s") || !strcmp(data, "2"))
		ret = SouthGravity;
	if (!strcasecmp(data, "southeast") || !strcasecmp(data, "se") || !strcmp(data, "3"))
		ret = SouthEastGravity;

	return ret;
}

static cmdret *
read_gravity(struct argspec *spec, struct sbuf *s, struct cmdarg **arg)
{
	char *input;

	if (s)
		input = xstrdup(sbuf_get(s));
	else
		input = get_input(spec->prompt, hist_GRAVITY,
		    trivial_completions);

	if (input) {
		int g = parse_wingravity(input);

		if (g == -1) {
			cmdret *ret = cmdret_new(RET_FAILURE,
			    "bad gravity '%s'", input);
			free(input);
			return ret;
		}

		*arg = xmalloc(sizeof(struct cmdarg));
		(*arg)->type = arg_GRAVITY;
		(*arg)->arg.gravity = g;
		(*arg)->string = input;

		return NULL;
	}

	*arg = NULL;
	return cmdret_new(RET_SUCCESS, NULL);
}

/*
 * Given a string, find a matching vscreen. First check if the string exactly
 * matches a vscreen name, then check if it is a number & lastly check if it
 * partially matches the name of a vscreen.
 */
static rp_vscreen *
find_vscreen(char *str)
{
	rp_vscreen *vscreen;
	int n;

	/* Exact matches are special cases. */
	if ((vscreen = screen_find_vscreen_by_name(rp_current_screen, str, 1)))
		return vscreen;

	/* Check if the user typed a vsceen number. */
	n = string_to_positive_int(str);
	if (n >= 0) {
		vscreen = screen_find_vscreen_by_number(rp_current_screen, n);
		if (vscreen)
			return vscreen;
	}

	vscreen = screen_find_vscreen_by_name(rp_current_screen, str, 0);
	return vscreen;
}

static cmdret *
read_vscreen(struct argspec *spec, struct sbuf *s, struct cmdarg **arg)
{
	char *input;

	if (s)
		input = xstrdup(sbuf_get(s));
	else
		input = get_input(spec->prompt, hist_VSCREEN,
		    vscreen_completions);

	if (input) {
		rp_vscreen *v = find_vscreen(input);
		if (v) {
			*arg = xmalloc(sizeof(struct cmdarg));
			(*arg)->type = arg_VSCREEN;
			(*arg)->arg.vscreen = v;
			(*arg)->string = input;
			return NULL;
		}

		cmdret *ret = cmdret_new(RET_FAILURE, "unknown vscreen '%s'",
		    input);
		free(input);
		return ret;
	}

	*arg = NULL;
	return cmdret_new(RET_SUCCESS, NULL);
}

static struct list_head *
hook_completions(char *str)
{
	struct list_head *list;
	struct rp_hook_db_entry *entry;

	/* Initialize our list. */
	list = xmalloc(sizeof(struct list_head));
	INIT_LIST_HEAD(list);

	for (entry = rp_hook_db; entry->name; entry++) {
		struct sbuf *hookname;

		hookname = sbuf_new(0);
		sbuf_copy(hookname, entry->name);
		list_add_tail(&hookname->node, list);
	}

	return list;
}

static cmdret *
read_hook(struct argspec *spec, struct sbuf *s, struct cmdarg **arg)
{
	char *input;

	if (s)
		input = xstrdup(sbuf_get(s));
	else
		input = get_input(spec->prompt, hist_HOOK, hook_completions);

	if (input) {
		struct list_head *hook = hook_lookup(input);

		if (hook) {
			*arg = xmalloc(sizeof(struct cmdarg));
			(*arg)->type = arg_HOOK;
			(*arg)->arg.hook = hook;
			(*arg)->string = input;
			return NULL;
		}

		cmdret *ret = cmdret_new(RET_FAILURE, "unknown hook '%s'",
		    input);
		free(input);
		return ret;
	}

	*arg = NULL;
	return cmdret_new(RET_SUCCESS, NULL);
}

static struct set_var *
find_variable(char *str)
{
	struct set_var *cur;

	list_for_each_entry(cur, &set_vars, node) {
		if (!strcmp(str, cur->var))
			return cur;
	}

	return NULL;
}

static struct list_head *
var_completions(char *str)
{
	struct list_head *list;
	struct set_var *cur;

	/* Initialize our list. */
	list = xmalloc(sizeof(struct list_head));
	INIT_LIST_HEAD(list);

	/* Grab all the vscreen names. */
	list_for_each_entry(cur, &set_vars, node) {
		struct sbuf *s;

		s = sbuf_new(0);
		sbuf_copy(s, cur->var);
		list_add_tail(&s->node, list);
	}

	return list;
}

static cmdret *
read_variable(struct argspec *spec, struct sbuf *s, struct cmdarg **arg)
{
	char *input;

	if (s)
		input = xstrdup(sbuf_get(s));
	else
		input = get_input(spec->prompt, hist_VARIABLE, var_completions);

	if (input) {
		struct set_var *var = find_variable(input);

		if (var == NULL) {
			cmdret *ret = cmdret_new(RET_FAILURE,
			    "unknown variable '%s'", input);
			free(input);
			return ret;
		}

		*arg = xmalloc(sizeof(struct cmdarg));
		(*arg)->type = arg_VARIABLE;
		(*arg)->arg.variable = var;
		(*arg)->string = input;

		return NULL;
	}

	*arg = NULL;
	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
read_number(struct argspec *spec, struct sbuf *s, struct cmdarg **arg)
{
	char *input;

	if (s)
		input = xstrdup(sbuf_get(s));
	else
		/* numbers should perhaps be more fine grained, or hist_NONE */
		input = get_input(spec->prompt, hist_OTHER,
		    trivial_completions);

	if (input) {
		char *ep;
		long lval;

		errno = 0;
		lval = strtol(input, &ep, 10);
		if (input[0] == '\0' || *ep != '\0')
			return cmdret_new(RET_FAILURE, "malformed number `%s'",
			    input);
		if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
		    (lval > INT_MAX || lval < INT_MIN))
			return cmdret_new(RET_FAILURE, "out of range number `%s'",
			    input);

		*arg = xmalloc(sizeof(struct cmdarg));
		(*arg)->type = arg_NUMBER;
		(*arg)->arg.number = lval;
		(*arg)->string = input;

		return NULL;
	}

	*arg = NULL;
	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
read_arg(struct argspec *spec, struct sbuf *s, struct cmdarg **arg,
    const char *command_name)
{
	cmdret *ret = NULL;

	switch (spec->type) {
	case arg_STRING:
	case arg_REST:
	case arg_RAW:
		ret = read_string(spec, s, hist_OTHER, trivial_completions, arg);
		break;
	case arg_KEYMAP:
		ret = read_keymap(spec, s, arg);
		break;
	case arg_KEY:
		ret = read_keydesc(spec, s, arg);
		break;
	case arg_NUMBER:
		ret = read_number(spec, s, arg);
		break;
	case arg_GRAVITY:
		ret = read_gravity(spec, s, arg);
		break;
	case arg_COMMAND:
		ret = read_command(spec, s, arg);
		break;
	case arg_SHELLCMD:
		ret = read_shellcmd(spec, s, arg, command_name);
		break;
	case arg_WINDOW:
		ret = read_window(spec, s, arg);
		break;
	case arg_FRAME:
		ret = read_frame(s, arg);
		break;
	case arg_VSCREEN:
		ret = read_vscreen(spec, s, arg);
		break;
	case arg_HOOK:
		ret = read_hook(spec, s, arg);
		break;
	case arg_VARIABLE:
		ret = read_variable(spec, s, arg);
		break;
	}

	return ret;
}

/* Return -1 on failure. Return the number of args on success. */
static cmdret *
parsed_input_to_args(int num_args, struct argspec *argspec,
    struct list_head *list, struct list_head *args, int *parsed_args,
    const char *command_name)
{
	struct sbuf *s;
	struct cmdarg *arg;
	cmdret *ret;

	PRINT_DEBUG(("list len: %d\n", list_size(list)));

	*parsed_args = 0;

	/* Convert the existing entries to cmdarg's. */
	list_for_each_entry(s, list, node) {
		if (*parsed_args >= num_args)
			break;

		ret = read_arg(&argspec[*parsed_args], s, &arg, command_name);
		/* If there was an error, then abort. */
		if (ret)
			return ret;

		list_add_tail(&arg->node, args);
		(*parsed_args)++;
	}

	return NULL;
}

/*
 * Prompt the user for missing arguments. Returns non-zero on failure. 0 on
 * success.
 */
static cmdret *
fill_in_missing_args(struct user_command *cmd, struct list_head *list,
    struct list_head *args, const char *command_name)
{
	cmdret *ret;
	struct cmdarg *arg;
	int i = 0;

	ret = parsed_input_to_args(cmd->num_args, cmd->args, list, args, &i,
	    command_name);
	if (ret)
		return ret;

	/* Fill in the rest of the required arguments. */
	for (; i < cmd->i_required_args; i++) {
		ret = read_arg(&cmd->args[i], NULL, &arg, command_name);
		if (ret)
			return ret;
		list_add_tail(&arg->node, args);
	}

	return NULL;
}

/*
 * Stick a list of sbuf's in list. if nargs >= 0 then only parse nargs
 * arguments and and the rest of the string to the list. Return 0 on success.
 * non-zero on failure. When raw is true, then when we hit nargs, we should
 * keep any whitespace at the beginning.  When false, gobble the whitespace.
 */
static cmdret *
parse_args(char *str, struct list_head * list, int nargs, int raw)
{
	cmdret *ret = NULL;
	char *i;
	char *tmp;
	int len = 0;
	int str_escape = 0;
	int in_str = 0;
	int gobble = 1;
	int parsed_args = 0;

	if (str == NULL)
		return NULL;

	tmp = xmalloc(strlen(str) + 1);

	for (i = str; *i; i++) {
		/* Have we hit the arg limit? */
		if (raw && parsed_args >= nargs) {
			struct sbuf *s = sbuf_new(0);
			if (!raw) {
				while (*i && isspace((unsigned char) *i))
					i++;
			}
			if (*i) {
				sbuf_concat(s, i);
				list_add_tail(&s->node, list);
			}
			len = 0;
			break;
		}

		/* Should we eat the whitespace? */
		if (gobble) {
			while (*i && isspace((unsigned char) *i))
				i++;
			gobble = 0;
		}

		/* Escaped characters always get added. */
		if (str_escape) {
			tmp[len] = *i;
			len++;
			str_escape = 0;
		} else if (*i == '\\') {
			str_escape = 1;
		} else if (*i == '"') {
			if (in_str) {
				/* End the arg. */
				struct sbuf *s = sbuf_new(0);

				sbuf_nconcat(s, tmp, len);
				list_add_tail(&s->node, list);
				len = 0;
				gobble = 1;
				in_str = 0;
				parsed_args++;
			} else if (len == 0) {
				/*
				 * A string open can only start at the
				 * beginning of an argument.
				 */
				in_str = 1;
			} else {
				ret = cmdret_new(RET_FAILURE,
				    "parse error in '%s'", str);
				break;
			}
		} else if (isspace((unsigned char) *i) && !in_str) {
			/* End the current arg, and start a new one. */
			struct sbuf *s = sbuf_new(0);
			sbuf_nconcat(s, tmp, len);
			list_add_tail(&s->node, list);
			len = 0;
			gobble = 1;
			parsed_args++;
		} else {
			/* Add the character to the argument. */
			tmp[len] = *i;
			len++;
		}
	}

	/* Add the remaining text in tmp. */
	if (ret == NULL && len) {
		struct sbuf *s = sbuf_new(0);
		sbuf_nconcat(s, tmp, len);
		list_add_tail(&s->node, list);
	}

	free(tmp);
	return ret;
}

/* Convert the list to an array, for easier access in commands. */
static struct cmdarg **
arg_array(struct list_head *head)
{
	int i = 0;
	struct cmdarg **args, *cur;

	args = xmalloc(sizeof(struct cmdarg *) * (list_size(head) + 1));
	list_for_each_entry(cur, head, node) {
		args[i] = cur;
		i++;
	}

	/* NULL terminate the array. */
	args[list_size(head)] = NULL;
	return args;
}

static void
arg_free(struct cmdarg *arg)
{
	if (!arg)
		return;

	/* read_frame doesn't fill in string. */
	free(arg->string);

	switch (arg->type) {
	case arg_KEY:
		free(arg->arg.key);
		break;
	case arg_REST:
	case arg_STRING:
	case arg_NUMBER:
	case arg_WINDOW:
	case arg_FRAME:
	case arg_COMMAND:
	case arg_SHELLCMD:
	case arg_KEYMAP:
	case arg_GRAVITY:
	case arg_VSCREEN:
	case arg_HOOK:
	case arg_VARIABLE:
	case arg_RAW:
		/* Do nothing */
		break;
	default:
		warnx("unknown arg type %d\n", arg->type);
		break;
	}

	free(arg);
}

cmdret *
command(int interactive, char *data)
{
	/* This static counter is used to exit from recursive alias calls. */
	static int alias_recursive_depth = 0;
	cmdret *result = NULL;
	char *cmd, *rest;
	char *input;
	struct user_command *uc;
	int i;

	if (data == NULL)
		return cmdret_new(RET_FAILURE, NULL);

	/* get a writable copy for strtok() */
	input = xstrdup(data);

	cmd = input;
	/* skip beginning whitespace. */
	while (*cmd && isspace((unsigned char) *cmd))
		cmd++;
	rest = cmd;
	/* skip til we get to whitespace */
	while (*rest && !isspace((unsigned char) *rest))
		rest++;
	/*
	 * mark that spot as the end of the command and make rest point to the
	 * rest of the string.
	 */
	if (*rest) {
		*rest = 0;
		rest++;
	}
	PRINT_DEBUG(("cmd==%s rest==%s\n", cmd, rest ? rest : "NULL"));

	/* Look for it in the aliases, first. */
	for (i = 0; i < alias_list_last; i++) {
		if (strcmp(cmd, alias_list[i].name) != 0)
			continue;

		struct sbuf *s;

		/*
		 * Append any arguments onto the end of the alias'
		 * command.
		 */
		s = sbuf_new(0);
		sbuf_concat(s, alias_list[i].alias);
		if (rest != NULL && *rest)
			sbuf_printf_concat(s, " %s", rest);

		alias_recursive_depth++;
		if (alias_recursive_depth >= MAX_ALIAS_RECURSIVE_DEPTH)
			result = cmdret_new(RET_FAILURE,
			    "command: alias recursion has exceeded "
			    "maximum depth");
		else
			result = command(interactive, sbuf_get(s));
		alias_recursive_depth--;

		sbuf_free(s);
		goto done;
	}

	/* If it wasn't an alias, maybe its a command. */
	list_for_each_entry(uc, &user_commands, node) {
		if (strcmp(cmd, uc->name) != 0)
			continue;

		struct sbuf *scur;
		struct cmdarg *acur;
		struct list_head *iter, *tmp;
		struct list_head head, args;
		int nargs = 0, raw = 0;

		INIT_LIST_HEAD(&args);
		INIT_LIST_HEAD(&head);

		/*
		 * We need to tell parse_args about arg_REST and
		 * arg_SHELLCMD.
		 */
		for (i = 0; i < uc->num_args; i++)
			if (uc->args[i].type == arg_REST ||
			    uc->args[i].type == arg_COMMAND ||
			    uc->args[i].type == arg_SHELLCMD ||
			    uc->args[i].type == arg_RAW) {
				raw = 1;
				nargs = i;
				break;
			}

		/* Parse the arguments and call the function. */
		result = parse_args(rest, &head, nargs, raw);
		if (result)
			goto free_lists;

		/* Interactive commands prompt the user for missing args. */
		if (interactive)
			result = fill_in_missing_args(uc, &head, &args,
			    uc->name);
		else {
			int parsed_args;
			result = parsed_input_to_args(uc->num_args,
			    uc->args, &head, &args, &parsed_args,
			    uc->name);
		}

		if (result == NULL) {
			if ((interactive && list_size(&args) < uc->i_required_args) ||
			    (!interactive && list_size(&args) < uc->ni_required_args)) {
				result = cmdret_new(RET_FAILURE,
				    "not enough arguments.");
				goto free_lists;
			} else if (list_size(&head) > uc->num_args) {
				result = cmdret_new(RET_FAILURE,
				    "command: too many arguments.");
				goto free_lists;
			} else {
				struct cmdarg **cmdargs = arg_array(&args);
				result = uc->func(interactive, cmdargs);
				free(cmdargs);
			}
		}
free_lists:
		/* Free the parsed strings */
		list_for_each_safe_entry(scur, iter, tmp, &head, node)
		    sbuf_free(scur);

		/* Free the args */
		list_for_each_safe_entry(acur, iter, tmp, &args, node)
		    arg_free(acur);

		goto done;
	}

	result = cmdret_new(RET_FAILURE, MESSAGE_UNKNOWN_COMMAND, cmd);

done:
	free(input);
	return result;
}

cmdret *
cmd_colon(int interactive, struct cmdarg **args)
{
	cmdret *result;
	char *input;

	if (args[0] == NULL)
		input = get_input(MESSAGE_PROMPT_COMMAND, hist_COMMAND,
		    colon_completions);
	else
		input = get_more_input(MESSAGE_PROMPT_COMMAND, ARG_STRING(0),
		    hist_COMMAND, BASIC, colon_completions);

	/* User aborted. */
	if (input == NULL)
		return cmdret_new(RET_FAILURE, NULL);

	result = command(1, input);
	free(input);
	return result;
}

cmdret *
cmd_exec(int interactive, struct cmdarg **args)
{
	spawn(ARG_STRING(0), current_frame(rp_current_vscreen));
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_execa(int interactive, struct cmdarg **args)
{
	spawn(ARG_STRING(0), NULL);
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_execf(int interactive, struct cmdarg **args)
{
	/*
	 * Tell spawn's shell to exec the command it finds, reusing the pid
	 * that fork() returned so matching to its NET_WM_PID later works
	 * properly.
	 */
	char *cmd = xsprintf("exec %s", ARG_STRING(1));
	spawn(cmd, ARG(0, frame));
	return cmdret_new(RET_SUCCESS, NULL);
}

int
spawn(char *cmd, rp_frame *frame)
{
	rp_child_info *child;
	int pid;

	pid = fork();
	if (pid == 0) {
		/*
		 * Some process setup to make sure the spawned process runs in
		 * its own session.
		 */
		putenv(rp_current_screen->display_string);
		if (setsid() == -1)
		{
			int ctty;

			ctty = open("/dev/tty", O_RDONLY);
			if (ctty != -1) {
				ioctl(ctty, TIOCNOTTY);
				close(ctty);
			}

			setpgid(0, 0);
		}

		execl("/bin/sh", "sh", "-c", cmd, (char *) NULL);
		_exit(1);
	}

	/* wait((int *) 0); */
	PRINT_DEBUG(("spawned %s with pid %d destined for frame %d\n", cmd,
	    pid, frame ? frame->number : -1));

	/* Add this child process to our list. */
	child = xmalloc(sizeof(rp_child_info));
	child->cmd = xstrdup(cmd);
	child->pid = pid;
	child->terminated = 0;
	child->frame = frame;
	child->vscreen = rp_current_vscreen;
	child->screen = rp_current_screen;
	child->window_mapped = 0;

	list_add(&child->node, &rp_children);

	return pid;
}

cmdret *
cmd_quit(int interactive, struct cmdarg **args)
{
	kill_signalled = 1;
	return cmdret_new(RET_SUCCESS, NULL);
}

/* Assign a new number to a window ala screen's number command. */
cmdret *
cmd_number(int interactive, struct cmdarg **args)
{
	int old_number, new_number;
	rp_window_elem *other_win, *win;

	/* Gather the args. */
	new_number = ARG(0, number);
	if (args[1])
		win = vscreen_find_window_by_number(rp_current_vscreen,
		    ARG(1, number));
	else
		win = vscreen_find_window(&rp_current_vscreen->mapped_windows,
		    current_window());

	/* Make the switch. */
	if (new_number >= 0 && win) {
		/* Find other window with same number and give it old number. */
		other_win = vscreen_find_window_by_number(rp_current_vscreen,
		    new_number);
		if (other_win != NULL) {
			old_number = win->number;
			other_win->number = old_number;

			/* Resort the window in the list */
			vscreen_resort_window(rp_current_vscreen, other_win);
		} else {
			numset_release(rp_current_vscreen->numset, win->number);
		}

		win->number = new_number;
		numset_add_num(rp_current_vscreen->numset, new_number);

		/* resort the the window in the list */
		vscreen_resort_window(rp_current_vscreen, win);

		/* Update the window list. */
		update_window_names(win->win->vscreen->screen,
		    defaults.window_fmt);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

/* Toggle the display of the program bar */
cmdret *
cmd_windows(int interactive, struct cmdarg **args)
{
	struct sbuf *window_list = NULL;
	int dummy;
	char *fmt;
	cmdret *ret;

	if (args[0] == NULL)
		fmt = defaults.window_fmt;
	else
		fmt = ARG_STRING(0);

	if (interactive) {
		rp_screen *s;
		s = rp_current_screen;
		ret = cmdret_new(RET_SUCCESS, NULL);
		if (defaults.bar_timeout == 0 && s->bar_is_raised) {
			hide_bar(s, 0);
			return ret;
		}
		show_bar(s, fmt);
	} else {
		window_list = sbuf_new(0);
		get_window_list(fmt, "\n", window_list, &dummy, &dummy);
		ret = cmdret_new(RET_SUCCESS, "%s", sbuf_get(window_list));
		sbuf_free(window_list);
	}
	return ret;
}

cmdret *
cmd_abort(int interactive, struct cmdarg **args)
{
	return cmdret_new(RET_SUCCESS, NULL);
}

/* Redisplay the current window by sending 2 resize events. */
cmdret *
cmd_redisplay(int interactive, struct cmdarg **args)
{
	force_maximize(current_window());
	return cmdret_new(RET_SUCCESS, NULL);
}

/* Reassign the prefix key. */
cmdret *
cmd_escape(int interactive, struct cmdarg **args)
{
	struct rp_key *key;
	rp_action *action;
	rp_keymap *map, *top;

	top = find_keymap(defaults.top_kmap);
	map = find_keymap(ROOT_KEYMAP);
	key = ARG(0, key);

	/* Update the "other" keybinding */
	action = find_keybinding(prefix_key.sym, prefix_key.state, map);
	if (action != NULL && !strcmp(action->data, "other")) {
		action->key = key->sym;
		action->state = key->state;
	}

	/* Update the "meta" keybinding */
	action = find_keybinding(prefix_key.sym, 0, map);
	if (action != NULL && !strcmp(action->data, "meta")) {
		action->key = key->sym;
		if (key->state != 0)
			action->state = 0;
		else
			action->state = RP_CONTROL_MASK;
	}

	/* Remove the grab on the current prefix key */
	ungrab_keys_all_wins();

	action = find_keybinding(prefix_key.sym, prefix_key.state, top);
	if (action != NULL && !strcmp(action->data, "readkey " ROOT_KEYMAP)) {
		action->key = key->sym;
		action->state = key->state;
	}

	/* Add the grab for the new prefix key */
	grab_keys_all_wins();

	/* Finally, keep track of the current prefix. */
	prefix_key.sym = key->sym;
	prefix_key.state = key->state;

	return cmdret_new(RET_SUCCESS, NULL);
}

/* User accessible call to display the passed in string. */
cmdret *
cmd_echo(int interactive, struct cmdarg **args)
{
	marked_message_printf(0, 0, "%s", ARG_STRING(0));

	return cmdret_new(RET_SUCCESS, NULL);
}


static cmdret *
read_split(char *str, int max, int *p)
{
	int a, b;

	if (sscanf(str, "%d/%d", &a, &b) == 2) {
		*p = (int) (max * (float) (a) / (float) (b));
	} else if (sscanf(str, "%d", p) == 1) {
		if (*p < 0)
			*p = max + *p;
	} else {
		/* Failed to read input. */
		return cmdret_new(RET_FAILURE, "bad split '%s'", str);
	}

	return NULL;
}

cmdret *
cmd_vsplit(int interactive, struct cmdarg **args)
{
	cmdret *ret;
	rp_frame *frame;
	int pixels;

	push_frame_undo(rp_current_vscreen);	/* fdump to stack */
	frame = current_frame(rp_current_vscreen);

	/* Default to dividing the frame in half. */
	if (args[0] == NULL)
		pixels = frame->height / 2;
	else {
		ret = read_split(ARG_STRING(0), frame->height, &pixels);
		if (ret)
			return ret;
	}

	if (pixels > 0)
		h_split_frame(frame, pixels);
	else
		return cmdret_new(RET_FAILURE, "vsplit: %s",
		    invalid_negative_arg);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_hsplit(int interactive, struct cmdarg **args)
{
	cmdret *ret;
	rp_frame *frame;
	int pixels;

	push_frame_undo(rp_current_vscreen);	/* fdump to stack */
	frame = current_frame(rp_current_vscreen);

	/* Default to dividing the frame in half. */
	if (args[0] == NULL)
		pixels = frame->width / 2;
	else {
		ret = read_split(ARG_STRING(0), frame->width, &pixels);
		if (ret)
			return ret;
	}

	if (pixels > 0)
		v_split_frame(frame, pixels);
	else
		return cmdret_new(RET_FAILURE, "hsplit: %s",
		    invalid_negative_arg);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_only(int interactive, struct cmdarg **args)
{
	push_frame_undo(rp_current_vscreen);	/* fdump to stack */
	remove_all_splits();
	maximize(current_window());

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_remove(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	push_frame_undo(rp_current_vscreen);	/* fdump to stack */

	if (num_frames(rp_current_vscreen) <= 1)
		return cmdret_new(RET_FAILURE,
		    "remove: cannot remove only frame");

	frame = find_frame_next(current_frame(rp_current_vscreen));
	if (frame) {
		remove_frame(current_frame(rp_current_vscreen));
		set_active_frame(frame, 0);
		show_frame_indicator(0);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_shrink(int interactive, struct cmdarg **args)
{
	push_frame_undo(rp_current_vscreen);	/* fdump to stack */
	resize_shrink_to_window(current_frame(rp_current_vscreen));
	return cmdret_new(RET_SUCCESS, NULL);
}

typedef struct resize_binding resize_binding;

struct resize_binding {
	struct rp_key key;
	enum resize_action {
		RESIZE_UNKNOWN = 0,
		RESIZE_VGROW,
		RESIZE_VSHRINK,
		RESIZE_HGROW,
		RESIZE_HSHRINK,
		RESIZE_TO_WINDOW,
		RESIZE_ABORT,
		RESIZE_END
	} action;
};

static resize_binding resize_bindings[] = {
	{ { INPUT_ABORT_KEY, INPUT_ABORT_MODIFIER }, RESIZE_ABORT},
	{ { RESIZE_VGROW_KEY, RESIZE_VGROW_MODIFIER }, RESIZE_VGROW},
	{ { RESIZE_VSHRINK_KEY, RESIZE_VSHRINK_MODIFIER }, RESIZE_VSHRINK},
	{ { RESIZE_HGROW_KEY, RESIZE_HGROW_MODIFIER }, RESIZE_HGROW},
	{ { RESIZE_HSHRINK_KEY, RESIZE_HSHRINK_MODIFIER }, RESIZE_HSHRINK},
	{ { RESIZE_SHRINK_TO_WINDOW_KEY, RESIZE_SHRINK_TO_WINDOW_MODIFIER }, RESIZE_TO_WINDOW},
	{ { RESIZE_END_KEY, RESIZE_END_MODIFIER }, RESIZE_END},

	/*
	 * Some more default keys (after the values from conf.h, so that they have lower
	 * priority): first the arrow keys:
	 */
	{ {XK_Escape, 0 }, RESIZE_ABORT},
	{ {XK_Down, 0 }, RESIZE_VGROW},
	{ {XK_Up, 0 }, RESIZE_VSHRINK},
	{ {XK_Right, 0 }, RESIZE_HGROW},
	{ {XK_Left, 0 }, RESIZE_HSHRINK},

	/* some vi-like bindings: */
	{ {XK_j, 0 }, RESIZE_VGROW},
	{ {XK_k, 0 }, RESIZE_VSHRINK},
	{ {XK_l, 0 }, RESIZE_HGROW},
	{ {XK_h, 0 }, RESIZE_HSHRINK},

	{ {0, 0 }, RESIZE_UNKNOWN},
};

cmdret *
cmd_resize(int interactive, struct cmdarg **args)
{
	rp_screen *s = rp_current_screen;

	/*
	 * If the user calls resize with arguments, treat it like the
	 * non-interactive version.
	 */
	if (interactive && args[0] == NULL) {
		char buffer[513];
		unsigned int mod;
		KeySym c;
		struct list_head *bk;

		/*
		 * If we haven't got at least 2 frames, there isn't anything to
		 * scale.
		 */
		if (num_frames(rp_current_vscreen) < 2)
			return cmdret_new(RET_FAILURE, NULL);

		/* Save the frameset in case the user aborts. */
		bk = vscreen_copy_frameset(rp_current_vscreen);

		/* Get ready to read keys. */
		grab_rat();
		XGrabKeyboard(dpy, s->key_window, False, GrabModeAsync,
		    GrabModeAsync, CurrentTime);

		while (1) {
			struct resize_binding *binding;

			show_frame_message(defaults.resize_fmt);
			read_key(&c, &mod, buffer, sizeof(buffer));

			/* Convert the mask to be compatible with us. */
			mod = x11_mask_to_rp_mask(mod);

			for (binding = resize_bindings; binding->action; binding++) {
				if (c == binding->key.sym &&
				    mod == binding->key.state)
					break;
			}

			if (binding->action == RESIZE_VGROW)
				resize_frame_vertically(
				    current_frame(rp_current_vscreen),
				    defaults.frame_resize_unit);
			else if (binding->action == RESIZE_VSHRINK)
				resize_frame_vertically(
				    current_frame(rp_current_vscreen),
				    -defaults.frame_resize_unit);
			else if (binding->action == RESIZE_HGROW)
				resize_frame_horizontally(
				    current_frame(rp_current_vscreen),
				    defaults.frame_resize_unit);
			else if (binding->action == RESIZE_HSHRINK)
				resize_frame_horizontally(
				    current_frame(rp_current_vscreen),
				    -defaults.frame_resize_unit);
			else if (binding->action == RESIZE_TO_WINDOW)
				resize_shrink_to_window(
				    current_frame(rp_current_vscreen));
			else if (binding->action == RESIZE_ABORT) {
				rp_frame *cur;

				vscreen_restore_frameset(rp_current_vscreen, bk);
				list_for_each_entry(cur,
				    &(rp_current_vscreen->frames), node) {
					maximize_all_windows_in_frame(cur);
				}
				break;
			} else if (binding->action == RESIZE_END) {
				frameset_free(bk);
				break;
			}
		}

		/* It is our responsibility to free this. */
		free(bk);

		hide_frame_indicator();
		ungrab_rat();
		XUngrabKeyboard(dpy, CurrentTime);
	} else {
		if (args[0] && args[1]) {
			resize_frame_horizontally(
			    current_frame(rp_current_vscreen),
			    ARG(0, number));
			resize_frame_vertically(
			    current_frame(rp_current_vscreen),
			    ARG(1, number));
		} else
			return cmdret_new(RET_FAILURE,
			    "resize: two numeric arguments required");
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_resizeunit(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d",
		    defaults.frame_resize_unit);

	if (ARG(0, number) >= 0)
		defaults.frame_resize_unit = ARG(0, number);
	else
		return cmdret_new(RET_FAILURE, "set resizeunit: %s",
		    invalid_negative_arg);

	return cmdret_new(RET_SUCCESS, NULL);
}

/* banish the rat pointer */
cmdret *
cmd_banish(int interactive, struct cmdarg **args)
{
	rp_screen *s;

	s = rp_current_screen;

	XWarpPointer(dpy, None, s->root, 0, 0, 0, 0, s->left + s->width - 2,
	    s->top + s->height - 2);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_banishrel(int interactive, struct cmdarg **args)
{
	rp_screen *s = rp_current_screen;
	rp_window *w = current_window();
	rp_frame *f = current_frame(rp_current_vscreen);

	if (w)
		XWarpPointer(dpy, None, w->w, 0, 0, 0, 0, w->x + w->width - 2,
		   w->y + w->height - 2);
	else
		XWarpPointer(dpy, None, s->root, 0, 0, 0, 0, f->x + f->width,
		   f->y + f->height);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_ratinfo(int interactive, struct cmdarg **args)
{
	rp_screen *s;
	Window root_win, child_win;
	int mouse_x, mouse_y, root_x, root_y;
	unsigned int mask;

	s = rp_current_screen;
	XQueryPointer(dpy, s->root, &root_win, &child_win, &mouse_x, &mouse_y,
	    &root_x, &root_y, &mask);

	return cmdret_new(RET_SUCCESS, "%d %d", mouse_x, mouse_y);
}

cmdret *
cmd_ratrelinfo(int interactive, struct cmdarg **args)
{
	rp_screen *s;
	rp_window *rpw;
	rp_frame *f;
	Window root_win, child_win;
	int mouse_x, mouse_y, root_x, root_y;
	unsigned int mask;

	s = rp_current_screen;
	rpw = current_window();
	f = current_frame(rp_current_vscreen);

	if (rpw)
		XQueryPointer(dpy, rpw->w, &root_win, &child_win, &mouse_x,
		    &mouse_y, &root_x, &root_y, &mask);
	else {
		XQueryPointer(dpy, s->root, &root_win, &child_win, &mouse_x,
		    &mouse_y, &root_x, &root_y, &mask);
		root_x -= f->x;
		root_y -= f->y;
	}

	return cmdret_new(RET_SUCCESS, "%d %d", root_x, root_y);
}

cmdret *
cmd_ratwarp(int interactive, struct cmdarg **args)
{
	rp_screen *s;

	s = rp_current_screen;
	XWarpPointer(dpy, None, s->root, 0, 0, 0, 0, ARG(0, number),
	    ARG(1, number));
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_ratrelwarp(int interactive, struct cmdarg **args)
{
	XWarpPointer(dpy, None, None, 0, 0, 0, 0, ARG(0, number),
	    ARG(1, number));
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_ratclick(int interactive, struct cmdarg **args)
{
	int button = 1;

	if (args[0]) {
		button = ARG(0, number);
		if (button < 1 || button > 3)
			return cmdret_new(RET_SUCCESS,
			    "ratclick: invalid argument");
	}

	XTestFakeButtonEvent(dpy, button, True, CurrentTime);
	XTestFakeButtonEvent(dpy, button, False, CurrentTime);
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_rathold(int interactive, struct cmdarg **args)
{
	int button = 1;

	if (args[1]) {
		button = ARG(1, number);
		if (button < 1 || button > 3)
			return cmdret_new(RET_SUCCESS,
			    "ratclick: invalid argument");
	}

	if (!strcmp(ARG_STRING(0), "down"))
		XTestFakeButtonEvent(dpy, button, True, CurrentTime);
	else if (!strcmp(ARG_STRING(0), "up"))
		XTestFakeButtonEvent(dpy, button, False, CurrentTime);
	else
		return cmdret_new(RET_FAILURE,
		    "rathold: '%s' invalid argument", ARG_STRING(0));

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_curframe(int interactive, struct cmdarg **args)
{
	if (interactive) {
		show_frame_indicator(1);
		return cmdret_new(RET_SUCCESS, NULL);
	}

	return cmdret_new(RET_SUCCESS, "%d",
	    current_frame(rp_current_vscreen)->number);
}

cmdret *
cmd_help(int interactive, struct cmdarg **args)
{
	rp_keymap *map;

	if (args[0])
		map = ARG(0, keymap);
	else
		map = find_keymap(ROOT_KEYMAP);

	if (interactive) {
		rp_screen *s = rp_current_screen;
		int i, old_i;
		int x = 20;
		int y = 20;
		int header_offset;
		int width, max_width = 0;
		/* 1 if we are drawing keys, 0 if we are drawing commands */
		int drawing_keys = 1;
		char *keysym_name;

		/* Switch to the default colormap. */
		if (current_window())
			XUninstallColormap(dpy, current_window()->colormap);
		XInstallColormap(dpy, s->def_cmap);

		XMapRaised(dpy, s->help_window);

		rp_draw_string(s, s->help_window, STYLE_NORMAL,
		    x, y + FONT_ASCENT(s), PROGNAME " key bindings", -1, NULL,
		    NULL);

		y += FONT_HEIGHT(s) * 2;

		/* Only print the "Command key" for the root keymap */
		if (map == find_keymap(ROOT_KEYMAP)) {
			rp_draw_string(s, s->help_window, STYLE_NORMAL,
			    x, y + FONT_ASCENT(s),
			    "Command key: ", -1, NULL, NULL);

			keysym_name = keysym_to_string(prefix_key.sym,
			    prefix_key.state);
			rp_draw_string(s, s->help_window, STYLE_NORMAL,
			    x + rp_text_width(s, "Command key: ", -1, NULL),
			    y + FONT_ASCENT(s),
			    keysym_name, -1, NULL, NULL);
			free(keysym_name);

			y += FONT_HEIGHT(s) * 2;
		}
		header_offset = y;

		i = old_i = 0;
		while (i < map->actions_last && old_i < map->actions_last) {
			if (drawing_keys) {
				keysym_name = keysym_to_string(map->actions[i].key,
				    map->actions[i].state);

				rp_draw_string(s, s->help_window, STYLE_NORMAL,
				    x, y + FONT_ASCENT(s),
				    keysym_name, -1, NULL, NULL);

				width = rp_text_width(s, keysym_name, -1, NULL);
				if (width > max_width)
					max_width = width;

				free(keysym_name);
			} else {
				rp_draw_string(s, s->help_window, STYLE_NORMAL,
				    x, y + FONT_ASCENT(s),
				    map->actions[i].data, -1, NULL, NULL);

				width = rp_text_width(s, map->actions[i].data,
				    -1, NULL);
				if (width > max_width)
					max_width = width;
			}

			y += FONT_HEIGHT(s);
			/* Make sure the next line fits entirely within the window. */
			if (y + FONT_HEIGHT(s) >= (s->top + s->height)) {
				if (drawing_keys) {
					x += max_width + 10;
					drawing_keys = 0;
					i = old_i;
				} else {
					x += max_width + 20;
					drawing_keys = 1;
					i++;
					old_i = i;
				}

				max_width = 0;
				y = header_offset;
			} else {
				i++;
				if (i == map->actions_last && drawing_keys) {
					x += max_width + 10;
					drawing_keys = 0;
					y = header_offset;
					i = old_i;
					max_width = 0;
				}
			}
		}

		read_any_key();
		XUnmapWindow(dpy, s->help_window);

		/* Possibly restore colormap. */
		if (current_window()) {
			XUninstallColormap(dpy, s->def_cmap);
			XInstallColormap(dpy, current_window()->colormap);
		}
		/* The help window overlaps the bar, so redraw it. */
		if (rp_current_screen->bar_is_raised)
			redraw_last_message();

		return cmdret_new(RET_SUCCESS, NULL);
	} else {
		struct sbuf *help_list;
		char *keysym_name;
		int i;
		cmdret *ret;

		help_list = sbuf_new(0);

		for (i = 0; i < map->actions_last; i++) {
			keysym_name = keysym_to_string(map->actions[i].key,
			    map->actions[i].state);
			sbuf_concat(help_list, keysym_name);
			free(keysym_name);
			sbuf_concat(help_list, " ");
			sbuf_concat(help_list, map->actions[i].data);
			if (i < map->actions_last - 1)
				sbuf_concat(help_list, "\n");
		}

		ret = cmdret_new(RET_SUCCESS, "%s", sbuf_get(help_list));
		sbuf_free(help_list);
		return ret;
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
set_rudeness(struct cmdarg **args)
{
	int num;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d",
		    rp_honour_transient_raise |
		    (rp_honour_normal_raise << 1) |
		    (rp_honour_transient_map << 2) |
		    (rp_honour_normal_map << 3));

	num = ARG(0, number);
	if (num < 0 || num > 15)
		return cmdret_new(RET_FAILURE, "rudeness: invalid level '%s'",
		    ARG_STRING(0));

	rp_honour_transient_raise = num & 1 ? 1 : 0;
	rp_honour_normal_raise = num & 2 ? 1 : 0;
	rp_honour_transient_map = num & 4 ? 1 : 0;
	rp_honour_normal_map = num & 8 ? 1 : 0;

	return cmdret_new(RET_SUCCESS, NULL);
}

char *
wingravity_to_string(int g)
{
	switch (g) {
	case NorthWestGravity:
		return "nw";
	case WestGravity:
		return "w";
	case SouthWestGravity:
		return "sw";
	case NorthGravity:
		return "n";
	case CenterGravity:
		return "c";
	case SouthGravity:
		return "s";
	case NorthEastGravity:
		return "ne";
	case EastGravity:
		return "e";
	case SouthEastGravity:
		return "se";
	}

	PRINT_DEBUG(("Unknown gravity!\n"));
	return "Unknown";
}

cmdret *
cmd_gravity(int interactive, struct cmdarg **args)
{
	int gravity;
	rp_window *win;

	win = current_window();
	if (!win)
		return cmdret_new(RET_FAILURE, NULL);

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s",
		    wingravity_to_string(win->gravity));

	if ((gravity = parse_wingravity(ARG_STRING(0))) < 0)
		return cmdret_new(RET_FAILURE, "gravity: unknown gravity");
	else {
		win->gravity = gravity;
		maximize(win);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_wingravity(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s",
		    wingravity_to_string(defaults.win_gravity));

	defaults.win_gravity = ARG(0, gravity);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_transgravity(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s",
		    wingravity_to_string(defaults.trans_gravity));

	defaults.trans_gravity = ARG(0, gravity);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_maxsizegravity(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s",
		    wingravity_to_string(defaults.maxsize_gravity));

	defaults.maxsize_gravity = ARG(0, gravity);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_msgwait(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.bar_timeout);

	if (ARG(0, number) < 0)
		return cmdret_new(RET_FAILURE, "msgwait: %s",
		    invalid_negative_arg);
	else
		defaults.bar_timeout = ARG(0, number);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_framemsgwait(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d",
		    defaults.frame_indicator_timeout);

	if (ARG(0, number) < -1)
		return cmdret_new(RET_FAILURE, "framemsgwait: %s",
		    invalid_negative_arg);
	else
		defaults.frame_indicator_timeout = ARG(0, number);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_bargravity(struct cmdarg **args)
{
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s",
		    wingravity_to_string(defaults.bar_location));

	mark_edge_frames();

	defaults.bar_location = ARG(0, gravity);

	list_for_each_entry(s, &rp_screens, node) {
		screen_update_workarea(s);
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static void
update_gc(rp_screen * s)
{
	XGCValues gcv;

	gcv.foreground = rp_glob_screen.fg_color;
	gcv.background = rp_glob_screen.bg_color;
	gcv.function = GXcopy;
	gcv.line_width = 1;
	gcv.subwindow_mode = IncludeInferiors;
	XFreeGC(dpy, s->normal_gc);
	s->normal_gc = XCreateGC(dpy, s->root,
	    GCForeground | GCBackground | GCFunction | GCLineWidth |
	    GCSubwindowMode, &gcv);
	gcv.foreground = rp_glob_screen.bg_color;
	gcv.background = rp_glob_screen.fg_color;
	XFreeGC(dpy, s->inverse_gc);
	s->inverse_gc = XCreateGC(dpy, s->root,
	    GCForeground | GCBackground | GCFunction | GCLineWidth |
	    GCSubwindowMode, &gcv);
}

static cmdret *
set_historysize(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.history_size);

	if (ARG(0, number) < 0)
		return cmdret_new(RET_FAILURE, "set historysize: %s",
		    invalid_negative_arg);

	defaults.history_size = ARG(0, number);
	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_font(struct cmdarg **args)
{
	XftFont *font;
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.font_string);

	mark_edge_frames();

	list_for_each_entry(s, &rp_screens, node) {
		font = XftFontOpenName(dpy, s->screen_num, ARG_STRING(0));
		if (font == NULL)
			return cmdret_new(RET_FAILURE, "set font: unknown font");

		XftFontClose(dpy, s->xft_font);
		s->xft_font = font;
	}

	free(defaults.font_string);

	defaults.font_string = xstrdup(ARG_STRING(0));

	list_for_each_entry(s, &rp_screens, node) {
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_padding(struct cmdarg **args)
{
	rp_screen *s;
	int l, t, r, b;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d %d %d %d",
		    defaults.padding_left,
		    defaults.padding_top,
		    defaults.padding_right,
		    defaults.padding_bottom);

	l = ARG(0, number);
	t = ARG(1, number);
	r = ARG(2, number);
	b = ARG(3, number);

	if (l < 0 || t < 0 || r < 0 || b < 0)
		return cmdret_new(RET_FAILURE, "set padding: %s",
		    invalid_negative_arg);

	mark_edge_frames();

	defaults.padding_left = l;
	defaults.padding_right = r;
	defaults.padding_top = t;
	defaults.padding_bottom = b;

	list_for_each_entry(s, &rp_screens, node) {
		screen_update_workarea(s);
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_border(struct cmdarg **args)
{
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d",
		    defaults.window_border_width);

	if (ARG(0, number) < 0)
		return cmdret_new(RET_FAILURE, "set border: %s",
		    invalid_negative_arg);

	mark_edge_frames();

	defaults.window_border_width = ARG(0, number);

	list_for_each_entry(s, &rp_screens, node) {
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_onlyborder(struct cmdarg **args)
{
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.only_border);

	if (ARG(0, number) != 0 && ARG(0, number) != 1)
		return cmdret_new(RET_FAILURE, "set onlyborder: invalid argument");

	mark_edge_frames();

	defaults.only_border = ARG(0, number);

	list_for_each_entry(s, &rp_screens, node) {
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_barborder(struct cmdarg **args)
{
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.bar_border_width);

	if (ARG(0, number) < 0)
		return cmdret_new(RET_FAILURE, "set barborder: %s",
		    invalid_negative_arg);

	mark_edge_frames();

	defaults.bar_border_width = ARG(0, number);

	/* Update the frame and bar windows. */
	list_for_each_entry(s, &rp_screens, node) {
		XSetWindowBorderWidth(dpy, s->bar_window,
		    defaults.bar_border_width);
		XSetWindowBorderWidth(dpy, s->frame_window,
		    defaults.bar_border_width);
		XSetWindowBorderWidth(dpy, s->input_window,
		    defaults.bar_border_width);

		screen_update_workarea(s);
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_barinpadding(struct cmdarg **args)
{
	rp_screen *s;
	int new_value;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.bar_in_padding);

	new_value = ARG(0, number);
	if (new_value != 0 && new_value != 1)
		return cmdret_new(RET_FAILURE,
		    "set barinpadding: invalid argument");

	mark_edge_frames();

	defaults.bar_in_padding = new_value;

	list_for_each_entry(s, &rp_screens, node) {
		screen_update_workarea(s);
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_inputwidth(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.input_window_size);

	if (ARG(0, number) < 0)
		return cmdret_new(RET_FAILURE, "set inputwidth: %s",
		    invalid_negative_arg);

	defaults.input_window_size = ARG(0, number);
	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_waitcursor(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d",
		    defaults.wait_for_key_cursor);

	if (ARG(0, number) != 0 && ARG(0, number) != 1)
		return cmdret_new(RET_FAILURE,
		    "set waitcursor: invalid argument");

	defaults.wait_for_key_cursor = ARG(0, number);
	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_infofmt(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.info_fmt);

	free(defaults.info_fmt);
	defaults.info_fmt = xstrdup(ARG_STRING(0));

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_topkmap(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.top_kmap);

	if (!find_keymap(ARG_STRING(0)))
		return cmdret_new(RET_FAILURE, "Unknown keymap %s",
		    ARG_STRING(0));

	ungrab_keys_all_wins();

	free(defaults.top_kmap);
	defaults.top_kmap = xstrdup(ARG_STRING(0));

	grab_keys_all_wins();
	XSync(dpy, False);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_winfmt(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.window_fmt);

	free(defaults.window_fmt);
	defaults.window_fmt = xstrdup(ARG_STRING(0));

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_winname(struct cmdarg **args)
{
	char *name;

	if (args[0] == NULL)
		switch (defaults.win_name) {
		case WIN_NAME_TITLE:
			return cmdret_new(RET_SUCCESS, "title");
		case WIN_NAME_RES_NAME:
			return cmdret_new(RET_SUCCESS, "name");
		case WIN_NAME_RES_CLASS:
			return cmdret_new(RET_SUCCESS, "class");
		default:
			PRINT_DEBUG(("Unknown win_name\n"));
			return cmdret_new(RET_FAILURE, "unknown");
		}

	name = ARG_STRING(0);

	if (!strncmp(name, "title", sizeof("title")))
		defaults.win_name = WIN_NAME_TITLE;
	else if (!strncmp(name, "name", sizeof("name")))
		defaults.win_name = WIN_NAME_RES_NAME;
	else if (!strncmp(name, "class", sizeof("class")))
		defaults.win_name = WIN_NAME_RES_CLASS;
	else
		return cmdret_new(RET_FAILURE,
		    "set winname: invalid argument `%s'", name);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_framefmt(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.frame_fmt);

	free(defaults.frame_fmt);
	defaults.frame_fmt = xstrdup(ARG_STRING(0));

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_fgcolor(struct cmdarg **args)
{
	XColor color, junk;
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.fgcolor_string);

	list_for_each_entry(s, &rp_screens, node) {
		if (!XAllocNamedColor(dpy, s->def_cmap, ARG_STRING(0), &color,
		    &junk))
			return cmdret_new(RET_FAILURE,
			    "set fgcolor: unknown color");

		rp_glob_screen.fg_color = color.pixel | (0xff << 24);
		update_gc(s);

		if (!XftColorAllocName(dpy, DefaultVisual(dpy, s->screen_num),
		    DefaultColormap(dpy, s->screen_num), ARG_STRING(0),
		    &s->xft_fg_color))
			return cmdret_new(RET_FAILURE,
			    "set fgcolor: unknown color");

		free(defaults.fgcolor_string);
		defaults.fgcolor_string = xstrdup(ARG_STRING(0));
	}

	redraw_sticky_bar_text(1);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_bgcolor(struct cmdarg **args)
{
	XColor color, junk;
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.bgcolor_string);

	list_for_each_entry(s, &rp_screens, node) {
		if (!XAllocNamedColor(dpy, s->def_cmap, ARG_STRING(0), &color,
		    &junk))
			return cmdret_new(RET_FAILURE,
			    "set bgcolor: unknown color");

		color.pixel |= (0xff << 24);
		rp_glob_screen.bg_color = color.pixel;
		update_gc(s);
		XSetWindowBackground(dpy, s->bar_window, color.pixel);
		XSetWindowBackground(dpy, s->input_window, color.pixel);
		XSetWindowBackground(dpy, s->frame_window, color.pixel);
		XSetWindowBackground(dpy, s->help_window, color.pixel);

		if (!XftColorAllocName(dpy, DefaultVisual(dpy, s->screen_num),
		    DefaultColormap(dpy, s->screen_num), ARG_STRING(0),
		    &s->xft_bg_color))
			return cmdret_new(RET_FAILURE,
			    "set fgcolor: unknown color");

		free(defaults.bgcolor_string);
		defaults.bgcolor_string = xstrdup(ARG_STRING(0));
	}

	redraw_sticky_bar_text(1);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_fwcolor(struct cmdarg **args)
{
	XColor color, junk;
	rp_window *win = current_window();
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.fwcolor_string);

	list_for_each_entry(s, &rp_screens, node) {
		if (!XAllocNamedColor(dpy, s->def_cmap, ARG_STRING(0), &color,
		    &junk))
			return cmdret_new(RET_FAILURE,
			    "set fwcolor: unknown color");

		rp_glob_screen.fw_color = color.pixel | (0xff << 24);
		update_gc(s);

		free(defaults.fwcolor_string);
		defaults.fwcolor_string = xstrdup(ARG_STRING(0));
	}

	/* Update current window. */
	if (win != NULL)
		XSetWindowBorder(dpy, win->w, rp_glob_screen.fw_color);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_bwcolor(struct cmdarg **args)
{
	XColor color, junk;
	rp_window *win, *cur_win = current_window();
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.bwcolor_string);

	list_for_each_entry(s, &rp_screens, node) {
		if (!XAllocNamedColor(dpy, s->def_cmap, ARG_STRING(0), &color,
		    &junk))
			return cmdret_new(RET_FAILURE,
			    "set bwcolor: unknown color");

		rp_glob_screen.bw_color = color.pixel | (0xff << 24);
		update_gc(s);

		free(defaults.bwcolor_string);
		defaults.bwcolor_string = xstrdup(ARG_STRING(0));
	}

	/* Update all the visible windows. */
	list_for_each_entry(win, &rp_mapped_window, node) {
		if (win != cur_win)
			XSetWindowBorder(dpy, win->w, rp_glob_screen.bw_color);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_barbordercolor(struct cmdarg **args)
{
	XColor color, junk;
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s",
		    defaults.barbordercolor_string);

	list_for_each_entry(s, &rp_screens, node) {
		if (!XAllocNamedColor(dpy, s->def_cmap, ARG_STRING(0), &color,
		    &junk))
			return cmdret_new(RET_FAILURE,
			    "set barbordercolor: unknown color");

		color.pixel |= (0xff << 24);
		rp_glob_screen.bar_border_color = color.pixel;
		update_gc(s);
		XSetWindowBorder(dpy, s->bar_window, color.pixel);
		XSetWindowBorder(dpy, s->input_window, color.pixel);
		XSetWindowBorder(dpy, s->frame_window, color.pixel);
		XSetWindowBorder(dpy, s->help_window, color.pixel);

		free(defaults.barbordercolor_string);
		defaults.barbordercolor_string = xstrdup(ARG_STRING(0));
	}

	redraw_sticky_bar_text(1);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_vscreens(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.vscreens);

	if (ARG(0, number) < 1)
		return cmdret_new(RET_FAILURE, "vscreens: invalid argument");

	if (vscreens_resize(ARG(0, number)) != 0)
		return cmdret_new(RET_FAILURE, "vscreens: failed resizing");

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_gap(struct cmdarg **args)
{
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.gap);

	if (ARG(0, number) < 0)
		return cmdret_new(RET_FAILURE, "gap: invalid argument");

	mark_edge_frames();

	defaults.gap = ARG(0, number);

	list_for_each_entry(s, &rp_screens, node) {
		screen_update_workarea(s);
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_ignoreresizehints(struct cmdarg **args)
{
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d",
		    defaults.ignore_resize_hints);

	if (ARG(0, number) < 0 || ARG(0, number) > 1)
		return cmdret_new(RET_FAILURE,
		    "ignoreresizehints: invalid argument");

	mark_edge_frames();

	defaults.ignore_resize_hints = ARG(0, number);

	list_for_each_entry(s, &rp_screens, node) {
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_resizefmt(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.resize_fmt);

	free(defaults.resize_fmt);
	defaults.resize_fmt = xstrdup(ARG_STRING(0));

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_winaddcurvscreen(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d",
		    defaults.win_add_cur_vscreen);

	if (ARG(0, number) < 0 || ARG(0, number) > 1)
		return cmdret_new(RET_FAILURE,
		    "winaddcurvscreen: invalid argument");

	defaults.win_add_cur_vscreen = ARG(0, number);

	return cmdret_new(RET_SUCCESS, NULL);
}
cmdret *
cmd_setenv(int interactive, struct cmdarg **args)
{
	const char *var = ARG_STRING(0), *val = ARG_STRING(1);

	PRINT_DEBUG(("setenv (\"%s\", \"%s\", 1)\n", var, val));
	if (setenv(var, val, 1) == -1)
		return cmdret_new(RET_FAILURE, "cmd_setenv failed: %s",
		    strerror(errno));

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_getenv(int interactive, struct cmdarg **args)
{
	char *value;

	value = getenv(ARG_STRING(0));
	if (value)
		return cmdret_new(RET_SUCCESS, "%s", value);

	return cmdret_new(RET_FAILURE, NULL);
}

/*
 * Thanks to Gergely Nagy <algernon@debian.org> for the original patch.
 */
cmdret *
cmd_chdir(int interactive, struct cmdarg **args)
{
	const char *dir;

	if (args[0] == NULL) {
		dir = get_homedir();
		if (dir == NULL) {
			return cmdret_new(RET_FAILURE,
			    "chdir: unable to find your HOME directory");
		}
	} else
		dir = ARG_STRING(0);

	if (chdir(dir) == -1)
		return cmdret_new(RET_FAILURE, "chdir: %s: %s", dir,
		    strerror(errno));

	return cmdret_new(RET_SUCCESS, NULL);
}

/*
 * Thanks to Gergely Nagy <algernon@debian.org> for the original patch.
 */
cmdret *
cmd_unsetenv(int interactive, struct cmdarg **args)
{
	const char *var = ARG_STRING(0);

	/*
	 * Use unsetenv() where possible since putenv("FOO") is not legit
	 * everywhere
	 */
	if (unsetenv(var) == -1)
		return cmdret_new(RET_FAILURE, "cmd_unsetenv failed: %s",
		    strerror(errno));

	return cmdret_new(RET_SUCCESS, NULL);
}

/*
 * Thanks to Gergely Nagy <algernon@debian.org> for the original patch.
 */
cmdret *
cmd_info(int interactive, struct cmdarg **args)
{
	struct sbuf *buf;
	if (current_window() != NULL) {
		rp_window *win = current_window();
		rp_window_elem *win_elem;
		win_elem = vscreen_find_window(&rp_current_vscreen->mapped_windows,
		    win);
		if (!win_elem)
			win_elem = vscreen_find_window(
			    &rp_current_vscreen->unmapped_windows, win);

		if (win_elem) {
			char *s;
			cmdret *ret;

			if (args[0] == NULL)
				s = defaults.info_fmt;
			else
				s = ARG_STRING(0);
			buf = sbuf_new(0);
			format_string(s, win_elem, buf);
			ret = cmdret_new(RET_SUCCESS, "%s", sbuf_get(buf));
			sbuf_free(buf);
			return ret;
		}
	}

	return cmdret_new(RET_SUCCESS, "No window.");
}

/*
 * Thanks to Gergely Nagy <algernon@debian.org> for the original patch.
 */
cmdret *
cmd_lastmsg(int interactive, struct cmdarg **args)
{
	show_last_message();
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_focusup(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	if ((frame = find_frame_up(current_frame(rp_current_vscreen))))
		set_active_frame(frame, 0);
	else
		show_frame_indicator(0);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_focusdown(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	if ((frame = find_frame_down(current_frame(rp_current_vscreen))))
		set_active_frame(frame, 0);
	else
		show_frame_indicator(0);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_focusleft(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	if ((frame = find_frame_left(current_frame(rp_current_vscreen))))
		set_active_frame(frame, 0);
	else
		show_frame_indicator(0);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_focusright(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	if ((frame = find_frame_right(current_frame(rp_current_vscreen))))
		set_active_frame(frame, 0);
	else
		show_frame_indicator(0);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_exchangeup(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	if ((frame = find_frame_up(current_frame(rp_current_vscreen))))
		exchange_with_frame(current_frame(rp_current_vscreen), frame);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_exchangedown(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	if ((frame = find_frame_down(current_frame(rp_current_vscreen))))
		exchange_with_frame(current_frame(rp_current_vscreen), frame);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_exchangeleft(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	if ((frame = find_frame_left(current_frame(rp_current_vscreen))))
		exchange_with_frame(current_frame(rp_current_vscreen), frame);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_exchangeright(int interactive, struct cmdarg **args)
{
	rp_frame *frame;

	if ((frame = find_frame_right(current_frame(rp_current_vscreen))))
		exchange_with_frame(current_frame(rp_current_vscreen), frame);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_swap(int interactive, struct cmdarg **args)
{
	rp_frame *dest_frame;
	rp_frame *src_frame;

	dest_frame = ARG(0, frame);
	src_frame = args[1] ? ARG(1, frame) : current_frame(rp_current_vscreen);

	if (!rp_have_xrandr) {
		if (vscreen_find_frame_by_frame(src_frame->vscreen,
		    dest_frame) == NULL)
			return cmdret_new(RET_FAILURE,
			    "swap: frames on different screens");
	}
	exchange_with_frame(src_frame, dest_frame);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_restart(int interactive, struct cmdarg **args)
{
	hup_signalled = 1;
	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_startupmessage(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.startup_message);

	if (ARG(0, number) != 0 && ARG(0, number) != 1)
		return cmdret_new(RET_FAILURE,
		    "set startupmessage: invalid argument");

	defaults.startup_message = ARG(0, number);
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_focuslast(int interactive, struct cmdarg **args)
{
	rp_frame *frame = find_last_frame();

	if (frame)
		set_active_frame(frame, 0);
	else
		return cmdret_new(RET_FAILURE, "focuslast: no other frame");

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_link(int interactive, struct cmdarg **args)
{
	char *cmd = NULL;
	rp_keymap *map;

	if (args[1])
		map = ARG(1, keymap);
	else
		map = find_keymap(ROOT_KEYMAP);

	cmd = resolve_command_from_keydesc(args[0]->string, 0, map);
	if (cmd)
		return command(interactive, cmd);

	return cmdret_new(RET_SUCCESS, NULL);
}

/*
 * Thanks to Doug Kearns <djkea2@mugc.its.monash.edu.au> for the original patch.
 */
static cmdret *
set_barpadding(struct cmdarg **args)
{
	rp_screen *s;
	int x, y;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d %d", defaults.bar_x_padding,
		    defaults.bar_y_padding);

	x = ARG(0, number);
	y = ARG(1, number);

	if (x < 0 || y < 0)
		return cmdret_new(RET_FAILURE, "set barpadding: %s",
		    invalid_negative_arg);

	mark_edge_frames();

	defaults.bar_x_padding = x;
	defaults.bar_y_padding = y;

	list_for_each_entry(s, &rp_screens, node) {
		screen_update_workarea(s);
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_barsticky(struct cmdarg **args)
{
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.bar_sticky);

	if (ARG(0, number) != 0 && ARG(0, number) != 1)
		return cmdret_new(RET_FAILURE,
		    "set barsticky: invalid argument");

	mark_edge_frames();

	defaults.bar_sticky = ARG(0, number);

	list_for_each_entry(s, &rp_screens, node) {
		hide_bar(s, 0);
		screen_update_workarea(s);
		screen_update_frames(s);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_stickyfmt(struct cmdarg **args)
{
	rp_screen *s;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.sticky_fmt);

	free(defaults.sticky_fmt);
	defaults.sticky_fmt = xstrdup(ARG_STRING(0));

	list_for_each_entry(s, &rp_screens, node) {
		hide_bar(s, 0);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_alias(int interactive, struct cmdarg **args)
{
	/* Add or update the alias. */
	add_alias(ARG_STRING(0), ARG_STRING(1));
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_unalias(int interactive, struct cmdarg **args)
{
	char *tmp;
	int i;

	/* Are we updating an existing alias, or creating a new one? */
	i = find_alias_index(ARG_STRING(0));
	if (i < 0)
		return cmdret_new(RET_SUCCESS, "unalias: alias not found");

	alias_list_last--;

	/*
	 * Free the alias and put the last alias in the the space to
	 * keep alias_list from becoming sparse. This code must jump
	 * through some hoops to correctly handle the case when
	 * alias_list_last == i.
	 */
	tmp = alias_list[i].alias;
	alias_list[i].alias = xstrdup(alias_list[alias_list_last].alias);
	free(tmp);
	free(alias_list[alias_list_last].alias);

	/* Do the same for the name element. */
	tmp = alias_list[i].name;
	alias_list[i].name = xstrdup(alias_list[alias_list_last].name);
	free(tmp);
	free(alias_list[alias_list_last].name);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_nextscreen(int interactive, struct cmdarg **args)
{
	rp_screen *new_screen;
	rp_frame *new_frame;

	new_screen = screen_next();

	/* No need to go through the motions when we don't have to. */
	if (screen_count() <= 1 || new_screen == rp_current_screen)
		return cmdret_new(RET_FAILURE, "nextscreen: no other screen");

	new_frame = vscreen_get_frame(new_screen->current_vscreen,
	    new_screen->current_vscreen->current_frame);

	set_active_frame(new_frame, 1);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_prevscreen(int interactive, struct cmdarg **args)
{
	rp_screen *new_screen;
	rp_frame *new_frame;

	new_screen = screen_prev();

	/* No need to go through the motions when we don't have to. */
	if (screen_count() <= 1 || new_screen == rp_current_screen)
		return cmdret_new(RET_SUCCESS, "prevscreen: no other screen");

	new_frame = vscreen_get_frame(new_screen->current_vscreen,
	    new_screen->current_vscreen->current_frame);

	set_active_frame(new_frame, 1);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_smove(int interactive, struct cmdarg **args)
{
	rp_window *w;
	rp_screen *screen;
	int new_screen;

	if ((w = current_window()) == NULL)
		return cmdret_new(RET_FAILURE, "smove: no focused window");

	new_screen = ARG(0, number);
	if (new_screen < 0)
		return cmdret_new(RET_FAILURE, "smove: out of range");

	screen = screen_number(new_screen);
	if (!screen)
		return cmdret_new(RET_FAILURE, "smove: screen %d not found",
		    new_screen);

	if (screen == rp_current_screen)
		return cmdret_new(RET_SUCCESS, NULL);

	vscreen_move_window(screen->current_vscreen, w);
	set_active_frame(current_frame(screen->current_vscreen), 1);
	set_active_window(w);
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_sselect(int interactive, struct cmdarg **args)
{
	int new_screen;
	rp_frame *new_frame;
	rp_screen *screen;

	new_screen = ARG(0, number);
	if (new_screen < 0)
		return cmdret_new(RET_FAILURE, "sselect: out of range");

	screen = screen_number(new_screen);
	if (!screen)
		return cmdret_new(RET_FAILURE, "sselect: screen %d not found",
		    new_screen);

	if (screen == rp_current_screen)
		return cmdret_new(RET_SUCCESS, NULL);

	new_frame = vscreen_get_frame(screen->current_vscreen,
	    screen->current_vscreen->current_frame);
	set_active_frame(new_frame, 1);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_warp(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.warp);

	if (ARG(0, number) != 0 && ARG(0, number) != 1)
		return cmdret_new(RET_FAILURE, "set warp: invalid argument");

	defaults.warp = ARG(0, number);
	return cmdret_new(RET_SUCCESS, NULL);
}

/*
 * Return a new string with the frame selector or it as a string if no selector
 * exists for the number.
 */

/* Select a frame by number. */
cmdret *
cmd_fselect(int interactive, struct cmdarg **args)
{
	set_active_frame(ARG(0, frame), 1);
	if (interactive)
		return cmdret_new(RET_SUCCESS, NULL);

	return cmdret_new(RET_SUCCESS, NULL);
}

char *
fdump(rp_vscreen *vscreen)
{
	struct sbuf *dump;
	rp_frame *cur;

	dump = sbuf_new(0);

	list_for_each_entry(cur, &(vscreen->frames), node) {
		char *frameset;

		frameset = frame_dump(cur, vscreen);
		sbuf_concat(dump, frameset);
		sbuf_concat(dump, ",");
		free(frameset);
	}
	sbuf_chop(dump);

	return sbuf_free_struct(dump);
}

cmdret *
cmd_fdump(int interactively, struct cmdarg **args)
{
	rp_screen *screen;
	cmdret *ret;
	char *dump;

	if (args[0] == NULL)
		screen = rp_current_screen;
	else {
		int snum;
		snum = ARG(0, number);

		if (snum < 0)
			return cmdret_new(RET_FAILURE,
			    "fdump: invalid negative screen number");
		else {
			screen = screen_number(snum);
			if (!screen)
				return cmdret_new(RET_FAILURE,
				    "fdump: screen %d not found", snum);
		}
	}

	dump = fdump(screen->current_vscreen);
	ret = cmdret_new(RET_SUCCESS, "%s", dump);
	free(dump);

	return ret;
}

cmdret *
frestore(char *data, rp_vscreen *v)
{
	char *token;
	char *d;
	rp_frame *new, *cur;
	rp_window *win;
	struct list_head fset;
	int max = -1;
	char *nexttok = NULL;

	INIT_LIST_HEAD(&fset);

	d = xstrdup(data);
	token = strtok_r(d, ",", &nexttok);
	if (token == NULL) {
		free(d);
		return cmdret_new(RET_FAILURE,
		    "frestore: invalid frame format");
	}

	/* Build the new frame set. */
	while (token != NULL) {
		new = frame_read(token, v);
		if (new == NULL) {
			free(d);
			return cmdret_new(RET_FAILURE,
			    "frestore: invalid frame format");
		}
		list_add_tail(&new->node, &fset);
		token = strtok_r(NULL, ",", &nexttok);
	}

	free(d);

	/* Clear all the frames. */
	list_for_each_entry(cur, &v->frames, node) {
		PRINT_DEBUG(("blank %d\n", cur->number));
		blank_frame(cur);
	}

	/* Get rid of the frames' numbers */
	vscreen_free_nums(v);

	/* Splice in our new frameset. */
	vscreen_restore_frameset(v, &fset);

	/* Process the frames a bit to make sure everything lines up. */
	list_for_each_entry(cur, &v->frames, node) {
		PRINT_DEBUG(("restore %d %d\n", cur->number, cur->win_number));

		/*
		 * Grab the frame's number, but if it already exists request a
		 * new one.
		 */
		if (!numset_add_num(v->frames_numset, cur->number)) {
			cur->number = numset_request(v->frames_numset);
		}
		/* Find the current frame based on last_access. */
		if (cur->last_access > max) {
			v->current_frame = cur->number;
			max = cur->last_access;
		}
		/* Update the window the frame points to. */
		if (cur->win_number != EMPTY) {
			set_frames_window(cur,
			    find_window_number(cur->win_number));
		}
	}

	/* Show the windows in the frames. */
	list_for_each_entry(win, &rp_mapped_window, node) {
		if (win->frame_number != EMPTY) {
			maximize(win);
			unhide_window(win);
		}
	}

	set_active_frame(current_frame(v), 0);
	update_bar(v->screen);
	show_frame_indicator(0);

	PRINT_DEBUG(("Done.\n"));
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_frestore(int interactively, struct cmdarg **args)
{
	push_frame_undo(rp_current_vscreen);	/* fdump to stack */
	return frestore(ARG_STRING(0), rp_current_vscreen);
}

cmdret *
cmd_verbexec(int interactive, struct cmdarg **args)
{
	marked_message_printf(0, 0, "Running %s", ARG_STRING(0));
	spawn(ARG_STRING(0), current_frame(rp_current_vscreen));
	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_winliststyle(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s",
		    defaults.window_list_style ? "column" : "row");

	if (!strcmp("column", ARG_STRING(0)))
		defaults.window_list_style = STYLE_COLUMN;
	else if (!strcmp("row", ARG_STRING(0)))
		defaults.window_list_style = STYLE_ROW;
	else
		return cmdret_new(RET_FAILURE,
		    "set winliststyle: invalid argument");

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_vrename(int interactive, struct cmdarg **args)
{
	if (screen_find_vscreen_by_name(rp_current_screen, ARG_STRING(0), 1))
		return cmdret_new(RET_FAILURE, "vrename: duplicate vscreen name");
	vscreen_rename(rp_current_vscreen, ARG_STRING(0));

	/* Update the vscreen list. */
	update_vscreen_names(rp_current_screen);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_vselect(int interactive, struct cmdarg **args)
{
	rp_vscreen *v;

	v = find_vscreen(ARG_STRING(0));
	if (v)
		set_current_vscreen(v);
	else
		return cmd_vscreens(interactive, NULL);

	return cmdret_new(RET_SUCCESS, NULL);
}

/* Show all the vscreens, with the current one highlighted. */
cmdret *
cmd_vscreens(int interactive, struct cmdarg **args)
{
	struct sbuf *vscreen_list = NULL;
	int dummy;
	cmdret *ret;

	if (interactive) {
		rp_screen *s;
		s = rp_current_screen;
		ret = cmdret_new(RET_SUCCESS, NULL);
		if (defaults.bar_timeout == 0 && s->bar_is_raised) {
			hide_bar(s, 0);
			return ret;
		}
		show_vscreen_bar(s);
	} else {
		vscreen_list = sbuf_new(0);
		get_vscreen_list(rp_current_screen, "\n", vscreen_list, &dummy,
		    &dummy);
		ret = cmdret_new(RET_SUCCESS, "%s", sbuf_get(vscreen_list));
		sbuf_free(vscreen_list);
	}
	return ret;
}

cmdret *
cmd_vnext(int interactive, struct cmdarg **args)
{
	rp_vscreen *v;
	v = vscreen_next_vscreen(rp_current_vscreen);
	if (!v)
		return cmdret_new(RET_FAILURE, "%s", "next vscreen failed");

	set_current_vscreen(v);
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_vprev(int interactive, struct cmdarg **args)
{
	rp_vscreen *v;
	v = vscreen_prev_vscreen(rp_current_vscreen);
	if (!v)
		return cmdret_new(RET_FAILURE, "%s", "prev vscreen failed");

	set_current_vscreen(v);
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_vother(int interactive, struct cmdarg **args)
{
	rp_vscreen *v;
	v = screen_last_vscreen(rp_current_screen);
	if (!v)
		/* todo: should we just return success here so it's a no-op? */
		return cmdret_new(RET_FAILURE, "%s", "last vscreen failed");

	set_current_vscreen(v);
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_addhook(int interactive, struct cmdarg **args)
{
	struct list_head *hook;
	struct sbuf *cmd;

	hook = hook_lookup(ARG_STRING(0));
	if (hook == NULL)
		return cmdret_new(RET_FAILURE, "addhook: unknown hook '%s'",
		    ARG_STRING(0));

	/* Add the command to the hook */
	cmd = sbuf_new(0);
	sbuf_copy(cmd, ARG_STRING(1));
	hook_add(hook, cmd);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_remhook(int interactive, struct cmdarg **args)
{
	struct sbuf *cmd;

	/* Remove the command from the hook */
	cmd = sbuf_new(0);
	sbuf_copy(cmd, ARG_STRING(1));
	hook_remove(ARG(0, hook), cmd);
	sbuf_free(cmd);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_listhook(int interactive, struct cmdarg **args)
{
	cmdret *ret;
	struct sbuf *buffer;
	struct list_head *hook;
	struct sbuf *cur;

	hook = hook_lookup(ARG_STRING(0));
	if (hook == NULL)
		return cmdret_new(RET_FAILURE, "listhook: unknown hook '%s'",
		    ARG_STRING(0));

	if (list_empty(hook))
		return cmdret_new(RET_FAILURE, " Nothing defined for %s ",
		    ARG_STRING(0));

	buffer = sbuf_new(0);

	list_for_each_entry(cur, hook, node) {
		sbuf_printf_concat(buffer, "%s", sbuf_get(cur));
		if (cur->node.next != hook)
			sbuf_printf_concat(buffer, "\n");
	}

	ret = cmdret_new(RET_SUCCESS, "%s", sbuf_get(buffer));
	sbuf_free(buffer);
	return ret;
}

cmdret *
cmd_readkey(int interactive, struct cmdarg **args)
{
	char *keysym_name;
	rp_action *key_action;
	KeySym keysym;	/* Key pressed */
	unsigned int mod;	/* Modifiers */
	int rat_grabbed = 0;
	rp_keymap *map;
	cmdret *ret;

	map = ARG(0, keymap);

	/*
	 * Change the mouse icon to indicate to the user we are waiting for
	 * more keystrokes
	 */
	if (defaults.wait_for_key_cursor) {
		grab_rat();
		rat_grabbed = 1;
	}
	read_single_key(&keysym, &mod, NULL, 0);

	if (rat_grabbed)
		ungrab_rat();

	if ((key_action = find_keybinding(keysym, x11_mask_to_rp_mask(mod), map))) {
		return command(1, key_action->data);
	}

	/* No key match, notify user. */
	keysym_name = keysym_to_string(keysym, x11_mask_to_rp_mask(mod));
	ret = cmdret_new(RET_FAILURE, "readkey: unbound key '%s'", keysym_name);
	free(keysym_name);
	return ret;
}

cmdret *
cmd_newkmap(int interactive, struct cmdarg **args)
{
	rp_keymap *map;

	map = find_keymap(ARG_STRING(0));
	if (map)
		return cmdret_new(RET_FAILURE,
		    "newkmap: keymap '%s' already exists", ARG_STRING(0));

	map = keymap_new(ARG_STRING(0));
	list_add_tail(&map->node, &rp_keymaps);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_delkmap(int interactive, struct cmdarg **args)
{
	rp_keymap *map, *top, *root;

	top = find_keymap(defaults.top_kmap);
	root = find_keymap(ROOT_KEYMAP);

	map = ARG(0, keymap);
	if (map == root || map == top)
		return cmdret_new(RET_FAILURE,
		    "delkmap: cannot delete keymap '%s'", ARG_STRING(0));

	list_del(&map->node);

	return cmdret_new(RET_SUCCESS, NULL);
}

static cmdret *
set_framesels(struct cmdarg **args)
{
	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%s", defaults.frame_selectors);

	free(defaults.frame_selectors);
	defaults.frame_selectors = xstrdup(ARG_STRING(0));
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_set(int interactive, struct cmdarg **args)
{
	struct sbuf *scur;
	struct cmdarg *acur;
	struct list_head *iter, *tmp;
	struct list_head head, arglist;
	int i, nargs = 0, raw = 0;
	int parsed_args;
	cmdret *result = NULL;
	struct cmdarg **cmdargs;
	char *input;

	if (args[0] == NULL) {
		/* List all the settings. */
		cmdret *ret;
		struct sbuf *s = sbuf_new(0);
		struct set_var *cur, *last;

		list_last(last, &set_vars, node);
		list_for_each_entry(cur, &set_vars, node) {
			cmdret *r;
			r = cur->set_fn(args);
			sbuf_printf_concat(s, "%s: %s", cur->var, r->output);
			/* Skip a newline on the last line. */
			if (cur != last)
				sbuf_concat(s, "\n");
			cmdret_free(r);
		}

		/* Return the accumulated string. */
		ret = cmdret_new(RET_SUCCESS, "%s", sbuf_get(s));
		sbuf_free(s);
		return ret;
	}

	INIT_LIST_HEAD(&arglist);
	INIT_LIST_HEAD(&head);

	/* We need to tell parse_args about arg_REST and arg_SHELLCMD. */
	for (i = 0; i < ARG(0, variable)->nargs; i++)
		if (ARG(0, variable)->args[i].type == arg_REST
		    || ARG(0, variable)->args[i].type == arg_COMMAND
		    || ARG(0, variable)->args[i].type == arg_SHELLCMD
		    || ARG(0, variable)->args[i].type == arg_RAW) {
			raw = 1;
			nargs = i;
			break;
		}

	/* Parse the arguments and call the function. */
	if (args[1])
		input = xstrdup(args[1]->string);
	else
		input = xstrdup("");

	result = parse_args(input, &head, nargs, raw);
	free(input);
	if (result)
		goto failed;

	result = parsed_input_to_args(ARG(0, variable)->nargs,
	    ARG(0, variable)->args, &head, &arglist, &parsed_args, NULL);
	if (result)
		goto failed;

	/* 0 or nargs is acceptable */
	if (list_size(&arglist) > 0 &&
	    list_size(&arglist) < ARG(0, variable)->nargs) {
		result = cmdret_new(RET_FAILURE, "not enough arguments.");
		goto failed;
	} else if (list_size(&head) > ARG(0, variable)->nargs) {
		result = cmdret_new(RET_FAILURE, "cmd_set: too many args.");
		goto failed;
	}

	cmdargs = arg_array(&arglist);
	result = ARG(0, variable)->set_fn(cmdargs);
	free(cmdargs);

	/* Free the lists. */
failed:
	/* Free the parsed strings */
	list_for_each_safe_entry(scur, iter, tmp, &head, node) {
		sbuf_free(scur);
	}

	/* Free the args */
	list_for_each_safe_entry(acur, iter, tmp, &arglist, node) {
		arg_free(acur);
	}

	return result;
}

cmdret *
cmd_sfdump(int interactively, struct cmdarg **args)
{
	char screen_suffix[16];
	cmdret *ret;
	struct sbuf *dump;
	rp_frame *cur_frame;
	rp_screen *cur_screen;

	dump = sbuf_new(0);

	list_for_each_entry(cur_screen, &rp_screens, node) {
		snprintf(screen_suffix, sizeof(screen_suffix), " %d,",
		    cur_screen->number);

		list_for_each_entry(cur_frame,
		    &(cur_screen->current_vscreen->frames), node) {
			char *frameset;

			frameset = frame_dump(cur_frame,
			    cur_screen->current_vscreen);
			sbuf_concat(dump, frameset);
			sbuf_concat(dump, screen_suffix);
			free(frameset);
		}
	}

	sbuf_chop(dump);
	ret = cmdret_new(RET_SUCCESS, "%s", sbuf_get(dump));
	sbuf_free(dump);
	return ret;
}

cmdret *
cmd_sfrestore(int interactively, struct cmdarg **args)
{
	char *copy, *ptr, *token;
	rp_screen *screen;
	int out_of_screen = 0, restored = 0;

	list_for_each_entry(screen, &rp_screens, node) {
		sbuf_free(screen->scratch_buffer);
		screen->scratch_buffer = sbuf_new(0);
	}

	copy = xstrdup(ARG_STRING(0));

	token = strtok(copy, ",");
	if (token == NULL) {
		free(copy);
		return cmdret_new(RET_FAILURE,
		    "sfrestore: invalid frame format");
	}

	while (token != NULL) {
		int snum;

		/* search for end of frameset */
		ptr = token;
		while (*ptr != ')')
			ptr++;
		ptr++;

		snum = string_to_positive_int(ptr);
		screen = screen_number(snum);
		if (screen) {
			/*
			 * clobber screen number here, frestore() doesn't need
			 * it
			 */
			*ptr = '\0';
			sbuf_concat(screen->scratch_buffer, token);
			sbuf_concat(screen->scratch_buffer, ",");
		} else
			out_of_screen++;

		/* continue with next frameset */
		token = strtok(NULL, ",");
	}

	free(copy);

	/* now restore the frames for each screen */
	list_for_each_entry(screen, &rp_screens, node) {
		cmdret *ret;

		if (strlen(sbuf_get(screen->scratch_buffer)) == 0)
			continue;

		push_frame_undo(screen->current_vscreen); /* fdump to stack */

		/*
		 * XXX save the failure of each frestore and display it in case
		 * of error
		 */
		ret = frestore(sbuf_get(screen->scratch_buffer),
		    screen->current_vscreen);
		if (ret->success)
			restored++;
		cmdret_free(ret);

		sbuf_free(screen->scratch_buffer);
		screen->scratch_buffer = NULL;
	}

	if (!out_of_screen)
		return cmdret_new(RET_SUCCESS, "screens restored: %d", restored);

	return cmdret_new(RET_SUCCESS,
	    "screens restored: %d, frames out of screen: %d",
	    restored, out_of_screen);
}

cmdret *
cmd_sdump(int interactive, struct cmdarg **args)
{
	cmdret *ret;
	struct sbuf *s;
	char *tmp;
	rp_screen *cur_screen;

	s = sbuf_new(0);
	list_for_each_entry(cur_screen, &rp_screens, node) {
		tmp = screen_dump(cur_screen);
		sbuf_concat(s, tmp);
		sbuf_concat(s, ",");
		free(tmp);
	}
	sbuf_chop(s);

	ret = cmdret_new(RET_SUCCESS, "%s", sbuf_get(s));
	sbuf_free(s);
	return ret;
}

static cmdret *
set_maxundos(struct cmdarg **args)
{
	rp_frame_undo *cur;

	if (args[0] == NULL)
		return cmdret_new(RET_SUCCESS, "%d", defaults.maxundos);

	if (ARG(0, number) < 0)
		return cmdret_new(RET_FAILURE, "set maxundos: %s",
		    invalid_negative_arg);

	defaults.maxundos = ARG(0, number);

	/* Delete any superfluous undos */
	while (list_size(&rp_frame_undos) > defaults.maxundos) {
		/* Delete the oldest node */
		list_last(cur, &rp_frame_undos, node);
		del_frame_undo(cur);
	}

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_cnext(int interactive, struct cmdarg **args)
{
	rp_window *cur, *last, *win;

	cur = current_window();
	if (!cur || !cur->res_class)	/* Can't be done. */
		return cmd_next(interactive, args);

	/* CUR !in cycle list, so LAST marks last node. */
	last = vscreen_prev_window(rp_current_vscreen, cur);

	if (last)
		for (win = vscreen_next_window(rp_current_vscreen, cur);
		    win;
		    win = vscreen_next_window(rp_current_vscreen, win)) {
			if (win->res_class
			    && strcmp(cur->res_class, win->res_class)) {
				set_active_window_force(win);
				return cmdret_new(RET_SUCCESS, NULL);
			}
			if (win == last)
				break;
		}

	return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_WINDOW);
}

cmdret *
cmd_cprev(int interactive, struct cmdarg **args)
{
	rp_window *cur, *last, *win;

	cur = current_window();
	if (!cur || !cur->res_class)	/* Can't be done. */
		return cmd_next(interactive, args);

	/* CUR !in cycle list, so LAST marks last node. */
	last = vscreen_next_window(rp_current_vscreen, cur);

	if (last)
		for (win = vscreen_prev_window(rp_current_vscreen, cur);
		    win;
		    win = vscreen_prev_window(rp_current_vscreen, win)) {
			if (win->res_class
			    && strcmp(cur->res_class, win->res_class)) {
				set_active_window_force(win);
				return cmdret_new(RET_SUCCESS, NULL);
			}
			if (win == last)
				break;
		}

	return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_WINDOW);
}

cmdret *
cmd_inext(int interactive, struct cmdarg **args)
{
	rp_window *cur, *last, *win;

	cur = current_window();
	if (!cur || !cur->res_class)	/* Can't be done. */
		return cmd_next(interactive, args);

	/* CUR !in cycle list, so LAST marks last node. */
	last = vscreen_prev_window(rp_current_vscreen, cur);

	if (last)
		for (win = vscreen_next_window(rp_current_vscreen, cur);
		    win;
		    win = vscreen_next_window(rp_current_vscreen, win)) {
			if (win->res_class
			    && !strcmp(cur->res_class, win->res_class)) {
				set_active_window_force(win);
				return cmdret_new(RET_SUCCESS, NULL);
			}
			if (win == last)
				break;
		}

	return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_WINDOW);
}

cmdret *
cmd_iprev(int interactive, struct cmdarg **args)
{
	rp_window *cur, *last, *win;

	cur = current_window();
	if (!cur || !cur->res_class)	/* Can't be done. */
		return cmd_next(interactive, args);

	/* CUR !in cycle list, so LAST marks last node. */
	last = vscreen_next_window(rp_current_vscreen, cur);

	if (last)
		for (win = vscreen_prev_window(rp_current_vscreen, cur);
		    win;
		    win = vscreen_prev_window(rp_current_vscreen, win)) {
			if (win->res_class
			    && !strcmp(cur->res_class, win->res_class)) {
				set_active_window_force(win);
				return cmdret_new(RET_SUCCESS, NULL);
			}
			if (win == last)
				break;
		}

	return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_WINDOW);
}

cmdret *
cmd_cother(int interactive, struct cmdarg **args)
{
	rp_window *cur, *w = NULL;

	cur = current_window();
	if (cur)
		w = vscreen_last_window_by_class(rp_current_vscreen,
		    cur->res_class);

	if (!w)
		return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_WINDOW);
	else
		set_active_window_force(w);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_iother(int interactive, struct cmdarg **args)
{
	rp_window *cur, *w = NULL;

	cur = current_window();
	if (cur)
		w = vscreen_last_window_by_class_complement(rp_current_vscreen,
		    cur->res_class);

	if (!w)
		return cmdret_new(RET_FAILURE, "%s", MESSAGE_NO_OTHER_WINDOW);
	else
		set_active_window_force(w);

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_undo(int interactive, struct cmdarg **args)
{
	rp_frame_undo *cur;

	cur = pop_frame_undo();
	if (!cur)
		return cmdret_new(RET_FAILURE,
		    "No more undo information available");
	else {
		cmdret *ret;

		ret = frestore(cur->frames, cur->vscreen);
		return ret;
	}
}

cmdret *
cmd_redo(int interactive, struct cmdarg **args)
{
	rp_frame_undo *cur;
	cmdret *ret;

	/* The current layout goes on the undo. */
	cur = pop_frame_redo();
	if (!cur)
		return cmdret_new(RET_FAILURE,
		    "No more redo information available");

	ret = frestore(cur->frames, cur->vscreen);
	return ret;
}

cmdret *
cmd_prompt(int interactive, struct cmdarg **args)
{
	cmdret *ret;
	char *output;

	if (args[0] == NULL)
		output = get_input(MESSAGE_PROMPT_COMMAND, hist_PROMPT,
		    trivial_completions);
	else {
		char *arg_str, *prefix;

		arg_str = ARG_STRING(0);
		prefix = strchr(arg_str, ':');

		if (prefix) {
			struct sbuf *query;

			prefix++;	/* Don't return the colon. */
			query = sbuf_new(prefix - arg_str);
			sbuf_nconcat(query, arg_str, prefix - arg_str);
			output = get_more_input(sbuf_get(query), prefix,
			    hist_PROMPT, BASIC, trivial_completions);
			sbuf_free(query);
		} else {
			output = get_input(arg_str, hist_PROMPT,
			    trivial_completions);
		}
	}

	if (output == NULL)
		return cmdret_new(RET_FAILURE, NULL);	/* User aborted */

	ret = cmdret_new(RET_SUCCESS, "%s", output);
	free(output);

	return ret;
}

cmdret *
cmd_describekey(int interactive, struct cmdarg **args)
{
	char *keysym_name;
	rp_action *key_action;
	KeySym keysym;	/* Key pressed */
	unsigned int mod;	/* Modifiers */
	int rat_grabbed = 0;
	rp_keymap *map;

	map = ARG(0, keymap);

	/*
	 * Change the mouse icon to indicate to the user we are waiting for
	 * more keystrokes
	 */
	if (defaults.wait_for_key_cursor) {
		grab_rat();
		rat_grabbed = 1;
	}
	read_single_key(&keysym, &mod, NULL, 0);

	if (rat_grabbed)
		ungrab_rat();

	if ((key_action = find_keybinding(keysym, x11_mask_to_rp_mask(mod),
	    map))) {
		cmdret *ret;
		keysym_name = keysym_to_string(keysym,
		    x11_mask_to_rp_mask(mod));
		ret = cmdret_new(RET_SUCCESS, "%s bound to '%s'", keysym_name,
		    key_action->data);
		free(keysym_name);
		return ret;
	} else {
		cmdret *ret;
		/* No key match, notify user. */
		keysym_name = keysym_to_string(keysym,
		    x11_mask_to_rp_mask(mod));
		ret = cmdret_new(RET_SUCCESS, "describekey: unbound key '%s'",
		    keysym_name);
		free(keysym_name);
		return ret;
	}
}

cmdret *
cmd_dedicate(int interactive, struct cmdarg **args)
{
	rp_frame *f;

	f = current_frame(rp_current_vscreen);
	if (f == NULL)
		return cmdret_new(RET_SUCCESS, NULL);

	if (args[0] != NULL) {
		int dedicated;

		dedicated = ARG(0, number);
		if (dedicated != 0 && dedicated != 1)
			return cmdret_new(RET_FAILURE,
			    "Invalid \"dedicate\" value, use 0 or 1.");
		f->dedicated = dedicated;
	} else
		/* Just toggle it, rather than on or off. */
		f->dedicated = !(f->dedicated);

	return cmdret_new(RET_SUCCESS, "Consider this frame %s.",
	    f->dedicated ? "chaste" : "promiscuous");
}

cmdret *
cmd_putsel(int interactive, struct cmdarg **args)
{
	set_selection(ARG_STRING(0));
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_getsel(int interactive, struct cmdarg **args)
{
	char *sel;
	cmdret *ret;
	sel = get_selection();
	if (sel != NULL) {
		ret = cmdret_new(RET_SUCCESS, "%s", sel);
		free(sel);
		return ret;
	}

	return cmdret_new(RET_FAILURE, "getsel: no X11 selection");
}

cmdret *
cmd_vmove(int interactive, struct cmdarg **args)
{
	rp_vscreen *v;
	rp_window *w;

	if ((w = current_window()) == NULL)
		return cmdret_new(RET_FAILURE, "vmove: no focused window");

	if (!(v = find_vscreen(ARG_STRING(0))))
		return cmdret_new(RET_FAILURE, "vmove: invalid vscreen");

	vscreen_move_window(v, w);
	set_current_vscreen(v);
	set_active_window(w);
	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_stick(int interactive, struct cmdarg **args)
{
	rp_window *cur = current_window();

	if (cur == NULL)
		return cmdret_new(RET_FAILURE, NULL);

	cur->sticky_frame = cur->frame_number;

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_unstick(int interactive, struct cmdarg **args)
{
	rp_window *cur = current_window();

	if (cur == NULL)
		return cmdret_new(RET_FAILURE, NULL);

	cur->sticky_frame = EMPTY;

	return cmdret_new(RET_SUCCESS, NULL);
}

cmdret *
cmd_commands(int interactive, struct cmdarg **args)
{
	struct sbuf *sb;
	struct user_command *cur;
	cmdret *ret;

	sb = sbuf_new(0);
	list_for_each_entry(cur, &user_commands, node) {
		sbuf_printf_concat(sb, "%s\n", cur->name);
	}
	sbuf_chop(sb);

	ret = cmdret_new(RET_SUCCESS, "%s", sbuf_get(sb));
	sbuf_free(sb);
	return ret;
}
