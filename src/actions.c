/* Copyright (C) 2000, 2001 Shawn Betts
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

#include <unistd.h>
#include <sys/wait.h>
#include <X11/keysym.h>
#include <string.h>
#include <time.h>

#include "ratpoison.h"

static rp_action *key_actions;
static int key_actions_last;
static int key_actions_table_size;

static user_command user_commands[] = 
  { /*@begin (tag required for genrpbindings) */
    {"abort",		cmd_abort,	arg_VOID},
    {"banish",          cmd_banish,     arg_VOID},
    {"bind",		cmd_bind,	arg_VOID},
    {"time", 		cmd_time, 	arg_VOID},
    {"colon",	 	cmd_colon,	arg_STRING},
    {"curframe",        cmd_curframe,   arg_VOID},
    {"delete", 		cmd_delete, 	arg_VOID},
    {"echo", 		cmd_echo, 	arg_STRING},
    {"escape",          cmd_escape,     arg_STRING},
    {"exec", 		cmd_exec, 	arg_STRING},
    {"focus",		cmd_next_frame,	arg_VOID},
    {"focusup",		cmd_focusup,	arg_VOID},
    {"focusdown",	cmd_focusdown,	arg_VOID},
    {"focusleft",	cmd_focusleft,	arg_VOID},
    {"focusright",	cmd_focusright,	arg_VOID},
    {"focuslast",	cmd_focuslast,	arg_VOID},
    {"meta",		cmd_meta,	arg_STRING},
    {"license",		cmd_license,	arg_VOID},
    {"help",		cmd_help,	arg_VOID},
    {"hsplit",		cmd_h_split,	arg_VOID},
    {"kill", 		cmd_kill, 	arg_VOID},
    {"redisplay", 	cmd_redisplay,	arg_VOID},
    {"newwm",		cmd_newwm,	arg_STRING},
    {"next", 		cmd_next, 	arg_VOID},
    {"number", 		cmd_number, 	arg_STRING},
    {"only",            cmd_only,       arg_VOID},
    {"other", 		cmd_other, 	arg_VOID},
    {"gravity",		cmd_gravity, 	arg_STRING},
    {"prev", 		cmd_prev, 	arg_VOID},
    {"quit",		cmd_quit,	arg_VOID},
    {"remove",          cmd_remove,     arg_VOID},
    {"rudeness",	cmd_rudeness,	arg_STRING},
    {"select", 		cmd_select,	arg_STRING},
    {"source",		cmd_source,	arg_STRING},
    {"split",		cmd_h_split,	arg_VOID},
    {"title", 		cmd_rename, 	arg_STRING},
    {"unbind",		cmd_unbind,	arg_STRING},
    {"version",		cmd_version,	arg_VOID},
    {"vsplit",		cmd_v_split,	arg_VOID},
    {"windows", 	cmd_windows, 	arg_VOID},
    {"setenv",		cmd_setenv,	arg_STRING},
    {"getenv",		cmd_getenv,	arg_STRING},
    {"chdir",		cmd_chdir,	arg_STRING},
    {"unsetenv",	cmd_unsetenv,	arg_STRING},
    {"info",		cmd_info,	arg_VOID},
    {"lastmsg",		cmd_lastmsg,	arg_VOID},
    {"restart",		cmd_restart,	arg_VOID},
    {"startup_message",	cmd_startup_message, arg_STRING},
    {"link",		cmd_link,	arg_STRING},
    {"alias",		cmd_alias,	arg_STRING},
    {"unalias",		cmd_unalias,	arg_STRING},
    {"prevscreen",	cmd_prevscreen,	arg_VOID},
    {"nextscreen",	cmd_nextscreen,	arg_VOID},
    /*@end (tag required for genrpbindings) */

    /* Commands to set default behavior. */
    {"defbargravity",		cmd_defbargravity,	arg_STRING},
    {"msgwait",			cmd_msgwait,	 	arg_STRING},
    {"defborder", 		cmd_defborder, 		arg_STRING},
    {"deffont", 		cmd_deffont, 		arg_STRING},
    {"definputwidth",		cmd_definputwidth,	arg_STRING},
    {"defmaxsizegravity",	cmd_defmaxsizegravity, 	arg_STRING},
    {"defpadding", 		cmd_defpadding, 	arg_STRING},
    {"defbarborder",		cmd_defbarborder,	arg_STRING},
    {"deftransgravity",		cmd_deftransgravity,	arg_STRING},
    {"defwaitcursor",		cmd_defwaitcursor,	arg_STRING},
    {"defwinfmt", 		cmd_defwinfmt, 		arg_STRING},
    {"defwinname", 		cmd_defwinname,		arg_STRING},
    {"defwingravity",		cmd_defwingravity,	arg_STRING},
    {"deffgcolor",		cmd_deffgcolor,		arg_STRING},
    {"defbgcolor",		cmd_defbgcolor,		arg_STRING},
    {"defbarpadding", 		cmd_defbarpadding, 	arg_STRING},

    /* Commands to help debug ratpoison. */
#ifdef DEBUG
#endif

    /* the following screen commands may or may not be able to be
       implemented.  See the screen documentation for what should be
       emulated with these commands */
#if 0
    {"msgminwait",	cmd_unimplemented,	arg_VOID},
    {"nethack",		cmd_unimplemented,	arg_VOID},
    {"sleep",		cmd_unimplemented,	arg_VOID},
    {"stuff", 		cmd_unimplemented, 	arg_VOID},
#endif
    {0,			0,		0} };

typedef struct
{
  char *name;
  char *alias;
} alias_t;
  
static alias_t *alias_list;
static int alias_list_size;
static int alias_list_last;

rp_action*
find_keybinding_by_action (char *action)
{
  int i;

  for (i=0; i<key_actions_last; i++)
    {
      if (!strcmp (key_actions[i].data, action))
	{
	  return &key_actions[i];
	}
    }

  return NULL;
}

rp_action*
find_keybinding (KeySym keysym, int state)
{
  int i;
  for (i = 0; i < key_actions_last; i++)
    {
      if (key_actions[i].key == keysym 
	  && key_actions[i].state == state)
	return &key_actions[i];
    }
  return NULL;
}

static char *
find_command_by_keydesc (char *desc)
{
  int i = 0;
  char *keysym_name;

  while (i < key_actions_last)
    {
      keysym_name = keysym_to_string (key_actions[i].key, key_actions[i].state);
      if (!strcmp (keysym_name, desc))
        {
          free (keysym_name);
          return key_actions[i].data;
        }
      free (keysym_name);
      i++;
    }

  return NULL;
}

static char *
resolve_command_from_keydesc (char *desc, int depth)
{
  char *cmd, *command;

  command = find_command_by_keydesc (desc);
  if (!command)
    return NULL;

  /* is it a link? */
  if (strncmp (command, "link", 4) || depth > MAX_LINK_DEPTH)
    /* it is not */
    return command;

  cmd = resolve_command_from_keydesc (&command[5], depth + 1);
  return (cmd != NULL) ? cmd : command;
}


static void
add_keybinding (KeySym keysym, int state, char *cmd)
{
  if (key_actions_last >= key_actions_table_size)
    {
      /* double the key table size */
      key_actions_table_size *= 2;
      key_actions = (rp_action*) xrealloc (key_actions, sizeof (rp_action) * key_actions_table_size);
      PRINT_DEBUG ("realloc()ed key_table %d\n", key_actions_table_size);
    }

  key_actions[key_actions_last].key = keysym;
  key_actions[key_actions_last].state = state;
  key_actions[key_actions_last].data = xstrdup (cmd); /* free this on
							 shutdown, or
							 re/unbinding */

  ++key_actions_last;
}

static void
replace_keybinding (rp_action *key_action, char *newcmd)
{
  if (strlen (key_action->data) < strlen (newcmd))
    key_action->data = (char*) realloc (key_action->data, strlen (newcmd) + 1);

  strcpy (key_action->data, newcmd);
}

