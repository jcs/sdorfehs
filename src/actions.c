/* ratpoison actions
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

#include <unistd.h>
#include <sys/wait.h>
#include <X11/keysym.h>
#include <string.h>
#include <time.h>

#include "ratpoison.h"

#define C ControlMask
#define M Mod1Mask		/* Meta */
#define A Mod2Mask		/* Alt */
#define S Mod3Mask		/* Super */
#define H Mod4Mask		/* Hyper */

/* rp_key_prefix key_prefix[] = */
/*   { {XK_t, C, "main-prefix"}, */
/*     {0,0,NULL} }; */

rp_action key_actions[] = 
  { {XK_t, 		C, 	"other",  	command}, 
    {XK_t, 		0, 	"generate",	command},

/*     {XK_Escape,		M,	"generate",	        command}, */

    {XK_g, 		C, 	"abort",		command},

    {XK_0, 		0, 	"number 0", 		command},
    {XK_0, 		C, 	"number 0", 		command},

    {XK_1, 		0, 	"number 1", 		command},
    {XK_1, 		C, 	"number 1", 		command},

    {XK_2, 		0, 	"number 2", 		command},
    {XK_2, 		C, 	"number 2", 		command},

    {XK_3, 		0, 	"number 3", 		command},
    {XK_3, 		C, 	"number 3", 		command},

    {XK_4, 		0, 	"number 4", 		command},
    {XK_4, 		C, 	"number 4", 		command},

    {XK_5, 		0, 	"number 5", 		command},
    {XK_5, 		C, 	"number 5", 		command},

    {XK_6, 		0, 	"number 6", 		command},
    {XK_6, 		C, 	"number 6", 		command},

    {XK_7, 		0, 	"number 7", 		command},
    {XK_7, 		C, 	"number 7", 		command},

    {XK_8, 		0, 	"number 8", 		command},
    {XK_8, 		C, 	"number 8", 		command},

    {XK_9, 		0, 	"number 9", 		command},
    {XK_9, 		C, 	"number 9", 		command},

    {XK_A, 		0, 	"title", 		command},
    {XK_A, 		C, 	"title", 		command},

    {XK_K, 		0, 	"kill", 		command},    
    {XK_K, 		C, 	"kill", 		command},    

    {XK_Return,		0,	"next",			command},
    {XK_Return,		C,	"next",			command},

    {XK_a, 		0, 	"clock", 		command},
    {XK_a, 		C, 	"clock", 		command},

    {XK_c, 		0, 	"exec " TERM_PROG, 	command},
    {XK_c, 		C, 	"exec " TERM_PROG, 	command},

    {XK_colon, 		0, 	"colon",		command},

    {XK_e, 		0, 	"exec " EMACS_PROG, 	command},
    {XK_e, 		C, 	"exec " EMACS_PROG,	command},

    {XK_exclam, 	0, 	"exec", 		command},
    {XK_exclam, 	C, 	"xterm", 	command},

    {XK_k, 		0, 	"delete", 		command},  
    {XK_k, 		C, 	"delete", 		command},  

    {XK_m, 		0, 	"maximize", 		command},
    {XK_m, 		C, 	"maximize", 		command},

    {XK_n, 		0,	"next", 		command}, 
    {XK_n, 		C,	"next", 		command}, 

    {XK_p, 		0, 	"prev", 		command},   
    {XK_p, 		C, 	"prev", 		command},   

    {XK_quoteright, 	0,     	"select", 		command},
    {XK_quoteright, 	C,     	"select", 		command},

    {XK_space, 		0, 	"next", 		command},
    {XK_space, 		C, 	"next", 		command},

    {XK_v, 		0, 	"version", 		command},
    {XK_v, 		C, 	"version", 		command},

    {XK_w, 		0, 	"windows",		command},
    {XK_w, 		C, 	"windows",		command},
    {0, 		0, 	0, 			0 } };

user_command user_commands[] = 
  { {"abort",		abort_keypress,		arg_VOID},
    {"next", 		next_window, 		arg_VOID},
    {"prev", 		prev_window, 		arg_VOID},
    {"exec", 		shell_command, 		arg_STRING},
    {"xterm", 		xterm_command, 		arg_STRING},
    {"number", 		goto_window_number, 	arg_NUMBER},
    {"select", 		goto_win_by_name, 	arg_STRING},
    {"colon",	 	command,		arg_STRING},
    {"kill", 		kill_window, 		arg_VOID},
    {"delete", 		delete_window, 		arg_VOID},
    {"other", 		last_window, 		arg_VOID},
    {"windows", 	toggle_bar, 		arg_VOID},
    {"title", 		rename_current_window, 	arg_STRING},
    {"clock", 		show_clock, 		arg_VOID},
    {"maximize", 	maximize, 		arg_VOID},
    {"newwm",		switch_to,		arg_STRING},
    {"generate",	generate_key_event,	arg_STRING},
    {"version",		show_version,		arg_VOID},
    {0,			0,			0} };


