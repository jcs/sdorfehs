/*
 * Functionality for a bar listing the windows currently managed.
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
#include <X11/Xft/Xft.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <sys/time.h>

#include "sdorfehs.h"

/* Possible values for bar_is_raised status. */
#define BAR_IS_HIDDEN		0
#define BAR_IS_WINDOW_LIST	1
#define BAR_IS_VSCREEN_LIST	2
#define BAR_IS_MESSAGE		3
#define BAR_IS_STICKY		4

#define BAR_IS_RAISED(s)	(s->bar_is_raised != BAR_IS_HIDDEN && \
				s->bar_is_raised != BAR_IS_STICKY)

/* A copy of the last message displayed in the message bar. */
static char *last_msg = NULL;
static int last_mark_start = 0;
static int last_mark_end = 0;

/* Buffers for sticky bar */
static struct sbuf *bar_buf = NULL;
static struct sbuf *bar_line = NULL;
struct list_head bar_chunks;
static char *last_bar_line;
static char *last_bar_window_fmt;
static char bar_tmp_line[256];
static Pixmap bar_pm;

struct bar_chunk {
	char *text;
	int length;
	char *color;
	char *font;
	char *cmd;
	int cmd_btn;
	int text_x;
	int text_width;
	struct list_head node;
};

static void draw_partial_string(rp_screen *s, char *msg, int len, int x_offset,
    int y_offset, int style, char *color);
static void marked_message_internal(char *msg, int mark_start, int mark_end,
    int bar_type);

/* Reset the alarm to auto-hide the bar in BAR_TIMEOUT seconds. */
static void
reset_alarm(void)
{
	struct timeval timeout;
	struct itimerval alarmtimer;

	memset(&timeout, 0, sizeof(timeout));
	memset(&alarmtimer, 0, sizeof(alarmtimer));

	timeout.tv_sec = (time_t)(defaults.bar_timeout);
	alarmtimer.it_value = timeout;
	setitimer(ITIMER_REAL, &alarmtimer, NULL);

	alarm_signalled = 0;
}

static int
bar_time_left(void)
{
	struct itimerval left;

	getitimer(ITIMER_REAL, &left);
	return left.it_value.tv_sec > 0;
}

int
bar_mkfifo(void)
{
	char *config_dir;

	config_dir = get_config_dir();
	rp_glob_screen.bar_fifo_path = xsprintf("%s/bar", config_dir);
	free(config_dir);

	unlink(rp_glob_screen.bar_fifo_path);

	if (mkfifo(rp_glob_screen.bar_fifo_path, S_IRUSR|S_IWUSR) == -1) {
		warn("failed creating bar FIFO at %s",
		    rp_glob_screen.bar_fifo_path);
		return 0;
	}

	return bar_open_fifo();
}

void
init_bar(void)
{
	rp_screen *primary = screen_primary();

	bar_buf = sbuf_new(sizeof(bar_tmp_line));
	bar_line = sbuf_new(sizeof(bar_tmp_line));
	bar_pm = XCreatePixmap(dpy, primary->bar_window, primary->width,
	    FONT_HEIGHT(rp_current_screen) * 2,
	    DefaultDepth(dpy, DefaultScreen(dpy)));

	INIT_LIST_HEAD(&bar_chunks);
}

/* Hide the bar from sight. */
void
hide_bar(rp_screen *s, int force)
{
	if (!s->full_screen_win && defaults.bar_sticky && !force) {
		redraw_sticky_bar_text(0);

		if (s == screen_primary())
			return;
		/* otherwise, we need to hide this secondary screen's bar */
	}

	s->bar_is_raised = BAR_IS_HIDDEN;
	XUnmapWindow(dpy, s->bar_window);

	/* Possibly restore colormap. */
	if (current_window()) {
		XUninstallColormap(dpy, s->def_cmap);
		XInstallColormap(dpy, current_window()->colormap);
	}
}

