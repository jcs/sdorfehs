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

    {XK_0, 		0, 	"select 0", 		command},
    {XK_1, 		0, 	"select 1", 		command},
    {XK_2, 		0, 	"select 2", 		command},
    {XK_3, 		0, 	"select 3", 		command},
    {XK_4, 		0, 	"select 4", 		command},
    {XK_5, 		0, 	"select 5", 		command},
    {XK_6, 		0, 	"select 6", 		command},
    {XK_7, 		0, 	"select 7", 		command},
    {XK_8, 		0, 	"select 8", 		command},
    {XK_9, 		0, 	"select 9", 		command},

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
    {XK_exclam, 	C, 	"colon exec " TERM_PROG " -e ", 	command},

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
  { {"abort",		cmd_abort,		arg_VOID},
    {"next", 		cmd_next, 		arg_VOID},
    {"prev", 		cmd_prev, 		arg_VOID},
    {"exec", 		cmd_exec, 		arg_STRING},
    {"select", 		cmd_select,		arg_STRING},
    {"colon",	 	cmd_colon,		arg_STRING},
    {"kill", 		cmd_kill, 		arg_VOID},
    {"delete", 		cmd_delete, 		arg_VOID},
    {"other", 		cmd_other, 		arg_VOID},
    {"windows", 	cmd_windows, 		arg_VOID},
    {"title", 		cmd_rename, 		arg_STRING},
    {"clock", 		cmd_clock, 		arg_VOID},
    {"maximize", 	maximize, 		arg_VOID},
    {"newwm",		cmd_newwm,		arg_STRING},
    {"generate",	cmd_generate,		arg_STRING}, /* rename to stuff */
    {"version",		cmd_version,		arg_VOID},

    /* the following screen commands may or may not be able to be
       implemented.  See the screen documentation for what should be
       emulated with these commands */

    {"echo", 		cmd_unimplemented, 	arg_VOID},
    {"stuff", 		cmd_unimplemented, 	arg_VOID},
    {"number", 		cmd_unimplemented, 	arg_VOID},
    {"hardcopy",	cmd_unimplemented,	arg_VOID},
    {"help",		cmd_unimplemented,	arg_VOID},
    {"lastmsg",		cmd_unimplemented,	arg_VOID},
    {"license",		cmd_unimplemented,	arg_VOID},
    {"lockscreen",	cmd_unimplemented,	arg_VOID},
    {"meta",		cmd_unimplemented,	arg_VOID},
    {"msgwait",		cmd_unimplemented,	arg_VOID},
    {"msgminwait",	cmd_unimplemented,	arg_VOID},
    {"nethack",		cmd_unimplemented,	arg_VOID},
    {"quit",		cmd_unimplemented,	arg_VOID},
    {"redisplay",	cmd_unimplemented,	arg_VOID},
    {"screen",		cmd_unimplemented,	arg_VOID},
    {"setenv",		cmd_unimplemented,	arg_VOID},
    {"shell",		cmd_unimplemented,	arg_VOID},
    {"shelltitle",	cmd_unimplemented,	arg_VOID},
    {"sleep",		cmd_unimplemented,	arg_VOID},
    {"sorendition",	cmd_unimplemented,	arg_VOID},
    {"split",		cmd_unimplemented,	arg_VOID},
    {"focus",		cmd_unimplemented,	arg_VOID},
    {"startup_message",	cmd_unimplemented,	arg_VOID},
    {0,			0,		0} };


void
cmd_unimplemented (void *data)
{
  marked_message (" FIXME:  unimplemented command ",0,8);
}

void
cmd_generate (void *data)
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
  ev1.xkey.keycode = XKeysymToKeycode (dpy, KEY_PREFIX);

  XSendEvent (dpy, rp_current_window->w, False, KeyPressMask, &ev1);

/*   XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, 't'), True, 0); */

  XSync (dpy, False);
}

void
cmd_prev (void *data)
{
  rp_window *w;

  if (!rp_current_window)
    message (MESSAGE_NO_MANAGED_WINDOWS);
  else 
    {
      w = find_window_prev (rp_current_window);

      if (w == rp_current_window)
	message (MESSAGE_NO_OTHER_WINDOW);
      else
	set_active_window (w);
    }
}

void
cmd_next (void *data)
{
  rp_window *w;

  if (!rp_current_window)
    message (MESSAGE_NO_MANAGED_WINDOWS);
  else 
    {
      w = find_window_next (rp_current_window);

      if (w == rp_current_window)
	message (MESSAGE_NO_OTHER_WINDOW);
      else
	set_active_window (w);
    }
}

void 
cmd_other (void *data)
{
  rp_window *w;

  w = find_window_other ();

  if (w == rp_current_window)
    message (MESSAGE_NO_OTHER_WINDOW);
  else
    set_active_window (w);
}

static int
string_to_window_number (str)
char *str;
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
void
cmd_select (void *data)
{
  char *str;
  int n;
  rp_window *w;

  if (data == NULL)
    str = get_input (MESSAGE_PROMPT_SWITCH_TO_WINDOW);
  else
    str = strdup ((char *) data);

  /* try by number */
  if ((n = string_to_window_number (str)) >= 0)
    {
      if ((w = find_window_number (n)))
	set_active_window (w);
      else
	message ("no window by that number (FIXME: show window list)");
    }
  else
    /* try by name */
    {
      if ((w = find_window_name (str)))
	set_active_window (w);
      else
	/* we need to format a string that includes the str */
	message ("MESSAGE_NO_WINDOW_NAMED (FIXME:)");
    }

  free (str);
}

void
cmd_rename (void *data)
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
cmd_delete (void *data)
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
cmd_kill (void *data)
{
  if (rp_current_window == NULL) return;

  XKillClient(dpy, rp_current_window->w);
}

void
cmd_version (void *data)
{
  message (" " PACKAGE " " VERSION " ");
}

void 
command (void *data)
{
  char *cmd, *rest;
  char *input;

  user_command *uc;
  struct sbuf *buf = NULL;
  
  if (data == NULL)
    return;

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
	  uc->func (rest);
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
}

void
cmd_colon (void *data)
{
  char *input;

  if (data == NULL)
    input = get_input (MESSAGE_PROMPT_COMMAND);
  else
    input = get_more_input (MESSAGE_PROMPT_COMMAND, data);

  command (input);
  
  free (input);
}

void
cmd_exec (void *data)
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
cmd_newwm(void *data)
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
cmd_clock (void *data)
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
      message (msg);
    }
  free(msg);
}


/* Toggle the display of the program bar */
void
cmd_windows (void *data)
{
  screen_info *s;

  if (rp_current_window)
    s = rp_current_window->scr;
  else
    s = &screens[0];

  if (!hide_bar (s)) show_bar (s);
}


void
cmd_abort (void *data)
{
}  

/* Send the current window the prefix key event */
/* void */
/* cmd_generate_prefix (void *data) */
/* { */
/*   XEvent ev; */
/*   ev = *rp_current_event; */

/*   ev.xkey.window = rp_current_window->w; */
/*   ev.xkey.state = MODIFIER_PREFIX; */
/*   XSendEvent (dpy, rp_current_window->w, False, KeyPressMask, &ev); */
/*   XSync (dpy, False); */
/* } */

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
