/* manage.h
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
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

#ifndef _RATPOISON_MANAGE_H
#define _RATPOISON_MANAGE_H 1

#include "data.h"

void clear_unmanaged_list (void);
char *list_unmanaged_windows (void);
void add_unmanaged_window (char *name);
int unmanaged_window (Window w);
rp_screen* current_screen (void);
void scanwins(rp_screen *s);
void unmanage (rp_window *w);
int update_window_name (rp_window *win);
void update_normal_hints (rp_window *win);
void rename_current_window (void);
void send_configure (Window w, int x, int y, int width, int height, int border);
void set_state (rp_window *win, int state);
long get_state (rp_window *win);

int window_is_transient (rp_window *win);
void update_window_information (rp_window *win);
void map_window (rp_window *win);

void maximize (rp_window *win);
void force_maximize (rp_window *win);

void grab_top_level_keys (Window w);
void ungrab_top_level_keys (Window w);
void ungrab_keys_all_wins (void);
void grab_keys_all_wins (void);

void hide_window (rp_window *win);
void unhide_window (rp_window *win);
void unhide_all_windows (void);
void unhide_window_below (rp_window *win);
void withdraw_window (rp_window *win);
void hide_others (rp_window *win);

#endif /* ! _RATPOISION_MANAGE_H */
