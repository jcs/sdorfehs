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

#ifndef SCREEN_H
#define SCREEN_H

int screen_bottom (rp_screen *s);
int screen_top (rp_screen *s);
int screen_right (rp_screen *s);
int screen_left (rp_screen *s);
int screen_height (rp_screen *s);
int screen_width (rp_screen *s);

struct list_head *screen_copy_frameset (rp_screen *s);
void screen_restore_frameset (rp_screen *s, struct list_head *head);
void screen_free_nums (rp_screen *s);
void frameset_free (struct list_head *head);
rp_frame *screen_get_frame (rp_screen *s, int frame_num);
rp_frame *screen_find_frame_by_frame (rp_screen *s, rp_frame *f);

void init_screens (int screen_arg, int screen_num);
void activate_screen (rp_screen *s);
void deactivate_screen (rp_screen *s);

int is_rp_window_for_screen (Window w, rp_screen *s);
int is_a_root_window (unsigned int w);

char *screen_dump (rp_screen *screen);

void screen_update (rp_screen *s, int width, int height);
#endif
