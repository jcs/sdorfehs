/* Copyright (C) 2000 Shawn Betts
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "ratpoison.h"

extern Display *dpy;

void
spawn(char *prog)
{
  /*
   * ugly dance to avoid leaving zombies.  Could use SIGCHLD,
   * but it's not very portable.
   */
  if (fork() == 0) {
    if (fork() == 0) {
      putenv(DisplayString(dpy));
      execlp(prog, prog, 0);
      fprintf(stderr, "ratpoison: exec %s ", prog);
      perror(" failed");
      exit(EXIT_FAILURE);
    }
    exit(0);
  }
  wait((int *) 0);
#ifdef DEBUG
  printf ("spawned %s\n", prog);
#endif
}

void
new_window (XCreateWindowEvent *e)
{
  rp_window *win;
  screen_info *s;

  if (e->override_redirect) return;

  s = find_screen (e->parent);
  win = find_window (e->window);

  if (s && !win && e->window != s->key_window && e->window != s->bar_window)
    {
      win = add_to_window_list (s, e->window);
      win->state = STATE_UNMAPPED;
    }
}

void 
unmap_notify (XEvent *ev)
{
  screen_info *s;
  rp_window *win;

  s = find_screen (ev->xunmap.event);
  win = find_window (ev->xunmap.window);

  if (s && win)
    {
      win->state = STATE_UNMAPPED;
      update_window_names (s);
    }
}

void 
map_request (XEvent *ev)
{
  screen_info *s;
  rp_window *win;

  s = find_screen (ev->xmap.event);
  win = find_window (ev->xmap.window);

  if (s && win) 
    {
      switch (win->state)
	{
	case STATE_UNMAPPED:
	  manage (win, s);
	case STATE_MAPPED:
	  XMapRaised (dpy, win->w);
	  rp_current_window = win;
	  set_active_window (rp_current_window);
	}
    }
  else
    {
      printf ("Not managed.\n");
      XMapWindow (dpy, ev->xmap.window);
    }
}

int
more_destroy_events ()
{
  XEvent ev;

  if (XCheckTypedEvent (dpy, DestroyNotify, &ev))
    {
      XPutBackEvent (dpy, &ev);
      return 1;
    }
  return 0;
}

void
destroy_window (XDestroyWindowEvent *ev)
{
  screen_info *s;
  rp_window *win;

  s = find_screen (ev->event);
  win = find_window (ev->window);
  if (s && win)
    {
      /* Goto the last accessed window. */
      if (win == rp_current_window) 
	{
	  printf ("Destroying current window.\n");
	  
	  /* If there are more DestroyNotify events, then it is unsafe
             to go to the last window since it could be
             deleted. Therefore, wait until the last DestroyNotify
             event and then switch windows. */
	  if (!more_destroy_events ()) last_window (); 
	  unmanage (win);
	}
      else
	{
	  printf ("Destroying some other window.\n");
	  unmanage (win);
	}
    }
}

void
configure_request (XConfigureRequestEvent *e)
{
  XConfigureEvent ce;
  rp_window *win;

  win = find_window (e->window);

  if (win)
    {
      ce.type = ConfigureNotify;
      ce.event = e->window;
      ce.window = e->window;
      ce.x = 0;
      ce.y = 0;
      ce.width = win->scr->root_attr.width;
      ce.height = win->scr->root_attr.height;
      ce.border_width = 0;      
      ce.above = None;
      ce.override_redirect = 0;

      if (e->value_mask & CWStackMode && win->state == STATE_MAPPED) 
	{
	  if (e->detail == Above)
	    {
	      rp_current_window = win;
	      set_active_window (rp_current_window);
	    }
	  else if (e->detail == Below && win == rp_current_window) 
	    {
	      last_window ();
	    }
	}

      XSendEvent(dpy, win->w, False, StructureNotifyMask, (XEvent*)&ce);
    }
}

void
delete_window ()
{
  XEvent ev;
  int status;

  if (rp_current_window == NULL) return;

  ev.xclient.type = ClientMessage;
  ev.xclient.window = rp_current_window->w;
  ev.xclient.message_type = wm_protocols;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = wm_delete;
  ev.xclient.data.l[1] = CurrentTime;

  status = XSendEvent(dpy, rp_current_window->w, False, 0, &ev);
  if (status == 0) fprintf(stderr, "ratpoison: XSendEvent failed\n");
}

void 
kill_window ()
{
  if (rp_current_window == NULL) return;

  XKillClient(dpy, rp_current_window->w);
}

static void
client_msg (XClientMessageEvent *ev)
{
  printf ("Recieved client message.\n");
}

