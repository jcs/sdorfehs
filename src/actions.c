/* actions.cpp -- all actions that can be performed with
   keystrokes */

#include <unistd.h>
#include <sys/wait.h>

#include "ratpoison.h"

/* Initialization of the key structure */
rp_action key_actions[] = { {'c', -1, "xterm", spawn},
			    {'e', -1, "emacs", spawn},
			    {'p', -1, 0, prev_window},
			    {'n', -1, 0, next_window},
			    {KEY_PREFIX, -1, 0, last_window},
			    {'w', -1, 0, toggle_bar},
			    {'k', ShiftMask, 0, kill_window},
			    {'k', 0, 0, delete_window},
			    {'\'', -1, 0, goto_win_by_name},
			    {'a', -1, 0, rename_current_window},
			    {'0', -1, 0, goto_window_0},
			    {'1', -1, 0, goto_window_1},
			    {'2', -1, 0, goto_window_2},
			    {'3', -1, 0, goto_window_3},
			    {'4', -1, 0, goto_window_4},
			    {'5', -1, 0, goto_window_5},
			    {'6', -1, 0, goto_window_6},
			    {'7', -1, 0, goto_window_7},
			    {'8', -1, 0, goto_window_8},
			    {'9', -1, 0, goto_window_9},
			    { 0, 0, 0, 0 } };

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
  rp_current_window = find_last_accessed_window ();
  set_active_window (rp_current_window);
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
      fprintf (stderr, "exec %s ", prog);
      perror(" failed");
      exit(EXIT_FAILURE);
    }
    exit(0);
  }
  wait((int *) 0);
  PRINT_DEBUG ("spawned %s\n", prog);
}

void
goto_window_number (int n)
{
  rp_window *win;

  if ((win = find_window_by_number (n)) == NULL)
    {
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
