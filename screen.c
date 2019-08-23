/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
 * Copyright (C) 2016 Mathieu OTHACEHE <m.othacehe@gmail.com>
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
#include <string.h>
#include <X11/cursorfont.h>

static void init_screen(rp_screen *s);

int
screen_width(rp_screen *s)
{
	return s->width - defaults.padding_right - defaults.padding_left;
}

int
screen_height(rp_screen *s)
{
	int ret = s->height - defaults.padding_bottom - defaults.padding_top;

	if (defaults.bar_sticky) {
		switch (defaults.bar_location) {
		case NorthEastGravity:
		case NorthGravity:
		case NorthWestGravity:
		case SouthEastGravity:
		case SouthGravity:
		case SouthWestGravity:
			ret -= sticky_bar_height(s);
			break;
		}
	}

	return ret;
}

int
screen_left(rp_screen *s)
{
	return s->left + defaults.padding_left;
}

int
screen_right(rp_screen *s)
{
	return screen_left(s) + s->width - defaults.padding_right;
}

int
screen_top(rp_screen *s)
{
	int ret = s->top + defaults.padding_top;

	if (defaults.bar_sticky) {
		switch (defaults.bar_location) {
		case NorthEastGravity:
		case NorthGravity:
		case NorthWestGravity:
			ret += sticky_bar_height(s);
			break;
		}
	}

	return ret;
}

int
screen_bottom(rp_screen *s)
{
	int ret = s->height - defaults.padding_bottom;

	if (defaults.bar_sticky) {
		switch (defaults.bar_location) {
		case SouthEastGravity:
		case SouthGravity:
		case SouthWestGravity:
			ret -= sticky_bar_height(s);
			break;
		}
	}

	return ret;
}

/* Given a root window, return the rp_screen struct */
rp_screen *
find_screen(Window w)
{
	rp_screen *cur;

	list_for_each_entry(cur, &rp_screens, node) {
		if (cur->root == w)
			return cur;
	}

	return NULL;
}

/* Given a window attr, return the rp_screen struct */
rp_screen *
find_screen_by_attr(XWindowAttributes attr)
{
	rp_screen *cur;

	list_for_each_entry(cur, &rp_screens, node) {
		if (attr.x >= cur->left &&
		    attr.x <= cur->left + cur->width &&
		    attr.y >= cur->top &&
		    attr.y <= cur->top + cur->height)
			return cur;
	}

	return NULL;
}

/* Return 1 if w is a root window of any of the screens. */
int
is_a_root_window(unsigned int w)
{
	rp_screen *cur;

	list_for_each_entry(cur, &rp_screens, node) {
		if (cur->root == w)
			return 1;
	}

	return 0;
}

rp_screen *
screen_number(int number)
{
	rp_screen *cur;

	list_for_each_entry(cur, &rp_screens, node) {
		if (cur->number == number)
			return cur;
	}

	return NULL;
}

static int
screen_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	rp_screen *sc_a = container_of(a, typeof(*sc_a), node);
	rp_screen *sc_b = container_of(b, typeof(*sc_b), node);

	if (sc_a->left < sc_b->left)
		return -1;
	if (sc_a->left > sc_b->left)
		return 1;

	if (sc_a->top > sc_b->top)
		return -1;
	if (sc_a->top < sc_b->top)
		return 1;

	return 0;
}

void
screen_sort(void)
{
	return list_sort(NULL, &rp_screens, screen_cmp);
}

static void
screen_set_numbers(void)
{
	rp_screen *cur;

	list_for_each_entry(cur, &rp_screens, node) {
		cur->number = numset_request(rp_glob_screen.numset);
	}
}

static void
screen_select_primary(void)
{
	rp_screen *cur;

	/* By default, take the first screen as current screen */
	list_first(cur, &rp_screens, node);
	if (!rp_current_screen)
		rp_current_screen = cur;

	if (!rp_have_xrandr)
		return;

	list_for_each_entry(cur, &rp_screens, node) {
		if (xrandr_is_primary(cur)) {
			rp_current_screen = cur;
			PRINT_DEBUG(("Xrandr primary screen %d detected\n",
				rp_current_screen->number));
			break;
		}
	}
}

