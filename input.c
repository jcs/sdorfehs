/*
 * Read keyboard input from the user.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include "sdorfehs.h"

/*
 * Convert an X11 modifier mask to the rp modifier mask equivalent, as best it
 * can (the X server may not have a hyper key defined, for instance).
 */
unsigned int
x11_mask_to_rp_mask(unsigned int mask)
{
	unsigned int result = 0;

	PRINT_INPUT_DEBUG(("x11 mask = %x\n", mask));

	result |= mask & ShiftMask ? RP_SHIFT_MASK : 0;
	result |= mask & ControlMask ? RP_CONTROL_MASK : 0;
	result |= mask & rp_modifier_info.meta_mod_mask ? RP_META_MASK : 0;
	result |= mask & rp_modifier_info.alt_mod_mask ? RP_ALT_MASK : 0;
	result |= mask & rp_modifier_info.hyper_mod_mask ? RP_HYPER_MASK : 0;
	result |= mask & rp_modifier_info.super_mod_mask ? RP_SUPER_MASK : 0;

	PRINT_INPUT_DEBUG(("rp mask = %x\n", mask));

	return result;
}

/*
 * Convert an rp modifier mask to the x11 modifier mask equivalent, as best it
 * can (the X server may not have a hyper key defined, for instance).
 */
unsigned int
rp_mask_to_x11_mask(unsigned int mask)
{
	unsigned int result = 0;

	PRINT_INPUT_DEBUG(("rp mask = %x\n", mask));

	result |= mask & RP_SHIFT_MASK ? ShiftMask : 0;
	result |= mask & RP_CONTROL_MASK ? ControlMask : 0;
	result |= mask & RP_META_MASK ? rp_modifier_info.meta_mod_mask : 0;
	result |= mask & RP_ALT_MASK ? rp_modifier_info.alt_mod_mask : 0;
	result |= mask & RP_HYPER_MASK ? rp_modifier_info.hyper_mod_mask : 0;
	result |= mask & RP_SUPER_MASK ? rp_modifier_info.super_mod_mask : 0;

	PRINT_INPUT_DEBUG(("x11 mask = %x\n", result));

	return result;
}

/* Figure out what keysyms are attached to what modifiers */
void
update_modifier_map(void)
{
	unsigned int modmasks[] = { Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask,
	    Mod5Mask };
	int row, col;	/* The row and column in the modifier table.  */
	int found_alt_or_meta;
	XModifierKeymap *mods;
	int min_code, max_code;
	int syms_per_code;
	KeySym *syms;

	rp_modifier_info.meta_mod_mask = 0;
	rp_modifier_info.alt_mod_mask = 0;
	rp_modifier_info.super_mod_mask = 0;
	rp_modifier_info.hyper_mod_mask = 0;
	rp_modifier_info.num_lock_mask = 0;
	rp_modifier_info.scroll_lock_mask = 0;

	XDisplayKeycodes(dpy, &min_code, &max_code);
	syms = XGetKeyboardMapping(dpy,
	    min_code, max_code - min_code + 1,
	    &syms_per_code);
	mods = XGetModifierMapping(dpy);

	for (row = 3; row < 8; row++) {
		found_alt_or_meta = 0;
		for (col = 0; col < mods->max_keypermod; col++) {
			KeyCode code =
			    mods->modifiermap[(row * mods->max_keypermod) + col];
			int code_col;

			PRINT_INPUT_DEBUG(("row: %d col: %d code: %d\n", row,
			    col, code));

			if (code == 0)
				continue;

			/* Are any of this keycode's keysyms a meta key?  */
			for (code_col = 0; code_col < syms_per_code; code_col++) {
				int sym = syms[((code - min_code) *
				    syms_per_code) + code_col];

				switch (sym) {
				case XK_Meta_L:
				case XK_Meta_R:
					found_alt_or_meta = 1;
					rp_modifier_info.meta_mod_mask |=
					    modmasks[row - 3];
					PRINT_INPUT_DEBUG(("Found Meta on %d\n",
					    rp_modifier_info.meta_mod_mask));
					break;

				case XK_Alt_L:
				case XK_Alt_R:
					found_alt_or_meta = 1;
					rp_modifier_info.alt_mod_mask |=
					    modmasks[row - 3];
					PRINT_INPUT_DEBUG(("Found Alt on %d\n",
					    rp_modifier_info.alt_mod_mask));
					break;

				case XK_Super_L:
				case XK_Super_R:
					if (!found_alt_or_meta) {
						rp_modifier_info.super_mod_mask |=
						    modmasks[row - 3];
						PRINT_INPUT_DEBUG(("Found "
						    "Super on %d\n",
						    rp_modifier_info.super_mod_mask));
					}
					code_col = syms_per_code;
					col = mods->max_keypermod;
					break;

				case XK_Hyper_L:
				case XK_Hyper_R:
					if (!found_alt_or_meta) {
						rp_modifier_info.hyper_mod_mask |=
						    modmasks[row - 3];
						PRINT_INPUT_DEBUG(("Found "
						    "Hyper on %d\n",
						    rp_modifier_info.hyper_mod_mask));
					}
					code_col = syms_per_code;
					col = mods->max_keypermod;

					break;

				case XK_Num_Lock:
					rp_modifier_info.num_lock_mask |=
					    modmasks[row - 3];
					PRINT_INPUT_DEBUG(("Found NumLock on %d\n",
						rp_modifier_info.num_lock_mask));
					break;

				case XK_Scroll_Lock:
					rp_modifier_info.scroll_lock_mask |=
					    modmasks[row - 3];
					PRINT_INPUT_DEBUG(("Found ScrollLock on %d\n",
					    rp_modifier_info.scroll_lock_mask));
					break;
				default:
					break;
				}
			}
		}
	}

	/* Stolen from Emacs 21.0.90 - xterm.c */
	/* If we couldn't find any meta keys, accept any alt keys as meta keys. */
	if (!rp_modifier_info.meta_mod_mask) {
		rp_modifier_info.meta_mod_mask = rp_modifier_info.alt_mod_mask;
		rp_modifier_info.alt_mod_mask = 0;
	}
	/*
	 * If some keys are both alt and meta, make them just meta, not alt.
	 */
	if (rp_modifier_info.alt_mod_mask & rp_modifier_info.meta_mod_mask) {
		rp_modifier_info.alt_mod_mask &= ~rp_modifier_info.meta_mod_mask;
	}
	XFree((char *) syms);
	XFreeModifiermap(mods);
}