/* Show window listing in bar. */
void
show_bar(rp_screen *s, char *fmt)
{
	s->bar_is_raised = BAR_IS_WINDOW_LIST;
	XMapRaised(dpy, s->bar_window);
	update_window_names(s, fmt);

	/* Switch to the default colormap */
	if (current_window())
		XUninstallColormap(dpy, current_window()->colormap);
	XInstallColormap(dpy, s->def_cmap);

	raise_utility_windows();

	reset_alarm();
}

/* Show vscreen listing in bar. */
void
show_vscreen_bar(rp_screen *s)
{
	s->bar_is_raised = BAR_IS_VSCREEN_LIST;
	XMapRaised(dpy, s->bar_window);
	update_vscreen_names(s);

	/* Switch to the default colormap */
	if (current_window())
		XUninstallColormap(dpy, current_window()->colormap);
	XInstallColormap(dpy, s->def_cmap);

	raise_utility_windows();

	reset_alarm();
}

int
bar_x(rp_screen *s, int width)
{
	int x = 0;

	switch (defaults.bar_location) {
	case NorthWestGravity:
	case WestGravity:
	case SouthWestGravity:
		x = s->left +
		    (defaults.bar_in_padding ? 0 : defaults.padding_left);
		break;
	case NorthGravity:
	case CenterGravity:
	case SouthGravity:
		x = s->left +
		    (s->width - width - defaults.bar_border_width * 2) / 2 -
		    (defaults.bar_in_padding ? 0 : defaults.padding_left);
		break;
	case NorthEastGravity:
	case EastGravity:
	case SouthEastGravity:
		x = s->left + s->width - width -
		    (defaults.bar_border_width * 2) -
		    (defaults.bar_in_padding ? 0 : defaults.padding_right);
		break;
	}

	return x;
}

int
bar_y(rp_screen *s, int height)
{
	int y = 0;

	switch (defaults.bar_location) {
	case NorthEastGravity:
	case NorthGravity:
	case NorthWestGravity:
		y = s->top +
		    (defaults.bar_in_padding ? 0 : defaults.padding_top);
		break;
	case EastGravity:
	case CenterGravity:
	case WestGravity:
		y = s->top + (s->height - height -
		    defaults.bar_border_width * 2) / 2 -
		    (defaults.bar_in_padding ? 0 : defaults.padding_top);
		break;
	case SouthEastGravity:
	case SouthGravity:
	case SouthWestGravity:
		y = s->top + (s->height - height -
		    defaults.bar_border_width * 2) -
		    (defaults.bar_in_padding ? 0 : defaults.padding_top);
		break;
	}

	return y;
}

int
sticky_bar_height(rp_screen *s)
{
	if (defaults.bar_sticky)
		return FONT_HEIGHT(s) +
		    (defaults.bar_border_width * 2) +
		    (defaults.bar_y_padding * 2);

	return 0;
}

void
update_bar(rp_screen *s)
{
	switch (s->bar_is_raised) {
	case BAR_IS_WINDOW_LIST:
		update_window_names(s, defaults.window_fmt);
		break;
	case BAR_IS_VSCREEN_LIST:
		update_vscreen_names(s);
		break;
	case BAR_IS_STICKY:
		hide_bar(s, 0);
		break;
	}
}

/*
 * To avoid having to parse the bar line text and redraw text over and over
 * again, text is drawn on a pixmap in two lines.  When the window format text
 * changes (such as when the current window changes) or the bar line text
 * changes, each line that is different is redrawn.  Then the two lines are
 * copied into the actual bar window side by side.
 */
