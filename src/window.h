/* functions for managing the window list 
 * Copyright (C) 2000, 2001, 2002, 2003 Shawn Betts
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

#ifndef _RATPOISON_LIST_H
#define _RATPOISON_LIST_H 1

#include "sbuf.h"

void free_window (rp_window *w);
rp_window *add_to_window_list (rp_screen *s, Window w);
void last_window ();
rp_window *find_window_in_list (Window w, struct list_head *list);
rp_window *find_window (Window w);
void maximize_current_window ();
void give_window_focus (rp_window *win, rp_window *last_win);
void set_active_window (rp_window *win);
void goto_window (rp_window *win);
void set_current_window (rp_window *win);
void update_window_gravity (rp_window *win);
char *window_name (rp_window *win);

#if 0
void unhide_transient_for (rp_window *win);
void hide_transient_for (rp_window *win);
void hide_transient_for_between (rp_window *win, rp_window *last);
int is_transient_ancestor (rp_window *win, rp_window *transient_for);
#endif

/* int goto_window_name (char *name); */
rp_window *find_window_other ();
rp_window *find_window_by_number (int n);
rp_window *find_window_name (char *name);
rp_window *find_window_prev (rp_window *w);
rp_window *find_window_prev_with_frame (rp_window *w);
rp_window *find_window_next (rp_window *w);
rp_window *find_window_next_with_frame (rp_window *w);
rp_window *find_window_number (int n);
void sort_window_list_by_number ();

void insert_into_list (rp_window *win, struct list_head *list);

void print_window_information (rp_window *win);
void get_window_list (char *fmt, char *delim, struct sbuf *buffer, 
		      int *mark_start, int *mark_end);
void init_window_stuff ();
void free_window_stuff ();

rp_frame *win_get_frame (rp_window *win);

#endif /* ! _RATPOISON_LIST_H */
