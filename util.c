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

#include "sdorfehs.h"

#include <ctype.h>
#include <err.h>

__dead void
fatal(const char *msg)
{
	fprintf(stderr, PROGNAME ": %s", msg);
	abort();
}

void *
xmalloc(size_t size)
{
	void *value;

	value = malloc(size);
	if (value == NULL)
		fatal("Virtual memory exhausted");
	return value;
}

void *
xrealloc(void *ptr, size_t size)
{
	void *value;

	value = realloc(ptr, size);
	if (value == NULL)
		fatal("Virtual memory exhausted");
	return value;
}

char *
xstrdup(const char *s)
{
	char *value;
	value = strdup(s);
	if (value == NULL)
		fatal("Virtual memory exhausted");
	return value;
}

/* Return a new string based on fmt. */
char *
xvsprintf(char *fmt, va_list ap)
{
	int size, nchars;
	char *buffer;
	va_list ap_copy;

	/* A reasonable starting value. */
	size = strlen(fmt) + 1;
	buffer = xmalloc(size);

	while (1) {
#if defined(va_copy)
		va_copy(ap_copy, ap);
#elif defined(__va_copy)
		__va_copy(ap_copy, ap);
#else
		/*
		 * If there is no copy macro then this MAY work. On some
		 * systems this could fail because va_list is a pointer so
		 * assigning one to the other as below wouldn't make a copy of
		 * the data, but just the pointer to the data.
		 */
		ap_copy = ap;
#endif
		nchars = vsnprintf(buffer, size, fmt, ap_copy);
#if defined(va_copy) || defined(__va_copy)
		va_end(ap_copy);
#endif

		if (nchars > -1 && nchars < size)
			return buffer;
		else if (nchars > -1)
			size = nchars + 1;
		else {
			free(buffer);
			break;
		}

		/* Resize the buffer and try again. */
		buffer = xrealloc(buffer, size);
	}

	return xstrdup("<FAILURE>");
}

/* Return a new string based on fmt. */
char *
xsprintf(char *fmt,...)
{
	char *buffer;
	va_list ap;

	va_start(ap, fmt);
	buffer = xvsprintf(fmt, ap);
	va_end(ap);

	return buffer;
}

/* strtok but do it for whitespace and be locale compliant. */
char *
strtok_ws(char *s)
{
	char *nonws;
	static char *last = NULL;

	if (s != NULL)
		last = s;
	else if (last == NULL) {
		warnx("strtok_ws() called but not initalized, this is a *BUG*");
		abort();
	}
	/* skip to first non-whitespace char. */
	while (*last && isspace((unsigned char) *last))
		last++;

	/*
	 * If we reached the end of the string here then there is no more data.
	 */
	if (*last == '\0')
		return NULL;

	/* Now skip to the end of the data. */
	nonws = last;
	while (*last && !isspace((unsigned char) *last))
		last++;
	if (*last) {
		*last = '\0';
		last++;
	}
	return nonws;
}
