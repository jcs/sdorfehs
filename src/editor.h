/* Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
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

#ifndef _RATPOISON_EDITOR_H
#define _RATPOISON_EDITOR_H 1


typedef enum edit_status
{
  EDIT_INSERT,
  EDIT_DELETE,
  EDIT_MOVE,
  EDIT_COMPLETE,
  EDIT_ABORT,
  EDIT_DONE,
  EDIT_NO_OP
} edit_status;

/* UTF-8 handling macros */
#define RP_IS_UTF8_CHAR(c) (defaults.utf8_locale && (c) & 0xC0)
#define RP_IS_UTF8_START(c) (defaults.utf8_locale && ((c) & 0xC0) == 0xC0)
#define RP_IS_UTF8_CONT(c) (defaults.utf8_locale && ((c) & 0xC0) == 0x80)

/* Input line functions */
rp_input_line *input_line_new (char *prompt, char *preinput, int history_id, completion_fn fn);
void input_line_free (rp_input_line *line);

edit_status execute_edit_action (rp_input_line *line, KeySym ch, unsigned int modifier, char *keysym_buf);

#endif /* ! _RATPOISON_EDITOR_H */
