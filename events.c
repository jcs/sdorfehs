/*
 * X events
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xmd.h>	/* for CARD32. */

#include "sdorfehs.h"

/*
 * The event currently being processed. Mostly used in functions from action.c
 * which need to forward events to other windows.
 */
XEvent rp_current_event;

/* RAISED is non zero if a raised message should be used 0 for a map message. */
void
show_rudeness_msg(rp_window *win, int raised)
{
	rp_vscreen *v = win->vscreen;
	rp_window_elem *elem = vscreen_find_window(&v->mapped_windows, win);

	if (v == rp_current_vscreen) {
		if (win->transient)
			marked_message_printf(0, 0, raised ?
			    MESSAGE_RAISE_TRANSIENT : MESSAGE_MAP_TRANSIENT,
			    elem->number, window_name(win));
		else
			marked_message_printf(0, 0, raised ?
			    MESSAGE_RAISE_WINDOW : MESSAGE_MAP_WINDOW,
			    elem->number, window_name(win));
	} else {
		if (win->transient)
			marked_message_printf(0, 0, raised ?
			    MESSAGE_RAISE_TRANSIENT_VSCREEN :
			    MESSAGE_MAP_TRANSIENT_VSCREEN,
			    elem->number, window_name(win),
			    win->vscreen->number);
		else
			marked_message_printf(0, 0, raised ?
			    MESSAGE_RAISE_WINDOW_VSCREEN :
			    MESSAGE_MAP_WINDOW_VSCREEN,
			    elem->number, window_name(win),
			    win->vscreen->number);
	}
}

static void
new_window(XCreateWindowEvent *e)
{
	rp_window *win;

	if (e->override_redirect)
		return;

	if (is_rp_window(e->window))
		return;

	win = find_window(e->window);
	if (win == NULL) {
		/* We'll figure out which vscreen to put this window in later */
		win = add_to_window_list(rp_current_screen, e->window);
	}
	update_window_information(win);

	PRINT_DEBUG(("created new window\n"));
}

static void
unmap_notify(XEvent *ev)
{
	rp_frame *frame;
	rp_window *win;

	/* ignore SubstructureNotify unmaps. */
	if (ev->xunmap.event != ev->xunmap.window
	    && ev->xunmap.send_event != True)
		return;

	/* FIXME: Should we only look in the mapped window list? */
	win = find_window_in_list(ev->xunmap.window, &rp_mapped_window);

	if (win == NULL)
		return;

	switch (win->state) {
	case IconicState:
		PRINT_DEBUG(("Withdrawing iconized window '%s'\n",
		    window_name(win)));
		if (ev->xunmap.send_event)
			withdraw_window(win);
		break;
	case NormalState:
		PRINT_DEBUG(("Withdrawing normal window '%s'\n",
		    window_name(win)));
		/*
		 * If the window was inside a frame, fill the frame with
		 * another window.
		 */
		frame = find_windows_frame(win);
		if (frame) {
			cleanup_frame(frame);
			if (frame->number == win->vscreen->current_frame
			    && rp_current_vscreen == win->vscreen)
				set_active_frame(frame, 0);
			/* Since we may have switched windows, call the hook. */
			if (frame->win_number != EMPTY)
				hook_run(&rp_switch_win_hook);
		}
		withdraw_window(win);
		break;
	}

	update_window_names(win->vscreen->screen, defaults.window_fmt);
}

