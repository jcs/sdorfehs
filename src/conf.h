/* Config file for ratpoison. Edit these values and recompile. 
 *  
 * Copyright (C) 2000 Shawn Betts
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA */

#include "data.h"
#include "actions.h"

#define KEY_PREFIX      't'
#define MODIFIER_PREFIX ControlMask

#define BAR_FG_COLOR    "Gray60"
#define BAR_BG_COLOR    "Lightgreen"
#define BAR_BOLD_COLOR  "Black" /* To indicate the current window */

#define FONT_NAME       "fixed"	/* The font you wish to use */
#define BAR_PADDING     3	/* The amount of padding on the top and bottom of the program bar  */
#define BAR_LOCATION    3	/* 0=bottom-left 1=top-left 2=bottom-right 3=top-right */
#define BAR_TIMEOUT     5	/* Number of seconds before the progam bar autohides 0=don't autohide */

