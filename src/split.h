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

int num_frames (screen_info *s);
rp_window *set_frames_window (rp_window_frame *frame, rp_window *win);
void cleanup_frame (rp_window_frame *frame);
void maximize_all_windows_in_frame (rp_window_frame *frame);
void h_split_frame (rp_window_frame *frame, int pixels);
void v_split_frame (rp_window_frame *frame, int pixels);
void remove_all_splits ();
void resize_shrink_to_window (rp_window_frame *frame);
void resize_frame_horizontally (rp_window_frame *frame, int diff);
void resize_frame_vertically (rp_window_frame *frame, int diff);
void remove_frame (rp_window_frame *frame);
rp_window *find_window_for_frame (rp_window_frame *frame);
rp_window_frame *find_windows_frame (rp_window *win);
rp_window_frame *find_frame_next (rp_window_frame *frame);
rp_window_frame *find_frame_prev (rp_window_frame *frame);
rp_window *current_window ();
void init_frame_lists ();
void init_frame_list (screen_info *screen);
void set_active_frame (rp_window_frame *frame);
void blank_frame (rp_window_frame *frame);
void show_frame_indicator ();
void hide_frame_indicator ();

void show_frame_message (char *msg);

rp_window_frame *find_frame_right (rp_window_frame *frame);
rp_window_frame *find_frame_left (rp_window_frame *frame);
rp_window_frame *find_frame_down (rp_window_frame *frame);
rp_window_frame *find_frame_up (rp_window_frame *frame);
rp_window_frame *find_last_frame (screen_info *s);
rp_window_frame *find_frame_number (screen_info *s, int num);

rp_window_frame *current_frame ();

#endif
