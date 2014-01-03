/* handles the handing out of and uniqueness of window numbers.
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

#include <stdlib.h>
#include <stdio.h>

#include "ratpoison.h"

/* Initialize a numset structure. */
static void
numset_init (struct numset *ns)
{
  ns->max_taken = 10;
  ns->num_taken = 0;

  ns->numbers_taken = xmalloc (ns->max_taken * sizeof (int));
}

static int
numset_num_is_taken (struct numset *ns, int n)
{
  int i;

  for (i=0; i<ns->num_taken; i++)
    {
      if (ns->numbers_taken[i] == n) return 1;
    }
  return 0;
}

/* returns index into numbers_taken that can be used. */
static int
numset_find_empty_cell (struct numset *ns)
{
  int i;

  for (i=0; i<ns->num_taken; i++)
    {
      if (ns->numbers_taken[i] == -1) return i;
    }

  /* no vacant ones, so grow the array. */
  if (ns->num_taken >= ns->max_taken)
    {
      ns->max_taken *= 2;
      ns->numbers_taken = xrealloc (ns->numbers_taken, sizeof (int) * ns->max_taken);
    }
  ns->num_taken++;

  return ns->num_taken-1;
}

int
numset_add_num (struct numset *ns, int n)
{
  int ec;

  PRINT_DEBUG(("ns=%p add_num %d\n", ns, n));

  if (numset_num_is_taken (ns, n))
    return 0; /* failed. */
  /* numset_find_empty_cell calls realloc on numbers_taken. So store
     the ret val in ec then use ec as an index into the array. */
  ec = numset_find_empty_cell(ns);
  ns->numbers_taken[ec] = n;
  return 1; /* success! */
}

/* returns a unique number that can be used as the window number in
   the program bar. */
int
numset_request (struct numset *ns)
{
  int i;

  /* look for a unique number, and add it to the list of taken
     numbers. */
  i = 0;
  while (!numset_add_num (ns, i)) i++;

  PRINT_DEBUG(("ns=%p request got %d\n", ns, i));

  return i;
}

/* When a window is destroyed, it gives back its window number with
   this function. */
void
numset_release (struct numset *ns, int n)
{
  int i;

  PRINT_DEBUG(("ns=%p release %d\n", ns, n));

  if (n < 0)
    PRINT_ERROR(("ns=%p Attempt to release %d!\n", ns, n));

  for (i=0; i<ns->num_taken; i++)
    {
      if (ns->numbers_taken[i] == n)
        {
          ns->numbers_taken[i] = -1;
          return;
        }
    }
}

/* Create a new numset and return a pointer to it. */
struct numset *
numset_new (void)
{
  struct numset *ns;

  ns = xmalloc (sizeof (struct numset));
  numset_init (ns);
  return ns;
}

/* Free a numset structure and it's internal data. */
void
numset_free (struct numset *ns)
{
  free (ns->numbers_taken);
  free (ns);
}

void
numset_clear (struct numset *ns)
{
  ns->num_taken = 0;
}
