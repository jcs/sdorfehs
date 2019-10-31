/*
 * Manage windows, such as Mapping them and making sure the proper key Grabs
 * have been put in place.
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>

#include "sdorfehs.h"

static char **unmanaged_window_list = NULL;
static int num_unmanaged_windows = 0;

void
clear_unmanaged_list(void)
{
	if (unmanaged_window_list) {
		int i;

		for (i = 0; i < num_unmanaged_windows; i++)
			free(unmanaged_window_list[i]);

		free(unmanaged_window_list);

		unmanaged_window_list = NULL;
	}
	num_unmanaged_windows = 0;
}

char *
list_unmanaged_windows(void)
{
	char *tmp = NULL;

	if (unmanaged_window_list) {
		struct sbuf *buf;
		int i;

		buf = sbuf_new(0);

		for (i = 0; i < num_unmanaged_windows; i++) {
			sbuf_concat(buf, unmanaged_window_list[i]);
			sbuf_concat(buf, "\n");
		}
		sbuf_chop(buf);
		tmp = sbuf_free_struct(buf);
	}
	return tmp;
}

void
add_unmanaged_window(char *name)
{
	char **tmp;

	if (!name)
		return;

	tmp = xmalloc((num_unmanaged_windows + 1) * sizeof(char *));

	if (unmanaged_window_list) {
		memcpy(tmp, unmanaged_window_list,
		    num_unmanaged_windows * sizeof(char *));
		free(unmanaged_window_list);
	}
	tmp[num_unmanaged_windows] = xstrdup(name);
	num_unmanaged_windows++;

	unmanaged_window_list = tmp;
}

void
grab_top_level_keys(Window w)
{
	rp_keymap *map = find_keymap(defaults.top_kmap);
	int i;

	if (map == NULL) {
		warnx("unable to find %s level keymap", defaults.top_kmap);
		return;
	}
	PRINT_INPUT_DEBUG(("grabbing top level key\n"));
	for (i = 0; i < map->actions_last; i++) {
		PRINT_INPUT_DEBUG(("%d\n", i));
		grab_key(map->actions[i].key, map->actions[i].state, w);
	}
}

void
ungrab_top_level_keys(Window w)
{
	XUngrabKey(dpy, AnyKey, AnyModifier, w);
}

void
ungrab_keys_all_wins(void)
{
	rp_window *cur;

	/* Remove the grab on the current prefix key */
	list_for_each_entry(cur, &rp_mapped_window, node) {
		ungrab_top_level_keys(cur->w);
	}
}

void
grab_keys_all_wins(void)
{
	rp_window *cur;

	/* Remove the grab on the current prefix key */
	list_for_each_entry(cur, &rp_mapped_window, node) {
		grab_top_level_keys(cur->w);
	}
}

void
update_normal_hints(rp_window *win)
{
	long supplied;

	XGetWMNormalHints(dpy, win->w, win->hints, &supplied);

	/* Print debugging output for window hints. */
#ifdef DEBUG
	if (win->hints->flags & PMinSize)
		PRINT_DEBUG(("minx: %d miny: %d\n", win->hints->min_width,
		    win->hints->min_height));

	if (win->hints->flags & PMaxSize)
		PRINT_DEBUG(("maxx: %d maxy: %d\n", win->hints->max_width,
		    win->hints->max_height));

	if (win->hints->flags & PResizeInc)
		PRINT_DEBUG(("incx: %d incy: %d\n", win->hints->width_inc,
		    win->hints->height_inc));
#endif
}


