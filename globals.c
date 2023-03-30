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

#include "sdorfehs.h"

#include <sys/types.h>
#include <sys/wait.h>

#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <pwd.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>

/*
 * Several systems seem not to have WAIT_ANY defined, so define it if it isn't.
 */
#ifndef WAIT_ANY
#define WAIT_ANY -1
#endif

/* Some systems don't define the close-on-exec flag in fcntl.h */
#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif

int alarm_signalled = 0;
int kill_signalled = 0;
int hup_signalled = 0;
int chld_signalled = 0;

int rp_font_ascent, rp_font_descent, rp_font_width;

Atom wm_name;
Atom wm_state;
Atom wm_change_state;
Atom wm_protocols;
Atom wm_delete;
Atom wm_take_focus;
Atom wm_colormaps;

Atom rp_selection;

/* TEXT atoms */
Atom xa_string;
Atom xa_compound_text;
Atom xa_utf8_string;

/* netwm atoms */
Atom _net_active_window;
Atom _net_client_list;
Atom _net_client_list_stacking;
Atom _net_current_desktop;
Atom _net_number_of_desktops;
Atom _net_supported;
Atom _net_workarea;
Atom _net_wm_name;
Atom _net_wm_pid;
Atom _net_wm_state;
Atom _net_wm_state_fullscreen;
Atom _net_wm_window_type;
Atom _net_wm_window_type_dialog;
Atom _net_wm_window_type_dock;
Atom _net_wm_window_type_splash;
Atom _net_wm_window_type_tooltip;
Atom _net_wm_window_type_utility;
Atom _net_supporting_wm_check;

LIST_HEAD(rp_screens);
rp_screen *rp_current_screen;
rp_global_screen rp_glob_screen;

Display *dpy;

int rp_have_xrandr;

LIST_HEAD(rp_children);
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
int rp_honour_vscreen_switch = 0;

char *rp_error_msg = NULL;

/* Global frame numset */
struct numset *rp_frame_numset;

/* The X11 selection globals */
rp_xselection selection;

static void
x_export_selection(void)
{
	rp_screen *screen;

	list_first(screen, &rp_screens, node);
	if (!screen)
		return;

	/* Hang the selections off screen 0's key window. */
	XSetSelectionOwner(dpy, XA_PRIMARY, screen->key_window, CurrentTime);
	if (XGetSelectionOwner(dpy, XA_PRIMARY) != screen->key_window)
		warnx("can't get primary selection");
	XChangeProperty(dpy, screen->root, XA_CUT_BUFFER0, xa_string, 8,
	    PropModeReplace, (unsigned char *) selection.text, selection.len);
}

void
set_nselection(char *txt, int len)
{
	int i;

	/* Update the selection structure */
	free(selection.text);

	/* Copy the string by hand. */
	selection.text = xmalloc(len + 1);
	selection.len = len + 1;
	for (i = 0; i < len; i++)
		selection.text[i] = txt[i];
	selection.text[len] = 0;

	x_export_selection();
}

void
set_selection(char *txt)
{
	/* Update the selection structure */
	free(selection.text);
	selection.text = xstrdup(txt);
	selection.len = strlen(txt);

	x_export_selection();
}

static char *
get_cut_buffer(void)
{
	int nbytes;
	char *data;

	PRINT_DEBUG(("trying the cut buffer\n"));

	data = XFetchBytes(dpy, &nbytes);

	if (data) {
		struct sbuf *s = sbuf_new(0);
		sbuf_nconcat(s, data, nbytes);
		XFree(data);
		return sbuf_free_struct(s);
	} else
		return NULL;
}

/* Lifted the code from rxvt. */
static char *
get_primary_selection(void)
{
	long nread;
	unsigned long bytes_after;
	XTextProperty ct;
	struct sbuf *s = sbuf_new(0);

	for (nread = 0, bytes_after = 1; bytes_after > 0; nread += ct.nitems) {
		if ((XGetWindowProperty(dpy, rp_current_screen->input_window,
		    rp_selection, (nread / 4), 4096, True, AnyPropertyType,
		    &ct.encoding, &ct.format, &ct.nitems, &bytes_after,
		    &ct.value) != Success)) {
			XFree(ct.value);
			sbuf_free(s);
			return NULL;
		}
		if (ct.value == NULL)
			continue;
		/*
		 * Accumulate the data. FIXME: ct.value may not be NULL
		 * terminated.
		 */
		sbuf_nconcat(s, (const char *) ct.value, ct.nitems);
		XFree(ct.value);
	}
	return sbuf_free_struct(s);
}

