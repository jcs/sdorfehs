#ifndef GROUP_H
#define GROUP_H

void init_groups ();
void group_add_window (rp_group *g, rp_window *w);
void group_free (rp_group *g);
rp_group *group_new (int number);

void group_del_window (rp_group *g, rp_window *win);
void groups_del_window (rp_window *win);

void group_map_window (rp_group *g, rp_window *win);
void groups_map_window (rp_window *win);

void group_unmap_window (rp_group *g, rp_window *win);
void groups_unmap_window (rp_window *win);

rp_window *group_prev_window (rp_group *g, rp_window *win);
rp_window *group_next_window (rp_group *g, rp_window *win);

rp_window *group_last_window (rp_group *g);

rp_group *group_prev_group ();
rp_group *group_next_group ();

rp_group *group_add_new_group ();

rp_window_elem *group_find_window (struct list_head *list, rp_window *win);
rp_window *group_find_window_by_number (rp_group *g, int num);

#endif
