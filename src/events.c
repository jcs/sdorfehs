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
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#include "ratpoison.h"

/* The event currently being processed. Mostly used in functions from
   action.c which need to forward events to other windows. */
XEvent rp_current_event;

static void
new_window (XCreateWindowEvent *e)
{
  rp_window *win;
  screen_info *s;

  if (e->override_redirect) return;

  s = find_screen (e->parent);

  win = find_window (e->window);

  if (s && !win && e->window != s->key_window && e->window != s->bar_window 
      && e->window != s->input_window && e->window != s->frame_window && e->window != s->help_window)
    {
      win = add_to_window_list (s, e->window);
      update_window_information (win);
    }
}

static void
cleanup_frame (rp_window_frame *frame)
{
  rp_window *win;

  win = find_window_other ();
  if (win)
    {
      rp_window *last_win;
      last_win = frame->win;
      frame->win = win;

      maximize (win);
      unhide_transient_for (win);
      unhide_window (win);

      if (is_transient_ancestor (last_win, win))
	{
	  hide_transient_for_between (last_win, win);
	}
      else if (last_win->transient && win->transient &&
	       last_win->transient_for == win->transient_for)
	{
	  /* Both last_win and win have the same transient_for so we
             don't need to hide anything more */
	}
      else
	{
	  hide_transient_for (last_win);
	}
    }
  else
    {
      hide_transient_for (frame->win);
      frame->win = NULL;
    }
}

static void 
unmap_notify (XEvent *ev)
{
  screen_info *s;
  rp_window *win;

  /* ignore SubstructureNotify unmaps. */
  if(ev->xunmap.event != ev->xunmap.window 
     && ev->xunmap.send_event != True) return;

  s = find_screen (ev->xunmap.event);

  /* FIXME: Should we only look in the mapped window list? */
  win = find_window_in_list (ev->xunmap.window, rp_mapped_window_sentinel);

  if (s && win)
    {
      rp_window_frame *frame;

      switch (win->state)
	{
	case IconicState:
	  PRINT_DEBUG ("Withdrawing iconized window '%s'\n", win->name);
	  if (ev->xunmap.send_event) withdraw_window (win);
	  break;
	case NormalState:
	  PRINT_DEBUG ("Withdrawing normal window '%s'\n", win->name);
	  /* If the window was inside a frame, fill the frame with another
	     window. */
	  frame = find_windows_frame (win);
	  if (frame) cleanup_frame (frame);
	  if (frame == rp_current_frame) set_active_frame (frame);

	  withdraw_window (win);
	  break;
	}

      update_window_names (s);
    }
}

static void 
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
	case WithdrawnState:
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
	case NormalState:
	  PRINT_DEBUG ("Mapped Window\n");
	  /* Its already mapped, so we don't have to do anything */

	  /* 	  maximize (win); */
	  /* 	  XMapRaised (dpy, win->w); */
	  /* 	  set_state (win, NormalState); */
	  /* 	  set_active_window (win); */
	  break;
	case IconicState:
	  PRINT_DEBUG ("Mapped iconic window\n");
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

static void
destroy_window (XDestroyWindowEvent *ev)
{
  rp_window *win;
  ignore_badwindow++;

  win = find_window (ev->window);

  if (win)
    {
      rp_window_frame *frame;

      frame = find_windows_frame (win);
      if (frame) cleanup_frame (frame);
      if (frame == rp_current_frame) set_active_frame (frame);

      unmanage (win);
    }

  ignore_badwindow--;
}

static void
configure_notify (XConfigureEvent *e)
{
  rp_window *win;

  win = find_window (e->window);
  if (win)
    {
      PRINT_DEBUG ("'%s' window notify: %d %d %d %d %d\n", win->name,
		   e->x, e->y, e->width, e->height, e->border_width);
    }
}

