/* Sloppy focus
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

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int (*defaulthandler) (Display *, XErrorEvent *);

int
errorhandler (Display *display, XErrorEvent *error)
{
  if (error->error_code != BadWindow)
    (*defaulthandler) (display, error);
  return 0;
}

int
spawn (char *cmd)
{
  int pid;

  pid = fork ();
  if (pid == 0)
    {
      execl ("/bin/sh", "sh", "-c", cmd, (char *)NULL);
      _exit (EXIT_FAILURE);
    }
  return pid;
}

int
main (void)
{
  Display *display;
  int i, numscreens;

  display = XOpenDisplay (NULL);
  if (!display)
    {
      fprintf (stderr, "sloppy: could not open display\n");
      exit (1);
    }

  /* Ensure child shell will have $RATPOISON set. */
  if (!getenv ("RATPOISON"))
    setenv ("RATPOISON", "ratpoison", 0);

  defaulthandler = XSetErrorHandler (errorhandler);
  numscreens = ScreenCount (display);

  for (i = 0; i < numscreens; i++)
    {
      unsigned int j, nwins;
      Window dw1, dw2, *wins;

      XSelectInput (display, RootWindow (display, i),
                    SubstructureNotifyMask);
      XQueryTree (display, RootWindow (display, i),
                  &dw1, &dw2, &wins, &nwins);
      for (j = 0; j < nwins; j++)
	XSelectInput (display, wins[j], EnterWindowMask);
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
                            EnterWindowMask);
            }
	} while (event.type != EnterNotify);

      /* A window was entered. select it. */
      {
        char shell[256];

        snprintf (shell, sizeof(shell),
                  "$RATPOISON -c \"select $($RATPOISON -c 'windows %%i %%n %%f' | grep '%ld' | awk '$3 != '$($RATPOISON -c curframe)' && $3 != \"\" {print $2}')\" 2>/dev/null",
                  event.xcrossing.window);
        spawn (shell);
        wait (NULL);
      }
    }

  XCloseDisplay (display);

  return 0;
}

/*
Local Variables: ***
compile-command: "gcc -g -Wall -O2  -I/usr/X11R6/include -o sloppy sloppy.c -L/usr/X11R6/lib -lX11" ***
End: ***
*/