static void
map_request(XEvent *ev)
{
	rp_window *win;

	win = find_window(ev->xmap.window);
	if (win == NULL) {
		PRINT_DEBUG(("Map request from an unknown window.\n"));
		XMapWindow(dpy, ev->xmap.window);
		return;
	}

	if (unmanaged_window(ev->xmap.window)) {
		PRINT_DEBUG(("Map request from an unmanaged window\n"));
		unmanage(win);
		XMapRaised(dpy, ev->xmap.window);
		return;
	}

	PRINT_DEBUG(("Map request from a managed window\n"));

	switch (win->state) {
	case WithdrawnState:
		if (unmanaged_window(win->w)) {
			PRINT_DEBUG(("Mapping Unmanaged Window\n"));
			XMapWindow(dpy, win->w);
			break;
		} else {
			PRINT_DEBUG(("Mapping Withdrawn Window\n"));
			map_window(win);
			break;
		}
		break;
	case IconicState:
		PRINT_DEBUG(("Mapping Iconic window\n"));

		if (win->vscreen != rp_current_vscreen) {
			/*
			 * It is always rude to raise a window in another
			 * vscreen
			 */
			show_rudeness_msg(win, 1);
			break;
		}

		/* Depending on the rudeness level, actually map the window. */
		if (win->last_access == 0) {
			if ((rp_honour_transient_map && win->transient)
			    || (rp_honour_normal_map && !win->transient))
				set_active_window(win);
		} else {
			if ((rp_honour_transient_raise && win->transient)
			    || (rp_honour_normal_raise && !win->transient))
				set_active_window(win);
			else
				show_rudeness_msg(win, 1);
		}
		break;
	default:
		PRINT_DEBUG(("Map request for window in unknown state %d\n",
		    win->state));
	}
}

static void
destroy_window(XDestroyWindowEvent *ev)
{
	rp_window *win;

	win = find_window(ev->window);
	if (win == NULL)
		return;

	ignore_badwindow++;

	/*
	 * If, somehow, the window is not withdrawn before it is destroyed,
	 * perform the necessary steps to withdraw the window before it is
	 * unmanaged.
	 */
	if (win->state == IconicState) {
		PRINT_DEBUG(("Destroying Iconic Window (%s)\n",
		    window_name(win)));
		withdraw_window(win);
	} else if (win->state == NormalState) {
		rp_frame *frame;

		PRINT_DEBUG(("Destroying Normal Window (%s)\n",
		    window_name(win)));
		frame = find_windows_frame(win);
		if (frame) {
			cleanup_frame(frame);
			if (frame->number == win->vscreen->current_frame
			    && rp_current_vscreen == win->vscreen)
				set_active_frame(frame, 0);
			/* Since we may have switched windows, call the hook. */
			if (frame->win_number != EMPTY)
				hook_run(&rp_switch_win_hook);
		}
		withdraw_window(win);
	}
	/*
	 * Now that the window is guaranteed to be in the unmapped window list,
	 * we can safely stop managing it.
	 */
	unmanage(win);
	ignore_badwindow--;
}

static void
configure_request(XConfigureRequestEvent *e)
{
	XWindowChanges changes;
	rp_window *win;

	win = find_window(e->window);

	if (win) {
		if (e->value_mask & CWStackMode) {
			if (e->detail == Above && win->state != WithdrawnState) {
				if (win->vscreen == rp_current_vscreen) {
					/*
					 * Depending on the rudeness level,
					 * actually map the window.
					 */
					if ((rp_honour_transient_raise &&
					    win->transient) ||
					    (rp_honour_normal_raise &&
					    !win->transient)) {
						if (win->state == IconicState)
							set_active_window(win);
						else if (find_windows_frame(win))
							goto_window(win);
					} else if (current_window() != win) {
						show_rudeness_msg(win, 1);
					}
				} else {
					show_rudeness_msg(win, 1);
				}
			}
			PRINT_DEBUG(("request CWStackMode %d\n", e->detail));
		}
		PRINT_DEBUG(("'%s' window size: %d %d %d %d %d\n",
		    window_name(win), win->x, win->y, win->width, win->height,
		    win->border));

		/* Collect the changes to be granted. */
		if (e->value_mask & CWBorderWidth) {
			changes.border_width = e->border_width;
			win->border = e->border_width;
			PRINT_DEBUG(("request CWBorderWidth %d\n",
			    e->border_width));
		}
		if (e->value_mask & CWWidth) {
			changes.width = e->width;
			win->width = e->width;
			PRINT_DEBUG(("request CWWidth %d\n", e->width));
		}
		if (e->value_mask & CWHeight) {
			changes.height = e->height;
			win->height = e->height;
			PRINT_DEBUG(("request CWHeight %d\n", e->height));
		}
		if (e->value_mask & CWX) {
			changes.x = e->x;
			win->x = e->x;
			PRINT_DEBUG(("request CWX %d\n", e->x));
		}
		if (e->value_mask & CWY) {
			changes.y = e->y;
			win->y = e->y;
			PRINT_DEBUG(("request CWY %d\n", e->y));
		}
		if (e->value_mask & (CWX|CWY|CWBorderWidth|CWWidth|CWHeight)) {
			/* Grant the request, then immediately maximize it. */
			XConfigureWindow(dpy, win->w,
			    e->value_mask & (CWX|CWY|CWBorderWidth|CWWidth|CWHeight),
			    &changes);
			XSync(dpy, False);
			if (win->state == NormalState)
				maximize(win);
		}
	} else {
		/*
		 * Its an unmanaged window, so give it what it wants. But don't
		 * change the stack mode.
		 */
		if (e->value_mask & CWX)
			changes.x = e->x;
		if (e->value_mask & CWY)
			changes.x = e->x;
		if (e->value_mask & CWWidth)
			changes.x = e->x;
		if (e->value_mask & CWHeight)
			changes.x = e->x;
		if (e->value_mask & CWBorderWidth)
			changes.x = e->x;
		XConfigureWindow(dpy, e->window,
		    e->value_mask & (CWX|CWY|CWBorderWidth|CWWidth|CWHeight),
		    &changes);
	}
}

