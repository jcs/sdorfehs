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

rp_action *key_actions;
int key_actions_last;
int key_actions_table_size;

rp_action*
find_keybinding (int keysym, int state)
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

static void
add_keybinding (int keysym, int state, char *cmd)
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
  key_actions[key_actions_last].data = strdup (cmd); /* free this on
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

void
initialize_default_keybindings (void)
{
  key_actions_table_size = 1;
  key_actions = (rp_action*) xmalloc (sizeof (rp_action) * key_actions_table_size);
  key_actions_last = 0;

  prefix_key.sym = KEY_PREFIX;
  prefix_key.state = MODIFIER_PREFIX;

  add_keybinding (prefix_key.sym, prefix_key.state, "other");
  add_keybinding (prefix_key.sym, 0, "generate");
  add_keybinding (XK_g, ControlMask, "abort");
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
  add_keybinding (XK_A, ControlMask, "title");
  add_keybinding (XK_K, 0, "kill");
  add_keybinding (XK_K, ControlMask, "kill");
  add_keybinding (XK_Return, 0, "next");
  add_keybinding (XK_Return, ControlMask,	"next");
  add_keybinding (XK_a, 0, "clock");
  add_keybinding (XK_a, ControlMask, "clock");
  add_keybinding (XK_b, 0, "banish");
  add_keybinding (XK_b, ControlMask, "banish");
  add_keybinding (XK_c, 0, "exec " TERM_PROG);
  add_keybinding (XK_c, ControlMask, "exec " TERM_PROG);
  add_keybinding (XK_colon, 0, "colon");
  add_keybinding (XK_exclam, 0, "exec");
  add_keybinding (XK_exclam, ControlMask, "colon exec " TERM_PROG " -e ");
  add_keybinding (XK_k, 0, "delete");
  add_keybinding (XK_k, ControlMask, "delete");
  add_keybinding (XK_m, 0, "maximize");
  add_keybinding (XK_m, ControlMask, "maximize");
  add_keybinding (XK_n, 0, "next");
  add_keybinding (XK_n, ControlMask, "next");
  add_keybinding (XK_p, 0, "prev");
  add_keybinding (XK_p, ControlMask, "prev");
  add_keybinding (XK_quoteright, 0, "select");
  add_keybinding (XK_quoteright, ControlMask, "select");
  add_keybinding (XK_space, 0, "next");
  add_keybinding (XK_space, ControlMask, "next");
  add_keybinding (XK_v, 0, "version");
  add_keybinding (XK_v, ControlMask, "version");
  add_keybinding (XK_w, 0, "windows");
  add_keybinding (XK_w, ControlMask, "windows");
  add_keybinding (XK_s, 0, "split");
  add_keybinding (XK_s, ControlMask, "split");
  add_keybinding (XK_S, 0, "vsplit");
  add_keybinding (XK_S, ControlMask, "vsplit");
  add_keybinding (XK_Tab, 0, "focus");
  add_keybinding (XK_Q, 0, "only");
  add_keybinding (XK_R, 0, "remove");
  add_keybinding (XK_f, 0, "curframe");
  add_keybinding (XK_f, ControlMask, "curframe");
  add_keybinding (XK_question, 0, "help");
}

user_command user_commands[] = 
  { {"abort",		cmd_abort,	arg_VOID},
    {"next", 		cmd_next, 	arg_VOID},
    {"prev", 		cmd_prev, 	arg_VOID},
    {"exec", 		cmd_exec, 	arg_STRING},
    {"select", 		cmd_select,	arg_STRING},
    {"colon",	 	cmd_colon,	arg_STRING},
    {"kill", 		cmd_kill, 	arg_VOID},
    {"delete", 		cmd_delete, 	arg_VOID},
    {"other", 		cmd_other, 	arg_VOID},
    {"windows", 	cmd_windows, 	arg_VOID},
    {"title", 		cmd_rename, 	arg_STRING},
    {"clock", 		cmd_clock, 	arg_VOID},
    {"maximize", 	cmd_maximize,	arg_VOID},
    {"newwm",		cmd_newwm,	arg_STRING},
    {"generate",	cmd_generate,	arg_STRING},	/* rename to stuff */
    {"version",		cmd_version,	arg_VOID},
    {"bind",		cmd_bind,	arg_VOID},
    {"source",		cmd_source,	arg_STRING},
    {"escape",          cmd_escape,     arg_STRING},
    {"echo", 		cmd_echo, 	arg_STRING},
    {"split",		cmd_h_split,	arg_VOID},
    {"hsplit",		cmd_h_split,	arg_VOID},
    {"vsplit",		cmd_v_split,	arg_VOID},
    {"focus",		cmd_next_frame,	arg_VOID},
    {"only",            cmd_only,       arg_VOID},
    {"remove",          cmd_remove,     arg_VOID},
    {"banish",          cmd_banish,     arg_VOID},
    {"curframe",        cmd_curframe,   arg_VOID},
    {"help",		cmd_help,	arg_VOID},
    {"quit",		cmd_quit,	arg_VOID},
    {"number", 		cmd_number, 	arg_STRING},
    {"rudeness",	cmd_rudeness,	arg_STRING},

    /* the following screen commands may or may not be able to be
       implemented.  See the screen documentation for what should be
       emulated with these commands */

    {"stuff", 		cmd_unimplemented, 	arg_VOID},
    {"hardcopy",	cmd_unimplemented,	arg_VOID},
    {"lastmsg",		cmd_unimplemented,	arg_VOID},
    {"license",		cmd_unimplemented,	arg_VOID},
    {"lockscreen",	cmd_unimplemented,	arg_VOID},
    {"meta",		cmd_unimplemented,	arg_VOID},
    {"msgwait",		cmd_unimplemented,	arg_VOID},
    {"msgminwait",	cmd_unimplemented,	arg_VOID},
    {"nethack",		cmd_unimplemented,	arg_VOID},
    {"redisplay",	cmd_unimplemented,	arg_VOID},
    {"screen",		cmd_unimplemented,	arg_VOID},
    {"setenv",		cmd_unimplemented,	arg_VOID},
    {"shell",		cmd_unimplemented,	arg_VOID},
    {"shelltitle",	cmd_unimplemented,	arg_VOID},
    {"sleep",		cmd_unimplemented,	arg_VOID},
    {"sorendition",	cmd_unimplemented,	arg_VOID},
    {"startup_message",	cmd_unimplemented,	arg_VOID},
    {0,			0,		0} };

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

