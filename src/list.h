/* functions for managing the window list 
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

#ifndef _LIST_H
#define _LIST_H

rp_window *add_to_window_list (screen_info *s, Window w);
void init_window_list ();
void remove_from_window_list (rp_window *w);
void next_window ();
void prev_window ();
void last_window ();
rp_window *find_window (Window w);
void maximize_current_window ();
void set_active_window (rp_window *rp_w);
void set_current_window (rp_window *win);
void goto_window_number (int n);
int goto_window_name (char *name);
rp_window *find_last_accessed_window ();
rp_window *find_window_by_number (int n);
#endif /* _LIST_H */
