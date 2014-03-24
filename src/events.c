/* Ratpoison X events
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
#include <X11/keysym.h>
#include <X11/Xmd.h>            /* for CARD32. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "ratpoison.h"

/* The event currently being processed. Mostly used in functions from
   action.c which need to forward events to other windows. */
XEvent rp_current_event;

/* RAISED is non zero if a raised message should be used 0 for a map message. */
void
show_rudeness_msg (rp_window *win, int raised)
{
  rp_group *g = groups_find_group_by_window (win);
  rp_window_elem *elem = group_find_window (&g->mapped_windows, win);
  if (g == rp_current_group)
    {
      if (win->transient)
        marked_message_printf (0, 0, raised ? MESSAGE_RAISE_TRANSIENT:MESSAGE_MAP_TRANSIENT,
                               elem->number, window_name (win));
      else
        marked_message_printf (0, 0, raised ? MESSAGE_RAISE_WINDOW:MESSAGE_MAP_WINDOW,
                               elem->number, window_name (win));
    }
  else
    {
      if (win->transient)
        marked_message_printf (0, 0, raised ? MESSAGE_RAISE_TRANSIENT_GROUP:MESSAGE_MAP_TRANSIENT_GROUP,
                               elem->number, window_name (win), g->name);
      else
        marked_message_printf (0, 0, raised ? MESSAGE_RAISE_WINDOW_GROUP:MESSAGE_MAP_WINDOW_GROUP,
                               elem->number, window_name (win), g->name);
    }
}

static void
new_window (XCreateWindowEvent *e)
{
  rp_window *win;
  rp_screen *s;

  if (e->override_redirect)
    return;

  win = find_window (e->window);

  /* In Xinerama mode, all windows have the same root, so check
   * all Xinerama screens
   */
  if (rp_have_xinerama)
    {
      /* New windows belong to the current screen */
      s = &screens[rp_current_screen];
    }
  else
    {
      s = find_screen (e->parent);
    }
  if (is_rp_window_for_screen(e->window, s)) return;

  if (s && win == NULL
      && e->window != s->key_window
      && e->window != s->bar_window
      && e->window != s->input_window
      && e->window != s->frame_window
      && e->window != s->help_window)
    {
      win = add_to_window_list (s, e->window);
      update_window_information (win);
    }
}

static void
unmap_notify (XEvent *ev)
{
  rp_frame *frame;
  rp_window *win;

  /* ignore SubstructureNotify unmaps. */
  if(ev->xunmap.event != ev->xunmap.window
     && ev->xunmap.send_event != True)
    return;

  /* FIXME: Should we only look in the mapped window list? */
  win = find_window_in_list (ev->xunmap.window, &rp_mapped_window);

  if (win == NULL)
    return;

  switch (win->state)
    {
    case IconicState:
      PRINT_DEBUG (("Withdrawing iconized window '%s'\n", window_name (win)));
      if (ev->xunmap.send_event) withdraw_window (win);
      break;
    case NormalState:
      PRINT_DEBUG (("Withdrawing normal window '%s'\n", window_name (win)));
      /* If the window was inside a frame, fill the frame with another
         window. */
      frame = find_windows_frame (win);
      if (frame)
        {
          cleanup_frame (frame);
          if (frame->number == win->scr->current_frame
              && current_screen() == win->scr)
            set_active_frame (frame, 0);
	  /* Since we may have switched windows, call the hook. */
	  if (frame->win_number != EMPTY)
	    hook_run (&rp_switch_win_hook);
        }

      withdraw_window (win);
      break;
    }

  update_window_names (win->scr, defaults.window_fmt);
}

static void
map_request (XEvent *ev)
{
  rp_window *win;

  win = find_window (ev->xmap.window);
  if (win == NULL)
    {
      PRINT_DEBUG (("Map request from an unknown window.\n"));
      XMapWindow (dpy, ev->xmap.window);
      return;
    }

  PRINT_DEBUG (("Map request from a managed window\n"));

  switch (win->state)
    {
    case WithdrawnState:
      if (unmanaged_window (win->w))
        {
          PRINT_DEBUG (("Mapping Unmanaged Window\n"));
          XMapWindow (dpy, win->w);
          break;
        }
      else
        {
          PRINT_DEBUG (("Mapping Withdrawn Window\n"));
          map_window (win);
          break;
        }
      break;
    case IconicState:
      PRINT_DEBUG (("Mapping Iconic window\n"));
      if (win->last_access == 0)
        {
          /* Depending on the rudeness level, actually map the
             window. */
          if ((rp_honour_transient_map && win->transient)
              || (rp_honour_normal_map && !win->transient))
            set_active_window (win);
        }
      else
        {
          /* Depending on the rudeness level, actually map the
             window. */
          if ((rp_honour_transient_raise && win->transient)
              || (rp_honour_normal_raise && !win->transient))
            set_active_window (win);
          else
            show_rudeness_msg (win, 1);
        }
      break;
    }
}