static void
configure_request (XConfigureRequestEvent *e)
{
  XWindowChanges changes;
  rp_window *win;

  win = find_window (e->window);

  if (win)
    {
      /* Updated our window struct */
      if (e->value_mask & CWX)
	{
	  changes.x = e->x + win->border;
	  win->x = e->x + win->border;
	  PRINT_DEBUG("request CWX %d\n", e->x);
	}
      if (e->value_mask & CWY)
	{
	  changes.y = e->y + win->border;
	  win->y = e->y + win->border;
	  PRINT_DEBUG("request CWY %d\n", e->y);
	}

      if (e->value_mask & CWStackMode && win->state == NormalState)
	{
	  if (e->detail == Above)
	    {
	      goto_window (win);
	    }
	  else if (e->detail == Below)
	    {
	      set_active_window (find_window_other ());
	    }

	  PRINT_DEBUG("request CWStackMode %d\n", e->detail);
	}

      if (e->value_mask & CWBorderWidth)
	{
	  changes.border_width = e->border_width;
	  win->border = e->border_width;
	  PRINT_DEBUG("request CWBorderWidth %d\n", e->border_width);
	}
      if (e->value_mask & CWWidth)
	{
	  changes.width = e->width;
	  win->width = e->width;
	  PRINT_DEBUG("request CWWidth %d\n", e->width);
	}
      if (e->value_mask & CWHeight)
	{
	  changes.height = e->height;
	  win->height = e->height;
	  PRINT_DEBUG("request CWHeight %d\n", e->height);
	}

      PRINT_DEBUG ("'%s' new window size: %d %d %d %d %d\n", win->name,
		   win->x, win->y, win->width, win->height, win->border);

      if (win->state != NormalState)
	{
	  /* The window isn't visible so grant it whatever it likes */
	  XConfigureWindow (dpy, win->w, e->value_mask & (CWX|CWY|CWBorderWidth|CWWidth|CWHeight), 
			    &changes);
	  send_configure (win);
	}
      else
	{
	  /* Draw the hard line. Get back in line, you misbehaving window! */
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
  PRINT_DEBUG ("Received client message.\n");

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
  else if (ev->message_type == wm_change_state)
    {
      rp_window *win;

      win = find_window (ev->window);
      if (win == NULL) return;
      if (ev->format == 32 && ev->data.l[0] == IconicState)
	{
	  if (win->state == NormalState)
	    {
	      /* TODO: Handle iconify events */
	    }
	}
      else
	{
	  PRINT_ERROR ("Non-standard WM_CHANGE_STATE format\n");
	}
    }
}

#ifdef USE_WAITFORKEY_CURSOR
static void
grab_rat ()
{
  XGrabPointer (dpy, current_screen()->root, True, 0, 
		GrabModeAsync, GrabModeAsync, 
		None, current_screen()->rat, CurrentTime);
}
#endif

#ifdef USE_WAITFORKEY_CURSOR
static void
ungrab_rat ()
{
  XUngrabPointer (dpy, CurrentTime);
}
#endif

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

  /* All functions hide the program bar and the frame indicator. */
  if (BAR_TIMEOUT > 0) hide_bar (s);
  hide_frame_indicator();

  XGetInputFocus (dpy, &fwin, &revert);
  XSetInputFocus (dpy, s->key_window, RevertToPointerRoot, CurrentTime);

  /* Change the mouse icon to indicate to the user we are waiting for
     more keystrokes */
#ifdef USE_WAITFORKEY_CURSOR
  grab_rat();
#endif

  read_key (&keysym, &mod, NULL, 0);

  if ((key_action = find_keybinding (keysym, mod)))
    {
      char *result;
      XSetInputFocus (dpy, fwin, revert, CurrentTime);
      result = command (1, key_action->data);
      
      /* Gobble the result. */
      if (result)
	free (result);
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

#ifdef USE_WAITFORKEY_CURSOR
  ungrab_rat();
#endif
}

static void
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
      if (current_window())
	{
	  ignore_badwindow++;
	  ev->xkey.window = current_window()->w;
	  XSendEvent (dpy, current_window()->w, False, KeyPressMask, ev);
	  XSync (dpy, False);
	  ignore_badwindow--;
	}
    }
}

/* Read a command off the window and execute it. Some commands return
   text. This text is passed back using the RP_COMMAND_RESULT
   Atom. The client will wait for this property change so something
   must be returned. */
static char *
execute_remote_command (Window w)
{
  char *result = NULL;
  Atom type_ret;
  int format_ret;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *req;

  if (XGetWindowProperty (dpy, w, rp_command,
			  0, 0, False, XA_STRING,
			  &type_ret, &format_ret, &nitems, &bytes_after,
			  &req) == Success
      &&
      XGetWindowProperty (dpy, w, rp_command,
			  0, (bytes_after / 4) + (bytes_after % 4 ? 1 : 0),
			  True, XA_STRING, &type_ret, &format_ret, &nitems, 
			  &bytes_after, &req) == Success)
    {
      if (req)
	{
	  PRINT_DEBUG ("command: %s\n", req);
	  result = command (0, req);
	}
      XFree (req);
    }
  else
    {
      PRINT_DEBUG ("Couldn't get RP_COMMAND Property\n");
    }

  return result;
}

/* Command requests are posted as a property change using the
   RP_COMMAND_REQUEST Atom on the root window. A Command request is a
   Window that holds the actual command as a property using the
   RP_COMMAND Atom. receive_command reads the list of Windows and
   executes their associated command. */
