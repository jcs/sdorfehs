/* handles the handing out of and uniqueness of window numbers.
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

#include <stdlib.h>
#include <stdio.h>

#include "ratpoison.h"


/* A list of the numbers taken. */
static int *numbers_taken;

/* the number of numbers currently stored in the numbers_taken
   array. */
static int num_taken;

/* the size of the numbers_taken array. */
static int max_taken;

static int
number_is_taken (int n)
{
  int i;

  for (i=0; i<num_taken; i++)
    {
      if (numbers_taken[i] == n) return 1;
    }
  return 0;
}

/* returns index into numbers_taken that can be used. */
static int
find_empty_cell ()
{
  int i;

  for (i=0; i<num_taken; i++)
    {
      if (numbers_taken[i] == -1) return i;
    }

  /* no vacant ones, so grow the array. */
  if (num_taken >= max_taken)
    {
      max_taken *= 2;
      numbers_taken = realloc (numbers_taken, sizeof (int) * max_taken);
      if (numbers_taken == NULL)
	{
	  PRINT_ERROR ("Out of memory\n");
	  exit (EXIT_FAILURE);
	}
    }
  num_taken++;

  return num_taken-1;
}

static int
add_to_list (int n)
{
  if (number_is_taken (n)) return 0; /* failed. */
  
  numbers_taken[find_empty_cell()] = n;
  return 1; /* success! */
}

/* returns a unique number that can be used as the window number in
   the program bar. */
int
get_unique_window_number ()
{
  int i;
  
  /* look for a unique number, and add it to the list of taken
     numbers. */
  i = 0;
  while (!add_to_list (i)) i++;

  return i;
}

/* When a window is destroyed, it gives back its window number with
   this function. */
void
return_window_number (int n)
{
  int i;

  for (i=0; i<num_taken; i++)
    {
      if (numbers_taken[i] == n) 
	{
	  numbers_taken[i] = -1;
	  return;
	}
    }
}


void
init_numbers ()
{
  max_taken = 10;
  num_taken = 0;

  numbers_taken = malloc (max_taken * sizeof (int));
  if (numbers_taken == NULL)
    {
      PRINT_ERROR ("Cannot allocate memory for numbers_taken.\n");
      exit (EXIT_FAILURE);
    }

}