/*
 * we need a keycode + modifier to generate the proper keysym (such as @).
 * Return 1 if successful, 0 otherwise. This function can fail if a keysym
 * doesn't map to a keycode.
 */
static int
keysym_to_keycode_mod(KeySym keysym, KeyCode * code, unsigned int *mod)
{
	KeySym lower, upper;

	*mod = 0;
	*code = XKeysymToKeycode(dpy, keysym);
	lower = XkbKeycodeToKeysym(dpy, *code, 0, 0);
	upper = XkbKeycodeToKeysym(dpy, *code, 0, 1);
	/*
	 * If you need to press shift to get the keysym, add the shift mask.
	 */
	if (upper == keysym && lower != keysym)
		*mod = ShiftMask;

	return *code != 0;
}

/*
 * Grab the key while ignoring annoying modifier keys including caps lock, num
 * lock, and scroll lock.
 */
void
grab_key(KeySym keysym, unsigned int modifiers, Window grab_window)
{
	unsigned int mod_list[8];
	int i;
	KeyCode keycode;
	unsigned int mod;

	/* Convert to a modifier mask that X Windows will understand. */
	modifiers = rp_mask_to_x11_mask(modifiers);
	if (!keysym_to_keycode_mod(keysym, &keycode, &mod))
		return;
	PRINT_INPUT_DEBUG(("keycode_mod: %ld %d %d\n", keysym, keycode, mod));
	modifiers |= mod;

	/*
	 * Create a list of all possible combinations of ignored modifiers.
	 * Assumes there are only 3 ignored modifiers.
	 */
	mod_list[0] = 0;
	mod_list[1] = LockMask;
	mod_list[2] = rp_modifier_info.num_lock_mask;
	mod_list[3] = mod_list[1] | mod_list[2];
	mod_list[4] = rp_modifier_info.scroll_lock_mask;
	mod_list[5] = mod_list[1] | mod_list[4];
	mod_list[6] = mod_list[2] | mod_list[4];
	mod_list[7] = mod_list[1] | mod_list[2] | mod_list[4];

	/* Grab every combination of ignored modifiers. */
	for (i = 0; i < 8; i++) {
		XGrabKey(dpy, keycode, modifiers | mod_list[i],
		    grab_window, True, GrabModeAsync, GrabModeAsync);
	}
}


