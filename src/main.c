/* Ratpoison.
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
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
#include <ctype.h>

#include "ratpoison.h"

/* Several systems seem not to have WAIT_ANY defined, so define it if
   it isn't. */
#ifndef WAIT_ANY
# define WAIT_ANY -1
#endif

/* Command line options */
static struct option ratpoison_longopts[] =
  { {"help",            no_argument,            0,      'h'},
    {"interactive",     no_argument,            0,      'i'},
    {"version", no_argument,            0,      'v'},
    {"command", required_argument,      0,      'c'},
    {"display", required_argument,      0,      'd'},
    {"screen",  required_argument,      0,      's'},
    {"file",            required_argument,      0,      'f'},
    {0,         0,                      0,      0} };

static char ratpoison_opts[] = "hvic:d:s:f:";

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

/* strtok but do it for whitespace and be locale compliant. */
char *
strtok_ws (char *s)
{
  char *nonws;
  static char *pointer = NULL;

  if (s)
    pointer = s;
  
  /* skip to first non-whitespace char. */
  while (*pointer && isspace (*pointer)) pointer++;

  /* If we reached the end of the string here then there is no more
     data. */
  if (*pointer == 0)
    return NULL;

  /* Now skip to the end of the data. */
  nonws = pointer;
  while (*pointer && !isspace (*pointer)) pointer++;
  if (*pointer)
    {
      *pointer = 0;
      pointer++;
    }
  return nonws;
}

/* A case insensitive strncmp. */
int
str_comp (char *s1, char *s2, int len)
{
  int i;

  for (i=0; i<len; i++)
    if (toupper (s1[i]) != toupper (s2[i])) return 0;

  return 1;
}

static void
sighandler (int signum)
{
  kill_signalled++;
}

static void
hup_handler (int signum)
{
  hup_signalled++;
}

static void
alrm_handler (int signum)
{
  alarm_signalled++;
}

/* Check for child processes that have quit but haven't been
   acknowledged yet. Update their structure. */
void
check_child_procs (void)
{
  rp_child_info *cur;
  int pid, status;
  while (1)
    {
      pid = waitpid (WAIT_ANY, &status, WNOHANG);
      if (pid <= 0)
        break;

      PRINT_DEBUG(("Child status: %d\n", WEXITSTATUS (status)));

      /* Find the child and update its structure. */
      list_for_each_entry (cur, &rp_children, node)
        {
          if (cur->pid == pid)
            {
              cur->terminated = 1;
              cur->status = WEXITSTATUS (status);
              break;
            }
        }

      chld_signalled = 1;
    }
}

void
chld_handler (int signum)
{
  int serrno;

  serrno = errno;
  check_child_procs();
  errno = serrno;
}

static int
handler (Display *d, XErrorEvent *e)
{
  char error_msg[100];

  if (e->request_code == X_ChangeWindowAttributes && e->error_code == BadAccess)
    {
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

static void
print_version (void)
{
  printf ("%s %s (built %s %s)\n", PACKAGE, VERSION, __DATE__, __TIME__);
  printf ("Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006 Shawn Betts\n\n");

  exit (EXIT_SUCCESS);
}

static void
print_help (void)
{
  printf ("Help for %s %s\n\n", PACKAGE, VERSION);
  printf ("-h, --help            Display this help screen\n");
  printf ("-v, --version         Display the version\n");
  printf ("-d, --display <dpy>   Set the X display to use\n");
  printf ("-s, --screen <num>    Only use the specified screen\n");
  printf ("-c, --command <cmd>   Send ratpoison a colon-command\n");
  printf ("-i, --interactive     Execute commands in interactive mode\n");
  printf ("-f, --file <file>     Specify an alternative configuration file\n\n");

  printf ("Report bugs to ratpoison-devel@nongnu.org\n\n");

  exit (EXIT_SUCCESS);
}

void
set_close_on_exec (FILE *fd)
{
  int fnum = fileno (fd);
  int flags = fcntl (fnum, F_GETFD);
  if (flags >= 0)
    fcntl (fnum, F_SETFD, flags | FD_CLOEXEC);
}

void
read_rc_file (FILE *file)
{
  size_t n = 256;
  char *partial;
  char *line;
  size_t linesize = n;

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
              cmdret *result;
              result = command (0, line);

              /* Gobble the result. */
              if (result)
                cmdret_free (result);
            }

          *line = '\0';
        }
    }


  free (line);
  free (partial);
}