char *
get_selection(void)
{
	Atom property;
	XEvent ev;
	rp_screen *s = rp_current_screen;
	int loops = 1000;

	/* Just insert our text, if we own the selection. */
	if (selection.text) {
		return xstrdup(selection.text);
	} else {
		/* be a good icccm citizen */
		XDeleteProperty(dpy, s->input_window, rp_selection);
		/*
		 * TODO: we shouldn't use CurrentTime here, use the time of the
		 * XKeyEvent, should we fake it?
		 */
		XConvertSelection(dpy, XA_PRIMARY, xa_string, rp_selection,
		    s->input_window, CurrentTime);

		/* This seems like a hack. */
		while (!XCheckTypedWindowEvent(dpy, s->input_window,
		    SelectionNotify, &ev)) {
			if (loops == 0) {
				warnx("selection request timed out");
				return NULL;
			}
			usleep(10000);
			loops--;
		}

		PRINT_DEBUG(("SelectionNotify event\n"));

		property = ev.xselection.property;

		if (property != None)
			return get_primary_selection();
		else
			return get_cut_buffer();
	}
}

/* The hook dictionary globals. */

LIST_HEAD(rp_key_hook);
LIST_HEAD(rp_switch_win_hook);
LIST_HEAD(rp_switch_frame_hook);
LIST_HEAD(rp_switch_screen_hook);
LIST_HEAD(rp_switch_vscreen_hook);
LIST_HEAD(rp_quit_hook);
LIST_HEAD(rp_restart_hook);
LIST_HEAD(rp_delete_window_hook);
LIST_HEAD(rp_new_window_hook);
LIST_HEAD(rp_title_changed_hook);

struct rp_hook_db_entry rp_hook_db[] =
	{{"key", &rp_key_hook},
	{"switchwin", &rp_switch_win_hook},
	{"switchframe", &rp_switch_frame_hook},
	{"switchscreen", &rp_switch_screen_hook},
	{"switchvscreen", &rp_switch_vscreen_hook},
	{"deletewindow", &rp_delete_window_hook},
	{"quit", &rp_quit_hook},
	{"restart", &rp_restart_hook},
	{"newwindow", &rp_new_window_hook},
	{"titlechanged", &rp_title_changed_hook},
	{NULL, NULL}
};

void
set_rp_window_focus(rp_window *win)
{
	PRINT_DEBUG(("Giving focus to '%s'\n", window_name(win)));
	XSetInputFocus(dpy, win->w,
	    RevertToPointerRoot, CurrentTime);
	set_atom(win->vscreen->screen->root, _net_active_window, XA_WINDOW,
	    &win->w, 1);
}

void
set_window_focus(Window window)
{
	PRINT_DEBUG(("Giving focus to %ld\n", window));
	XSetInputFocus(dpy, window,
	    RevertToPointerRoot, CurrentTime);
}

XftFont *
rp_get_font(rp_screen *s, char *font)
{
	XftFont *f;
	int fslots = sizeof(s->xft_font_cache) / sizeof(struct rp_font);
	int x;

	if (!font || font[0] == '\0')
		return s->xft_font;

	for (x = 0; x < fslots; x++) {
		if (!s->xft_font_cache[x].name)
			break;

		if (strcmp(s->xft_font_cache[x].name, font) == 0)
			return s->xft_font_cache[x].font;
	}

	/* not in the cache, make sure we can open it first */
	f = XftFontOpenName(dpy, DefaultScreen(dpy), font);
	if (!f) {
		warnx("failed opening xft font \"%s\"", font);
		return s->xft_font;
	}

	PRINT_DEBUG(("font \"%s\" not in font cache\n", font));

	/* free up the last slot if needed */
	if (x == fslots) {
		free(s->xft_font_cache[x - 1].name);
		XftFontClose(dpy, s->xft_font_cache[x - 1].font);
	}

	/* shift all the cache entries to free up the first slot */
	for (x = fslots - 1; x >= 1; x--)
		memcpy(&s->xft_font_cache[x], &s->xft_font_cache[x - 1],
		    sizeof(struct rp_font));

	s->xft_font_cache[0].name = xstrdup(font);
	s->xft_font_cache[0].font = f;

	return f;
}

