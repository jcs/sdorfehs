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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "ratpoison.h"

static void init_screen (screen_info *s, int screen_num);

int rat_x;
int rat_y;
int rat_visible = 1;		/* rat is visible by default */

Atom wm_state;
Atom wm_change_state;
Atom wm_protocols;
Atom wm_delete;
Atom wm_take_focus;
Atom wm_colormaps;

Atom rp_restart;
Atom rp_kill;

screen_info *screens;
int num_screens;
Display *dpy;

int ignore_badwindow = 0;

static XFontStruct *font;

char **myargv;

XGCValues gv;

struct rp_key prefix_key;

/* Command line options */
static struct option ratpoison_longopts[] = { {"help", no_argument, 0, 'h'},
					      {"version", no_argument, 0, 'v'},
					      {"restart", no_argument, 0, 'r'},
					      {"kill", no_argument, 0, 'k'},
					      {0, 0, 0, 0} };
static char ratpoison_opts[] = "hvrk";

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
  PRINT_DEBUG("realloc: %d\n", size);
  return value;
}

void
sighandler (int signum)
{
  fprintf (stderr, "ratpoison: Agg! I've been SHOT!\n"); 
  clean_up ();
  exit (EXIT_FAILURE);
}

void
hup_handler (int signum)
{
  /* Doesn't function correctly. The event IS placed on the queue but
     XSync() doesn't seem to sync it and until other events come in
     the restart event won't be processed. */
  PRINT_DEBUG ("Restarting with a fresh plate.\n"); 

  send_restart ();
  XSync(dpy, False); 
}

void
alrm_handler (int signum)
{
  int i;

  PRINT_DEBUG ("alarm recieved.\n");

  /* FIXME: should only hide 1 bar, but we hide them all. */
  for (i=0; i<num_screens; i++)
    {
      hide_bar (&screens[i]);
    }
  XSync (dpy, False);
}

int
handler (Display *d, XErrorEvent *e)
{
  char error_msg[100];

  if (e->request_code == X_ChangeWindowAttributes && e->error_code == BadAccess) {
    fprintf(stderr, "ratpoison: There can be only ONE.\n");
    exit(EXIT_FAILURE);
  }  

  if (ignore_badwindow && e->error_code == BadWindow) return 0;

  strcpy (error_msg, "ERROR: ");

  XGetErrorText (d, e->error_code, error_msg + 7, sizeof (error_msg) - 7);
  fprintf (stderr, "ratpoison: %s!\n", error_msg);

  marked_message (error_msg, 0, strlen (error_msg));

/*   exit (EXIT_FAILURE);  */
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
      PRINT_ERROR ("Error %d fetching SIGALRM handler\n", errno );
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
	      PRINT_ERROR ("Error %d setting SIGALRM handler\n", errno );
	    }
	}
    }
}

void
print_version ()
{
  printf ("%s %s\n", PACKAGE, VERSION);
  printf ("Copyright (C) 2000 Shawn Betts\n\n");

  exit (EXIT_SUCCESS);
}  

void
print_help ()
{
  printf ("Help for %s %s\n\n", PACKAGE, VERSION);
  printf ("-h, --help            Display this help screen\n");
  printf ("-v, --version         Display the version\n");
  printf ("-r, --restart         Restart ratpoison\n");
  printf ("-k, --kill            Kill ratpoison\n\n");

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
      if ((strlen (line) + strlen (partial)) <= linesize)
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

	  PRINT_DEBUG ("rcfile line: %s\n", line);

	  /* do it */
	  command (line);

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
    fprintf (stderr, "ratpoison: $HOME not set!?\n");
  else
    {
      char *filename = (char*)xmalloc (strlen (homedir) + strlen ("/.ratpoisonrc") + 1);
      sprintf (filename, "%s/.ratpoisonrc", homedir);
      
      if ((fileptr = fopen (filename, "r")) == NULL)
	{
	  /* we probably don't need to report this, its not an error */
	  fprintf (stderr, "ratpoison: could not open %s\n", filename);

	  if ((fileptr = fopen ("/etc/ratpoisonrc", "r")) == NULL)
	    {
	      /* neither is this */
	      fprintf (stderr, "ratpoison: could not open /etc/ratpoisonrc\n");
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

int
main (int argc, char *argv[])
{
  int i;
  int c;
  int do_kill = 0;
  int do_restart = 0;

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
	case 'k':
	  do_kill = 1;
	  break;
	case 'r':
	  do_restart = 1;
	  break;
	default:
	  exit (EXIT_FAILURE);
	}
    }

  if (!(dpy = XOpenDisplay (NULL)))
    {
      fprintf (stderr, "Can't open display\n");
      return EXIT_FAILURE;
    }

  /* Setup signal handlers. */
  XSetErrorHandler(handler);  
  set_sig_handler (SIGALRM, alrm_handler);
  set_sig_handler (SIGTERM, sighandler);
  set_sig_handler (SIGINT, sighandler);
  set_sig_handler (SIGHUP, hup_handler);

  /* Set ratpoison specific Atoms. */
  rp_restart = XInternAtom (dpy, "RP_RESTART", False);
  rp_kill = XInternAtom (dpy, "RP_KILL", False);

  if (do_kill)
    {
      send_kill ();
      XSync (dpy, False);
      clean_up ();
      return EXIT_SUCCESS;
    }
  if (do_restart)
    {
      send_restart ();
      XSync (dpy, False);
      clean_up ();
      return EXIT_SUCCESS;
    }

  /* Set our Atoms */
  wm_state = XInternAtom(dpy, "WM_STATE", False);
  wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);
  wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
  wm_colormaps = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);

  init_numbers ();
  init_window_list ();
  initialize_default_keybindings ();

  font = XLoadQueryFont (dpy, FONT);
  if (font == NULL)
    {
      fprintf (stderr, "ratpoison: Cannot load font %s.\n", FONT);
      exit (EXIT_FAILURE);
    }

  num_screens = ScreenCount (dpy);
  screens = (screen_info *)xmalloc (sizeof (screen_info) * num_screens);

  PRINT_DEBUG ("%d screens.\n", num_screens);

  /* Initialize the screens */
  for (i=0; i<num_screens; i++)
    {
      init_screen (&screens[i], i);
    }

  read_startup_files ();

  /* Set an initial window as active. */
  set_active_window (find_window_other ());
  
  handle_events ();

  return EXIT_SUCCESS;
}

