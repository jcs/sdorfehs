/* Standard header for ratpoison.
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

#ifndef _RATPOISON_H
#define _RATPOISON_H 1

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <fcntl.h>

#if defined(__BASE_FILE__)
#define RP_FILE_NAME __BASE_FILE__
#else
#define RP_FILE_NAME __FILE__
#endif

/* Helper macro for error and debug reporting. */
#define PRINT_LINE(type) printf (PACKAGE ":%s:%d: %s: ",RP_FILE_NAME,  __LINE__, #type)

/* Error and debug reporting macros. */
#define PRINT_ERROR(fmt)                        \
do {                                            \
  PRINT_LINE (error);                           \
  printf fmt;                                   \
  fflush (stdout);                              \
} while (0)

#define PRINT_WARNING(fmt)                      \
do {                                            \
  PRINT_LINE (warning);                         \
  printf fmt;                                   \
  fflush (stdout);                              \
} while (0)

#ifdef DEBUG
#define PRINT_DEBUG(fmt)                        \
do {                                            \
  PRINT_LINE (debug);                           \
  printf fmt;                                   \
  fflush (stdout);                              \
} while (0)
#else
#define PRINT_DEBUG(fmt) do {} while (0)
#endif /* DEBUG */

#include "conf.h"

#include "data.h"
#include "globals.h"
#include "manage.h"
#include "window.h"
#include "bar.h"
#include "events.h"
#include "number.h"
#include "input.h"
#include "messages.h"
#include "communications.h"
#include "sbuf.h"
#include "split.h"
#include "frame.h"
#include "screen.h"
#include "group.h"
#include "editor.h"
#include "history.h"
#include "completions.h"
#include "hook.h"
#include "xrandr.h"
#include "format.h"
#include "utf8.h"
#include "util.h"

void set_extents_of_fontset (XFontSet font);
XFontSet load_query_font_set (Display *disp, const char *fontset_name);

#endif /* ! _RATPOISON_H */
