/* functions for managing the window list */

#ifndef _LIST_H
#define _LIST_H

rp_window *add_to_window_list (screen_info *s, Window w);
void init_window_list ();
void remove_from_window_list (rp_window *w);
void next_window ();
void prev_window ();
void last_window ();
rp_window *find_window (Window w);
void maximize_current_window ();
void set_active_window (rp_window *rp_w);
void set_current_window (rp_window *win);
void goto_window_number (int n);
#endif /* _LIST_H */
