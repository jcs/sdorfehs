/* functions for managing the program bar
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

#ifndef _BAR_H
#define _BAR_H

void update_window_names (screen_info *s);
int show_bar (screen_info *s);
int hide_bar (screen_info *s);
int bar_y (screen_info *s);
int bar_x (screen_info *s, int width);
void display_msg_in_bar (screen_info *s, char *msg, int hl_start, int hl_end);
#endif _BAR_H