static void
destroy_window (XDestroyWindowEvent *ev)
{
  rp_window *win;

  win = find_window (ev->window);
  if (win == NULL) return;

  ignore_badwindow++;

  /* If, somehow, the window is not withdrawn before it is destroyed,
     perform the necessary steps to withdraw the window before it is
     unmanaged. */
  if (win->state == IconicState)
    {
      PRINT_DEBUG (("Destroying Iconic Window (%s)\n", window_name (win)));
      withdraw_window (win);
    }
  else if (win->state == NormalState)
    {
      rp_frame *frame;

      PRINT_DEBUG (("Destroying Normal Window (%s)\n", window_name (win)));
      frame = find_windows_frame (win);
      if (frame)
        {
          cleanup_frame (frame);
          if (frame->number == win->scr->current_frame
              && current_screen() == win->scr)
            set_active_frame (frame, 0);
          /* Since we may have switched windows, call the hook. */
          if (frame->win_number != EMPTY)
            hook_run (&rp_switch_win_hook);
        }
      withdraw_window (win);
    }

  /* Now that the window is guaranteed to be in the unmapped window
     list, we can safely stop managing it. */
  unmanage (win);
  ignore_badwindow--;
}

static void
configure_request (XConfigureRequestEvent *e)
{
  XWindowChanges changes;
  rp_window *win;

  win = find_window (e->window);

  if (win)
    {
      if (e->value_mask & CWStackMode)
        {
          if (e->detail == Above && win->state != WithdrawnState)
            {
              /* Depending on the rudeness level, actually map the
                 window. */
              if ((rp_honour_transient_raise && win->transient)
                  || (rp_honour_normal_raise && !win->transient))
                {
                  if (win->state == IconicState)
                    set_active_window (win);
                  else if (find_windows_frame (win))
                    goto_window (win);
                }
              else if (current_window() != win)
                {
                  show_rudeness_msg (win, 1);
                }

            }

          PRINT_DEBUG(("request CWStackMode %d\n", e->detail));
        }

      PRINT_DEBUG (("'%s' window size: %d %d %d %d %d\n", window_name (win),
                   win->x, win->y, win->width, win->height, win->border));

      /* Collect the changes to be granted. */
      if (e->value_mask & CWBorderWidth)
        {
          changes.border_width = e->border_width;
          win->border = e->border_width;
          PRINT_DEBUG(("request CWBorderWidth %d\n", e->border_width));
        }

      if (e->value_mask & CWWidth)
        {
          changes.width = e->width;
          win->width = e->width;
          PRINT_DEBUG(("request CWWidth %d\n", e->width));
        }

      if (e->value_mask & CWHeight)
        {
          changes.height = e->height;
          win->height = e->height;
          PRINT_DEBUG(("request CWHeight %d\n", e->height));
        }

      if (e->value_mask & CWX)
        {
          changes.x = e->x;
          win->x = e->x;
          PRINT_DEBUG(("request CWX %d\n", e->x));
        }

      if (e->value_mask & CWY)
        {
          changes.y = e->y;
          win->y = e->y;
          PRINT_DEBUG(("request CWY %d\n", e->y));
        }

      if (e->value_mask & (CWX|CWY|CWBorderWidth|CWWidth|CWHeight))
        {
          /* Grant the request, then immediately maximize it. */
          XConfigureWindow (dpy, win->w,
                            e->value_mask & (CWX|CWY|CWBorderWidth|CWWidth|CWHeight),
                            &changes);
          XSync(dpy, False);
          if (win->state == NormalState)
            maximize (win);
        }
    }
  else
    {
      /* Its an unmanaged window, so give it what it wants. But don't
         change the stack mode.*/
      if (e->value_mask & CWX) changes.x = e->x;
      if (e->value_mask & CWY) changes.x = e->x;
      if (e->value_mask & CWWidth) changes.x = e->x;
      if (e->value_mask & CWHeight) changes.x = e->x;
      if (e->value_mask & CWBorderWidth) changes.x = e->x;
      XConfigureWindow (dpy, e->window,
                        e->value_mask & (CWX|CWY|CWBorderWidth|CWWidth|CWHeight),
                        &changes);
    }
}