static int
remove_keybinding (KeySym keysym, int state)
{
  int i;
  int found = -1;

  for (i=0; i<key_actions_last; i++)
    {
      if (key_actions[i].key == keysym && key_actions[i].state == state)
	{
	  found = i;
	  break;
	}
    }

  if (found >= 0)
    {
      free (key_actions[found].data);

      memmove (&key_actions[found], &key_actions[found+1], 
	       sizeof (rp_action) * (key_actions_last - found - 1));
      key_actions_last--;
      
      return 1;
    }

  return 0;
}

void
initialize_default_keybindings (void)
{
  key_actions_table_size = 1;
  key_actions = (rp_action*) xmalloc (sizeof (rp_action) * key_actions_table_size);
  key_actions_last = 0;

  /* Initialive the alias list. */
  alias_list_size = 5;
  alias_list_last = 0;
  alias_list = xmalloc (sizeof (alias_t) * alias_list_size);

  prefix_key.sym = KEY_PREFIX;
  prefix_key.state = MODIFIER_PREFIX;

  add_keybinding (prefix_key.sym, prefix_key.state, "other");
  add_keybinding (prefix_key.sym, 0, "meta");
  add_keybinding (XK_g, RP_CONTROL_MASK, "abort");
  add_keybinding (XK_0, 0, "select 0");
  add_keybinding (XK_1, 0, "select 1");
  add_keybinding (XK_2, 0, "select 2");
  add_keybinding (XK_3, 0, "select 3");
  add_keybinding (XK_4, 0, "select 4");
  add_keybinding (XK_5, 0, "select 5");
  add_keybinding (XK_6, 0, "select 6");
  add_keybinding (XK_7, 0, "select 7");
  add_keybinding (XK_8, 0, "select 8");
  add_keybinding (XK_9, 0, "select 9");
  add_keybinding (XK_minus, 0, "select -");
  add_keybinding (XK_A, 0, "title");
  add_keybinding (XK_A, RP_CONTROL_MASK, "title");
  add_keybinding (XK_K, 0, "kill");
  add_keybinding (XK_K, RP_CONTROL_MASK, "kill");
  add_keybinding (XK_Return, 0, "next");
  add_keybinding (XK_Return, RP_CONTROL_MASK,	"next");
  add_keybinding (XK_a, 0, "time");
  add_keybinding (XK_a, RP_CONTROL_MASK, "time");
  add_keybinding (XK_b, 0, "banish");
  add_keybinding (XK_b, RP_CONTROL_MASK, "banish");
  add_keybinding (XK_c, 0, "exec " TERM_PROG);
  add_keybinding (XK_c, RP_CONTROL_MASK, "exec " TERM_PROG);
  add_keybinding (XK_colon, 0, "colon");
  add_keybinding (XK_exclam, 0, "exec");
  add_keybinding (XK_exclam, RP_CONTROL_MASK, "colon exec " TERM_PROG " -e ");
  add_keybinding (XK_i, 0, "info");
  add_keybinding (XK_i, RP_CONTROL_MASK, "info");
  add_keybinding (XK_k, 0, "delete");
  add_keybinding (XK_k, RP_CONTROL_MASK, "delete");
  add_keybinding (XK_l, 0, "redisplay");
  add_keybinding (XK_l, RP_CONTROL_MASK, "redisplay");
  add_keybinding (XK_m, 0, "lastmsg");
  add_keybinding (XK_m, RP_CONTROL_MASK, "lastmsg");
  add_keybinding (XK_n, 0, "next");
  add_keybinding (XK_n, RP_CONTROL_MASK, "next");
  add_keybinding (XK_p, 0, "prev");
  add_keybinding (XK_p, RP_CONTROL_MASK, "prev");
  add_keybinding (XK_quoteright, 0, "select");
  add_keybinding (XK_quoteright, RP_CONTROL_MASK, "select");
  add_keybinding (XK_space, 0, "next");
  add_keybinding (XK_space, RP_CONTROL_MASK, "next");
  add_keybinding (XK_v, 0, "version");
  add_keybinding (XK_v, RP_CONTROL_MASK, "version");
  add_keybinding (XK_V, 0, "license");
  add_keybinding (XK_V, RP_CONTROL_MASK, "license");
  add_keybinding (XK_w, 0, "windows");
  add_keybinding (XK_w, RP_CONTROL_MASK, "windows");
  add_keybinding (XK_s, 0, "split");
  add_keybinding (XK_s, RP_CONTROL_MASK, "split");
  add_keybinding (XK_S, 0, "vsplit");
  add_keybinding (XK_S, RP_CONTROL_MASK, "vsplit");
  add_keybinding (XK_Tab, 0, "focus");
  add_keybinding (XK_Tab, RP_META_MASK, "focuslast");
  add_keybinding (XK_Q, 0, "only");
  add_keybinding (XK_R, 0, "remove");
  add_keybinding (XK_f, 0, "curframe");
  add_keybinding (XK_f, RP_CONTROL_MASK, "curframe");
  add_keybinding (XK_question, 0, "help");
}

/* return a KeySym from a string that contains either a hex value or
   an X keysym description */
static int string_to_keysym (char *str) 
{ 
  int retval; 
  int keysym;

  retval = sscanf (str, "0x%x", &keysym);

  if (!retval || retval == EOF)
    keysym = XStringToKeysym (str);

  return keysym;
}

/* Parse a key description. 's' is, naturally, the key description. */
struct rp_key*
parse_keydesc (char *s)
{
  static struct rp_key key;
  struct rp_key *p = &key;
  char *token, *next_token, *keydesc;

  if (s == NULL) 
    return NULL;

  /* Avoid mangling s. */
  keydesc = xstrdup (s);

  p->state = 0;
  p->sym = 0;

  if (!strchr (keydesc, '-'))
    {
      /* Its got no hyphens in it, so just grab the keysym */
      p->sym = string_to_keysym (keydesc);
    }
  else
    {
      /* Its got hyphens, so parse out the modifiers and keysym */
      token = strtok (keydesc, "-");

      if (token == NULL)
	{
	  /* It was nothing but hyphens */
	  free (keydesc);
	  return NULL;
	}

      do
	{
	  next_token = strtok (NULL, "-");

	  if (next_token == NULL)
	    {
	      /* There is nothing more to parse and token contains the
		 keysym name. */
	      p->sym = string_to_keysym (token);
	    }
	  else
	    {
	      /* Which modifier is it? Only accept modifiers that are
		 present. ie don't accept a hyper modifier if the keymap
		 has no hyper key. */
	      if (!strcmp (token, "C"))
		{
		  p->state |= RP_CONTROL_MASK;
		}
	      else if (!strcmp (token, "M"))
		{
		  p->state |= RP_META_MASK;
		}
	      else if (!strcmp (token, "A"))
		{
		  p->state |= RP_ALT_MASK;
		}
	      else if (!strcmp (token, "S"))
		{
		  p->state |= RP_SUPER_MASK;
		}
	      else if (!strcmp (token, "H"))
		{
		  p->state |= RP_HYPER_MASK;
		}
	      else
		{
		  free (keydesc);
		  return NULL;
		}
	    }

	  token = next_token;
	} while (next_token != NULL);
    }

  free (keydesc);

  if (!p->sym)
    return NULL;
  else
    return p;
}

char *
cmd_bind (int interactive, void *data)
{
  char *keydesc;
  char *cmd;

  if (!data)
    {
      message (" bind: at least one argument required ");
      return NULL;
    }

  keydesc = (char*) xmalloc (strlen (data + 1));
  sscanf (data, "%s", keydesc);
  cmd = data + strlen (keydesc);

  /* Gobble remaining whitespace before command starts */
  while (*cmd == ' ')
    {
      cmd++;
    }
  
  if (!keydesc)
    message (" bind: at least one argument required ");
  else
    {
      if (!cmd || !*cmd)
	{
	  /* If no comand is specified, then unbind the key. */
	  cmd_unbind (interactive, keydesc);
	}
      else
	{
	  struct rp_key *key = parse_keydesc (keydesc);
	  
	  if (key)
	    {
	      rp_action *key_action;

	      if ((key_action = find_keybinding (key->sym, key->state)))
		replace_keybinding (key_action, cmd);
	      else
		add_keybinding (key->sym, key->state, cmd);
	    }
	  else
	    message (" bind: could not parse key description ");
	}
    }

  free (keydesc);

  return NULL;
}