static char *
get_wmname(Window w)
{
	char *name = NULL;
	XTextProperty text_prop;
	Atom type = None;
	int ret = None, n;
	unsigned long nitems, bytes_after;
	int format;
	char **cl;
	unsigned char *val = NULL;

	/*
	 * Try to use the window's _NET_WM_NAME ewmh property
	 */
	ret = XGetWindowProperty(dpy, w, _net_wm_name, 0, 40, False,
	    xa_utf8_string, &type, &format, &nitems,
	    &bytes_after, &val);
	/* We have a valid UTF-8 string */
	if (ret == Success && type == xa_utf8_string && format == 8 &&
	    nitems > 0) {
		name = xstrdup((char *) val);
		XFree(val);
		PRINT_DEBUG(("Fetching window name using "
		    "_NET_WM_NAME succeeded\n"));
		PRINT_DEBUG(("WM_NAME: %s\n", name));
		return name;
	}
	/* Something went wrong for whatever reason */
	if (ret == Success && val)
		XFree(val);
	PRINT_DEBUG(("Could not fetch window name using _NET_WM_NAME\n"));

	if (XGetWMName(dpy, w, &text_prop) == 0) {
		PRINT_DEBUG(("XGetWMName failed\n"));
		return NULL;
	}
	PRINT_DEBUG(("WM_NAME encoding: "));
	if (text_prop.encoding == xa_string)
		PRINT_DEBUG(("STRING\n"));
	else if (text_prop.encoding == xa_compound_text)
		PRINT_DEBUG(("COMPOUND_TEXT\n"));
	else if (text_prop.encoding == xa_utf8_string)
		PRINT_DEBUG(("UTF8_STRING\n"));
	else
		PRINT_DEBUG(("unknown (%d)\n", (int) text_prop.encoding));

	/*
	 * It seems that most applications supporting UTF8_STRING and
	 * _NET_WM_NAME don't bother making their WM_NAME available as
	 * UTF8_STRING (but only as either STRING or COMPOUND_TEXT). Let's try
	 * anyway.
	 */
	if (text_prop.encoding == xa_utf8_string) {
		ret = Xutf8TextPropertyToTextList(dpy, &text_prop, &cl, &n);
		PRINT_DEBUG(("Xutf8TextPropertyToTextList: %s\n",
			ret == Success ? "success" : "error"));
	} else {
		/*
		 * XmbTextPropertyToTextList should be fine for all cases, even
		 * UTF8_STRING encoded WM_NAME
		 */
		ret = XmbTextPropertyToTextList(dpy, &text_prop, &cl, &n);
		PRINT_DEBUG(("XmbTextPropertyToTextList: %s\n",
			ret == Success ? "success" : "error"));
	}

	if (ret == Success && cl && n > 0) {
		name = xstrdup(cl[0]);
		XFreeStringList(cl);
	} else if (text_prop.value) {
		/* Convertion failed, try to get the raw string */
		name = xstrdup((char *) text_prop.value);
		XFree(text_prop.value);
	}
	if (name == NULL) {
		PRINT_DEBUG(("I can't get the WMName.\n"));
	} else {
		PRINT_DEBUG(("WM_NAME: '%s'\n", name));
	}

	return name;
}

static XClassHint *
get_class_hints(Window w)
{
	XClassHint *class;

	class = XAllocClassHint();

	if (class == NULL)
		errx(1, "not enough memory for WM_CLASS structure");

	XGetClassHint(dpy, w, class);

	return class;
}

/*
 * Reget the WM_NAME property for the window and update its name. Return 1 if
 * the name changed.
 */
int
update_window_name(rp_window *win)
{
	char *newstr;
	int changed = 0;
	XClassHint *class;

	newstr = get_wmname(win->w);
	if (newstr != NULL) {
		changed = changed || win->wm_name == NULL ||
		    strcmp(newstr, win->wm_name);
		free(win->wm_name);
		win->wm_name = newstr;
	}
	class = get_class_hints(win->w);

	if (class->res_class != NULL
	    && (win->res_class == NULL || strcmp(class->res_class, win->res_class))) {
		changed = 1;
		free(win->res_class);
		win->res_class = xstrdup(class->res_class);
	}
	if (class->res_name != NULL
	    && (win->res_name == NULL || strcmp(class->res_name, win->res_name))) {
		changed = 1;
		free(win->res_name);
		win->res_name = xstrdup(class->res_name);
	}
	XFree(class->res_name);
	XFree(class->res_class);
	XFree(class);
	return changed;
}

/*
 * This function is used to determine if the window should be treated as a
 * transient.
 */
int
window_is_transient(rp_window *win)
{
	return win->transient
#ifdef ASPECT_WINDOWS_ARE_TRANSIENTS
	|| win->hints->flags & PAspect
#endif
#ifdef MAXSIZE_WINDOWS_ARE_TRANSIENTS
	|| (win->hints->flags & PMaxSize
	    && (win->hints->max_width < win->vscr->screen->width
		|| win->hints->max_height < win->vscr->screen->height))
#endif
	;
}

