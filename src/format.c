/*
 * Copyright (C) 2006 Antti Nykänen <aon@iki.fi>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ratpoison.h"

/* Function prototypes for format char expanders. */
#define RP_FMT(fn) static void fmt_ ## fn (rp_window_elem *elem, struct sbuf *buf)
RP_FMT(framenum);
RP_FMT(lastaccess);
RP_FMT(name);
RP_FMT(number);
RP_FMT(resname);
RP_FMT(resclass);
RP_FMT(status);
RP_FMT(windowid);
RP_FMT(height);
RP_FMT(width);
RP_FMT(incheight);
RP_FMT(incwidth);
RP_FMT(gravity);
RP_FMT(screen);
RP_FMT(xinescreen);
RP_FMT(transient);
RP_FMT(maxsize);
RP_FMT(pid);

struct fmt_item {
  /* The format character */
  char fmt_char;

  /* Callback to return the expanded string. */
  void (*fmt_fn)(rp_window_elem *, struct sbuf *);
};

struct fmt_item fmt_items[] = {
  { 'a', fmt_resname },
  { 'g', fmt_gravity },
  { 'h', fmt_height },
  { 'H', fmt_incheight },
  { 'c', fmt_resclass },
  { 'f', fmt_framenum },
  { 'i', fmt_windowid },
  { 'l', fmt_lastaccess },
  { 'n', fmt_number },
  { 'p', fmt_pid },
  { 's', fmt_status },
  { 'S', fmt_screen },
  { 't', fmt_name },
  { 'T', fmt_transient },
  { 'M', fmt_maxsize },  
  { 'w', fmt_width },
  { 'W', fmt_incwidth },
  { 'x', fmt_xinescreen },
  { 0, NULL }
};

/* if width >= 0 then limit the width of s to width chars. */
static void
concat_width (struct sbuf *buf, char *s, int width)
{
  if (width >= 0)
    {
      char *s1 = xsprintf ("%%.%ds", width);
      char *s2 = xsprintf (s1, s);
      sbuf_concat (buf, s2);
      free (s1);
      free (s2);
    }
  else
    sbuf_concat (buf, s);
}

void
format_string (char *fmt, rp_window_elem *win_elem, struct sbuf *buffer)
{
#define STATE_READ   0
#define STATE_NUMBER 1
#define STATE_ESCAPE 2
  int state = STATE_READ;
  char dbuf[10];
  int width = -1;
  struct sbuf *retbuf;
  int fip, found;

  retbuf = sbuf_new (0);

  for(; *fmt; fmt++)
    {
      if (*fmt == '%' && state == STATE_READ)
        {
          state = STATE_ESCAPE;
          continue;
        }

      if ((state == STATE_ESCAPE || state == STATE_NUMBER) && isdigit(*fmt))
        {
          /* Accumulate the width one digit at a time. */
          if (state == STATE_ESCAPE)
            width = 0;
          width *= 10;
          width += *fmt - '0';
          state = STATE_NUMBER;
          continue;
        }

      found = 0;
      if (state == STATE_ESCAPE || state == STATE_NUMBER)
        {
          if (*fmt == '%')
              sbuf_concat (buffer, "%");
          else
            {
              for (fip = 0; fmt_items[fip].fmt_char; fip++)
                {
                  if (fmt_items[fip].fmt_char == *fmt)
                    {
                      sbuf_clear (retbuf);
                      fmt_items[fip].fmt_fn(win_elem, retbuf);
                      concat_width (buffer, sbuf_get (retbuf), width);
                      found = 1;
                      break;
                    }
                }
              if (!found)
                {
                  sbuf_printf_concat (buffer, "%%%c", *fmt);
                  break;
                }
            }
          state = STATE_READ;
          width = -1;
        }
      else
        {
          /* Insert the character. */
          dbuf[0] = *fmt;
          dbuf[1] = 0;
          sbuf_concat (buffer, dbuf);
        }
    }
  sbuf_free (retbuf);
#undef STATE_READ
#undef STATE_ESCAPE
#undef STATE_NUMBER
}