char *
cmd_unbind (int interactive, void *data)
{
  struct rp_key *key;
  char *keydesc;

  if (!data)
    {
      message (" unbind: one argument required ");
      return NULL;
    }

  keydesc = (char*) xmalloc (strlen (data + 1));
  sscanf (data, "%s", keydesc);
  key = parse_keydesc (keydesc);

  if (key)
    {
      if (!remove_keybinding (key->sym, key->state))
	marked_message_printf (0, 0, " %s unbound key ", keydesc);
    }
  else
    {
      marked_message_printf (0, 0, " %s unknown key ", keydesc);
    }

  free (keydesc);
  return NULL;
}

char *
cmd_unimplemented (int interactive, void *data)
{
  marked_message (" FIXME:  unimplemented command ",0,8);

  return NULL;
}

char *
cmd_source (int interactive, void *data)
{
  FILE *fileptr;

  if ((fileptr = fopen ((char*) data, "r")) == NULL)
    message (" source: error opening file ");
  else
    {
      read_rc_file (fileptr);
      fclose (fileptr);
    }

  return NULL;
}

char *
cmd_meta (int interactive, void *data)
{
  XEvent ev1, ev;
  ev = rp_current_event;

  if (current_window() == NULL) return NULL;

  PRINT_DEBUG ("type==%d\n", ev.xkey.type);
  PRINT_DEBUG ("serial==%ld\n", ev.xkey.serial);
  PRINT_DEBUG ("send_event==%d\n", ev.xkey.send_event);
  PRINT_DEBUG ("display=%p\n", ev.xkey.display);
  /*   PRINT_DEBUG ("root==%x  ???\n", ev.xkey.root); */
  /*   PRINT_DEBUG ("window==%x  ???\n", ev.xkey.window); */
  /*   PRINT_DEBUG ("subwindow==%x  ???\n", ev.xkey.subwindow); */
  PRINT_DEBUG ("time==%ld\n", ev.xkey.time);
  PRINT_DEBUG ("x==%d  y==%d\n", ev.xkey.x, ev.xkey.y);
  PRINT_DEBUG ("x_root==%d  y_root==%d\n", ev.xkey.x_root, ev.xkey.y_root);
  PRINT_DEBUG ("state==%d\n", ev.xkey.state);
  PRINT_DEBUG ("keycode==%d\n", ev.xkey.keycode);
  PRINT_DEBUG ("same_screen=%d\n", ev.xkey.same_screen);

  /* I am not sure which of the following fields I have to fill in or
     what to fill them in with (rcy) I wouldnt be suprised if this
     breaks in some cases.  */

  ev1.xkey.type = KeyPress;
  /*   ev1.xkey.serial =  */
  /*   ev1.xkey.send_event = */
  ev1.xkey.display = dpy;
  /*   ev1.xkey.root =  */
  ev1.xkey.window = current_window()->w;
  /*   ev1.xkey.subwindow =  */
  /*   ev1.xkey.time = ev.xkey.time; */
  /*   ev1.xkey.x == */
  /*   ev1.xkey.y == */
  /*   ev1.xkey.x_root == */
  /*   ev1.xkey.y_root == */

  ev1.xkey.state = rp_mask_to_x11_mask (prefix_key.state);
  ev1.xkey.keycode = XKeysymToKeycode (dpy, prefix_key.sym);

  XSendEvent (dpy, current_window()->w, False, KeyPressMask, &ev1);

  /*   XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, 't'), True, 0); */

  XSync (dpy, False);

  return NULL;
}

char *
cmd_prev (int interactive, void *data)
{
  rp_window *w;

  /* If the current frame is empty find the last accessed window and
     put it in the frame */
  if (!current_window())
    {
      set_active_window (find_window_other());
      if (!current_window())
	message (MESSAGE_NO_MANAGED_WINDOWS);

      return NULL;
    }
  else
    {
      w = find_window_prev (current_window());

      if (!w)
	message (MESSAGE_NO_OTHER_WINDOW);
      else
	set_active_window (w);
    }

  return NULL;
}

char *
cmd_prev_frame (int interactive, void *data)
{
  rp_window_frame *frame;

  frame = find_frame_prev (current_screen()->rp_current_frame);
  if (!frame)
    message (MESSAGE_NO_OTHER_WINDOW);
  else
    set_active_frame (frame);

  return NULL;
}

char *
cmd_next (int interactive, void *data)
{
  rp_window *w;

  /* If the current frame is empty find the last accessed window and
     put it in the frame */
  if (!current_window())
    {
      set_active_window (find_window_other());
      if (!current_window())
	message (MESSAGE_NO_MANAGED_WINDOWS);

      return NULL;
    }
  else 
    {
      w = find_window_next (current_window());

      if (!w)
	message (MESSAGE_NO_OTHER_WINDOW);
      else
	set_active_window (w);
    }

  return NULL;
}

char *
cmd_next_frame (int interactive, void *data)
{
  rp_window_frame *frame;

  frame = find_frame_next (current_screen()->rp_current_frame);
  if (!frame)
    message (MESSAGE_NO_OTHER_FRAME);
  else
    set_active_frame (frame);

  return NULL;
}

char *
cmd_other (int interactive, void *data)
{
  rp_window *w;

  w = find_window_other ();

  if (!w)
    message (MESSAGE_NO_OTHER_WINDOW);
  else
    set_active_window (w);

  return NULL;
}

static int
string_to_window_number (char *str)
{
  int i;
  char *s;

  for (i = 0, s = str; *s; s++)
    {
      if (*s < '0' || *s > '9')
        break;
      i = i * 10 + (*s - '0');
    }

  return *s ? -1 : i;
}

/* switch to window number or name */
char *
cmd_select (int interactive, void *data)
{
  char *str;
  int n;
  rp_window *w;

  if (data == NULL)
    str = get_input (MESSAGE_PROMPT_SWITCH_TO_WINDOW);
  else
    str = xstrdup ((char *) data);

  /* User aborted. */
  if (str == NULL)
    return NULL;

  /* Only search if the string contains something to search for. */
  if (strlen (str) > 0)
    {
      if (strlen (str) == 1 && str[0] == '-')
	{
	  blank_frame (current_screen()->rp_current_frame);
	}
/*       else if ((w = find_window_name (str))) */
/* 	{ */
/* 	  goto_window (w); */
/* 	} */
      /* try by number */
      else if ((n = string_to_window_number (str)) >= 0)
	{
	  if ((w = find_window_number (n)))
	    goto_window (w);
	  else
	    /* show the window list as feedback */
	    show_bar (current_screen ());
	}
      else
	/* try by name */
	{
	  if ((w = find_window_name (str)))
	    goto_window (w);
	  else
	    /* we need to format a string that includes the str */
	    message (" no window by that name ");
	}
    }

  free (str);

  return NULL;
}

char *
cmd_rename (int interactive, void *data)
{
  char *winname;
  
  if (current_window() == NULL) return NULL;

  if (data == NULL)
    winname = get_input (MESSAGE_PROMPT_NEW_WINDOW_NAME);
  else
    winname = xstrdup ((char *) data);

  /* User aborted. */
  if (winname == NULL)
    return NULL;

  if (*winname)
    {
      free (current_window()->user_name);
      current_window()->user_name = xmalloc (sizeof (char) * strlen (winname) + 1);

      strcpy (current_window()->user_name, winname);

      current_window()->named = 1;
  
      /* Update the program bar. */
      update_window_names (current_screen());
    }

  free (winname);

  return NULL;
}