Atom
get_net_wm_window_type(rp_window *win)
{
	Atom type, window_type = None;
	int format;
	unsigned long nitems;
	unsigned long bytes_left;
	unsigned char *data;

	if (win == NULL)
		return None;

	if (XGetWindowProperty(dpy, win->w, _net_wm_window_type, 0, 1L,
	    False, XA_ATOM, &type, &format, &nitems, &bytes_left,
	    &data) == Success && nitems > 0) {
		window_type = *(Atom *)data;
		XFree(data);
		PRINT_DEBUG(("_NET_WM_WINDOW_TYPE = %ld (%s)\n", window_type,
		    XGetAtomName(dpy, window_type)));
	}
	return window_type;
}

void
update_window_information(rp_window *win)
{
	XWindowAttributes attr;

	update_window_name(win);

	/* Get the WM Hints */
	update_normal_hints(win);

	/* Get the colormap */
	XGetWindowAttributes(dpy, win->w, &attr);
	win->colormap = attr.colormap;
	win->x = attr.x;
	win->y = attr.y;
	win->width = attr.width;
	win->height = attr.height;
	win->border = attr.border_width;

	/* Transient status */
	win->transient = XGetTransientForHint(dpy, win->w, &win->transient_for);

	if (get_net_wm_window_type(win) == _net_wm_window_type_dialog)
		win->transient = 1;

	PRINT_DEBUG(("update_window_information: x:%d y:%d width:%d height:%d "
	    "transient:%d\n", win->x, win->y, win->width, win->height,
	    win->transient));

	update_window_gravity(win);
}

void
unmanage(rp_window *w)
{
	list_del(&w->node);
	groups_del_window(w);

	remove_atom(rp_glob_screen.root, _net_client_list, XA_WINDOW, w->w);
	remove_atom(rp_glob_screen.root, _net_client_list_stacking, XA_WINDOW,
	    w->w);

	free_window(w);
}

/* When starting up scan existing windows and start managing them. */
void
scanwins(void)
{
	rp_window *win;
	XWindowAttributes attr;
	unsigned int i, nwins;
	Window dw1, dw2, *wins;

	XQueryTree(dpy, rp_glob_screen.root, &dw1, &dw2, &wins, &nwins);
	PRINT_DEBUG(("windows: %d\n", nwins));

	for (i = 0; i < nwins; i++) {
		rp_screen *screen;

		XGetWindowAttributes(dpy, wins[i], &attr);
		if (is_rp_window(wins[i])
		    || attr.override_redirect == True
		    || unmanaged_window(wins[i]))
			continue;

		screen = find_screen_by_attr(attr);
		if (!screen)
			list_first(screen, &rp_screens, node);

		win = add_to_window_list(screen, wins[i]);

		PRINT_DEBUG(("map_state: %s\n",
			attr.map_state == IsViewable ? "IsViewable" :
			attr.map_state == IsUnviewable ? "IsUnviewable" :
			"IsUnmapped"));
		PRINT_DEBUG(("state: %s\n",
			get_state(win) == IconicState ? "Iconic" :
			get_state(win) == NormalState ? "Normal" : "Other"));

		/* Collect mapped and iconized windows. */
		if (attr.map_state == IsViewable
		    || (attr.map_state == IsUnmapped
			&& get_state(win) == IconicState))
			map_window(win);
	}

	XFree(wins);
}

int
unmanaged_window(Window w)
{
	rp_window tmp;
	Atom win_type;
	char *wname;
	int i;

	if (!unmanaged_window_list)
		return 0;

	wname = get_wmname(w);
	if (!wname)
		return 0;

	for (i = 0; i < num_unmanaged_windows; i++) {
		if (!strcmp(unmanaged_window_list[i], wname)) {
			free(wname);
			return 1;
		}
	}

	free(wname);

	tmp.w = w;
	win_type = get_net_wm_window_type(&tmp);
	if (win_type == _net_wm_window_type_dock ||
	    win_type == _net_wm_window_type_splash ||
	    win_type == _net_wm_window_type_tooltip)
		return 1;

	return 0;
}

/* Set the state of the window. */
void
set_state(rp_window *win, int state)
{
	unsigned long data[2];

	win->state = state;

	data[0] = (long) win->state;
	data[1] = (long) None;

	set_atom(win->w, wm_state, wm_state, data, 2);
}

/* Get the WM state of the window. */
long
get_state(rp_window *win)
{
	long state = WithdrawnState;
	Atom type;
	int format;
	unsigned long nitems;
	unsigned long bytes_left;
	unsigned char *data;

	if (win == NULL)
		return state;

	if (XGetWindowProperty(dpy, win->w, wm_state, 0L, 2L, False, wm_state,
	    &type, &format, &nitems, &bytes_left, &data) == Success &&
	    nitems > 0) {
		state = *(long *)data;
		XFree(data);
	}
	return state;
}