void
redraw_sticky_bar_text(int force)
{
	rp_screen *s = screen_primary();
	struct list_head *iter, *tmp;
	struct bar_chunk *chunk;
	struct sbuf *tbuf, *curcmd, *curtxt;
	char *tline, *font, *color, *clickcmd;
	int diff = 0, len, cmd = 0, skip = 0, xftx = 0, x, clickcmdbtn = 0;
	int width, height;

	if (!force && (s->full_screen_win || !defaults.bar_sticky ||
	    bar_time_left()))
		return;

	/*
	 * If we were showing a message or window list before, make sure we
	 * clear it all.
	 */
	if (s->bar_is_raised != BAR_IS_STICKY)
		force = 1;

	width = s->width - (defaults.bar_border_width * 2);
	if (!defaults.bar_in_padding)
		width -= defaults.padding_right + defaults.padding_left;
	height = FONT_HEIGHT(s) + (defaults.bar_y_padding * 2);
	XMoveResizeWindow(dpy, s->bar_window, bar_x(s, width), bar_y(s, height),
	    width, height);

	if (force) {
		XFreePixmap(dpy, bar_pm);
		XClearWindow(dpy, s->bar_window);
		bar_pm = XCreatePixmap(dpy, s->bar_window, width,
		    FONT_HEIGHT(s) * 2, DefaultDepth(dpy, DefaultScreen(dpy)));
	}

	s->bar_is_raised = BAR_IS_STICKY;
	XMapRaised(dpy, s->bar_window);

	/* check window title for changes */
	tbuf = sbuf_new(0);
	get_current_window_in_fmt(defaults.sticky_fmt, tbuf);
	diff = (last_bar_window_fmt == NULL ||
	    strcmp(last_bar_window_fmt, sbuf_get(tbuf)) != 0);

	if (diff || force) {
		PRINT_DEBUG(("redrawing window format in bar\n"));

		if (last_bar_window_fmt != NULL)
			free(last_bar_window_fmt);
		last_bar_window_fmt = xstrdup(sbuf_get(tbuf));

		XFillRectangle(dpy, bar_pm, s->inverse_gc, 0, 0, s->width,
		    FONT_HEIGHT(s));

		rp_draw_string(s, bar_pm, STYLE_NORMAL, 0, FONT_ASCENT(s),
		    last_bar_window_fmt, strlen(last_bar_window_fmt),
		    NULL, NULL);
	}
	sbuf_free(tbuf);

	XCopyArea(dpy, bar_pm, s->bar_window, s->inverse_gc,
	    0, 0, (s->width / 2) - (defaults.bar_x_padding * 2), FONT_HEIGHT(s),
	    defaults.bar_x_padding, defaults.bar_y_padding);

	/* Repeat for bar text line */

	diff = (last_bar_line == NULL ||
	    strcmp(last_bar_line, sbuf_get(bar_line)) != 0);
	if (!diff && !force)
		goto redraw_bar_text;

	PRINT_DEBUG(("recalculating bar chunks\n"));

	if (last_bar_line)
		free(last_bar_line);
	last_bar_line = xstrdup(sbuf_get(bar_line));
	len = strlen(last_bar_line);

	curcmd = sbuf_new(0);
	curtxt = sbuf_new(0);

	list_for_each_safe_entry(chunk, iter, tmp, &bar_chunks, node) {
		free(chunk->text);
		if (chunk->color)
			free(chunk->color);
		if (chunk->font)
			free(chunk->font);
		if (chunk->cmd)
			free(chunk->cmd);
		list_del(&chunk->node);
		free(chunk);
	}

	color = font = clickcmd = NULL;
	for (x = 0; x < len; x++) {
		if (last_bar_line[x] == '\\' && !skip) {
			skip = 1;
			continue;
		}

		if (cmd) {
			if (last_bar_line[x] == ')' && !skip) {
				tline = sbuf_get(curcmd);
				if (strncmp(tline, "fg(", 3) == 0) {
					/* ^fg(green)text^fg() */
					if (color)
						free(color);
					color = NULL;
					if (strlen(tline) > 3)
						color = xstrdup(tline + 3);
				} else if (strncmp(tline, "fn(", 3) == 0) {
					/* ^fn(courier)text^fn() */
					if (font)
						free(font);
					font = NULL;
					if (strlen(tline) > 3)
						font = xstrdup(tline + 3);
				} else if (strncmp(tline, "ca(", 3) == 0) {
					/* ^ca(1,some command)*^ca() */
					char btn[2];
					if (clickcmd)
						free(clickcmd);
					clickcmd = NULL;
					if (strlen(tline) > 4) {
						btn[0] = (tline + 3)[0];
						btn[1] = '\0';
						clickcmdbtn = atoi(btn);
						clickcmd = xstrdup(tline + 3 +
						    2);
					}
				} else {
					PRINT_DEBUG(("unsupported bar command "
					    "\"%s\", ignoring\n", tline));
				}
				sbuf_clear(curcmd);
				cmd = 0;
			} else
				sbuf_nconcat(curcmd, last_bar_line + x, 1);
		} else if (last_bar_line[x] == '^' && !skip) {
			cmd = 1;
			chunk = xmalloc(sizeof(struct bar_chunk));
			memset(chunk, 0, sizeof(struct bar_chunk));
			chunk->text = xstrdup(sbuf_get(curtxt));
			chunk->length = strlen(chunk->text);
			chunk->color = color ? xstrdup(color) : NULL;
			chunk->font = font ? xstrdup(font) : NULL;
			chunk->cmd = clickcmd ? xstrdup(clickcmd) : NULL;
			chunk->cmd_btn = clickcmdbtn;
			list_add_tail(&chunk->node, &bar_chunks);
			sbuf_clear(curtxt);
		} else {
			sbuf_nconcat(curtxt, last_bar_line + x, 1);
		}

		skip = 0;
	}
	tline = sbuf_get(curtxt);
	if (strlen(tline)) {
		chunk = xmalloc(sizeof(struct bar_chunk));
		memset(chunk, 0, sizeof(struct bar_chunk));
		chunk->text = xstrdup(sbuf_get(curtxt));
		chunk->length = strlen(chunk->text);
		chunk->color = color ? xstrdup(color) : NULL;
		chunk->font = font ? xstrdup(font) : NULL;
		chunk->cmd = clickcmd ? xstrdup(clickcmd) : NULL;
		chunk->cmd_btn = clickcmdbtn;
		list_add_tail(&chunk->node, &bar_chunks);
	}
	sbuf_free(curcmd);
	sbuf_free(curtxt);
	if (color)
		free(color);
	if (font)
		free(font);
	if (clickcmd)
		free(clickcmd);

redraw_bar_text:
	XFillRectangle(dpy, bar_pm, s->inverse_gc, 0, FONT_HEIGHT(s), s->width,
	    FONT_HEIGHT(s));
	xftx = 0;
	list_for_each_entry(chunk, &bar_chunks, node) {
		rp_draw_string(s, bar_pm, STYLE_NORMAL,
		    (width / 2) + xftx,
	    	    FONT_HEIGHT(s) + FONT_ASCENT(s),
		    chunk->text, chunk->length,
		    chunk->font ? chunk->font : NULL,
		    chunk->color ? chunk->color : NULL);

		xftx += (chunk->text_width = rp_text_width(s, chunk->text,
		    chunk->length, chunk->font));
	}
	if (xftx > (width / 2) - (defaults.bar_x_padding * 2))
		xftx = (width / 2) - (defaults.bar_x_padding * 2);

	/* update each chunk's text_x relative to its final location */
	x = 0;
	list_for_each_entry_prev(chunk, &bar_chunks, node) {
		chunk->text_x = s->width - defaults.bar_x_padding -
		    chunk->text_width - x;
		x += chunk->text_width;
	}

	XCopyArea(dpy, bar_pm, s->bar_window, s->inverse_gc,
	    xftx + (defaults.bar_x_padding * 2), FONT_HEIGHT(s),
	    (width / 2) - (defaults.bar_x_padding * 2), FONT_HEIGHT(s),
	    (width / 2) + defaults.bar_x_padding,
	    defaults.bar_y_padding);

	/* Our XMapRaise may have covered one */
	raise_utility_windows();
}

