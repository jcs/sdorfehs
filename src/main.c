/* Ratpoison.
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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>

#include "ratpoison.h"

/* Several systems seem not to have WAIT_ANY defined, so define it if
   it isn't. */
#ifndef WAIT_ANY
# define WAIT_ANY -1
#endif

static void init_screen (screen_info *s, int screen_num);

int alarm_signalled = 0;
int kill_signalled = 0;
int hup_signalled = 0;
int chld_signalled = 0;
int rat_x;
int rat_y;
int rat_visible = 1;		/* rat is visible by default */

Atom wm_name;
Atom wm_state;
Atom wm_change_state;
Atom wm_protocols;
Atom wm_delete;
Atom wm_take_focus;
Atom wm_colormaps;

Atom rp_command;
Atom rp_command_request;
Atom rp_command_result;

int rp_current_screen;
screen_info *screens;
int num_screens;
Display *dpy;

struct rp_child_info child_info;
struct rp_defaults defaults;

int ignore_badwindow = 0;

char **myargv;

struct rp_key prefix_key;

struct modifier_info rp_modifier_info;

/* rudeness levels */
int rp_honour_transient_raise = 1;
int rp_honour_normal_raise = 1;
int rp_honour_transient_map = 1;
int rp_honour_normal_map = 1;

char *rp_error_msg = NULL;

/* Command line options */
static struct option ratpoison_longopts[] = 
  { {"help", 	no_argument, 		0, 	'h'},
    {"version", no_argument, 		0, 	'v'},
    {"command", required_argument, 	0, 	'c'},
    {"display", required_argument, 	0, 	'd'},
    {"screen",  required_argument,      0,      's'},
    {0, 	0, 			0, 	0} };

static char ratpoison_opts[] = "hvc:d:s:";

void
fatal (const char *msg)
{
  fprintf (stderr, "ratpoison: %s", msg);
  abort ();
}

void *
xmalloc (size_t size)
{
  register void *value = malloc (size);
  if (value == 0)
    fatal ("Virtual memory exhausted");
  return value;
}

void *
xrealloc (void *ptr, size_t size)
{
  register void *value = realloc (ptr, size);
  if (value == 0)
    fatal ("Virtual memory exhausted");
  PRINT_DEBUG (("realloc: %d\n", size));
  return value;
}

char *
xstrdup (char *s)
{
  char *value;
  value = strdup (s);
  if (value == 0)
    fatal ("Virtual memory exhausted");
  return value;
}

/* Return a new string based on fmt. */
char *
xvsprintf (char *fmt, va_list ap)
{
  int size, nchars;
  char *buffer;
  va_list ap_copy;

  /* A resonable starting value. */
  size = strlen (fmt) + 1;
  buffer = (char *)xmalloc (size);

  while (1)
    {
#if defined(va_copy)
      va_copy (ap_copy, ap);
#elif defined(__va_copy)
      __va_copy (ap_copy, ap);
#else
      /* If there is no copy macro then this MAY work. On some systems
	 this could fail because va_list is a pointer so assigning one
	 to the other as below wouldn't make a copy of the data, but
	 just the pointer to the data. */
      ap_copy = ap;
#endif
      nchars = vsnprintf (buffer, size, fmt, ap_copy);
#if defined(va_copy) || defined(__va_copy)
      va_end (ap_copy);
#endif

      if (nchars > -1 && nchars < size)
	return buffer;
      else if (nchars > -1)
	size = nchars + 1;
      else
	size *= 2;

      /* Resize the buffer and try again. */
      buffer = (char *)xrealloc (buffer, size);
    }

  /* Never gets here. */
  return NULL;
}

/* Return a new string based on fmt. */
char *
xsprintf (char *fmt, ...)
{
  char *buffer;
  va_list ap;

  va_start (ap, fmt);
  buffer = xvsprintf (fmt, ap);
  va_end (ap);

  return buffer;
}

void
sighandler (int signum)
{
  kill_signalled++;
}

void
hup_handler (int signum)
{
  hup_signalled++;
}

void
alrm_handler (int signum)
{
  alarm_signalled++;
}

