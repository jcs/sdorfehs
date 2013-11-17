/* Function prototypes for handling string buffers.
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
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

#ifndef _RATPOISON_SBUF_H
#define _RATPOISON_SBUF_H 1

#include <stdlib.h>

struct sbuf
{
  char *data;
  size_t len;
  size_t maxsz;

  /* sbuf can exist in a list. */
  struct list_head node;
};

struct sbuf *sbuf_new (size_t initsz);
void sbuf_free (struct sbuf *b);
char *sbuf_free_struct (struct sbuf *b);
char *sbuf_concat (struct sbuf *b, const char *str);
char *sbuf_nconcat (struct sbuf *b, const char *str, int len);
char *sbuf_copy (struct sbuf *b, const char *str);
char *sbuf_clear (struct sbuf *b);
char *sbuf_get (struct sbuf *b);
char *sbuf_printf (struct sbuf *b, char *fmt, ...);
char *sbuf_printf_concat (struct sbuf *b, char *fmt, ...);
void  sbuf_chop (struct sbuf *b);

#endif /* ! _RATPOISON_SBUF_H */
