/* Copyright (C) 2000, 2001 Shawn Betts
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
 *
 */

#ifndef SPLIT_H
#define SPLIT_H

void h_split_frame (rp_window_frame *frame);
void v_split_frame (rp_window_frame *frame);
void remove_all_splits ();
void remove_frame (rp_window_frame *frame);
rp_window *find_window_for_frame (rp_window_frame *frame);
rp_window_frame *find_windows_frame (rp_window *win);
rp_window_frame *find_frame_next (rp_window_frame *frame);
rp_window_frame *find_frame_prev (rp_window_frame *frame);
rp_window *current_window ();
void init_frame_list ();
void set_active_frame (rp_window_frame *frame);
void show_frame_indicator ();
void hide_frame_indicator ();

#endif