static void
init_global_screen(rp_global_screen *s)
{
	XColor color, junk;
	int screen_num;

	screen_num = DefaultScreen(dpy);
	s->root = RootWindow(dpy, screen_num);

	s->numset = numset_new();
	s->fg_color = WhitePixel(dpy, screen_num);

	if (XAllocNamedColor(dpy, DefaultColormap(dpy, screen_num),
	    defaults.fgcolor_string, &color, &junk))
		rp_glob_screen.fg_color = color.pixel;
	else {
		PRINT_ERROR(("failed allocating fgcolor %s\n",
		    defaults.fgcolor_string));
		s->fg_color = WhitePixel(dpy, screen_num);
	}

	if (XAllocNamedColor(dpy, DefaultColormap(dpy, screen_num),
	    defaults.bgcolor_string, &color, &junk))
		rp_glob_screen.bg_color = color.pixel;
	else {
		PRINT_ERROR(("failed allocating bgcolor %s\n",
		    defaults.bgcolor_string));
		s->bg_color = BlackPixel(dpy, screen_num);
	}

	if (XAllocNamedColor(dpy, DefaultColormap(dpy, screen_num),
	    defaults.fwcolor_string, &color, &junk))
		rp_glob_screen.fw_color = color.pixel;
	else {
		PRINT_ERROR(("failed allocating fwcolor %s\n",
		    defaults.fwcolor_string));
		s->fw_color = BlackPixel(dpy, screen_num);
	}

	if (XAllocNamedColor(dpy, DefaultColormap(dpy, screen_num),
	    defaults.bwcolor_string, &color, &junk))
		rp_glob_screen.bw_color = color.pixel;
	else {
		PRINT_ERROR(("failed allocating bwcolor %s\n",
		    defaults.bwcolor_string));
		s->bw_color = BlackPixel(dpy, screen_num);
	}
}

void
init_screens(void)
{
	int i;
	int screen_count = 0;
	int *rr_outputs = NULL;
	rp_screen *screen;

	/* Get the number of screens */
	if (rp_have_xrandr)
		screen_count = xrandr_query_screen(&rr_outputs);
	else
		screen_count = ScreenCount(dpy);

	/* Create our global frame numset */
	rp_frame_numset = numset_new();

	init_global_screen(&rp_glob_screen);

	for (i = 0; i < screen_count; i++) {
		screen = xmalloc(sizeof(*screen));
		list_add(&screen->node, &rp_screens);

		if (rp_have_xrandr)
			xrandr_fill_screen(rr_outputs[i], screen);
		else
			xrandr_fill_screen(i, screen);

		init_screen(screen);
	}

	screen_sort();
	screen_set_numbers();
	screen_select_primary();

	free(rr_outputs);
}

static void
init_rat_cursor(rp_screen *s)
{
	s->rat = XCreateFontCursor(dpy, XC_icon);
}