static void
client_msg(XClientMessageEvent *ev)
{
	rp_screen *s = find_screen(ev->window);
	rp_window *win = NULL;

	PRINT_DEBUG(("Received client message for window 0x%lx.\n", ev->window));

	if (s == NULL) {
		win = find_window_in_list(ev->window, &rp_mapped_window);
		if (!win) {
			PRINT_DEBUG(("can't find screen or window for "
			    "XClientMessageEvent\n"));
			return;
		}

		s = win->vscreen->screen;
	}

	if (ev->window == s->root) {
		if (ev->format != 32)
			return;

		if (ev->message_type == _net_current_desktop) {
			rp_vscreen *v;

			PRINT_DEBUG(("Received _NET_CURRENT_DESKTOP = %ld\n",
			    ev->data.l[0]));

			v = screen_find_vscreen_by_number(s, ev->data.l[0]);
			if (v)
				set_current_vscreen(v);
		} else {
			warnx("unsupported message type %ld\n",
			    ev->message_type);
		}

		return;
	}

	if (ev->message_type == wm_change_state) {
		rp_window *win;

		PRINT_DEBUG(("WM_CHANGE_STATE\n"));

		win = find_window(ev->window);
		if (win == NULL)
			return;
		if (ev->format == 32 && ev->data.l[0] == IconicState) {
			/*
			 * FIXME: This means clients can hide themselves
			 * without the user's intervention. This is bad, but
			 * Emacs is the only program I know of that iconifies
			 * itself and this is generally from the user pressing
			 * C-z.
			 */
			PRINT_DEBUG(("Iconify Request.\n"));
			if (win->state == NormalState) {
				rp_window *w = find_window_other(win->vscreen);

				if (w)
					set_active_window(w);
				else
					blank_frame(vscreen_get_frame(
					    win->vscreen,
					    win->vscreen->current_frame));
			}
		} else {
			warnx("non-standard WM_CHANGE_STATE format");
		}
	} else if (win && ev->message_type == _net_wm_state) {
		PRINT_DEBUG(("_NET_WM_STATE for window 0x%lx, action %ld, "
		    "property 0x%lx\n", win->w, ev->data.l[0], ev->data.l[1]));

		if (ev->data.l[1] == _net_wm_state_fullscreen) {
			if (ev->data.l[0] == _NET_WM_STATE_ADD ||
			    (ev->data.l[0] == _NET_WM_STATE_TOGGLE &&
			    win->full_screen == 0))
				window_full_screen(win);
			else
				window_full_screen(NULL);
		}
	} else if (win && ev->message_type == _net_active_window) {
		PRINT_DEBUG(("_NET_ACTIVE_WINDOW raise: 0x%lx\n", win->w));
		rp_window *w = find_window(win->w);
		if (w == NULL) {
			PRINT_DEBUG(("no _NET_ACTIVE_WINDOW 0x%lx\n", win->w));
			return;
		}
		set_current_vscreen(w->vscreen);
		set_active_window(w);
	} else {
		PRINT_DEBUG(("unknown client message type 0x%lx\n",
		    ev->message_type));
	}
}

