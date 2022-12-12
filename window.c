/*
 * functions for handling the window list
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

#include "sdorfehs.h"

LIST_HEAD(rp_unmapped_window);
LIST_HEAD(rp_mapped_window);

struct numset *rp_window_numset;

static void set_active_window_body(rp_window * win, int force);

/* Get the mouse position relative to the the specified window */
static void
get_mouse_position(rp_window *win, int *mouse_x, int *mouse_y)
{
	Window root_win, child_win;
	int root_x, root_y;
	unsigned int mask;

	XQueryPointer(dpy, win->vscreen->screen->root, &root_win, &child_win,
	    mouse_x, mouse_y, &root_x, &root_y, &mask);
}

void
free_window(rp_window *w)
{
	if (w == NULL)
		return;

	free(w->user_name);
	free(w->res_name);
	free(w->res_class);
	free(w->wm_name);

	XFree(w->hints);

	free(w);
}

void
update_window_gravity(rp_window *win)
{
	if (win->transient)
		win->gravity = defaults.trans_gravity;
	else if (win->hints->flags & PMaxSize || win->hints->flags & PAspect)
		win->gravity = defaults.maxsize_gravity;
	else
		win->gravity = defaults.win_gravity;
}

char *
window_name(rp_window *win)
{
	if (win == NULL)
		return NULL;

	if (win->named)
		return win->user_name;

	switch (defaults.win_name) {
	case WIN_NAME_RES_NAME:
		if (win->res_name)
			return win->res_name;
		else
			return win->user_name;

	case WIN_NAME_RES_CLASS:
		if (win->res_class)
			return win->res_class;
		else
			return win->user_name;

		/*
		 * if we're not looking for the res name or res class, then
		 * we're looking for the window title.
		 */
	default:
		if (win->wm_name)
			return win->wm_name;
		else
			return win->user_name;
	}

	return NULL;
}

/*
 * FIXME: we need to verify that the window is running on the same host as
 * something.  otherwise there could be overlapping PIDs.
 */
struct rp_child_info *
get_child_info(Window w, int add)
{
	rp_child_info *cur;
	unsigned long pid = 0;

	if (!get_atom(w, _net_wm_pid, XA_CARDINAL, 0, &pid, 1, NULL)) {
		PRINT_DEBUG(("Couldn't get _NET_WM_PID Property\n"));
		long nresults;
		XResClientIdValue *results = NULL;
		XResClientIdSpec specs;
		specs.client = w;
		specs.mask = XRES_CLIENT_ID_PID_MASK;
		if (XResQueryClientIds(dpy, 1, &specs, &nresults, &results) != Success)
			pid = 0;
		else {
			int i;
			for (i = 0; i < nresults; i++) {
				if (results[i].spec.mask == XRES_CLIENT_ID_PID_MASK) {
					pid = *(unsigned long *)(results[i].value);
					break;
				}
			}
			XFree(results);
		}
	}

	PRINT_DEBUG(("NET_WM_PID: %ld\n", pid));

	if (pid) {
		list_for_each_entry(cur, &rp_children, node)
			if (pid == cur->pid)
				return cur;
	}

	if (!add)
		return NULL;

	/*
	 * A new process is creating windows that we didn't directly spawn
	 * (otherwise it would be in rp_children via spawn())
	 */
	cur = xmalloc(sizeof(rp_child_info));
	cur->cmd = NULL;
	cur->pid = pid;
	cur->terminated = 0;
	cur->frame = current_frame(rp_current_vscreen);
	cur->vscreen = rp_current_vscreen;
	cur->screen = rp_current_screen;
	cur->window_mapped = 0;

	list_add(&cur->node, &rp_children);

	return cur;
}

