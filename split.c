/*
 * Functions for handling window splitting and tiling.
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

#include <unistd.h>
#include <err.h>
#include <string.h>

#include "sdorfehs.h"

#define VERTICALLY 0
#define HORIZONTALLY 1

static void
update_last_access(rp_frame *frame)
{
	static int counter = 0;

	frame->last_access = counter;
	counter++;
}

rp_frame *
current_frame(rp_vscreen *v)
{
	return vscreen_get_frame(v, v->current_frame);
}

void
cleanup_frame(rp_frame *frame)
{
	rp_window *win;
	rp_vscreen *vscreen = frame->vscreen;

	win = find_window_other(vscreen);
	if (win == NULL) {
		set_frames_window(frame, NULL);
		return;
	}
	set_frames_window(frame, win);

	maximize(win);
	unhide_window(win);

	if (!window_is_transient(win))
		hide_others(win);
}

rp_window *
set_frames_window(rp_frame *frame, rp_window *win)
{
	int last_win;

	last_win = frame->win_number;
	if (win) {
		frame->win_number = win->number;
		win->frame_number = frame->number;

		/*
		 * We need to make sure that win and frame are on the same
		 * screen, since with Xrandr, windows can move from one screen
		 * to another.
		 */
		win->vscreen = frame->vscreen;
	} else {
		frame->win_number = EMPTY;
	}

	return find_window_number(last_win);
}

void
maximize_all_windows_in_frame(rp_frame *frame)
{
	rp_window *win;

	list_for_each_entry(win, &rp_mapped_window, node) {
		if (win->frame_number == frame->number) {
			maximize(win);
		}
	}
}

/* Make the frame occupy the entire screen */
void
maximize_frame(rp_frame *frame)
{
	rp_vscreen *v = frame->vscreen;

	frame->x = screen_left(v->screen);
	frame->y = screen_top(v->screen);

	frame->width = screen_width(v->screen);
	frame->height = screen_height(v->screen);
}

/* Create a full screen frame */
static void
create_initial_frame(rp_vscreen *vscreen)
{
	rp_frame *frame;

	frame = frame_new(vscreen);
	vscreen->current_frame = frame->number;
	list_add_tail(&frame->node, &vscreen->frames);

	update_last_access(frame);

	maximize_frame(frame);
	set_frames_window(frame, NULL);
}

void
init_frame_list(rp_vscreen *vscreen)
{
	INIT_LIST_HEAD(&vscreen->frames);

	create_initial_frame(vscreen);
}

rp_frame *
find_last_frame(rp_vscreen *v)
{
	rp_frame *cur_frame, *last = NULL;
	int last_access = -1;

	list_for_each_entry(cur_frame, &v->frames, node) {
		if (cur_frame->number != v->current_frame &&
		    cur_frame->last_access > last_access) {
			last_access = cur_frame->last_access;
			last = cur_frame;
		}
	}

	return last;
}

/* Return the frame that contains the window. */
rp_frame *
find_windows_frame(rp_window *win)
{
	rp_vscreen *v;
	rp_frame *cur;

	v = win->vscreen;

	list_for_each_entry(cur, &v->frames, node) {
		if (cur->win_number == win->number)
			return cur;
	}

	return NULL;
}

int
num_frames(rp_vscreen *v)
{
	return list_size(&v->frames);
}

rp_frame *
find_frame_next(rp_frame *frame)
{
	if (frame == NULL)
		return NULL;
	return list_next_entry(frame, &frame->vscreen->frames, node);
}

rp_frame *
find_frame_prev(rp_frame *frame)
{
	if (frame == NULL)
		return NULL;
	return list_prev_entry(frame, &frame->vscreen->frames, node);
}

rp_window *
current_window(void)
{
	return find_window_number(current_frame(rp_current_vscreen)->win_number);
}