static void
receive_command ()
{
  char *result;
  Atom type_ret;
  int format_ret;
  unsigned long nitems;
  unsigned long bytes_after;
  void *prop_return;

  do
    {
      if (XGetWindowProperty (dpy, DefaultRootWindow (dpy),
			      rp_command_request, 0, 
			      sizeof (Window) / 4 + (sizeof (Window) % 4 ?1:0),
			      True, XA_WINDOW, &type_ret, &format_ret, &nitems,
			      &bytes_after, (unsigned char **)&prop_return) == Success)
	{
	  if (prop_return)
	    {
	      Window w;

	      w = *(Window *)prop_return;
	      XFree (prop_return);

	      result = execute_remote_command (w);
	      if (result)
		{
		  XChangeProperty (dpy, w, rp_command_result, XA_STRING,
				   8, PropModeReplace, result, strlen (result));
		  free (result);
		}
	      else
		{
		  XChangeProperty (dpy, w, rp_command_result, XA_STRING,
				   8, PropModeReplace, NULL, 0);
		}
	    }
	  else
	    {
	      PRINT_DEBUG ("Couldn't get RP_COMMAND_REQUEST Property\n");
	    }

	  PRINT_DEBUG ("command requests: %ld\n", nitems);
	}
    } while (nitems > 0);
			  
}

static void
property_notify (XEvent *ev)
{
  rp_window *win;

  PRINT_DEBUG ("atom: %ld\n", ev->xproperty.atom);

  if (ev->xproperty.atom == rp_command_request
      && ev->xproperty.window == DefaultRootWindow (dpy)
      && ev->xproperty.state == PropertyNewValue)
    {
      PRINT_DEBUG ("ratpoison command\n");
      receive_command();
    }

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

static void
colormap_notify (XEvent *ev)
{
  rp_window *win;

  win = find_window (ev->xcolormap.window);

  if (win != NULL)
    {
      XWindowAttributes attr;

      /* SDL sets the colormap just before destroying the window, so
	 ignore BadWindow errors. */
      ignore_badwindow++;

      XGetWindowAttributes (dpy, win->w, &attr);
      win->colormap = attr.colormap;

      if (win == current_window())
	{
	  XInstallColormap (dpy, win->colormap);
	}

      ignore_badwindow--;
    }
}	  

static void
focus_change (XFocusChangeEvent *ev)
{
  rp_window *win;

  /* We're only interested in the NotifyGrab mode */
  if (ev->mode != NotifyGrab) return;

  win = find_window (ev->window);

  if (win != NULL)
    {
      PRINT_DEBUG ("Re-grabbing prefix key\n");
      grab_prefix_key (win->w);
    }
}

static void
mapping_notify (XMappingEvent *ev)
{
  rp_window *cur;

  /* Remove the grab on the current prefix key */
  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
      ungrab_prefix_key (cur->w);
    }

  switch (ev->request)
    {
    case MappingModifier:
      update_modifier_map();
      /* This is meant to fall through.  */
    case MappingKeyboard:
      XRefreshKeyboardMapping (ev);
      break;
    }

  /* Add the grab on the current prefix key */
  for (cur = rp_mapped_window_sentinel->next; 
       cur != rp_mapped_window_sentinel; 
       cur = cur->next)
    {
      grab_prefix_key (cur->w);
    }
}

/* Given an event, call the correct function to handle it. */
static void
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
      focus_change (&ev->xfocus);
      break;
    case FocusIn:
      PRINT_DEBUG ("FocusIn\n");
      focus_change (&ev->xfocus);
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
      mapping_notify( &ev->xmapping );
      break;
    default:
      PRINT_DEBUG ("Unhandled event %d\n", ev->type);
    }
}

static void
get_event (XEvent *ev)
{
  int x_fd;
  fd_set fds;
  struct timeval t;

  /* An alarm means we need to hide the popup windows. */
  if (alarm_signalled > 0)
    {
      int i;

      PRINT_DEBUG ("Alarm recieved.\n");

      for (i=0; i<num_screens; i++)
	{
	  hide_bar (&screens[i]);
	}
      hide_frame_indicator();
      alarm_signalled = 0;
    }

  if (hup_signalled > 0)
    {
      PRINT_DEBUG ("Restarting with a fresh plate.\n"); 
      send_restart ();
    }

  if (kill_signalled > 0)
    {
      fprintf (stderr, "ratpoison: Agg! I've been SHOT!\n"); 
      clean_up ();
      exit (EXIT_FAILURE);
    }

  /* Is there anything in the event qeue? */
  if (QLength (dpy) > 0)
    {
      XNextEvent (dpy, ev);
      return;
    }

  /* If the event queue is empty, then select on it until there is
     something. */
  x_fd = ConnectionNumber (dpy);
  FD_ZERO (&fds);
  FD_SET (x_fd, &fds);
  t.tv_sec = 0;
  t.tv_usec = 0;

  if (select(x_fd+1, &fds, NULL, NULL, &t) == 1) 
    {
    XNextEvent(dpy, ev);
    return;
    }

  XFlush(dpy);
  FD_SET(x_fd, &fds);

  if (select(x_fd+1, &fds, NULL, NULL, NULL) == 1) 
    {
    XNextEvent(dpy, ev);
    return;
    }
}

void
handle_events ()
{
  for (;;) 
    {
      get_event (&rp_current_event);
      delegate_event (&rp_current_event);
    }
}