void
chld_handler (int signum)
{
  int pid, status, serrno;
  serrno = errno;

  while (1)
    {
      pid = waitpid (WAIT_ANY, &status, WNOHANG);
      if (pid <= 0)
	break;

      PRINT_DEBUG(("Child status: %d\n", WEXITSTATUS (status)));

      /* Tell ratpoison about the CHLD signal. We are only interested
	 in reporting commands that failed to execute. These processes
	 have a return value of 127 (according to the sh manual). */
      if (WEXITSTATUS (status) == 127)
	{
	  chld_signalled = 1;
	  child_info.pid = pid;
	  child_info.status = status;
	}
    }
  errno = serrno;  
}

int
handler (Display *d, XErrorEvent *e)
{
  char error_msg[100];

  if (e->request_code == X_ChangeWindowAttributes && e->error_code == BadAccess) {
    fprintf(stderr, "ratpoison: There can be only ONE.\n");
    exit(EXIT_FAILURE);
  }  

#ifdef IGNORE_BADWINDOW
  return 0;
#else
  if (ignore_badwindow && e->error_code == BadWindow) return 0;
#endif

  XGetErrorText (d, e->error_code, error_msg, sizeof (error_msg));
  fprintf (stderr, "ratpoison: ERROR: %s!\n", error_msg);

  /* If there is already an error to report, replace it with this new
     one. */
  if (rp_error_msg) 
    free (rp_error_msg);
  rp_error_msg = xstrdup (error_msg);

  return 0;
}

void
set_sig_handler (int sig, void (*action)(int))
{
  /* use sigaction because SVR4 systems do not replace the signal
    handler by default which is a tip of the hat to some god-aweful
    ancient code.  So use the POSIX sigaction call instead. */
  struct sigaction act;
       
  /* check setting for sig */
  if (sigaction (sig, NULL, &act)) 
    {
      PRINT_ERROR (("Error %d fetching SIGALRM handler\n", errno ));
    } 
  else 
    {
      /* if the existing action is to ignore then leave it intact
	 otherwise add our handler */
      if (act.sa_handler != SIG_IGN) 
	{
	  act.sa_handler = action;
	  sigemptyset(&act.sa_mask);
	  act.sa_flags = 0;
	  if (sigaction (sig, &act, NULL)) 
	    {
	      PRINT_ERROR (("Error %d setting SIGALRM handler\n", errno ));
	    }
	}
    }
}

void
print_version ()
{
  printf ("%s %s\n", PACKAGE, VERSION);
  printf ("Copyright (C) 2000, 2001 Shawn Betts\n\n");

  exit (EXIT_SUCCESS);
}  

void
print_help ()
{
  printf ("Help for %s %s\n\n", PACKAGE, VERSION);
  printf ("-h, --help            Display this help screen\n");
  printf ("-v, --version         Display the version\n");
  printf ("-d, --display <dpy>   Set the X display to use\n");
  printf ("-s, --screen <num>    Only use the specified screen\n");
  printf ("-c, --command <cmd>   Send ratpoison a colon-command\n\n");

  printf ("Report bugs to ratpoison-devel@lists.sourceforge.net\n\n");

  exit (EXIT_SUCCESS);
}

void
read_rc_file (FILE *file)
{
  size_t n = 256;
  char *partial;
  char *line;
  int linesize = n;

  partial = (char*)xmalloc(n);
  line = (char*)xmalloc(linesize);

  *line = '\0';
  while (fgets (partial, n, file) != NULL)
    {
      if ((strlen (line) + strlen (partial)) >= linesize)
	{
	  linesize *= 2;
	  line = (char*) xrealloc (line, linesize);
	}

      strcat (line, partial);

      if (feof(file) || (*(line + strlen(line) - 1) == '\n'))
	{
	  /* FIXME: this is a hack, command() should properly parse
	     the command and args (ie strip whitespace, etc) 

	     We should not care if there is a newline (or vertical
	     tabs or linefeeds for that matter) at the end of the
	     command (or anywhere between tokens). */
	  if (*(line + strlen(line) - 1) == '\n')
	    *(line + strlen(line) - 1) = '\0';

	  PRINT_DEBUG (("rcfile line: %s\n", line));

	  /* do it */
	  if (*line != '#')
	    {
	      char *result;
	      result = command (0, line);

	      /* Gobble the result. */
	      if (result)
		free (result);
	    }

	  *line = '\0';
	}
    }


  free (line);
  free (partial);
}

