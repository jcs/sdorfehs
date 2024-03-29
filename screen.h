/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
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

#ifndef SCREEN_H
#define SCREEN_H

int screen_bottom(rp_screen *s);
int screen_top(rp_screen *s);
int screen_right(rp_screen *s);
int screen_left(rp_screen *s);
int screen_height(rp_screen *s);
int screen_width(rp_screen *s);

rp_screen *find_screen(Window w);
rp_screen *find_screen_by_attr(XWindowAttributes w);

void init_screens(void);
void activate_screen(rp_screen *s);
void deactivate_screen(rp_screen *s);

int is_rp_window(Window w);
int is_a_root_window(unsigned int w);

char *screen_dump(rp_screen *screen);

void screen_update(rp_screen *s, int left, int top, int width, int height);
void screen_update_frames(rp_screen *s);
void screen_update_workarea(rp_screen *s);

int screen_count(void);
rp_screen *screen_primary(void);
rp_screen *screen_next(void);
rp_screen *screen_prev(void);

rp_screen *screen_number(int number);

void screen_sort(void);

rp_screen *screen_add(int rr_output);
void screen_del(rp_screen *s);
void screen_free(rp_screen *s);
void screen_free_final(void);

#endif
