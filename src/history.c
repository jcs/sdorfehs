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

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "ratpoison.h"

#ifdef HAVE_HISTORY
#include "readline/history.h"
#endif

static char *
get_history_filename (void)
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

#ifdef HAVE_HISTORY
void
history_load (void)
{
  char *filename = get_history_filename ();

  if (filename && read_history (filename) != 0)
    PRINT_DEBUG (("ratpoison: could not read %s - %s\n", filename, strerror (errno)));

  free (filename);
}

void
history_save (void)
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
history_reset (void)
{
  using_history();
}

void
history_add (int history_id, char *item)
{
  HIST_ENTRY *h;

  if (history_id == hist_NONE || history_id == hist_SHELLCMD)
    return;

  h = history_get (history_length);

  if (item == NULL || *item == '\0' || isspace (*item) || (h != NULL && !strcmp (h->line, item)))
    return;

  PRINT_DEBUG (("Adding item: %s\n", item));
  add_history (item);
}
#endif /* HAVE_HISTORY */

static const char *
extract_shell_part (const char *p)
{
  if (strncmp(p, "exec", 4) &&
      strncmp(p, "verbexec", 8))
    return NULL;
  while( *p && !isspace(*p) )
    p++;
  while( *p && isspace(*p) )
    p++;
  if (*p)
    return p;
  return NULL;
}

#ifdef HAVE_HISTORY
const char *
history_previous (int history_id)
{
  HIST_ENTRY *h = NULL;
  const char *p;

  if (history_id == hist_NONE)
    return NULL;

  h = previous_history();

  if (history_id == hist_SHELLCMD) {
    p = NULL;
    while( h && h->line && !(p = extract_shell_part(h->line)))
      h = previous_history();
    return p;
  }

  return h ? h->line : NULL;
}

const char *
history_next (int history_id)
{
  HIST_ENTRY *h = NULL;
  const char *p;

  if (history_id == hist_NONE)
    return NULL;

  h = next_history();

  if (history_id == hist_SHELLCMD) {
    p = NULL;
    while( h && h->line && !(p = extract_shell_part(h->line)))
      h = next_history();
    return p;
  }
  return h ? h->line : NULL;
}

int history_expand_line (int history_id, char *string, char **output)
{
  return history_expand (string, output);
}
#else /* HAVE_HISTORY */

void
history_add (int history_id, char *item)
{
}

void
history_load (void)
{
}

void
history_save (void)
{
}

void
history_reset (void)
{
}

void
history_resize (int size)
{
}

const char *
history_previous (int history_id)
{
  return NULL;
}

const char *
history_next (int history_id)
{
  return NULL;
}

int history_expand_line (int history_id, char *string, char **output)
{
  *output = xstrdup(string);
  return 0;
}

#endif /* HAVE_HISTORY */