static void
read_startup_files ()
{
  char *homedir;
  FILE *fileptr;

  /* first check $HOME/.ratpoisonrc and if that does not exist then try
     /etc/ratpoisonrc */

  homedir = getenv ("HOME");
  if (!homedir)
    {
      PRINT_ERROR (("ratpoison: $HOME not set!?\n"));
    }
  else
    {
      char *filename = (char*)xmalloc (strlen (homedir) + strlen ("/.ratpoisonrc") + 1);
      sprintf (filename, "%s/.ratpoisonrc", homedir);
      
      if ((fileptr = fopen (filename, "r")) == NULL)
	{
	  /* we probably don't need to report this, its not an error */
	  PRINT_DEBUG (("ratpoison: could not open %s\n", filename));

	  if ((fileptr = fopen ("/etc/ratpoisonrc", "r")) == NULL)
	    {
	      /* neither is this */
	      PRINT_DEBUG (("ratpoison: could not open /etc/ratpoisonrc\n"));
	    }
	}

      if (fileptr)
	{
	  read_rc_file (fileptr);
	  fclose (fileptr);
	}

      free (filename);
    }
}

/* Odd that we spend so much code on making sure the silly welcome
   message is correct. Oh well... */
static void
show_welcome_message ()
{
  rp_action *help_action;
  char *prefix, *help;
  
  prefix = keysym_to_string (prefix_key.sym, prefix_key.state);

  /* Find the help key binding. */
  help_action = find_keybinding_by_action ("help");
  if (help_action)
    help = keysym_to_string (help_action->key, help_action->state);
  else
    help = NULL;


  if (help)
    {
      /* A little kludge to use ? instead of `question' for the help
	 key. */
      if (!strcmp (help, "question"))
	marked_message_printf (0, 0, MESSAGE_WELCOME, prefix, "?");
      else
	marked_message_printf (0, 0, MESSAGE_WELCOME, prefix, help);

      free (help);
    }
  else
    {
      marked_message_printf (0, 0, MESSAGE_WELCOME, prefix, ":help");
    }
  
  free (prefix);
}

static void
init_defaults ()
{
  defaults.win_gravity     = NorthWestGravity;
  defaults.trans_gravity   = CenterGravity;
  defaults.maxsize_gravity = CenterGravity;

  defaults.input_window_size   = 200;
  defaults.window_border_width = 1;
  defaults.bar_x_padding       = 0;
  defaults.bar_y_padding       = 0;
  defaults.bar_location        = NorthEastGravity;
  defaults.bar_timeout 	       = 5;
  defaults.bar_border_width    = 1;

  defaults.frame_indicator_timeout = 1;
  defaults.frame_resize_unit = 10;

  defaults.padding_left   = 0;
  defaults.padding_right  = 0;
  defaults.padding_top 	  = 0;
  defaults.padding_bottom = 0;

  defaults.font = XLoadQueryFont (dpy, "9x15bold");
  if (defaults.font == NULL)
    {
      fprintf (stderr, "ratpoison: Cannot load font %s.\n", "9x15bold");
      exit (EXIT_FAILURE);
    }

  defaults.wait_for_key_cursor = 1;

  defaults.window_fmt = xstrdup ("%n%s%t");

  defaults.win_name = WIN_NAME_TITLE;
  defaults.startup_message = 1;
  defaults.warp = 1;
  defaults.window_list_style = STYLE_ROW;
}

