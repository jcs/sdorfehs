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
#include <limits.h>

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

struct history_item {
	struct list_head node;
	char line[];
};

static struct history {
	struct list_head head, *current;
	size_t count;
} histories[hist_COUNT];

#ifndef HAVE_GETLINE
static ssize_t
getline(char **lineptr, size_t *n, FILE *f)
{
  size_t ofs;

  if (!*lineptr) {
    *lineptr = xmalloc (4096);
    *n = 4096;
  }
  ofs = 0;
  do {
    (*lineptr)[ofs] = '\0';
    if (!fgets (*lineptr, (*n) - ofs, f)) {
      /* do not tread unterminated last lines as errors,
       * (it's still a malformed text file and noone should
       *  have created it) */
      return ofs?(ssize_t)ofs:-1;
    }
    ofs += strlen ((*lineptr) + ofs);
    if (ofs >= *n)
      /* should never happen... */
      return -1;
    if (ofs > 0 && (*lineptr)[ofs-1] == '\n')
      return ofs;
    if (ofs + 1 == *n) {
      if (*n >= INT_MAX - 4096)
	return -1;
      *n += 4096;
      *lineptr = xrealloc(*lineptr, *n);
    }
  } while(1);
}
#endif

static void
history_add_upto (int history_id, const char *item, size_t max)
{
  struct history *h = histories + history_id;
  struct history_item *i;
  size_t item_len;

  if (item == NULL || *item == '\0' || isspace(*item))
    return;

  list_last (i, &histories[history_id].head, node);
  if (i && !strcmp (i->line, item))
    return;

  if (history_id == hist_COMMAND) {
    const char *p = extract_shell_part (item);
    if (p)
      history_add_upto (hist_SHELLCMD, p, max);
  }

  if (defaults.history_compaction && max != INT_MAX) {
    struct list_head *l;

    for (l = h->head.prev ; l != &h->head ; l = l->prev) {
      if (!strcmp (list_entry(l, struct history_item, node)->line, item)) {
	list_del (l);
	list_add_tail (l, &h->head);
	return;
      }
    }
  }

  while (h->count >= max) {
	  list_first (i, &h->head, node);
	  if (!i) {
		  h->count = 0;
		  break;
	  }
	  list_del (&i->node);
	  free (i);
	  h->count--;
  }

  if( max == 0 )
	  return;

  item_len = strlen(item);
  i = xmalloc (sizeof(struct history_item) + item_len + 1);

  memcpy (i->line, item, item_len + 1);
  list_add_tail (&i->node, &h->head);
  h->count++;
}

void
history_add (int history_id, char *item)
{
  history_add_upto (history_id, item, defaults.history_size);
}

void
history_load (void)
{
  char *filename = get_history_filename ();
  FILE *f;
  char *line = NULL;
  size_t s = 0;
  ssize_t linelen;
  int id;

  for (id = hist_NONE ; id < hist_COUNT ; id++ ) {
    INIT_LIST_HEAD (&histories[id].head);
    histories[id].current = &histories[id].head;
    histories[id].count = 0;
  }

  if (!filename)
    return;
  f = fopen (filename, "r");
  if (!f) {
    PRINT_DEBUG (("ratpoison: could not read %s - %s\n", filename, strerror (errno)));
    free (filename);
    return;
  }

  while ((linelen = getline (&line, &s, f)) >= 0) {
    while (linelen > 0 && (line[linelen-1] == '\n' || line[linelen-1] == '\r')) {
      line[--linelen] = '\0';
    }
    if (linelen == 0)
      continue;
    /* defaults.history_size might be only set later */
    history_add_upto (hist_COMMAND, line, INT_MAX);
  }
  if (ferror (f)) {
    PRINT_DEBUG (("ratpoison: error reading %s - %s\n", filename, strerror (errno)));
    fclose(f);
    free (filename);
    return;
  }
  if (!fclose(f))
    PRINT_DEBUG (("ratpoison: error reading %s - %s\n", filename, strerror (errno)));
  free (filename);
}

void
history_save (void)
{
  char *filename = get_history_filename ();
  FILE *f;
  struct history_item *item;

  if (!defaults.history_size)
    return;

  if (!filename)
    return;
  f = fopen (filename, "w");
  if (!f) {
    PRINT_DEBUG (("ratpoison: could not write %s - %s\n", filename, strerror (errno)));
    free (filename);
    return;
  }

  list_for_each_entry(item, &histories[hist_COMMAND].head, node) {
    fputs(item->line, f);
    putc('\n', f);
  }

  if (ferror (f)) {
    PRINT_DEBUG (("ratpoison: error writing %s - %s\n", filename, strerror (errno)));
    fclose(f);
    free (filename);
    return;
  }
  if (!fclose(f))
    PRINT_DEBUG (("ratpoison: error writing %s - %s\n", filename, strerror (errno)));
  free (filename);
}

void
history_reset (void)
{
  int id;

  for (id = hist_NONE ; id < hist_COUNT ; id++ )
    	histories[id].current = &histories[id].head;
}

void
history_resize (int size)
{
  struct history_item *i;
  struct history *h;
  int id;

  for (id = hist_NONE ; id < hist_COUNT ; id++ ) {
	  h = histories + id;
	  while (h->count >= (size_t)size) {
		  list_first (i, &h->head, node);
		  list_del (&i->node);
		  free (i);
		  h->count--;
	  }
  }
}

const char *
history_previous (int history_id)
{
  if (history_id == hist_NONE)
    return NULL;
  /* return NULL, if list empty or already at first */
  if (histories[history_id].current == histories[history_id].head.next)
    return NULL;
  histories[history_id].current = histories[history_id].current->prev;
  return list_entry(histories[history_id].current, struct history_item, node)->line;
}

const char *
history_next (int history_id)
{
  if (history_id == hist_NONE)
    return NULL;
  /* return NULL, if list empty or already behind last */
  if (histories[history_id].current == &histories[history_id].head)
    return NULL;
  histories[history_id].current = histories[history_id].current->next;
  if (histories[history_id].current == &histories[history_id].head)
    return NULL;
  return list_entry(histories[history_id].current, struct history_item, node)->line;
}

int history_expand_line (int history_id, char *string, char **output)
{
#ifdef HAVE_HISTORY
  struct history_item *item;

  if (strchr (string, '!')) {
    clear_history ();
    using_history ();
    list_for_each_entry(item, &histories[history_id].head, node) {
      add_history (item->line);
    }
    return history_expand (string, output);
  }
#endif
  *output = xstrdup(string);
  return 0;
}
