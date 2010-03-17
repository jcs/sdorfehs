/* Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
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

int num_frames (rp_screen *s);
rp_window *set_frames_window (rp_frame *frame, rp_window *win);
void cleanup_frame (rp_frame *frame);
void maximize_all_windows_in_frame (rp_frame *frame);
void h_split_frame (rp_frame *frame, int pixels);
void v_split_frame (rp_frame *frame, int pixels);
void remove_all_splits (void);
void resize_shrink_to_window (rp_frame *frame);
void resize_frame_horizontally (rp_frame *frame, int diff);
void resize_frame_vertically (rp_frame *frame, int diff);
void remove_frame (rp_frame *frame);
rp_window *find_window_for_frame (rp_frame *frame);
rp_frame *find_windows_frame (rp_window *win);
rp_frame *find_frame_next (rp_frame *frame);
rp_frame *find_frame_prev (rp_frame *frame);
rp_window *current_window (void);
void init_frame_lists (void);
void init_frame_list (rp_screen *screen);
void set_active_frame (rp_frame *frame, int force_indicator);
void exchange_with_frame (rp_frame *cur, rp_frame *frame);
void blank_frame (rp_frame *frame);
void show_frame_indicator (int force);
void hide_frame_indicator (void);

void show_frame_message (char *msg);

rp_frame *find_frame_right (rp_frame *frame);
rp_frame *find_frame_left (rp_frame *frame);
rp_frame *find_frame_down (rp_frame *frame);
rp_frame *find_frame_up (rp_frame *frame);
rp_frame *find_last_frame (void);
rp_frame * find_frame_number (int num);

rp_frame *current_frame (void);

#endif