char *
cmd_delete (int interactive, void *data)
{
  XEvent ev;
  int status;

  if (current_window() == NULL) return NULL;

  ev.xclient.type = ClientMessage;
  ev.xclient.window = current_window()->w;
  ev.xclient.message_type = wm_protocols;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = wm_delete;
  ev.xclient.data.l[1] = CurrentTime;

  status = XSendEvent(dpy, current_window()->w, False, 0, &ev);
  if (status == 0)
    PRINT_DEBUG ("Delete window failed\n");

  return NULL;
}

char *
cmd_kill (int interactive, void *data)
{
  if (current_window() == NULL) return NULL;

  XKillClient(dpy, current_window()->w);

  return NULL;
}

char *
cmd_version (int interactive, void *data)
{
  message (" " PACKAGE " " VERSION " ");
  return NULL;
}

char *
command (int interactive, char *data)
{
  /* This static counter is used to exit from recursive alias calls. */
  static int alias_recursive_depth = 0;
  char *result = NULL;
  char *cmd, *rest;
  char *input;
  user_command *uc;
  int i;
  
  if (data == NULL)
    return NULL;

  /* get a writable copy for strtok() */
  input = xstrdup ((char *) data);

  cmd = strtok (input, " ");

  if (cmd == NULL)
    goto done;

  rest = strtok (NULL, "\0");

  /* Gobble whitespace */
  if (rest)
    {
      while (*rest == ' ') 
	rest++;
      /* If rest is empty, then we have no argument. */
      if (*rest == '\0')
	rest = NULL;
    }

  PRINT_DEBUG ("cmd==%s rest==%s\n", cmd, (char*)rest);

  /* Look for it in the aliases, first. */
  for (i=0; i<alias_list_last; i++)
    {
      if (!strcmp (cmd, alias_list[i].name))
	{
	  struct sbuf *s;

	  /* Append any arguments onto the end of the alias' command. */
	  s = sbuf_new (0);
	  sbuf_concat (s, alias_list[i].alias);
	  if (rest != NULL)
	    sbuf_printf_concat (s, " %s", rest);

	  alias_recursive_depth++;
	  if (alias_recursive_depth >= MAX_ALIAS_RECURSIVE_DEPTH)
	    message (" command: alias recursion has exceeded maximum depth ");
	  else
	    result = command (interactive, sbuf_get (s));
	  alias_recursive_depth--;

	  sbuf_free (s);
	  goto done;
	}
    }

  /* If it wasn't an alias, maybe its a command. */
  for (uc = user_commands; uc->name; uc++)
    {
      if (!strcmp (cmd, uc->name))
	{
	  result = uc->func (interactive, rest);
	  goto done;
	}
    }

  marked_message_printf (0, 0, MESSAGE_UNKNOWN_COMMAND, cmd);

 done:
  free (input);

  return result;
}

char *
cmd_colon (int interactive, void *data)
{
  char *result;
  char *input;

  if (data == NULL)
    input = get_input (MESSAGE_PROMPT_COMMAND);
  else
    input = get_more_input (MESSAGE_PROMPT_COMMAND, data);

  /* User aborted. */
  if (input == NULL)
    return NULL;

  result = command (1, input);

  /* Gobble the result. */
  if (result)
    free (result);
  
  free (input);

  return NULL;
}

char *
cmd_exec (int interactive, void *data)
{
  char *cmd;

  if (data == NULL)
    cmd = get_input (MESSAGE_PROMPT_SHELL_COMMAND);
  else
    cmd = xstrdup ((char *) data);

  /* User aborted. */
  if (cmd == NULL)
    return NULL;

  spawn (cmd);

  free (cmd);

  return NULL;
}


void
spawn(void *data)
{
  char *cmd = data;
  /* Ugly dance to avoid leaving zombies.  Could use SIGCHLD, but it's
     not very portable. */
  if (fork() == 0) 
    {
      if (fork() == 0) 
	{
	  /* Some process setup to make sure the spawned process runs
	     in its own session. */
	  putenv(current_screen()->display_string);
#ifdef HAVE_SETSID
	  setsid();
#endif
#if defined (HAVE_SETPGID)
	  setpgid (0, 0);
#elif defined (HAVE_SETPGRP)
	  setpgrp (0, 0);
#endif
	  execl("/bin/sh", "sh", "-c", cmd, 0);
	  _exit(EXIT_FAILURE);
	}
      _exit(EXIT_SUCCESS);
    }
  wait((int *) 0);
  PRINT_DEBUG ("spawned %s\n", cmd);
}

/* Switch to a different Window Manager. Thanks to 
"Chr. v. Stuckrad" <stucki@math.fu-berlin.de> for the patch. */
char *
cmd_newwm(int interactive, void *data)
{
  char *prog;

  if (data == NULL)
    prog = get_input (MESSAGE_PROMPT_SWITCH_WM);
  else
    prog = xstrdup ((char *) data);

  /* User aborted. */
  if (prog == NULL)
    return NULL;

  PRINT_DEBUG ("Switching to %s\n", prog);

  putenv(current_screen()->display_string);
  execlp(prog, prog, 0);

  PRINT_ERROR ("exec %s ", prog);
  perror(" failed");

  free (prog);

  return NULL;
}

char *
cmd_quit(int interactive, void *data)
{
  kill_signalled = 1;
  return NULL;
}

/* Show the current time on the bar. Thanks to Martin Samuelsson
   <cosis@lysator.liu.se> for the patch. Thanks to Jonathan Walther
   <krooger@debian.org> for making it pretty. */
char *
cmd_time (int interactive, void *data)
{
  char *msg, *tmp;
  time_t timep;

  timep = time(NULL);
  tmp = ctime(&timep);
  msg = xmalloc (strlen (tmp));
  strncpy(msg, tmp, strlen (tmp) - 1);	/* Remove the newline */
  msg[strlen(tmp) - 1] = 0;

  marked_message_printf (0, 0, " %s ", msg);
  free (msg);
  
  return NULL;
}

/* Assign a new number to a window ala screen's number command. Thanks
   to Martin Samuelsson <cosis@lysator.liu.se> for the original
   patch. */
char *
cmd_number (int interactive, void *data)
{
  int old_number, new_number;
  rp_window *other_win, *win;
  char *str;
  char *tmp;

  if (data == NULL)
    {
      print_window_information (current_window());
      return NULL;
    }
  else
    {
      str = xstrdup ((char *) data);
    }

  tmp = strtok (str, " ");
  if (tmp)
    {
      new_number = string_to_window_number (tmp);
      if (new_number < 0)
	{
	  message (" number:  Bad argument ");
	  free (str);
	  return NULL;
	}
    }
  else
    {
      /* Impossible, but we'll live with it. */
      print_window_information (current_window());
      free (str);
      return NULL;
    }      

  /* Get the rest of the string and see if the user specified a target
     window. */
  tmp = strtok (NULL, "");
  if (tmp)
    {
      int win_number;

      PRINT_DEBUG ("2nd: '%s'\n", tmp); 

      win_number = string_to_window_number (tmp);
      if (win_number < 0)
	{
	  message (" number: Bad argument ");
	  free (str);
	  return NULL;
	}

      win = find_window_number (win_number);
    }
  else
    {
      PRINT_DEBUG ("2nd: NULL\n");
      win = current_window();
    }

  if ( new_number >= 0 && win)
    {
      /* Find other window with same number and give it old number. */
      other_win = find_window_number (new_number);
      if (other_win != NULL)
	{
	  old_number = win->number;
	  other_win->number = old_number;

	  /* Resort the the window in the list */
	  remove_from_list (other_win);
	  insert_into_list (other_win, rp_mapped_window_sentinel);
	}
      else
	{
	  return_window_number (win->number);
	}

      win->number = new_number;
      add_window_number (new_number);

      /* resort the the window in the list */
      remove_from_list (win);
      insert_into_list (win, rp_mapped_window_sentinel);

      /* Update the window list. */
      update_window_names (win->scr);
    }

  free (str);

  return NULL;
}