static void
read_startup_files (char *alt_rcfile)
{
  char *homedir;
  FILE *fileptr = NULL;

  if (alt_rcfile)
    {
      if ((fileptr = fopen (alt_rcfile, "r")) == NULL)
        {
          /* we probably don't need to report this, its not an error */
          PRINT_DEBUG (("ratpoison: could not open %s\n", alt_rcfile));
        }
    }
  else
    {
      /* first check $HOME/.ratpoisonrc and if that does not exist then try
         /etc/ratpoisonrc */

      homedir = getenv ("HOME");
      if (!homedir)
        {
          PRINT_ERROR (("ratpoison: $HOME not set!?\n"));
        }
      else
        {
          char *filename;
          filename = xsprintf ("%s/.ratpoisonrc", homedir);

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
          free (filename);
        }
    }

  if (fileptr)
    {
      set_close_on_exec(fileptr);
      read_rc_file (fileptr);
      fclose (fileptr);
    }
}

/* Odd that we spend so much code on making sure the silly welcome
   message is correct. Oh well... */
static void
show_welcome_message (void)
{
  rp_action *help_action;
  char *prefix, *help;
  rp_keymap *map;

  prefix = keysym_to_string (prefix_key.sym, prefix_key.state);

  map = find_keymap (ROOT_KEYMAP);

  /* Find the help key binding. */
  help_action = find_keybinding_by_action ("help " ROOT_KEYMAP, map);
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
init_defaults (void)
{
  defaults.top_kmap = xstrdup(TOP_KEYMAP);

  defaults.win_gravity     = NorthWestGravity;
  defaults.trans_gravity   = CenterGravity;
  defaults.maxsize_gravity = CenterGravity;

  defaults.input_window_size   = 200;
  defaults.window_border_width = 1;
  defaults.bar_x_padding       = 4;
  defaults.bar_y_padding       = 0;
  defaults.bar_location        = NorthEastGravity;
  defaults.bar_timeout         = 5;
  defaults.bar_border_width    = 1;

  defaults.frame_indicator_timeout = 1;
  defaults.frame_resize_unit = 10;

  defaults.padding_left   = 0;
  defaults.padding_right  = 0;
  defaults.padding_top    = 0;
  defaults.padding_bottom = 0;

  defaults.font = load_query_font_set (dpy, DEFAULT_FONT);
  if (defaults.font == NULL)
    {
      fprintf (stderr, "ratpoison: Cannot load font %s.\n", DEFAULT_FONT);
      exit (EXIT_FAILURE);
    }
  defaults.font_string = xstrdup (DEFAULT_FONT);
  set_extents_of_fontset (defaults.font);

  defaults.fgcolor_string = xstrdup ("black");
  defaults.bgcolor_string = xstrdup ("white");

  defaults.wait_for_key_cursor = 1;

  defaults.window_fmt = xstrdup ("%n%s%t");
  defaults.info_fmt = xstrdup ("(%H, %W) %n(%t)");
  defaults.frame_fmt = xstrdup ("Current Frame");

  defaults.win_name = WIN_NAME_TITLE;
  defaults.startup_message = 1;
  defaults.warp = 0;
  defaults.window_list_style = STYLE_COLUMN;

  defaults.history_size = 20;
  defaults.frame_selectors = xstrdup ("");
  defaults.maxundos = 20;
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
  unsigned char interactive = 0;
  char *alt_rcfile = NULL;

  myargv = argv;
  setlocale(LC_CTYPE, "");

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
        case 'i':
          interactive = 1;
          break;
        case 'f':
          alt_rcfile = xstrdup (optarg);
          break;

        default:
          exit (EXIT_FAILURE);
        }
    }

  /* Report extra unparsed arguments. */
  if (optind < argc)
    {
      fprintf (stderr, "Error: junk arguments: ");
      while (optind < argc)
        fprintf (stderr, "%s ", argv[optind++]);
      fputc ('\n', stderr);
      exit (EXIT_FAILURE);
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
  rp_selection = XInternAtom (dpy, "RP_SELECTION", False);

  if (cmd_count > 0)
    {
      int i;

      for (i=0; i<cmd_count; i++)
        {
          if (screen_arg)
            send_command (interactive, (unsigned char *)command[i], screen_num);
          else
            send_command (interactive, (unsigned char *)command[i], -1);

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

  /* netwm atoms */
  _net_wm_pid = XInternAtom(dpy, "_NET_WM_PID", False);
  PRINT_DEBUG (("_NET_WM_PID = %ld\n", _net_wm_pid));
  _net_supported = XInternAtom(dpy, "_NET_SUPPORTED", False);
  PRINT_DEBUG (("_NET_SUPPORTED = %ld\n", _net_supported));

  /* Setup signal handlers. */
  XSetErrorHandler(handler);
  set_sig_handler (SIGALRM, alrm_handler);
  set_sig_handler (SIGTERM, sighandler);
  set_sig_handler (SIGINT, sighandler);
  set_sig_handler (SIGHUP, hup_handler);
  set_sig_handler (SIGCHLD, chld_handler);

  /* Add RATPOISON to the environment */
  putenv (xsprintf ("RATPOISON=%s", argv[0]));

  /* Setup ratpoison's internal structures */
  init_globals ();
  init_defaults ();
  init_groups ();
  init_window_stuff ();
  init_xinerama ();
  init_screens (screen_arg, screen_num);

  init_frame_lists ();
  update_modifier_map ();
  init_user_commands();
  initialize_default_keybindings ();
#ifdef HAVE_HISTORY
  history_load ();
#endif /* HAVE_HISTORY */

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

  read_startup_files (alt_rcfile);
  if (alt_rcfile)
    free (alt_rcfile);

  /* Indicate to the user that ratpoison has booted. */
  if (defaults.startup_message)
    show_welcome_message();

  /* If no window has focus, give the key_window focus. */
  if (current_window() == NULL)
    set_window_focus (current_screen()->key_window);

  listen_for_events ();

  return EXIT_SUCCESS;
}

static void
free_screen (rp_screen *s)
{
  rp_frame *frame;
  struct list_head *iter, *tmp;

  /* Relinquish our hold on the root window. */
  XSelectInput(dpy, RootWindow (dpy, s->screen_num), 0);

  list_for_each_safe_entry (frame, iter, tmp, &s->frames, node)
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
}

void
clean_up (void)
{
  int i;

#ifdef HAVE_HISTORY
  history_save ();
#endif /* HAVE_HISTORY */

  free_keymaps ();
  free_aliases ();
  free_bar ();
  free_groups ();

  free_window_stuff ();

  for (i=0; i<num_screens; i++)
    {
      free_screen (&screens[i]);
    }
  free (screens);

  /* Delete the undo histories */
  while (list_size (&rp_frame_undos) > 0)
    {
      /* Delete the oldest node */
      rp_frame_undo *cur;
      list_last (cur, &rp_frame_undos, node);
      del_frame_undo (cur);
    }

  /* Free the global frame numset shared by all screens. */
  numset_free (rp_frame_numset);

  free_xinerama();

  XFreeFontSet (dpy, defaults.font);
  free (defaults.window_fmt);

  XSetInputFocus (dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
  XCloseDisplay (dpy);
}

void
set_extents_of_fontset (XFontSet font)
{
  XFontSetExtents *extent;
  extent = XExtentsOfFontSet(font);
  rp_font_ascent = extent->max_logical_extent.height * 9 / 10;
  rp_font_descent = extent->max_logical_extent.height / 5;
  rp_font_width = extent->max_logical_extent.width;
}

XFontSet load_query_font_set (Display *disp, const char *fontset_name)
{
  XFontSet fontset;
  int  missing_charset_count;
  char **missing_charset_list;
  char *def_string;

  fontset = XCreateFontSet(disp, fontset_name,
                           &missing_charset_list, &missing_charset_count,
                           &def_string);
  if (missing_charset_count) {
    PRINT_DEBUG (("Missing charsets in FontSet(%s) creation.\n", fontset_name));
    XFreeStringList(missing_charset_list);
  }
  return fontset;
}
