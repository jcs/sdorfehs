/* Config file for ratpoison. Edit these values and recompile. 
 * Copyright (C) 2000, 2001 Shawn Betts
 *
 * This file is part of ratpoison.
 *
 * ratpoison is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * ratpoison is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 */

#ifndef _RATPOISON_CONF_H
#define _RATPOISON_CONF_H 1

#include "data.h"
#include "actions.h"

#define KEY_PREFIX      XK_t
#define MODIFIER_PREFIX ControlMask

/* This is the abort key when typing input. */
#define INPUT_ABORT_KEY      XK_g
#define INPUT_ABORT_MODIFIER ControlMask

/* Treat windows with maxsize hints as if they were a transient window
   (don't hide the windows underneath, and center them) */
#define MAXSIZE_WINDOWS_ARE_TRANSIENTS

/* Pressing a key sends the mouse to the bottom right corner. This
   doesn't work very well yet. */
/* #define HIDE_MOUSE  */

/* If for some sick reason you don't want ratpoison to manage a
   window, put its name in this list. These windows get drawn but
   ratpoison won't have any knowledge of them and you won't be able to
   jump to them or give them keyboard focus. This has been added
   mostly for use with hand-helds. */
#define UNMANAGED_WINDOW_LIST "xapm","xclock","xscribble"

#endif /* !_ _RATPOISON_CONF_H */
