/*
 * Config file for ratpoison. Edit these values and recompile.
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
 *
 * This file is part of ratpoison.
 *
 * ratpoison is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2, or (at your option) any later
 * version.
 *
 * ratpoison is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this software; see the file COPYING.  If not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _RATPOISON_CONF_H
#define _RATPOISON_CONF_H 1

#include "data.h"
#include "actions.h"

#define PROGNAME	"ratpoison"
#define VERSION		"20190816"

/* Enter command mode with Control+T by default. */
#define KEY_PREFIX      XK_t
#define MODIFIER_PREFIX RP_CONTROL_MASK

/* Terminal executed by default. */
#define TERM_PROG	"xterm"

/* This is the abort key when typing input. */
#define INPUT_ABORT_KEY      XK_g
#define INPUT_ABORT_MODIFIER RP_CONTROL_MASK

/* This is the previous history entry key when typing input. */
#define INPUT_PREV_HISTORY_KEY      XK_p
#define INPUT_PREV_HISTORY_MODIFIER RP_CONTROL_MASK

/* This is the next history entry key when typing input. */
#define INPUT_NEXT_HISTORY_KEY      XK_n
#define INPUT_NEXT_HISTORY_MODIFIER RP_CONTROL_MASK

/* Key used to enlarge frame vertically when in resize mode.  */
#define RESIZE_VGROW_KEY      XK_n
#define RESIZE_VGROW_MODIFIER RP_CONTROL_MASK

/* Key used to shrink frame vertically when in resize mode.  */
#define RESIZE_VSHRINK_KEY      XK_p
#define RESIZE_VSHRINK_MODIFIER RP_CONTROL_MASK

/* Key used to enlarge frame horizontally when in resize mode.  */
#define RESIZE_HGROW_KEY      XK_f
#define RESIZE_HGROW_MODIFIER RP_CONTROL_MASK

/* Key used to shrink frame horizontally when in resize mode.  */
#define RESIZE_HSHRINK_KEY      XK_b
#define RESIZE_HSHRINK_MODIFIER RP_CONTROL_MASK

/* Key used to shrink frame to fit it's current window.  */
#define RESIZE_SHRINK_TO_WINDOW_KEY             XK_s
#define RESIZE_SHRINK_TO_WINDOW_MODIFIER        0

/* Key used to exit resize mode.  */
#define RESIZE_END_KEY      XK_Return
#define RESIZE_END_MODIFIER 0

/* Number of history items to store. */
#define INPUT_MAX_HISTORY 50

/*
 * Treat windows with maxsize hints as if they were a transient window (don't
 * hide the windows underneath, and center them)
 */
#define MAXSIZE_WINDOWS_ARE_TRANSIENTS

/*
 * Treat windows with aspect hints as if they were a transient window (don't
 * hide the windows underneath, and center them)
 */
#define ASPECT_WINDOWS_ARE_TRANSIENTS

/*
 * An alias command could recursively call inself infinitely. This stops that
 * behavior.
 */
#define MAX_ALIAS_RECURSIVE_DEPTH 16

/* Maximum depth of a link. Used in the 'link' command. */
#define MAX_LINK_DEPTH 16

/*
 * Bad window messages can be safely ignored now that ratpoison has become
 * stable enough. Comment this line if you wish to be notified about bad window
 * messages.
 */
#define IGNORE_BADWINDOW 1

/* This is the name of the first group that is created. */
#define DEFAULT_GROUP_NAME "default"

/* Maximum allowed history size */
#define MAX_HISTORY_SIZE 100

/* The default filename in which to store the history */
#define HISTORY_FILE ".ratpoison_history"

/* The name of the root keymap */
#define ROOT_KEYMAP "root"

/* The name of the top level keymap */
#define TOP_KEYMAP "top"

/* The default font */
#define DEFAULT_XFT_FONT "monospace:size=12"

#endif	/* !_ _RATPOISON_CONF_H */