/* Allocate a new window and add it to the list of managed windows */
rp_window *
add_to_window_list(rp_screen *s, Window w)
{
	struct rp_child_info *child_info;
	rp_window *new_window;
	rp_vscreen *vscreen = NULL;
	int frame_num = -1;

	new_window = xmalloc(sizeof(rp_window));

	new_window->w = w;
	new_window->vscreen = s->current_vscreen;
	new_window->last_access = 0;
	new_window->state = WithdrawnState;
	new_window->number = -1;
	new_window->sticky_frame = EMPTY;
	new_window->frame_number = EMPTY;
	new_window->intended_frame_number = -1;
	new_window->named = 0;
	new_window->hints = XAllocSizeHints();
	new_window->colormap = DefaultColormap(dpy, s->screen_num);
	new_window->transient = XGetTransientForHint(dpy, new_window->w,
	    &new_window->transient_for);
	PRINT_DEBUG(("transient %d\n", new_window->transient));
	new_window->full_screen = 0;

	update_window_gravity(new_window);

	get_mouse_position(new_window, &new_window->mouse_x,
	    &new_window->mouse_y);

	XSelectInput(dpy, new_window->w, WIN_EVENTS);

	new_window->user_name = xstrdup("Unnamed");

	new_window->wm_name = NULL;
	new_window->res_name = NULL;
	new_window->res_class = NULL;

	/* Add the window to the end of the unmapped list. */
	list_add_tail(&new_window->node, &rp_unmapped_window);

	child_info = get_child_info(w, 1);
	if (child_info) {
		if (child_info->vscreen != new_window->vscreen)
			new_window->vscreen = child_info->vscreen;

		if (!child_info->window_mapped) {
			rp_frame *frame = vscreen_find_frame_by_frame(
			    child_info->vscreen, child_info->frame);

			PRINT_DEBUG(("frame=%p\n", frame));
			vscreen = child_info->vscreen;
			if (frame)
				frame_num = frame->number;
			/* Only map the first window in the launch frame. */
			child_info->window_mapped = 1;
		}
	}

	/*
	 * Add the window to the vscreen its pid was launched in or the current
	 * one.
	 */
	if (vscreen)
		vscreen_add_window(vscreen, new_window);
	else
		vscreen_add_window(new_window->vscreen, new_window);

	PRINT_DEBUG(("frame_num: %d\n", frame_num));
	if (frame_num >= 0)
		new_window->intended_frame_number = frame_num;

	return new_window;
}

/* Check to see if the window is in the list of windows. */
rp_window *
find_window_in_list(Window w, struct list_head *list)
{
	rp_window *cur;

	list_for_each_entry(cur, list, node) {
		if (cur->w == w)
			return cur;
	}

	return NULL;
}

/* Check to see if the window is in any of the lists of windows. */
rp_window *
find_window(Window w)
{
	rp_window *win = NULL;

	win = find_window_in_list(w, &rp_mapped_window);

	if (!win) {
		win = find_window_in_list(w, &rp_unmapped_window);
		if (win)
			PRINT_DEBUG(("Window found in unmapped window list\n"));
		else
			PRINT_DEBUG(("Window not found.\n"));
	} else {
		PRINT_DEBUG(("Window found in mapped window list.\n"));
	}

	return win;
}

rp_window *
find_window_number(int n)
{
	rp_window *cur;

	list_for_each_entry(cur, &rp_mapped_window, node) {
		/* if (cur->state == STATE_UNMAPPED) continue; */

		if (n == cur->number)
			return cur;
	}

	return NULL;
}

rp_window *
find_window_name(char *name, int exact_match)
{
	rp_window_elem *cur;

	if (!exact_match) {
		list_for_each_entry(cur, &rp_current_vscreen->mapped_windows,
		    node) {
			if (str_comp(name, window_name(cur->win), strlen(name)))
				return cur->win;
		}
	} else {
		list_for_each_entry(cur, &rp_current_vscreen->mapped_windows,
		    node) {
			if (!strcmp(name, window_name(cur->win)))
				return cur->win;
		}
	}

	/* didn't find it */
	return NULL;
}

/* TODO: remove this */
rp_window *
find_window_other(rp_vscreen *vscreen)
{
	return vscreen_last_window(vscreen);
}

/*
 * Assumes the list is sorted by increasing number. Inserts win into to Right
 * place to keep the list sorted.
 */
void
insert_into_list(rp_window *win, struct list_head *list)
{
	rp_window *cur;

	list_for_each_entry(cur, list, node) {
		if (cur->number > win->number) {
			list_add_tail(&win->node, &cur->node);
			return;
		}
	}

	list_add_tail(&win->node, list);
}

static void
save_mouse_position(rp_window *win)
{
	Window root_win, child_win;
	int root_x, root_y;
	unsigned int mask;

	/*
	 * In the case the XQueryPointer raises a BadWindow error, the window
	 * is not mapped or has been destroyed so it doesn't matter what we
	 * store in mouse_x and mouse_y since they will never be used again.
	 */

	ignore_badwindow++;

	XQueryPointer(dpy, win->w, &root_win, &child_win,
	    &root_x, &root_y, &win->mouse_x, &win->mouse_y, &mask);

	ignore_badwindow--;
}

