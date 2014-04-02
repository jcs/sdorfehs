/* Functions for handling string buffers.
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

#include <string.h>

#include "ratpoison.h"
#include "sbuf.h"

struct sbuf *
sbuf_new (size_t initsz)
{
  struct sbuf *b = xmalloc (sizeof (struct sbuf));

  if (initsz < 1)
    initsz = 1;

  b->data = xmalloc (initsz);
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
      free (b->data);
      free (b);
    }
}

/* Free the structure but return the string. */
char *
sbuf_free_struct (struct sbuf *b)
{
  if (b != NULL)
    {
      char *tmp;
      tmp = b->data;
      free (b);
      return tmp;
    }

  return NULL;
}

char *
sbuf_nconcat (struct sbuf *b, const char *str, int len)
{
  size_t minsz = b->len + len + 1;

  if (b->maxsz < minsz)
    {
      b->data = xrealloc (b->data, minsz);
      b->maxsz = minsz;
    }

  memcpy (b->data + b->len, str, minsz - b->len - 1);
  b->len = minsz - 1;
  *(b->data + b->len) = 0;

  return b->data;
}


char *
sbuf_concat (struct sbuf *b, const char *str)
{
  return sbuf_nconcat (b, str, strlen (str));
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

char *
sbuf_printf (struct sbuf *b, char *fmt, ...)
{
  va_list ap;

  free (b->data);

  va_start (ap, fmt);
  b->data = xvsprintf (fmt, ap);
  va_end (ap);

  b->len = strlen (b->data);
  b->maxsz = b->len + 1;

  return b->data;
}

char *
sbuf_printf_concat (struct sbuf *b, char *fmt, ...)
{
  char *buffer;
  va_list ap;

  va_start (ap, fmt);
  buffer = xvsprintf (fmt, ap);
  va_end (ap);

  sbuf_concat (b, buffer);
  free (buffer);

  return b->data;
}

void
sbuf_chop (struct sbuf *b)
{
  if (b->len)
    {
      b->data[--(b->len)] = '\0';
    }
}
