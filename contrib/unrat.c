/* Warp the pointer to the bottom right corner of the screen any time a key is pressed.
 *
 * Copyright (C) 2005 Shawn Betts <sabetts@vcn.bc.ca>
 *
 * unrat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * unrat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 */

/*
This is what I used to compile it:

gcc -g -Wall -O2  -I/usr/X11R6/include -o unrat unrat.c -L /usr/X11R6/lib -lX11
*/

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <stdio.h>
#include <stdlib.h>

int (*defaulthandler) (Display *, XErrorEvent *);

int
errorhandler (Display *display, XErrorEvent *error)
{
  if (error->error_code != BadWindow)
    (*defaulthandler) (display,error);

  return 0;
}

int
main (void)
{
  Display *display;
  int i, numscreens;

  display = XOpenDisplay (NULL);
  if (!display)
    {
      fprintf (stderr, "unrat: could not open display\n");
      exit (1);
    }

  defaulthandler = XSetErrorHandler (errorhandler);
  numscreens = ScreenCount (display);

  for (i = 0; i < numscreens; i++)
    {
      unsigned int j, nwins;
      Window dw1, dw2, *wins;

      XSelectInput (display, RootWindow (display, i),
                    KeyReleaseMask | SubstructureNotifyMask);
      XQueryTree (display, RootWindow (display, i),
                  &dw1, &dw2, &wins, &nwins);
      for (j = 0; j < nwins; j++)
        XSelectInput (display, wins[j], KeyReleaseMask);
    }

  while (1)
    {
      XEvent event;
      do
        {
          XNextEvent (display, &event);
          if (event.type == CreateNotify)
            {
              XSelectInput (display, event.xcreatewindow.window,
                          KeyReleaseMask);
            }
        } while (event.type != KeyRelease);

      /* A key was pressed. warp the rat. */
      for (i = 0; i < numscreens; i++)
        {
          int x, y, wx, wy;
          unsigned int mask;
          Window root, child;

          XQueryPointer (display, RootWindow (display, i),
                         &root, &child,
                         &x, &y, &wx, &wy,
                         &mask);
          if (x < DisplayWidth (display, i) - 1
              || y < DisplayHeight (display, i) - 1)
            {
              XWarpPointer (display, None, RootWindow (display, i),
                            0, 0, 0, 0,
                            DisplayWidth (display, i),
                            DisplayHeight (display, i));
            }
        }
    }

  XCloseDisplay (display);

  return 0;
}