/* Takes focus away from last_win and gives focus to win */
void
give_window_focus(rp_window *win, rp_window *last_win)
{
	/*
	 * counter increments every time this function is called. This way we
	 * can track which window was last accessed.
	 */
	static int counter = 1;

	/*
	 * Warp the cursor to the window's saved position if last_win and win
	 * are different windows.
	 */
	if (last_win != NULL && win != last_win) {
		if (last_win->full_screen)
			window_full_screen(NULL);
		save_mouse_position(last_win);
		XSetWindowBorder(dpy, last_win->w, rp_glob_screen.bw_color);
	}
	if (win == NULL)
		return;

	counter++;
	win->last_access = counter;
	unhide_window(win);

	if (defaults.warp) {
		PRINT_DEBUG(("Warp pointer\n"));
		XWarpPointer(dpy, None, win->w,
		    0, 0, 0, 0, win->mouse_x, win->mouse_y);
	}
	/* Swap colormaps */
	if (last_win != NULL)
		XUninstallColormap(dpy, last_win->colormap);
	XInstallColormap(dpy, win->colormap);

	XSetWindowBorder(dpy, win->w, rp_glob_screen.fw_color);

	/* Finally, give the window focus */
	rp_current_screen = win->vscreen->screen;
	rp_current_screen->current_vscreen = win->vscreen;
	set_rp_window_focus(win);

	raise_utility_windows();

	XSync(dpy, False);
}

/* In the current frame, set the active window to win. win will have focus. */
void
set_active_window(rp_window *win)
{
	set_active_window_body(win, 0);
}

void
set_active_window_force(rp_window *win)
{
	set_active_window_body(win, 1);
}

static rp_frame *
find_frame_non_dedicated(rp_vscreen *current_vscreen)
{
	rp_frame *cur;
	rp_screen *screen;

	list_for_each_entry(screen, &rp_screens, node) {
		if (current_vscreen == screen->current_vscreen)
			continue;

		list_for_each_entry(cur, &screen->current_vscreen->frames,
		    node) {
			if (!cur->dedicated)
				return cur;
		}
	}

	return NULL;
}

static void
set_active_window_body(rp_window *win, int force)
{
	rp_window *last_win;
	rp_frame *frame = NULL, *last_frame = NULL;

	if (win == NULL)
		return;

	PRINT_DEBUG(("intended_frame_number: %d\n",
	    win->intended_frame_number));

	/* use the intended frame if we can. */
	if (win->intended_frame_number >= 0) {
		frame = vscreen_get_frame(win->vscreen,
		    win->intended_frame_number);
		win->intended_frame_number = -1;
		if (frame != current_frame(win->vscreen))
			last_frame = current_frame(win->vscreen);
	}
	if (frame == NULL)
		frame = vscreen_get_frame(win->vscreen,
		    win->vscreen->current_frame);

	if (frame->dedicated && !force) {
		/* Try to find a non-dedicated frame. */
		rp_frame *non_dedicated;

		non_dedicated = find_frame_non_dedicated(win->vscreen);
		if (non_dedicated != NULL) {
			last_frame = frame;
			frame = non_dedicated;
			if (win->vscreen == rp_current_vscreen)
				set_active_frame(frame, 0);
		}
	}
	last_win = set_frames_window(frame, win);

	if (last_win != NULL)
		PRINT_DEBUG(("last window: %s\n", window_name(last_win)));
	PRINT_DEBUG(("new window: %s\n", window_name(win)));

	/* Make sure the window comes up full screen */
	maximize(win);

	if (win->vscreen == rp_current_vscreen)
		/* Focus the window. */
		give_window_focus(win, last_win);

	/*
	 * The other windows in the frame will be hidden if this window doesn't
	 * qualify as a transient window (ie dialog box.
	 */
	if (!window_is_transient(win))
		hide_others(win);

	if (win->vscreen == rp_current_vscreen)
		/* Make sure the program bar is always on the top */
		update_window_names(win->vscreen->screen, defaults.window_fmt);

	XSync(dpy, False);

	/* If we switched frame, go back to the old one. */
	if (win->vscreen == rp_current_vscreen) {
		if (last_frame != NULL)
			set_active_frame(last_frame, 0);

		/* Call the switch window hook */
		hook_run(&rp_switch_win_hook);
	}
}

/*
 * Go to the window, switching frames if the window is already in a frame.
 */