static void
handle_key(KeySym ks, unsigned int mod, rp_screen *s)
{
	rp_action *key_action;
	rp_keymap *map = find_keymap(defaults.top_kmap);

	if (map == NULL) {
		warnx("unable to find %s keymap", defaults.top_kmap);
		return;
	}
	PRINT_DEBUG(("handling key...\n"));

	/* All functions hide the program bar and the frame indicator. */
	if (defaults.bar_timeout > 0 && !defaults.bar_sticky)
		hide_bar(s, 1);
	hide_frame_indicator();

	/* Disable any alarm that was going to go off. */
	alarm(0);
	alarm_signalled = 0;

	/* Call the top level key pressed hook. */
	hook_run(&rp_key_hook);

	PRINT_DEBUG(("handle_key\n"));

	/*
	 * Read a key and execute the command associated with it on the default
	 * keymap.  Ignore the key if it doesn't have a binding.
	 */
	if ((key_action = find_keybinding(ks, x11_mask_to_rp_mask(mod), map))) {
		cmdret *result;

		PRINT_DEBUG(("%s\n", key_action->data));

		if (defaults.bar_sticky)
			hide_bar(s, 0);

		result = command(1, key_action->data);

		if (result) {
			if (result->output)
				message(result->output);
			cmdret_free(result);
		}
	} else {
		if (defaults.bar_sticky)
			hide_bar(s, 0);

		PRINT_DEBUG(("Impossible: No matching key"));
	}
}

static void
key_press(XEvent *ev)
{
	rp_screen *s;
	unsigned int modifier;
	KeySym ks;

	s = rp_current_screen;
	if (!s)
		return;

	modifier = ev->xkey.state;
	cook_keycode(&ev->xkey, &ks, &modifier, NULL, 0, 1);

	handle_key(ks, modifier, s);
}

static void
button_press(XEvent *ev)
{
	rp_screen *s;
	XButtonEvent *xbe = (XButtonEvent *)ev;

	s = rp_current_screen;
	if (!s)
		return;

	if (xbe->window == s->bar_window)
		bar_handle_click(s, xbe);
}

static void
property_notify(XEvent *ev)
{
	rp_window *win;

	PRINT_DEBUG(("atom: %ld (%s)\n", ev->xproperty.atom,
	    XGetAtomName(dpy, ev->xproperty.atom)));

	win = find_window(ev->xproperty.window);
	if (!win)
		return;

	if (ev->xproperty.atom == _net_wm_pid) {
		struct rp_child_info *child_info;

		PRINT_DEBUG(("updating _NET_WM_PID\n"));
		child_info = get_child_info(win->w, 1);
		if (child_info && !child_info->window_mapped) {
			if (child_info->frame) {
				PRINT_DEBUG(("frame=%p\n", child_info->frame));
				win->intended_frame_number =
				    child_info->frame->number;
				/*
				 * Only map the first window in the launch
				 * frame.
				 */
				child_info->window_mapped = 1;
			}
		}
	} else if (ev->xproperty.atom == XA_WM_NAME) {
		PRINT_DEBUG(("updating window name\n"));
		if (update_window_name(win)) {
			update_window_names(win->vscreen->screen,
			    defaults.window_fmt);
			hook_run(&rp_title_changed_hook);
		}
	} else if (ev->xproperty.atom == XA_WM_NORMAL_HINTS) {
		PRINT_DEBUG(("updating window normal hints\n"));
		update_normal_hints(win);
		if (win->state == NormalState)
			maximize(win);
	} else if (ev->xproperty.atom == XA_WM_TRANSIENT_FOR) {
		PRINT_DEBUG(("Transient for\n"));
		win->transient = XGetTransientForHint(dpy, win->w,
		    &win->transient_for);
	} else if (ev->xproperty.atom == _net_wm_state) {
		check_state(win);
	} else {
		PRINT_DEBUG(("Unhandled property notify event: %ld\n",
		    ev->xproperty.atom));
	}
}

