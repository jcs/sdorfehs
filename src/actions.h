/* 
 * Copyright (C) 2000 Shawn Betts
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA */

/* Prototypes of all actions that can be performed with keystrokes. */

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
void show_clock (void *data);

extern rp_action key_actions[];