void
bar_handle_click(rp_screen *s, XButtonEvent *e)
{
	struct bar_chunk *chunk;

	PRINT_DEBUG(("bar click at %d,%d button %d\n", e->x, e->y, e->button));

	list_for_each_entry(chunk, &bar_chunks, node) {
		if (!chunk->cmd)
			continue;

		if (e->button == chunk->cmd_btn && e->x >= chunk->text_x &&
		    e->x <= (chunk->text_x + chunk->text_width)) {
			PRINT_DEBUG(("executing bar click action %s\n",
			    chunk->cmd));
			spawn(chunk->cmd, current_frame(rp_current_vscreen));
		}
	}
}

/*
 * Note that we use marked_message_internal to avoid resetting the alarm.
 */
void
update_window_names(rp_screen *s, char *fmt)
{
	struct sbuf *bar_buffer;
	int mark_start = 0;
	int mark_end = 0;
	char *delimiter;

	bar_buffer = sbuf_new(0);

	if (s->bar_is_raised == BAR_IS_STICKY) {
		redraw_sticky_bar_text(0);
	} else if (s->bar_is_raised == BAR_IS_WINDOW_LIST) {
		delimiter = (defaults.window_list_style == STYLE_ROW) ?
		    " " : "\n";

		get_window_list(fmt, delimiter, bar_buffer, &mark_start,
		    &mark_end);
		marked_message(sbuf_get(bar_buffer), mark_start, mark_end,
		    BAR_IS_WINDOW_LIST);
	}

	sbuf_free(bar_buffer);
}