static void
colormap_notify(XEvent *ev)
{
	rp_window *win;

	win = find_window(ev->xcolormap.window);

	if (win != NULL) {
		XWindowAttributes attr;

		/*
		 * SDL sets the colormap just before destroying the window, so
		 * ignore BadWindow errors.
		 */
		ignore_badwindow++;

		XGetWindowAttributes(dpy, win->w, &attr);
		win->colormap = attr.colormap;

		if (win == current_window()
		    && !rp_current_screen->bar_is_raised) {
			XInstallColormap(dpy, win->colormap);
		}
		ignore_badwindow--;
	}
}

static void
focus_change(XFocusChangeEvent *ev)
{
	rp_window *win;

	/* We're only interested in the NotifyGrab mode */
	if (ev->mode != NotifyGrab)
		return;

	win = find_window(ev->window);

	if (win != NULL) {
		PRINT_DEBUG(("Re-grabbing prefix key\n"));
		grab_top_level_keys(win->w);
	}
}

static void
mapping_notify(XMappingEvent *ev)
{
	ungrab_keys_all_wins();

	switch (ev->request) {
	case MappingModifier:
		update_modifier_map();
		/* This is meant to fall through.  */
	case MappingKeyboard:
		XRefreshKeyboardMapping(ev);
		break;
	}

	grab_keys_all_wins();
}

static void
configure_notify(XConfigureEvent *ev)
{
	rp_screen *s;

	s = find_screen(ev->window);
	if (s != NULL)
		/*
		 * This is a root window of a screen, look if its width or
		 * height changed:
		 */
		screen_update(s, ev->x, ev->y, ev->width, ev->height);
}

/*
 * This is called when an application has requested the selection. Copied from
 * rxvt.
 */
