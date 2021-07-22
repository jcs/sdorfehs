/*
 * Copyright (c) 2019 joshua stein <jcs@jcs.org>
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

#include <err.h>
#include "sdorfehs.h"

rp_vscreen *vscreen_next(void);
rp_vscreen *vscreen_prev(void);

static int vscreen_access = 1;

void
init_vscreen(rp_vscreen *v, rp_screen *s)
{
	v->screen = s;
	v->number = numset_request(s->vscreens_numset);
	v->frames_numset = numset_new();
	v->numset = numset_new();
	v->last_access = 0;

	if (v->number == 0)
		v->name = xstrdup(DEFAULT_VSCREEN_NAME);
	else
		v->name = xsprintf("%d", v->number);

	INIT_LIST_HEAD(&v->unmapped_windows);
	INIT_LIST_HEAD(&v->mapped_windows);
}

void
vscreen_del(rp_vscreen *v)
{
	rp_screen *s = v->screen;
	rp_vscreen *target = NULL;
	rp_window *cur;

	if (v->number != 0)
		target = screen_find_vscreen_by_number(s, 0);

	list_for_each_entry(cur, &rp_mapped_window, node) {
		if (cur->vscr == v && target)
			vscreen_move_window(target, cur);
	}

	if (s->current_vscreen == v && target)
		set_current_vscreen(target);

	numset_release(s->vscreens_numset, v->number);

	vscreen_free(v);

	list_del(&v->node);
	free(v);
}

rp_vscreen *
vscreen_next(void)
{
	return list_next_entry(rp_current_screen->current_vscreen,
	    &rp_current_screen->vscreens, node);
}

rp_vscreen *
vscreen_prev(void)
{
	return list_prev_entry(rp_current_screen->current_vscreen,
	    &rp_current_screen->vscreens, node);
}

void
vscreen_free(rp_vscreen *v)
{
	rp_frame *frame;
	struct list_head *iter, *tmp;

	list_for_each_safe_entry(frame, iter, tmp, &v->frames, node)
		frame_free(v, frame);
}

int
vscreens_resize(int n)
{
	struct list_head *tmp, *iter;
	rp_vscreen *cur;
	rp_screen *scr;
	int x;

	PRINT_DEBUG(("Resizing vscreens from %d to %d\n", defaults.vscreens,
	    n));

	if (n < 1)
		return 1;

	if (n < defaults.vscreens) {
		list_for_each_entry(scr, &rp_screens, node) {
			list_for_each_safe_entry(cur, iter, tmp, &scr->vscreens,
			    node) {
				if (cur->number < n)
					continue;

				if (scr->current_vscreen == cur)
					set_current_vscreen(
					    screen_find_vscreen_by_number(scr,
					    0));

				vscreen_del(cur);
			}
		}
	} else if (n > defaults.vscreens) {
		list_for_each_entry(scr, &rp_screens, node) {
			for (x = defaults.vscreens; x <= n; x++) {
				cur = xmalloc(sizeof(rp_vscreen));
				init_vscreen(cur, scr);
				list_add_tail(&cur->node, &scr->vscreens);
				init_frame_list(cur);
			}
		}
	}

	defaults.vscreens = n;

	return 0;
}

/* Returns a pointer to a list of frames. */
struct list_head *
vscreen_copy_frameset(rp_vscreen *v)
{
	struct list_head *head;
	rp_frame *cur;

	/* Init our new list. */
	head = xmalloc(sizeof(struct list_head));
	INIT_LIST_HEAD(head);

	/* Copy each frame to our new list. */
	list_for_each_entry(cur, &v->frames, node) {
		list_add_tail(&(frame_copy(cur))->node, head);
	}

	return head;
}

/* Set head as the frameset, deleting the existing one. */
void
vscreen_restore_frameset(rp_vscreen *v, struct list_head *head)
{
	frameset_free(&v->frames);
	INIT_LIST_HEAD(&v->frames);

	/* Hook in our new frameset. */
	list_splice(head, &v->frames);
}

