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
#include <X11/keysym.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ratpoison.h"

extern Display *dpy;

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
      long data[2] = { WithdrawnState, None };

      /* Give back the window number. the window will get another one,
         if it in remapped. */
      return_window_number (win->number);
      win->number = -1;
      win->state = STATE_UNMAPPED;
      
      /* Update the state of the actual window */
      XRemoveFromSaveSet (dpy, win->w);
      XChangeProperty(dpy, win->w, wm_state, wm_state, 32,
		      PropModeReplace, (unsigned char *)data, 2);

      ignore_badwindow = 1;
      XSync(dpy, False);
      ignore_badwindow = 0;

      if (rp_current_window == win)
	{
	  last_window (NULL);
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
      switch (win->state)
	{
	case STATE_UNMAPPED:
	  if (unmanaged_window (win->w))
	    {
	      XMapRaised (dpy, win->w);
	      break;
	    }
	  else
	    manage (win, s);	/* fall through */
	case STATE_MAPPED:
	  XMapRaised (dpy, win->w);
	  rp_current_window = win;
	  set_active_window (rp_current_window);
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

	  /* Switch to last viewed window */
	  ignore_badwindow = 1;
	  last_window (NULL);
	  ignore_badwindow = 0;
	}
      else
	{
	  PRINT_DEBUG ("Destroying some other window.\n");
	  unmanage (win);
	}
    }
}

void
configure_request (XConfigureRequestEvent *e)
{
  XWindowChanges wc;
  XConfigureEvent ce;
  rp_window *win;

  win = find_window (e->window);

  if (win)
    {
      PRINT_DEBUG ("window req: %d %d %d %d %d\n", e->x, e->y, e->width, e->height, e->border_width);

      wc.x = PADDING_LEFT;
      wc.y = PADDING_TOP;
      wc.width = win->scr->root_attr.width - PADDING_LEFT - PADDING_RIGHT;
      wc.height = win->scr->root_attr.height - PADDING_TOP - PADDING_BOTTOM;
      wc.border_width = 0;

      ce.type = ConfigureNotify;
      ce.event = e->window;
      ce.window = e->window;
      ce.x = PADDING_LEFT;
      ce.y = PADDING_TOP;
      ce.width = win->scr->root_attr.width - PADDING_LEFT - PADDING_RIGHT;
      ce.height = win->scr->root_attr.height - PADDING_TOP - PADDING_BOTTOM;
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
	      last_window (NULL);
	    }
	}

      XSendEvent(dpy, win->w, False, StructureNotifyMask, (XEvent*)&ce);
      XConfigureWindow (dpy, win->w, 
			CWX | CWY | CWWidth | CWHeight | CWBorderWidth,
			&wc);
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
  const rp_action *i;
  int revert;
  Window fwin;
  XEvent ev;
  int keysym;

  PRINT_DEBUG ("handling key.\n");

  /* All functions hide the program bar. */
  if (BAR_TIMEOUT > 0) hide_bar (s);

  XGetInputFocus (dpy, &fwin, &revert);
  XSetInputFocus (dpy, s->key_window, RevertToPointerRoot, CurrentTime);

  do
    {  
      XMaskEvent (dpy, KeyPressMask, &ev);
      keysym = XLookupKeysym((XKeyEvent *) &ev, 0);
    } while (keysym == XK_Shift_L      
	     || keysym == XK_Shift_R      
	     || keysym == XK_Control_L    
	     || keysym == XK_Control_R    
	     || keysym == XK_Caps_Lock    
	     || keysym == XK_Shift_Lock   
	     || keysym == XK_Meta_L       
	     || keysym == XK_Meta_R       
	     || keysym == XK_Alt_L        
	     || keysym == XK_Alt_R        
	     || keysym == XK_Super_L      
	     || keysym == XK_Super_R      
	     || keysym == XK_Hyper_L      
	     || keysym == XK_Hyper_R); /* ignore modifier keypresses. */

  XSetInputFocus (dpy, fwin, revert, CurrentTime);

  if (keysym == KEY_PREFIX && !ev.xkey.state)
    {
      /* Generate the prefix keystroke for the app */
      ev.xkey.window = fwin;
      ev.xkey.state = MODIFIER_PREFIX;
      XSendEvent (dpy, fwin, False, KeyPressMask, &ev);
      XSync (dpy, False);
      return;
    }

  for (i = key_actions; i->key != 0; i++)
    {
      if (keysym == i->key)
	if (i->state == -1 || ev.xkey.state == i->state)
	  {
	    (*i->func)(i->data);
	    break;
	  }
    }
}

void
key_press (XEvent *ev)
{
  screen_info *s;
  unsigned int modifier = ev->xkey.state;
  int ks = XLookupKeysym((XKeyEvent *) ev, 0);

  s = find_screen (ev->xkey.root);

  if (rat_visible)
    {
      XWarpPointer (dpy, None, s->root, 0, 0, 0, 0, s->root_attr.width, s->root_attr.height);
/*        rat_visible = 0;  */
    }

  if (!s) return;

  if (ks == KEY_PREFIX && (modifier & MODIFIER_PREFIX))
    {
      handle_key (s);
    }
  else
    {
      if (rp_current_window)
	{
	  ev->xkey.window = rp_current_window->w;
	  XSendEvent (dpy, rp_current_window->w, False, KeyPressMask, ev);
	  XSync (dpy, False);
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
      if (ev->xproperty.atom == XA_WM_NAME) 
	{
	  PRINT_DEBUG ("updating window name\n");
	  if (update_window_name (win))
	    {
	      update_window_names (win->scr);
	    }
	}
    }
}

void
rat_motion (XMotionEvent *ev)
{
  if (!rat_visible)
    {
      XWarpPointer (dpy, None, ev->root, 0, 0, 0, 0, rat_x, rat_y);
      /*  rat_visible = 1; */
    }

  rat_x = ev->x_root;
  rat_y = ev->y_root;
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
      
    case UnmapNotify:
      PRINT_DEBUG ("UnmapNotify\n");
      unmap_notify (ev);
      break;

    case MotionNotify:
      PRINT_DEBUG ("MotionNotify\n");
      rat_motion (&ev->xmotion);
      break;

    case Expose:
      PRINT_DEBUG ("Expose\n");
      break;
    case FocusOut:
      PRINT_DEBUG ("FocusOut\n");
      break;
    case ConfigureNotify:
      PRINT_DEBUG ("ConfigureNotify\n");
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
      delegate_event (&ev);
    }
}


