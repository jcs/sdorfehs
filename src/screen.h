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

#ifndef SCREEN_H
#define SCREEN_H

int screen_bottom (screen_info *s);
int screen_top (screen_info *s);
int screen_right (screen_info *s);
int screen_left (screen_info *s);
int screen_height (screen_info *s);
int screen_width (screen_info *s);

struct list_head *screen_copy_frameset (screen_info *s);
void screen_restore_frameset (screen_info *s, struct list_head *head);
void frameset_free (struct list_head *head);
rp_window_frame *screen_get_frame (screen_info *s, int frame_num);

#endif