/* Return the name of the keysym. caller must free returned pointer */
char *
keysym_to_string(KeySym keysym, unsigned int modifier)
{
	struct sbuf *name;
	char *tmp;

	name = sbuf_new(0);

	if (modifier & RP_SHIFT_MASK)
		sbuf_concat(name, "S-");
	if (modifier & RP_CONTROL_MASK)
		sbuf_concat(name, "C-");
	if (modifier & RP_META_MASK)
		sbuf_concat(name, "M-");
	if (modifier & RP_ALT_MASK)
		sbuf_concat(name, "A-");
	if (modifier & RP_HYPER_MASK)
		sbuf_concat(name, "H-");
	if (modifier & RP_SUPER_MASK)
		sbuf_concat(name, "s-");

	/*
	 * On solaris machines (perhaps other machines as well) this call can
	 * return NULL. In this case use the "NULL" string.
	 */
	tmp = XKeysymToString(keysym);
	if (tmp == NULL)
		tmp = "NULL";

	sbuf_concat(name, tmp);

	return sbuf_free_struct(name);
}

/*
 * Cooks a keycode + modifier into a keysym + modifier. This should be used
 * anytime meaningful key information is to be extracted from a KeyPress or
 * KeyRelease event.
 *
 * returns the number of bytes in keysym_name. If you are not interested in the
 * keysym name pass in NULL for keysym_name and 0 for len.
 */
int
cook_keycode(XKeyEvent *ev, KeySym *keysym, unsigned int *mod,
    char *keysym_name, int len, int ignore_bad_mods)
{
	int nbytes;
	int shift = 0;
	KeySym lower, upper;

	if (ignore_bad_mods) {
		ev->state &= ~(LockMask
		    | rp_modifier_info.num_lock_mask
		    | rp_modifier_info.scroll_lock_mask);
	}
	if (len > 0)
		len--;
	nbytes = XLookupString(ev, keysym_name, len, keysym, NULL);

	/* Null terminate the string (not all X servers do it for us). */
	if (keysym_name) {
		keysym_name[nbytes] = '\0';
	}
	/* Find out if XLookupString gobbled the shift modifier */
	if (ev->state & ShiftMask) {
		lower = XkbKeycodeToKeysym(dpy, ev->keycode, 0, 0);
		upper = XkbKeycodeToKeysym(dpy, ev->keycode, 0, 1);
		/*
		 * If the keysym isn't affected by the shift key, then keep the
		 * shift modifier.
		 */
		if (lower == upper)
			shift = ShiftMask;
	}
	*mod = ev->state;
	*mod &= (rp_modifier_info.meta_mod_mask
	    | rp_modifier_info.alt_mod_mask
	    | rp_modifier_info.hyper_mod_mask
	    | rp_modifier_info.super_mod_mask
	    | ControlMask
	    | shift);

	return nbytes;
}

/* Wait for a key and discard it. */
void
read_any_key(void)
{
	char buffer[513];
	unsigned int mod;
	KeySym c;

	read_single_key(&c, &mod, buffer, sizeof(buffer));
}

/* The same as read_key, but handle focusing the key_window and reverting focus. */
int
read_single_key(KeySym *keysym, unsigned int *modifiers, char *keysym_name,
    int len)
{
	Window focus;
	int revert;
	int nbytes;

	XGetInputFocus(dpy, &focus, &revert);
	set_window_focus(rp_current_screen->key_window);
	nbytes = read_key(keysym, modifiers, keysym_name, len);
	set_window_focus(focus);

	return nbytes;
}

int
read_key(KeySym *keysym, unsigned int *modifiers, char *keysym_name, int len)
{
	XEvent ev;
	int nbytes;

	/* Read a key from the keyboard. */
	do {
		XMaskEvent(dpy, KeyPressMask | KeyRelease, &ev);
		*modifiers = ev.xkey.state;
		nbytes = cook_keycode(&ev.xkey, keysym, modifiers, keysym_name,
		    len, 0);
	} while (IsModifierKey(*keysym) || ev.xkey.type == KeyRelease);

	return nbytes;
}

