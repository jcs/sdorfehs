/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>

#include "sdorfehs.h"

static char *
get_history_filename(void)
{
	const char *homedir;
	char *filename;

	homedir = get_homedir();

	if (homedir) {
		struct sbuf *buf;

		buf = sbuf_new(0);
		sbuf_printf(buf, "%s/.config/sdorfehs/%s", homedir,
		    HISTORY_FILE);
		filename = sbuf_free_struct(buf);
	} else {
		filename = xstrdup(HISTORY_FILE);
	}

	return filename;
}

static const char *
extract_shell_part(const char *p)
{
	if (strncmp(p, "exec", 4) && strncmp(p, "verbexec", 8))
		return NULL;
	while (*p && !isspace((unsigned char) *p))
		p++;
	while (*p && isspace((unsigned char) *p))
		p++;
	if (*p)
		return p;
	return NULL;
}

struct history_item {
	struct list_head node;
	char *line;
};

static struct history {
	struct list_head head, *current;
	size_t count;
}       histories[hist_COUNT];

static void
history_add_upto(int history_id, const char *item, size_t max)
{
	struct history *h = histories + history_id;
	struct history_item *i;

	if (item == NULL || *item == '\0' || isspace((unsigned char) *item))
		return;

	list_last(i, &histories[history_id].head, node);
	if (i && !strcmp(i->line, item))
		return;

	if (history_id == hist_COMMAND) {
		const char *p = extract_shell_part(item);
		if (p)
			history_add_upto(hist_SHELLCMD, p, max);
	}
	while (h->count >= max) {
		list_first(i, &h->head, node);
		if (!i) {
			h->count = 0;
			break;
		}
		list_del(&i->node);
		free(i->line);
		free(i);
		h->count--;
	}

	if (max == 0)
		return;

	i = xmalloc(sizeof(*i));
	i->line = xstrdup(item);

	list_add_tail(&i->node, &h->head);
	h->count++;
}

void
history_add(int history_id, const char *item)
{
	history_add_upto(history_id, item, defaults.history_size);
}

void
history_load(void)
{
	char *filename;
	FILE *f;
	char *line = NULL;
	size_t s = 0;
	ssize_t linelen;
	int id;

	for (id = hist_NONE; id < hist_COUNT; id++) {
		INIT_LIST_HEAD(&histories[id].head);
		histories[id].current = &histories[id].head;
		histories[id].count = 0;
	}

	filename = get_history_filename();
	if (!filename)
		return;

	f = fopen(filename, "r");
	if (!f) {
		warn("could not load history from %s", filename);
		free(filename);
		return;
	}
	while ((linelen = getline(&line, &s, f)) >= 0) {
		while (linelen > 0 && (line[linelen - 1] == '\n' ||
		    line[linelen - 1] == '\r')) {
			line[--linelen] = '\0';
		}
		if (linelen == 0)
			continue;
		/* defaults.history_size might be only set later */
		history_add_upto(hist_COMMAND, line, INT_MAX);
	}
	free(line);
	if (ferror(f)) {
		PRINT_DEBUG(("error reading history %s: %s\n", filename,
		    strerror(errno)));
		fclose(f);
		free(filename);
		return;
	}
	if (fclose(f))
		PRINT_DEBUG(("error reading %s: %s\n", filename,
		    strerror(errno)));
	free(filename);
}

void
history_save(void)
{
	char *filename;
	FILE *f;
	struct history_item *item;

	if (!defaults.history_size)
		return;

	filename = get_history_filename();
	if (!filename)
		return;

	f = fopen(filename, "w");
	if (!f) {
		PRINT_DEBUG(("could not write history to %s: %s\n", filename,
		    strerror(errno)));
		free(filename);
		return;
	}
	if (fchmod(fileno(f), 0600) == -1)
		warn("could not change mode to 0600 on %s", filename);

	list_for_each_entry(item, &histories[hist_COMMAND].head, node) {
		fputs(item->line, f);
		putc('\n', f);
	}

	if (ferror(f)) {
		PRINT_DEBUG(("error writing history to %s: %s\n", filename,
		    strerror(errno)));
		fclose(f);
		free(filename);
		return;
	}
	if (fclose(f))
		PRINT_DEBUG(("error writing history to %s: %s\n", filename,
		    strerror(errno)));
	free(filename);
}

void
history_reset(void)
{
	int id;

	for (id = hist_NONE; id < hist_COUNT; id++)
		histories[id].current = &histories[id].head;
}

const char *
history_previous(int history_id)
{
	if (history_id == hist_NONE)
		return NULL;
	/* return NULL, if list empty or already at first */
	if (histories[history_id].current == histories[history_id].head.next)
		return NULL;
	histories[history_id].current = histories[history_id].current->prev;
	return list_entry(histories[history_id].current,
	    struct history_item, node)->line;
}

const char *
history_next(int history_id)
{
	if (history_id == hist_NONE)
		return NULL;
	/* return NULL, if list empty or already behind last */
	if (histories[history_id].current == &histories[history_id].head)
		return NULL;
	histories[history_id].current = histories[history_id].current->next;
	if (histories[history_id].current == &histories[history_id].head)
		return NULL;
	return list_entry(histories[history_id].current, struct history_item,
	    node)->line;
}