/* Toggle the display of the program bar */
char *
cmd_windows (int interactive, void *data)
{
  struct sbuf *window_list = NULL;
  char *tmp;
  int dummy;
  screen_info *s;

  if (interactive)
    {
      s = current_screen ();
      if (!hide_bar (s)) show_bar (s);
      
      return NULL;
    }
  else
    {
      window_list = sbuf_new (0);
      if (data)
	get_window_list ((char *)data, "\n", window_list, &dummy, &dummy);
      else
	get_window_list (defaults.window_fmt, "\n", window_list, &dummy, &dummy);
      tmp = sbuf_get (window_list);
      free (window_list);

      return tmp;
    }
}

char *
cmd_abort (int interactive, void *data)
{
  return NULL;
}  

/* Redisplay the current window by sending 2 resize events. */
char *
cmd_redisplay (int interactive, void *data)
{
  force_maximize (current_window());
  return NULL;
}

/* Reassign the prefix key. */
char *
cmd_escape (int interactive, void *data)
{
  rp_window *cur;
  struct rp_key *key;
  rp_action *action;

  key = parse_keydesc (data);

  if (key)
    {
      /* Update the "other" keybinding */
      action = find_keybinding(prefix_key.sym, prefix_key.state);
      if (action != NULL && !strcmp (action->data, "other"))
	{
	  action->key = key->sym;
	  action->state = key->state;
	}

      /* Update the "meta" keybinding */
      action = find_keybinding(prefix_key.sym, 0);
      if (action != NULL && !strcmp (action->data, "meta"))
	{
	  action->key = key->sym;
	  action->state = 0;
	}

      /* Remove the grab on the current prefix key */
      for (cur = rp_mapped_window_sentinel->next; 
	   cur != rp_mapped_window_sentinel; 
	   cur = cur->next)
	{
	  ungrab_prefix_key (cur->w);
	}

      prefix_key.sym = key->sym;
      prefix_key.state = key->state;

      /* Add the grab for the new prefix key */
      for (cur = rp_mapped_window_sentinel->next; 
	   cur != rp_mapped_window_sentinel; 
	   cur = cur->next)
	{
	  grab_prefix_key (cur->w);
	}
    }
  else
    {
      message (" escape: could not parse key description ");
    }

  return NULL;
}

/* User accessible call to display the passed in string. */
char *
cmd_echo (int interactive, void *data)
{
  if (data)
    marked_message_printf (0, 0, " %s ", (char *)data);

  return NULL;
}

char *
cmd_h_split (int interactive, void *data)
{
  h_split_frame (current_screen()->rp_current_frame);
  return NULL;
}

char *
cmd_v_split (int interactive, void *data)
{
  v_split_frame (current_screen()->rp_current_frame);
  return NULL;
}

char *
cmd_only (int interactive, void *data)
{
  remove_all_splits();
  maximize (current_window());

  return NULL;
}

char *
cmd_remove (int interactive, void *data)
{
  rp_window_frame *frame;

  frame = find_frame_next (current_screen()->rp_current_frame);

  if (frame)
    {
      remove_frame (current_screen()->rp_current_frame);
      set_active_frame (frame);
    }

  return NULL;
}

/* banish the rat pointer */
char *
cmd_banish (int interactive, void *data)
{
  screen_info *s;

  s = current_screen ();

  XWarpPointer (dpy, None, s->root, 0, 0, 0, 0, s->root_attr.width - 2, s->root_attr.height - 2); 
  return NULL;
}

char *
cmd_curframe (int interactive, void *data)
{
  show_frame_indicator();
  return NULL;
}

/* Thanks to Martin Samuelsson <cosis@lysator.liu.se> for the
   original patch. */
char *
cmd_license (int interactive, void *data)
{
  screen_info *s = current_screen();
  XEvent ev;
  Window fwin;			/* Window currently in focus */
  int revert;			
  int x = 10;
  int y = 10;
  int i;
  int max_width = 0;
  char *license_text[] = { PACKAGE " " VERSION,
			   "",
			   "Copyright (C) 2000, 2001 Shawn Betts",
			   "",
			   "ratpoison is free software; you can redistribute it and/or modify ",
			   "it under the terms of the GNU General Public License as published by ",
			   "the Free Software Foundation; either version 2, or (at your option) ",
			   "any later version.",
			   "",
			   "ratpoison is distributed in the hope that it will be useful, ",
			   "but WITHOUT ANY WARRANTY; without even the implied warranty of ",
			   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the ",
			   "GNU General Public License for more details.",
			   "",
			   "You should have received a copy of the GNU General Public License ",
			   "along with this software; see the file COPYING.  If not, write to ",
			   "the Free Software Foundation, Inc., 59 Temple Place, Suite 330, ",
			   "Boston, MA 02111-1307 USA",
			   "",
			   "Send bugreports, fixes, enhancements, t-shirts, money, beer & pizza ",
			   "to ratpoison-devel@lists.sourceforge.net or visit ",
			   "http://ratpoison.sourceforge.net/",
			   "",
			   "[Press any key to end.] ",
			   NULL};

  XMapRaised (dpy, s->help_window);

  XGetInputFocus (dpy, &fwin, &revert);
  XSetInputFocus (dpy, s->help_window, RevertToPointerRoot, CurrentTime);

  /* Find the longest line. */
  for(i=0; license_text[i]; i++)
    {
      int tmp;

      tmp = XTextWidth (defaults.font, license_text[i], strlen (license_text[i]));
      if (tmp > max_width)
	max_width = tmp;
    }

  /* Offset the text so its in the center. */
  x = (s->root_attr.width - max_width) / 2;
  y = (s->root_attr.height - i * FONT_HEIGHT (defaults.font)) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  /* Print the text. */
  for(i=0; license_text[i]; i++)
  {
    XDrawString (dpy, s->help_window, s->normal_gc,
	         x, y + defaults.font->max_bounds.ascent,
	         license_text[i], strlen (license_text[i]));

    y += FONT_HEIGHT (defaults.font);
  }

  /* Wait for a key press. */
  XMaskEvent (dpy, KeyPressMask, &ev);
  XUnmapWindow (dpy, s->help_window);
  XSetInputFocus (dpy, fwin, revert, CurrentTime);

  /* The help window overlaps the bar, so redraw it. */
  if (current_screen()->bar_is_raised)
    show_last_message();

  return NULL;
}

char *
cmd_help (int interactive, void *data)
{
  screen_info *s = current_screen();
  XEvent ev;
  Window fwin;			/* Window currently in focus */
  int revert;			
  int i, old_i;
  int x = 10;
  int y = 0;
  int max_width = 0;
  int drawing_keys = 1;		/* 1 if we are drawing keys 0 if we are drawing commands */
  char *keysym_name;

  XMapRaised (dpy, s->help_window);

  XGetInputFocus (dpy, &fwin, &revert);
  XSetInputFocus (dpy, s->help_window, RevertToPointerRoot, CurrentTime);

  XDrawString (dpy, s->help_window, s->normal_gc,
	       10, y + defaults.font->max_bounds.ascent,
	       "ratpoison key bindings", strlen ("ratpoison key bindings"));

  y += FONT_HEIGHT (defaults.font) * 2;

  XDrawString (dpy, s->help_window, s->normal_gc,
	       10, y + defaults.font->max_bounds.ascent,
	       "Command key: ", strlen ("Command key: "));

  keysym_name = keysym_to_string (prefix_key.sym, prefix_key.state);
  XDrawString (dpy, s->help_window, s->normal_gc,
	       10 + XTextWidth (defaults.font, "Command key: ", strlen ("Command key: ")),
	       y + defaults.font->max_bounds.ascent,
	       keysym_name, strlen (keysym_name));
  free (keysym_name);

  y += FONT_HEIGHT (defaults.font) * 2;
  
  i = 0;
  old_i = 0;
  while (i<key_actions_last || drawing_keys)
    {
      if (drawing_keys)
	{
	  keysym_name = keysym_to_string (key_actions[i].key, key_actions[i].state);

	  XDrawString (dpy, s->help_window, s->normal_gc,
		       x, y + defaults.font->max_bounds.ascent,
		       keysym_name, strlen (keysym_name));

	  if (XTextWidth (defaults.font, keysym_name, strlen (keysym_name)) > max_width)
	    max_width = XTextWidth (defaults.font, keysym_name, strlen (keysym_name));

	  free (keysym_name);
	}
      else
	{
	  XDrawString (dpy, s->help_window, s->normal_gc,
		       x, y + defaults.font->max_bounds.ascent,
		       key_actions[i].data, strlen (key_actions[i].data));

	  if (XTextWidth (defaults.font, key_actions[i].data, strlen (key_actions[i].data)) > max_width)
	    {
	      max_width = XTextWidth (defaults.font, key_actions[i].data, strlen (key_actions[i].data));
	    }
	}

      y += FONT_HEIGHT (defaults.font);
      if (y > s->root_attr.height)
	{
	  if (drawing_keys)
	    {
	      x += max_width + 10;
	      drawing_keys = 0;
	      i = old_i;
	    }
	  else
	    {
	      x += max_width + 20;
	      drawing_keys = 1;
	      old_i = i;
	    }

	  max_width = 0;
	  y = FONT_HEIGHT (defaults.font) * 4;
	}
      else
	{
	  i++;
	  if (i >= key_actions_last && drawing_keys)
	    {
	      x += max_width + 10;
	      drawing_keys = 0;
	      y = FONT_HEIGHT (defaults.font) * 4;
	      i = old_i;
	      max_width = 0;
	    }
	}
    }

  XMaskEvent (dpy, KeyPressMask, &ev);
  XUnmapWindow (dpy, s->help_window);
  XSetInputFocus (dpy, fwin, revert, CurrentTime);

  /* The help window overlaps the bar, so redraw it. */
  if (current_screen()->bar_is_raised)
    show_last_message();

  return NULL;
}