void
check_state(rp_window *win)
{
	Atom state;
	unsigned long read, left;
	int i, fs;

	for (i = 0, left = 1; left; i += read) {
		read = get_atom(win->w, _net_wm_state, XA_ATOM, i, &state, 1,
		    &left);
		if (!read)
			break;

		if (state == _net_wm_state_fullscreen) {
			fs = 1;
			window_full_screen(win);
		} else {
			PRINT_DEBUG(("unhandled window state %ld (%s)\n",
			    state, XGetAtomName(dpy, state)));
		}
	}

	if (win->full_screen && !fs)
		window_full_screen(NULL);
}

static void
move_window(rp_window *win)
{
	rp_frame *frame;
	int t, t2;

	if (win->frame_number == EMPTY) {
		PRINT_DEBUG(("%s: window has no frame\n", __func__));
		return;
	}

	frame = win_get_frame(win);

	if (win->full_screen) {
		win->x = win->vscr->screen->left;
		win->y = win->vscr->screen->top;
		return;
	}

	/* X coord. */
	switch (win->gravity) {
	case NorthWestGravity:
	case WestGravity:
	case SouthWestGravity:
		win->x = frame->x +
		    (defaults.gap * (frame_left_screen_edge(frame) ? 1 : 0.5));
		break;
	case NorthGravity:
	case CenterGravity:
	case SouthGravity:
		t = (defaults.gap * (frame_left_screen_edge(frame) ? 1 : 0.5));
		t2 = (defaults.gap * (frame_right_screen_edge(frame) ? 1 : 0.5));
		win->x = frame->x + t +
		    ((frame->width - t - t2 - win->width) / 2);
		break;
	case NorthEastGravity:
	case EastGravity:
	case SouthEastGravity:
		win->x = frame->x + frame->width - win->width -
		    (defaults.gap * (frame_right_screen_edge(frame) ? 1 : 0.5));
		break;
	}

	/* Y coord. */
	switch (win->gravity) {
	case NorthEastGravity:
	case NorthGravity:
	case NorthWestGravity:
		win->y = frame->y +
		    (defaults.gap * (frame_top_screen_edge(frame) ? 1 : 0.5));
		break;
	case EastGravity:
	case CenterGravity:
	case WestGravity:
		t = (defaults.gap * (frame_top_screen_edge(frame) ? 1 : 0.5));
		t2 = (defaults.gap * (frame_bottom_screen_edge(frame) ? 1 : 0.5));
		win->y = frame->y + t +
		    ((frame->height - t - t2 - win->height) / 2);
		break;
	case SouthEastGravity:
	case SouthGravity:
	case SouthWestGravity:
		win->y = frame->y + frame->height - win->height -
		    (defaults.gap * (frame_bottom_screen_edge(frame) ? 1 : 0.5));
		break;
	}

	if (win->x < frame->x)
		win->x = frame->x;
	if (win->y < frame->y)
		win->y = frame->y;
}

/*
 * set a good standard window's x,y,width,height fields to maximize the window.
 */
