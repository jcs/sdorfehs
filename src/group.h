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

#ifndef GROUP_H
#define GROUP_H

void init_groups (void);
void free_groups (void);

void group_add_window (rp_group *g, rp_window *w);
void group_resort_window (rp_group *g, rp_window_elem *w);
void group_free (rp_group *g);
rp_group *group_new (int number, char *name);
int group_delete_group (rp_group *g);

void group_del_window (rp_group *g, rp_window *win);
void groups_del_window (rp_window *win);

void group_map_window (rp_group *g, rp_window *win);
void groups_map_window (rp_window *win);

void group_unmap_window (rp_group *g, rp_window *win);
void groups_unmap_window (rp_window *win);

struct numset *group_get_numset (void);
void get_group_list (char *delim, struct sbuf *buffer, int *mark_start,
                int *mark_end);

rp_window *group_prev_window (rp_group *g, rp_window *win);
rp_window *group_next_window (rp_group *g, rp_window *win);
rp_group *groups_find_group_by_name (char *s, int exact_match);
rp_group *groups_find_group_by_number (int n);
rp_group *groups_find_group_by_window (rp_window *win);
rp_group *groups_find_group_by_group (rp_group *g);

rp_window *group_last_window (rp_group *g, rp_screen *screen);

rp_group *group_prev_group (void);
rp_group *group_next_group (void);
rp_group *group_last_group (void);

void group_resort_group (rp_group *g);

rp_group *group_add_new_group (char *name);
void group_rename (rp_group *g, char *name);

rp_window_elem *group_find_window (struct list_head *list, rp_window *win);
rp_window_elem *group_find_window_by_number (rp_group *g, int num);

void group_move_window (rp_group *to, rp_window *win);
void groups_merge (rp_group *from, rp_group *to);

void set_current_group (rp_group *g);

rp_window *group_last_window_by_class (rp_group *g, char *class);
rp_window *group_last_window_by_class_complement (rp_group *g, char *class);
#endif