char *
cmd_rudeness (int interactive, void *data)
{
  int num;

  if (data == NULL && !interactive)
    return xsprintf ("%d", 
		     rp_honour_transient_raise
		     | (rp_honour_normal_raise << 1)
		     | (rp_honour_transient_map << 2)
		     | (rp_honour_normal_map << 3));
		     
  if (data == NULL)
    {
      message (" Rudeness level required ");
      return NULL;
    }

  if (sscanf ((char *)data, "%d", &num) < 1)
    {
      marked_message_printf (0, 0, " Bad rudeness level: %s ", (char *)data);
      return NULL;
    }

  rp_honour_transient_raise = num & 1;
  rp_honour_normal_raise = num & 2;
  rp_honour_transient_map = num & 4;
  rp_honour_normal_map = num & 8;

  return NULL;
}

static int
parse_wingravity (char *data)
{
  int ret = -1;

  if (!strcasecmp (data, "northwest") || !strcasecmp (data, "nw"))
    ret = NorthWestGravity;
  if (!strcasecmp (data, "north") || !strcasecmp (data, "n"))
    ret = NorthGravity;
  if (!strcasecmp (data, "northeast") || !strcasecmp (data, "ne"))
    ret = NorthEastGravity;
  if (!strcasecmp (data, "west") || !strcasecmp (data, "w"))
    ret = WestGravity;
  if (!strcasecmp (data, "center") || !strcasecmp (data, "c"))
    ret = CenterGravity;
  if (!strcasecmp (data, "East") || !strcasecmp (data, "e"))
    ret = EastGravity;
  if (!strcasecmp (data, "southwest") || !strcasecmp (data, "sw"))
    ret = SouthWestGravity;
  if (!strcasecmp (data, "South") || !strcasecmp (data, "s"))
    ret = SouthGravity;
  if (!strcasecmp (data, "southeast") || !strcasecmp (data, "se"))
    ret = SouthEastGravity;

  return ret;
}

static char *
wingravity_to_string (int g)
{
  switch (g)
    {
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

  PRINT_DEBUG ("Unknown gravity!\n");
  return "Unknown";
}

char *
cmd_gravity (int interactive, void *data)
{
  int gravity;
  rp_window *win;

  if (current_window() == NULL) return NULL;
  win = current_window();

  if (data == NULL && !interactive) 
    return xstrdup (wingravity_to_string (win->gravity));

  if (data == NULL
      || (gravity = parse_wingravity (data)) < 0)
    {
      message (" gravity: Unknown gravity ");
    }
  else
    {
      win->gravity = gravity;
      maximize (win);
    }

  return NULL;
}

char *
cmd_defwingravity (int interactive, void *data)
{
  int gravity;

  if (data == NULL && !interactive) 
    return xstrdup (wingravity_to_string (defaults.win_gravity));

  if (data == NULL
      || (gravity = parse_wingravity (data)) < 0)
    {
      message (" defwingravity: Unknown gravity ");
    }
  else
    {
      defaults.win_gravity = gravity;
    }

  return NULL;
}

char *
cmd_deftransgravity (int interactive, void *data)
{
  int gravity;

  if (data == NULL && !interactive)
    return xstrdup (wingravity_to_string (defaults.trans_gravity));

  if (data == NULL
      || (gravity = parse_wingravity (data)) < 0)
    {
      message (" gravity: Unknown gravity ");
    }
  else
    {
      defaults.trans_gravity = gravity;
    }

  return NULL;
}

char *
cmd_defmaxsizegravity (int interactive, void *data)
{
  int gravity;

  if (data == NULL && !interactive)
    return xstrdup (wingravity_to_string (defaults.maxsize_gravity));

  if (data == NULL
      || (gravity = parse_wingravity (data)) < 0)
    {
      message (" defmaxsizegravity: Unknown gravity ");
    }
  else
    {
      defaults.maxsize_gravity = gravity;
    }

  return NULL;
}

char *
cmd_msgwait (int interactive, void *data)
{
  int tmp;

  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.bar_timeout);
    
  if (data == NULL
      || sscanf (data, "%d", &tmp) < 1)
    {
      message (" msgwait: One argument required ");
    }

  if (tmp >= 0)
    defaults.bar_timeout = tmp;
  else
    message (" msgwait: Bad argument ");    

  return NULL;
}

char *
cmd_defbargravity (int interactive, void *data)
{
  int gravity;

  if (data == NULL && !interactive)
    return xstrdup (wingravity_to_string (defaults.bar_location));

  if (data == NULL)
    {
      message (" defbargravity: One argument required ");
      return NULL;
    }

  if ((gravity = parse_wingravity (data)) < 0)
    {
      message (" defbargravity: Bad location ");
      return NULL;
    }
  else
    {
      defaults.bar_location = gravity;
    }

  return NULL;
}

static void
update_gc (screen_info *s)
{
  XGCValues gv;

  gv.foreground = s->fg_color;
  gv.background = s->bg_color;
  gv.function = GXcopy;
  gv.line_width = 1;
  gv.subwindow_mode = IncludeInferiors;
  gv.font = defaults.font->fid;
  XFreeGC (dpy, s->normal_gc);
  s->normal_gc = XCreateGC(dpy, s->root, 
			   GCForeground | GCBackground 
			   | GCFunction | GCLineWidth
			   | GCSubwindowMode | GCFont, &gv);
}

static void
update_all_gcs ()
{
  int i;

  for (i=0; i<num_screens; i++)
    {
      update_gc (&screens[i]);
    }
}


char *
cmd_deffont (int interactive, void *data)
{
  XFontStruct *font;

  if (data == NULL) return NULL;

  font = XLoadQueryFont (dpy, (char *)data);
  if (font == NULL)
    {
      message (" deffont: Unknown font ");
      return NULL;
    }

  /* Save the font as the default. */
  XFreeFont (dpy, defaults.font);
  defaults.font = font;
  update_all_gcs();

  return NULL;
}