static int
window_fits_in_frame(rp_window *win, rp_frame *frame)
{
	/*
	 * If the window has minimum size hints, make sure they are smaller
	 * than the frame.
	 */
	if (win->hints->flags & PMinSize) {
		if (win->hints->min_width > frame->width
		    ||
		    win->hints->min_height > frame->height) {
			return 0;
		}
	}
	return 1;
}

/*
 * Search the list of mapped windows for a window that will fit in the
 * specified frame.
 */
rp_window *
find_window_for_frame(rp_frame *frame)
{
	rp_vscreen *v = frame->vscreen;
	int last_access = 0;
	rp_window_elem *most_recent = NULL;
	rp_window_elem *cur;

	list_for_each_entry(cur, &v->mapped_windows, node) {
		if (cur->win != current_window()
		    && cur->win->sticky_frame != frame->number
		    && !find_windows_frame(cur->win)
		    && cur->win->last_access >= last_access
		    && window_fits_in_frame(cur->win, frame)
		    && cur->win->frame_number == EMPTY) {
			most_recent = cur;
			last_access = cur->win->last_access;
		}
	}

	if (most_recent)
		return most_recent->win;

	return NULL;
}

/*
 * Splits the frame in 2. if way is 0 then split vertically otherwise split it
 * horizontally.
 */
static void
split_frame(rp_frame *frame, int way, int pixels)
{
	rp_vscreen *v;
	rp_window *win;
	rp_frame *new_frame;

	v = frame->vscreen;

	/* Make our new frame. */
	new_frame = frame_new(v);

	/* Add the frame to the frameset. */
	list_add(&new_frame->node, &current_frame(rp_current_vscreen)->node);

	set_frames_window(new_frame, NULL);

	if (way == HORIZONTALLY) {
		new_frame->x = frame->x;
		new_frame->y = frame->y + pixels;
		new_frame->width = frame->width;
		new_frame->height = frame->height - pixels;

		frame->height = pixels;
	} else {
		new_frame->x = frame->x + pixels;
		new_frame->y = frame->y;
		new_frame->width = frame->width - pixels;
		new_frame->height = frame->height;

		frame->width = pixels;
	}

	win = find_window_for_frame(new_frame);
	if (win) {
		PRINT_DEBUG(("Found a window for the frame!\n"));

		set_frames_window(new_frame, win);

		maximize(win);
		unhide_window(win);
	} else {
		PRINT_DEBUG(("No window fits the frame.\n"));

		set_frames_window(new_frame, NULL);
	}

	/* resize the existing frame */
	if (frame->win_number != EMPTY) {
		maximize_all_windows_in_frame(frame);
		XRaiseWindow(dpy, find_window_number(frame->win_number)->w);
	}
	update_bar(v->screen);
	show_frame_indicator(0);
}

/*
 * Splits the window vertically leaving the original with 'pixels' pixels .
 */
void
v_split_frame(rp_frame *frame, int pixels)
{
	split_frame(frame, VERTICALLY, pixels);
}

/*
 * Splits the frame horizontally leaving the original with 'pixels' pixels .
 */
void
h_split_frame(rp_frame *frame, int pixels)
{
	split_frame(frame, HORIZONTALLY, pixels);
}

void
remove_all_splits(void)
{
	struct list_head *tmp, *iter;
	rp_vscreen *v = rp_current_vscreen;
	rp_frame *frame;
	rp_window *win;

	/* Hide all the windows not in the current frame. */
	list_for_each_entry(win, &rp_mapped_window, node) {
		if (win->frame_number != v->current_frame && win->vscreen == v)
			hide_window(win);

		if (win->sticky_frame != EMPTY &&
		    win->sticky_frame != v->current_frame && win->vscreen == v)
			win->sticky_frame = EMPTY;
	}

	/* Delete all the frames except the current one. */
	list_for_each_safe_entry(frame, iter, tmp, &v->frames, node) {
		if (frame->number != v->current_frame) {
			list_del(&frame->node);
			frame_free(v, frame);
		}
	}

	/* Maximize the frame and the windows in the frame. */
	maximize_frame(current_frame(rp_current_vscreen));
	maximize_all_windows_in_frame(current_frame(rp_current_vscreen));
}

