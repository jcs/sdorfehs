/*
 * our datatypes and global variables
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

#ifndef _SDORFEHS_DATA_H
#define _SDORFEHS_DATA_H

#include "linkedlist.h"
#include "number.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

typedef struct rp_window rp_window;
typedef struct rp_screen rp_screen;
typedef struct rp_global_screen rp_global_screen;
typedef struct rp_vscreen rp_vscreen;
typedef struct rp_action rp_action;
typedef struct rp_keymap rp_keymap;
typedef struct rp_frame rp_frame;
typedef struct rp_child_info rp_child_info;
typedef struct rp_window_elem rp_window_elem;
typedef struct rp_completions rp_completions;
typedef struct rp_input_line rp_input_line;

enum rp_edge {
	EDGE_TOP = (1 << 1),
	EDGE_LEFT = (1 << 2),
	EDGE_RIGHT = (1 << 3),
	EDGE_BOTTOM = (1 << 4),
};

struct rp_frame {
	rp_vscreen *vscreen;

	int number;
	int x, y, width, height;

	/* The number of the window that is focused in this frame. */
	int win_number;

	/* The number of the window to focus when restoring this frame. */
	int restore_win_number;

	/* For determining the last frame. */
	int last_access;

	/*
	 * Boolean that is set when a frame is `dedicated' (a.k.a. glued) to
	 * one window.
	 */
	unsigned int dedicated;

	/* Whether this frame is touching an edge before a screen update */
	enum rp_edge edges;

	struct list_head node;
};

struct rp_window {
	rp_vscreen *vscreen;
	Window w;
	int state;
	int last_access;
	int named;

	/*
	 * A number uniquely identifying this window. This is a different
	 * number than the one given to it by the vscreen it is in. This number
	 * is used for internal purposes, whereas the vscreen number is what
	 * the user sees.
	 */
	int number;

	/* Window name hints. */
	char *user_name;
	char *wm_name;
	char *res_name;
	char *res_class;

	/* Dimensions */
	int x, y, width, height, border, full_screen;

	/* WM Hints */
	XSizeHints *hints;

	/* Colormap */
	Colormap colormap;

	/* Is this a transient window? */
	int transient;
	Window transient_for;

	/* Saved mouse position */
	int mouse_x, mouse_y;

	/*
	 * The alignment of the window. Decides to what side or corner the
	 * window sticks to.
	 */
	int gravity;

	/*
	 * A window can be visible inside a frame but not the frame's current
	 * window.  This keeps track of what frame the window was mapped into.
	 */
	int frame_number;

	/* The frame number we want to remain in */
	int sticky_frame;

	/*
	 * Sometimes a window is intended for a certain frame. When a window is
	 * mapped and this is >0 then use the frame (if it exists).
	 */
	int intended_frame_number;

	struct list_head node;
};

struct rp_window_elem {
	rp_window *win;
	int number;
	struct list_head node;
};

struct rp_global_screen {
	Window root, wm_check;
	unsigned long fgcolor, bgcolor, fwcolor, bwcolor, bar_bordercolor;

	/* This numset is responsible for giving out numbers for each screen */
	struct numset *numset;

	/* The path to and open fd of our control socket */
	char *control_socket_path;
	int control_socket_fd;

	/* The path to and open fd of our bar FIFO */
	char *bar_fifo_path;
	int bar_fifo_fd;
};

struct xrandr_info {
	int output;
	int crtc;
	int primary;
	char *name;
};

struct rp_vscreen {
	rp_screen *screen;

	/* Virtual screen number, handled by rp_screen's vscreens_numset */
	int number;

	/* Name */
	char *name;

	/* For determining the last vscreen. */
	int last_access;

	/*
	 * A list of frames that may or may not contain windows. There should
	 * always be one in the list.
	 */
	struct list_head frames;

	/* Keep track of which numbers have been given to frames. */
	struct numset *frames_numset;

	/*
	 * The number of the currently focused frame. One for each vscreen so
	 * when you switch vscreens the focus doesn't get frobbed.
	 */
	int current_frame;

	/* The list of windows participating in this vscreen. */
	struct list_head mapped_windows, unmapped_windows;

	/*
	 * This numset is responsible for giving out numbers for each window in
	 * the vscreen.
	 */
	struct numset *numset;

	struct list_head node;
};

struct rp_font {
	char *name;
	XftFont *font;
};

struct rp_screen {
	GC normal_gc, inverse_gc;
	Window root, bar_window, key_window, input_window, frame_window,
	    help_window;
	int bar_is_raised;
	int screen_num;	/* Our screen number as dictated by X */
	Colormap def_cmap;
	Cursor rat;

	/* Screen number, handled by rp_global_screen numset */
	int number;

	struct xrandr_info xrandr;

	/* Here to abstract over the Xrandr vs X screens difference */
	int left, top, width, height;

	char *display_string;

	/* Used by sfrestore */
	struct sbuf *scratch_buffer;

	XftFont *xft_font;
	struct rp_font xft_font_cache[5];
	XftColor xft_fgcolor, xft_bgcolor;

	struct list_head vscreens;
	struct numset *vscreens_numset;
	rp_vscreen *current_vscreen;

	rp_window *full_screen_win;

