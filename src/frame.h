/* Copyright (C) 2000-2003 Shawn Betts
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

#ifndef FRAME_H
#define FRAME_H

void frame_move_down (rp_window_frame *frame, int amount);
void frame_move_up (rp_window_frame *frame, int amount);
void frame_move_right (rp_window_frame *frame, int amount);
void frame_move_left (rp_window_frame *frame, int amount);
void frame_resize_down (rp_window_frame *frame, int amount);
void frame_resize_up (rp_window_frame *frame, int amount);
void frame_resize_right (rp_window_frame *frame, int amount);
void frame_resize_left (rp_window_frame *frame, int amount);
int frame_height(rp_window_frame *frame);
int frame_width(rp_window_frame *frame);
int frame_bottom (rp_window_frame *frame);
int frame_right (rp_window_frame *frame);
int frame_top (rp_window_frame *frame);
int frame_left (rp_window_frame *frame);

rp_window_frame *frame_new (screen_info *s);
void frame_free (screen_info *s, rp_window_frame *f);
rp_window_frame *frame_copy (rp_window_frame *frame);
char *frame_dump (rp_window_frame *frame);
rp_window_frame *frame_read (char *str);

#endif