/*
 * Note that we use marked_message_internal to avoid resetting the alarm.
 */
void
update_vscreen_names(rp_screen *s)
{
	struct sbuf *bar_buffer;
	int mark_start = 0;
	int mark_end = 0;
	char *delimiter;

	if (s->bar_is_raised != BAR_IS_VSCREEN_LIST)
		return;

	delimiter = (defaults.window_list_style == STYLE_ROW) ? " " : "\n";

	bar_buffer = sbuf_new(0);

	get_vscreen_list(s, delimiter, bar_buffer, &mark_start, &mark_end);
	marked_message_internal(sbuf_get(bar_buffer), mark_start, mark_end,
	    BAR_IS_VSCREEN_LIST);

	sbuf_free(bar_buffer);
}

void
message(char *s)
{
	marked_message(s, 0, 0, BAR_IS_MESSAGE);
}

void
marked_message_printf(int mark_start, int mark_end, char *fmt,...)
{
	char *buffer;
	va_list ap;

	va_start(ap, fmt);
	buffer = xvsprintf(fmt, ap);
	va_end(ap);

	marked_message(buffer, mark_start, mark_end, BAR_IS_MESSAGE);
	free(buffer);
}

static int
count_lines(char *msg, int len)
{
	int ret = 1;
	int i;

	if (len < 1)
		return 1;

	for (i = 0; i < len; i++) {
		if (msg[i] == '\n')
			ret++;
	}

	return ret;
}


static int
max_line_length(char *msg)
{
	rp_screen *s = screen_primary();
	size_t i;
	size_t start;
	int ret = 0;

	/* Count each line and keep the length of the longest one. */
	for (start = 0, i = 0; i <= strlen(msg); i++) {
		if (msg[i] == '\n' || msg[i] == '\0') {
			int current_width;

			/* Check if this line is the longest so far. */
			current_width = rp_text_width(s, msg + start,
			    i - start, NULL);
			if (current_width > ret) {
				ret = current_width;
			}
			/* Update the start of the new line. */
			start = i + 1;
		}
	}

	return ret;
}

static int
pos_in_line(char *msg, int pos)
{
	int ret;
	int i;

	if (pos <= 0)
		return 0;

	/*
	 * Go backwards until we hit the beginning of the string or a new line.
	 */
	ret = 0;
	for (i = pos - 1; i >= 0; ret++, i--) {
		if (msg[i] == '\n')
			break;
	}

	return ret;
}