	struct sbuf *bar_text;

	/* This structure can exist in a list. */
	struct list_head node;
};

struct rp_action {
	KeySym key;
	unsigned int state;
	char *data;	/* misc data to be passed to the function */
	/* void (*func)(void *); */
};

struct rp_keymap {
	char *name;
	rp_action *actions;
	int actions_last;
	int actions_size;

	/* This structure can be part of a list. */
	struct list_head node;
};

struct rp_key {
	KeySym sym;
	unsigned int state;
};

struct rp_defaults {
	/*
	 * Default positions for new normal windows, transient windows, and
	 * normal windows with maxsize hints.
	 */
	int win_gravity;
	int trans_gravity;
	int maxsize_gravity;

	int input_window_size;
	int window_border_width;
	int only_border;

	int bar_x_padding;
	int bar_y_padding;
	int bar_location;
	int bar_timeout;
	int bar_border_width;
	int bar_in_padding;
	int bar_sticky;

	int frame_indicator_timeout;
	int frame_resize_unit;

	int padding_left;
	int padding_right;
	int padding_top;
	int padding_bottom;

	char *font_string;

	char *fgcolor_string;
	char *bgcolor_string;
	char *fwcolor_string;
	char *bwcolor_string;
	char *barbordercolor_string;

	int wait_for_key_cursor;

	char *window_fmt;
	char *info_fmt;
	char *sticky_fmt;
	char *resize_fmt;

	/* Which name to use: wm_name, res_name, res_class. */
	int win_name;

	int startup_message;

	/*
	 * Decides whether the window list is displayed in a row or a column.
	 */
	int window_list_style;

	/* Pointer warping toggle. */
	int warp;

	int history_size;
	/* remove older history when adding the same again */
	int history_compaction;
	/* expand ! when compiled with libhistory */
	int history_expansion;

	char *frame_selectors;

	/* How many frame sets to remember when undoing. */
	int maxundos;

	/* The name of the top level keymap */
	char *top_kmap;

	/* Frame indicator format */
	char *frame_fmt;

	/* Number of virtual screens */
	int vscreens;

	/* Window gap */
	int gap;

	/* Whether to ignore window size hints */
	int ignore_resize_hints;

	/* New mapped window always uses current vscreen */
	int win_add_cur_vscreen;
};

/* Information about a child process. */
struct rp_child_info {
	/* The command that was executed. */
	char *cmd;

	/* PID of the process. */
	int pid;

	/* Return status when the child process finished. */
	int status;

	/* When this is != 0 then the process finished. */
	int terminated;

	/* what was current when it was launched? */
	rp_frame *frame;
	rp_screen *screen;
	rp_vscreen *vscreen;

	/*
	 * Non-zero when the pid has mapped a window. This is to prevent every
	 * window the program opens from getting mapped in the frame it was
	 * launched from. Only the first window should do this.
	 */
	int window_mapped;

	/* This structure can exist in a list. */
	struct list_head node;
};

/*
 * These defines should be used to specify the modifier mask for keys and they
 * are translated into the X11 modifier mask when the time comes to compare
 * modifier masks.
 */
#define RP_SHIFT_MASK   1
#define RP_CONTROL_MASK 2
#define RP_META_MASK    4
#define RP_ALT_MASK     8
#define RP_SUPER_MASK   16
#define RP_HYPER_MASK   32

struct modifier_info {
	/* unsigned int mode_switch_mask; */
	unsigned int meta_mod_mask;
	unsigned int alt_mod_mask;
	unsigned int super_mod_mask;
	unsigned int hyper_mod_mask;

	/*
	 * Keep track of these because they mess up the grab and should be
	 * ignored.
	 */
	unsigned int num_lock_mask;
	unsigned int scroll_lock_mask;
};

typedef struct list_head *(*completion_fn) (char *string);

/*
  BASIC: The completion shall begin with the same characters as the partial
  string. Case is ignored.

  SUBSTRING: The partial string shall be a subpart of the completion. Case
  is ignored.
*/
enum completion_styles {
	BASIC,
	SUBSTRING
};

struct rp_completions {
	/*
	 * A pointer to the partial string that is being completed. We need to
	 * store this so that the user can cycle through all possible
	 * completions.
	 */
	char *partial;

	/*
	 * A pointer to the string that was last matched string. Used to keep
	 * track of where we are in the completion list.
	 */
	struct sbuf *last_match;

	/* A list of sbuf's which are possible completions. */
	struct list_head completion_list;

	/* The function that generates the completions. */
	completion_fn complete_fn;

	/*
	 * virgin = 1 means no completions have been attempted on the input
	 * string.
	 */
	unsigned short int virgin;

	/* The completion style used to perform string comparisons */
	enum completion_styles style;
};

struct rp_input_line {
	char *buffer;
	char *prompt;
	char *saved;
	size_t position;
	size_t length;
	size_t size;
	rp_completions *compl;
	Atom selection;
	int history_id;
};

/* The hook dictionary. */
struct rp_hook_db_entry {
	char *name;
	struct list_head *hook;
};

typedef struct rp_xselection rp_xselection;
struct rp_xselection {
	char *text;
	int len;
};

#endif	/* _SDORFEHS_DATA_H */
