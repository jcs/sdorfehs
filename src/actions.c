/* actions.cpp -- all actions that can be performed with
   keystrokes */

#include <unistd.h>
#include <sys/wait.h>
#include <X11/keysym.h>

#include "ratpoison.h"

/* Initialization of the key structure */
rp_action key_actions[] = { {KEY_PREFIX,	0,		0,		generate_prefix},
                            {XK_c,		-1,		"xterm",	spawn},
			    {XK_e,		-1,		"emacs",	spawn},
			    {XK_p,		-1,		0,		prev_window},
			    {XK_n,		-1,		0,		next_window},
			    {XK_colon,		0,		0,		execute_command},
			    {KEY_PREFIX,	-1,		0,		last_window},
			    {XK_w,		-1,		0,		toggle_bar},
			    {XK_K,		0,		0,		kill_window},
			    {XK_k,		0,		0,		delete_window},
			    {XK_quoteright,	-1,		0,		goto_win_by_name},
			    {XK_a,		-1,		0,		rename_current_window},
			    {XK_g,		ControlMask,	0,		abort_keypress},
			    {XK_0,		-1,		0,		goto_window_0},
			    {XK_1,		-1,		0,		goto_window_1},
			    {XK_2,		-1,		0,		goto_window_2},
			    {XK_3,		-1,		0,		goto_window_3},
			    {XK_4,		-1,		0,		goto_window_4},
			    {XK_5,		-1,		0,		goto_window_5},
			    {XK_6,		-1,		0,		goto_window_6},
			    {XK_7,		-1,		0,		goto_window_7},
			    {XK_8,		-1,		0,		goto_window_8},
			    {XK_9,		-1,		0,		goto_window_9},
			    { 0,		0,		0,		0 } };

void
prev_window (void *data)
{
  if (rp_current_window != NULL)
    {
      set_current_window (rp_current_window->prev);
      if (rp_current_window == NULL) 
	{
	  rp_current_window = rp_window_tail;
	}
      if (rp_current_window->state == STATE_UNMAPPED) prev_window (NULL);
      set_active_window (rp_current_window);
    }
}

void
next_window (void *data)
{
  if (rp_current_window != NULL)
    {
      rp_current_window = rp_current_window->next;
      if (rp_current_window == NULL) 
	{
	  rp_current_window = rp_window_head;
	}
      if (rp_current_window->state == STATE_UNMAPPED) next_window (NULL);
      set_active_window (rp_current_window);
    }
}


void
last_window (void *data)
{
  rp_window *oldwin = rp_current_window;

  rp_current_window = find_last_accessed_window ();
  set_active_window (rp_current_window);

  if (rp_current_window == oldwin)
    {
      display_msg_in_bar (&screens[0], "No other window.");
    }
}


void
goto_win_by_name (void *data)
{
  char winname[100];
  
  if (rp_current_window == NULL) return;

  get_input (rp_current_window->scr, "Window: ", winname, 100);
  PRINT_DEBUG ("user entered: %s\n", winname);

  goto_window_name (winname);
}

void
rename_current_window (void *data)
{
  char winname[100];
  
  if (rp_current_window == NULL) return;

  get_input (rp_current_window->scr, "Name: ", winname, 100);
  PRINT_DEBUG ("user entered: %s\n", winname);

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
execute_command (void *data)
{
  char cmd[100];

  if (rp_current_window)
    {
      get_input (rp_current_window->scr, "Command: ", cmd, 100);
    }
  else
    {
      /* FIXME: We can always assume there is 1 screen, but which one
	 is the active one? Need to test on multi-screen x-servers. */
      get_input (&screens[0], "Command: ", cmd, 100);
    }

  PRINT_DEBUG ("user entered: %s\n", cmd);

  spawn (cmd);
}

void
spawn(void *data)
{
  char *prog = data;
  /*
   * ugly dance to avoid leaving zombies.  Could use SIGCHLD,
   * but it's not very portable.
   */
  if (fork() == 0) {
    if (fork() == 0) {
      putenv(DisplayString(dpy));
      execlp(prog, prog, 0);

      PRINT_ERROR ("exec %s ", prog);
      perror(" failed");

      exit(EXIT_FAILURE);
    }
    exit(0);
  }
  wait((int *) 0);
  PRINT_DEBUG ("spawned %s\n", prog);
}

/* Switch to a different Window Manager. Thanks to 
"Chr. v. Stuckrad" <stucki@math.fu-berlin.de> for the patch. */
void
switch_to(void *which)
{
  char *prog=(char *)which;

  PRINT_DEBUG ("Switching to %s\n", prog);

  putenv(DisplayString(dpy)); 
  execlp(prog, prog, 0);

  PRINT_ERROR ("exec %s ", prog);
  perror(" failed");
}

/* Quit ratpoison. Thanks to 
"Chr. v. Stuckrad" <stucki@math.fu-berlin.de> for the patch. */
void
bye(void *dummy)
{
  PRINT_DEBUG ("Exiting\n");
  clean_up ();
  exit (EXIT_SUCCESS);
}

void
goto_window_number (int n)
{
  rp_window *win;

  if ((win = find_window_by_number (n)) == NULL)
    {
      /* Display window list to indicate failure. */
      /* FIXME: We can always assume there is 1 screen, but which one
	 is the active one? Need to test on multi-screen x-servers. */
      show_bar (&screens[0]);
      return;
    }

  rp_current_window = win;
  set_active_window (rp_current_window);
}

void
goto_window_0 (void *data)
{
  goto_window_number (0);
}

void
goto_window_1 (void *data)
{
  goto_window_number (1);
}

void
goto_window_2 (void *data)
{
  goto_window_number (2);
}

void
goto_window_3 (void *data)
{
  goto_window_number (3);
}

void
goto_window_4 (void *data)
{
  goto_window_number (4);
}

void
goto_window_5 (void *data)
{
  goto_window_number (5);
}

void
goto_window_6 (void *data)
{
  goto_window_number (6);
}

void
goto_window_7 (void *data)
{
  goto_window_number (7);
}

void
goto_window_8 (void *data)
{
  goto_window_number (8);
}

void
goto_window_9 (void *data)
{
  goto_window_number (9);
}

/* Toggle the display of the program bar */
void
toggle_bar (void *data)
{
  screen_info *s;

  if (rp_current_window) 
    {
      s = rp_current_window->scr;
      if (!hide_bar (s)) show_bar (s);
    }
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