static int
line_beginning(char *msg, int pos)
{
	int ret = 0;
	int i;

	if (pos <= 0)
		return 0;

	/*
	 * Go backwards until we hit a new line or the beginning of the string.
	 */
	for (i = pos - 1; i >= 0; --i) {
		if (msg[i] == '\n') {
			ret = i + 1;
			break;
		}
	}

	return ret;
}

static void
draw_partial_string(rp_screen *s, char *msg, int len, int x_offset,
    int y_offset, int style, char *color)
{
	rp_draw_string(s, s->bar_window, style,
	    defaults.bar_x_padding + x_offset,
	    defaults.bar_y_padding + FONT_ASCENT(s) + y_offset * FONT_HEIGHT(s),
	    msg, len + 1, NULL, color);
}

#define REASON_NONE    0x00
#define REASON_STYLE   0x01
#define REASON_NEWLINE 0x02
static void
draw_string(rp_screen *s, char *msg, int mark_start, int mark_end)
{
	int i, start;
	int x_offset, y_offset;	/* Base coordinates where to print. */
	int print_reason = REASON_NONE;	/* Should we print something? */
	int style = STYLE_NORMAL, next_style = STYLE_NORMAL;
	int msg_len, part_len;

	start = 0;
	x_offset = y_offset = 0;
	msg_len = strlen(msg);

	/* Walk through the string, print each part. */
	for (i = 0; i < msg_len; ++i) {
		/* Should we ignore style hints? */
		if (mark_start != mark_end) {
			if (i == mark_start) {
				next_style = STYLE_INVERSE;
				if (i > start)
					print_reason |= REASON_STYLE;
			} else if (i == mark_end) {
				next_style = STYLE_NORMAL;
				if (i > start)
					print_reason |= REASON_STYLE;
			}
		}
		if (msg[i] == '\n')
			print_reason |= REASON_NEWLINE;

		if (print_reason != REASON_NONE) {
			/* Strip the trailing newline if necessary. */
			part_len = i - start -
			    ((print_reason & REASON_NEWLINE) ? 1 : 0);

			draw_partial_string(s, msg + start, part_len,
			    x_offset, y_offset, style, NULL);

			/* Adjust coordinates. */
			if (print_reason & REASON_NEWLINE) {
				x_offset = 0;
				y_offset++;
				/* Skip newline. */
				start = i + 1;
			} else {
				x_offset += rp_text_width(s, msg + start,
				    part_len, NULL);
				start = i;
			}

			print_reason = REASON_NONE;
		}
		style = next_style;
	}

	part_len = i - start - 1;

	/* Print the last line. */
	draw_partial_string(s, msg + start, part_len, x_offset, y_offset,
	    style, NULL);

	XSync(dpy, False);
}
#undef REASON_NONE
#undef REASON_STYLE
#undef REASON_NEWLINE

/*
 * Move the marks if they are outside the string or if the start is after the
 * end.
 */
static void
correct_mark(int msg_len, int *mark_start, int *mark_end)
{
	/* Make sure the marks are inside the string. */
	if (*mark_start < 0)
		*mark_start = 0;

	if (*mark_end < 0)
		*mark_end = 0;

	if (*mark_start > msg_len)
		*mark_start = msg_len;

	if (*mark_end > msg_len)
		*mark_end = msg_len;

	/* Make sure the marks aren't reversed. */
	if (*mark_start > *mark_end) {
		int tmp;
		tmp = *mark_start;
		*mark_start = *mark_end;
		*mark_end = tmp;
	}
}

/* Raise the bar and put it in the right spot */
static void
prepare_bar(rp_screen *s, int width, int height, int bar_type)
{
	if (defaults.bar_sticky)
		width = s->width - (defaults.bar_border_width * 2);
	else
		width = width < s->width ? width : s->width;
	if (!defaults.bar_in_padding)
		width -= defaults.padding_right + defaults.padding_left;
	height = height < s->height ? height : s->height;
	XMoveResizeWindow(dpy, s->bar_window, bar_x(s, width), bar_y(s, height),
	    width, height);

	/* Map the bar if needed */
	if (!BAR_IS_RAISED(s)) {
		s->bar_is_raised = bar_type;
		XMapRaised(dpy, s->bar_window);

		/* Switch to the default colormap */
		if (current_window())
			XUninstallColormap(dpy, current_window()->colormap);
		XInstallColormap(dpy, s->def_cmap);
	}
	XRaiseWindow(dpy, s->bar_window);
	XClearWindow(dpy, s->bar_window);

	raise_utility_windows();

	XSync(dpy, False);
}