/* Given a screen, free the frames' numbers from the numset. */
void
vscreen_free_nums(rp_vscreen *v)
{
	rp_frame *cur;

	list_for_each_entry(cur, &v->frames, node) {
		numset_release(v->frames_numset, cur->number);
	}
}

/*
 * Given a list of frames, free them, but don't remove their numbers from the
 * numset.
 */
void
frameset_free(struct list_head *head)
{
	rp_frame *frame;
	struct list_head *iter, *tmp;

	list_for_each_safe_entry(frame, iter, tmp, head, node) {
		/*
		 * FIXME: what if frames has memory inside its struct that
		 * needs to be freed?
		 */
		free(frame);
	}
}

rp_frame *
vscreen_get_frame(rp_vscreen *v, int frame_num)
{
	rp_frame *cur;

	list_for_each_entry(cur, &v->frames, node) {
		if (cur->number == frame_num)
			return cur;
	}

	return NULL;
}

rp_frame *
vscreen_find_frame_by_frame(rp_vscreen *v, rp_frame *f)
{
	rp_frame *cur;

	list_for_each_entry(cur, &v->frames, node) {
		PRINT_DEBUG(("cur=%p f=%p\n", cur, f));
		if (cur == f)
			return cur;
	}

	return NULL;
}

void
set_current_vscreen(rp_vscreen *v)
{
	rp_window *win;
	rp_frame *frame;

	if (v == NULL || (v->screen == rp_current_screen &&
	    v->screen->current_vscreen == v))
		return;

	window_full_screen(NULL);

	list_for_each_entry(frame, &rp_current_vscreen->frames, node)
		frame->restore_win_number = frame->win_number;

	hide_vscreen_windows(rp_current_vscreen);

	rp_current_screen = v->screen;
	v->screen->current_vscreen = v;
	v->last_access = vscreen_access++;

	list_for_each_entry(frame, &rp_current_vscreen->frames, node) {
		if (frame->restore_win_number == EMPTY)
			continue;

		win = find_window_number(frame->restore_win_number);
		if (win && win->vscr != v) {
			warnx("restore win for frame %d on incorrect vscreen\n",
			    frame->number);
			win = NULL;
		}

		frame->restore_win_number = 0;

		if (!win)
			/* previously focused window disappeared */
			win = find_window_for_frame(frame);

		if (!win)
			continue;

		win->frame_number = frame->number;

		maximize(win);
		unhide_window(win);
	}

	set_window_focus(v->screen->key_window);

	if ((frame = current_frame(v)))
		set_active_frame(frame, 0);

	update_bar(v->screen);
	show_frame_indicator(0);

	raise_utility_windows();

	/* TODO: rp_switch_vscreen_hook */
}

void
vscreen_move_window(rp_vscreen *to, rp_window *w)
{
	rp_vscreen *from = w->vscr;
	rp_frame *f;
	rp_window_elem *we;
	struct rp_child_info *child;

	we = vscreen_find_window(&from->mapped_windows, w);
	if (we == NULL) {
		PRINT_DEBUG(("Unable to find window in mapped window lists.\n"));
		return;
	}

	f = find_windows_frame(w);
	w->vscr = to;
	w->sticky_frame = EMPTY;

	/* forget that this window was in the frame it was in */
	if (f)
		f->win_number = EMPTY;

	numset_release(from->numset, we->number);
	list_del(&we->node);

	we->number = numset_request(to->numset);
	vscreen_insert_window(&to->mapped_windows, we);

	if (to == rp_current_vscreen)
		set_active_window_force(w);
	else
		hide_window(w);

	/*
	 * Update rp_children so that any new windows from this application
	 * will appear on the vscreen we just moved to
	 */
	child = get_child_info(w->w, 0);
	if (!child)
		return;

	child->frame = current_frame(to);
	child->vscreen = to;
	child->screen = to->screen;
}