static void
init_screen (screen_info *s, int screen_num)
{
  XColor fg_color, bg_color,/*  bold_color, */ junk;

  s->screen_num = screen_num;
  s->root = RootWindow (dpy, screen_num);
  s->def_cmap = DefaultColormap (dpy, screen_num);
  s->font = font;
  XGetWindowAttributes (dpy, s->root, &s->root_attr);

  /* Get our program bar colors */
  if (!XAllocNamedColor (dpy, s->def_cmap, FOREGROUND, &fg_color, &junk))
    {
      fprintf (stderr, "ratpoison: Unknown color '%s'\n", FOREGROUND);
    }

  if (!XAllocNamedColor (dpy, s->def_cmap, BACKGROUND, &bg_color, &junk))
    {
      fprintf (stderr, "ratpoison: Unknown color '%s'\n", BACKGROUND);
    }

/*   if (!XAllocNamedColor (dpy, s->def_cmap, BAR_BOLD_COLOR, &bold_color, &junk)) */
/*     { */
/*       fprintf (stderr, "ratpoison: Unknown color '%s'\n", BAR_BOLD_COLOR); */
/*     } */

  /* Setup the GC for drawing the font. */
  gv.foreground = fg_color.pixel;
  gv.background = bg_color.pixel;
  gv.function = GXcopy;
  gv.line_width = 1;
  gv.subwindow_mode = IncludeInferiors;
  gv.font = font->fid;
  s->normal_gc = XCreateGC(dpy, s->root, 
			   GCForeground | GCBackground | GCFunction 
			   | GCLineWidth | GCSubwindowMode | GCFont, 
			   &gv);
/*   gv.foreground = bold_color.pixel; */
/*   s->bold_gc = XCreateGC(dpy, s->root,  */
/* 			 GCForeground | GCBackground | GCFunction  */
/* 			 | GCLineWidth | GCSubwindowMode | GCFont,  */
/* 			 &gv); */

  XSelectInput(dpy, s->root,
               PropertyChangeMask | ColormapChangeMask
               | SubstructureRedirectMask | KeyPressMask 
               | SubstructureNotifyMask );

  /* Create the program bar window. */
  s->bar_is_raised = 0;
  s->bar_window = XCreateSimpleWindow (dpy, s->root, 0, 0,
				       1, 1, 1, fg_color.pixel, bg_color.pixel);
  XSelectInput (dpy, s->bar_window, StructureNotifyMask);

  /* Setup the window that will recieve all keystrokes once the prefix
     key has been pressed. */
  s->key_window = XCreateSimpleWindow (dpy, s->root, 0, 0, 1, 1, 0, WhitePixel (dpy, 0), BlackPixel (dpy, 0));
  XSelectInput (dpy, s->key_window, KeyPressMask);
  XMapWindow (dpy, s->key_window);

  /* Create the input window. */
  s->input_window = XCreateSimpleWindow (dpy, s->root, 0, 0, 
  					 1, 1, 1, fg_color.pixel, bg_color.pixel);
  XSelectInput (dpy, s->input_window, KeyPressMask);

  XSync (dpy, 0);

  scanwins (s);
}

void
clean_up ()
{
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