void
rp_clear_cached_fonts(rp_screen *s)
{
	int x;

	for (x = 0; x < (sizeof(s->xft_font_cache) / sizeof(struct rp_font));
	    x++) {
		if (s->xft_font_cache[x].name) {
			free(s->xft_font_cache[x - 1].name);
			XftFontClose(dpy, s->xft_font_cache[x].font);
		}
	}
}

void
rp_draw_string(rp_screen *s, Drawable d, int style, int x, int y, char *string,
    int length, char *font, char *color)
{
	XftDraw *draw;
	XftColor xftcolor;
	XftFont *f = rp_get_font(s, font);

	if (length < 0)
		length = strlen(string);

	draw = XftDrawCreate(dpy, d, DefaultVisual(dpy, s->screen_num),
	    DefaultColormap(dpy, s->screen_num));
	if (!draw) {
		warnx("no Xft font available");
		return;
	}

	if (color == NULL) {
		if (style == STYLE_NORMAL)
			memcpy(&xftcolor, &s->xft_fg_color, sizeof(XftColor));
		else
			memcpy(&xftcolor, &s->xft_bg_color, sizeof(XftColor));
	} else {
		/*
		 * This won't actually allocate anything if the color is
		 * already allocated.
		 */
		if (!XftColorAllocName(dpy, DefaultVisual(dpy, s->screen_num),
		    DefaultColormap(dpy, s->screen_num), color, &xftcolor)) {
			warnx("couldn't XftColorAllocName \"%s\"", color);
			memcpy(&xftcolor, &s->xft_fg_color, sizeof(XftColor));
		}
	}

	XftDrawStringUtf8(draw, &xftcolor, f, x, y, (FcChar8 *)string, length);
	XftDrawDestroy(draw);
}

int
rp_text_width(rp_screen *s, char *string, int count, char *font)
{
	XGlyphInfo extents;
	XftFont *f = rp_get_font(s, font);

	if (count < 0)
		count = strlen(string);

	XftTextExtentsUtf8(dpy, f, (FcChar8 *)string, count, &extents);

	return extents.xOff;
}

/* A case insensitive strncmp. */
int
str_comp(char *s1, char *s2, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		if (toupper((unsigned char)s1[i]) != toupper((unsigned char)s2[i]))
			return 0;

	return 1;
}

/*
 * Check for child processes that have quit but haven't been acknowledged yet.
 * Update their structure.
 */
void
check_child_procs(void)
{
	rp_child_info *cur;
	int pid, status;
	while (1) {
		pid = waitpid(WAIT_ANY, &status, WNOHANG);
		if (pid <= 0)
			break;

		PRINT_DEBUG(("Child status: %d\n", WEXITSTATUS(status)));

		/* Find the child and update its structure. */
		list_for_each_entry(cur, &rp_children, node) {
			if (cur->pid == pid) {
				cur->terminated = 1;
				cur->status = WEXITSTATUS(status);
				break;
			}
		}

		chld_signalled = 1;
	}
}

void
chld_handler(int signum)
{
	int serrno;

	serrno = errno;
	check_child_procs();
	errno = serrno;
}

void
set_sig_handler(int sig, void (*action)(int))
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = action;
	sigemptyset(&act.sa_mask);
	if (sigaction(sig, &act, NULL))
		warnx("error setting signal handler");
}