void
generate_key_event (void *data)
{
  XEvent ev1, ev;
  ev = *rp_current_event;

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
  ev1.xkey.window = rp_current_window->w;
/*   ev1.xkey.subwindow =  */
/*   ev1.xkey.time = ev.xkey.time; */
/*   ev1.xkey.x == */
/*   ev1.xkey.y == */
/*   ev1.xkey.x_root == */
/*   ev1.xkey.y_root == */

  ev1.xkey.state = 0;
  ev1.xkey.keycode = XKeysymToKeycode (dpy, 's');

  XSendEvent (dpy, rp_current_window->w, False, KeyPressMask, &ev1);

/*   XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, 't'), True, 0); */

  XSync (dpy, False);
}

void
prev_window (void *data)
{
  rp_window *new_win = (rp_window *)data;

  if (new_win == NULL) new_win = rp_current_window;
  if (new_win != NULL)
    {
      new_win = new_win->prev;
      if (new_win == NULL) 
	new_win = rp_window_tail;
      if (new_win->state == STATE_UNMAPPED) 
	prev_window (new_win);
      else
	{
	  if (rp_current_window == new_win)
	    message (MESSAGE_NO_OTHER_WINDOW, 0, 0);
	  else
	    set_active_window (new_win);
	}
    }
  else
    {
      message (MESSAGE_NO_MANAGED_WINDOWS, 0, 0);
    }
}


void
next_window (void *data)
{
  rp_window *new_win = (rp_window *)data;

  if (new_win == NULL) new_win = rp_current_window;
  if (new_win != NULL)
    {
      new_win = new_win->next;
      if (new_win == NULL) 
	new_win = rp_window_head;
      if (new_win->state == STATE_UNMAPPED) 
	next_window (new_win);
      else
	{
	  if (rp_current_window == new_win)
	    message (MESSAGE_NO_OTHER_WINDOW, 0, 0);
	  else
	    set_active_window (new_win);
	}
    }
  else
    {
      message (MESSAGE_NO_MANAGED_WINDOWS, 0, 0);
    }
}


void
last_window (void *data)
{
  rp_window *oldwin = rp_current_window;

  /* what happens if find_last_accessed_window() returns something
     funky? (rcy) */
  set_active_window (find_last_accessed_window ());

  if (rp_current_window == oldwin)
    {
      message (MESSAGE_NO_OTHER_WINDOW, 0, 0);
    }
}


void
goto_win_by_name (void *data)
{
  char *winname;
  
  if (rp_current_window == NULL) return;

  if (data == NULL)
    winname = get_input (MESSAGE_PROMPT_GOTO_WINDOW_NAME);
  else
    winname = strdup ((char *) data);

  goto_window_name (winname);

  free (winname);
}

void
rename_current_window (void *data)
{
  char *winname;
  
  if (rp_current_window == NULL) return;

  if (data == NULL)
    winname = get_input (MESSAGE_PROMPT_NEW_WINDOW_NAME);
  else
    winname = strdup ((char *) data);

  if (*winname)
    {
      free (rp_current_window->name);
      rp_current_window->name = malloc (sizeof (char) * strlen (winname) + 1);
      if (rp_current_window->name == NULL)
	{
	  PRINT_ERROR ("Out of memory\n");
	  exit (EXIT_FAILURE);
	}

      strcpy (rp_current_window->name, winname);

      rp_current_window->named = 1;
  
      /* Update the program bar. */
      update_window_names (rp_current_window->scr);
    }

  free (winname);
}


void
delete_window (void *data)
{
  XEvent ev;
  int status;

  if (rp_current_window == NULL) return;

  ev.xclient.type = ClientMessage;
  ev.xclient.window = rp_current_window->w;
  ev.xclient.message_type = wm_protocols;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = wm_delete;
  ev.xclient.data.l[1] = CurrentTime;

  status = XSendEvent(dpy, rp_current_window->w, False, 0, &ev);
  if (status == 0) fprintf(stderr, "ratpoison: delete window failed\n");
}

