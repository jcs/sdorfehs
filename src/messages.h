/* Ratpoison messages.
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

#ifndef _RATPOISON_MESSAGES_H
#define _RATPOISON_MESSAGES_H 1

#include "config.h"

#define MESSAGE_NO_OTHER_WINDOW " No other window "
#define MESSAGE_NO_MANAGED_WINDOWS " No managed windows "
#define MESSAGE_UNKNOWN_COMMAND ": unknown command "

#define MESSAGE_PROMPT_GOTO_WINDOW_NAME " Switch to window: "
#define MESSAGE_PROMPT_NEW_WINDOW_NAME " Set window's title to: "
#define MESSAGE_PROMPT_SHELL_COMMAND "/bin/sh -c "
#define MESSAGE_PROMPT_COMMAND ":"
#define MESSAGE_PROMPT_SWITCH_WM " Switch to wm: "
#define MESSAGE_PROMPT_XTERM_COMMAND MESSAGE_PROMPT_SHELL_COMMAND TERM_PROG " -e "

#endif /* ! _RATPOISON_MESSAGES_H */