char *
cmd_defpadding (int interactive, void *data)
{
  rp_window_frame *frame;
  int l, t, r, b;

  if (data == NULL && !interactive)
    return xsprintf ("%d %d %d %d", 
		     defaults.padding_left,
		     defaults.padding_right,
		     defaults.padding_top,
		     defaults.padding_bottom);

  if (data == NULL
      || sscanf (data, "%d %d %d %d", &l, &t, &r, &b) < 4)
    {
      message (" defpadding: Four arguments required ");
      return NULL;
    }

  /* Resize the frames to make sure they are not too big and not too
     small. */
  for (frame = current_screen()->rp_window_frame_sentinel->next; 
       frame != current_screen()->rp_window_frame_sentinel;
       frame = frame->next)
    {
      int bk_pos, bk_len;

      /* Resize horizontally. */
      bk_pos = frame->x;
      bk_len = frame->width;

      if (frame->x == defaults.padding_left)
	{
	  frame->x = l;
	  frame->width += bk_pos - l;
	}

      if (bk_pos + bk_len == current_screen()->root_attr.width - defaults.padding_right)
	frame->width = current_screen()->root_attr.width - r - frame->x;

      /* Resize vertically. */
      bk_pos = frame->y;
      bk_len = frame->height;

      if (frame->y == defaults.padding_top)
	{
	  frame->y = t;
	  frame->height += bk_pos - t;
	}

      if (bk_pos + bk_len == current_screen()->root_attr.height - defaults.padding_bottom)
	frame->height = current_screen()->root_attr.height - b - frame->y;

      maximize_all_windows_in_frame (frame);
    }

  defaults.padding_left   = l;
  defaults.padding_right  = r;
  defaults.padding_top 	  = t;
  defaults.padding_bottom = b;

  return NULL;
}

char *
cmd_defborder (int interactive, void *data)
{
  int tmp;
  rp_window *win;

  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.window_border_width);

  if (data == NULL
      || sscanf (data, "%d", &tmp) < 1)
    {
      message (" defborder: One argument required ");
      return NULL;
    }

  if (tmp >= 0)
    defaults.window_border_width = tmp;
  else
    {
    message (" defborder: Bad argument ");
    return NULL;
    }

  /* Update all the visible windows. */
  for (win = rp_mapped_window_sentinel->next; 
       win != rp_mapped_window_sentinel; 
       win = win->next)
    {
      if (win->frame)
	maximize (win);
    }

  return NULL;
}

char *
cmd_defbarborder (int interactive, void *data)
{
  int tmp;
  int i;

  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.bar_border_width);

  if (data == NULL
      || sscanf (data, "%d", &tmp) < 1)
    {
      message (" defbarborder: One argument required ");
      return NULL;
    }

  if (tmp >= 0)
    defaults.bar_border_width = tmp;
  else
    {
      message (" defbarborder: Bad argument ");
      return NULL;
    }

  /* Update the frame and bar windows. */
  for (i=0; i<num_screens; i++)
    {
      XSetWindowBorderWidth (dpy, screens[i].bar_window, defaults.bar_border_width);
      XSetWindowBorderWidth (dpy, screens[i].frame_window, defaults.bar_border_width);
      XSetWindowBorderWidth (dpy, screens[i].input_window, defaults.bar_border_width);
    }

  return NULL;
}

char *
cmd_definputwidth (int interactive, void *data)
{
  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.input_window_size);

  if (data == NULL
      || sscanf (data, "%d", &defaults.input_window_size) < 1)
    {
      message (" definputwidth: One argument required ");
    }

  return NULL;
}

char *
cmd_defwaitcursor (int interactive, void *data)
{
  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.wait_for_key_cursor);

  if (data == NULL
      || sscanf (data, "%d", &defaults.wait_for_key_cursor) < 1)
    {
      message (" defwaitforkey: One argument required ");
    }

  return NULL;    
}

char *
cmd_defwinfmt (int interactive, void *data)
{
  if (data == NULL && !interactive)
    return xstrdup (defaults.window_fmt);

  if (data == NULL)
    return NULL;

  free (defaults.window_fmt);
  defaults.window_fmt = xstrdup (data);

  return NULL;
}

char *
cmd_defwinname (int interactive, void *data)
{
  char *name;

  if (data == NULL && !interactive)
    switch (defaults.win_name)
      {
      case 0:
	return xstrdup ("title");
      case 1:
	return xstrdup ("name");
      case 2:
	return xstrdup ("class");
      default:
	PRINT_DEBUG ("Unknown win_name\n");
	return xstrdup ("unknown");
      }

  if (data == NULL)
    {
      message (" defwinname: One argument required ");
      return NULL;
    }

  name = (char *)data;

  /* FIXME: Using strncmp is sorta dirty since `title' and
     `titlefoobar' would both match. But its quick and dirty. */
  if (!strncmp (name, "title", 5))
    defaults.win_name = 0;
  else if (!strncmp (name, "name", 4))
    defaults.win_name = 1;
  else if (!strncmp (name, "class", 5))
    defaults.win_name = 2;
  else
    message (" defwinname: Bad argument ");

  return NULL;      
}

char *
cmd_deffgcolor (int interactive, void *data)
{
  int i;
  XColor color, junk;

  if (data == NULL)
    {
      message (" deffgcolor: One argument required ");
      return NULL;
    }

  for (i=0; i<num_screens; i++)
    {
      if (!XAllocNamedColor (dpy, screens[i].def_cmap, (char *)data, &color, &junk))
	{
	  message (" deffgcolor: Unknown color ");
	  return NULL;
	}

      screens[i].fg_color = color.pixel;
      update_gc (&screens[i]);
      XSetWindowBorder (dpy, screens[i].bar_window, color.pixel);
      XSetWindowBorder (dpy, screens[i].input_window, color.pixel);
      XSetWindowBorder (dpy, screens[i].frame_window, color.pixel);
      XSetWindowBorder (dpy, screens[i].help_window, color.pixel);
    }

  return NULL;
}

char *
cmd_defbgcolor (int interactive, void *data)
{
  int i;
  XColor color, junk;

  if (data == NULL)
    {
      message (" defbgcolor: One argument required ");
      return NULL;
    }

  for (i=0; i<num_screens; i++)
    {
      if (!XAllocNamedColor (dpy, screens[i].def_cmap, (char *)data, &color, &junk))
	{
	  message (" defbgcolor: Unknown color ");
	  return NULL;
	}

      screens[i].bg_color = color.pixel;
      update_gc (&screens[i]);
      XSetWindowBackground (dpy, screens[i].bar_window, color.pixel);
      XSetWindowBackground (dpy, screens[i].input_window, color.pixel);
      XSetWindowBackground (dpy, screens[i].frame_window, color.pixel);
      XSetWindowBackground (dpy, screens[i].help_window, color.pixel);
    }

  return NULL;
}

#ifndef HAVE_SETENV
/* For systems, such as Solaris, where setenv is not implemented
 * in libc */
/* FIXME: overwrite has no effect in this implementation! */
int
setenv (const char *name, const char *value, int overwrite)
{
  char *tmp;
  int i;

  tmp = (char *)malloc (strlen(name) + strlen(value) + 2);
  strcpy(tmp, name);
  strcat(tmp, "=");
  strcat(tmp, value);

  i = putenv(tmp);

  free(tmp);

  return i;
}
#endif

#ifndef HAVE_UNSETENV
/* For systems which lack unsetenv (eg, Solaris) */
void
unsetenv (const char *name)
{
  char *tmp;

  tmp = (char *)malloc (strlen(name) + 2);
  strcpy (tmp, name);
  strcat (tmp, "=");

  putenv(tmp);

  free(tmp);
}
#endif

char *
cmd_setenv (int interactive, void *data)
{
  char *var, *string;

  if (data == NULL)
    {
      message (" setenv: Two arguments required ");
      return NULL;
    }

  /* Get the 2 arguments. */
  var = xmalloc (strlen (data) + 1);
  string = xmalloc (strlen (data) + 1);
  if (sscanf (data, "%s %s", var, string) < 2)
    {
      message (" setenv: Two arguments required ");
      free (var);
      free (string);
      return NULL;
    }

  setenv (var, string, 1);

  free (var);
  free (string);
  return NULL;
}