void 
kill_window (void *data)
{
  if (rp_current_window == NULL) return;

  XKillClient(dpy, rp_current_window->w);
}

void
show_version (void *data)
{
  message (" " PACKAGE " " VERSION " ", 0, 0);
}

void
command (void *data)
{
  char *input;
  char *cmd, *rest;
  void *arg;

  user_command *uc;
  struct sbuf *buf = NULL;
  
  if (data == NULL)
    input = get_input (MESSAGE_PROMPT_COMMAND);
  else
    input = strdup ((char *) data);
  
  if (input == NULL)
    return;

  cmd = strtok (input, " ");

  if (cmd == NULL)
    {
      free (input);
      return;
    }

  rest = strtok (NULL, "\0");

  PRINT_DEBUG ("cmd==%s rest==%s\n", cmd, (char*)rest);

  /* find the command */
  for (uc = user_commands; uc->name; uc++)
    {
      if (!strcmp (cmd, uc->name))
	{
	  /* create an arg out of the rest */
	  switch (uc->argtype)
	    {
	    case arg_VOID:
	      arg = NULL;
	      break;

	    case arg_STRING:
	      arg = rest;
	      break;

	    case arg_NUMBER:
	      if (rest)
		sscanf (rest, "%d", (int*)&arg);
	      else
		arg = 0;
	      break;

	    default:
	      abort ();
	    }

	  uc->func (arg);

          free (input);
	  return;
	}
    }

  /* couldnt find the command name */

  buf = sbuf_new (strlen(MESSAGE_UNKNOWN_COMMAND + strlen (cmd) + 4));
  sbuf_copy (buf, MESSAGE_UNKNOWN_COMMAND);
  sbuf_concat (buf, "'");
  sbuf_concat (buf, cmd);
  sbuf_concat (buf, "' ");

  message (sbuf_get (buf), 0, 0);
  
  sbuf_free (buf);

  free (input);
}

void
shell_command (void *data)
{
  char *cmd;

  if (data == NULL)
    cmd = get_input (MESSAGE_PROMPT_SHELL_COMMAND);
  else
    cmd = strdup ((char *) data);

  spawn (cmd);

  free (cmd);
}

void
xterm_command (void *data)
{
  char *cmd, *realcmd;

  if (data == NULL)
    cmd = get_input (MESSAGE_PROMPT_XTERM_COMMAND);
  else
    cmd = strdup ((char *) data);

  realcmd = (char *) malloc (strlen (cmd) + strlen (TERM_PROG) + 5);

  sprintf(realcmd, "%s -e %s", TERM_PROG, cmd);
  spawn (realcmd);

  free(cmd);
  free(realcmd);
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
void
switch_to(void *data)
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
}

/* Quit ratpoison. Thanks to 
"Chr. v. Stuckrad" <stucki@math.fu-berlin.de> for the patch. */
/* static void */
/* bye(void *dummy) */
/* { */
/*   PRINT_DEBUG ("Exiting\n"); */
/*   clean_up (); */
/*   exit (EXIT_SUCCESS); */
/* } */

/* Show the current time on the bar. Thanks to 
   Martin Samuelsson <cosis@lysator.liu.se> for the patch. */
void
show_clock (void *data)
{
  screen_info *s;
  char *msg;
  time_t timep;
 
  msg = malloc(9);
  if(msg == NULL)
    {
      PRINT_DEBUG ("Error allocation memory in show_clock()\n");
      return;
    }
 
  timep = time(NULL);
  if(timep == ((time_t)-1))
    {
      perror("In show_clock() ");
      return;
    }
 
  strncpy(msg, ctime(&timep) + 11, 8); /* FIXME: a little bit hardcoded looking */
  msg[8] = '\0';
 
  if (rp_current_window) 
    {
      s = rp_current_window->scr;
      message (msg, 0, 0);
    }
  free(msg);
}

void
goto_window_number (void *data)
{
  int n = (int)data;
  rp_window *win;

  win = find_window_by_number (n);

  if (win == NULL)
    {
      /* Display window list to indicate failure. */
      /* FIXME: We can always assume there is 1 screen, but which one
	 is the active one? Need to test on multi-screen x-servers. */
      show_bar (&screens[0]);
      return;
    }

  if (win == rp_current_window)
    {
      /* TODO: Add message "This IS window # (xxx) */
    }

  set_active_window (win);
}