static void
get_mark_box(char *msg, size_t mark_start, size_t mark_end, int *x, int *y,
    int *width, int *height)
{
	rp_screen *s = rp_current_screen;
	int start, end;
	int mark_end_is_new_line = 0;
	int start_line;
	int end_line;
	int start_pos_in_line;
	int end_pos_in_line;
	int start_line_beginning;
	int end_line_beginning;

	/*
	 * If the mark_end is on a new line or the end of the string, then back
	 * it up one character.
	 */
	if (msg[mark_end - 1] == '\n' || mark_end == strlen(msg)) {
		mark_end--;
		mark_end_is_new_line = 1;
	}
	start_line = count_lines(msg, mark_start);
	end_line = count_lines(msg, mark_end);

	start_pos_in_line = pos_in_line(msg, mark_start);
	end_pos_in_line = pos_in_line(msg, mark_end);

	start_line_beginning = line_beginning(msg, mark_start);
	end_line_beginning = line_beginning(msg, mark_end);

	PRINT_DEBUG(("start_line = %d, end_line = %d\n", start_line, end_line));
	PRINT_DEBUG(("start_line_beginning = %d, end_line_beginning = %d\n",
		start_line_beginning, end_line_beginning));

	if (mark_start == 0 || start_pos_in_line == 0)
		start = 0;
	else
		start = rp_text_width(s, &msg[start_line_beginning],
		    start_pos_in_line, NULL) + defaults.bar_x_padding;

	end = rp_text_width(s, &msg[end_line_beginning],
	    end_pos_in_line, NULL) + defaults.bar_x_padding * 2;

	if (mark_end != strlen(msg))
		end -= defaults.bar_x_padding;

	/*
	 * A little hack to highlight to the end of the line, if the mark_end
	 * is at the end of a line.
	 */
	if (mark_end_is_new_line) {
		*width = max_line_length(msg) + defaults.bar_x_padding * 2;
	} else {
		*width = end - start;
	}

	*x = start;
	*y = (start_line - 1) * FONT_HEIGHT(s) + defaults.bar_y_padding;
	*height = (end_line - start_line + 1) * FONT_HEIGHT(s);
}

static void
draw_box(rp_screen *s, int x, int y, int width, int height)
{
	XGCValues lgv;
	GC lgc;
	unsigned long mask;

	lgv.foreground = rp_glob_screen.fg_color;
	mask = GCForeground;
	lgc = XCreateGC(dpy, s->root, mask, &lgv);

	XFillRectangle(dpy, s->bar_window, lgc, x, y, width, height);
	XFreeGC(dpy, lgc);
}

static void
draw_mark(rp_screen *s, char *msg, int mark_start, int mark_end)
{
	int x, y, width, height;

	/* when this happens, there is no mark. */
	if (mark_end == 0 || mark_start == mark_end)
		return;

	get_mark_box(msg, mark_start, mark_end,
	    &x, &y, &width, &height);
	draw_box(s, x, y, width, height);
}

static void
update_last_message(char *msg, int mark_start, int mark_end)
{
	free(last_msg);
	last_msg = xstrdup(msg);
	last_mark_start = mark_start;
	last_mark_end = mark_end;
}

void
marked_message(char *msg, int mark_start, int mark_end, int bar_type)
{
	/* Schedule the bar to be hidden after some amount of time. */
	reset_alarm();
	marked_message_internal(msg, mark_start, mark_end, bar_type);
}

