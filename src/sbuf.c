/* Functions for handling string buffers.
 * Copyright (C) 2000, 2001 Shawn Betts
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

#include "ratpoison.h"
#include "sbuf.h"

/* ------------------------------------ move to separate file */
#include <stdlib.h>
#include <stdio.h>

void
fatal (const char *msg)
{
  fprintf (stderr, "%s", msg);
  abort ();
}

void *
xmalloc (size_t size)
{
  register void *value = malloc (size);
  if (value == 0)
    fatal ("virtual memory exhausted");
  return value;
}

void *
xrealloc (void *ptr, size_t size)
{
  register void *value = realloc (ptr, size);
  if (value == 0)
    fatal ("Virtual memory exhausted");
  PRINT_DEBUG("realloc: %d\n", size);
  return value;
}
/*------------------------------------------------------------*/

struct sbuf *
sbuf_new (size_t initsz)
{
  struct sbuf *b = (struct sbuf*) xmalloc (sizeof (struct sbuf));

  if (initsz < 1) 
    initsz = 1;

  b->data = (char*) xmalloc (initsz);
  b->maxsz = initsz;

  b->data[0] = '\0';
  b->len = 0;

  return b;
}

void
sbuf_free (struct sbuf *b)
{
  if (b != NULL)
    {
      if (b->data != NULL)
	free (b->data);
      
      free (b);
    }
}

char *
sbuf_concat (struct sbuf *b, const char *str)
{
  size_t minsz = b->len + strlen (str) + 1;

  if (b->maxsz < minsz)
    {
      b->data = (char*) xrealloc (b->data, minsz);
      b->maxsz = minsz;
    }
  
  memcpy (b->data + b->len, str, minsz - b->len - 1);
  b->len = minsz - 1;
  *(b->data + b->len) = 0;

  return b->data;
}

char *
sbuf_copy (struct sbuf *b, const char *str)
{
  b->len = 0;
  return sbuf_concat (b, str);
}

char *
sbuf_clear (struct sbuf *b)
{
  b->len = 0;
  b->data[0] = '\0';
  return b->data;
}

char *
sbuf_get (struct sbuf *b)
{
  return b->data;
}
