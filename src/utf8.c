/*
 * Copyright (C) 2017  Jeremie Courreges-Anglas <jca@wxcvcbn.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "ratpoison.h"

#ifdef HAVE_LANGINFO_CODESET
# include <langinfo.h>
#endif

int utf8_locale;

int
utf8_check_locale(void)
{
#ifdef HAVE_LANGINFO_CODESET
  utf8_locale = !strcmp (nl_langinfo (CODESET), "UTF-8");
#endif
  return utf8_locale;
}

int
isu8char(char c)
{
  return utf8_locale && (c) & 0xC0;
}

int
isu8start(char c)
{
  return utf8_locale && ((c) & 0xC0) == 0xC0;
}

int
isu8cont(char c)
{
  return utf8_locale && ((c) & 0xC0) == 0x80;
}
