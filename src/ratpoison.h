/* Standard header for ratpoison.
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

#ifndef _RATPOISON_H
#define _RATPOISON_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <X11/Xlib.h>

/* Some error reporting macros */
#define PRE_PRINT_LOCATION fprintf (stderr, "%s:%s():%d: ", __FILE__, __FUNCTION__, __LINE__);
#define PRINT_ERROR(format, args...) \
        { fprintf (stderr, PACKAGE ":error -- "); PRE_PRINT_LOCATION; fprintf (stderr, format, ## args); }

/* Some debugging macros */
#ifdef DEBUG
#  define PRINT_DEBUG(format, args...) \
          { fprintf (stderr, PACKAGE ":debug -- "); PRE_PRINT_LOCATION; fprintf (stderr, format, ## args); }
#else
#  define PRINT_DEBUG(format, args...) 
#endif /* DEBUG */

extern XGCValues gv;

#include "conf.h"

#include "data.h"
#include "manage.h"
#include "list.h"
#include "bar.h"
#include "events.h"
#include "number.h"
#include "input.h"
#include "messages.h"
#include "communications.h"
#include "sbuf.h"
#include "split.h"

void clean_up ();
screen_info *find_screen (Window w);

void read_rc_file (FILE *file);

void fatal (const char *msg);
void *xmalloc (size_t size);
void *xrealloc (void *ptr, size_t size);
char *xstrdup (char *s);
char *xsprintf (char *fmt, ...);
char *xvsprintf (char *fmt, va_list ap);

#endif /* ! _RATPOISON_H */
