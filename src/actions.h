/* actions.h -- prototypes of all actions that can be performed with
   keystrokes */

void switch_to(void *which);
void bye(void *dummy);
void generate_prefix (void *data);
void abort_keypress (void *data);
void goto_window_9 (void *data);
void goto_window_8 (void *data);
void goto_window_7 (void *data);
void goto_window_6 (void *data);
void goto_window_5 (void *data);
void goto_window_4 (void *data);
void goto_window_3 (void *data);
void goto_window_2 (void *data);
void goto_window_1 (void *data);
void goto_window_0 (void *data);
void spawn(void *data);
void execute_command (void *data);
void kill_window (void *data);
void delete_window (void *data);
void rename_current_window (void *data);
void goto_win_by_name (void *data);
void last_window (void *data);
void next_window (void *data);
void prev_window (void *data);
void toggle_bar (void *data);
void maximize (void *data);

extern rp_action key_actions[];