/* Shrink the size of the frame to fit it's current window. */
void
resize_shrink_to_window(rp_frame *frame)
{
	rp_window *win;

	if (frame->win_number == EMPTY)
		return;

	win = find_window_number(frame->win_number);

	resize_frame_horizontally(frame,
	    win->width + (win->border * 2) + (defaults.gap * 2) - frame->width);
	resize_frame_vertically(frame,
	    win->height + (win->border * 2) + (defaults.gap * 2) -
	        frame->height);
}

/*
 * resize_frame is a generic frame resizer that can resize vertically,
 * horizontally, to the right, to the left, etc. It all depends on the
 * functions passed to it. Returns -1 if the resize failed, 0 for success.
 */
static int
resize_frame(rp_frame *frame, rp_frame *pusher, int diff,
    int (*c1)(rp_frame *), int (c2)(rp_frame *),
    int (*c3)(rp_frame *), int (c4)(rp_frame *),
    void (*resize1)(rp_frame *, int),
    void (*resize2)(rp_frame *, int),
    int (*resize3)(rp_frame *, rp_frame *, int))
{
	rp_vscreen *v = frame->vscreen;
	rp_frame *cur;

	/*
	 * Loop through the frames and determine which ones are affected by
	 * resizing frame.
	 */
	list_for_each_entry(cur, &v->frames, node) {
		if (cur == frame || cur == pusher)
			continue;
		/*
		 * If cur is touching frame along the axis that is being moved
		 * then this frame is affected by the resize.
		 */
		if ((*c1)(cur) == (*c3)(frame)) {
			/* If the frame can't get any smaller, then fail. */
			if (diff > 0 && abs((*c3)(cur) - (*c1)(cur)) - diff <=
			    (defaults.window_border_width * 2) +
			    (defaults.gap * 2))
				return -1;

			/*
			 * Test for this circumstance:
			 * --+ | |+-+ |f||c| | |+-+ --+
			 *
			 * In this case, resizing cur will not affect any other
			 * frames, so just do the resize.
			 */
			if (((*c2)(cur) >= (*c2)(frame))
			    && (*c4)(cur) <= (*c4)(frame)) {
				(*resize2)(cur, -diff);
				maximize_all_windows_in_frame(cur);
			}
			/*
			 * Otherwise, cur's corners are either strictly outside
			 * frame's corners, or one of them is inside and the
			 * other isn't. In either of these cases, resizing cur
			 * will affect other adjacent frames, so find them and
			 * resize them first (recursive step) and then resize
			 * cur.
			 */
			else if (((*c2)(cur) < (*c2)(frame)
					&& (*c4)(cur) > (*c4)(frame))
				    || ((*c2)(cur) >= (*c2)(frame)
					&& (*c2)(cur) < (*c4)(frame))
				    || ((*c4)(cur) > (*c2)(frame)
				&& (*c4)(cur) <= (*c4)(frame))) {
				/* Attempt to resize cur. */
				if (resize3(cur, frame, -diff) == -1)
					return -1;
			}
		}
	}

	/* Finally, resize the frame and the windows inside. */
	(*resize1)(frame, diff);
	maximize_all_windows_in_frame(frame);

	return 0;
}

static int resize_frame_bottom(rp_frame *frame, rp_frame *pusher, int diff);
static int resize_frame_top(rp_frame *frame, rp_frame *pusher, int diff);
static int resize_frame_left(rp_frame *frame, rp_frame *pusher, int diff);
static int resize_frame_right(rp_frame *frame, rp_frame *pusher, int diff);

/* Resize frame by moving it's right side. */
static int
resize_frame_right(rp_frame *frame, rp_frame *pusher, int diff)
{
	return resize_frame(frame, pusher, diff,
	    frame_left, frame_top, frame_right, frame_bottom,
	    frame_resize_right, frame_resize_left, resize_frame_left);
}