static void
client_msg (XClientMessageEvent *ev)
{
  PRINT_DEBUG (("Received client message.\n"));

  if (ev->message_type == wm_change_state)
    {
      rp_window *win;

      PRINT_DEBUG (("WM_CHANGE_STATE\n"));

      win = find_window (ev->window);
      if (win == NULL) return;
      if (ev->format == 32 && ev->data.l[0] == IconicState)
        {
          /* FIXME: This means clients can hide themselves without the
             user's intervention. This is bad, but Emacs is the only
             program I know of that iconifies itself and this is
             generally from the user pressing C-z.  */
          PRINT_DEBUG (("Iconify Request.\n"));
          if (win->state == NormalState)
            {
              rp_window *w = find_window_other(win->scr);

              if (w)
                set_active_window (w);
              else
                blank_frame (screen_get_frame (win->scr, win->scr->current_frame));
            }
        }
      else
        {
          PRINT_ERROR (("Non-standard WM_CHANGE_STATE format\n"));
        }
    }
}

static void
handle_key (KeySym ks, unsigned int mod, rp_screen *s)
{
  rp_action *key_action;
  rp_keymap *map = find_keymap (defaults.top_kmap);

  if (map == NULL)
    {
      PRINT_ERROR (("Unable to find %s keymap\n", defaults.top_kmap));
      return;
    }

  PRINT_DEBUG (("handling key...\n"));

  /* All functions hide the program bar and the frame indicator. */
  if (defaults.bar_timeout > 0) hide_bar (s);
  hide_frame_indicator();

  /* Disable any alarm that was going to go off. */
  alarm (0);
  alarm_signalled = 0;

  /* Call the top level key pressed hook. */
  hook_run (&rp_key_hook);

  PRINT_DEBUG (("handle_key\n"));

  /* Read a key and execute the command associated with it on the
     default keymap. Ignore the key if it doesn't have a binding. */
  if ((key_action = find_keybinding (ks, x11_mask_to_rp_mask (mod), map)))
    {
      cmdret *result;

      PRINT_DEBUG(("%s\n", key_action->data));

      result = command (1, key_action->data);

      if (result)
        {
          if (result->output)
            message (result->output);
          cmdret_free (result);
        }
    }
  else
    {
      PRINT_DEBUG(("Impossible: No matching key"));
    }
}

static void
key_press (XEvent *ev)
{
  rp_screen *s;
  unsigned int modifier;
  KeySym ks;

  if (rp_have_xinerama)
    s = current_screen();
  else
    s = find_screen (ev->xkey.root);

  if (!s) return;

#ifdef HIDE_MOUSE
  XWarpPointer (dpy, None, s->root, 0, 0, 0, 0, s->left + s->width - 2, s->top + s->height - 2);
#endif

  modifier = ev->xkey.state;
  cook_keycode ( &ev->xkey, &ks, &modifier, NULL, 0, 1);

  handle_key (ks, modifier, s);
}

/* Read a command off the window and execute it. Some commands return
   text. This text is passed back using the RP_COMMAND_RESULT
   Atom. The client will wait for this property change so something
   must be returned. */
static cmdret *
execute_remote_command (Window w)
{
  int status;
  cmdret *ret;
  Atom type_ret;
  int format_ret;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *req;

  status = XGetWindowProperty (dpy, w, rp_command,
                               0, 0, False, xa_string,
                               &type_ret, &format_ret, &nitems, &bytes_after,
                               &req);

  if (status != Success || req == NULL)
    {
      return cmdret_new (RET_FAILURE, "Couldn't get RP_COMMAND Property");
    }

  /* XGetWindowProperty always allocates one extra byte even if
     the property is zero length. */
  XFree (req);

  status = XGetWindowProperty (dpy, w, rp_command,
                               0, (bytes_after / 4) + (bytes_after % 4 ? 1 : 0),
                               True, xa_string, &type_ret, &format_ret, &nitems,
                               &bytes_after, &req);

  if (status != Success || req == NULL)
    {
      return cmdret_new (RET_FAILURE, "Couldn't get RP_COMMAND Property");
    }

  PRINT_DEBUG (("command: %s\n", req));
  ret = command (req[0], (char *)&req[1]);
  XFree (req);

  return ret;
}