static void
maximize_window(rp_window *win, int transient)
{
	rp_frame *frame;
	int maxw, maxh;
	float gap;

	frame = win_get_frame(win);

	/* We can't maximize a window if it has no frame. */
	if (frame == NULL) {
		PRINT_DEBUG(("%s: no frame\n", __func__));
		return;
	}

	/* Set the window's border */
	if ((defaults.only_border == 0 && num_frames(win->vscr) <= 1) ||
	    win->full_screen)
		win->border = 0;
	else
		win->border = defaults.window_border_width;

	if (win->full_screen) {
		maxw = win->vscr->screen->width;
		maxh = win->vscr->screen->height;
	} else {
		if (win->hints->flags & PMaxSize) {
			maxw = win->hints->max_width;
			maxh = win->hints->max_height;
		} else if (transient) {
			maxw = win->width;
			maxh = win->height;
		} else {
			maxw = frame->width;
			maxh = frame->height;
		}

		if (maxw > frame->width)
			maxw = frame->width;
		if (maxh > frame->height)
			maxh = frame->height;

		PRINT_DEBUG(("adjusted to frame, maxsize %d %d\n", maxw, maxh));

		if (!transient) {
			gap = (frame_right_screen_edge(frame) ? 1 : 0.5);
			gap += (frame_left_screen_edge(frame) ? 1 : 0.5);
			if (maxw > (frame->width - (gap * defaults.gap)))
				maxw -= gap * defaults.gap;

			gap = (frame_top_screen_edge(frame) ? 1 : 0.5);
			gap += (frame_bottom_screen_edge(frame) ? 1 : 0.5);
			if (maxh > (frame->height - (gap * defaults.gap)))
				maxh -= gap * defaults.gap;

			if (!(defaults.only_border == 0 &&
			    num_frames(win->vscr) <= 1)) {
				maxw -= win->border * 2;
				maxh -= win->border * 2;
			}
		}
	}

	/* Honour the window's aspect ratio. */
	PRINT_DEBUG(("aspect: %ld\n", win->hints->flags & PAspect));
	if (!win->full_screen && (win->hints->flags & PAspect)) {
		float ratio = (float) maxw / maxh;
		float min_ratio = (float) win->hints->min_aspect.x /
		    win->hints->min_aspect.y;
		float max_ratio = (float) win->hints->max_aspect.x /
		    win->hints->max_aspect.y;
		PRINT_DEBUG(("ratio=%f min_ratio=%f max_ratio=%f\n",
			ratio, min_ratio, max_ratio));
		if (ratio < min_ratio) {
			maxh = (int) (maxw / min_ratio);
		} else if (ratio > max_ratio) {
			maxw = (int) (maxh * max_ratio);
		}

		PRINT_DEBUG(("honored ratio, maxsize %d %d\n", maxw, maxh));
	}
	/*
	 * Make sure we maximize to the nearest Resize Increment specified by
	 * the window
	 */
	if (!defaults.ignore_resize_hints && !win->full_screen &&
	    (win->hints->flags & PResizeInc)) {
		int amount;
		int delta;

		if (win->hints->width_inc) {
			amount = maxw - win->width;
			delta = amount % win->hints->width_inc;
			if (amount < 0 && delta)
				amount -= win->hints->width_inc;
			amount -= delta;
			maxw = amount + win->width;
		}
		if (win->hints->height_inc) {
			amount = maxh - win->height;
			delta = amount % win->hints->height_inc;
			if (amount < 0 && delta)
				amount -= win->hints->height_inc;
			amount -= delta;
			maxh = amount + win->height;
		}

		PRINT_DEBUG(("applied width_inc/height_inc, maxsize %d %d\n",
		    maxw, maxh));
	}
	PRINT_DEBUG(("final maxsize: %d %d\n", maxw, maxh));

	win->width = maxw;
	win->height = maxh;
}

/*
 * Maximize the current window if data = 0, otherwise assume it is a pointer to
 * a window that should be maximized
 */
void
maximize(rp_window *win)
{
	if (!win)
		win = current_window();
	if (!win)
		return;

	/* Handle maximizing transient windows differently. */
	maximize_window(win, win->transient);

	/* Reposition the window. */
	move_window(win);

	PRINT_DEBUG(("Resizing %s window '%s' to x:%d y:%d w:%d h:%d\n",
	    win->transient ? "transient" : "normal",
	    window_name(win), win->x, win->y, win->width, win->height));

	/* Actually do the maximizing. */
	XMoveResizeWindow(dpy, win->w, win->vscr->screen->left + win->x,
	    win->vscr->screen->top + win->y, win->width, win->height);
	XSetWindowBorderWidth(dpy, win->w, win->border);

	XSync(dpy, False);
}

/*
 * Maximize the current window but don't treat transient windows differently.
 */
void
force_maximize(rp_window *win)
{
	if (!win)
		win = current_window();
	if (!win)
		return;

	maximize_window(win, 0);

	/* Reposition the window. */
	move_window(win);

	/*
	 * This little dance is to force a maximize event. If the window is
	 * already "maximized" X11 will optimize away the event since to
	 * geometry changes were made. This initial resize solves the problem.
	 */
	if (!defaults.ignore_resize_hints && (win->hints->flags & PResizeInc)) {
		XMoveResizeWindow(dpy, win->w, win->vscr->screen->left + win->x,
		    win->vscr->screen->top + win->y,
		    win->width + win->hints->width_inc,
		    win->height + win->hints->height_inc);
	} else {
		XResizeWindow(dpy, win->w, win->width + 1, win->height + 1);
	}

	XSync(dpy, False);

	/* Resize the window to its proper maximum size. */
	XMoveResizeWindow(dpy, win->w, win->vscr->screen->left + win->x,
	    win->vscr->screen->top + win->y, win->width, win->height);
	XSetWindowBorderWidth(dpy, win->w, win->border);

	XSync(dpy, False);
}