static void
marked_message_internal(char *msg, int mark_start, int mark_end, int bar_type)
{
	rp_screen *s;
	int num_lines;
	int width;
	int height;

	if (bar_type == BAR_IS_STICKY)
		s = screen_primary();
	else
		s = rp_current_screen;

	PRINT_DEBUG(("msg = %s\n", msg ? msg : "NULL"));
	PRINT_DEBUG(("mark_start = %d, mark_end = %d\n", mark_start, mark_end));

	/* Calculate the width and height of the window. */
	num_lines = count_lines(msg, strlen(msg));
	width = defaults.bar_x_padding * 2 + max_line_length(msg);
	if (!defaults.bar_in_padding)
		width -= defaults.padding_right + defaults.padding_left;
	height = FONT_HEIGHT(s) * num_lines + defaults.bar_y_padding * 2;

	prepare_bar(s, width, height, bar_type);

	/* Draw the mark over the designated part of the string. */
	correct_mark(strlen(msg), &mark_start, &mark_end);
	draw_mark(s, msg, mark_start, mark_end);

	draw_string(s, msg, mark_start, mark_end);

	/* Keep a record of the message. */
	update_last_message(msg, mark_start, mark_end);

	if (bar_type != BAR_IS_STICKY && bar_time_left())
		reset_alarm();
}

/*
 * Use this just to update the bar. show_last_message will draw it and leave it
 * up for a period of time.
 */
void
redraw_last_message(void)
{
	char *msg;

	if (last_msg == NULL)
		return;

	/*
	 * A little kludge to avoid last_msg in marked_message from being
	 * strdup'd right after freeing the pointer. Note: in this case
	 * marked_message's msg arg would have been the same as last_msg.
	 */
	msg = xstrdup(last_msg);
	marked_message_internal(msg, last_mark_start, last_mark_end,
	    BAR_IS_MESSAGE);
	free(msg);
}

void
show_last_message(void)
{
	redraw_last_message();
	reset_alarm();
}

/* Free any memory associated with the bar. */
void
free_bar(void)
{
	free(last_msg);
	last_msg = NULL;
}

int
bar_open_fifo(void)
{
	rp_glob_screen.bar_fifo_fd = open(rp_glob_screen.bar_fifo_path,
	    O_RDONLY|O_NONBLOCK|O_CLOEXEC);
	if (rp_glob_screen.bar_fifo_fd == -1) {
		warn("failed opening newly-created bar FIFO at %s",
		    rp_glob_screen.bar_fifo_path);
		rp_glob_screen.bar_fifo_fd = -1;
		return -1;
	}

	return 0;
}

void
bar_read_fifo(void)
{
	ssize_t ret;
	int x, start;

	PRINT_DEBUG(("bar FIFO data to read\n"));

	for (;;) {
		memset(bar_tmp_line, 0, sizeof(bar_tmp_line));
		ret = read(rp_glob_screen.bar_fifo_fd, &bar_tmp_line,
		    sizeof(bar_tmp_line));
		if (ret < 1) {
			if (ret == 0)
				PRINT_DEBUG(("FIFO %d closed, re-opening\n",
				    rp_glob_screen.bar_fifo_fd));
			else if (ret == -1 && errno != EAGAIN)
				PRINT_DEBUG(("error reading bar FIFO: %s\n",
				    strerror(errno)));

			close(rp_glob_screen.bar_fifo_fd);
			bar_open_fifo();
			break;
		}

		for (x = 0, start = 0; x < ret; x++) {
			if (bar_tmp_line[x] == '\0') {
				sbuf_nconcat(bar_buf, bar_tmp_line + start,
				    x - start);
				start = x + 1;
				break;
			} else if (bar_tmp_line[x] == '\n') {
				sbuf_nconcat(bar_buf, bar_tmp_line + start,
				    x - start);
				sbuf_copy(bar_line, sbuf_get(bar_buf));
				redraw_sticky_bar_text(0);
				sbuf_clear(bar_buf);
				start = x + 1;
			}
		}

		if (x == ret)
			sbuf_nconcat(bar_buf, bar_tmp_line + start, x - start);
	}
}
