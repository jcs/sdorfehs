/*
 * Copyright (C) 2017 Will Storey <will@summercat.com>
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

#include "ratpoison.h"

#include <assert.h>
#include <locale.h>

void test_sbuf_utf8_nconcat(void);

int main(void)
{
  utf8_locale = 1;

  test_sbuf_utf8_nconcat();
  return 0;
}

void
test_sbuf_utf8_nconcat(void)
{
  struct sbuf *buf = NULL;

  /* Zero length string, no limit. */

  buf = sbuf_new(0);
  sbuf_utf8_nconcat(buf, "", -1);
  assert(strcmp(sbuf_get(buf), "") == 0);
  sbuf_free(buf);

  /* Zero length string, non-zero limit. */

  buf = sbuf_new(0);
  sbuf_utf8_nconcat(buf, "", 5);
  assert(strcmp(sbuf_get(buf), "") == 0);
  sbuf_free(buf);

  /* ASCII string, no limit. */

  buf = sbuf_new(0);
  sbuf_utf8_nconcat(buf, "hi there", -1);
  assert(strcmp(sbuf_get(buf), "hi there") == 0);
  sbuf_utf8_nconcat(buf, " you", -1);
  assert(strcmp(sbuf_get(buf), "hi there you") == 0);
  sbuf_free(buf);

  /* ASCII string, non-zero limit, truncated. */

  buf = sbuf_new(0);
  sbuf_utf8_nconcat(buf, "hi there", 4);
  assert(strcmp(sbuf_get(buf), "hi t") == 0);
  sbuf_free(buf);

  /* ASCII string, non-zero limit, not truncated. */

  buf = sbuf_new(0);
  sbuf_utf8_nconcat(buf, "hi", 4);
  assert(strcmp(sbuf_get(buf), "hi") == 0);
  sbuf_free(buf);

  /* UTF-8 string, no limit. */

  buf = sbuf_new(0);
  /* 0xe2 0x84 0xa2 is U+2122, the trademark symbol. */
  sbuf_utf8_nconcat(buf, "hi \xe2\x84\xa2 there", -1);
  assert(strcmp(sbuf_get(buf), "hi \xe2\x84\xa2 there") == 0);
  sbuf_free(buf);

  /* UTF-8 string, non-zero limit, truncated at an okay spot counting either
     by bytes or by characters. */

  buf = sbuf_new(0);
  sbuf_utf8_nconcat(buf, "hi \xe2\x84\xa2 there you", 11);
  assert(strcmp(sbuf_get(buf), "hi \xe2\x84\xa2 there ") == 0);
  sbuf_free(buf);

  /* UTF-8 string, non-zero limit, truncated such that if the limit were in
     bytes that we would cut in the middle of a character. */

  buf = sbuf_new(0);
  sbuf_utf8_nconcat(buf, "hi \xe2\x84\xa2 there you", 5);
  assert(strcmp(sbuf_get(buf), "hi \xe2\x84\xa2 ") == 0);
  sbuf_free(buf);

  /* UTF-8 string, non-zero limit, not truncated. */

  buf = sbuf_new(0);
  sbuf_utf8_nconcat(buf, "hi \xe2\x84\xa2 there you", 20);
  assert(strcmp(sbuf_get(buf), "hi \xe2\x84\xa2 there you") == 0);
  sbuf_free(buf);

  /* Invalid character. */

  buf = sbuf_new(0);
  /* This is an invalid UTF-8 sequence. It's missing 0xa2. */
  sbuf_utf8_nconcat(buf, "hi \xe2\x84 there you", 20);
  assert(strcmp(sbuf_get(buf), "hi \xe2\x84 there you") == 0);
  sbuf_free(buf);
}
