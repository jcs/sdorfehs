/*
 * Copyright (c) 2019 joshua stein <jcs@jcs.org>
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

#ifndef VSCREEN_H
#define VSCREEN_H

void init_vscreen(rp_vscreen *v, rp_screen *s);
void vscreen_del(rp_vscreen *v);
void vscreen_free(rp_vscreen *v);
int vscreens_resize(int n);

rp_vscreen *vscreens_find_vscreen_by_number(rp_screen *s, int n);

struct list_head *vscreen_copy_frameset(rp_vscreen *v);
void vscreen_restore_frameset(rp_vscreen *v, struct list_head *head);
void vscreen_free_nums(rp_vscreen *v);
void frameset_free(struct list_head *head);
rp_frame *vscreen_get_frame(rp_vscreen *v, int frame_num);
rp_frame *vscreen_find_frame_by_frame(rp_vscreen *v, rp_frame *f);

void set_current_vscreen(rp_vscreen *v);
void vscreen_move_window(rp_vscreen *v, rp_window *w);

#define rp_current_vscreen	(rp_current_screen->current_vscreen)

#endif