void
set_close_on_exec(int fd)
{
	int flags = fcntl(fd, F_GETFD);
	if (flags >= 0)
		fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

void
read_rc_file(FILE *file)
{
	char *line;
	size_t linesize = 256;

	line = xmalloc(linesize);

	while (getline(&line, &linesize, file) != -1) {
		line[strcspn(line, "\n")] = '\0';

		PRINT_DEBUG(("rcfile line: %s\n", line));

		if (*line != '\0' && *line != '#') {
			cmdret *result;
			result = command(0, line);

			/* Gobble the result. */
			if (result)
				cmdret_free(result);
		}
	}

	free(line);
}

const char *
get_homedir(void)
{
	char *homedir;

	homedir = getenv("HOME");
	if (homedir != NULL && homedir[0] == '\0')
		homedir = NULL;

	if (homedir == NULL) {
		struct passwd *pw;

		pw = getpwuid(getuid());
		if (pw != NULL)
			homedir = pw->pw_dir;

		if (homedir != NULL && homedir[0] == '\0')
			homedir = NULL;
	}

	return homedir;
}

char *
get_config_dir(void)
{
	DIR *d;
	const char *homedir;
	char *xdg_config, *home_config;
	int xdg_alloc = 0;

	homedir = get_homedir();
	if (!homedir)
		errx(1, "no home directory");

	xdg_config = getenv("XDG_CONFIG_HOME");
	if (xdg_config == NULL || !strlen(xdg_config)) {
		xdg_config = xsprintf("%s/.config", homedir);
		xdg_alloc = 1;
	}

	if (!(d = opendir(xdg_config))) {
		if (mkdir(xdg_config, 0755) == -1)
			err(1, "failed creating %s", xdg_config);

		if (!(d = opendir(xdg_config)))
			err(1, "failed opening %s", xdg_config);
	}
	closedir(d);

	home_config = xsprintf("%s/sdorfehs", xdg_config);
	if (!(d = opendir(home_config))) {
		if (mkdir(home_config, 0755) == -1)
			err(1, "failed creating %s", home_config);

		if (!(d = opendir(home_config)))
			err(1, "failed opening %s", home_config);
	}
	closedir(d);

	if (xdg_alloc)
		free(xdg_config);

	return home_config;
}

void
clean_up(void)
{
	rp_screen *cur;
	rp_vscreen *vcur;
	struct list_head *iter, *tmp, *iter2, *tmp2;

	history_save();

	free_keymaps();
	free_aliases();
	free_user_commands();
	free_bar();
	free_window_stuff();

	list_for_each_safe_entry(cur, iter, tmp, &rp_screens, node) {
		list_for_each_safe_entry(vcur, iter2, tmp2, &cur->vscreens, node)
			vscreen_del(vcur);

		list_del(&cur->node);
		screen_free(cur);
		free(cur);
	}

	screen_free_final();

	/* Delete the undo histories */
	clear_frame_undos();

	/* Free the global frame numset shared by all screens. */
	numset_free(rp_frame_numset);

	free(defaults.window_fmt);

	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XCloseDisplay(dpy);
}

void
register_atom(Atom *a, char *name)
{
	*a = XInternAtom(dpy, name, False);
	PRINT_DEBUG(("Registered Atom %ld = %s\n", (unsigned long)*a, name));
	append_atom(DefaultRootWindow(dpy), _net_supported, XA_ATOM, a, 1);
}

int
set_atom(Window w, Atom a, Atom type, unsigned long *val, unsigned long nitems)
{
	return (XChangeProperty(dpy, w, a, type, 32, PropModeReplace,
	    (unsigned char *)val, nitems) == Success);
}

int
append_atom(Window w, Atom a, Atom type, unsigned long *val,
    unsigned long nitems)
{
	return (XChangeProperty(dpy, w, a, type, 32, PropModeAppend,
	    (unsigned char *)val, nitems) == Success);
}

unsigned long
get_atom(Window w, Atom a, Atom type, unsigned long off, unsigned long *ret,
    unsigned long nitems, unsigned long *left)
{
	Atom real_type;
	int i, real_format = 0;
	unsigned long items_read = 0;
	unsigned long bytes_left = 0;
	unsigned long *p;
	unsigned char *data;

	XGetWindowProperty(dpy, w, a, off, nitems, False, type, &real_type,
	    &real_format, &items_read, &bytes_left, &data);

	if (real_format == 32 && items_read) {
		p = (unsigned long *)data;
		for (i = 0; i < items_read; i++)
			*ret++ = *p++;
		XFree(data);
		if (left)
			*left = bytes_left;
		return items_read;
	}

	return 0;
}

void
remove_atom(Window w, Atom a, Atom type, unsigned long remove)
{
	unsigned long tmp, read, left, *new;
	int i, j = 0;

	read = get_atom(w, a, type, 0, &tmp, 1, &left);
	if (!read)
		return;

	new = malloc((read + left) * sizeof(*new));
	if (read && tmp != remove)
		new[j++] = tmp;

	for (i = 1, read = left = 1; read && left; i += read) {
		read = get_atom(w, a, type, i, &tmp, 1, &left);
		if (!read)
			break;
		if (tmp != remove)
			new[j++] = tmp;
	}

	if (j)
		XChangeProperty(dpy, w, a, type, 32, PropModeReplace,
		    (unsigned char *)new, j);
	else
		XDeleteProperty(dpy, w, a);

	free(new);
}