void
goto_window(rp_window *win)
{
	rp_frame *frame;

	/* There is nothing to do if it is already the current window. */
	if (current_window() == win)
		return;

	frame = find_windows_frame(win);
	if (frame) {
		set_active_frame(frame, 0);
	} else {
		set_active_window(win);
	}
}

void
get_current_window_in_fmt(char *fmt, struct sbuf *buffer)
{
	rp_window_elem *we;

	if (buffer == NULL)
		return;

	sbuf_clear(buffer);
	find_window_other(rp_current_vscreen);

	list_for_each_entry(we, &rp_current_vscreen->mapped_windows, node) {
		if (we->win != current_window())
			continue;

		format_string(fmt, we, buffer);
	}
}

/*
 * get the window list and store it in buffer delimiting each window with
 * delim.  mark_start and mark_end will be filled with the text positions for
 * the start and end of the current window.
 */
void
get_window_list(char *fmt, char *delim, struct sbuf *buffer,
    int *mark_start, int *mark_end)
{
	rp_window_elem *we;

	if (buffer == NULL)
		return;

	sbuf_clear(buffer);
	find_window_other(rp_current_vscreen);

	/* We only loop through the current vscreen to look for windows. */
	list_for_each_entry(we, &rp_current_vscreen->mapped_windows, node) {
		PRINT_DEBUG(("%d-%s\n", we->number, window_name(we->win)));

		if (we->win == current_window())
			*mark_start = strlen(sbuf_get(buffer));

		/*
		 * A hack, pad the window with a space at the beginning and end
		 * if there is no delimiter.
		 */
		if (!delim)
			sbuf_concat(buffer, " ");

		format_string(fmt, we, buffer);

		/*
		 * A hack, pad the window with a space at the beginning and end
		 * if there is no delimiter.
		 */
		if (!delim)
			sbuf_concat(buffer, " ");

		/*
		 * Only put the delimiter between the windows, and not after
		 * the the last window.
		 */
		if (delim && we->node.next != &rp_current_vscreen->mapped_windows)
			sbuf_concat(buffer, delim);

		if (we->win == current_window()) {
			*mark_end = strlen(sbuf_get(buffer));
		}
	}

	if (!strcmp(sbuf_get(buffer), "")) {
		sbuf_copy(buffer, MESSAGE_NO_MANAGED_WINDOWS);
	}
}

void
init_window_stuff(void)
{
	rp_window_numset = numset_new();
}

void
free_window_stuff(void)
{
	rp_window *cur;
	struct list_head *tmp, *iter;

	list_for_each_safe_entry(cur, iter, tmp, &rp_unmapped_window, node) {
		list_del(&cur->node);
		vscreen_del_window(cur->vscreen, cur);
		free_window(cur);
	}

	list_for_each_safe_entry(cur, iter, tmp, &rp_mapped_window, node) {
		list_del(&cur->node);
		vscreen_unmap_window(cur->vscreen, cur);
		vscreen_del_window(cur->vscreen, cur);
		free_window(cur);
	}

	numset_free(rp_window_numset);
}

rp_frame *
win_get_frame(rp_window *win)
{
	if (win->frame_number != EMPTY)
		return vscreen_get_frame(win->vscreen, win->frame_number);

	return NULL;
}

void
change_windows_vscreen(rp_vscreen *old_vscreen, rp_vscreen *new_vscreen)
{
	rp_window *win;

	list_for_each_entry(win, &rp_mapped_window, node) {
		if (win->vscreen == old_vscreen)
			win->vscreen = new_vscreen;
	}
}

void
window_full_screen(rp_window *win)
{
	rp_window *oldfs;

	if ((oldfs = rp_current_screen->full_screen_win)) {
		if (win == oldfs)
			return;

		PRINT_DEBUG(("making window 0x%lx no longer "
		    "full-screen\n", oldfs->w));

		oldfs->full_screen = 0;
		remove_atom(oldfs->w, _net_wm_state, XA_ATOM,
		    _net_wm_state_fullscreen);
		maximize(oldfs);
	}

	if (!win) {
		rp_current_screen->full_screen_win = NULL;
		hide_bar(rp_current_screen, 0);
		return;
	}

	PRINT_DEBUG(("making window 0x%lx full-screen\n", win->w));
	hide_bar(rp_current_screen, 1);

	rp_current_screen->full_screen_win = win;
	win->full_screen = 1;
	set_atom(win->w, _net_wm_state, XA_ATOM,
	    &_net_wm_state_fullscreen, 1);
	maximize(win);
}
