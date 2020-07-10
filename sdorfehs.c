/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>

#include "sdorfehs.h"

static void
sighandler(int signum)
{
	kill_signalled++;
}

static void
hup_handler(int signum)
{
	hup_signalled++;
}

static void
alrm_handler(int signum)
{
	alarm_signalled++;
}

static int
handler(Display *d, XErrorEvent *e)
{
	char error_msg[100];

	if (e->request_code == X_ChangeWindowAttributes &&
	    e->error_code == BadAccess)
		errx(1, "another window manager is already running");

#ifdef IGNORE_BADWINDOW
	return 0;
#else
	if (ignore_badwindow && e->error_code == BadWindow)
		return 0;
#endif

	XGetErrorText(d, e->error_code, error_msg, sizeof(error_msg));
	warnx("X error: %s", error_msg);

	/*
	 * If there is already an error to report, replace it with this new one.
	 */
	free(rp_error_msg);
	rp_error_msg = xstrdup(error_msg);

	return 0;
}

static void
print_help(void)
{
	printf("%s %s\n", PROGNAME, VERSION);
	printf("usage: %s [-h]\n", PROGNAME);
	printf("       %s [-d dpy] [-c cmd] [-i] [-f file]\n", PROGNAME);
	exit(0);
}

static int
read_startup_files(const char *alt_rcfile)
{
	FILE *fileptr = NULL;
	char *config_dir, *filename;

	if (alt_rcfile && ((fileptr = fopen(alt_rcfile, "r")) == NULL)) {
		warn("could not open %s\n", alt_rcfile);
		return -1;
	}

	config_dir = get_config_dir();
	filename = xsprintf("%s/config", config_dir);
	fileptr = fopen(filename, "r");
	if (fileptr == NULL && errno != ENOENT)
		warn("could not open %s", filename);
	free(config_dir);
	free(filename);

	if (fileptr != NULL) {
		set_close_on_exec(fileno(fileptr));
		read_rc_file(fileptr);
		fclose(fileptr);
		return 1;
	}

	return 0;
}

/*
 * Odd that we spend so much code on making sure the silly welcome message is
 * correct.  Oh well...
 */
static void
show_welcome_message(void)
{
	rp_action *help_action;
	char *prefix, *help;
	rp_keymap *map;

	prefix = keysym_to_string(prefix_key.sym, prefix_key.state);

	map = find_keymap(ROOT_KEYMAP);

	/* Find the help key binding. */
	help_action = find_keybinding_by_action("help " ROOT_KEYMAP, map);
	if (help_action)
		help = keysym_to_string(help_action->key, help_action->state);
	else
		help = NULL;


	if (help) {
		/*
		 * A little kludge to use ? instead of `question' for the help key.
		 */
		if (!strcmp(help, "question"))
			marked_message_printf(0, 0, MESSAGE_WELCOME, prefix, "?");
		else
			marked_message_printf(0, 0, MESSAGE_WELCOME, prefix, help);

		free(help);
	} else {
		marked_message_printf(0, 0, MESSAGE_WELCOME, prefix, ":help");
	}

	free(prefix);
}

static void
init_defaults(void)
{
	unsigned long atom = 0;

	defaults.top_kmap = xstrdup(TOP_KEYMAP);

	defaults.win_gravity = NorthWestGravity;
	defaults.trans_gravity = CenterGravity;
	defaults.maxsize_gravity = CenterGravity;

	defaults.input_window_size = 200;
	defaults.window_border_width = 1;
	defaults.only_border = 1;
	defaults.bar_x_padding = 14;
	defaults.bar_y_padding = 10;
	defaults.bar_location = NorthWestGravity;
	defaults.bar_timeout = 3;
	defaults.bar_border_width = 0;
	defaults.bar_in_padding = 0;
	defaults.bar_sticky = 1;

	defaults.frame_indicator_timeout = 1;
	defaults.frame_resize_unit = 10;

	defaults.padding_left = 0;
	defaults.padding_right = 0;
	defaults.padding_top = 0;
	defaults.padding_bottom = 0;

	defaults.font_string = xstrdup(DEFAULT_XFT_FONT);

	defaults.fgcolor_string = xstrdup("#eeeeee");
	defaults.bgcolor_string = xstrdup("black");
	defaults.fwcolor_string = xstrdup("black");
	defaults.bwcolor_string = xstrdup("black");

	defaults.wait_for_key_cursor = 1;

	defaults.window_fmt = xstrdup("%n%s%t");
	defaults.info_fmt = xstrdup("(%H, %W) %n(%t)");
	defaults.frame_fmt = xstrdup("Frame %f (%Wx%H)");
	defaults.sticky_fmt = xstrdup("%t");
	defaults.resize_fmt = xstrdup("Resize frame (%Wx%H)");

	defaults.win_name = WIN_NAME_TITLE;
	defaults.startup_message = 1;
	defaults.warp = 0;
	defaults.window_list_style = STYLE_COLUMN;

	defaults.history_size = 20;
	defaults.frame_selectors = xstrdup("");
	defaults.maxundos = 20;

	get_atom(DefaultRootWindow(dpy), _net_number_of_desktops, XA_CARDINAL,
	    0, &atom, 1, NULL);
	if (atom > 0)
		defaults.vscreens = (int)atom;
	else
		defaults.vscreens = 12;

	defaults.gap = 20;

	defaults.ignore_resize_hints = 0;
}

