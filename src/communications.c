/* communications.c -- Send commands to a running copy of ratpoison.
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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include "ratpoison.h"

void
send_restart ()
{
  XEvent ev;
  int status;

  ev.xclient.type = ClientMessage;
  ev.xclient.window = DefaultRootWindow (dpy);
  ev.xclient.message_type = rp_restart;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = rp_restart;
  ev.xclient.data.l[1] = CurrentTime;

  status = XSendEvent (dpy, DefaultRootWindow (dpy), False, SubstructureRedirectMask, &ev); 
  if (status == 0)
    {
      PRINT_ERROR ("failed to send restart event\n");
    }
}

void
send_kill ()
{
  XEvent ev;
  int status;

  ev.xclient.type = ClientMessage;
  ev.xclient.window = DefaultRootWindow (dpy);
  ev.xclient.message_type = rp_kill;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = rp_kill;
  ev.xclient.data.l[1] = CurrentTime;

  status = XSendEvent (dpy, DefaultRootWindow (dpy), False, SubstructureRedirectMask, &ev); 
  if (status == 0)
    {
      PRINT_ERROR ("failed to send kill event\n");
    }
}

int
send_command (unsigned char *cmd)
{
  return XChangeProperty (dpy, DefaultRootWindow (dpy), rp_command, XA_STRING,
			  8, PropModeAppend, cmd, strlen (cmd) + 1);
}
