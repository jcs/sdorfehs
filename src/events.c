/* Ratpoison X events
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
#include <X11/keysym.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ratpoison.h"

/* The event currently being processed. Mostly used in functions from
   action.c which need to forward events to other windows. */
XEvent *rp_current_event;

void
new_window (XCreateWindowEvent *e)
{
  rp_window *win;
  screen_info *s;

  if (e->override_redirect) return;

  s = find_screen (e->parent);

  win = find_window (e->window);

  if (s && !win && e->window != s->key_window && e->window != s->bar_window 
      && e->window != s->input_window)
    {
      win = add_to_window_list (s, e->window);
      update_window_information (win);
    }
}

void 
unmap_notify (XEvent *ev)
{
  screen_info *s;
  rp_window *win;

  s = find_screen (ev->xunmap.event);

  /* FIXME: Should we only look in the mapped window list? */
  win = find_window_in_list (ev->xunmap.window, rp_mapped_window_sentinel);

  if (s && win)
    {
      long data[2] = { WithdrawnState, None };

      /* Give back the window number. the window will get another one,
         if it in remapped. */
      return_window_number (win->number);
      win->number = -1;
      win->state = STATE_UNMAPPED;

      remove_from_list (win);
      append_to_list (win, rp_unmapped_window_sentinel);
      
      /* Update the state of the actual window */
      ignore_badwindow = 1;

      XRemoveFromSaveSet (dpy, win->w);
      XChangeProperty(dpy, win->w, wm_state, wm_state, 32,
		      PropModeReplace, (unsigned char *)data, 2);

      XSync(dpy, False);

      ignore_badwindow = 0;

      if (rp_current_window == win)
	{
	  cmd_other (NULL);
	}

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
      PRINT_DEBUG ("Map from a managable window\n");

      switch (win->state)
	{
	case STATE_UNMAPPED:
	  PRINT_DEBUG ("Unmapped window\n");
	  if (unmanaged_window (win->w))
	    {
	      PRINT_DEBUG ("Unmanaged Window\n");
	      XMapWindow (dpy, win->w);
	      break;
	    }
	  else
	    {
	      PRINT_DEBUG ("managed Window\n");

	      map_window (win);
	      break;
	    }
	case STATE_MAPPED:
	  PRINT_DEBUG ("Mapped Window\n");

	  maximize (win);
	  XMapRaised (dpy, win->w);
	  set_state (win, NormalState);
	  set_active_window (win);
	  break;
	}
    }
  else
    {
      PRINT_DEBUG ("Not managed.\n");
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
  rp_window *win;

  win = find_window (ev->window);

  if (win)
    {
      /* Goto the last accessed window. */
      if (win == rp_current_window) 
	{
	  PRINT_DEBUG ("Destroying current window.\n");
	  
	  unmanage (win);

	  if (rp_mapped_window_sentinel->next == rp_mapped_window_sentinel
	      && rp_mapped_window_sentinel->prev == rp_mapped_window_sentinel)
	    {
	      rp_current_window = NULL;
	    }
	  else
	    {
	      /* Switch to last viewed window */
	      set_current_window( find_window_other() );
	    }
	}
      else
	{
	  PRINT_DEBUG ("Destroying some other window.\n");
	  unmanage (win);
	}
    }
}

void
configure_notify (XConfigureEvent *e)
{
  rp_window *win;

  win = find_window (e->window);
  if (win)
    {
      PRINT_DEBUG ("'%s' window notify: %d %d %d %d %d\n", win->name,
		   e->x, e->y, e->width, e->height, e->border_width);

      /* Once we get the notify that everything went through, try
	 maximizing. Netscape doesn't seem to like it here. */
/*       maximize (win); */
    }
}

void
configure_request (XConfigureRequestEvent *e)
{
  rp_window *win;

  win = find_window (e->window);

  if (win)
    {
      /* Updated our window struct */
      if (e->value_mask & CWX)
	{
	  win->x = e->x + win->border;
	  PRINT_DEBUG("request CWX %d\n", e->x);
	}
      if (e->value_mask & CWY)
	{
	  win->y = e->y + win->border;
	  PRINT_DEBUG("request CWY %d\n", e->y);
	}

      if (e->value_mask & CWStackMode && win->state == STATE_MAPPED)
	{
	  if (e->detail == Above)
	    {
	      set_active_window (win);
	    }
	  else if (e->detail == Below)
	    {
	      set_active_window (find_window_other ());
	    }

	  PRINT_DEBUG("request CWStackMode %d\n", e->detail);
	}


      PRINT_DEBUG ("'%s' new window size: %d %d %d %d %d\n", win->name,
		   win->x, win->y, win->width, win->height, win->border);

      if (e->value_mask & (CWWidth|CWHeight|CWBorderWidth))
	{
	  if (e->value_mask & CWBorderWidth)
	    {
	      win->border = e->border_width;
	      PRINT_DEBUG("request CWBorderWidth %d\n", e->border_width);
	    }
	  if (e->value_mask & CWWidth)
	    {
	      win->width = e->width;
	      PRINT_DEBUG("request CWWidth %d\n", e->width);
	    }
	  if (e->value_mask & CWHeight)
	    {
	      win->height = e->height;
	      PRINT_DEBUG("request CWHeight %d\n", e->height);
	    }

	  maximize (win);
	  send_configure (win);
	}
      else
	{
	  maximize (win);
	  send_configure (win);
	}
    }
  else
    {
      PRINT_DEBUG ("FIXME: Don't handle this\n");
    }
}

static void
client_msg (XClientMessageEvent *ev)
{
  PRINT_DEBUG ("Recieved client message.\n");

  if (ev->message_type == rp_restart)
    {
      PRINT_DEBUG ("Restarting\n");
      clean_up (); 
      execvp(myargv[0], myargv);
    }
  else if (ev->message_type == rp_kill)
    {
      PRINT_DEBUG ("Exiting\n");
      clean_up ();
      exit (EXIT_SUCCESS);
    }
}

static void
handle_key (screen_info *s)
{
  char *keysym_name;
  char *msg;
  rp_action *key_action;
  int revert;			
  Window fwin;			/* Window currently in focus */
  KeySym keysym;		/* Key pressed */
  unsigned int mod;		/* Modifiers */

  PRINT_DEBUG ("handling key...\n");

  /* All functions hide the program bar. Unless the bar doesn't time
     out. */
  if (BAR_TIMEOUT > 0) hide_bar (s);

  XGetInputFocus (dpy, &fwin, &revert);
  XSetInputFocus (dpy, s->key_window, RevertToPointerRoot, CurrentTime);

  read_key (&keysym, &mod, NULL, 0);

  if ((key_action = find_keybinding (keysym, mod)))
    {
      XSetInputFocus (dpy, fwin, revert, CurrentTime);
      command (key_action->data);
    }
  else
    {
      keysym_name = keysym_to_string (keysym, mod);
      msg = (char *) xmalloc ( strlen ( keysym_name ) + 20 );

      snprintf (msg, strlen (keysym_name) + 13, "%s unbound key!", keysym_name);
      free (keysym_name);

      PRINT_DEBUG ("%s\n", msg);

      /* No key match, notify user. */
      XSetInputFocus (dpy, fwin, revert, CurrentTime);
      message (msg);

      free (msg);
    }
}

void
key_press (XEvent *ev)
{
  screen_info *s;
  unsigned int modifier;
  KeySym ks;

  s = find_screen (ev->xkey.root);

#ifdef HIDE_MOUSE
  XWarpPointer (dpy, None, s->root, 0, 0, 0, 0, s->root_attr.width - 2, s->root_attr.height - 2); 
#endif

  if (!s) return;

  modifier = ev->xkey.state;
  cook_keycode ( &ev->xkey, &ks, &modifier, NULL, 0);

  if (ks == prefix_key.sym && (modifier == prefix_key.state))
    {
      handle_key (s);
    }
  else
    { 
      if (rp_current_window)
	{
	  ignore_badwindow = 1;
	  ev->xkey.window = rp_current_window->w;
	  XSendEvent (dpy, rp_current_window->w, False, KeyPressMask, ev);
	  XSync (dpy, False);
	  ignore_badwindow = 0;
	}
    }
}

void
property_notify (XEvent *ev)
{
  rp_window *win;

  PRINT_DEBUG ("atom: %ld\n", ev->xproperty.atom);

  win = find_window (ev->xproperty.window);
  
  if (win)
    {
      switch (ev->xproperty.atom)
	{
	case XA_WM_NAME:
	  PRINT_DEBUG ("updating window name\n");
	  update_window_name (win);
	  update_window_names (win->scr);
	  break;

	case XA_WM_NORMAL_HINTS:
	  PRINT_DEBUG ("updating window normal hints\n");
	  update_normal_hints (win);
	  break;

	case XA_WM_TRANSIENT_FOR:
	  PRINT_DEBUG ("Transient for\n");
	  break;

	default:
	  PRINT_DEBUG ("Unhandled property notify event\n");
	  break;
	}
    }
}

void
colormap_notify (XEvent *ev)
{
  rp_window *win;

  win = find_window (ev->xcolormap.window);

  if (win != NULL)
    {
      XWindowAttributes attr;

      XGetWindowAttributes (dpy, win->w, &attr);
      win->colormap = attr.colormap;

      if (win == rp_current_window)
	{
	  XInstallColormap (dpy, win->colormap);
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
      PRINT_DEBUG ("ConfigureRequest\n");
      configure_request (&ev->xconfigurerequest);
      break;
    case CirculateRequest:
      PRINT_DEBUG ("CirculateRequest\n");
      break;
    case CreateNotify:
      PRINT_DEBUG ("CreateNotify\n");
      new_window (&ev->xcreatewindow);
      break;
    case DestroyNotify:
      PRINT_DEBUG ("DestroyNotify\n");
      destroy_window (&ev->xdestroywindow);
      break;
    case ClientMessage:
      PRINT_DEBUG ("ClientMessage\n");
      client_msg (&ev->xclient);
      break;
    case ColormapNotify:
      PRINT_DEBUG ("ColormapNotify\n");
      colormap_notify (ev);
      break;
    case PropertyNotify:
      PRINT_DEBUG ("PropertyNotify\n");
      property_notify (ev);
      break;
    case SelectionClear:
      PRINT_DEBUG ("SelectionClear\n");
      break;
    case SelectionNotify:
      PRINT_DEBUG ("SelectionNotify\n");
      break;
    case SelectionRequest:
      PRINT_DEBUG ("SelectionRequest\n");
      break;
    case EnterNotify:
      PRINT_DEBUG ("EnterNotify\n");
      break;
    case ReparentNotify:
      PRINT_DEBUG ("ReparentNotify\n");
      break;
    case FocusIn:
      PRINT_DEBUG ("FocusIn\n");
      break;

    case MapRequest:
      PRINT_DEBUG ("MapRequest\n");
      map_request (ev);
      break;

    case KeyPress:
      PRINT_DEBUG ("KeyPress %d %d\n", ev->xkey.keycode, ev->xkey.state);
      key_press (ev);
      break;

    case KeyRelease: 
      PRINT_DEBUG ("KeyRelease %d %d\n", ev->xkey.keycode, ev->xkey.state);
      break;

    case UnmapNotify:
      PRINT_DEBUG ("UnmapNotify\n");
      unmap_notify (ev);
      break;

    case MotionNotify:
      PRINT_DEBUG ("MotionNotify\n");
      break;

    case Expose:
      PRINT_DEBUG ("Expose\n");
      break;
    case FocusOut:
      PRINT_DEBUG ("FocusOut\n");
      break;
    case ConfigureNotify:
      PRINT_DEBUG ("ConfigureNotify\n");
      configure_notify (&ev->xconfigure);
      break;
    case MapNotify:
      PRINT_DEBUG ("MapNotify\n");
      break;
    case MappingNotify:
      PRINT_DEBUG ("MappingNotify\n");
      break;
    default:
      PRINT_DEBUG ("Unhandled event %d\n", ev->type);
    }
}

void
handle_events ()
{
  XEvent ev;

  for (;;) 
    {
      XNextEvent (dpy, &ev);
      rp_current_event = &ev;
      delegate_event (&ev);
    }
}


