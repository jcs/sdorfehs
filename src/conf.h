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

#define KEY_PREFIX      't'
#define MODIFIER_PREFIX ControlMask

/* Pressing a key sends the mouse to the bottom right corner. This
   doesn't work very well yet. */
//#define HIDE_MOUSE 

/* Quit ratpoison when there are no more managed windows. */
//#define AUTO_CLOSE		

/* The minimum size of the input window */
#define INPUT_WINDOW_SIZE 200

#define BAR_FG_COLOR    "black"
#define BAR_BG_COLOR    "white"
#define FONT_NAME       "9x15bold"

/* The amount of padding on the top and bottom of the message bar. */
#define BAR_Y_PADDING   0       

/* The amount of padding on the left and right of the message bar. */
#define BAR_X_PADDING   0       

/* 0=bottom-left 1=top-left 2=bottom-right 3=top-right */
#define BAR_LOCATION    3 

/* Number of seconds before the progam bar autohides.  Setting it to 0
   disable autohide. */
#define BAR_TIMEOUT     5 

/* space not to be taken up around managed windows */
#define PADDING_LEFT 	0 
#define PADDING_TOP 	0
#define PADDING_RIGHT 	0
#define PADDING_BOTTOM 	0

/* If for some sick reason you don't want ratpoison to manage a
   window, put its name in this list. These windows get drawn but
   ratpoison won't have any knowledge of them and you won't be able to
   jump to them or give them keyboard focus. This has been added
   mostly for use with hand-helds. */
#define UNMANAGED_WINDOW_LIST "xapm","xclock","xscribble"

#endif /* !_ _RATPOISON_CONF_H */
