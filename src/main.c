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

/* Command line options */
static struct option ratpoison_longopts[] =
  { {"help",            no_argument,            0,      'h'},
    {"interactive",     no_argument,            0,      'i'},
    {"version", no_argument,            0,      'v'},
    {"command", required_argument,      0,      'c'},
    {"display", required_argument,      0,      'd'},
    {"file",            required_argument,      0,      'f'},
    {0,         0,                      0,      0} };

static char ratpoison_opts[] = "hvic:d:s:f:";

static void
sighandler (int signum UNUSED)
{
  kill_signalled++;
}

static void
hup_handler (int signum UNUSED)
{
  hup_signalled++;
}

static void
alrm_handler (int signum UNUSED)
{
  alarm_signalled++;
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
  free (rp_error_msg);
  rp_error_msg = xstrdup (error_msg);

  return 0;
}

static void
print_version (void)
{
  printf ("%s %s\n", PACKAGE, VERSION);
  printf ("Copyright (C) 2000-2008 Shawn Betts\n\n");

  exit (EXIT_SUCCESS);
}

static void
print_help (void)
{
  printf ("Help for %s %s\n\n", PACKAGE, VERSION);
  printf ("-h, --help            Display this help screen\n");
  printf ("-v, --version         Display the version\n");
  printf ("-d, --display <dpy>   Set the X display to use\n");
  printf ("-c, --command <cmd>   Send ratpoison a colon-command\n");
  printf ("-i, --interactive     Execute commands in interactive mode\n");
  printf ("-f, --file <file>     Specify an alternative configuration file\n\n");

  printf ("Report bugs to %s\n\n", PACKAGE_BUGREPORT);

  exit (EXIT_SUCCESS);
}

static int
read_startup_files (const char *alt_rcfile)
{
  FILE *fileptr = NULL;

  if (alt_rcfile)
    {
      if ((fileptr = fopen (alt_rcfile, "r")) == NULL)
        {
          PRINT_ERROR (("ratpoison: could not open %s (%s)\n", alt_rcfile,
                        strerror (errno)));
          return -1;
        }
    }
  else
    {
      const char *homedir;
      char *filename;

      /* first check $HOME/.ratpoisonrc */
      homedir = get_homedir ();
      if (!homedir)
        PRINT_ERROR (("ratpoison: no home directory!?\n"));
      else
        {
          filename = xsprintf ("%s/.ratpoisonrc", homedir);
          fileptr = fopen (filename, "r");
          if (fileptr == NULL && errno != ENOENT)
            PRINT_ERROR (("ratpoison: could not open %s (%s)\n",
                          filename, strerror (errno)));
          free (filename);
        }

      if (fileptr == NULL)
        {
          /* couldn't open $HOME/.ratpoisonrc, fall back on system config */
          filename = xsprintf ("%s/ratpoisonrc", SYSCONFDIR);

          fileptr = fopen (filename, "r");
          if (fileptr == NULL && errno != ENOENT)
            PRINT_ERROR (("ratpoison: could not open %s (%s)\n",
                          filename, strerror (errno)));
          free (filename);
        }
    }

  if (fileptr != NULL)
    {
      set_close_on_exec (fileno (fileptr));
      read_rc_file (fileptr);
      fclose (fileptr);
    }
  return 0;
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
  defaults.only_border         = 1;
  defaults.bar_x_padding       = 4;
  defaults.bar_y_padding       = 0;
  defaults.bar_location        = NorthEastGravity;
  defaults.bar_timeout         = 5;
  defaults.bar_border_width    = 1;
  defaults.bar_in_padding      = 0;

  defaults.frame_indicator_timeout = 1;
  defaults.frame_resize_unit = 10;

  defaults.padding_left   = 0;
  defaults.padding_right  = 0;
  defaults.padding_top    = 0;
  defaults.padding_bottom = 0;

#ifdef USE_XFT_FONT
  defaults.font_string = xstrdup (DEFAULT_XFT_FONT);
#else
  /* Attempt to load a font */
  defaults.font = load_query_font_set (dpy, DEFAULT_FONT);
  if (defaults.font == NULL)
    {
      PRINT_ERROR (("ratpoison: Cannot load font %s.\n", DEFAULT_FONT));
      defaults.font = load_query_font_set (dpy, BACKUP_FONT);
      if (defaults.font == NULL)
        {
          PRINT_ERROR (("ratpoison: Cannot load backup font %s . You lose.\n", BACKUP_FONT));
          exit (EXIT_FAILURE);
        }
    }

  defaults.font_string = xstrdup (DEFAULT_FONT);
  set_extents_of_fontset (defaults.font);
#endif

  defaults.fgcolor_string = xstrdup ("black");
  defaults.bgcolor_string = xstrdup ("white");
  defaults.fwcolor_string = xstrdup ("black");
  defaults.bwcolor_string = xstrdup ("black");

  defaults.wait_for_key_cursor = 1;

  defaults.window_fmt = xstrdup ("%n%s%t");
  defaults.info_fmt = xstrdup ("(%H, %W) %n(%t)");
  defaults.frame_fmt = xstrdup ("Current Frame");

  defaults.win_name = WIN_NAME_TITLE;
  defaults.startup_message = 1;
  defaults.warp = 0;
  defaults.window_list_style = STYLE_COLUMN;

  defaults.history_size = 20;
  defaults.history_compaction = True;
  defaults.history_expansion = False;
  defaults.frame_selectors = xstrdup ("");
  defaults.maxundos = 20;
}

