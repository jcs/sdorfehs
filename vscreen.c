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

void
init_vscreen(rp_vscreen *v, rp_screen *s)
{
	v->screen = s;
	v->number = numset_request(s->vscreens_numset);
	v->frames_numset = numset_new();

	init_groups(v);
}

void
vscreen_del(rp_vscreen *v)
{
	rp_screen *s = v->screen;
	rp_vscreen *target = NULL;
	rp_window *cur;

	if (v->number != 0)
		target = vscreens_find_vscreen_by_number(s, 0);

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
					    vscreens_find_vscreen_by_number(scr,
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

rp_vscreen *
vscreens_find_vscreen_by_number(rp_screen *s, int n)
{
	rp_vscreen *cur;

	list_for_each_entry(cur, &s->vscreens, node) {
		if (cur->number == n)
			return cur;
	}

	return NULL;
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
vscreen_move_window(rp_vscreen *v, rp_window *w)
{
	rp_group *cur, *oldg = NULL;
	rp_window_elem *we = NULL;
	rp_window *last;
	rp_frame *f;

	/* find the group this window is currently in */
	list_for_each_entry(cur, &w->vscr->groups, node) {
		we = group_find_window(&cur->mapped_windows, w);
		if (we) {
			oldg = cur;
			break;
		}
	}

	/* and its frame */
	f = find_windows_frame(w);

	/* move it to the new vscreen in its current group */
	group_move_window(v->current_group, w);
	w->vscr = v;
	w->sticky_frame = EMPTY;

	/* forget that this window was in the frame it was in */
	if (f)
		f->win_number = EMPTY;

	/* cycle the window's old group */
	if (oldg) {
		last = group_last_window(oldg);
		if (last)
			set_active_window_force(last);
	}

	hide_window(w);
}
