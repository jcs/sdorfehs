/* manage.h 
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

#ifndef _RATPOISON_MANAGE_H
#define _RATPOISON_MANAGE_H 1

#include "data.h"

int unmanaged_window (Window w);
screen_info* current_screen ();
void scanwins(screen_info *s);
void unmanage (rp_window *w);
int update_window_name (rp_window *win);
void update_normal_hints (rp_window *win);
void rename_current_window ();
void send_configure (rp_window *win);
void set_state (rp_window *win, int state);

void update_window_information (rp_window *win);
void map_window (rp_window *win);

void maximize (rp_window *win);
void force_maximize (rp_window *win);

void grab_prefix_key (Window w);
void ungrab_prefix_key (Window w);

void hide_window (rp_window *win);
void unhide_window (rp_window *win);
void unhide_window_below (rp_window *win);
void withdraw_window (rp_window *win);
void hide_others (rp_window *win);

#endif /* ! _RATPOISION_MANAGE_H */