/* Resize frame by moving it's left side. */
static int
resize_frame_left(rp_frame *frame, rp_frame *pusher, int diff)
{
	return resize_frame(frame, pusher, diff,
	    frame_right, frame_top, frame_left, frame_bottom,
	    frame_resize_left, frame_resize_right, resize_frame_right);
}

/* Resize frame by moving it's top side. */
static int
resize_frame_top(rp_frame *frame, rp_frame *pusher, int diff)
{
	return resize_frame(frame, pusher, diff,
	    frame_bottom, frame_left, frame_top, frame_right,
	    frame_resize_up, frame_resize_down, resize_frame_bottom);
}

/* Resize frame by moving it's bottom side. */
static int
resize_frame_bottom(rp_frame *frame, rp_frame *pusher, int diff)
{
	return resize_frame(frame, pusher, diff,
	    frame_top, frame_left, frame_bottom, frame_right,
	    frame_resize_down, frame_resize_up, resize_frame_top);
}

/*
 * Resize frame diff pixels by expanding it to the right. If the frame is
 * against the right side of the screen, expand it to the left.
 */
void
resize_frame_horizontally(rp_frame *frame, int diff)
{
	int (*resize_fn)(rp_frame *, rp_frame *, int);
	struct list_head *l;
	rp_vscreen *v = frame->vscreen;

	if (num_frames(v) < 2 || diff == 0)
		return;

	if (frame_width(frame) + diff <= (defaults.window_border_width * 2) +
	    (defaults.gap * 2))
		return;

	/* Find out which resize function to use. */
	if (frame_right(frame) < screen_right(v->screen)) {
		resize_fn = resize_frame_right;
	} else if (frame_left(frame) > screen_left(v->screen)) {
		resize_fn = resize_frame_left;
	} else {
		return;
	}

	/*
	 * Copy the frameset. If the resize fails, then we restore the original
	 * one.
	 */
	l = vscreen_copy_frameset(v);

	if ((*resize_fn)(frame, NULL, diff) == -1) {
		vscreen_restore_frameset(v, l);
	} else {
		frameset_free(l);
	}

	/* It's our responsibility to free this. */
	free(l);
}

/*
 * Resize frame diff pixels by expanding it down. If the frame is against the
 * bottom of the screen, expand it up.
 */
void
resize_frame_vertically(rp_frame *frame, int diff)
{
	int (*resize_fn)(rp_frame *, rp_frame *, int);
	struct list_head *l;
	rp_vscreen *v = frame->vscreen;

	if (num_frames(v) < 2 || diff == 0)
		return;

	if (frame_height(frame) + diff <= (defaults.window_border_width * 2) +
	    (defaults.gap * 2))
		return;

	/* Find out which resize function to use. */
	if (frame_bottom(frame) < screen_bottom(v->screen)) {
		resize_fn = resize_frame_bottom;
	} else if (frame_top(frame) > screen_top(v->screen)) {
		resize_fn = resize_frame_top;
	} else {
		return;
	}

	/*
	 * Copy the frameset. If the resize fails, then we restore the original
	 * one.
	 */
	l = vscreen_copy_frameset(v);

	if ((*resize_fn)(frame, NULL, diff) == -1) {
		vscreen_restore_frameset(v, l);
	} else {
		frameset_free(l);
	}

	/* It's our responsibility to free this. */
	free(l);
}

static int
frame_is_below(rp_frame *src, rp_frame *frame)
{
	if (frame->y > src->y)
		return 1;
	return 0;
}

static int
frame_is_above(rp_frame *src, rp_frame *frame)
{
	if (frame->y < src->y)
		return 1;
	return 0;
}

static int
frame_is_left(rp_frame *src, rp_frame *frame)
{
	if (frame->x < src->x)
		return 1;
	return 0;
}

static int
frame_is_right(rp_frame *src, rp_frame *frame)
{
	if (frame->x > src->x)
		return 1;
	return 0;
}