static void
init_screen(rp_screen *s)
{
	XGCValues gcv;
	struct sbuf *buf;
	rp_vscreen *vscreen;
	char *colon;
	int screen_num, x;

	screen_num = DefaultScreen(dpy);

	if (!rp_have_xrandr) {
		s->left = 0;
		s->top = 0;
		s->width = DisplayWidth(dpy, screen_num);
		s->height = DisplayHeight(dpy, screen_num);
	}
	/*
	 * Select on some events on the root window, if this fails, then there
	 * is already a WM running and the X Error handler will catch it,
	 * terminating us.
	 */
	XSelectInput(dpy, RootWindow(dpy, screen_num),
	    PropertyChangeMask | ColormapChangeMask
	    | SubstructureRedirectMask | SubstructureNotifyMask
	    | StructureNotifyMask);
	XSync(dpy, False);

	INIT_LIST_HEAD(&s->vscreens);
	s->vscreens_numset = numset_new();

	for (x = 0; x < defaults.vscreens; x++) {
		vscreen = xmalloc(sizeof(rp_vscreen));
		init_vscreen(vscreen, s);
		list_add_tail(&vscreen->node, &s->vscreens);

		if (x == 0)
			s->current_vscreen = vscreen;
	}

	s->scratch_buffer = NULL;

	/* Build the display string for each screen */
	buf = sbuf_new(0);
	sbuf_printf(buf, "DISPLAY=%s", DisplayString(dpy));
	colon = strrchr(sbuf_get(buf), ':');
	if (colon) {
		char *dot;

		dot = strrchr(sbuf_get(buf), '.');
		if (!dot || dot < colon) {
			/*
			 * no dot was found or it belongs to fqdn - append
			 * screen_num to the end
			 */
			sbuf_printf_concat(buf, ".%d", screen_num);
		}
	}
	s->display_string = sbuf_free_struct(buf);

	PRINT_DEBUG(("display string: %s\n", s->display_string));

	s->root = RootWindow(dpy, screen_num);
	s->screen_num = screen_num;
	s->def_cmap = DefaultColormap(dpy, screen_num);
	s->full_screen_win = NULL;

	init_rat_cursor(s);

	/* Setup the GC for drawing the font. */
	gcv.foreground = rp_glob_screen.fg_color;
	gcv.background = rp_glob_screen.bg_color;
	gcv.function = GXcopy;
	gcv.line_width = 1;
	gcv.subwindow_mode = IncludeInferiors;
	s->normal_gc = XCreateGC(dpy, s->root,
	    GCForeground | GCBackground | GCFunction
	    | GCLineWidth | GCSubwindowMode,
	    &gcv);
	gcv.foreground = rp_glob_screen.bg_color;
	gcv.background = rp_glob_screen.fg_color;
	s->inverse_gc = XCreateGC(dpy, s->root,
	    GCForeground | GCBackground | GCFunction
	    | GCLineWidth | GCSubwindowMode,
	    &gcv);

	/* Create the program bar window. */
	s->bar_is_raised = 0;
	s->bar_window = XCreateSimpleWindow(dpy, s->root, 0, 0, 1, 1,
	    defaults.bar_border_width,
	    rp_glob_screen.fg_color, rp_glob_screen.bg_color);

	/*
	 * Setup the window that will receive all keystrokes once the prefix
	 * key has been pressed.
	 */
	s->key_window = XCreateSimpleWindow(dpy, s->root, 0, 0, 1, 1, 0,
	    WhitePixel(dpy, screen_num),
	    BlackPixel(dpy, screen_num));
	XSelectInput(dpy, s->key_window, KeyPressMask | KeyReleaseMask);

	/* Create the input window. */
	s->input_window = XCreateSimpleWindow(dpy, s->root, 0, 0, 1, 1,
	    defaults.bar_border_width,
	    rp_glob_screen.fg_color, rp_glob_screen.bg_color);
	XSelectInput(dpy, s->input_window, KeyPressMask | KeyReleaseMask);

	/* Create the frame indicator window */
	s->frame_window = XCreateSimpleWindow(dpy, s->root, 1, 1, 1, 1,
	    defaults.bar_border_width, rp_glob_screen.fg_color,
	    rp_glob_screen.bg_color);

	/* Create the help window */
	s->help_window = XCreateSimpleWindow(dpy, s->root, s->left, s->top,
	    s->width, s->height, 0, rp_glob_screen.fg_color,
	    rp_glob_screen.bg_color);
	XSelectInput(dpy, s->help_window, KeyPressMask);

	activate_screen(s);

	XSync(dpy, 0);

	s->xft_font = XftFontOpenName(dpy, screen_num, DEFAULT_XFT_FONT);
	if (s->xft_font) {
		if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen_num),
		    DefaultColormap(dpy, screen_num),
		    defaults.fgcolor_string, &s->xft_fg_color)) {
			PRINT_ERROR(("Failed to allocate font fg color\n"));
			XftFontClose(dpy, s->xft_font);
			s->xft_font = NULL;
		}
		if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen_num),
		    DefaultColormap(dpy, screen_num),
		    defaults.bgcolor_string, &s->xft_bg_color)) {
			PRINT_ERROR(("Failed to allocate font fg color\n"));
			XftFontClose(dpy, s->xft_font);
			s->xft_font = NULL;
		}
	} else {
		PRINT_ERROR(("Failed to open font\n"));
	}
}

