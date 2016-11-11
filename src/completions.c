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

#define _GNU_SOURCE
#include <string.h>

#include "ratpoison.h"
#include "completions.h"

rp_completions *
completions_new (completion_fn list_fn, enum completion_styles style)
{
  rp_completions *c;

  c = xmalloc (sizeof(rp_completions));

  INIT_LIST_HEAD (&c->completion_list);
  c->complete_fn = list_fn;
  c->last_match = NULL;
  c->partial = NULL;
  c->virgin = 1;
  c->style = style;

  return c;
}

void
completions_free (rp_completions *c)
{
  struct sbuf *cur;
  struct list_head *tmp, *iter;

  /* Clear our list */
  list_for_each_safe_entry (cur, iter, tmp, &c->completion_list, node)
    {
      list_del (&cur->node);
      sbuf_free (cur);
    }

  /* Free the partial string. */
  free (c->partial);

  free (c);
}

static void
completions_assign (rp_completions *c, struct list_head *new_list)
{
  struct sbuf *cur;
  struct list_head *tmp, *iter;

  /* Clear our list */
  list_for_each_safe_entry (cur, iter, tmp, &c->completion_list, node)
    {
      list_del (&cur->node);
      sbuf_free (cur);
    }

  /* splice the list into completion_list. Note that we SHOULDN'T free
     new_list, because they share the same memory. */
  INIT_LIST_HEAD (&c->completion_list);
  list_splice (new_list, &c->completion_list);

  list_first (c->last_match, &c->completion_list, node);
}

static void
completions_update (rp_completions *c, char *partial)
{
  struct list_head *new_list;

  new_list = c->complete_fn (partial);

  c->virgin = 0;
  free (c->partial);
  c->partial = xstrdup (partial);

  completions_assign (c, new_list);

  /* Free the head structure for our list. */
  free (new_list);
}


/* Return true if completion is an alternative for partial string,
   given the style used. */
static int
completions_match(rp_completions *c, char *completion, char *partial)
{
  int match = 0;

  switch (c->style)
    {
    case BASIC:
      match = str_comp (completion, partial, strlen(partial));
      break;
    case SUBSTRING:
      match = (strcasestr (completion, partial) != NULL);
      break;
    }

  return match;
}

static char *
completions_prev_match (rp_completions *c)
{
  struct sbuf *cur;

  /* search forward from our last match through the list looking for
     another match. */
  for (cur = list_prev_entry (c->last_match, &c->completion_list, node);
       cur != c->last_match;
       cur = list_prev_entry (cur, &c->completion_list, node))
    {
      if (completions_match (c, sbuf_get (cur), c->partial))
        {
          /* We found a match so update our last_match pointer and
             return the string. */
          c->last_match = cur;
          return sbuf_get (cur);
        }
    }

  return NULL;
}

static char *
completions_next_match (rp_completions *c)
{
  struct sbuf *cur;

  /* search forward from our last match through the list looking for
     another match. */
  for (cur = list_next_entry (c->last_match, &c->completion_list, node);
       cur != c->last_match;
       cur = list_next_entry (cur, &c->completion_list, node))
    {
      if (completions_match (c, sbuf_get (cur), c->partial))
        {
          /* We found a match so update our last_match pointer and
             return the string. */
          c->last_match = cur;
          return sbuf_get (cur);
        }
    }

  return NULL;
}

/* Return a completed string that starts with partial. */
char *
completions_complete (rp_completions *c, char *partial, int direction)
{
  if (c->virgin)
    {
      completions_update (c, partial);

      /* Since it's never been completed on and c->last_match points
         to the first element of the list which may be a match. So
         check it. FIXME: This is a bit of a hack. */
      if (c->last_match == NULL)
        return NULL;

      /* c->last_match contains the first match in the forward
         direction. So if we're looking for the previous match, then
         check the previous element from last_match. */
      if (direction == COMPLETION_PREVIOUS)
        c->last_match = list_prev_entry (c->last_match, &c->completion_list, node);

      /* Now check if last_match is a match for partial. */
      if (completions_match (c, sbuf_get (c->last_match), c->partial))
        return sbuf_get (c->last_match);
    }

  if (c->last_match == NULL)
    return NULL;

  /* Depending on the direction, find our "next" match. */
  if (direction == COMPLETION_NEXT)
    return completions_next_match (c);

  /* Otherwise get the previous match */
  return completions_prev_match (c);
}