/* map the unmapped window win */
void
map_window(rp_window *win)
{
	PRINT_DEBUG(("Mapping the unmapped window %s\n", window_name(win)));

	/* Fill in the necessary data about the window */
	update_window_information(win);
	win->number = numset_request(rp_window_numset);
	grab_top_level_keys(win->w);

	/* Put win in the mapped window list */
	list_del(&win->node);
	insert_into_list(win, &rp_mapped_window);

	/* Update all groups. */
	groups_map_window(win->vscr, win);

	/*
	 * The window has never been accessed since it was brought back from the
	 * Withdrawn state.
	 */
	win->last_access = 0;

	/* It is now considered iconic and set_active_window can handle the
	 * rest. */
	set_state(win, IconicState);

	/* Depending on the rudeness level, actually map the window. */
	if ((rp_honour_transient_map && win->transient)
	    || (rp_honour_normal_map && !win->transient))
		set_active_window(win);
	else
		show_rudeness_msg(win, 0);

	append_atom(rp_glob_screen.root, _net_client_list, XA_WINDOW, &win->w,
	    1);
	append_atom(rp_glob_screen.root, _net_client_list_stacking, XA_WINDOW,
	    &win->w, 1);

	hook_run(&rp_new_window_hook);
}

void
hide_window(rp_window *win)
{
	if (win == NULL)
		return;

	/* An unmapped window is not inside a frame. */
	win->frame_number = EMPTY;

	/* Ignore the unmap_notify event. */
	XSelectInput(dpy, win->w, WIN_EVENTS & ~(StructureNotifyMask));
	XUnmapWindow(dpy, win->w);
	XSelectInput(dpy, win->w, WIN_EVENTS);
	/*
	 * Ensure that the window doesn't have the focused border color. This
	 * is needed by remove_frame and possibly others.
	 */
	XSetWindowBorder(dpy, win->w, rp_glob_screen.bw_color);
	set_state(win, IconicState);
}

void
unhide_window(rp_window * win)
{
	if (win == NULL)
		return;

	/* Always raise the window. */
	XRaiseWindow(dpy, win->w);

	if (win->state != IconicState)
		return;

	XMapWindow(dpy, win->w);
	set_state(win, NormalState);
}

void
unhide_all_windows(void)
{
	struct list_head *tmp, *iter;
	rp_window *win;

	list_for_each_safe_entry(win, iter, tmp, &rp_mapped_window, node)
	    unhide_window(win);
}

void
withdraw_window(rp_window *win)
{
	if (win == NULL)
		return;

	PRINT_DEBUG(("withdraw_window on '%s'\n", window_name(win)));

	if (win->full_screen)
		window_full_screen(NULL);

	/*
	 * Give back the window number. the window will get another one, if it is
	 * remapped.
	 */
	if (win->number == -1)
		warnx("attempting to withdraw '%s' with number -1!",
		    window_name(win));

	numset_release(rp_window_numset, win->number);
	win->number = -1;

	list_move_tail(&win->node, &rp_unmapped_window);

	/* Update the groups. */
	groups_unmap_window(win);

	ignore_badwindow++;

	XRemoveFromSaveSet(dpy, win->w);
	set_state(win, WithdrawnState);
	XSync(dpy, False);

	ignore_badwindow--;

	/* Call our hook */
	hook_run(&rp_delete_window_hook);
}

/* Hide all other mapped windows except for win in win's frame. */
void
hide_others(rp_window *win)
{
	rp_frame *frame;
	rp_window *cur;

	if (win == NULL)
		return;
	frame = find_windows_frame(win);
	if (frame == NULL)
		return;

	list_for_each_entry(cur, &rp_mapped_window, node) {
		if (find_windows_frame(cur)
		    || cur->state != NormalState
		    || cur->frame_number != frame->number)
			continue;

		hide_window(cur);
	}
}

/* Hide any window displayed on the given screen */
void
hide_vscreen_windows(rp_vscreen *v)
{
	rp_window *cur;

	list_for_each_entry(cur, &rp_mapped_window, node)
		if (cur->vscr == v)
			hide_window(cur);
}