/* Command requests are posted as a property change using the
   RP_COMMAND_REQUEST Atom on the root window. A Command request is a
   Window that holds the actual command as a property using the
   RP_COMMAND Atom. receive_command reads the list of Windows and
   executes their associated command. */
static void
receive_command (Window root)
{
  cmdret *cmd_ret;
  char *result;
  Atom type_ret;
  int format_ret;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *prop_return;
  int offset;

  /* Init offset to 0. In the case where there is more than one window
     in the property, a partial read does not delete the property and
     we need to grab the next window by incementing offset to the
     offset of the next window. */
  offset = 0;
  do
    {
      int ret;
      int length;
      Window w;

      length = sizeof (Window) / 4 + (sizeof (Window) % 4 ?1:0);
      ret = XGetWindowProperty (dpy, root,
                                rp_command_request,
                                offset, length,
                                True, XA_WINDOW, &type_ret, &format_ret,
                                &nitems,
                                &bytes_after, &prop_return);

      /* Update the offset to point to the next window (if there is
         another one). */
      offset += length;

      if (ret != Success)
        {
          PRINT_ERROR (("XGetWindowProperty Failed\n"));
          if (prop_return)
            XFree (prop_return);
          break;
        }

      /* If there was no window, then we're done. */
      if (prop_return == NULL)
        {
          PRINT_DEBUG (("No property to read\n"));
          break;
        }

      /* We grabbed a window, so now read the command stored in
         this window and execute it. */
      w = *(Window *)prop_return;
      XFree (prop_return);
      cmd_ret = execute_remote_command (w);

      /* notify the client of any text that was returned by the
         command.  see communications.c:receive_command_result() */
      if (cmd_ret->output)
        result = xsprintf ("%c%s", cmd_ret->success ? '1':'0', cmd_ret->output);
      else if (!cmd_ret->success)
        result = xstrdup("0");
      else
	result = NULL;

      if (result)
        XChangeProperty (dpy, w, rp_command_result, xa_string,
                         8, PropModeReplace, (unsigned char *)result, strlen (result));
      else
        XChangeProperty (dpy, w, rp_command_result, xa_string,
                         8, PropModeReplace, NULL, 0);
      free (result);
      cmdret_free (cmd_ret);
    } while (bytes_after > 0);
}