struct numset *
vscreen_get_numset(rp_vscreen *v)
{
	return v->numset;
}

/*
 * Get the vscreen list and store it in buffer delimiting each window with
 * delim.  mark_start and mark_end will be filled with the text positions for
 * the start and end of the current window.
 */
void
get_vscreen_list(rp_screen *screen, char *delim, struct sbuf *buffer,
    int *mark_start, int *mark_end)
{
	rp_vscreen *cur, *last;

	if (buffer == NULL)
		return;

	sbuf_clear(buffer);

	last = screen_last_vscreen(screen);
	list_for_each_entry(cur, &screen->vscreens, node) {
		char *fmt;
		char separator;

		if (cur == screen->current_vscreen) {
			*mark_start = strlen(sbuf_get(buffer));
			separator = '*';
		} else if (cur == last)
			separator = '+';
		else
			separator = '-';

		/*
		 * A hack, pad the vscreen with a space at the beginning and end
		 * if there is no delimiter.
		 */
		if (!delim)
			sbuf_concat(buffer, " ");

		fmt = xsprintf("%d%c%s", cur->number, separator, cur->name);
		sbuf_concat(buffer, fmt);
		free(fmt);

		/*
		 * A hack, pad the vscreen with a space at the beginning and
		 * end if there is no delimiter.
		 */
		if (!delim)
			sbuf_concat(buffer, " ");

		/*
		 * Only put the delimiter between the vscreen, and not after
		 * the the last vscreen.
		 */
		if (delim && cur->node.next != &screen->vscreens)
			sbuf_concat(buffer, delim);

		if (cur == screen->current_vscreen)
			*mark_end = strlen(sbuf_get(buffer));
	}
}

void
vscreen_rename(rp_vscreen *v, char *name)
{
	free(v->name);
	v->name = xstrdup(name);
}

rp_vscreen *
vscreen_next_vscreen(rp_vscreen *vscreen)
{
	return list_next_entry(vscreen->screen->current_vscreen,
	    &vscreen->screen->vscreens, node);
}

rp_vscreen *
vscreen_prev_vscreen(rp_vscreen *vscreen)
{
	return list_prev_entry(vscreen->screen->current_vscreen,
	    &vscreen->screen->vscreens, node);
}

rp_vscreen *
screen_last_vscreen(rp_screen *screen)
{
	int last_access = 0;
	rp_vscreen *most_recent = NULL;
	rp_vscreen *cur;

	list_for_each_entry(cur, &screen->vscreens, node) {
		if (cur != screen->current_vscreen &&
		    cur->last_access > last_access) {
			most_recent = cur;
			last_access = cur->last_access;
		}
	}
	return most_recent;
}

rp_vscreen *
screen_find_vscreen_by_number(rp_screen *s, int n)
{
	rp_vscreen *cur;

	list_for_each_entry(cur, &s->vscreens, node) {
		if (cur->number == n)
			return cur;
	}

	return NULL;
}

rp_vscreen *
screen_find_vscreen_by_name(rp_screen *s, char *name, int exact_match)
{
	rp_vscreen *cur;

	if (exact_match) {
		list_for_each_entry(cur, &s->vscreens, node) {
			if (cur->name && !strcmp(cur->name, name))
				return cur;
		}
	} else {
		list_for_each_entry(cur, &s->vscreens, node) {
			if (cur->name && str_comp(name, cur->name,
			    strlen(name)))
				return cur;
		}
	}

	return NULL;
}

rp_window_elem *
vscreen_find_window(struct list_head *list, rp_window *win)
{
	rp_window_elem *cur;

	list_for_each_entry(cur, list, node) {
		if (cur->win == win)
			return cur;
	}

	return NULL;
}

rp_window_elem *
vscreen_find_window_by_number(rp_vscreen *v, int num)
{
	rp_window_elem *cur;

	list_for_each_entry(cur, &v->mapped_windows, node) {
		if (cur->number == num)
			return cur;
	}

	return NULL;

}