void
activate_screen(rp_screen *s)
{
	/* Add netwm support. FIXME: I think this is busted. */
	XChangeProperty(dpy, RootWindow(dpy, s->screen_num),
	    _net_supported, XA_ATOM, 32, PropModeReplace,
	    (unsigned char *) &_net_wm_pid, 1);

	/* set window manager name */
	XChangeProperty(dpy, RootWindow(dpy, s->screen_num),
	    _net_wm_name, xa_utf8_string, 8, PropModeReplace,
	    (unsigned char *)PROGNAME, strlen(PROGNAME));
	XMapWindow(dpy, s->key_window);
}

void
deactivate_screen(rp_screen *s)
{
	/* Unmap its key window */
	XUnmapWindow(dpy, s->key_window);

	/* delete everything so noone sees them while we are not there */
	XDeleteProperty(dpy, RootWindow(dpy, s->screen_num), _net_supported);
	XDeleteProperty(dpy, RootWindow(dpy, s->screen_num), _net_wm_name);
}

static int
is_rp_window_for_given_screen(Window w, rp_screen *s)
{
	if (w != s->key_window &&
	    w != s->bar_window &&
	    w != s->input_window &&
	    w != s->frame_window &&
	    w != s->help_window)
		return 0;
	return 1;
}

int
is_rp_window(Window w)
{
	rp_screen *cur;

	list_for_each_entry(cur, &rp_screens, node) {
		if (is_rp_window_for_given_screen(w, cur))
			return 1;
	}

	return 0;
}

char *
screen_dump(rp_screen *screen)
{
	char *tmp;
	struct sbuf *s;

	s = sbuf_new(0);
	if (rp_have_xrandr)
		sbuf_printf(s, "%s ", screen->xrandr.name);

	sbuf_printf_concat(s, "%d %d %d %d %d %d",
	    screen->number,
	    screen->left,
	    screen->top,
	    screen->width,
	    screen->height,
	    (rp_current_screen == screen) ? 1 : 0	/* is current? */
	    );

	/* Extract the string and return it, and don't forget to free s. */
	tmp = sbuf_get(s);
	free(s);
	return tmp;
}

int
screen_count(void)
{
	return list_size(&rp_screens);
}

rp_screen *
screen_next(void)
{
	return list_next_entry(rp_current_screen, &rp_screens, node);
}

rp_screen *
screen_prev(void)
{
	return list_prev_entry(rp_current_screen, &rp_screens, node);
}

static void
screen_remove_current(void)
{
	rp_screen *new_screen;
	rp_frame *new_frame;
	rp_window *cur_win;
	int cur_frame;

	cur_win = current_window();
	new_screen = screen_next();

	cur_frame = new_screen->current_vscreen->current_frame;
	new_frame = vscreen_get_frame(new_screen->current_vscreen, cur_frame);

	set_active_frame(new_frame, 1);

	hide_window(cur_win);
}

