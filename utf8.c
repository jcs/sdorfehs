/*
 * Copyright (C) 2017
 * Jeremie Courreges-Anglas <jca@wxcvcbn.org>
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

#include "sdorfehs.h"

#include <langinfo.h>

int
isu8char(char c)
{
	return (c) & 0xC0;
}

int
isu8start(char c)
{
	return ((c) & 0xC0) == 0xC0;
}

int
isu8cont(char c)
{
	return ((c) & 0xC0) == 0x80;
}
