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

rp_vscreen *screen_find_vscreen_by_number(rp_screen *s, int n);
rp_vscreen *screen_find_vscreen_by_name(rp_screen *s, char *name, int exact_match);

struct list_head *vscreen_copy_frameset(rp_vscreen *v);
void vscreen_restore_frameset(rp_vscreen *v, struct list_head *head);
void vscreen_free_nums(rp_vscreen *v);
void frameset_free(struct list_head *head);
rp_frame *vscreen_get_frame(rp_vscreen *v, int frame_num);
rp_frame *vscreen_find_frame_by_frame(rp_vscreen *v, rp_frame *f);

void set_current_vscreen(rp_vscreen *v);
void vscreen_move_window(rp_vscreen *v, rp_window *w);


void vscreen_add_window(rp_vscreen *v, rp_window *w);
void vscreen_resort_window(rp_vscreen *v, rp_window_elem *w);
void vscreen_insert_window(struct list_head *h, rp_window_elem *w);

void vscreen_del_window(rp_vscreen *v, rp_window *win);

void vscreen_map_window(rp_vscreen *v, rp_window *win);

void vscreen_unmap_window(rp_vscreen *v, rp_window *win);

struct numset *vscreen_get_numset(rp_vscreen *v);
void get_vscreen_list(rp_screen *s, char *delim, struct sbuf *buffer,
    int *mark_start, int *mark_end);

rp_window *vscreen_prev_window(rp_vscreen *v, rp_window *win);
rp_window *vscreen_next_window(rp_vscreen *v, rp_window *win);

rp_window *vscreen_last_window(rp_vscreen *v);

rp_vscreen *vscreen_prev_vscreen(rp_vscreen *v);
rp_vscreen *vscreen_next_vscreen(rp_vscreen *v);
rp_vscreen *screen_last_vscreen(rp_screen *screen);

void vscreen_rename(rp_vscreen *v, char *name);

rp_window_elem *vscreen_find_window(struct list_head *list, rp_window *win);
rp_window_elem *vscreen_find_window_by_number(rp_vscreen *g, int num);

void vscreen_move_window(rp_vscreen *to, rp_window *win);
void vscreens_merge(rp_vscreen *from, rp_vscreen *to);

void set_current_vscreen(rp_vscreen *v);

rp_window *vscreen_last_window_by_class(rp_vscreen *v, char *class);
rp_window *vscreen_last_window_by_class_complement(rp_vscreen *v, char *class);

#define rp_current_vscreen	(rp_current_screen->current_vscreen)

#endif