static void
fmt_framenum (rp_window_elem *win_elem, struct sbuf *buf)
{
  if (win_elem->win->frame_number != EMPTY)
    {
      sbuf_printf_concat (buf, "%d", win_elem->win->frame_number);
    }
  else
    sbuf_copy (buf, " ");
}

static void
fmt_lastaccess (rp_window_elem *win_elem, struct sbuf *buf)
{
  sbuf_printf_concat (buf, "%d", win_elem->win->last_access);
}

static void
fmt_name (rp_window_elem *win_elem, struct sbuf *buf)
{
  sbuf_copy(buf, window_name (win_elem->win));
}

static void
fmt_number (rp_window_elem *win_elem, struct sbuf *buf)
{
  sbuf_printf_concat (buf, "%d", win_elem->number);
}

static void
fmt_resname (rp_window_elem *win_elem, struct sbuf *buf)
{
  if (win_elem->win->res_name)
    sbuf_copy (buf, win_elem->win->res_name);
  else
    sbuf_copy (buf, "None");
}

static void
fmt_resclass (rp_window_elem *win_elem, struct sbuf *buf)
{
  if (win_elem->win->res_class)
    sbuf_copy (buf, win_elem->win->res_class);
  else
    sbuf_copy (buf, "None");
}

static void
fmt_status (rp_window_elem *win_elem, struct sbuf *buf)
{
  rp_window *other_window;

  other_window = find_window_other (current_screen());
  if (win_elem->win == other_window)
    sbuf_copy (buf, "+");
  else if (win_elem->win == current_window())
    sbuf_copy (buf, "*");
  else
    sbuf_copy (buf, "-");
}

static void
fmt_windowid (rp_window_elem *elem, struct sbuf *buf)
{
  sbuf_printf_concat (buf, "%ld", (unsigned long)elem->win->w);
}

static void
fmt_height (rp_window_elem *elem, struct sbuf *buf)
{
  sbuf_printf_concat (buf, "%d", elem->win->height);
}

static void
fmt_width (rp_window_elem *elem, struct sbuf *buf)
{
  sbuf_printf_concat (buf, "%d", elem->win->width);
}

static void
fmt_incheight (rp_window_elem *elem, struct sbuf *buf)
{
  int height;
  height = elem->win->height;

  if (elem->win->hints->flags & PResizeInc)
    height /= elem->win->hints->height_inc;

  sbuf_printf_concat (buf, "%d", height);
}

static void
fmt_incwidth (rp_window_elem *elem, struct sbuf *buf)
{
  int width;
  width = elem->win->width;

  if (elem->win->hints->flags & PResizeInc)
    width /= elem->win->hints->width_inc;

  sbuf_printf_concat (buf, "%d", width);
}

static void
fmt_gravity (rp_window_elem *elem, struct sbuf *buf)
{
  sbuf_copy (buf, wingravity_to_string (elem->win->gravity));
}

static void
fmt_screen (rp_window_elem *elem, struct sbuf *buf)
{
  sbuf_printf_concat (buf, "%d", elem->win->scr->screen_num);
}

static void
fmt_xinescreen (rp_window_elem *elem, struct sbuf *buf)
{
  sbuf_printf_concat (buf, "%d", elem->win->scr->xine_screen_num);
}

static void
fmt_transient (rp_window_elem *elem, struct sbuf *buf)
{
  if (elem->win->transient)
    sbuf_concat (buf, "Transient");
}

static void
fmt_maxsize (rp_window_elem *elem, struct sbuf *buf)
{
  if (elem->win->hints->flags & PMaxSize)
    sbuf_concat (buf, "Maxsize");
}

static void
fmt_pid (rp_window_elem *elem, struct sbuf *buf)
{
  struct rp_child_info *info;

  info = get_child_info (elem->win->w);
  if (info)
    sbuf_printf_concat (buf, "%d", info->pid);
  else
    sbuf_concat (buf, "?");
}
