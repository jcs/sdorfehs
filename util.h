/*
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

#ifndef UTIL_H
#define UTIL_H

#ifndef __dead
#define __dead	__attribute__((__noreturn__))
#endif

__dead void fatal(const char *msg);
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);
char *xvsprintf(char *fmt, va_list ap);
char *xsprintf(char *fmt,...);
char *strtok_ws(char *s);
int str_comp(char *s1, char *s2, size_t len);

#endif
