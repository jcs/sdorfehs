/*
 * manage.h
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

#ifndef _SDORFEHS_MANAGE_H
#define _SDORFEHS_MANAGE_H 1

#include "data.h"

void clear_unmanaged_list(void);
char *list_unmanaged_windows(void);
void add_unmanaged_window(char *name);
int unmanaged_window(Window w);
void scanwins(void);
void unmanage(rp_window *w);
int update_window_name(rp_window *win);
void update_normal_hints(rp_window *win);
void rename_current_window(void);
void set_state(rp_window *win, int state);
long get_state(rp_window *win);
void check_state(rp_window *win);

int window_is_transient(rp_window *win);
Atom get_net_wm_window_type(rp_window *win);
int is_unmanaged_window_type(Window win);
void update_window_information(rp_window *win);
void map_window(rp_window *win);

void maximize(rp_window *win);
void force_maximize(rp_window *win);

void grab_top_level_keys(Window w);
void ungrab_top_level_keys(Window w);
void ungrab_keys_all_wins(void);
void grab_keys_all_wins(void);

void hide_window(rp_window *win);
void unhide_window(rp_window *win);
void unhide_all_windows(void);
void withdraw_window(rp_window *win);
void hide_others(rp_window *win);
void hide_vscreen_windows(rp_vscreen *v);
void raise_utility_windows(void);

#endif	/* ! _SDORFEHS_MANAGE_H */
