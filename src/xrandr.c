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

#ifdef HAVE_XRANDR

#include <X11/extensions/Xrandr.h>

static int xrandr_evbase;

#define XRANDR_MAJOR 1
#define XRANDR_MINOR 3

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

  rp_have_xrandr = 1;
}

int
xrandr_query_screen (int **outputs)
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

    output_array[count++] = res->outputs[i];

    XRRFreeOutputInfo (outinfo);
  }

  XRRFreeScreenResources (res);

  *outputs = output_array;
  return count;
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

int
xrandr_is_primary (rp_screen *screen)
{
  return screen->xrandr.primary;
}

void
xrandr_fill_screen (int rr_output, rp_screen *screen)
{
  XRRScreenResources *res;
  XRROutputInfo *outinfo;
  XRRCrtcInfo *crtinfo;
  RROutput primary;

  res = XRRGetScreenResourcesCurrent (dpy, RootWindow (dpy, DefaultScreen (dpy)));
  outinfo = XRRGetOutputInfo (dpy, res, rr_output);
  if (!outinfo->crtc)
    goto free_res;

  crtinfo = XRRGetCrtcInfo (dpy, res, outinfo->crtc);
  if (!crtinfo)
    goto free_out;

  primary = XRRGetOutputPrimary (dpy, RootWindow (dpy, DefaultScreen (dpy)));
  if (rr_output == primary)
    screen->xrandr.primary = 1;
  else
    screen->xrandr.primary = 0;

  screen->xrandr.name = xstrdup (outinfo->name);
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
    screen = screen_add (ev->output);
    screen_sort ();
    PRINT_DEBUG (("%s: Added screen %s with crtc %lu\n", __func__,
                  screen->xrandr.name,
                  (unsigned long)outinfo->crtc));
  } else if (screen && !outinfo->crtc) {
    PRINT_DEBUG (("%s: Removing screen %s\n", __func__,
                  screen->xrandr.name));
    screen_del (screen);
  }

  XRRFreeOutputInfo (outinfo);
  XRRFreeScreenResources (res);
}

#ifdef DEBUG
static const char *
xrandr_rotation_string (Rotation r)
{
  static char buf[64];

#define CASE(c) case c : return #c
  switch (r)
    {
      CASE(RR_Rotate_0);
      CASE(RR_Rotate_90);
      CASE(RR_Rotate_180);
      CASE(RR_Rotate_270);
#undef CASE
    default:
      snprintf(buf, sizeof buf, "Unknown rotation %hu", (unsigned short)r);
      return buf;
  }
}
#endif

static void
xrandr_crtc_change (XRRCrtcChangeNotifyEvent *ev)
{
  rp_screen *screen;

  if (!ev->crtc || !ev->width || !ev->height)
    return;

  screen = xrandr_screen_crtc (ev->crtc);

  PRINT_DEBUG (("%s: crtc %s, rotation %s "
                "ev->x %d, ev->y %d, ev->width %d, ev->height %d\n",
                __func__, screen ? "found" : "not found",
                xrandr_rotation_string (ev->rotation),
                ev->x, ev->y, ev->width, ev->height));

  if (!screen)
    return;

  if (ev->rotation == RR_Rotate_90 || ev->rotation == RR_Rotate_270)
    screen_update (screen, ev->x, ev->y, ev->height, ev->width);
  else
    screen_update (screen, ev->x, ev->y, ev->width, ev->height);
}

void
xrandr_notify (XEvent *ev)
{
  XRRNotifyEvent *n_event;
  XRROutputChangeNotifyEvent *o_event;
  XRRCrtcChangeNotifyEvent *c_event;

  if (ev->type != xrandr_evbase + RRNotify)
    return;

  PRINT_DEBUG (("--- Handling RRNotify ---\n"));

  n_event = (XRRNotifyEvent *)ev;
  switch (n_event->subtype) {
  case RRNotify_OutputChange:
    PRINT_DEBUG (("---          XRROutputChangeNotifyEvent ---\n"));
    o_event = (XRROutputChangeNotifyEvent *)ev;
    xrandr_output_change (o_event);
    break;
  case RRNotify_CrtcChange:
    PRINT_DEBUG (("---          XRRCrtcChangeNotifyEvent ---\n"));
    c_event = (XRRCrtcChangeNotifyEvent *)ev;
    xrandr_crtc_change (c_event);
    break;
  case RRNotify_OutputProperty:
    PRINT_DEBUG (("---          RRNotify_OutputProperty ---\n"));
    break;
  default:
    PRINT_DEBUG (("---          Unknown subtype %d ---\n", n_event->subtype));
    break;
  }
}

#else /* HAVE_XRANDR */

void
init_xrandr(void)
{
  /* Nothing */
}

int
xrandr_query_screen (int **outputs)
{
  (void)outputs;
  fatal("xrandr_query_screen shouldn't be called.  This is a BUG.");
  return 0;
}

int
xrandr_is_primary (rp_screen *screen)
{
  (void)screen;
  fatal("xrandr_is_primary shouldn't be called.  This is a BUG.");
  return 0;
}

void
xrandr_fill_screen (int rr_output, rp_screen *screen)
{
  (void)rr_output;
  memset(&screen->xrandr, 0, sizeof(screen->xrandr));
  screen->xrandr.primary = (rr_output == 0);
  screen->xrandr.output  = rr_output;
  screen->xrandr.name = xstrdup ("N/A");
}

void
xrandr_notify (XEvent *ev)
{
  (void)ev;
  fatal("xrandr_notify shouldn't be called.  This is a BUG.");
}

#endif /* HAVE_XRANDR */