/*
 * Insert a window_elem into the correct spot in the vscreen's window list to
 * preserve window number ordering.
 */
void
vscreen_insert_window(struct list_head *h, rp_window_elem *w)
{
	rp_window_elem *cur;

	list_for_each_entry(cur, h, node) {
		if (cur->number > w->number) {
			list_add_tail(&w->node, &cur->node);
			return;
		}
	}

	list_add_tail(&w->node, h);
}

static int
vscreen_in_list(struct list_head *h, rp_window_elem *w)
{
	rp_window_elem *cur;

	list_for_each_entry(cur, h, node) {
		if (cur == w)
			return 1;
	}

	return 0;
}

/*
 * If a window_elem's number has changed then the list has to be resorted.
 */
void
vscreen_resort_window(rp_vscreen *v, rp_window_elem *w)
{
	/* Only a mapped window can be resorted. */
	if (!vscreen_in_list(&v->mapped_windows, w)) {
		PRINT_DEBUG(("Attempting to resort an unmapped window!\n"));
		return;
	}
	list_del(&w->node);
	vscreen_insert_window(&v->mapped_windows, w);
}

void
vscreen_add_window(rp_vscreen *v, rp_window *w)
{
	rp_window_elem *we;

	/* Create our container structure for the window. */
	we = xmalloc(sizeof(rp_window_elem));
	we->win = w;
	we->number = -1;

	/* Finally, add it to our list. */
	list_add_tail(&we->node, &v->unmapped_windows);
}

void
vscreen_map_window(rp_vscreen *v, rp_window *win)
{
	rp_window_elem *we;

	we = vscreen_find_window(&v->unmapped_windows, win);
	if (we) {
		we->number = numset_request(v->numset);
		list_del(&we->node);
		vscreen_insert_window(&v->mapped_windows, we);
	}
}

void
vscreen_unmap_window(rp_vscreen *v, rp_window *win)
{
	rp_window_elem *we;

	we = vscreen_find_window(&v->mapped_windows, win);
	if (we) {
		numset_release(v->numset, we->number);
		list_move_tail(&we->node, &v->unmapped_windows);
	}
}

void
vscreen_del_window(rp_vscreen *v, rp_window *win)
{
	rp_window_elem *cur;
	struct list_head *iter, *tmp;

	/* The assumption is that a window is unmapped before it's deleted. */
	list_for_each_safe_entry(cur, iter, tmp, &v->unmapped_windows, node) {
		if (cur->win == win) {
			list_del(&cur->node);
			free(cur);
		}
	}

	/*
	 * Make sure the window isn't in the list of mapped windows. This would
	 * mean there is a bug.
	 */
#ifdef DEBUG
	list_for_each_entry(cur, &v->mapped_windows, node) {
		if (cur->win == win)
			PRINT_DEBUG(("This window wasn't removed from the "
			    "mapped window list.\n"));
	}
#endif
}

rp_window *
vscreen_last_window(rp_vscreen *v)
{
	rp_frame *f;
	rp_window_elem *most_recent = NULL;
	rp_window_elem *cur;
	int last_access = 0;

	f = current_frame(v);
	list_for_each_entry(cur, &v->mapped_windows, node) {
		if (cur->win->sticky_frame != EMPTY &&
		    (!f || (cur->win->sticky_frame != f->number)))
			continue;

		if (cur->win->last_access >= last_access
		    && cur->win != current_window()
		    && !find_windows_frame(cur->win)
		    && (cur->win->vscr == v || rp_have_xrandr)) {
			most_recent = cur;
			last_access = cur->win->last_access;
		}
	}

	if (most_recent)
		return most_recent->win;

	return NULL;
}

