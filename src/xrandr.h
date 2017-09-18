/*
 * Copyright (C) 2016 Mathieu OTHACEHE
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

#ifndef XRANDR_H
#define XRANDR_H

#include "ratpoison.h"

void init_xrandr(void);
int xrandr_query_screen(int **outputs);
int xrandr_is_primary (rp_screen *screen);
void xrandr_fill_screen(int rr_output, rp_screen *screen);
void xrandr_notify(XEvent *ev);

#endif
