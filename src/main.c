/* 
 * Copyright (C) 2000 Shawn Betts
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

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

/* Command line options */
static struct option ratpoison_longopts[] = { {"help", no_argument, 0, 'h'},
					      {"version", no_argument, 0, 'v'},
					      {"restart", no_argument, 0, 'r'},
					      {"kill", no_argument, 0, 'k'},
					      {0, 0, 0, 0} };
static char ratpoison_opts[] = "hvrk";

void
sighandler ()
{
  fprintf (stderr, "ratpoison: Agg! I've been SHOT!\n"); 
  clean_up ();
  exit (EXIT_FAILURE);
}

void
send_restart ()
{
  XEvent ev;
  int status;

  ev.xclient.type = ClientMessage;
  ev.xclient.window = DefaultRootWindow (dpy);
  ev.xclient.message_type = rp_restart;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = rp_restart;
  ev.xclient.data.l[1] = CurrentTime;

  status = XSendEvent (dpy, DefaultRootWindow (dpy), False, SubstructureRedirectMask, &ev); 
  if (status == 0)
    {
      PRINT_ERROR ("failed to send restart event\n");
    }
}

void
send_kill ()
{
  XEvent ev;
  int status;

  ev.xclient.type = ClientMessage;
  ev.xclient.window = DefaultRootWindow (dpy);
  ev.xclient.message_type = rp_kill;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = rp_kill;
  ev.xclient.data.l[1] = CurrentTime;

  status = XSendEvent (dpy, DefaultRootWindow (dpy), False, SubstructureRedirectMask, &ev); 
  if (status == 0)
    {
      PRINT_ERROR ("failed to send kill event\n");
    }
}


void
hup_handler ()
{
  /* Doesn't function correctly. The event IS placed on the queue but
     XSync() doesn't seem to sync it and until other events come in
     the restart event won't be processed. */
  PRINT_DEBUG ("Restarting with a fresh plate.\n"); 

  send_restart ();
  XSync(dpy, False); 
}

void
alrm_handler ()
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

  XGetErrorText (d, e->error_code, error_msg, sizeof (error_msg));
  fprintf (stderr, "ratpoison: %s!\n", error_msg);

  exit (EXIT_FAILURE); 
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
  if (signal (SIGALRM, alrm_handler) == SIG_IGN) signal (SIGALRM, SIG_IGN);
  if (signal (SIGTERM, sighandler) == SIG_IGN) signal (SIGTERM, SIG_IGN);
  if (signal (SIGINT, sighandler) == SIG_IGN) signal (SIGINT, SIG_IGN);
  if (signal (SIGHUP, hup_handler) == SIG_IGN) signal (SIGHUP, SIG_IGN);

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

  font = XLoadQueryFont (dpy, FONT_NAME);
  if (font == NULL)
    {
      fprintf (stderr, "ratpoison: Cannot load font %s.\n", FONT_NAME);
      exit (EXIT_FAILURE);
    }

  num_screens = ScreenCount (dpy);
  if ((screens = (screen_info *)malloc (sizeof (screen_info) * num_screens)) == NULL)
    {
      PRINT_ERROR ("Out of memory!\n");
      exit (EXIT_FAILURE);
    }  

  PRINT_DEBUG ("%d screens.\n", num_screens);

  /* Initialize the screens */
  for (i=0; i<num_screens; i++)
    {
      init_screen (&screens[i], i);
    }

  /* Set an initial window as active. */
  rp_current_window = rp_window_head;
  set_active_window (rp_current_window);
  
  handle_events ();

  return EXIT_SUCCESS;
}

static void
init_screen (screen_info *s, int screen_num)
{
  XColor fg_color, bg_color, bold_color, junk;
  XGCValues gv;

  s->screen_num = screen_num;
  s->root = RootWindow (dpy, screen_num);
  s->def_cmap = DefaultColormap (dpy, screen_num);
  s->font = font;
  XGetWindowAttributes (dpy, s->root, &s->root_attr);

  /* Get our program bar colors */
  if (!XAllocNamedColor (dpy, s->def_cmap, BAR_FG_COLOR, &fg_color, &junk))
    {
      fprintf (stderr, "ratpoison: Unknown color '%s'\n", BAR_FG_COLOR);
    }

  if (!XAllocNamedColor (dpy, s->def_cmap, BAR_BG_COLOR, &bg_color, &junk))
    {
      fprintf (stderr, "ratpoison: Unknown color '%s'\n", BAR_BG_COLOR);
    }

  if (!XAllocNamedColor (dpy, s->def_cmap, BAR_BOLD_COLOR, &bold_color, &junk))
    {
      fprintf (stderr, "ratpoison: Unknown color '%s'\n", BAR_BOLD_COLOR);
    }

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
  gv.foreground = bold_color.pixel;
  s->bold_gc = XCreateGC(dpy, s->root, 
			 GCForeground | GCBackground | GCFunction 
			 | GCLineWidth | GCSubwindowMode | GCFont, 
			 &gv);

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