static void
handle_key (screen_info *s)
{
  int revert;
  Window fwin;
  XEvent ev;

#ifdef DEBUG
  printf ("handling key.\n");
#endif

  XGetInputFocus (dpy, &fwin, &revert);
  XSetInputFocus (dpy, s->key_window, RevertToPointerRoot, CurrentTime);
  XMaskEvent (dpy, KeyPressMask, &ev);
  XSetInputFocus (dpy, fwin, revert, CurrentTime);

  if (XLookupKeysym((XKeyEvent *) &ev, 0) == KEY_PREFIX && !ev.xkey.state)
    {
      /* Generate the prefix keystroke for the app */
      ev.xkey.window = fwin;
      ev.xkey.state = MODIFIER_PREFIX;
      XSendEvent (dpy, fwin, False, KeyPressMask, &ev);
      XSync (dpy, False);
      return;
    }

  if (XLookupKeysym((XKeyEvent *) &ev, 0) >= '0' 
      && XLookupKeysym((XKeyEvent *) &ev, 0) <= '9')
    {
      goto_window_number (XLookupKeysym((XKeyEvent *) &ev, 0) - '0');
      return;
    }

  switch (XLookupKeysym((XKeyEvent *) &ev, 0))
    {
    case KEY_XTERM:
      spawn (TERM_PROG);
      break;
    case KEY_EMACS:
      spawn (EMACS_PROG);
      break;
    case KEY_PREVWINDOW:
      prev_window ();
      break;
    case KEY_NEXTWINDOW:
      next_window ();
      break;
    case KEY_TOGGLEBAR:
      toggle_bar (s);
      break;
    case KEY_LASTWINDOW:
      last_window ();
      break;
    case KEY_DELETE:
      if (ev.xkey.state & ShiftMask) kill_window ();
      else delete_window ();
      break;
    default:
      fprintf (stderr, "Unknown key command %c", (char)XKeycodeToKeysym(dpy, ev.xkey.keycode, 0));
      break;
    }
}

void
key_press (XEvent *ev)
{
  screen_info *s;
  unsigned int modifier = ev->xkey.state;
  int ks = XLookupKeysym((XKeyEvent *) ev, 0);

  s = find_screen (ev->xkey.root);

  if (s && ks == KEY_PREFIX && (modifier & MODIFIER_PREFIX))
    {
      handle_key (s);
    }
}

void
property_notify (XEvent *ev)
{
  rp_window *win;

  printf ("atom: %ld\n", ev->xproperty.atom);

  win = find_window (ev->xproperty.window);
  if (win)
    {
      if (ev->xproperty.atom == XA_WM_NAME) 
	{
	  printf ("updating window name\n");
	  update_window_name (win);      
	}
    }
}

/* Given an event, call the correct function to handle it. */
void
delegate_event (XEvent *ev)
{
  switch (ev->type)
    {
    case ConfigureRequest:
      printf ("ConfigureRequest\n");
      configure_request (&ev->xconfigurerequest);
      break;
    case CirculateRequest:
      printf ("CirculateRequest\n");
      break;
    case CreateNotify:
      printf ("CreateNotify\n");
      new_window (&ev->xcreatewindow);
      break;
    case DestroyNotify:
      printf ("DestroyNotify\n");
      destroy_window (&ev->xdestroywindow);
      break;
    case ClientMessage:
      client_msg (&ev->xclient);
      printf ("ClientMessage\n");
      break;
    case ColormapNotify:
      printf ("ColormapNotify\n");
      break;
    case PropertyNotify:
      printf ("PropertyNotify\n");
      property_notify (ev);
      break;
    case SelectionClear:
      printf ("SelectionClear\n");
      break;
    case SelectionNotify:
      printf ("SelectionNotify\n");
      break;
    case SelectionRequest:
      printf ("SelectionRequest\n");
      break;
    case EnterNotify:
      printf ("EnterNotify\n");
      break;
    case ReparentNotify:
      printf ("ReparentNotify\n");
      break;
    case FocusIn:
      printf ("FocusIn\n");
      break;

    case MapRequest:
      printf ("MapRequest\n");
      map_request (ev);
      break;

    case KeyPress:
      printf ("KeyPress\n");
      key_press (ev);
      break;
      
    case UnmapNotify:
      printf ("UnmapNotify\n");
      unmap_notify (ev);
      break;

    case MotionNotify:
      printf ("MotionNotify\n");
      break;
    case Expose:
      printf ("Expose\n");
      break;
    case FocusOut:
      printf ("FocusOut\n");
      break;
    case ConfigureNotify:
      printf ("ConfigureNotify\n");
      break;
    case MapNotify:
      printf ("MapNotify\n");
      break;
    case MappingNotify:
      printf ("MappingNotify\n");
      break;
    default:
      printf ("Unhandled event %d\n", ev->type);
    }
}

void
handle_events ()
{
  XEvent ev;

  for (;;) 
    {
      XNextEvent (dpy, &ev);
      delegate_event (&ev);
    }
}


