#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "ratpoison.h"

static void init_screen (screen_info *s, int screen_num);

Atom wm_state;
Atom wm_change_state;
Atom wm_protocols;
Atom wm_delete;
Atom wm_take_focus;
Atom wm_colormaps;

Atom rp_restart;

screen_info *screens;
int num_screens;
Display *dpy;
int exit_signal = 0;		/* Set by the signal handler. if this
                                   is set, quit. */
static XFontStruct *font;

char **myargv;

void
sighandler ()
{
  fprintf (stderr, "ratpoison: Agg! I've been SHOT!\n"); 
  clean_up ();
  exit (EXIT_FAILURE);
}

void
hup_handler ()
{
  /* This doesn't seem to restart more than once for some reason...*/

  fprintf (stderr, "ratpoison: Restarting with a fresh plate.\n"); 
  clean_up ();
  execvp(myargv[0], myargv);
}

void
alrm_handler ()
{
  int i;

#ifdef DEBUG
  printf ("alarm recieved.\n");
#endif

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
  if (e->request_code == X_ChangeWindowAttributes && e->error_code == BadAccess) {
    fprintf(stderr, "ratpoison: There can be only ONE.\n");
    exit(EXIT_FAILURE);
  }  

  fprintf (stderr, "ratpoison: Ya some error happened, but whatever.\n");
  return 0;
}

int
main (int argc, char *argv[])
{
  int i;

  myargv = argv;

  if (!(dpy = XOpenDisplay (NULL)))
    {
      fprintf (stderr, "Can't open display\n");
      return EXIT_FAILURE;
    }

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
      fprintf (stderr, "ratpoison:main.c:Out of memory!\n");
      exit (EXIT_FAILURE);
    }  

  printf ("%d screens.\n", num_screens);

  /* Initialize the screens */
  for (i=0; i<num_screens; i++)
    {
      init_screen (&screens[i], i);
    }

  /* Setup signal handlers. */
  //  XSetErrorHandler(handler);  
  if (signal (SIGALRM, alrm_handler) == SIG_IGN) signal (SIGALRM, SIG_IGN);
  if (signal (SIGTERM, sighandler) == SIG_IGN) signal (SIGTERM, SIG_IGN);
  if (signal (SIGINT, sighandler) == SIG_IGN) signal (SIGINT, SIG_IGN);
  if (signal (SIGHUP, hup_handler) == SIG_IGN) signal (SIGHUP, SIG_IGN);

  /* Set our Atoms */
  wm_state = XInternAtom(dpy, "WM_STATE", False);
  wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);
  wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
  wm_colormaps = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);

  rp_restart = XInternAtom (dpy, "RP_RESTART", False);

  XSync (dpy, False);

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
      fprintf (stderr, "Unknown color '%s'\n", BAR_FG_COLOR);
    }

  if (!XAllocNamedColor (dpy, s->def_cmap, BAR_BG_COLOR, &bg_color, &junk))
    {
      fprintf (stderr, "Unknown color '%s'\n", BAR_BG_COLOR);
    }

  if (!XAllocNamedColor (dpy, s->def_cmap, BAR_BOLD_COLOR, &bold_color, &junk))
    {
      fprintf (stderr, "Unknown color '%s'\n", BAR_BOLD_COLOR);
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
  XSync (dpy, 0);

  /* Create the program bar window. */
  s->bar_is_raised = 0;
  s->bar_window = XCreateSimpleWindow (dpy, s->root, 0, 0,
				       1, 1, 1, fg_color.pixel, bg_color.pixel);

  /* Setup the window that will recieve all keystrokes once the prefix
     key has been pressed. */
  s->key_window = XCreateSimpleWindow (dpy, s->root, 0, 0, 1, 1, 0, WhitePixel (dpy, 0), BlackPixel (dpy, 0));
  XSelectInput (dpy, s->bar_window, StructureNotifyMask);
  XMapWindow (dpy, s->key_window);
  grab_keys (s);

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