int
main (int argc, char *argv[])
{
  int i;
  int c;
  char **command = NULL;
  int cmd_count = 0;
  int screen_arg = 0;
  int screen_num = 0;
  char *display = NULL;

  myargv = argv;

  /* Parse the arguments */
  while (1)
    {
      int option_index = 0;

      c = getopt_long (argc, argv, ratpoison_opts, ratpoison_longopts, &option_index);
      if (c == -1) break;

      switch (c)
	{
	case 'h':
	  print_help ();
	  break;
	case 'v':
	  print_version ();
	  break;
	case 'c':
	  if (!command)
	    {
	      command = xmalloc (sizeof(char *));
	      cmd_count = 0;
	    }
	  else
	    {
	      command = xrealloc (command, sizeof (char *) * (cmd_count + 1));
	    }

	  command[cmd_count] = xstrdup (optarg);
	  cmd_count++;
	  break;
	case 'd':
	  display = xstrdup (optarg);
	  break;
	case 's':
	  screen_arg = 1;
	  screen_num = strtol (optarg, NULL, 10);
	  break;
	  
	default:
	  exit (EXIT_FAILURE);
	}
    }

  if (!(dpy = XOpenDisplay (display)))
    {
      fprintf (stderr, "Can't open display\n");
      return EXIT_FAILURE;
    }

  /* Set ratpoison specific Atoms. */
  rp_command = XInternAtom (dpy, "RP_COMMAND", False);
  rp_command_request = XInternAtom (dpy, "RP_COMMAND_REQUEST", False);
  rp_command_result = XInternAtom (dpy, "RP_COMMAND_RESULT", False);

  if (cmd_count > 0)
    {
      int i;

      for (i=0; i<cmd_count; i++)
	{
	  send_command (command[i]);
	  free (command[i]);
	}

      free (command);
      XCloseDisplay (dpy);
      return EXIT_SUCCESS;
    }

  /* Set our Atoms */
  wm_name =  XInternAtom(dpy, "WM_NAME", False);
  wm_state = XInternAtom(dpy, "WM_STATE", False);
  wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);
  wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
  wm_colormaps = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);

  /* Setup signal handlers. */
  XSetErrorHandler(handler);  
  set_sig_handler (SIGALRM, alrm_handler);
  set_sig_handler (SIGTERM, sighandler);
  set_sig_handler (SIGINT, sighandler);
  set_sig_handler (SIGHUP, hup_handler);
  set_sig_handler (SIGCHLD, chld_handler);

  /* Setup ratpoison's internal structures */
  init_defaults ();
  init_window_stuff ();

  /* Get the number of screens */
  num_screens = ScreenCount (dpy);

  /* make sure the screen specified is valid. */
  if (screen_arg)
    {
      if (screen_num < 0 || screen_num >= num_screens)
	{
	  fprintf (stderr, "%d is an invalid screen for the display\n", screen_num);
	  exit (EXIT_FAILURE);
	}

      /* we're only going to use one screen. */
      num_screens = 1;
    }

  /* Initialize the screens */
  screens = (screen_info *)xmalloc (sizeof (screen_info) * num_screens);
  PRINT_DEBUG (("%d screens.\n", num_screens));

  if (screen_arg)
    {
      init_screen (&screens[0], screen_num);
    }
  else
    {
      for (i=0; i<num_screens; i++)
	{
	  init_screen (&screens[i], i);
	}
    }

  init_frame_lists ();
  update_modifier_map ();
  initialize_default_keybindings ();

  /* Scan for windows */
  if (screen_arg)
    {
      rp_current_screen = screen_num;
      scanwins (&screens[0]);
    }
  else
    {
      rp_current_screen = 0;
      for (i=0; i<num_screens; i++)
	{
	  scanwins (&screens[i]);
	}
    }

  read_startup_files ();

  /* Indicate to the user that ratpoison has booted. */
  if (defaults.startup_message)
    show_welcome_message();

  /* If no window has focus, give the key_window focus. */
  if (current_window() == NULL)
    XSetInputFocus (dpy, current_screen()->key_window, 
		    RevertToPointerRoot, CurrentTime);

  listen_for_events ();

  return EXIT_SUCCESS;
}

static void
init_rat_cursor (screen_info *s)
{
  s->rat = XCreateFontCursor( dpy, XC_icon );
}

