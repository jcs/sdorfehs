/* Ratpoison messages.
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

#ifndef _RATPOISON_MESSAGES_H
#define _RATPOISON_MESSAGES_H 1

#define MESSAGE_NO_OTHER_WINDOW "No other window"
#define MESSAGE_NO_OTHER_FRAME          "No other frame"
#define MESSAGE_NO_MANAGED_WINDOWS      "No managed windows"
#define MESSAGE_UNKNOWN_COMMAND ": unknown command '%s'"
#define MESSAGE_WINDOW_INFORMATION      "This is window %d (%s)"

#define MESSAGE_RAISE_TRANSIENT "Raise request from transient window %d (%s)"
#define MESSAGE_RAISE_WINDOW            "Raise request from window %d (%s)"
#define MESSAGE_RAISE_TRANSIENT_GROUP   "Raise request from transient window %d (%s) in group %s"
#define MESSAGE_RAISE_WINDOW_GROUP      "Raise request from window %d (%s) in group %s"
#define MESSAGE_MAP_TRANSIENT           "New transient window %d (%s)"
#define MESSAGE_MAP_WINDOW              "New window %d (%s)"
#define MESSAGE_MAP_TRANSIENT_GROUP     "New transient window %d (%s) in group %s"
#define MESSAGE_MAP_WINDOW_GROUP        "New window %d (%s) in group %s"

#define MESSAGE_PROMPT_SWITCH_TO_WINDOW "Switch to window: "
#define MESSAGE_PROMPT_NEW_WINDOW_NAME  "Set window's title to: "
#define MESSAGE_PROMPT_SHELL_COMMAND    "/bin/sh -c "
#define MESSAGE_PROMPT_COMMAND          ":"
#define MESSAGE_PROMPT_SWITCH_WM        "Switch to wm: "
#define MESSAGE_PROMPT_XTERM_COMMAND    MESSAGE_PROMPT_SHELL_COMMAND TERM_PROG " -e "
#define MESSAGE_PROMPT_SWITCH_TO_GROUP  "Switch to group: "
#define MESSAGE_PROMPT_SELECT_VAR  "Variable: "
#define MESSAGE_PROMPT_VAR_VALUE  "Value: "

#define MESSAGE_WELCOME "Welcome to ratpoison! Hit `%s %s' for help."

#define EMPTY_FRAME_MESSAGE "Current Frame"

#endif /* ! _RATPOISON_MESSAGES_H */
