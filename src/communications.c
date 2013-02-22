/* communications.c -- Send commands to a running copy of ratpoison.
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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include <string.h>

#include "ratpoison.h"


/* Sending commands to ratpoison */
static int
receive_command_result (Window w)
{
  int query;
  int return_status = RET_FAILURE;
  Atom type_ret;
  int format_ret;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *result = NULL;

  /* First, find out how big the property is. */
  query = XGetWindowProperty (dpy, w, rp_command_result,
			      0, 0, False, xa_string,
			      &type_ret, &format_ret, &nitems, &bytes_after,
			      &result);

  /* Failed to retrieve property. */
  if (query != Success || result == NULL)
    {
      PRINT_DEBUG (("failed to get command result length\n"));
      return return_status;
    }

  /* XGetWindowProperty always allocates one extra byte even if
     the property is zero length. */
  XFree (result);

  /* Now that we have the length of the message, we can get the
     whole message. */
  query = XGetWindowProperty (dpy, w, rp_command_result,
			      0, (bytes_after / 4) + (bytes_after % 4 ? 1 : 0),
			      True, xa_string, &type_ret, &format_ret, &nitems,
			      &bytes_after, &result);

  /* Failed to retrieve property. */
  if (query != Success || result == NULL)
    {
      PRINT_DEBUG (("failed to get command result\n"));
      return return_status;
    }

  /*
   * We can receive:
   * - an empty string, indicating a success but no output
   * - a string starting with '1', indicating a success and an output
   * - a string starting with '0', indicating a failure and an optional output
   */
  switch (result[0])
    {
    case '\0': /* Command succeeded but no string to print */
      return_status = RET_SUCCESS;
      break;
    case '0': /* Command failed, don't print an empty line if no explanation
		 was given */
      if (result[1] != '\0')
	fprintf (stderr, "%s\n", &result[1]);
      return_status = RET_FAILURE;
      break;
    case '1': /* Command succeeded, print the output */
      printf ("%s\n", &result[1]);
      return_status = RET_SUCCESS;
      break;
    default: /* We probably got junk, so ignore it */
      return_status = RET_FAILURE;
    }

  /* Free the result. */
  XFree (result);

  return return_status;
}

int
send_command (unsigned char interactive, unsigned char *cmd, int screen_num)
{
  Window w, root;
  int done = 0, return_status = RET_FAILURE;
  struct sbuf *s;

  s = sbuf_new(0);
  sbuf_printf(s, "%c%s", interactive, cmd);


  /* If the user specified a specific screen, then send the event to
     that screen. */
  if (screen_num >= 0)
    root = RootWindow (dpy, screen_num);
  else
    root = DefaultRootWindow (dpy);

  w = XCreateSimpleWindow (dpy, root, 0, 0, 1, 1, 0, 0, 0);

  /* Select first to avoid race condition */
  XSelectInput (dpy, w, PropertyChangeMask);

  XChangeProperty (dpy, w, rp_command, xa_string,
                   8, PropModeReplace, (unsigned char*)sbuf_get(s), strlen ((char *)cmd) + 2);

  XChangeProperty (dpy, root,
                   rp_command_request, XA_WINDOW,
                   8, PropModeAppend, (unsigned char *)&w, sizeof (Window));

  sbuf_free (s);

  while (!done)
    {
      XEvent ev;

      XMaskEvent (dpy, PropertyChangeMask, &ev);
      if (ev.xproperty.atom == rp_command_result
          && ev.xproperty.state == PropertyNewValue)
        {
	  return_status = receive_command_result(ev.xproperty.window);
	  done = 1;
        }
    }

  XDestroyWindow (dpy, w);

  return return_status;
}