static void
init_screen (screen_info *s, int screen_num)
{
  XGCValues gv;

  /* Select on some events on the root window, if this fails, then
     there is already a WM running and the X Error handler will catch
     it, terminating ratpoison. */
  XSelectInput(dpy, RootWindow (dpy, screen_num),
	       PropertyChangeMask | ColormapChangeMask
	       | SubstructureRedirectMask | SubstructureNotifyMask );
  XSync (dpy, False);

  /* Create the numset for the frames. */
  s->frames_numset = numset_new ();

  /* Build the display string for each screen */
  s->display_string = xmalloc (strlen(DisplayString (dpy)) + 21);
  sprintf (s->display_string, "DISPLAY=%s", DisplayString (dpy));
  if (strrchr (DisplayString (dpy), ':'))
    {
      char *dot;

      dot = strrchr(s->display_string, '.');
      if (dot)
	sprintf(dot, ".%i", screen_num);
    }

  PRINT_DEBUG (("%s\n", s->display_string));

  s->screen_num = screen_num;
  s->root = RootWindow (dpy, screen_num);
  s->def_cmap = DefaultColormap (dpy, screen_num);
  XGetWindowAttributes (dpy, s->root, &s->root_attr);
  
  init_rat_cursor (s);

  s->fg_color = BlackPixel (dpy, s->screen_num);
  s->bg_color = WhitePixel (dpy, s->screen_num);

  /* Setup the GC for drawing the font. */
  gv.foreground = s->fg_color;
  gv.background = s->bg_color;
  gv.function = GXcopy;
  gv.line_width = 1;
  gv.subwindow_mode = IncludeInferiors;
  gv.font = defaults.font->fid;
  s->normal_gc = XCreateGC(dpy, s->root, 
			   GCForeground | GCBackground | GCFunction 
			   | GCLineWidth | GCSubwindowMode | GCFont, 
			   &gv);

  /* Create the program bar window. */
  s->bar_is_raised = 0;
  s->bar_window = XCreateSimpleWindow (dpy, s->root, 0, 0, 1, 1, 
				       defaults.bar_border_width, 
				       s->fg_color, s->bg_color);

  /* Setup the window that will recieve all keystrokes once the prefix
     key has been pressed. */
  s->key_window = XCreateSimpleWindow (dpy, s->root, 0, 0, 1, 1, 0, 
				       WhitePixel (dpy, s->screen_num), 
				       BlackPixel (dpy, s->screen_num));
  XSelectInput (dpy, s->key_window, KeyPressMask | KeyReleaseMask);
  XMapWindow (dpy, s->key_window);

  /* Create the input window. */
  s->input_window = XCreateSimpleWindow (dpy, s->root, 0, 0, 1, 1, 
					 defaults.bar_border_width, 
					 s->fg_color, s->bg_color);
  XSelectInput (dpy, s->input_window, KeyPressMask | KeyReleaseMask);

  /* Create the frame indicator window */
  s->frame_window = XCreateSimpleWindow (dpy, s->root, 1, 1, 1, 1, defaults.bar_border_width, 
					 s->fg_color, s->bg_color);

  /* Create the help window */
  s->help_window = XCreateSimpleWindow (dpy, s->root, 0, 0, s->root_attr.width,
					s->root_attr.height, 0, s->fg_color, s->bg_color);
  XSelectInput (dpy, s->help_window, KeyPressMask);

  XSync (dpy, 0);
}

static void
free_screen (screen_info *s)
{
  rp_window_frame *frame;
  struct list_head *iter, *tmp;

  list_for_each_safe_entry (frame, iter, tmp, &s->rp_window_frames, node)
    {
      frame_free (s, frame);
    }
 
  XDestroyWindow (dpy, s->bar_window);
  XDestroyWindow (dpy, s->key_window);
  XDestroyWindow (dpy, s->input_window);
  XDestroyWindow (dpy, s->frame_window);
  XDestroyWindow (dpy, s->help_window);

  XFreeCursor (dpy, s->rat);
  XFreeColormap (dpy, s->def_cmap);
  XFreeGC (dpy, s->normal_gc);

  free (s->display_string);

  numset_free (s->frames_numset);
}

void
clean_up ()
{
  int i;

  free_keybindings ();
  free_aliases ();
  free_bar ();
  free_history ();

  free_window_stuff ();
  
  for (i=0; i<num_screens; i++)
    {
      free_screen (&screens[i]);
    }
  free (screens);

  XFreeFont (dpy, defaults.font);
  free (defaults.window_fmt);

  XSetInputFocus (dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
  XCloseDisplay (dpy);
}

/* Given a root window, return the screen_info struct */
screen_info *
find_screen (Window w)
{
  int i;

  for (i=0; i<num_screens; i++)
    if (screens[i].root == w) return &screens[i];

   return NULL;
 }