/* Toggle the display of the program bar */
void
toggle_bar (void *data)
{
  screen_info *s;

  if (rp_current_window)
    s = rp_current_window->scr;
  else
    s = &screens[0];

  if (!hide_bar (s)) show_bar (s);
}


void
abort_keypress (void *data)
{
}  

/* Send the current window the prefix key event */
void
generate_prefix (void *data)
{
  XEvent ev;
  ev = *rp_current_event;

  ev.xkey.window = rp_current_window->w;
  ev.xkey.state = MODIFIER_PREFIX;
  XSendEvent (dpy, rp_current_window->w, False, KeyPressMask, &ev);
  XSync (dpy, False);
}

/* Set a transient window's x,y,width,height fields to maximize the
   window. */
static void
maximize_transient (rp_window *win)
{
  int maxx, maxy;

  /* Honour the window's maximum size */
  if (win->hints->flags & PMaxSize)
    {
      maxx = win->hints->max_width;
      maxy = win->hints->max_height;
    }
  else
    {
      maxx = win->width;
      maxy = win->height;

      /* Make sure we maximize to the nearest Resize Increment specified
	 by the window */
      if (win->hints->flags & PResizeInc)
	{
	  int amount;

	  amount = maxx - win->width;
	  amount -= amount % win->hints->width_inc;
	  PRINT_DEBUG ("amount x: %d\n", amount);
	  maxx = amount + win->width;

	  amount = maxy - win->height;
	  amount -= amount % win->hints->height_inc;
	  PRINT_DEBUG ("amount y: %d\n", amount);
	  maxy = amount + win->height;
	}
    }

  PRINT_DEBUG ("maxsize: %d %d\n", maxx, maxy);

  win->x = PADDING_LEFT - win->width / 2 + (win->scr->root_attr.width - PADDING_LEFT - PADDING_RIGHT) / 2;;
  win->y = PADDING_TOP - win->height / 2 + (win->scr->root_attr.height - PADDING_TOP - PADDING_BOTTOM) / 2;;
  win->width = maxx;
  win->height = maxy;
}

/* set a good standard window's x,y,width,height fields to maximize the window. */
static void
maximize_normal (rp_window *win)
{
  int maxx, maxy;

  int off_x = 0;
  int off_y = 0;

  /* Honour the window's maximum size */
  if (win->hints->flags & PMaxSize)
    {
      maxx = win->hints->max_width;
      maxy = win->hints->max_height;
      
      /* centre the non-maximized window */
/*       off_x = ((win->scr->root_attr.width - PADDING_LEFT - PADDING_RIGHT) - win->hints->max_width) / 2; */
/*       off_y = ((win->scr->root_attr.height - PADDING_TOP - PADDING_BOTTOM) - win->hints->max_height) / 2; */
    }
  else
    {
      maxx = win->scr->root_attr.width - PADDING_LEFT - PADDING_RIGHT;
      maxy = win->scr->root_attr.height - PADDING_TOP - PADDING_BOTTOM;

      /* Make sure we maximize to the nearest Resize Increment specified
	 by the window */
      if (win->hints->flags & PResizeInc)
	{
	  int amount;

	  amount = maxx - win->width;
	  amount -= amount % win->hints->width_inc;
	  PRINT_DEBUG ("amount x: %d\n", amount);
	  maxx = amount + win->width;

	  amount = maxy - win->height;
	  amount -= amount % win->hints->height_inc;
	  PRINT_DEBUG ("amount y: %d\n", amount);
	  maxy = amount + win->height;
	}
    }

  PRINT_DEBUG ("maxsize: %d %d\n", maxx, maxy);

  win->x = PADDING_LEFT + off_x;
  win->y = PADDING_TOP + off_y;
  win->width = maxx;
  win->height = maxy;
}

/* Maximize the current window if data = 0, otherwise assume it is a
   pointer to a window that should be maximized */
void
maximize (void *data)
{
  rp_window *win = (rp_window *)data;

  if (!win) win = rp_current_window;
  if (!win) return;

  /* Handle maximizing transient windows differently */
  if (win->transient) 
    {
      maximize_transient (win);
    }
  else
    {
      maximize_normal (win);
    }

  /* Actually do the maximizing */
  XMoveResizeWindow (dpy, win->w, win->x, win->y, win->width, win->height);

  /* I don't think this should be here, but it doesn't seem to hurt. */
  send_configure (win);

  XSync (dpy, False);
}