char *
cmd_getenv (int interactive, void *data)
{
  char *var;
  char *result = NULL;
  char *value;

  if (data == NULL)
    {
      message (" getenv: One argument required ");
      return NULL;
    }

  /* Get the 2 arguments. */
  var = xmalloc (strlen (data) + 1);
  if (sscanf (data, "%s", var) < 1)
    {
      message (" getenv: one argument required ");
      free (var);
      return NULL;
    }

  value = getenv (var);

  if (interactive)
    {
      marked_message_printf (0,0, " %s ", value);
      return NULL;
    }

  if (value)
    {
      result = xmalloc (strlen (value) + 1);
      strcpy (result, getenv (var));
    }

  return result;
}

/* Thanks to Gergely Nagy <algernon@debian.org> for the original
   patch. */
char *
cmd_chdir (int interactive, void *data)
{
  if (!data)
    {
      char *homedir = getenv("HOME");
      if (homedir)
        chdir (homedir);
    }

  chdir ((char *)data);
  return NULL;
}

/* Thanks to Gergely Nagy <algernon@debian.org> for the original
   patch. */
char *
cmd_unsetenv (int interactive, void *data)
{
  if (data == NULL)
    {
      message (" unsetenv: One argument is required ");
      return NULL;
    }

  unsetenv ((char *)data);

  return NULL;
}

/* Thanks to Gergely Nagy <algernon@debian.org> for the original
   patch. */
char *
cmd_info (int interactive, void *data)
{
  if (current_window() == NULL)
    {
      marked_message_printf (0, 0, " (%d, %d) No window ",
			     current_screen()->root_attr.width,
			     current_screen()->root_attr.height);
    }
  else
    {
      rp_window *win = current_window();
      marked_message_printf (0, 0, " (%d,%d) %d(%s) %s", win->width, win->height,
			     win->number, window_name (win), win->transient ? "Transient ":"");
    }

  return NULL;
}

/* Thanks to Gergely Nagy <algernon@debian.org> for the original
   patch. */
char *
cmd_lastmsg (int interactive, void *data)
{
  show_last_message();
  return NULL;
}

char *
cmd_focusup (int interactive, void *data)
{
  rp_window_frame *frame;

  if ((frame = find_frame_up (current_screen()->rp_current_frame)))
    set_active_frame (frame);

  return NULL;
}

char *
cmd_focusdown (int interactive, void *data)
{
  rp_window_frame *frame;

  if ((frame = find_frame_down (current_screen()->rp_current_frame)))
    set_active_frame (frame);

  return NULL;
}

char *
cmd_focusleft (int interactive, void *data)
{
  rp_window_frame *frame;

  if ((frame = find_frame_left (current_screen()->rp_current_frame)))
    set_active_frame (frame);

  return NULL;
}

char *
cmd_focusright (int interactive, void *data)
{
  rp_window_frame *frame;

  if ((frame = find_frame_right (current_screen()->rp_current_frame)))
    set_active_frame (frame);

  return NULL;
}

char *
cmd_restart (int interactive, void *data)
{
  hup_signalled = 1;
  return NULL;
}

char *
cmd_startup_message (int interactive, void *data)
{
  if (data == NULL && !interactive)
    return xsprintf ("%s", defaults.startup_message ? "on":"off");

  if (data == NULL)
    {
      message (" startup_message; one argument required ");
      return NULL;
    }

  if (!strcasecmp (data, "on"))
    defaults.startup_message = 1;
  else if (!strcasecmp (data, "off"))
    defaults.startup_message = 0;
  else
    message (" startup_message; Invalid argument ");

  return NULL;
}

char *
cmd_focuslast (int interactive, void *data)
{
  rp_window_frame *frame = find_last_frame(current_screen());

  if (frame)
      set_active_frame (frame);
  else
    message (" focuslast: No other frame ");

  return NULL;
}

char *
cmd_link (int interactive, void *data)
{
  char *cmd = NULL;

  if (!data)
    return NULL;

  cmd = resolve_command_from_keydesc ((char *)data, 0);
  if (cmd)
    return command (interactive, cmd);

  return NULL;
}

/* Thanks to Doug Kearns <djkea2@mugc.its.monash.edu.au> for the
   original patch. */
char *
cmd_defbarpadding (int interactive, void *data)
{
  int x, y;

  if (data == NULL && !interactive)
    return xsprintf ("%d %d", defaults.bar_x_padding, defaults.bar_y_padding);

  if (data == NULL
      || sscanf (data, "%d %d", &x, &y) < 2) 
    {
      message (" defbarpadding: Two arguments required ");
      return NULL;
    }

  if (x >= 0 && y >= 0)
    {
      defaults.bar_x_padding = x;
      defaults.bar_y_padding = y;
    }
  else
    {
      message (" defbarpadding: Bad argument ");    
    }
  return NULL;
}

/* Search the alias table for a match. If a match is found, return its
   index into the table. Otherwise return -1. */
static int
find_alias_index (char *name)
{
  int i;

  for (i=0; i<alias_list_last; i++)
    if (!strcmp (name, alias_list[i].name))
      return i;

  return -1;
}

char *
cmd_alias (int interactive, void *data)
{
  char *name, *alias;
  int index;
  
  if (data == NULL)
    {
      message (" alias: At least one argument required ");
      return NULL;
    }

  /* Parse out the arguments. */
  name = strtok (data, " ");
  alias = strtok (NULL, "\0");

  if (name == NULL || alias == NULL)
    {
      message (" alias: Two arguments required ");
      return NULL;
    }

  /* Are we updating an existing alias, or creating a new one? */
  index = find_alias_index (name);
  if (index >= 0)
    {
      free (alias_list[index].alias);
      alias_list[index].alias = xstrdup (alias);
    }
  else
    {
      if (alias_list_last >= alias_list_size)
	{
	  alias_list_size *= 2;
	  alias_list = xrealloc (alias_list, sizeof (alias_t) * alias_list_size);
	}

      alias_list[alias_list_last].name = xstrdup (name);
      alias_list[alias_list_last].alias = xstrdup (alias);
      alias_list_last++;
    }

  return NULL;
}

char *
cmd_unalias (int interactive, void *data)
{
  char *name;
  int index;
  
  if (data == NULL)
    {
      message (" unalias: One argument required ");
      return NULL;
    }

  /* Parse out the arguments. */
  name = (char *)data;

  /* Are we updating an existing alias, or creating a new one? */
  index = find_alias_index (name);
  if (index >= 0)
    {
      char *tmp;

      alias_list_last--;

      /* Free the alias and put the last alias in the the space to
	 keep alias_list from becoming sparse. This code must jump
	 through some hoops to correctly handle the case when
	 alias_list_last == index. */
      tmp = alias_list[index].alias;
      alias_list[index].alias = xstrdup (alias_list[alias_list_last].alias);
      free (tmp);
      free (alias_list[alias_list_last].alias);

      /* Do the same for the name element. */
      tmp = alias_list[index].name;
      alias_list[index].name = xstrdup (alias_list[alias_list_last].name);
      free (tmp);
      free (alias_list[alias_list_last].name);
    }
  else
    {
      message (" unalias: alias not found ");
    }

  return NULL;  
}

char *
cmd_nextscreen (int interactive, void *data)
{
  int new_screen;

  /* No need to go through the motions when we don't have to. */
  if (num_screens <= 1)
    {
      message (" nextscreen: No other screen ");
      return NULL;
    }
  new_screen = rp_current_screen + 1;
  if (new_screen >= num_screens)
    new_screen = 0;

  set_active_frame (screens[new_screen].rp_current_frame);

  return NULL;
}

char *
cmd_prevscreen (int interactive, void *data)
{
  int new_screen;

  /* No need to go through the motions when we don't have to. */
  if (num_screens <= 1) 
    {
      message (" prevscreen: No other screen ");
      return NULL;
    }

  new_screen = rp_current_screen - 1;
  if (new_screen < 0)
    new_screen = num_screens - 1;

  set_active_frame (screens[new_screen].rp_current_frame);

  return NULL;
}
