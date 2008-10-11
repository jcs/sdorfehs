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
 */

#ifndef FRAME_H
#define FRAME_H

void frame_move_down (rp_frame *frame, int amount);
void frame_move_up (rp_frame *frame, int amount);
void frame_move_right (rp_frame *frame, int amount);
void frame_move_left (rp_frame *frame, int amount);
void frame_resize_down (rp_frame *frame, int amount);
void frame_resize_up (rp_frame *frame, int amount);
void frame_resize_right (rp_frame *frame, int amount);
void frame_resize_left (rp_frame *frame, int amount);
int frame_height(rp_frame *frame);
int frame_width(rp_frame *frame);
int frame_bottom (rp_frame *frame);
int frame_right (rp_frame *frame);
int frame_top (rp_frame *frame);
int frame_left (rp_frame *frame);

rp_frame *frame_new (rp_screen *s);
void frame_free (rp_screen *s, rp_frame *f);
rp_frame *frame_copy (rp_frame *frame);
char *frame_dump (rp_frame *frame, rp_screen *screen);
rp_frame *frame_read (char *str, rp_screen *screen);

rp_screen *frames_screen (rp_frame *);

#endif