int
main (int argc, char *argv[])
{
  int c;
  char **cmd = NULL;
  int cmd_count = 0;
  char *display = NULL;
  unsigned char interactive = 0;
  char *alt_rcfile = NULL;

  setlocale (LC_CTYPE, "");
  utf8_check_locale();

  if (XSupportsLocale ())
    {
      if (!XSetLocaleModifiers (""))
	PRINT_ERROR (("Couldn't set X locale modifiers.\n"));
    }
  else
    PRINT_ERROR (("X doesn't seem to support your locale.\n"));

  /* Parse the arguments */
  myargv = argv;
  while (1)
    {
      c = getopt_long (argc, argv, ratpoison_opts, ratpoison_longopts, NULL);
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
          cmd = xrealloc (cmd, sizeof (char *) * (cmd_count + 1));
          cmd[cmd_count++] = xstrdup (optarg);
          break;
        case 'd':
          display = optarg;
          break;
        case 'i':
          interactive = 1;
          break;
        case 'f':
          alt_rcfile = optarg;
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
      exit (EXIT_FAILURE);
    }

  set_close_on_exec (ConnectionNumber (dpy));

  /* Set ratpoison specific Atoms. */
  rp_command = XInternAtom (dpy, "RP_COMMAND", False);
  rp_command_request = XInternAtom (dpy, "RP_COMMAND_REQUEST", False);
  rp_command_result = XInternAtom (dpy, "RP_COMMAND_RESULT", False);
  rp_selection = XInternAtom (dpy, "RP_SELECTION", False);

  /* TEXT atoms */
  xa_string = XA_STRING;
  xa_compound_text = XInternAtom(dpy, "COMPOUND_TEXT", False);
  xa_utf8_string = XInternAtom(dpy, "UTF8_STRING", False);

  if (cmd_count > 0)
    {
      int j, exit_status = EXIT_SUCCESS;

      for (j = 0; j < cmd_count; j++)
        {
          if (!send_command (interactive, (unsigned char *)cmd[j]))
	    exit_status = EXIT_FAILURE;
          free (cmd[j]);
        }

      free (cmd);
      XCloseDisplay (dpy);
      return exit_status;
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
  _net_wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
  _net_wm_window_type_dialog = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  _net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);

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
  init_defaults ();
  init_xkb ();
  init_groups ();
  init_window_stuff ();
  init_xrandr ();
  init_screens ();

  init_frame_lists ();
  update_modifier_map ();
  init_user_commands();
  initialize_default_keybindings ();
  history_load ();

  scanwins ();

  if (read_startup_files (alt_rcfile) == -1)
    return EXIT_FAILURE;

  /* Indicate to the user that ratpoison has booted. */
  if (defaults.startup_message)
    show_welcome_message();

  /* If no window has focus, give the key_window focus. */
  if (current_window() == NULL)
    set_window_focus (rp_current_screen->key_window);

  listen_for_events ();

  return EXIT_SUCCESS;
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