void
screen_update(rp_screen *s, int left, int top, int width, int height)
{
	rp_frame *f;
	rp_vscreen *v;
	int oldwidth, oldheight;

	PRINT_DEBUG(("screen_update (left=%d, top=%d, width=%d, height=%d)\n",
		left, top, width, height));

	oldwidth = s->width;
	oldheight = s->height;

	s->left = left;
	s->top = top;
	s->width = width;
	s->height = height;

	if (defaults.bar_sticky)
		hide_bar(s, 0);

	XMoveResizeWindow(dpy, s->help_window, s->left, s->top, s->width,
	    s->height);

	list_for_each_entry(v, &s->vscreens, node) {
		list_for_each_entry(f, &v->frames, node) {
			f->x = (f->x * width) / oldwidth;
			f->width = (f->width * width) / oldwidth;
			f->y = (f->y * height) / oldheight;
			f->height = (f->height * height) / oldheight;
			maximize_all_windows_in_frame(f);
		}
	}
}

void
screen_update_frames(rp_screen *s)
{
	rp_vscreen *v;
	rp_frame *f;

	list_for_each_entry(v, &s->vscreens, node) {
		list_for_each_entry(f, &v->frames, node) {
			if (f->x < screen_left(v->screen))
				f->x = screen_left(v->screen);

			if (f->y < screen_top(v->screen))
				f->y = screen_top(v->screen);

			if (f->x + f->width > screen_right(v->screen))
				f->width = screen_right(v->screen) - f->x;

			if (f->y + f->height > screen_bottom(v->screen))
				f->height = screen_bottom(v->screen) - f->y;

			maximize_all_windows_in_frame(f);
		}
	}
}

rp_screen *
screen_add(int rr_output)
{
	rp_screen *screen;

	screen = xmalloc(sizeof(*screen));
	list_add(&screen->node, &rp_screens);

	screen->number = numset_request(rp_glob_screen.numset);

	xrandr_fill_screen(rr_output, screen);
	init_screen(screen);
	init_frame_list(screen->current_vscreen);

	if (screen_count() == 1) {
		rp_current_screen = screen;
		change_windows_vscreen(NULL, rp_current_vscreen);
		set_window_focus(rp_current_screen->key_window);
	}
	return screen;
}

void
screen_del(rp_screen *s)
{
	rp_vscreen *v;
	struct list_head *iter, *tmp;

	if (s == rp_current_screen) {
		if (screen_count() == 1) {
			list_for_each_safe_entry(v, iter, tmp, &s->vscreens,
			    node)
				hide_vscreen_windows(v);

			rp_current_screen = NULL;
		} else {
			/*
			 * The deleted screen cannot be the current screen
			 * anymore, focus the next one.
	                 */
			screen_remove_current();
		}
	} else {
		list_for_each_safe_entry(v, iter, tmp, &s->vscreens, node)
			hide_vscreen_windows(v);
	}

	numset_release(rp_glob_screen.numset, s->number);

	list_for_each_safe_entry(v, iter, tmp, &s->vscreens, node)
		vscreen_del(v);

	screen_free(s);

	list_del(&s->node);
	free(s);
}

void
screen_free(rp_screen *s)
{
	deactivate_screen(s);

	XDestroyWindow(dpy, s->bar_window);
	XDestroyWindow(dpy, s->key_window);
	XDestroyWindow(dpy, s->input_window);
	XDestroyWindow(dpy, s->frame_window);
	XDestroyWindow(dpy, s->help_window);

	if (s->xft_font) {
		XftColorFree(dpy, DefaultVisual(dpy, s->screen_num),
		    DefaultColormap(dpy, s->screen_num), &s->xft_fg_color);
		XftColorFree(dpy, DefaultVisual(dpy, s->screen_num),
		    DefaultColormap(dpy, s->screen_num), &s->xft_bg_color);
		XftFontClose(dpy, s->xft_font);
	}

	XFreeCursor(dpy, s->rat);
	XFreeColormap(dpy, s->def_cmap);
	XFreeGC(dpy, s->normal_gc);
	XFreeGC(dpy, s->inverse_gc);

	free(s->display_string);
	free(s->xrandr.name);
}

void
screen_free_final(void)
{
	/* Relinquish our hold on the root window. */
	XSelectInput(dpy, RootWindow(dpy, DefaultScreen(dpy)), 0);

	numset_free(rp_glob_screen.numset);
}
