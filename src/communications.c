/* communications.c -- Send commands to a running copy of ratpoison.
 * Copyright (C) 2000, 2001, 2002, 2003 Shawn Betts
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
static void
receive_command_result (Window w)
{
  int status;
  Atom type_ret;
  int format_ret;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *result = NULL;

  /* First, find out how big the property is. */
  status = XGetWindowProperty (dpy, w, rp_command_result,
			       0, 0, False, XA_STRING,
			       &type_ret, &format_ret, &nitems, &bytes_after,
			       &result);

  /* Failed to retrieve property. */
  if (status != Success || result == NULL)
    {
      PRINT_DEBUG (("failed to get command result length\n"));
      return;
    }

  /* XGetWindowProperty always allocates one extra byte even if
     the property is zero length. */
  XFree (result);

  /* Now that we have the length of the message, we can get the
     whole message. */
  status = XGetWindowProperty (dpy, w, rp_command_result,
			       0, (bytes_after / 4) + (bytes_after % 4 ? 1 : 0),
			       True, XA_STRING, &type_ret, &format_ret, &nitems, 
			       &bytes_after, &result);

  /* Failed to retrieve property. */
  if (status != Success || result == NULL)
    {
      PRINT_DEBUG (("failed to get command result\n"));
      return;
    }

  /* If result is not the empty string, print it. */
  if (strlen (result))
    printf ("%s\n", result);

  /* Free the result. */
  XFree (result);
}

int
send_command (unsigned char interactive, unsigned char *cmd, int screen_num)
{
  Window w, root;
  int done = 0;
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

  XChangeProperty (dpy, w, rp_command, XA_STRING,
		   8, PropModeReplace, sbuf_get(s), strlen (cmd) + 2);

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
	  receive_command_result(ev.xproperty.window);
	  done = 1;
	}
    }

  XDestroyWindow (dpy, w);

  return 1;
}