int
main(int argc, char *argv[])
{
	int c;
	char **cmd = NULL;
	int cmd_count = 0;
	char *display = NULL;
	unsigned char interactive = 0;
	char *alt_rcfile = NULL;

	setlocale(LC_CTYPE, "");

	if (XSupportsLocale()) {
		if (!XSetLocaleModifiers(""))
			warnx("couldn't set X locale modifiers");
	} else
		warnx("X doesn't seem to support your locale");

	/* Parse the arguments */
	myargv = argv;
	while ((c = getopt(argc, argv, "c:d:hif:")) != -1) {
		switch (c) {
		case 'c':
			cmd = xrealloc(cmd, sizeof(char *) * (cmd_count + 1));
			cmd[cmd_count++] = xstrdup(optarg);
			break;
		case 'd':
			display = optarg;
			break;
		case 'f':
			alt_rcfile = optarg;
			break;
		case 'h':
			print_help();
			break;
		case 'i':
			interactive = 1;
			break;
		default:
			warnx("unsupported option %c", c);
			print_help();
		}
	}

	/* Report extra unparsed arguments. */
	if (optind < argc) {
		warnx("bogus arguments:");
		while (optind < argc)
			fprintf(stderr, "%s ", argv[optind++]);
		fputc('\n', stderr);
		exit(1);
	}
	if (!(dpy = XOpenDisplay(display)))
		errx(1, "can't open display %s", display);
	set_close_on_exec(ConnectionNumber(dpy));

	/* Set our own specific Atoms. */
	rp_command = XInternAtom(dpy, "RP_COMMAND", False);
	rp_command_request = XInternAtom(dpy, "RP_COMMAND_REQUEST", False);
	rp_command_result = XInternAtom(dpy, "RP_COMMAND_RESULT", False);
	rp_selection = XInternAtom(dpy, "RP_SELECTION", False);

	/* TEXT atoms */
	xa_string = XA_STRING;
	xa_compound_text = XInternAtom(dpy, "COMPOUND_TEXT", False);
	xa_utf8_string = XInternAtom(dpy, "UTF8_STRING", False);

	if (cmd_count > 0) {
		int j, exit_status = 0;

		for (j = 0; j < cmd_count; j++) {
			if (!send_command(interactive, (unsigned char *) cmd[j]))
				exit_status = 1;
			free(cmd[j]);
		}

		free(cmd);
		XCloseDisplay(dpy);
		return exit_status;
	}

	/* must be first */
	register_atom(&_net_supported, "_NET_SUPPORTED");

	register_atom(&wm_change_state, "WM_CHANGE_STATE");
	register_atom(&wm_colormaps, "WM_COLORMAP_WINDOWS");
	register_atom(&wm_delete, "WM_DELETE_WINDOW");
	register_atom(&wm_name, "WM_NAME");
	register_atom(&wm_protocols, "WM_PROTOCOLS");
	register_atom(&wm_state, "WM_STATE");
	register_atom(&wm_take_focus, "WM_TAKE_FOCUS");

	register_atom(&_net_active_window, "_NET_ACTIVE_WINDOW");
	register_atom(&_net_client_list, "_NET_CLIENT_LIST");
	register_atom(&_net_client_list_stacking, "_NET_CLIENT_LIST_STACKING");
	register_atom(&_net_current_desktop, "_NET_CURRENT_DESKTOP");
	register_atom(&_net_number_of_desktops, "_NET_NUMBER_OF_DESKTOPS");
	register_atom(&_net_workarea, "_NET_WORKAREA");

	register_atom(&_net_wm_name, "_NET_WM_NAME");
	register_atom(&_net_wm_pid, "_NET_WM_PID");
	register_atom(&_net_wm_state, "_NET_WM_STATE");
	register_atom(&_net_wm_state_fullscreen, "_NET_WM_STATE_FULLSCREEN");
	register_atom(&_net_wm_window_type, "_NET_WM_WINDOW_TYPE");
	register_atom(&_net_wm_window_type_dialog, "_NET_WM_WINDOW_TYPE_DIALOG");
	register_atom(&_net_wm_window_type_dock, "_NET_WM_WINDOW_TYPE_DOCK");
	register_atom(&_net_wm_window_type_splash, "_NET_WM_WINDOW_TYPE_SPLASH");
	register_atom(&_net_wm_window_type_tooltip,
	    "_NET_WM_WINDOW_TYPE_TOOLTIP");
	register_atom(&_net_wm_window_type_utility,
	    "_NET_WM_WINDOW_TYPE_UTILITY");

	/* Setup signal handlers. */
	XSetErrorHandler(handler);
	set_sig_handler(SIGALRM, alrm_handler);
	set_sig_handler(SIGTERM, sighandler);
	set_sig_handler(SIGINT, sighandler);
	set_sig_handler(SIGHUP, hup_handler);
	set_sig_handler(SIGCHLD, chld_handler);

	if (bar_mkfifo() == -1)
		return 1;

	/* Setup our internal structures */
	init_defaults();
	init_window_stuff();
	init_xrandr();
	init_screens();

	set_atom(rp_glob_screen.root, _net_number_of_desktops, XA_CARDINAL,
	    (unsigned long *)&defaults.vscreens, 1);

	init_frame_lists();
	update_modifier_map();
	init_user_commands();
	initialize_default_keybindings();
	history_load();
	init_bar();

	scanwins();

	c = read_startup_files(alt_rcfile);
	if (c == -1)
		return 1;
	else if (c == 0) {
		/* No config file, just do something basic. */
		cmdret *result;
		if ((result = command(0, "hsplit")))
			cmdret_free(result);
	}

	/* Indicate to the user that we have booted. */
	if (defaults.startup_message)
		show_welcome_message();

	/* If no window has focus, give the key_window focus. */
	if (current_window() == NULL)
		set_window_focus(rp_current_screen->key_window);

#ifdef __OpenBSD__
	/* cpath/wpath/fattr needed for history_save() */
	pledge("stdio rpath cpath wpath fattr unix proc exec", NULL);
#endif

	listen_for_events();

	return 0;
}