static void
update_input_window(rp_screen *s, rp_input_line *line)
{
	int prompt_width, input_width, total_width;
	int char_len = 0, height;
	GC lgc;
	XGCValues gcv;

	prompt_width = rp_text_width(s, line->prompt, -1, NULL);
	input_width = rp_text_width(s, line->buffer, line->length, NULL);
	total_width = defaults.bar_x_padding * 2 + prompt_width + input_width +
	    MAX_FONT_WIDTH(defaults.font);
	height = (FONT_HEIGHT(s) + defaults.bar_y_padding * 2);

	if (isu8start(line->buffer[line->position])) {
		do {
			char_len++;
		} while (isu8cont(line->buffer[line->position + char_len]));
	} else
		char_len = 1;

	if (total_width < defaults.input_window_size + prompt_width)
		total_width = defaults.input_window_size + prompt_width;

	if (defaults.bar_sticky) {
		XWindowAttributes attr;
		XGetWindowAttributes(dpy, s->bar_window, &attr);

		if (total_width < attr.width)
			total_width = attr.width;
	}

	XMoveResizeWindow(dpy, s->input_window,
	    bar_x(s, total_width), bar_y(s, height), total_width,
	    (FONT_HEIGHT(s) + defaults.bar_y_padding * 2));

	XClearWindow(dpy, s->input_window);

	rp_draw_string(s, s->input_window, STYLE_NORMAL,
	    defaults.bar_x_padding,
	    defaults.bar_y_padding + FONT_ASCENT(s),
	    line->prompt,
	    -1, NULL, NULL);

	rp_draw_string(s, s->input_window, STYLE_NORMAL,
	    defaults.bar_x_padding + prompt_width,
	    defaults.bar_y_padding + FONT_ASCENT(s),
	    line->buffer,
	    line->length, NULL, NULL);

	gcv.function = GXxor;
	gcv.foreground = rp_glob_screen.fg_color ^ rp_glob_screen.bg_color;
	lgc = XCreateGC(dpy, s->input_window, GCFunction | GCForeground, &gcv);

	/* Draw a cheap-o cursor - MkIII */
	XFillRectangle(dpy, s->input_window, lgc,
	    defaults.bar_x_padding + prompt_width +
	    rp_text_width(s, line->buffer, line->position, NULL),
	    defaults.bar_y_padding,
	    rp_text_width(s, &line->buffer[line->position], char_len, NULL),
	    FONT_HEIGHT(s));

	XFlush(dpy);
	XFreeGC(dpy, lgc);
}

char *
get_input(char *prompt, int history_id, completion_fn fn)
{
	return get_more_input(prompt, "", history_id, BASIC, fn);
}

char *
get_more_input(char *prompt, char *preinput, int history_id,
    enum completion_styles style, completion_fn compl_fn)
{
	/* Emacs 21 uses a 513 byte string to store the keysym name. */
	char keysym_buf[513];
	rp_screen *s = rp_current_screen;
	KeySym ch;
	unsigned int modifier;
	rp_input_line *line;
	char *final_input;
	edit_status status;
	Window focus;
	int revert, done = 0;

	history_reset();

	/* Create our line structure */
	line = input_line_new(prompt, preinput, history_id, style, compl_fn);

	/* Switch to the default colormap. */
	if (current_window())
		XUninstallColormap(dpy, current_window()->colormap);
	XInstallColormap(dpy, s->def_cmap);

	XMapWindow(dpy, s->input_window);
	XRaiseWindow(dpy, s->input_window);

	hide_bar(s, 1);

	XClearWindow(dpy, s->input_window);
	/* Switch focus to our input window to read the next key events. */
	XGetInputFocus(dpy, &focus, &revert);
	set_window_focus(s->input_window);
	XSync(dpy, False);

	update_input_window(s, line);

	while (!done) {
		read_key(&ch, &modifier, keysym_buf, sizeof(keysym_buf));
		modifier = x11_mask_to_rp_mask(modifier);
		PRINT_INPUT_DEBUG(("ch = %ld, modifier = %d, keysym_buf = %s\n",
			ch, modifier, keysym_buf));
		status = execute_edit_action(line, ch, modifier, keysym_buf);

		switch (status) {
		case EDIT_COMPLETE:
		case EDIT_DELETE:
		case EDIT_INSERT:
		case EDIT_MOVE:
			/*
			 * If the text changed (and we didn't just complete
			 * something) then set the virgin bit.
			 */
			if (status != EDIT_COMPLETE)
				line->compl->virgin = 1;
			/* In all cases, we need to redisplay the input string. */
			update_input_window(s, line);
			break;
		case EDIT_NO_OP:
			break;
		case EDIT_ABORT:
			final_input = NULL;
			done = 1;
			break;
		case EDIT_DONE:
			final_input = xstrdup(line->buffer);
			done = 1;
			break;
		default:
			errx(1, "unhandled input status %d; this is a *BUG*",
			    status);
		}
	}

	/* Clean up our line structure */
	input_line_free(line);

	/* Revert focus. */
	set_window_focus(focus);

	/* Possibly restore colormap. */
	if (current_window()) {
		XUninstallColormap(dpy, s->def_cmap);
		XInstallColormap(dpy, current_window()->colormap);
	}

	/* Re-show the sticky bar if we hid it earlier */
	if (defaults.bar_sticky)
		hide_bar(s, 0);

	XUnmapWindow(dpy, s->input_window);

	/*
	 * XXX: Without this, the input window doesn't show up again if we need
	 * to prompt the user for more input, until the user types a character.
	 * Figure out what is actually causing this and remove this.
	 */
	usleep(1);

	return final_input;
}