struct rp_key*
parse_keydesc (char *keydesc)
{
  static struct rp_key key;
  struct rp_key *p = &key;
  char *token, *next_token;

  if (!keydesc) 
    return NULL;

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
		  p->state |= ControlMask;
		}
	      else if (!strcmp (token, "M") && rp_modifier_info.meta_mod_mask)
		{
		  p->state |= rp_modifier_info.meta_mod_mask;
		}
	      else if (!strcmp (token, "A") && rp_modifier_info.alt_mod_mask)
		{
		  p->state |= rp_modifier_info.alt_mod_mask;
		}
	      else if (!strcmp (token, "S") && rp_modifier_info.super_mod_mask)
		{
		  p->state |= rp_modifier_info.super_mod_mask;
		}
	      else if (!strcmp (token, "H") && rp_modifier_info.hyper_mod_mask)
		{
		  p->state |= rp_modifier_info.hyper_mod_mask;
		}
	      else
		{
		  return NULL;
		}
	    }

	  token = next_token;
	} while (next_token != NULL);
    }

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
	message (" bind: need a command to bind to key ");
      else
	{
	  struct rp_key *key = parse_keydesc (keydesc);
	  
	  if (key)
	    {
	      rp_action *key_action;

	      /* 	      char foo[1000]; */
	      /* 	      sprintf (foo, " %d %ld : '%s' ", key->state, key->sym, cmd); */
	      /* 	      message (foo); */

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
cmd_generate (int interactive, void *data)
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

  ev1.xkey.state = prefix_key.state;
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

  frame = find_frame_prev (rp_current_frame);
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

  frame = find_frame_next (rp_current_frame);
  if (!frame)
    message (MESSAGE_NO_OTHER_WINDOW);
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
    str = strdup ((char *) data);

  /* Only search if the string contains something to search for. */
  if (strlen (str) > 0)
    {
      if (strlen (str) == 1 && str[0] == '-')
	{
	  blank_frame (rp_current_frame);
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
    winname = strdup ((char *) data);

  if (*winname)
    {
      free (current_window()->name);
      current_window()->name = xmalloc (sizeof (char) * strlen (winname) + 1);

      strcpy (current_window()->name, winname);

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
  if (status == 0) fprintf(stderr, "ratpoison: delete window failed\n");

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
  char *result = NULL;
  char *cmd, *rest;
  char *input;

  user_command *uc;
  struct sbuf *buf = NULL;
  
  if (data == NULL)
    return NULL;

  /* get a writable copy for strtok() */
  input = strdup ((char *) data);

  cmd = strtok (input, " ");

  if (cmd == NULL)
    goto done;

  rest = strtok (NULL, "\0");

  PRINT_DEBUG ("cmd==%s rest==%s\n", cmd, (char*)rest);

  /* find the command */
  for (uc = user_commands; uc->name; uc++)
    {
      if (!strcmp (cmd, uc->name))
	{
	  result = uc->func (interactive, rest);
	  goto done;
	}
    }

  /* couldnt find the command name */
  buf = sbuf_new (strlen(MESSAGE_UNKNOWN_COMMAND + strlen (cmd) + 4));
  sbuf_copy (buf, MESSAGE_UNKNOWN_COMMAND);
  sbuf_concat (buf, "'");
  sbuf_concat (buf, cmd);
  sbuf_concat (buf, "' ");

  message (sbuf_get (buf));
  
  sbuf_free (buf);

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
    cmd = strdup ((char *) data);

  spawn (cmd);

  free (cmd);

  return NULL;
}


void
spawn(void *data)
{
  char *cmd = data;
  /*
   * ugly dance to avoid leaving zombies.  Could use SIGCHLD,
   * but it's not very portable.
   */
  if (fork() == 0) {
    if (fork() == 0) {
      putenv(DisplayString(dpy));
      execl("/bin/sh", "sh", "-c", cmd, 0);

      PRINT_ERROR ("exec '%s' ", cmd); 
      perror(" failed");

      exit(EXIT_FAILURE);
    }
    exit(0);
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
    prog = strdup ((char *) data);

  PRINT_DEBUG ("Switching to %s\n", prog);

  putenv(DisplayString(dpy));   
  execlp(prog, prog, 0);

  PRINT_ERROR ("exec %s ", prog);
  perror(" failed");

  free (prog);

  return NULL;
}

/* Quit ratpoison. Thanks to 
"Chr. v. Stuckrad" <stucki@math.fu-berlin.de> for the patch. */
char *
cmd_quit(int interactive, void *data)
{
  PRINT_DEBUG ("Exiting\n");
  clean_up ();
  exit (EXIT_SUCCESS);

  /* Never gets here. */
  return NULL;
}

/* Show the current time on the bar. Thanks to Martin Samuelsson
   <cosis@lysator.liu.se> for the patch. Thanks to Jonathan Walther
   <krooger@debian.org> for making it pretty. */
char *
cmd_clock (int interactive, void *data)
{
  char *msg, *tmp;
  time_t timep;

  timep = time(NULL);
  tmp = ctime(&timep);
  msg = xmalloc (strlen (tmp));
  strncpy(msg, tmp, strlen (tmp) - 1);	/* Remove the newline */
  msg[strlen(tmp) - 1] = 0;

  message (msg);
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
  rp_window *other_win;
  char *str;
  
  if (current_window() == NULL) return NULL;

  if (data == NULL)
    {
      print_window_information (current_window());
      return NULL;
    }
  else
    {
      str = strdup ((char *) data);
    }

  if ((new_number = string_to_window_number (str)) >= 0)
    {
      /* Find other window with same number and give it old number. */
      other_win = find_window_number (new_number);
      if (other_win != NULL)
	{
	  old_number = current_window()->number;
	  other_win->number = old_number;

	  /* Resort the the window in the list */
	  remove_from_list (other_win);
	  insert_into_list (other_win, rp_mapped_window_sentinel);
	}
      else
	{
	  return_window_number (current_window()->number);
	}

      current_window()->number = new_number;
      add_window_number (new_number);

      /* resort the the window in the list */
      remove_from_list (current_window());
      insert_into_list (current_window(), rp_mapped_window_sentinel);

      /* Update the window list. */
      update_window_names (current_screen());
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
      get_window_list ("\n", window_list, &dummy, &dummy);
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

/* Send the current window the prefix key event */
/* void */
/* cmd_generate_prefix (void *data) */
/* { */
/*   XEvent ev; */
/*   ev = *rp_current_event; */

/*   ev.xkey.window = current_window()->w; */
/*   ev.xkey.state = MODIFIER_PREFIX; */
/*   XSendEvent (dpy, current_window()->w, False, KeyPressMask, &ev); */
/*   XSync (dpy, False); */
/* } */

/* Maximize the current window. */
char *
cmd_maximize (int interactive, void *data)
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
      /* Update the "generate" keybinding */
      action = find_keybinding(prefix_key.sym, 0);
      if (action != NULL)
	{
	  action->key = key->sym;
	  action->state = 0;
	}

      /* Update the "other" keybinding */
      action = find_keybinding(prefix_key.sym, prefix_key.state);
      if (action != NULL)
	{
	  action->key = key->sym;
	  action->state = key->state;
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
  if (data) message ((char *)data);
  return NULL;
}

char *
cmd_h_split (int interactive, void *data)
{
  h_split_frame (rp_current_frame);
  return NULL;
}

char *
cmd_v_split (int interactive, void *data)
{
  v_split_frame (rp_current_frame);
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

  frame = find_frame_next (rp_current_frame);

  if (frame)
    {
      remove_frame (rp_current_frame);
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
	       10, y + s->font->max_bounds.ascent,
	       "ratpoison key bindings", strlen ("ratpoison key bindings"));

  y += FONT_HEIGHT (s->font) * 2;

  XDrawString (dpy, s->help_window, s->normal_gc,
	       10, y + s->font->max_bounds.ascent,
	       "Command key: ", strlen ("Command key: "));

  keysym_name = keysym_to_string (prefix_key.sym, prefix_key.state);
  XDrawString (dpy, s->help_window, s->normal_gc,
	       10 + XTextWidth (s->font, "Command key: ", strlen ("Command key: ")),
	       y + s->font->max_bounds.ascent,
	       keysym_name, strlen (keysym_name));
  free (keysym_name);

  y += FONT_HEIGHT (s->font) * 2;
  
  i = 0;
  old_i = 0;
  while (i<key_actions_last || drawing_keys)
    {
      if (drawing_keys)
	{
	  keysym_name = keysym_to_string (key_actions[i].key, key_actions[i].state);

	  XDrawString (dpy, s->help_window, s->normal_gc,
		       x, y + s->font->max_bounds.ascent,
		       keysym_name, strlen (keysym_name));

	  if (XTextWidth (s->font, keysym_name, strlen (keysym_name)) > max_width)
	    {
	      max_width = XTextWidth (s->font, keysym_name, strlen (keysym_name));
	    }

	  free (keysym_name);
	}
      else
	{
	  XDrawString (dpy, s->help_window, s->normal_gc,
		       x, y + s->font->max_bounds.ascent,
		       key_actions[i].data, strlen (key_actions[i].data));

	  if (XTextWidth (s->font, key_actions[i].data, strlen (key_actions[i].data)) > max_width)
	    {
	      max_width = XTextWidth (s->font, key_actions[i].data, strlen (key_actions[i].data));
	    }
	}

      y += FONT_HEIGHT (s->font);
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
	  y = FONT_HEIGHT (s->font) * 4;
	}
      else
	{
	  i++;
	  if (i >= key_actions_last && drawing_keys)
	    {
	      x += max_width + 10;
	      drawing_keys = 0;
	      y = FONT_HEIGHT (s->font) * 4;
	      i = old_i;
	      max_width = 0;
	    }
	}
    }

  XMaskEvent (dpy, KeyPressMask, &ev);
  XUnmapWindow (dpy, s->help_window);
  XSetInputFocus (dpy, fwin, revert, CurrentTime);

  return NULL;
}

char *
cmd_rudeness (int interactive, void *data)
{
  int num;

  if (data == NULL)
    {
      message ("Rudeness level required");
      return NULL;
    }

  if (sscanf ((char *)data, "%d", &num) < 1)
    {
      message ("Bad rudeness level");
      return NULL;
    }

  rp_honour_transient_raise = num & 1;
  rp_honour_normal_raise = num & 2;
  rp_honour_transient_map = num & 4;
  rp_honour_normal_map = num & 8;

  return NULL;
}