static int
total_frame_area(rp_vscreen *v)
{
	int area = 0;
	rp_frame *cur;

	list_for_each_entry(cur, &v->frames, node) {
		area += cur->width * cur->height;
	}

	return area;
}

/* Return 1 if frames f1 and f2 overlap */
static int
frames_overlap(rp_frame *f1, rp_frame *f2)
{
	if (f1->x >= f2->x + f2->width
	    || f1->y >= f2->y + f2->height
	    || f2->x >= f1->x + f1->width
	    || f2->y >= f1->y + f1->height) {
		return 0;
	}
	return 1;
}

/* Return 1 if w's frame overlaps any other window's frame */
static int
frame_overlaps(rp_frame *frame)
{
	rp_vscreen *v;
	rp_frame *cur;

	v = frame->vscreen;

	list_for_each_entry(cur, &v->frames, node) {
		if (cur != frame && frames_overlap(cur, frame)) {
			return 1;
		}
	}
	return 0;
}

void
remove_frame(rp_frame *frame)
{
	rp_vscreen *v;
	int area;
	rp_frame *cur;
	rp_window *win;

	if (frame == NULL)
		return;

	v = frame->vscreen;

	area = total_frame_area(v);
	PRINT_DEBUG(("Total Area: %d\n", area));

	list_del(&frame->node);
	win = find_window_number(frame->win_number);
	hide_window(win);
	hide_others(win);

	list_for_each_entry(win, &rp_mapped_window, node) {
		if (win->sticky_frame == frame->number && win->vscreen == v)
			win->sticky_frame = EMPTY;
	}

	list_for_each_entry(cur, &v->frames, node) {
		rp_frame tmp_frame;
		int fits = 0;

		/* Backup the frame */
		memcpy(&tmp_frame, cur, sizeof(rp_frame));

		if (frame_is_below(frame, cur)
		    || frame_is_above(frame, cur)) {
			if (frame_is_below(frame, cur))
				cur->y = frame->y;
			cur->height += frame->height;
		}
		PRINT_DEBUG(("Attempting vertical Frame y=%d height=%d\n",
		    cur->y, cur->height));
		PRINT_DEBUG(("New Total Area: %d\n", total_frame_area(v)));

		/*
		 * If the area is bigger than before, the frame takes up too
		 * much space.  If the current frame and the deleted frame
		 * DON'T overlap then the current window took up just the right
		 * amount of space but didn't take up the space left behind by
		 * the deleted window. If any active frames overlap, it could
		 * have taken up the right amount of space, overlaps with the
		 * deleted frame but obviously didn't fit.
		 */
		if (total_frame_area(v) > area || !frames_overlap(cur, frame) ||
		    frame_overlaps(cur)) {
			PRINT_DEBUG(("Didn't fit vertically\n"));

			/* Restore the current window's frame */
			memcpy(cur, &tmp_frame, sizeof(rp_frame));
		} else {
			PRINT_DEBUG(("It fit vertically!!\n"));

			/* update the frame backup */
			memcpy(&tmp_frame, cur, sizeof(rp_frame));
			fits = 1;
		}

		if (frame_is_left(frame, cur)
		    || frame_is_right(frame, cur)) {
			if (frame_is_right(frame, cur))
				cur->x = frame->x;
			cur->width += frame->width;
		}
		PRINT_DEBUG(("Attempting horizontal Frame x=%d width=%d\n",
		    cur->x, cur->width));
		PRINT_DEBUG(("New Total Area: %d\n", total_frame_area(v)));

		/* Same test as the vertical test, above. */
		if (total_frame_area(v) > area || !frames_overlap(cur, frame) ||
		    frame_overlaps(cur)) {
			PRINT_DEBUG(("Didn't fit horizontally\n"));

			/* Restore the current window's frame */
			memcpy(cur, &tmp_frame, sizeof(rp_frame));
		} else {
			PRINT_DEBUG(("It fit horizontally!!\n"));
			fits = 1;
		}

		if (fits) {
			/*
			 * The current frame fits into the new space so keep
			 * its new frame parameters and maximize the window to
			 * fit the new frame size.
			 */
			if (cur->win_number != EMPTY) {
				rp_window *new =
				    find_window_number(cur->win_number);
				maximize_all_windows_in_frame(cur);
				XRaiseWindow(dpy, new->w);
			}
		} else {
			memcpy(cur, &tmp_frame, sizeof(rp_frame));
		}
	}

	frame_free(v, frame);
}