static void
selection_request(XSelectionRequestEvent *rq)
{
	XEvent ev;
	CARD32 target_list[4];
	Atom target;
	static Atom xa_targets = None;
	static Atom xa_text = None;	/* XXX */
	XTextProperty ct;
	XICCEncodingStyle style;
	char *cl[4];

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
		    (unsigned char *) target_list,
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
			style = (rq->target == xa_compound_text) ?
			    XCompoundTextStyle : XStdICCTextStyle;
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
selection_clear(void)
{
	free(selection.text);
	selection.text = NULL;
	selection.len = 0;
}

/* Given an event, call the correct function to handle it. */
static void
delegate_event(XEvent *ev)
{
	if (rp_have_xrandr)
		xrandr_notify(ev);

	switch (ev->type) {
	case ConfigureRequest:
		PRINT_DEBUG(("--- Handling ConfigureRequest ---\n"));
		configure_request(&ev->xconfigurerequest);
		break;

	case CreateNotify:
		PRINT_DEBUG(("--- Handling CreateNotify ---\n"));
		new_window(&ev->xcreatewindow);
		break;

	case DestroyNotify:
		PRINT_DEBUG(("--- Handling DestroyNotify ---\n"));
		destroy_window(&ev->xdestroywindow);
		break;

	case ClientMessage:
		PRINT_DEBUG(("--- Handling ClientMessage ---\n"));
		client_msg(&ev->xclient);
		break;

	case ColormapNotify:
		PRINT_DEBUG(("--- Handling ColormapNotify ---\n"));
		colormap_notify(ev);
		break;

	case PropertyNotify:
		PRINT_DEBUG(("--- Handling PropertyNotify ---\n"));
		property_notify(ev);
		break;

	case MapRequest:
		PRINT_DEBUG(("--- Handling MapRequest ---\n"));
		map_request(ev);
		break;

	case KeyPress:
		PRINT_DEBUG(("--- Handling KeyPress ---\n"));
		key_press(ev);
		break;

	case ButtonPress:
		PRINT_DEBUG(("--- Handling ButtonPress ---\n"));
		button_press(ev);
		break;

	case UnmapNotify:
		PRINT_DEBUG(("--- Handling UnmapNotify ---\n"));
		unmap_notify(ev);
		break;

	case FocusOut:
		PRINT_DEBUG(("--- Handling FocusOut ---\n"));
		focus_change(&ev->xfocus);
		break;

	case FocusIn:
		PRINT_DEBUG(("--- Handling FocusIn ---\n"));
		focus_change(&ev->xfocus);
		break;

	case MappingNotify:
		PRINT_DEBUG(("--- Handling MappingNotify ---\n"));
		mapping_notify(&ev->xmapping);
		break;

	case SelectionRequest:
		selection_request(&ev->xselectionrequest);
		break;

	case SelectionClear:
		selection_clear();
		break;

	case ConfigureNotify:
		if (!rp_have_xrandr) {
			PRINT_DEBUG(("--- Handling ConfigureNotify ---\n"));
			configure_notify(&ev->xconfigure);
		}
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
		PRINT_DEBUG(("--- Unknown event %d ---\n", -ev->type));
	}
}

static void
handle_signals(void)
{
	/* An alarm means we need to hide the popup windows. */
	if (alarm_signalled > 0) {
		rp_screen *cur;

		PRINT_DEBUG(("SIGALRM received\n"));

		/* Only hide the bar if it times out. */
		if (defaults.bar_timeout > 0) {
			list_for_each_entry(cur, &rp_screens, node) {
				hide_bar(cur, 0);
			}
		}
		hide_frame_indicator();
		alarm_signalled = 0;
	}
	if (chld_signalled > 0) {
		rp_child_info *cur;
		struct list_head *iter, *tmp;

		/* Report and remove terminated processes. */
		list_for_each_safe_entry(cur, iter, tmp, &rp_children, node) {
			if (cur->terminated) {
				/* Report any child that didn't return 0. */
				if (cur->status != 0)
					marked_message_printf(0, 0,
					    "/bin/sh -c \"%s\" finished (%d)",
					    cur->cmd, cur->status);
				list_del(&cur->node);
				free(cur->cmd);
				free(cur);
			}
		}

		chld_signalled = 0;
	}
	if (hup_signalled > 0) {
		PRINT_DEBUG(("restarting\n"));
		hook_run(&rp_restart_hook);
		clean_up();
		execvp(myargv[0], myargv);
	}
	if (kill_signalled > 0) {
		PRINT_DEBUG(("exiting\n"));
		hook_run(&rp_quit_hook);
		clean_up();
		exit(0);
	}
	/* Report any X11 errors that have occurred. */
	if (rp_error_msg) {
		marked_message_printf(0, 6, "X error: %s", rp_error_msg);
		free(rp_error_msg);
		rp_error_msg = NULL;
	}
}

/* The main loop. */
void
listen_for_events(void)
{
	struct pollfd pfd[3];
	int pollfifo = 1;

	memset(&pfd, 0, sizeof(pfd));
	pfd[0].fd = ConnectionNumber(dpy);
	pfd[0].events = POLLIN;
	pfd[1].fd = rp_glob_screen.control_socket_fd;
	pfd[1].events = POLLIN;
	pfd[2].fd = rp_glob_screen.bar_fifo_fd;
	pfd[2].events = POLLIN;

	/* Loop forever. */
	for (;;) {
		handle_signals();

		if (!XPending(dpy)) {
			if (pollfifo && rp_glob_screen.bar_fifo_fd == -1)
				pollfifo = 0;
			else if (pollfifo)
				pfd[2].fd = rp_glob_screen.bar_fifo_fd;

			poll(pfd, pollfifo ? 3 : 2, -1);

			if (pollfifo && (pfd[2].revents & (POLLERR|POLLNVAL))) {
				warnx("error polling on FIFO");
				pollfifo = 0;
				continue;
			}

			if (pollfifo && (pfd[2].revents & (POLLHUP|POLLIN)))
				bar_read_fifo();

			if (pfd[1].revents & (POLLHUP|POLLIN))
				receive_command();

			if (!XPending(dpy))
				continue;
		}

		XNextEvent(dpy, &rp_current_event);
		delegate_event(&rp_current_event);
		XSync(dpy, False);
	}
}
