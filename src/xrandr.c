/* functions for grabbing information about xrandr screens
 * Copyright (C) 2016 Mathieu OTHACEHE <m.othacehe@gmail.com>
 *
 * This file is part of ratpoison.
 *
 * ratpoison is free software; you can redistribute it and/or moify
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

#include <X11/extensions/Xrandr.h>

static int xrandr_evbase;

#define XRANDR_MAJOR 1
#define XRANDR_MINOR 2

void
init_xrandr (void)
{
  int errbase, major, minor;

  if (!XRRQueryExtension (dpy, &xrandr_evbase, &errbase)) {
    return;
  }

  if (XRRQueryVersion (dpy, &major, &minor) == 0) {
    return;
  }

  if (major != XRANDR_MAJOR ||
      (major == XRANDR_MAJOR && minor < XRANDR_MINOR)) {
    PRINT_ERROR (("Xrandr version %d.%d is not supported\n", major, minor));
    return;
  }

  XRRSelectInput (dpy, RootWindow (dpy, DefaultScreen(dpy)),
                  RRCrtcChangeNotifyMask | RROutputChangeNotifyMask);
}

int *
xrandr_query_screen (int *screen_count)
{
  XRRScreenResources *res;
  XRROutputInfo *outinfo;
  int *output_array;
  int count = 0;
  int i;

  res = XRRGetScreenResources (dpy, RootWindow (dpy, DefaultScreen (dpy)));
  output_array = xmalloc (res->noutput * sizeof(int));

  for (i = 0; i < res->noutput; i++) {
    outinfo = XRRGetOutputInfo (dpy, res, res->outputs[i]);
    if (!outinfo->crtc)
      continue;

    output_array[count] = res->outputs[i];
    count++;

    XRRFreeOutputInfo (outinfo);
  }

  *screen_count = count;
  XRRFreeScreenResources (res);

  return output_array;
}

static rp_screen *
xrandr_screen_output (int rr_output)
{
  rp_screen *cur;

  list_for_each_entry (cur, &rp_screens, node)
    {
      if (cur->xrandr.output == rr_output)
        return cur;
    }

  return NULL;
}

static rp_screen *
xrandr_screen_crtc (int rr_crtc)
{
  rp_screen *cur;

  list_for_each_entry (cur, &rp_screens, node)
    {
      if (cur->xrandr.crtc == rr_crtc)
        return cur;
    }

  return NULL;
}

void
xrandr_fill_screen (int rr_output, rp_screen *screen)
{
  XRRScreenResources *res;
  XRROutputInfo *outinfo;
  XRRCrtcInfo *crtinfo;

  res = XRRGetScreenResourcesCurrent (dpy, RootWindow (dpy, DefaultScreen (dpy)));
  outinfo = XRRGetOutputInfo (dpy, res, rr_output);
  if (!outinfo->crtc)
    goto free_res;

  crtinfo = XRRGetCrtcInfo (dpy, res, outinfo->crtc);
  if (!crtinfo)
    goto free_out;

  screen->xrandr.name = sbuf_new (0);
  sbuf_concat (screen->xrandr.name, outinfo->name);

  screen->xrandr.output  = rr_output;
  screen->xrandr.crtc    = outinfo->crtc;

  screen->left   = crtinfo->x;
  screen->top    = crtinfo->y;
  screen->width  = crtinfo->width;
  screen->height = crtinfo->height;

  XRRFreeCrtcInfo (crtinfo);
 free_out:
  XRRFreeOutputInfo (outinfo);
 free_res:
  XRRFreeScreenResources (res);
}

static void
xrandr_output_change (XRROutputChangeNotifyEvent *ev)
{
  XRRScreenResources *res;
  XRROutputInfo *outinfo;
  rp_screen *screen;

  res = XRRGetScreenResourcesCurrent (dpy, RootWindow (dpy, DefaultScreen (dpy)));
  outinfo = XRRGetOutputInfo (dpy, res, ev->output);

  screen = xrandr_screen_output (ev->output);
  if (!screen && outinfo->crtc) {
    screen_add (ev->output);
    screen_sort ();
  } else if (screen && !outinfo->crtc) {
    screen_del (screen);
  }

  XRRFreeOutputInfo (outinfo);
  XRRFreeScreenResources (res);
}

static void
xrandr_crtc_change (XRRCrtcChangeNotifyEvent *ev)
{
  rp_screen *screen;

  if (!ev->crtc || !ev->width || !ev->height)
    return;

  screen = xrandr_screen_crtc (ev->crtc);
  if (screen)
    screen_update (screen, ev->x, ev->y, ev->width, ev->height);
}

void
xrandr_notify (XEvent *ev)
{
  int ev_code = xrandr_evbase + RRNotify;
  XRRNotifyEvent *n_event;
  XRROutputChangeNotifyEvent *o_event;
  XRRCrtcChangeNotifyEvent *c_event;

  if (ev->type != ev_code)
    return;

  n_event = (XRRNotifyEvent *)ev;
  switch (n_event->subtype) {
  case RRNotify_OutputChange:
    o_event = (XRROutputChangeNotifyEvent *)ev;
    xrandr_output_change (o_event);
    break;
  case RRNotify_CrtcChange:
    c_event = (XRRCrtcChangeNotifyEvent *)ev;
    xrandr_crtc_change (c_event);
    break;
  default:
    break;
  }
}
