/* Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
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

/* A hook is simply a list of strings that get passed to command() in
   sequence. */

#include "ratpoison.h"

#include <string.h>

void
hook_add (struct list_head *hook, struct sbuf *s)
{
  struct sbuf *cur;

  /* Check if it's in the list already. */
  list_for_each_entry (cur, hook, node)
    {
      if (!strcmp (sbuf_get (cur), sbuf_get (s)))
        {
          sbuf_free (s);
          return;
        }
    }

  /* It's not in the list, so add it. */
  list_add_tail (&s->node, hook);
}

void
hook_remove (struct list_head *hook, struct sbuf *s)
{
  struct list_head *tmp, *iter;
  struct sbuf *cur;

  /* If it's in the list, delete it. */
  list_for_each_safe_entry (cur, iter, tmp, hook, node)
    {
      if (!strcmp (sbuf_get (cur), sbuf_get (s)))
        {
          list_del (&cur->node);
          sbuf_free (cur);
        }
    }
}

void
hook_run (struct list_head *hook)
{
  struct sbuf *cur;
  cmdret *result;

  list_for_each_entry (cur, hook, node)
    {
      result = command (1, sbuf_get (cur));
      if (result)
        {
          if (result->output)
            message (result->output);
          cmdret_free (result);
        }
    }
}

struct list_head *
hook_lookup (char *s)
{
  struct rp_hook_db_entry *entry;

  for (entry = rp_hook_db; entry->name; entry++)
    {
      if (!strcmp (s, entry->name))
        {
          return entry->hook;
        }
    }

  return NULL;
}
