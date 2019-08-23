/*
 * Function prototypes.
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

#ifndef _SDORFEHS_NUMBER_H
#define _SDORFEHS_NUMBER_H 1

struct numset;

struct numset *numset_new(void);
void numset_free(struct numset *ns);
void numset_release(struct numset *ns, int n);
int numset_request(struct numset *ns);
int numset_add_num(struct numset *ns, int n);

#endif	/* ! _SDORFEHS_NUMBER_H */