static void
property_notify (XEvent *ev)
{
  rp_window *win;

  PRINT_DEBUG (("atom: %ld\n", ev->xproperty.atom));

  if (ev->xproperty.atom == rp_command_request
      && is_a_root_window (ev->xproperty.window)
      && ev->xproperty.state == PropertyNewValue)
    {
      PRINT_DEBUG (("ratpoison command\n"));
      receive_command(ev->xproperty.window);
    }

  win = find_window (ev->xproperty.window);

  if (win)
    {
      if (ev->xproperty.atom == _net_wm_pid)
        {
          struct rp_child_info *child_info;

          PRINT_DEBUG (("updating _NET_WM_PID\n"));
          child_info = get_child_info(win->w);
          if (child_info && !child_info->window_mapped)
            {
              if (child_info->frame)
                {
                  PRINT_DEBUG (("frame=%p\n", child_info->frame));
                  win->intended_frame_number = child_info->frame->number;
		  /* Only map the first window in the launch frame. */
		  child_info->window_mapped = 1;
                }
              /* TODO: also adopt group information? */
            }
        } else
        switch (ev->xproperty.atom)
          {
          case XA_WM_NAME:
            PRINT_DEBUG (("updating window name\n"));
            if (update_window_name (win)) {
	      update_window_names (win->scr, defaults.window_fmt);
	      hook_run (&rp_title_changed_hook);
	    }
            break;

          case XA_WM_NORMAL_HINTS:
            PRINT_DEBUG (("updating window normal hints\n"));
            update_normal_hints (win);
            if (win->state == NormalState)
              maximize (win);
            break;

          case XA_WM_TRANSIENT_FOR:
            PRINT_DEBUG (("Transient for\n"));
            win->transient = XGetTransientForHint (dpy, win->w, &win->transient_for);
            break;

          default:
            PRINT_DEBUG (("Unhandled property notify event: %ld\n", ev->xproperty.atom));
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

      if (win == current_window()
	  && !current_screen()->bar_is_raised)
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
      PRINT_DEBUG (("Re-grabbing prefix key\n"));
      grab_top_level_keys (win->w);
    }
}

static void
mapping_notify (XMappingEvent *ev)
{
  ungrab_keys_all_wins();

  switch (ev->request)
    {
    case MappingModifier:
      update_modifier_map();
      /* This is meant to fall through.  */
    case MappingKeyboard:
      XRefreshKeyboardMapping (ev);
      break;
    }

  grab_keys_all_wins();
}

static void
configure_notify (XConfigureEvent *ev)
{
  rp_screen *s;

  s = find_screen(ev->window);
  if (s != NULL)
    /* This is a root window of a screen,
     * look if its width or height changed: */
    screen_update(s,ev->width,ev->height);
}

/* This is called whan an application has requested the
   selection. Copied from rxvt. */
static void
selection_request (XSelectionRequestEvent *rq)
{
  XEvent          ev;
  CARD32          target_list[4];
  Atom            target;
  static Atom     xa_targets = None;
  static Atom     xa_text = None; /* XXX */
  XTextProperty   ct;
  XICCEncodingStyle style;
  char           *cl[4];

  if (xa_text == None)
    xa_text = XInternAtom(dpy, "TEXT", False);
  if (xa_targets == None)
    xa_targets = XInternAtom(dpy, "TARGETS", False);

  ev.xselection.type = SelectionNotify;
  ev.xselection.property = None;
  ev.xselection.display = rq->display;
  ev.xselection.requestor = rq->requestor;
  ev.xselection.selection = rq->selection;
  ev.xselection.target = rq->target;
  ev.xselection.time = rq->time;

  if (rq->target == xa_targets) {
    target_list[0] = (CARD32) xa_targets;
    target_list[1] = (CARD32) xa_string;
    target_list[2] = (CARD32) xa_text;
    target_list[3] = (CARD32) xa_compound_text;
    XChangeProperty(dpy, rq->requestor, rq->property, rq->target,
                    (8 * sizeof(target_list[0])), PropModeReplace,
                    (unsigned char *)target_list,
                    (sizeof(target_list) / sizeof(target_list[0])));
    ev.xselection.property = rq->property;
  } else if (rq->target == xa_string
             || rq->target == xa_compound_text
             || rq->target == xa_text) {
    if (rq->target == xa_string) {
      style = XStringStyle;
      target = xa_string;
    } else {
      target = xa_compound_text;
      style = (rq->target == xa_compound_text) ? XCompoundTextStyle
        : XStdICCTextStyle;
    }
    cl[0] = selection.text;
    XmbTextListToTextProperty(dpy, cl, 1, style, &ct);
    XChangeProperty(dpy, rq->requestor, rq->property,
                    target, 8, PropModeReplace,
                    ct.value, ct.nitems);
    ev.xselection.property = rq->property;
  }
  XSendEvent(dpy, rq->requestor, False, 0, &ev);
}

static void
selection_clear (void)
{
  free (selection.text);
  selection.text = NULL;
  selection.len = 0;
}

/* Given an event, call the correct function to handle it. */
static void
delegate_event (XEvent *ev)
{
  switch (ev->type)
    {
    case ConfigureRequest:
      PRINT_DEBUG (("--- Handling ConfigureRequest ---\n"));
      configure_request (&ev->xconfigurerequest);
      break;

    case CreateNotify:
      PRINT_DEBUG (("--- Handling CreateNotify ---\n"));
      new_window (&ev->xcreatewindow);
      break;

    case DestroyNotify:
      PRINT_DEBUG (("--- Handling DestroyNotify ---\n"));
      destroy_window (&ev->xdestroywindow);
      break;

    case ClientMessage:
      PRINT_DEBUG (("--- Handling ClientMessage ---\n"));
      client_msg (&ev->xclient);
      break;

    case ColormapNotify:
      PRINT_DEBUG (("--- Handling ColormapNotify ---\n"));
      colormap_notify (ev);
      break;

    case PropertyNotify:
      PRINT_DEBUG (("--- Handling PropertyNotify ---\n"));
      property_notify (ev);
      break;

    case MapRequest:
      PRINT_DEBUG (("--- Handling MapRequest ---\n"));
      map_request (ev);
      break;

    case KeyPress:
      PRINT_DEBUG (("--- Handling KeyPress ---\n"));
      key_press (ev);
      break;

    case UnmapNotify:
      PRINT_DEBUG (("--- Handling UnmapNotify ---\n"));
      unmap_notify (ev);
      break;

    case FocusOut:
      PRINT_DEBUG (("--- Handling FocusOut ---\n"));
      focus_change (&ev->xfocus);
      break;

    case FocusIn:
      PRINT_DEBUG (("--- Handling FocusIn ---\n"));
      focus_change (&ev->xfocus);
      break;

    case MappingNotify:
      PRINT_DEBUG (("--- Handling MappingNotify ---\n"));
      mapping_notify( &ev->xmapping );
      break;

    case SelectionRequest:
      selection_request(&ev->xselectionrequest);
      break;

    case SelectionClear:
      selection_clear();
      break;

    case ConfigureNotify:
      PRINT_DEBUG (("--- Handling ConfigureNotify ---\n"));
      configure_notify( &ev->xconfigure );
      break;
	
    case MapNotify:
    case Expose:
    case MotionNotify:
    case KeyRelease:
    case ReparentNotify:
    case EnterNotify:
    case SelectionNotify:
    case CirculateRequest:
      /* Ignore these events. */
      break;

    default:
      PRINT_DEBUG (("--- Unknown event %d ---\n",- ev->type));
    }
}

static void
handle_signals (void)
{
  /* An alarm means we need to hide the popup windows. */
  if (alarm_signalled > 0)
    {
      int i;

      PRINT_DEBUG (("Alarm received.\n"));

      /* Only hide the bar if it times out. */
      if (defaults.bar_timeout > 0)
        for (i=0; i<num_screens; i++)
          hide_bar (&screens[i]);

      hide_frame_indicator();
      alarm_signalled = 0;
    }

  if (chld_signalled > 0)
    {
      rp_child_info *cur;
      struct list_head *iter, *tmp;

      /* Report and remove terminated processes. */
      list_for_each_safe_entry (cur, iter, tmp, &rp_children, node)
        {
          if (cur->terminated)
            {
              /* Report any child that didn't return 0. */
              if (cur->status != 0)
                marked_message_printf (0,0, "/bin/sh -c \"%s\" finished (%d)",
                                       cur->cmd, cur->status);
              list_del  (&cur->node);
              free (cur->cmd);
              free (cur);
            }
        }

      chld_signalled = 0;
    }

  if (rp_exec_newwm)
    {
      int i;

      PRINT_DEBUG (("Switching to %s\n", rp_exec_newwm));

      putenv(current_screen()->display_string);
      unhide_all_windows();
      XSync(dpy, False);
      for (i=0; i<num_screens; i++)
      {
	      deactivate_screen(&screens[i]);
      }
      execlp (rp_exec_newwm, rp_exec_newwm, (char *)NULL);

      /* Failed. Clean up. */
      PRINT_ERROR (("exec %s ", rp_exec_newwm));
      perror(" failed");
      free (rp_exec_newwm);
      rp_exec_newwm = NULL;
      for (i=0; i<num_screens; i++)
      {
	      activate_screen(&screens[i]);
      }
    }

  if (hup_signalled > 0)
    {
      PRINT_DEBUG (("Restarting\n"));
      hook_run (&rp_restart_hook);
      clean_up ();
      execvp(myargv[0], myargv);
    }

  if (kill_signalled > 0)
    {
      PRINT_DEBUG (("Exiting\n"));
      hook_run (&rp_quit_hook);
      clean_up ();
      exit (EXIT_SUCCESS);
    }

  /* Report any X11 errors that have occurred. */
  if (rp_error_msg)
    {
      marked_message_printf (0, 6, "ERROR: %s", rp_error_msg);
      free (rp_error_msg);
      rp_error_msg = NULL;
    }
}

/* The main loop. */
void
listen_for_events (void)
{
  int x_fd;
  fd_set fds;

  x_fd = ConnectionNumber (dpy);
  FD_ZERO (&fds);

  /* Loop forever. */
  for (;;)
    {
      handle_signals ();

      /* Handle the next event. */
      FD_SET (x_fd, &fds);
      XFlush(dpy);

      if (QLength (dpy) > 0
          || select(x_fd+1, &fds, NULL, NULL, NULL) == 1)
        {
          XNextEvent (dpy, &rp_current_event);
          delegate_event (&rp_current_event);
          XSync(dpy, False);
        }
    }
}