/*
 * Switch the input focus to another frame, and therefore a different window.
 */
void
set_active_frame(rp_frame *frame, int force_indicator)
{
	rp_vscreen *old_v = rp_current_vscreen;
	rp_vscreen *v = frame->vscreen;
	int old = old_v->current_frame;
	rp_window *win, *old_win;
	rp_frame *old_frame;

	win = find_window_number(frame->win_number);
	old_frame = current_frame(rp_current_vscreen);
	if (old_frame) {
		old_win = find_window_number(old_frame->win_number);
	} else {
		old_win = NULL;
	}

	/* Make the switch */
	give_window_focus(win, old_win);
	update_last_access(frame);
	v->current_frame = frame->number;

	/* If frame->win == NULL, then rp_current_screen is not updated. */
	rp_current_screen = v->screen;
	rp_current_screen->current_vscreen = v;

	update_bar(rp_current_screen);

	/* Possibly show the frame indicator. */
	if ((old != v->current_frame && num_frames(v) > 1) || v != old_v) {
		if (v != old_v)
			force_indicator = 1;

		show_frame_indicator(force_indicator);

		/*
		 * run the frame switch hook. We call it in here because this
		 * is when a frame switch ACTUALLY (for sure) happens.
		 */
		hook_run(&rp_switch_frame_hook);
	}
	/*
	 * If the frame has no window to give focus to, give the key window
	 * focus.
	 */
	if (frame->win_number == EMPTY) {
		set_window_focus(v->screen->key_window);
	}
	/* Call the switchscreen hook, when appropriate. */
	if (v->screen != old_v->screen) {
		hook_run(&rp_switch_screen_hook);
		hook_run(&rp_switch_vscreen_hook);
	}
}

void
exchange_with_frame(rp_frame *cur, rp_frame *frame)
{
	rp_window *win, *last_win;

	if (frame == NULL || frame == cur)
		return;

	/* Exchange the windows in the frames */
	win = find_window_number(cur->win_number);
	last_win = set_frames_window(frame, win);
	set_frames_window(cur, last_win);

	/* Make sure the windows comes up full screen */
	if (last_win)
		maximize(last_win);
	if (win) {
		maximize(win);
		/* Make sure the program bar is always on the top */
		update_window_names(win->vscreen->screen, defaults.window_fmt);
	}
	/* Make the switch */
	update_last_access(frame);

	set_active_frame(frame, 0);
}

void
blank_frame(rp_frame *frame)
{
	rp_vscreen *v;
	rp_window *win;

	if (frame->win_number == EMPTY)
		return;

	v = frame->vscreen;

	win = find_window_number(frame->win_number);
	hide_window(win);
	hide_others(win);

	set_frames_window(frame, NULL);

	update_bar(v->screen);

	/* Give the key window focus. */
	set_window_focus(v->screen->key_window);
}

void
hide_frame_indicator(void)
{
	rp_screen *cur;

	list_for_each_entry(cur, &rp_screens, node) {
		XUnmapWindow(dpy, cur->frame_window);
	}
}

void
show_frame_indicator(int force)
{
	if (num_frames(rp_current_vscreen) > 1 || force) {
		hide_frame_indicator();
		if (defaults.frame_indicator_timeout != -1) {
			show_frame_message(defaults.frame_fmt);
			alarm(defaults.frame_indicator_timeout);
		}
	}
}