rp_window *
vscreen_next_window(rp_vscreen *v, rp_window *win)
{
	rp_window_elem *cur, *we;
	rp_frame *f;

	/* If there is no window, then get the last accessed one. */
	if (win == NULL)
		return vscreen_last_window(v);

	/*
	 * If we can't find the window, then it's in a different vscreen, so
	 * get the last accessed one in this vscreen.
	 */
	we = vscreen_find_window(&v->mapped_windows, win);
	if (we == NULL)
		return vscreen_last_window(v);

	/*
	 * The window is in this vscreen, so find the next one in the list that
	 * isn't already displayed.
	 */
	f = current_frame(v);
	for (cur = list_next_entry(we, &v->mapped_windows, node);
	    cur != we;
	    cur = list_next_entry(cur, &v->mapped_windows, node)) {
		if (cur->win->sticky_frame != EMPTY &&
		    (!f || (cur->win->sticky_frame != f->number)))
			continue;

		if (!find_windows_frame(cur->win) &&
		    (cur->win->vscr == win->vscr || rp_have_xrandr))
			return cur->win;
	}

	return NULL;
}

rp_window *
vscreen_prev_window(rp_vscreen *v, rp_window *win)
{
	rp_window_elem *cur, *we;
	rp_frame *f;

	/* If there is no window, then get the last accessed one. */
	if (win == NULL)
		return vscreen_last_window(v);

	/*
	 * If we can't find the window, then it's in a different vscreen, so
	 * get the last accessed one in this vscreen.
	 */
	we = vscreen_find_window(&v->mapped_windows, win);
	if (we == NULL)
		return vscreen_last_window(v);

	/*
	 * The window is in this vscreen, so find the previous one in the list
	 * that isn't already displayed.
	 */
	f = current_frame(v);
	for (cur = list_prev_entry(we, &v->mapped_windows, node);
	    cur != we;
	    cur = list_prev_entry(cur, &v->mapped_windows, node)) {
		if (cur->win->sticky_frame != EMPTY &&
		    (!f || (cur->win->sticky_frame != f->number)))
			continue;

		if (!find_windows_frame(cur->win) && rp_have_xrandr)
			return cur->win;
	}

	return NULL;
}

void
vscreens_merge(rp_vscreen *from, rp_vscreen *to)
{
	rp_window_elem *cur;
	struct list_head *iter, *tmp;

	/* Merging a vscreen with itself makes no sense. */
	if (from == to)
		return;

	/* Move the unmapped windows. */
	list_for_each_safe_entry(cur, iter, tmp, &from->unmapped_windows, node) {
		list_del(&cur->node);
		list_add_tail(&cur->node, &to->unmapped_windows);
	}

	/* Move the mapped windows. */
	list_for_each_safe_entry(cur, iter, tmp, &from->mapped_windows, node) {
		numset_release(from->numset, cur->number);
		list_del(&cur->node);

		cur->number = numset_request(to->numset);
		vscreen_insert_window(&to->mapped_windows, cur);
	}
}

/* Used by :cother / :iother  */
rp_window *
vscreen_last_window_by_class(rp_vscreen *v, char *class)
{
	int last_access = 0;
	rp_window_elem *most_recent = NULL;
	rp_window_elem *cur;

	list_for_each_entry(cur, &v->mapped_windows, node) {
		if (cur->win->last_access >= last_access
		    && cur->win != current_window()
		    && !find_windows_frame(cur->win)
		    && strcmp(class, cur->win->res_class)) {
			most_recent = cur;
			last_access = cur->win->last_access;
		}
	}

	if (most_recent)
		return most_recent->win;

	return NULL;
}

/* Used by :cother / :iother  */
rp_window *
vscreen_last_window_by_class_complement(rp_vscreen *v, char *class)
{
	int last_access = 0;
	rp_window_elem *most_recent = NULL;
	rp_window_elem *cur;

	list_for_each_entry(cur, &v->mapped_windows, node) {
		if (cur->win->last_access >= last_access
		    && cur->win != current_window()
		    && !find_windows_frame(cur->win)
		    && !strcmp(class, cur->win->res_class)) {
			most_recent = cur;
			last_access = cur->win->last_access;
		}
	}

	if (most_recent)
		return most_recent->win;

	return NULL;
}
