/* Copyright (C) 2000-2004 Shawn Betts <sabetts@vcn.bc.ca>
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

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "ratpoison.h"

#ifdef HAVE_HISTORY

#include "readline/history.h"

static char *
get_history_filename ()
{
  char *homedir = getenv ("HOME");
  char *filename;

  if (homedir)
    {
      filename = xmalloc (strlen (homedir) + strlen ("/" HISTORY_FILE) + 1);
      sprintf (filename, "%s/" HISTORY_FILE, homedir);
    }
  else
    {
      filename = xstrdup (HISTORY_FILE);
    }

  return filename;
}

void
history_load ()
{
  char *filename = get_history_filename ();

  if (filename && read_history (filename) != 0)
    PRINT_DEBUG (("ratpoison: could not read %s - %s\n", filename, strerror (errno)));

  free (filename);
}

void
history_save ()
{
  char *filename = get_history_filename ();

  if (filename && write_history (filename) != 0)
    PRINT_ERROR (("ratpoison: could not write %s - %s\n", filename, strerror (errno)));

  free (filename);
}

void
history_resize (int size)
{
  stifle_history (size);
}

void
history_reset ()
{
  using_history();
}

void
history_add (char *item)
{
  HIST_ENTRY *h = history_get (history_length);
  
  if (item == NULL || *item == '\0' || isspace (*item) || (h != NULL && !strcmp (h->line, item)))
    return;

  PRINT_DEBUG (("Adding item: %s\n", item));
  add_history (item);
}

char *
history_previous ()
{
  HIST_ENTRY *h = NULL;

  h = previous_history();

  return h ? h->line : NULL;
}

char *
history_next ()
{
  HIST_ENTRY *h = NULL;

  h = next_history();

  return h ? h->line : NULL;
}

char *
history_list_items ()
{
  HIST_ENTRY **hlist;
  struct sbuf *list;
  char *tmp;
  int i;

  list = sbuf_new (0);
  hlist = history_list ();

  if (!hlist) return NULL;

  for (i = 0; hlist[i] != NULL; i++)
    {
      sbuf_concat (list, hlist[i]->line);
      if (i < history_length - 1)
        sbuf_concat (list, "\n");
    }

  tmp = sbuf_get (list);
  free (list);

  return tmp;
}

int history_expand_line (char *string, char **output)
{
  return history_expand (string, output);
}

#endif /* HAVE_HISTORY */