void
show_frame_message(char *msg)
{
	rp_screen *s = rp_current_screen;
	int width, height;
	rp_frame *frame;
	rp_window *win;
	rp_window_elem *elem = NULL;
	struct sbuf *msgbuf;

	frame = current_frame(rp_current_vscreen);
	win = current_window();
	if (win) {
		elem = vscreen_find_window(&win->vscreen->mapped_windows, win);
		if (!elem)
			warnx("window 0x%lx not on any vscreen\n",
			    (unsigned long)win->w);
	}
	/* A frame doesn't always contain a window. */
	msgbuf = sbuf_new(0);
	if (elem)
		format_string(msg, elem, msgbuf);
	else
		sbuf_concat(msgbuf, EMPTY_FRAME_MESSAGE);

	width = defaults.bar_x_padding * 2
	    + rp_text_width(s, msgbuf->data, msgbuf->len, NULL);
	height = (FONT_HEIGHT(s) + defaults.bar_y_padding * 2);

	/*
	 * We don't want another frame indicator to be displayed on another
	 * screen at the same time, so we hide it before bringing it back
	 * again.
	 */
	hide_frame_indicator();

	XMoveResizeWindow(dpy, s->frame_window,
	    frame->x + frame->width / 2 - width / 2,
	    frame->y + frame->height / 2 - height / 2,
	    width, height);

	XMapRaised(dpy, s->frame_window);
	XClearWindow(dpy, s->frame_window);
	XSync(dpy, False);

	rp_draw_string(s, s->frame_window, STYLE_NORMAL,
	    defaults.bar_x_padding,
	    defaults.bar_y_padding + FONT_ASCENT(s),
	    msgbuf->data, msgbuf->len, NULL, NULL);

	sbuf_free(msgbuf);
}

rp_frame *
find_frame_up(rp_frame *frame)
{
	rp_frame *cur, *winner = NULL;
	rp_vscreen *v = frame->vscreen;
	int wingap = 0, curgap;

	list_for_each_entry(cur, &v->frames, node) {
		if (frame_top(frame) != frame_bottom(cur))
			continue;

		curgap = abs(frame_left(frame) - frame_left(cur));
		if (!winner || (curgap < wingap)) {
			winner = cur;
			wingap = curgap;
		}
	}

	return winner;
}

rp_frame *
find_frame_down(rp_frame *frame)
{
	rp_frame *cur, *winner = NULL;
	rp_vscreen *v = frame->vscreen;
	int wingap = 0, curgap;

	list_for_each_entry(cur, &v->frames, node) {
		if (frame_bottom(frame) != frame_top(cur))
			continue;

		curgap = abs(frame_left(frame) - frame_left(cur));
		if (!winner || (curgap < wingap)) {
			winner = cur;
			wingap = curgap;
		}
	}

	return winner;
}

rp_frame *
find_frame_left(rp_frame *frame)
{
	rp_frame *cur, *winner = NULL;
	rp_vscreen *v = frame->vscreen;
	int wingap = 0, curgap;

	list_for_each_entry(cur, &v->frames, node) {
		if (frame_left(frame) != frame_right(cur))
			continue;

		curgap = abs(frame_top(frame) - frame_top(cur));
		if (!winner || (curgap < wingap)) {
			winner = cur;
			wingap = curgap;
		}
	}

	return winner;
}

rp_frame *
find_frame_right(rp_frame *frame)
{
	rp_frame *cur, *winner = NULL;
	rp_vscreen *v = frame->vscreen;
	int wingap = 0, curgap;

	list_for_each_entry(cur, &v->frames, node) {
		if (frame_right(frame) != frame_left(cur))
			continue;

		curgap = abs(frame_top(frame) - frame_top(cur));
		if (!winner || (curgap < wingap)) {
			winner = cur;
			wingap = curgap;
		}
	}

	return winner;
}

rp_frame *
find_frame_number(rp_vscreen *v, int num)
{
	rp_frame *cur_frame;

	list_for_each_entry(cur_frame, &v->frames, node) {
		if (cur_frame->number == num)
			return cur_frame;
	}

	return NULL;
}
