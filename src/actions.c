/* Copyright (C) 2000, 2001, 2002, 2003 Shawn Betts
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

#include <unistd.h>
#include <sys/wait.h>
#include <X11/keysym.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <errno.h>

#include "ratpoison.h"


/* rp_keymaps is ratpoison's list of keymaps. */
LIST_HEAD(rp_keymaps);

static user_command user_commands[] = 
  { /*@begin (tag required for genrpbindings) */
    {"abort",		cmd_abort,		arg_VOID},
    {"banish",          cmd_banish,     	arg_VOID},
    {"time", 		cmd_time, 		arg_VOID},
    {"colon",	 	cmd_colon,		arg_STRING},
    {"curframe",        cmd_curframe,   	arg_VOID},
    {"delete", 		cmd_delete, 		arg_VOID},
    {"echo", 		cmd_echo, 		arg_STRING},
    {"escape",          cmd_escape,     	arg_STRING},
    {"exec", 		cmd_exec, 		arg_STRING},
    {"focus",		cmd_next_frame,		arg_VOID},
    {"focusup",		cmd_focusup,		arg_VOID},
    {"focusdown",	cmd_focusdown,		arg_VOID},
    {"focusleft",	cmd_focusleft,		arg_VOID},
    {"focusright",	cmd_focusright,		arg_VOID},
    {"focuslast",	cmd_focuslast,		arg_VOID},
    {"meta",		cmd_meta,		arg_STRING},
    {"license",		cmd_license,		arg_VOID},
    {"help",		cmd_help,		arg_VOID},
    {"hsplit",		cmd_h_split,		arg_STRING},
    {"kill", 		cmd_kill, 		arg_VOID},
    {"redisplay", 	cmd_redisplay,		arg_VOID},
    {"newwm",		cmd_newwm,		arg_STRING},
    {"next", 		cmd_next, 		arg_VOID},
    {"number", 		cmd_number, 		arg_STRING},
    {"only",            cmd_only,       	arg_VOID},
    {"other", 		cmd_other, 		arg_VOID},
    {"gravity",		cmd_gravity, 		arg_STRING},
    {"prev", 		cmd_prev, 		arg_VOID},
    {"quit",		cmd_quit,		arg_VOID},
    {"remove",          cmd_remove,     	arg_VOID},
    {"rudeness",	cmd_rudeness,		arg_STRING},
    {"select", 		cmd_select,		arg_STRING},
    {"source",		cmd_source,		arg_STRING},
    {"split",		cmd_v_split,		arg_STRING},
    {"title", 		cmd_rename, 		arg_STRING},
    {"version",		cmd_version,		arg_VOID},
    {"vsplit",		cmd_v_split,		arg_STRING},
    {"windows", 	cmd_windows, 		arg_VOID},
    {"setenv",		cmd_setenv,		arg_STRING},
    {"getenv",		cmd_getenv,		arg_STRING},
    {"chdir",		cmd_chdir,		arg_STRING},
    {"unsetenv",	cmd_unsetenv,		arg_STRING},
    {"info",		cmd_info,		arg_VOID},
    {"lastmsg",		cmd_lastmsg,		arg_VOID},
    {"restart",		cmd_restart,		arg_VOID},
    {"startup_message",	cmd_startup_message, 	arg_STRING},
    {"link",		cmd_link,		arg_STRING},
    {"alias",		cmd_alias,		arg_STRING},
    {"unalias",		cmd_unalias,		arg_STRING},
    {"prevscreen",	cmd_prevscreen,		arg_VOID},
    {"nextscreen",	cmd_nextscreen,		arg_VOID},
    {"warp",		cmd_warp,		arg_STRING},
    {"resize",		cmd_resize,		arg_STRING},
    {"shrink",		cmd_shrink,		arg_VOID},
    {"tmpwm",		cmd_tmpwm,		arg_STRING},
    {"fselect",		cmd_fselect,		arg_VOID},
    {"fdump",		cmd_fdump,		arg_STRING},
    {"frestore",	cmd_frestore,		arg_STRING},
    {"verbexec", 	cmd_verbexec, 		arg_STRING},
    {"unmanage",	cmd_unmanage,		arg_STRING},
    {"clrunmanaged",	cmd_clrunmanaged, 	arg_VOID},
    {"gnext",		cmd_gnext,		arg_VOID},
    {"gprev", 		cmd_gprev, 		arg_VOID},
    {"gnew", 		cmd_gnew, 		arg_VOID},
    {"gnewbg", 		cmd_gnewbg, 		arg_VOID},
    {"gselect",		cmd_gselect,		arg_STRING},
    {"gdelete",		cmd_gdelete,		arg_STRING},
    {"groups",		cmd_groups,		arg_VOID},
    {"gmove",		cmd_gmove,		arg_VOID},
    {"gmerge",		cmd_gmerge,		arg_VOID},
    {"addhook",         cmd_addhook,            arg_STRING},
    {"remhook",         cmd_remhook,            arg_STRING},
    {"listhook",        cmd_listhook,           arg_STRING},
    {"readkey",          cmd_readkey,             arg_STRING},
    {"newkmap",          cmd_newkmap,             arg_STRING},
    {"delkmap",          cmd_delkmap,             arg_STRING},
    {"definekey",          cmd_definekey,             arg_STRING},

    /* Commands to set default behavior. */
    {"defbargravity",		cmd_defbargravity,	arg_STRING},
    {"msgwait",			cmd_msgwait,	 	arg_STRING},
    {"defborder", 		cmd_defborder, 		arg_STRING},
    {"deffont", 		cmd_deffont, 		arg_STRING},
    {"definputwidth",		cmd_definputwidth,	arg_STRING},
    {"defmaxsizegravity",	cmd_defmaxsizegravity, 	arg_STRING},
    {"defpadding", 		cmd_defpadding, 	arg_STRING},
    {"defbarborder",		cmd_defbarborder,	arg_STRING},
    {"deftransgravity",		cmd_deftransgravity,	arg_STRING},
    {"defwaitcursor",		cmd_defwaitcursor,	arg_STRING},
    {"defwinfmt", 		cmd_defwinfmt, 		arg_STRING},
    {"defwinname", 		cmd_defwinname,		arg_STRING},
    {"defwingravity",		cmd_defwingravity,	arg_STRING},
    {"deffgcolor",		cmd_deffgcolor,		arg_STRING},
    {"defbgcolor",		cmd_defbgcolor,		arg_STRING},
    {"defbarpadding", 		cmd_defbarpadding, 	arg_STRING},
    {"defresizeunit", 		cmd_defresizeunit, 	arg_STRING},
    {"defwinliststyle",		cmd_defwinliststyle,	arg_STRING},
    {"defframesels",            cmd_defframesels,       arg_STRING},
    /*@end (tag required for genrpbindings) */

    /* Commands to help debug ratpoison. */
#ifdef DEBUG
#endif

    /* the following screen commands may or may not be able to be
       implemented.  See the screen documentation for what should be
       emulated with these commands */
#if 0
    {"msgminwait",	cmd_unimplemented,	arg_VOID},
    {"nethack",		cmd_unimplemented,	arg_VOID},
    {"sleep",		cmd_unimplemented,	arg_VOID},
    {"stuff", 		cmd_unimplemented, 	arg_VOID},
#endif
    {0,			0,		0} };

typedef struct
{
  char *name;
  char *alias;
} alias_t;
  
static alias_t *alias_list;
static int alias_list_size;
static int alias_list_last;

rp_action*
find_keybinding_by_action (char *action, rp_keymap *map)
{
  int i;

  for (i=0; i<map->actions_last; i++)
    {
      if (!strcmp (map->actions[i].data, action))
	{
	  return &map->actions[i];
	}
    }

  return NULL;
}

rp_action*
find_keybinding (KeySym keysym, int state, rp_keymap *map)
{
  int i;
  for (i = 0; i < map->actions_last; i++)
    {
      if (map->actions[i].key == keysym 
	  && map->actions[i].state == state)
	return &map->actions[i];
    }
  return NULL;
}

static char *
find_command_by_keydesc (char *desc, rp_keymap *map)
{
  int i = 0;
  char *keysym_name;

  while (i < map->actions_last)
    {
      keysym_name = keysym_to_string (map->actions[i].key, map->actions[i].state);
      if (!strcmp (keysym_name, desc))
        {
          free (keysym_name);
          return map->actions[i].data;
        }
      free (keysym_name);
      i++;
    }

  return NULL;
}

static char *
resolve_command_from_keydesc (char *desc, int depth, rp_keymap *map)
{
  char *cmd, *command;

  command = find_command_by_keydesc (desc, map);
  if (!command)
    return NULL;

  /* is it a link? */
  if (strncmp (command, "link", 4) || depth > MAX_LINK_DEPTH)
    /* it is not */
    return command;

  cmd = resolve_command_from_keydesc (&command[5], depth + 1, map);
  return (cmd != NULL) ? cmd : command;
}


static void
add_keybinding (KeySym keysym, int state, char *cmd, rp_keymap *map)
{
  if (map->actions_last >= map->actions_size)
    {
      /* double the key table size */
      map->actions_size *= 2;
      map->actions = (rp_action*) xrealloc (map->actions, sizeof (rp_action) * map->actions_size);
      PRINT_DEBUG (("realloc()ed key_table %d\n", map->actions_size));
    }

  map->actions[map->actions_last].key = keysym;
  map->actions[map->actions_last].state = state;
  /* free this on shutdown, or re/unbinding */
  map->actions[map->actions_last].data = xstrdup (cmd); 

  map->actions_last++;
}

static void
replace_keybinding (rp_action *key_action, char *newcmd)
{
  if (strlen (key_action->data) < strlen (newcmd))
    key_action->data = (char*) realloc (key_action->data, strlen (newcmd) + 1);

  strcpy (key_action->data, newcmd);
}

static int
remove_keybinding (KeySym keysym, int state, rp_keymap *map)
{
  int i;
  int found = -1;

  for (i=0; i<map->actions_last; i++)
    {
      if (map->actions[i].key == keysym && map->actions[i].state == state)
	{
	  found = i;
	  break;
	}
    }

  if (found >= 0)
    {
      free (map->actions[found].data);

      memmove (&map->actions[found], &map->actions[found+1], 
	       sizeof (rp_action) * (map->actions_last - found - 1));
      map->actions_last--;
      
      return 1;
    }

  return 0;
}

rp_keymap *
keymap_new (char *name)
{
  rp_keymap *map;

  /* All keymaps must have a name. */
  if (name == NULL) 
    return NULL;

  map = xmalloc (sizeof (rp_keymap));
  map->name = xstrdup (name);
  map->actions_size = 1;
  map->actions = (rp_action*) xmalloc (sizeof (rp_action) * map->actions_size);
  map->actions_last = 0;

  return map;
}

rp_keymap *
find_keymap (char *name)
{
  rp_keymap *cur;

  list_for_each_entry (cur, &rp_keymaps, node)
    {
      if (!strcmp (name, cur->name))
	{
	  return cur;
	}
    }

  return NULL;
}

/* Search the alias table for a match. If a match is found, return its
   index into the table. Otherwise return -1. */
static int
find_alias_index (char *name)
{
  int i;

  for (i=0; i<alias_list_last; i++)
    if (!strcmp (name, alias_list[i].name))
      return i;

  return -1;
}

static void
add_alias (char *name, char *alias)
{
  int index;

  /* Are we updating an existing alias, or creating a new one? */
  index = find_alias_index (name);
  if (index >= 0)
    {
      free (alias_list[index].alias);
      alias_list[index].alias = xstrdup (alias);
    }
  else
    {
      if (alias_list_last >= alias_list_size)
	{
	  alias_list_size *= 2;
	  alias_list = xrealloc (alias_list, sizeof (alias_t) * alias_list_size);
	}

      alias_list[alias_list_last].name = xstrdup (name);
      alias_list[alias_list_last].alias = xstrdup (alias);
      alias_list_last++;
    }
}

void
initialize_default_keybindings (void)
{
  rp_keymap *map, *top;

  map = keymap_new (ROOT_KEYMAP);
  list_add (&map->node, &rp_keymaps);

  top = keymap_new (TOP_KEYMAP);
  list_add (&top->node, &rp_keymaps);

  /* Initialive the alias list. */
  alias_list_size = 5;
  alias_list_last = 0;
  alias_list = xmalloc (sizeof (alias_t) * alias_list_size);

  prefix_key.sym = KEY_PREFIX;
  prefix_key.state = MODIFIER_PREFIX;

  /* Add the prefix key to the top-level map. */
  add_keybinding (prefix_key.sym, prefix_key.state, "readkey " ROOT_KEYMAP, top);

  add_keybinding (prefix_key.sym, prefix_key.state, "other", map);
  add_keybinding (prefix_key.sym, 0, "meta", map);
  add_keybinding (XK_g, RP_CONTROL_MASK, "abort", map);
  add_keybinding (XK_0, 0, "select 0", map);
  add_keybinding (XK_1, 0, "select 1", map);
  add_keybinding (XK_2, 0, "select 2", map);
  add_keybinding (XK_3, 0, "select 3", map);
  add_keybinding (XK_4, 0, "select 4", map);
  add_keybinding (XK_5, 0, "select 5", map);
  add_keybinding (XK_6, 0, "select 6", map);
  add_keybinding (XK_7, 0, "select 7", map);
  add_keybinding (XK_8, 0, "select 8", map);
  add_keybinding (XK_9, 0, "select 9", map);
  add_keybinding (XK_minus, 0, "select -", map);
  add_keybinding (XK_A, 0, "title", map);
  add_keybinding (XK_A, RP_CONTROL_MASK, "title", map);
  add_keybinding (XK_K, 0, "kill", map);
  add_keybinding (XK_K, RP_CONTROL_MASK, "kill", map);
  add_keybinding (XK_Return, 0, "next", map);
  add_keybinding (XK_Return, RP_CONTROL_MASK,	"next", map);
  add_keybinding (XK_a, 0, "time", map);
  add_keybinding (XK_a, RP_CONTROL_MASK, "time", map);
  add_keybinding (XK_b, 0, "banish", map);
  add_keybinding (XK_b, RP_CONTROL_MASK, "banish", map);
  add_keybinding (XK_c, 0, "exec " TERM_PROG, map);
  add_keybinding (XK_c, RP_CONTROL_MASK, "exec " TERM_PROG, map);
  add_keybinding (XK_colon, 0, "colon", map);
  add_keybinding (XK_exclam, 0, "exec", map);
  add_keybinding (XK_exclam, RP_CONTROL_MASK, "colon exec " TERM_PROG " -e ", map);
  add_keybinding (XK_i, 0, "info", map);
  add_keybinding (XK_i, RP_CONTROL_MASK, "info", map);
  add_keybinding (XK_k, 0, "delete", map);
  add_keybinding (XK_k, RP_CONTROL_MASK, "delete", map);
  add_keybinding (XK_l, 0, "redisplay", map);
  add_keybinding (XK_l, RP_CONTROL_MASK, "redisplay", map);
  add_keybinding (XK_m, 0, "lastmsg", map);
  add_keybinding (XK_m, RP_CONTROL_MASK, "lastmsg", map);
  add_keybinding (XK_n, 0, "next", map);
  add_keybinding (XK_n, RP_CONTROL_MASK, "next", map);
  add_keybinding (XK_p, 0, "prev", map);
  add_keybinding (XK_p, RP_CONTROL_MASK, "prev", map);
  add_keybinding (XK_quoteright, 0, "select", map);
  add_keybinding (XK_quoteright, RP_CONTROL_MASK, "select", map);
  add_keybinding (XK_space, 0, "next", map);
  add_keybinding (XK_space, RP_CONTROL_MASK, "next", map);
  add_keybinding (XK_v, 0, "version", map);
  add_keybinding (XK_v, RP_CONTROL_MASK, "version", map);
  add_keybinding (XK_V, 0, "license", map);
  add_keybinding (XK_V, RP_CONTROL_MASK, "license", map);
  add_keybinding (XK_w, 0, "windows", map);
  add_keybinding (XK_w, RP_CONTROL_MASK, "windows", map);
  add_keybinding (XK_s, 0, "split", map);
  add_keybinding (XK_s, RP_CONTROL_MASK, "split", map);
  add_keybinding (XK_S, 0, "hsplit", map);
  add_keybinding (XK_S, RP_CONTROL_MASK, "hsplit", map);
  add_keybinding (XK_Tab, 0, "focus", map);
  add_keybinding (XK_Tab, RP_META_MASK, "focuslast", map);
  add_keybinding (XK_Q, 0, "only", map);
  add_keybinding (XK_R, 0, "remove", map);
  add_keybinding (XK_f, 0, "fselect", map);
  add_keybinding (XK_f, RP_CONTROL_MASK, "fselect", map);
  add_keybinding (XK_F, 0, "curframe", map);
  add_keybinding (XK_r, 0, "resize", map);
  add_keybinding (XK_r, RP_CONTROL_MASK, "resize", map);
  add_keybinding (XK_question, 0, "help " ROOT_KEYMAP, map);

  add_alias ("unbind", "definekey " ROOT_KEYMAP);
  add_alias ("bind", "definekey " ROOT_KEYMAP);
}

void
keymap_free (rp_keymap *map)
{
  int i;

  /* Free the data in the actions. */
  for (i=0; i<map->actions_last; i++)
    {
      free (map->actions[i].data);
    }

  /* Free the map data. */
  free (map->actions);
  free (map->name);

  /* ...and the map itself. */
  free (map);
}

void
free_keymaps ()
{
  rp_keymap *cur;
  struct list_head *tmp, *iter;

  list_for_each_safe_entry (cur, iter, tmp, &rp_keymaps, node)
    {
      list_del (&cur->node);
      keymap_free (cur);
    }
}

void
free_aliases ()
{
  int i;

  /* Free the alias data. */
  for (i=0; i<alias_list_last; i++)
    {
      free (alias_list[i].name);
      free (alias_list[i].alias);
    }

  /* Free the alias list. */
  free (alias_list);
}

/* return a KeySym from a string that contains either a hex value or
   an X keysym description */
static int string_to_keysym (char *str) 
{ 
  int retval; 
  int keysym;

  retval = sscanf (str, "0x%x", &keysym);

  if (!retval || retval == EOF)
    keysym = XStringToKeysym (str);

  return keysym;
}

/* Parse a key description. 's' is, naturally, the key description. */
struct rp_key*
parse_keydesc (char *s)
{
  static struct rp_key key;
  struct rp_key *p = &key;
  char *token, *next_token, *keydesc;

  if (s == NULL) 
    return NULL;

  /* Avoid mangling s. */
  keydesc = xstrdup (s);

  p->state = 0;
  p->sym = 0;

  if (!strchr (keydesc, '-'))
    {
      /* Its got no hyphens in it, so just grab the keysym */
      p->sym = string_to_keysym (keydesc);
    }
  else
    {
      /* Its got hyphens, so parse out the modifiers and keysym */
      token = strtok (keydesc, "-");

      if (token == NULL)
	{
	  /* It was nothing but hyphens */
	  free (keydesc);
	  return NULL;
	}

      do
	{
	  next_token = strtok (NULL, "-");

	  if (next_token == NULL)
	    {
	      /* There is nothing more to parse and token contains the
		 keysym name. */
	      p->sym = string_to_keysym (token);
	    }
	  else
	    {
	      /* Which modifier is it? Only accept modifiers that are
		 present. ie don't accept a hyper modifier if the keymap
		 has no hyper key. */
	      if (!strcmp (token, "C"))
		{
		  p->state |= RP_CONTROL_MASK;
		}
	      else if (!strcmp (token, "M"))
		{
		  p->state |= RP_META_MASK;
		}
	      else if (!strcmp (token, "A"))
		{
		  p->state |= RP_ALT_MASK;
		}
	      else if (!strcmp (token, "S"))
		{
		  p->state |= RP_SUPER_MASK;
		}
	      else if (!strcmp (token, "H"))
		{
		  p->state |= RP_HYPER_MASK;
		}
	      else
		{
		  free (keydesc);
		  return NULL;
		}
	    }

	  token = next_token;
	} while (next_token != NULL);
    }

  free (keydesc);

  if (!p->sym)
    return NULL;
  else
    return p;
}

/* Unmanage window */
char *
cmd_unmanage (int interactive, char *data)
{
  if (data == NULL && !interactive)
    return list_unmanaged_windows();

  if (data)
    add_unmanaged_window(data);
  else message(" unmanage: at least one argument required ");

  return NULL;
}

/* Clear the unmanaged window list */
char *
cmd_clrunmanaged (int interactive, char *data)
{
  clear_unmanaged_list();
  return NULL;
}

char *
cmd_definekey (int interactive, char *data)
{
  rp_keymap *map;
  struct rp_key *key;
  char *cmd;
  char *keydesc;
  char *token, *tmp;

  if (!data)
    {
      message (" definekey: at least two arguments required ");
      return NULL;
    }

  /* Make a copy of the input. */
/*   tmp = xstrdup (data); */
  tmp = xmalloc (strlen (data) + 1);
  strcpy (tmp, data);

  /* Read the keymap */
  token = strtok (tmp, " ");
  map = find_keymap (token);

  /* Make sure the keymap exists */
  if (map == NULL)
    {
      marked_message_printf (0, 0, " definekey: keymap '%s' not found ", token);
      free (tmp);
      return NULL;
    }

  /* Read the key description */
  token = strtok (NULL, " ");

  if (!token)
    {
      message (" definekey: at least two arguments required ");
      free (tmp);
      return NULL;
    }

  keydesc = xstrdup (token);

  /* Read the command. */
  token = strtok (NULL, "");
  if (token)
    cmd = xstrdup (token);
  else
    cmd = NULL;

  free (tmp);

  /* Parse the key description. Do this after the above strtok,
     because parse_keydesc uses strtok. */
  key = parse_keydesc (keydesc);

  if (key == NULL)
    {
      marked_message_printf (0, 0, " definekey: unknown key '%s' ", keydesc);
      free (keydesc);
      if (cmd)
	free (cmd);
      return NULL;
    }

  /* Gobble remaining whitespace before command starts */
  if (cmd)
    {
      tmp = cmd;
      while (*cmd == ' ')
	{
	  cmd++;
	}
      /* Do a little dance to make sure we don't leak. */
      cmd = xstrdup (cmd);
      free (tmp);
    }

  /* If we're updating the top level map, we'll need to update the
     keys grabbed. */
  if (map == find_keymap (TOP_KEYMAP))
    ungrab_keys_all_wins ();

  if (!cmd || !*cmd)
    {
      /* If no comand is specified, then unbind the key. */
      if (!remove_keybinding (key->sym, key->state, map))
	marked_message_printf (0, 0, " definekey: key '%s' is not bound ", keydesc);
    }
  else
    {
      rp_action *key_action;

      if ((key_action = find_keybinding (key->sym, key->state, map)))
	replace_keybinding (key_action, cmd);
      else
	add_keybinding (key->sym, key->state, cmd, map);
    }

  /* Update the grabbed keys. */
  if (map == find_keymap (TOP_KEYMAP))
    grab_keys_all_wins ();
  XSync (dpy, False);

  free (keydesc);
  if (cmd)
    free (cmd);

  return NULL;  
}

char *
cmd_unimplemented (int interactive, char *data)
{
  marked_message (" FIXME:  unimplemented command ",0,8);

  return NULL;
}

char *
cmd_source (int interactive, char *data)
{
  FILE *fileptr;

  if ((fileptr = fopen (data, "r")) == NULL)
    marked_message_printf (0, 0, " source: %s : %s ", data, strerror(errno));
  else
    {
      set_close_on_exec (fileptr);
      read_rc_file (fileptr);
      fclose (fileptr);
    }

  return NULL;
}

char *
cmd_meta (int interactive, char *data)
{
  XEvent ev1, ev;
  ev = rp_current_event;

  if (current_window() == NULL) return NULL;

  ev1.xkey.type = KeyPress;
  ev1.xkey.display = dpy;
  ev1.xkey.window = current_window()->w;
  ev1.xkey.state = rp_mask_to_x11_mask (prefix_key.state);
  ev1.xkey.keycode = XKeysymToKeycode (dpy, prefix_key.sym);

  XSendEvent (dpy, current_window()->w, False, KeyPressMask, &ev1);

  /*   XTestFakeKeyEvent (dpy, XKeysymToKeycode (dpy, 't'), True, 0); */

  XSync (dpy, False);

  return NULL;
}

char *
cmd_prev (int interactive, char *data)
{
  if (!current_window())
    {
      set_active_window (group_prev_window (rp_current_group, NULL));
      if (!current_window())
	message (MESSAGE_NO_MANAGED_WINDOWS);
    }
  else
    {
      rp_window *win;

      win = group_prev_window (rp_current_group, current_window());
      if (win == NULL)
	message (MESSAGE_NO_OTHER_WINDOW);
      else
	set_active_window (win);
    }

  return NULL;
}

char *
cmd_prev_frame (int interactive, char *data)
{
  rp_frame *frame;

  frame = find_frame_next (current_frame());
  if (!frame)
    message (MESSAGE_NO_OTHER_FRAME);
  else
    set_active_frame (frame);

  return NULL;
}

char *
cmd_next (int interactive, char *data)
{
  if (!current_window())
    {
      set_active_window (group_next_window (rp_current_group, NULL));
      if (!current_window())
	message (MESSAGE_NO_MANAGED_WINDOWS);
    }
  else
    {
      rp_window *win;

      win = group_next_window (rp_current_group, current_window());
      if (win == NULL)
	message (MESSAGE_NO_OTHER_WINDOW);
      else
	set_active_window (win);
    }

  return NULL;
}

char *
cmd_next_frame (int interactive, char *data)
{
  rp_frame *frame;

  frame = find_frame_next (current_frame());
  if (!frame)
    message (MESSAGE_NO_OTHER_FRAME);
  else
    set_active_frame (frame);

  return NULL;
}

char *
cmd_other (int interactive, char *data)
{
  rp_window *w;

/*   w = find_window_other (); */
  w = group_last_window (rp_current_group);

  if (!w)
    message (MESSAGE_NO_OTHER_WINDOW);
  else
    set_active_window (w);

  return NULL;
}

static int
string_to_window_number (char *str)
{
  int i;
  char *s;

  for (i = 0, s = str; *s; s++)
    {
      if (*s < '0' || *s > '9')
        break;
      i = i * 10 + (*s - '0');
    }

  return *s ? -1 : i;
}


struct list_head *
trivial_completions (char* str)
{
  struct list_head *list;

  /* Initialize our list. */
  list = xmalloc (sizeof (struct list_head));
  INIT_LIST_HEAD (list);

  return list;
}

struct list_head *
window_completions (char* str)
{
  rp_window_elem *cur;
  struct list_head *list;

  /* Initialize our list. */
  list = xmalloc (sizeof (struct list_head));
  INIT_LIST_HEAD (list);

  /* Gather the names of all the windows. */
  list_for_each_entry (cur, &rp_current_group->mapped_windows, node)
    {
      struct sbuf *name;

      name = sbuf_new (0);
      sbuf_copy (name, window_name (cur->win));
      list_add_tail (&name->node, list);
    }

  return list;
}

/* switch to window number or name */
char *
cmd_select (int interactive, char *data)
{
  char *str;
  int n;

  if (data == NULL)
    str = get_input (MESSAGE_PROMPT_SWITCH_TO_WINDOW, window_completions);
  else
    str = xstrdup (data);

  /* User aborted. */
  if (str == NULL)
    return NULL;

  /* Only search if the string contains something to search for. */
  if (strlen (str) > 0)
    {
      if (strlen (str) == 1 && str[0] == '-')
	{
	  blank_frame (current_frame());
	}
      /* try by number */
      else if ((n = string_to_window_number (str)) >= 0)
	{
	  rp_window_elem *elem = group_find_window_by_number (rp_current_group, n);

	  if (elem)
	    goto_window (elem->win);
	  else
	    /* show the window list as feedback */
	    show_bar (current_screen ());
	}
      else
	/* try by name */
	{
	  rp_window *win = find_window_name (str);

	  if (win)
	    goto_window (win);
	  else
	    marked_message_printf (0, 0, " select: unknown window '%s' ", str);
	}
    }

  free (str);

  return NULL;
}

char *
cmd_rename (int interactive, char *data)
{
  char *winname;
  
  if (current_window() == NULL) return NULL;

  if (data == NULL)
    winname = get_input (MESSAGE_PROMPT_NEW_WINDOW_NAME, trivial_completions);
  else
    winname = xstrdup (data);

  /* User aborted. */
  if (winname == NULL)
    return NULL;

  if (*winname)
    {
      free (current_window()->user_name);
      current_window()->user_name = xmalloc (sizeof (char) * strlen (winname) + 1);

      strcpy (current_window()->user_name, winname);

      current_window()->named = 1;
  
      /* Update the program bar. */
      update_window_names (current_screen());
    }

  free (winname);

  return NULL;
}


char *
cmd_delete (int interactive, char *data)
{
  XEvent ev;
  int status;

  if (current_window() == NULL) return NULL;

  ev.xclient.type = ClientMessage;
  ev.xclient.window = current_window()->w;
  ev.xclient.message_type = wm_protocols;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = wm_delete;
  ev.xclient.data.l[1] = CurrentTime;

  status = XSendEvent(dpy, current_window()->w, False, 0, &ev);
  if (status == 0)
    PRINT_DEBUG (("Delete window failed\n"));

  return NULL;
}

char *
cmd_kill (int interactive, char *data)
{
  if (current_window() == NULL) return NULL;

  XKillClient(dpy, current_window()->w);

  return NULL;
}

char *
cmd_version (int interactive, char *data)
{
  message (" " PACKAGE " " VERSION " ");
  return NULL;
}

char *
command (int interactive, char *data)
{
  /* This static counter is used to exit from recursive alias calls. */
  static int alias_recursive_depth = 0;
  char *result = NULL;
  char *cmd, *rest;
  char *input;
  user_command *uc;
  int i;
  
  if (data == NULL)
    return NULL;

  /* get a writable copy for strtok() */
  input = xstrdup (data);

  cmd = strtok (input, " ");

  if (cmd == NULL)
    goto done;

  rest = strtok (NULL, "\0");

  /* Gobble whitespace */
  if (rest)
    {
      while (*rest == ' ')
	rest++;
      /* If rest is empty, then we have no argument. */
      if (*rest == '\0')
	rest = NULL;
    }

  PRINT_DEBUG (("cmd==%s rest==%s\n", cmd, rest));

  /* Look for it in the aliases, first. */
  for (i=0; i<alias_list_last; i++)
    {
      if (!strcmp (cmd, alias_list[i].name))
	{
	  struct sbuf *s;

	  /* Append any arguments onto the end of the alias' command. */
	  s = sbuf_new (0);
	  sbuf_concat (s, alias_list[i].alias);
	  if (rest != NULL)
	    sbuf_printf_concat (s, " %s", rest);

	  alias_recursive_depth++;
	  if (alias_recursive_depth >= MAX_ALIAS_RECURSIVE_DEPTH)
	    message (" command: alias recursion has exceeded maximum depth ");
	  else
	    result = command (interactive, sbuf_get (s));
	  alias_recursive_depth--;

	  sbuf_free (s);
	  goto done;
	}
    }

  /* If it wasn't an alias, maybe its a command. */
  for (uc = user_commands; uc->name; uc++)
    {
      if (!strcmp (cmd, uc->name))
	{
	  result = uc->func (interactive, rest);
	  goto done;
	}
    }

  marked_message_printf (0, 0, MESSAGE_UNKNOWN_COMMAND, cmd);

 done:
  free (input);

  return result;
}

struct list_head *
colon_completions (char* str)
{
  int i;
  struct sbuf *s;
  struct list_head *list;

  /* Initialize our list. */
  list = xmalloc (sizeof (struct list_head));
  INIT_LIST_HEAD (list);

  /* Put all the aliases in our list. */
  for(i=0; i<alias_list_last; ++i)
    {
      s = sbuf_new (0);
      sbuf_copy (s, alias_list[i].name);
      /* The space is so when the user completes a space is
	 conveniently inserted after the command. */
      sbuf_concat (s, " ");
      list_add_tail (&s->node, list);
    }

  /* Put all the commands in our list. */
  for(i=0; user_commands[i].name; ++i)
    {
      s = sbuf_new (0);
      sbuf_copy (s, user_commands[i].name);
      /* The space is so when the user completes a space is
	 conveniently inserted after the command. */
      sbuf_concat (s, " ");
      list_add_tail (&s->node, list);
    }

  return list;
}

char *
cmd_colon (int interactive, char *data)
{
  char *result;
  char *input;

  if (data == NULL)
    input = get_input (MESSAGE_PROMPT_COMMAND, colon_completions);
  else
    input = get_more_input (MESSAGE_PROMPT_COMMAND, data, colon_completions);

  /* User aborted. */
  if (input == NULL)
    return NULL;

  result = command (1, input);

  /* Gobble the result. */
  if (result)
    free (result);
  
  free (input);

  return NULL;
}

struct list_head *
exec_completions (char *str)
{
  size_t n = 256;
  char *partial;
  struct sbuf *line;
  FILE *file;
  struct list_head *head;
  char *completion_string;

  /* Initialize our list. */
  head = xmalloc (sizeof (struct list_head));
  INIT_LIST_HEAD (head);
	
  /* FIXME: A Bash dependancy?? */
  completion_string = xsprintf("bash -c \"compgen -ac %s|sort\"", str);
  file = popen (completion_string, "r");
  free (completion_string);
  if (!file)
    {
      PRINT_ERROR (("popen failed\n"));
      return head;
    }

  partial = (char*)xmalloc (n);

  /* Read data from the file, split it into lines and store it in a
     list. */
  line = sbuf_new (0);
  while (fgets (partial, n, file) != NULL)
    {
      /* Read a chunk from the file into our line accumulator. */
      sbuf_concat (line, partial);

      if (feof(file) || (*(sbuf_get (line) + strlen(sbuf_get (line)) - 1) == '\n'))
	{
	  char *s;
	  struct sbuf *elem;

	  s = sbuf_get (line);

	  /* Frob the newline into */
	  if (*(s + strlen(s) - 1) == '\n')
	    *(s + strlen(s) - 1) = '\0';
	      
	  /* Add our line to the list. */
	  elem = sbuf_new (0);
	  sbuf_copy (elem, s);
	  /* The space is so when the user completes a space is
	     conveniently inserted after the command. */
	  sbuf_concat (elem, " ");
	  list_add_tail (&elem->node, head);

	  sbuf_clear (line);
	}
    }

  free (partial);
  pclose (file);

  return head;
}

char *
cmd_exec (int interactive, char *data)
{
  char *cmd;

  if (data == NULL)
    cmd = get_input (MESSAGE_PROMPT_SHELL_COMMAND, exec_completions);
  else
    cmd = xstrdup (data);

  /* User aborted. */
  if (cmd == NULL)
    return NULL;

  spawn (cmd);

  free (cmd);

  return NULL;
}


int
spawn(char *cmd)
{
  rp_child_info *child;
  int pid;

  pid = fork();
  if (pid == 0) 
    {
      /* Some process setup to make sure the spawned process runs
	 in its own session. */
      putenv(current_screen()->display_string);
#ifdef HAVE_SETSID
      setsid();
#endif
#if defined (HAVE_SETPGID)
      setpgid (0, 0);
#elif defined (HAVE_SETPGRP)
      setpgrp (0, 0);
#endif
      execl("/bin/sh", "sh", "-c", cmd, 0);
      _exit(EXIT_FAILURE);
    }

/*   wait((int *) 0); */
  PRINT_DEBUG (("spawned %s\n", cmd));

  /* Add this child process to our list. */
  child = malloc (sizeof (rp_child_info));
  child->cmd = strdup (cmd);
  child->pid = pid;
  child->terminated = 0;

  list_add (&child->node, &rp_children);

  return pid;
}

/* Switch to a different Window Manager. Thanks to 
"Chr. v. Stuckrad" <stucki@math.fu-berlin.de> for the patch. */
char *
cmd_newwm(int interactive, char *data)
{
  char *prog;

  if (data == NULL)
    prog = get_input (MESSAGE_PROMPT_SWITCH_WM, trivial_completions);
  else
    prog = xstrdup (data);

  /* User aborted. */
  if (prog == NULL)
    return NULL;

  PRINT_DEBUG (("Switching to %s\n", prog));

  putenv(current_screen()->display_string);
  execlp(prog, prog, 0);

  PRINT_ERROR (("exec %s ", prog));
  perror(" failed");

  free (prog);

  return NULL;
}

char *
cmd_quit(int interactive, char *data)
{
  kill_signalled = 1;
  return NULL;
}

/* Show the current time on the bar. Thanks to Martin Samuelsson
   <cosis@lysator.liu.se> for the patch. Thanks to Jonathan Walther
   <krooger@debian.org> for making it pretty. */
char *
cmd_time (int interactive, char *data)
{
  char *msg, *tmp;
  time_t timep;

  timep = time(NULL);
  tmp = ctime(&timep);
  msg = xmalloc (strlen (tmp));
  strncpy(msg, tmp, strlen (tmp) - 1);	/* Remove the newline */
  msg[strlen(tmp) - 1] = 0;

  marked_message_printf (0, 0, " %s ", msg);
  free (msg);
  
  return NULL;
}

/* Assign a new number to a window ala screen's number command. */
char *
cmd_number (int interactive, char *data)
{
  int old_number, new_number;
  rp_window_elem *other_win, *win;
  char *str;
  char *tmp;

  if (data == NULL)
    {
      print_window_information (current_window());
      return NULL;
    }

  /* Read in the number requested by the user. */
  str = xstrdup (data);
  tmp = strtok (str, " ");
  if (tmp)
    {
      new_number = string_to_window_number (tmp);
      if (new_number < 0)
	{
	  message (" number: invalid argument ");
	  free (str);
	  return NULL;
	}
    }
  else
    {
      /* Impossible, but we'll live with it. */
      print_window_information (current_window());
      free (str);
      return NULL;
    }      

  /* Get the rest of the string and see if the user specified a target
     window. */
  tmp = strtok (NULL, "");
  if (tmp)
    {
      int win_number;

      PRINT_DEBUG (("2nd: '%s'\n", tmp));

      win_number = string_to_window_number (tmp);
      if (win_number < 0)
	{
	  message (" number: invalid argument ");
	  free (str);
	  return NULL;
	}

      win = group_find_window_by_number (rp_current_group, win_number);
    }
  else
    {
      PRINT_DEBUG (("2nd: NULL\n"));
      win = group_find_window (&rp_current_group->mapped_windows, current_window());
    }

  /* Make the switch. */
  if ( new_number >= 0 && win)
    {
      /* Find other window with same number and give it old number. */
      other_win = group_find_window_by_number (rp_current_group, new_number);
      if (other_win != NULL)
	{
	  old_number = win->number;
	  other_win->number = old_number;

	  /* Resort the window in the list */
	  group_resort_window (rp_current_group, other_win);
	}
      else
	{
	  numset_release (rp_current_group->numset, win->number);
	}

      win->number = new_number;
      numset_add_num (rp_current_group->numset, new_number);

      /* resort the the window in the list */
      group_resort_window (rp_current_group, win);

      /* Update the window list. */
      update_window_names (win->win->scr);
    }

  free (str);

  return NULL;
}

/* Toggle the display of the program bar */
char *
cmd_windows (int interactive, char *data)
{
  struct sbuf *window_list = NULL;
  char *tmp;
  int dummy;
  rp_screen *s;

  if (interactive)
    {
      s = current_screen ();
      /* This is a yukky hack. If the bar already hidden then show the
	 bar. This handles the case when msgwait is 0 (the bar sticks)
	 and the user uses this command to toggle the bar on and
	 off. OR the timeout is >0 then show the bar. Which means,
	 always show the bar if msgwait is >0 which fixes the case
	 when a command in the prefix hook displays the bar. */
      if (!hide_bar (s) || defaults.bar_timeout > 0) show_bar (s);
      
      return NULL;
    }
  else
    {
      window_list = sbuf_new (0);
      if (data)
	get_window_list (data, "\n", window_list, &dummy, &dummy);
      else
	get_window_list (defaults.window_fmt, "\n", window_list, &dummy, &dummy);
      tmp = sbuf_get (window_list);
      free (window_list);

      return tmp;
    }
}

char *
cmd_abort (int interactive, char *data)
{
  return NULL;
}  

/* Redisplay the current window by sending 2 resize events. */
char *
cmd_redisplay (int interactive, char *data)
{
  force_maximize (current_window());
  return NULL;
}

/* Reassign the prefix key. */
char *
cmd_escape (int interactive, char *data)
{
  struct rp_key *key;
  rp_action *action;
  rp_keymap *map, *top;

  top = find_keymap (TOP_KEYMAP);
  map = find_keymap (ROOT_KEYMAP);
  key = parse_keydesc (data);

  if (key)
    {
      /* Update the "other" keybinding */
      action = find_keybinding(prefix_key.sym, prefix_key.state, map);
      if (action != NULL && !strcmp (action->data, "other"))
	{
	  action->key = key->sym;
	  action->state = key->state;
	}

      /* Update the "meta" keybinding */
      action = find_keybinding(prefix_key.sym, 0, map);
      if (action != NULL && !strcmp (action->data, "meta"))
	{
	  action->key = key->sym;
	  action->state = 0;
	}

      /* Remove the grab on the current prefix key */
      ungrab_keys_all_wins();

      action = find_keybinding(prefix_key.sym, prefix_key.state, top);
      if (action != NULL && !strcmp (action->data, "readkey " ROOT_KEYMAP))
	{
	  action->key = key->sym;
	  action->state = key->state;
	}

      /* Add the grab for the new prefix key */
      grab_keys_all_wins();

      /* Finally, keep track of the current prefix. */
      prefix_key.sym = key->sym;
      prefix_key.state = key->state;
    }
  else
    {
      marked_message_printf (0, 0, " escape: unknown key '%s' ", data);
    }

  return NULL;
}

/* User accessible call to display the passed in string. */
char *
cmd_echo (int interactive, char *data)
{
  if (data)
    marked_message_printf (0, 0, " %s ", data);

  return NULL;
}

static int
read_split (const char *str, int max)
{
  int a, b, p;

  if (sscanf(str, "%d/%d", &a, &b) == 2)
    {
      p = (int)(max * (float)(a) / (float)(b));
    }
  else if (sscanf(str, "%d", &p) == 1)
    {
      if (p < 0)
	p = max + p;
    }
  else
    {
      /* Failed to read input. */
      return -1;
    }

  /* Input out of range. */
  if (p <= 0 || p >= max)
    return -1;

  return p;
}

char *
cmd_v_split (int interactive, char *data)
{
  rp_frame *frame;
  int pixels;

  frame = current_frame();

  /* Default to dividing the frame in half. */
  if (data == NULL)
    pixels = frame->height / 2;
  else 
    pixels = read_split (data, frame->height);

  if (pixels > 0)
    h_split_frame (frame, pixels);
  else
    message (" vsplit: invalid argument ");    

  return NULL;
}

char *
cmd_h_split (int interactive, char *data)
{
  rp_frame *frame;
  int pixels;

  frame = current_frame();

  /* Default to dividing the frame in half. */
  if (data == NULL)
    pixels = frame->width / 2;
  else
    pixels = read_split (data, frame->width);

  if (pixels > 0)
    v_split_frame (frame, pixels);
  else
    message (" hsplit: invalid argument ");    

  return NULL;
}

char *
cmd_only (int interactive, char *data)
{
  remove_all_splits();
  maximize (current_window());

  return NULL;
}

char *
cmd_remove (int interactive, char *data)
{
  rp_screen *s = current_screen();
  rp_frame *frame;

  if (num_frames(s) <= 1)
    {
      message (" remove: cannot remove only frame ");
      return NULL;
    }

  frame = find_frame_next (current_frame());

  if (frame)
    {
      remove_frame (current_frame());
      set_active_frame (frame);
      show_frame_indicator();
    }

  return NULL;
}

char *
cmd_shrink (int interactive, char *data)
{
  resize_shrink_to_window (current_frame());
  return NULL;
}

char *
cmd_resize (int interactive, char *data)
{
  rp_screen *s = current_screen ();

  /* If the user calls resize with arguments, treat it like the
     non-interactive version. */
  if (interactive && data == NULL)
    {
      int nbytes;
      char buffer[513];
      unsigned int mod;
      KeySym c;
      struct list_head *bk;

      /* If we haven't got at least 2 frames, there isn't anything to
	 scale. */
      if (num_frames (s) < 2) return NULL;

      XGrabKeyboard (dpy, s->key_window, False, GrabModeSync, GrabModeAsync, CurrentTime);

      /* Save the frameset in case the user aborts. */
      bk = screen_copy_frameset (s);

      while (1)
	{
	  show_frame_message (" Resize frame ");
	  nbytes = read_key (&c, &mod, buffer, sizeof (buffer));

	  /* Convert the mask to be compatible with ratpoison. */
	  mod = x11_mask_to_rp_mask (mod);

	  if (c == RESIZE_VGROW_KEY && mod == RESIZE_VGROW_MODIFIER)
	    resize_frame_vertically (current_frame(), defaults.frame_resize_unit);
	  else if (c == RESIZE_VSHRINK_KEY && mod == RESIZE_VSHRINK_MODIFIER)
	    resize_frame_vertically (current_frame(), -defaults.frame_resize_unit);
	  else if (c == RESIZE_HGROW_KEY && mod == RESIZE_HGROW_MODIFIER)
	    resize_frame_horizontally (current_frame(), defaults.frame_resize_unit);
	  else if (c == RESIZE_HSHRINK_KEY && mod == RESIZE_HSHRINK_MODIFIER)
	    resize_frame_horizontally (current_frame(), -defaults.frame_resize_unit);
	  else if (c == RESIZE_SHRINK_TO_WINDOW_KEY 
		   && mod == RESIZE_SHRINK_TO_WINDOW_MODIFIER)
	    resize_shrink_to_window (current_frame());
	  else if (c == INPUT_ABORT_KEY && mod == INPUT_ABORT_MODIFIER)
	    {
	      rp_frame *cur;

	      screen_restore_frameset (s, bk);
	      list_for_each_entry (cur, &s->frames, node)
		{
		  maximize_all_windows_in_frame (cur);
		}
	      break;
	    }
	  else if (c == RESIZE_END_KEY && mod == RESIZE_END_MODIFIER)
	    {
	      frameset_free (bk);
	      break;
	    }
	}

      /* It is our responsibility to free this. */
      free (bk);

      hide_frame_indicator ();
      XUngrabKeyboard (dpy, CurrentTime);
    }
  else
    {
      int xdelta, ydelta;
      
      if (data == NULL)
	{
	  message (" resize: two numeric arguments required ");
	  return NULL;
	}

      if (sscanf (data, "%d %d", &xdelta, &ydelta) < 2)
	{
	  message (" resize: two numeric arguments required ");
	  return NULL;
	}

      resize_frame_horizontally (current_frame(), xdelta);
      resize_frame_vertically (current_frame(), ydelta);
    }

  return NULL;
}

char *
cmd_defresizeunit (int interactive, char *data)
{
  int tmp;

  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.frame_resize_unit);

  if (data == NULL || sscanf (data, "%d", &tmp) < 1)
    {
      message (" defresizeunit: one argument required ");
      return NULL;
    }

  if (tmp >= 0)
    defaults.frame_resize_unit = tmp;
  else
    message (" defresizeunit: invalid argument ");

  return NULL;
}

/* banish the rat pointer */
char *
cmd_banish (int interactive, char *data)
{
  rp_screen *s;

  s = current_screen ();

  XWarpPointer (dpy, None, s->root, 0, 0, 0, 0, s->left + s->width - 2, s->top + s->height - 2); 
  return NULL;
}

char *
cmd_curframe (int interactive, char *data)
{
  show_frame_indicator();
  return NULL;
}

/* Thanks to Martin Samuelsson <cosis@lysator.liu.se> for the
   original patch. */
char *
cmd_license (int interactive, char *data)
{
  rp_screen *s = current_screen();
  XEvent ev;
  int x = 10;
  int y = 10;
  int i;
  int max_width = 0;
  char *license_text[] = { PACKAGE " " VERSION,
			   "",
			   "Copyright (C) 2000, 2001 Shawn Betts",
			   "",
			   "ratpoison is free software; you can redistribute it and/or modify ",
			   "it under the terms of the GNU General Public License as published by ",
			   "the Free Software Foundation; either version 2, or (at your option) ",
			   "any later version.",
			   "",
			   "ratpoison is distributed in the hope that it will be useful, ",
			   "but WITHOUT ANY WARRANTY; without even the implied warranty of ",
			   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the ",
			   "GNU General Public License for more details.",
			   "",
			   "You should have received a copy of the GNU General Public License ",
			   "along with this software; see the file COPYING.  If not, write to ",
			   "the Free Software Foundation, Inc., 59 Temple Place, Suite 330, ",
			   "Boston, MA 02111-1307 USA",
			   "",
			   "Send bugreports, fixes, enhancements, t-shirts, money, beer & pizza ",
			   "to ratpoison-devel@lists.sourceforge.net or visit ",
			   "http://ratpoison.sourceforge.net/",
			   "",
			   "[Press any key to end.] ",
			   NULL};

  XMapRaised (dpy, s->help_window);
  XGrabKeyboard (dpy, s->help_window, False, GrabModeSync, GrabModeAsync, CurrentTime);

  /* Find the longest line. */
  for(i=0; license_text[i]; i++)
    {
      int tmp;

      tmp = XTextWidth (defaults.font, license_text[i], strlen (license_text[i]));
      if (tmp > max_width)
	max_width = tmp;
    }

  /* Offset the text so its in the center. */
  x = s->left + (s->width - max_width) / 2;
  y = s->top + (s->height - i * FONT_HEIGHT (defaults.font)) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  /* Print the text. */
  for(i=0; license_text[i]; i++)
  {
    XDrawString (dpy, s->help_window, s->normal_gc,
	         x, y + defaults.font->max_bounds.ascent,
	         license_text[i], strlen (license_text[i]));

    y += FONT_HEIGHT (defaults.font);
  }

  /* Wait for a key press. */
  XMaskEvent (dpy, KeyPressMask, &ev);

  /* Revert focus. */
  XUngrabKeyboard (dpy, CurrentTime);
  XUnmapWindow (dpy, s->help_window);

  /* The help window overlaps the bar, so redraw it. */
  if (current_screen()->bar_is_raised)
    show_last_message();

  return NULL;
}

char *
cmd_help (int interactive, char *data)
{
  rp_keymap *map;

  if (data == NULL)
    {
      message (" help: One argument required ");
      return NULL;
    }

  map = find_keymap (data);
  if (map == NULL)
    {
      marked_message_printf (0, 0, " help: keymap '%s' not found ", data);
      return NULL;
    }



  if (interactive)
    {
      rp_screen *s = current_screen();
      XEvent ev;
      int i, old_i;
      int x = 10;
      int y = 0;
      int max_width = 0;
      int drawing_keys = 1;		/* 1 if we are drawing keys 0 if we are drawing commands */
      char *keysym_name;

      XMapRaised (dpy, s->help_window);
      XGrabKeyboard (dpy, s->help_window, False, GrabModeSync, GrabModeAsync, CurrentTime);

      XDrawString (dpy, s->help_window, s->normal_gc,
                   10, y + defaults.font->max_bounds.ascent,
                   "ratpoison key bindings", strlen ("ratpoison key bindings"));

      y += FONT_HEIGHT (defaults.font) * 2;

      XDrawString (dpy, s->help_window, s->normal_gc,
                   10, y + defaults.font->max_bounds.ascent,
                   "Command key: ", strlen ("Command key: "));

      keysym_name = keysym_to_string (prefix_key.sym, prefix_key.state);
      XDrawString (dpy, s->help_window, s->normal_gc,
                   10 + XTextWidth (defaults.font, "Command key: ", strlen ("Command key: ")),
                   y + defaults.font->max_bounds.ascent,
                   keysym_name, strlen (keysym_name));
      free (keysym_name);

      y += FONT_HEIGHT (defaults.font) * 2;

      i = 0;
      old_i = 0;
      while (i<map->actions_last || drawing_keys)
	{
	  if (drawing_keys)
	    {
	      keysym_name = keysym_to_string (map->actions[i].key, map->actions[i].state);

	      XDrawString (dpy, s->help_window, s->normal_gc,
			   x, y + defaults.font->max_bounds.ascent,
			   keysym_name, strlen (keysym_name));

	      if (XTextWidth (defaults.font, keysym_name, strlen (keysym_name)) > max_width)
		max_width = XTextWidth (defaults.font, keysym_name, strlen (keysym_name));

	      free (keysym_name);
	    }
	  else
	    {
	      XDrawString (dpy, s->help_window, s->normal_gc,
			   x, y + defaults.font->max_bounds.ascent,
			   map->actions[i].data, strlen (map->actions[i].data));

	      if (XTextWidth (defaults.font, map->actions[i].data, strlen (map->actions[i].data)) > max_width)
		{
		  max_width = XTextWidth (defaults.font, map->actions[i].data, strlen (map->actions[i].data));
		}
	    }

	  y += FONT_HEIGHT (defaults.font);
	  /* Make sure the next line fits entirely within the window. */
	  if (y + FONT_HEIGHT (defaults.font) >= (s->top + s->height))
	    {
	      if (drawing_keys)
		{
		  x += max_width + 10;
		  drawing_keys = 0;
		  i = old_i;
		}
	      else
		{
		  x += max_width + 20;
		  drawing_keys = 1;
		  i++;
		  old_i = i;
		}

	      max_width = 0;
	      y = FONT_HEIGHT (defaults.font) * 4;
	    }
	  else
	    {
	      i++;
	      if (i >= map->actions_last && drawing_keys)
		{
		  x += max_width + 10;
		  drawing_keys = 0;
		  y = FONT_HEIGHT (defaults.font) * 4;
		  i = old_i;
		  max_width = 0;
		}
	    }
	}

      XMaskEvent (dpy, KeyPressMask, &ev);
      XUngrabKeyboard (dpy, CurrentTime);
      XUnmapWindow (dpy, s->help_window);

      /* The help window overlaps the bar, so redraw it. */
      if (current_screen()->bar_is_raised)
        show_last_message();

      return NULL;
    }
  else
    {
      struct sbuf *help_list;
      char *keysym_name;
      char *tmp;
      int i;

      help_list = sbuf_new (0);

      for (i = 0; i < map->actions_last; i++)
	{
	  keysym_name = keysym_to_string (map->actions[i].key, map->actions[i].state);
	  sbuf_concat (help_list, keysym_name);
	  free (keysym_name);
	  sbuf_concat (help_list, " ");
	  sbuf_concat (help_list, map->actions[i].data);
	  if (i < map->actions_last - 1)
	    sbuf_concat (help_list, "\n");
	}

      tmp = sbuf_get (help_list);
      free (help_list);

      return tmp;
    }

  return NULL;
}

char *
cmd_rudeness (int interactive, char *data)
{
  int num;

  if (data == NULL && !interactive)
    return xsprintf ("%d", 
		     rp_honour_transient_raise
		     | (rp_honour_normal_raise << 1)
		     | (rp_honour_transient_map << 2)
		     | (rp_honour_normal_map << 3));
		     
  if (data == NULL)
    {
      message (" rudeness: one argument required ");
      return NULL;
    }

  if (sscanf (data, "%d", &num) < 1 || num < 0 || num > 15)
    {
      marked_message_printf (0, 0, " rudeness: invalid level '%s' ", data);
      return NULL;
    }

  rp_honour_transient_raise = num & 1 ? 1 : 0;
  rp_honour_normal_raise    = num & 2 ? 1 : 0;
  rp_honour_transient_map   = num & 4 ? 1 : 0;
  rp_honour_normal_map      = num & 8 ? 1 : 0;

  return NULL;
}

static int
parse_wingravity (char *data)
{
  int ret = -1;

  if (!strcasecmp (data, "northwest") || !strcasecmp (data, "nw"))
    ret = NorthWestGravity;
  if (!strcasecmp (data, "north") || !strcasecmp (data, "n"))
    ret = NorthGravity;
  if (!strcasecmp (data, "northeast") || !strcasecmp (data, "ne"))
    ret = NorthEastGravity;
  if (!strcasecmp (data, "west") || !strcasecmp (data, "w"))
    ret = WestGravity;
  if (!strcasecmp (data, "center") || !strcasecmp (data, "c"))
    ret = CenterGravity;
  if (!strcasecmp (data, "East") || !strcasecmp (data, "e"))
    ret = EastGravity;
  if (!strcasecmp (data, "southwest") || !strcasecmp (data, "sw"))
    ret = SouthWestGravity;
  if (!strcasecmp (data, "South") || !strcasecmp (data, "s"))
    ret = SouthGravity;
  if (!strcasecmp (data, "southeast") || !strcasecmp (data, "se"))
    ret = SouthEastGravity;

  return ret;
}

static char *
wingravity_to_string (int g)
{
  switch (g)
    {
    case NorthWestGravity:
      return "nw";
    case WestGravity:
      return "w";
    case SouthWestGravity:
      return "sw";
    case NorthGravity:
      return "n";
    case CenterGravity:
      return "c";
    case SouthGravity:
      return "s";
    case NorthEastGravity:
      return "ne";
    case EastGravity:
      return "e";
    case SouthEastGravity:
      return "se";
    }

  PRINT_DEBUG (("Unknown gravity!\n"));
  return "Unknown";
}

char *
cmd_gravity (int interactive, char *data)
{
  int gravity;
  rp_window *win;

  if (current_window() == NULL) return NULL;
  win = current_window();

  if (data == NULL && !interactive) 
    return xstrdup (wingravity_to_string (win->gravity));

  if (data == NULL)
    {
      message (" gravity: one argument required ");
      return NULL;
    }

  if ((gravity = parse_wingravity (data)) < 0)
    message (" gravity: unknown gravity ");
  else
    {
      win->gravity = gravity;
      maximize (win);
    }

  return NULL;
}

char *
cmd_defwingravity (int interactive, char *data)
{
  int gravity;

  if (data == NULL && !interactive) 
    return xstrdup (wingravity_to_string (defaults.win_gravity));

  if (data == NULL)
    {
      message (" defwingravity: one argument required ");
      return NULL;
    }

  if ((gravity = parse_wingravity (data)) < 0)
    message (" defwingravity: unknown gravity ");
  else
    defaults.win_gravity = gravity;

  return NULL;
}

char *
cmd_deftransgravity (int interactive, char *data)
{
  int gravity;

  if (data == NULL && !interactive)
    return xstrdup (wingravity_to_string (defaults.trans_gravity));
 
  if (data == NULL)
    {
      message (" deftransgravity: one argument required ");
      return NULL;
    }

  if ((gravity = parse_wingravity (data)) < 0)
    message (" deftransgravity: unknown gravity ");
  else
    defaults.trans_gravity = gravity;

  return NULL;
}

char *
cmd_defmaxsizegravity (int interactive, char *data)
{
  int gravity;

  if (data == NULL && !interactive)
    return xstrdup (wingravity_to_string (defaults.maxsize_gravity));

  if (data == NULL)
    {
      message (" defmaxsizegravity: one argument required ");
      return NULL;
    }

  if ((gravity = parse_wingravity (data)) < 0)
    message (" defmaxsizegravity: unknown gravity ");
  else
    defaults.maxsize_gravity = gravity;

  return NULL;
}

char *
cmd_msgwait (int interactive, char *data)
{
  int tmp;

  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.bar_timeout);
    
  if (data == NULL)
    {
      message (" msgwait: one argument required ");
      return NULL;
    }

  if (sscanf (data, "%d", &tmp) < 1 || tmp < 0)
    message (" msgwait: invalid argument ");    
  else
    defaults.bar_timeout = tmp;

  return NULL;
}

char *
cmd_defbargravity (int interactive, char *data)
{
  int gravity;

  if (data == NULL && !interactive)
    return xstrdup (wingravity_to_string (defaults.bar_location));

  if (data == NULL)
    {
      message (" defbargravity: one argument required ");
      return NULL;
    }

  if ((gravity = parse_wingravity (data)) < 0)
    message (" defbargravity: unknown gravity ");
  else
    defaults.bar_location = gravity;

  return NULL;
}

static void
update_gc (rp_screen *s)
{
  XGCValues gv;

  gv.foreground = s->fg_color;
  gv.background = s->bg_color;
  gv.function = GXcopy;
  gv.line_width = 1;
  gv.subwindow_mode = IncludeInferiors;
  gv.font = defaults.font->fid;
  XFreeGC (dpy, s->normal_gc);
  s->normal_gc = XCreateGC(dpy, s->root, 
			   GCForeground | GCBackground 
			   | GCFunction | GCLineWidth
			   | GCSubwindowMode | GCFont, &gv);
}

static void
update_all_gcs ()
{
  int i;

  for (i=0; i<num_screens; i++)
    {
      update_gc (&screens[i]);
    }
}


char *
cmd_deffont (int interactive, char *data)
{
  XFontStruct *font;

  if (data == NULL) return NULL;

  font = XLoadQueryFont (dpy, data);
  if (font == NULL)
    {
      message (" deffont: unknown font ");
      return NULL;
    }

  /* Save the font as the default. */
  XFreeFont (dpy, defaults.font);
  defaults.font = font;
  update_all_gcs();

  return NULL;
}

char *
cmd_defpadding (int interactive, char *data)
{
  rp_frame *frame;
  int l, t, r, b;

  if (data == NULL && !interactive)
    return xsprintf ("%d %d %d %d", 
		     defaults.padding_left,
		     defaults.padding_right,
		     defaults.padding_top,
		     defaults.padding_bottom);

  if (data == NULL
      || sscanf (data, "%d %d %d %d", &l, &t, &r, &b) < 4)
    {
      message (" defpadding: four arguments required ");
      return NULL;
    }

  /* Resize the frames to make sure they are not too big and not too
     small. */
  list_for_each_entry (frame,&(current_screen()->frames),node)
    {
      int bk_pos, bk_len;

      /* Resize horizontally. */
      bk_pos = frame->x;
      bk_len = frame->width;

      if (frame->x == defaults.padding_left)
	{
	  frame->x = l;
	  frame->width += bk_pos - l;
	}

      if ((bk_pos + bk_len) == (current_screen()->left + current_screen()->width - defaults.padding_right))
	frame->width = current_screen()->left + current_screen()->width - r - frame->x;

      /* Resize vertically. */
      bk_pos = frame->y;
      bk_len = frame->height;

      if (frame->y == defaults.padding_top)
	{
	  frame->y = t;
	  frame->height += bk_pos - t;
	}

      if ((bk_pos + bk_len) == (current_screen()->top + current_screen()->height - defaults.padding_bottom))
	frame->height = current_screen()->top + current_screen()->height - b - frame->y;

      maximize_all_windows_in_frame (frame);
    }

  defaults.padding_left   = l;
  defaults.padding_right  = r;
  defaults.padding_top 	  = t;
  defaults.padding_bottom = b;

  return NULL;
}

char *
cmd_defborder (int interactive, char *data)
{
  int tmp;
  rp_window *win;

  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.window_border_width);

  if (data == NULL)
    {
      message (" defborder: one argument required ");
      return NULL;
    }

  if (sscanf (data, "%d", &tmp) < 1 || tmp < 0)
    {
      message (" defborder: invalid argument ");
      return NULL;
    }

  defaults.window_border_width = tmp;

  /* Update all the visible windows. */
  list_for_each_entry (win,&rp_mapped_window,node)
    {
      if (win_get_frame (win))
	maximize (win);
    }

  return NULL;
}

char *
cmd_defbarborder (int interactive, char *data)
{
  int tmp;
  int i;

  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.bar_border_width);

  if (data == NULL)
    {
      message (" defbarborder: one argument required ");
      return NULL;
    }

  if (sscanf (data, "%d", &tmp) < 1 || tmp < 0)
    {
      message (" defbarborder: invalid argument ");
      return NULL;
    }

  defaults.bar_border_width = tmp;

  /* Update the frame and bar windows. */
  for (i=0; i<num_screens; i++)
    {
      XSetWindowBorderWidth (dpy, screens[i].bar_window, defaults.bar_border_width);
      XSetWindowBorderWidth (dpy, screens[i].frame_window, defaults.bar_border_width);
      XSetWindowBorderWidth (dpy, screens[i].input_window, defaults.bar_border_width);
    }

  return NULL;
}

char *
cmd_definputwidth (int interactive, char *data)
{
  int tmp;

  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.input_window_size);

  if (data == NULL)
    {
      message (" definputwidth: one argument required ");
      return NULL;
    }

  if (sscanf (data, "%d", &tmp) < 1 || tmp < 0)
    message (" definputwidth: invalid argument ");
  else
    defaults.input_window_size = tmp;

  return NULL;
}

char *
cmd_defwaitcursor (int interactive, char *data)
{
  if (data == NULL && !interactive)
    return xsprintf ("%d", defaults.wait_for_key_cursor);

  if (data == NULL
      || sscanf (data, "%d", &defaults.wait_for_key_cursor) < 1)
    {
      message (" defwaitcursor: one argument required ");
    }

  return NULL;    
}

char *
cmd_defwinfmt (int interactive, char *data)
{
  if (data == NULL && !interactive)
    return xstrdup (defaults.window_fmt);

  if (data == NULL)
    return NULL;

  free (defaults.window_fmt);
  defaults.window_fmt = xstrdup (data);

  return NULL;
}

char *
cmd_defwinname (int interactive, char *data)
{
  char *name;

  if (data == NULL && !interactive)
    switch (defaults.win_name)
      {
      case WIN_NAME_TITLE:
	return xstrdup ("title");
      case WIN_NAME_RES_NAME:
	return xstrdup ("name");
      case WIN_NAME_RES_CLASS:
	return xstrdup ("class");
      default:
	PRINT_DEBUG (("Unknown win_name\n"));
	return xstrdup ("unknown");
      }

  if (data == NULL)
    {
      message (" defwinname: one argument required ");
      return NULL;
    }

  name = data;

  /* FIXME: Using strncmp is sorta dirty since `title' and
     `titlefoobar' would both match. But its quick and dirty. */
  if (!strncmp (name, "title", 5))
    defaults.win_name = WIN_NAME_TITLE;
  else if (!strncmp (name, "name", 4))
    defaults.win_name = WIN_NAME_RES_NAME;
  else if (!strncmp (name, "class", 5))
    defaults.win_name = WIN_NAME_RES_CLASS;
  else
    message (" defwinname: invalid argument ");

  return NULL;      
}

char *
cmd_deffgcolor (int interactive, char *data)
{
  int i;
  XColor color, junk;

  if (data == NULL)
    {
      message (" deffgcolor: one argument required ");
      return NULL;
    }

  for (i=0; i<num_screens; i++)
    {
      if (!XAllocNamedColor (dpy, screens[i].def_cmap, data, &color, &junk))
	{
	  message (" deffgcolor: unknown color ");
	  return NULL;
	}

      screens[i].fg_color = color.pixel;
      update_gc (&screens[i]);
      XSetWindowBorder (dpy, screens[i].bar_window, color.pixel);
      XSetWindowBorder (dpy, screens[i].input_window, color.pixel);
      XSetWindowBorder (dpy, screens[i].frame_window, color.pixel);
      XSetWindowBorder (dpy, screens[i].help_window, color.pixel);
    }

  return NULL;
}

char *
cmd_defbgcolor (int interactive, char *data)
{
  int i;
  XColor color, junk;

  if (data == NULL)
    {
      message (" defbgcolor: one argument required ");
      return NULL;
    }

  for (i=0; i<num_screens; i++)
    {
      if (!XAllocNamedColor (dpy, screens[i].def_cmap, data, &color, &junk))
	{
	  message (" defbgcolor: unknown color ");
	  return NULL;
	}

      screens[i].bg_color = color.pixel;
      update_gc (&screens[i]);
      XSetWindowBackground (dpy, screens[i].bar_window, color.pixel);
      XSetWindowBackground (dpy, screens[i].input_window, color.pixel);
      XSetWindowBackground (dpy, screens[i].frame_window, color.pixel);
      XSetWindowBackground (dpy, screens[i].help_window, color.pixel);
    }

  return NULL;
}

char *
cmd_setenv (int interactive, char *data)
{
  char *token, *dup;
  struct sbuf *env;

  if (data == NULL)
    {
      message (" setenv: two arguments required ");
      return NULL;
    }

  /* Setup the environment string. */
  env = sbuf_new(0);

  /* Get the 2 arguments. */
  dup = xstrdup (data);
  token = strtok (dup, " ");
  if (token == NULL)
    {
      message (" setenv: two arguments required ");
      free (dup);
      sbuf_free (env);
      return NULL;
    }
  sbuf_concat (env, token);
  sbuf_concat (env, "=");

  token = strtok (NULL, "\0");
  if (token == NULL)
    {
      message (" setenv: two arguments required ");
      free (dup);
      sbuf_free (env);
      return NULL;
    }
  sbuf_concat (env, token);

  free (dup);

  /* Stick it in the environment. */
  PRINT_DEBUG(("%s\n", sbuf_get(env)));
  putenv (sbuf_get (env));

  /* According to the docs, the actual string is placed in the
     environment, not the data the string points to. This means
     modifying the string (or freeing it) directly changes the
     environment. So, don't free the environment string, just the sbuf
     data structure. */
  env->data = NULL;
  sbuf_free (env);

  return NULL;
}

char *
cmd_getenv (int interactive, char *data)
{
  char *var;
  char *result = NULL;
  char *value;

  if (data == NULL)
    {
      message (" getenv: one argument required ");
      return NULL;
    }

  /* Get the argument. */
  var = xmalloc (strlen (data) + 1);
  if (sscanf (data, "%s", var) < 1)
    {
      message (" getenv: one argument required ");
      free (var);
      return NULL;
    }

  value = getenv (var);

  if (interactive)
    {
      marked_message_printf (0, 0, " %s ", value);
      return NULL;
    }

  if (value)
    {
      result = xmalloc (strlen (value) + 1);
      strcpy (result, getenv (var));
    }

  return result;
}

/* Thanks to Gergely Nagy <algernon@debian.org> for the original
   patch. */
char *
cmd_chdir (int interactive, char *data)
{
  char *dir;

  if (!data)
    {
      dir = getenv ("HOME");
      if (dir == NULL || *dir == '\0')
        {
	  message (" chdir: HOME not set ");
          return NULL;
	}
    }
  else
    dir = data;

  if (chdir (dir) == -1)
    marked_message_printf (0, 0, " chdir: %s : %s ", dir, strerror(errno));

  return NULL;
}

/* Thanks to Gergely Nagy <algernon@debian.org> for the original
   patch. */
char *
cmd_unsetenv (int interactive, char *data)
{
  if (data == NULL)
    {
      message (" unsetenv: one argument is required ");
      return NULL;
    }

  /* Remove all instances of the env. var. */
  putenv (data);

  return NULL;
}

/* Thanks to Gergely Nagy <algernon@debian.org> for the original
   patch. */
char *
cmd_info (int interactive, char *data)
{
  if (current_window() == NULL)
    {
      marked_message_printf (0, 0, " (%d, %d) No window ",
			     current_screen()->width,
			     current_screen()->height);
    }
  else
    {
      rp_window *win = current_window();
      marked_message_printf (0, 0, " (%d,%d) %d(%s) %s", win->width, win->height,
			     win->number, window_name (win), win->transient ? "Transient ":"");
    }

  return NULL;
}

/* Thanks to Gergely Nagy <algernon@debian.org> for the original
   patch. */
char *
cmd_lastmsg (int interactive, char *data)
{
  show_last_message();
  return NULL;
}

char *
cmd_focusup (int interactive, char *data)
{
  rp_frame *frame;

  if ((frame = find_frame_up (current_frame())))
    set_active_frame (frame);

  return NULL;
}

char *
cmd_focusdown (int interactive, char *data)
{
  rp_frame *frame;

  if ((frame = find_frame_down (current_frame())))
    set_active_frame (frame);

  return NULL;
}

char *
cmd_focusleft (int interactive, char *data)
{
  rp_frame *frame;

  if ((frame = find_frame_left (current_frame())))
    set_active_frame (frame);

  return NULL;
}

char *
cmd_focusright (int interactive, char *data)
{
  rp_frame *frame;

  if ((frame = find_frame_right (current_frame())))
    set_active_frame (frame);

  return NULL;
}

char *
cmd_restart (int interactive, char *data)
{
  hup_signalled = 1;
  return NULL;
}

char *
cmd_startup_message (int interactive, char *data)
{
  if (data == NULL && !interactive)
    return xsprintf ("%s", defaults.startup_message ? "on":"off");

  if (data == NULL)
    {
      message (" startup_message: one argument required ");
      return NULL;
    }

  if (!strcasecmp (data, "on"))
    defaults.startup_message = 1;
  else if (!strcasecmp (data, "off"))
    defaults.startup_message = 0;
  else
    message (" startup_message: invalid argument ");

  return NULL;
}

char *
cmd_focuslast (int interactive, char *data)
{
  rp_frame *frame = find_last_frame();

  if (frame)
      set_active_frame (frame);
  else
    message (" focuslast: no other frame ");

  return NULL;
}

char *
cmd_link (int interactive, char *data)
{
/*   char *cmd = NULL; */

/*   if (!data) */
/*     return NULL; */

/*   cmd = resolve_command_from_keydesc (data, 0); */
/*   if (cmd) */
/*     return command (interactive, cmd); */

  return NULL;
}

/* Thanks to Doug Kearns <djkea2@mugc.its.monash.edu.au> for the
   original patch. */
char *
cmd_defbarpadding (int interactive, char *data)
{
  int x, y;

  if (data == NULL && !interactive)
    return xsprintf ("%d %d", defaults.bar_x_padding, defaults.bar_y_padding);

  if (data == NULL
      || sscanf (data, "%d %d", &x, &y) < 2) 
    {
      message (" defbarpadding: two arguments required ");
      return NULL;
    }

  if (x >= 0 && y >= 0)
    {
      defaults.bar_x_padding = x;
      defaults.bar_y_padding = y;
    }
  else
    {
      message (" defbarpadding: invalid argument ");    
    }
  return NULL;
}

char *
cmd_alias (int interactive, char *data)
{
  char *name, *alias;
  
  if (data == NULL)
    {
      message (" alias: two arguments required ");
      return NULL;
    }

  /* Parse out the arguments. */
  name = strtok (data, " ");
  alias = strtok (NULL, "\0");

  if (name == NULL || alias == NULL)
    {
      message (" alias: two arguments required ");
      return NULL;
    }

  /* Add or update the alias. */
  add_alias (name, alias);

  return NULL;
}

char *
cmd_unalias (int interactive, char *data)
{
  char *name;
  int index;
  
  if (data == NULL)
    {
      message (" unalias: one argument required ");
      return NULL;
    }

  /* Parse out the arguments. */
  name = data;

  /* Are we updating an existing alias, or creating a new one? */
  index = find_alias_index (name);
  if (index >= 0)
    {
      char *tmp;

      alias_list_last--;

      /* Free the alias and put the last alias in the the space to
	 keep alias_list from becoming sparse. This code must jump
	 through some hoops to correctly handle the case when
	 alias_list_last == index. */
      tmp = alias_list[index].alias;
      alias_list[index].alias = xstrdup (alias_list[alias_list_last].alias);
      free (tmp);
      free (alias_list[alias_list_last].alias);

      /* Do the same for the name element. */
      tmp = alias_list[index].name;
      alias_list[index].name = xstrdup (alias_list[alias_list_last].name);
      free (tmp);
      free (alias_list[alias_list_last].name);
    }
  else
    {
      message (" unalias: alias not found ");
    }

  return NULL;  
}

char *
cmd_nextscreen (int interactive, char *data)
{
  int new_screen;

  /* No need to go through the motions when we don't have to. */
  if (num_screens <= 1)
    {
      message (" nextscreen: no other screen ");
      return NULL;
    }
  new_screen = rp_current_screen + 1;
  if (new_screen >= num_screens)
    new_screen = 0;

  set_active_frame (screen_get_frame (&screens[new_screen], screens[new_screen].current_frame));

  return NULL;
}

char *
cmd_prevscreen (int interactive, char *data)
{
  int new_screen;

  /* No need to go through the motions when we don't have to. */
  if (num_screens <= 1) 
    {
      message (" prevscreen: no other screen ");
      return NULL;
    }

  new_screen = rp_current_screen - 1;
  if (new_screen < 0)
    new_screen = num_screens - 1;

  set_active_frame (screen_get_frame (&screens[new_screen], screens[new_screen].current_frame));

  return NULL;
}

char *
cmd_warp (int interactive, char *data)
{
  if (data == NULL && !interactive)
    return xsprintf ("%s", defaults.warp ? "on":"off");

  if (data == NULL)
    {
      message (" warp: one argument required ");
      return NULL;
    }

  if (!strcasecmp (data, "on"))
    defaults.warp = 1;
  else if (!strcasecmp (data, "off"))
    defaults.warp = 0;
  else
    message (" warp: invalid argument ");
 
  return NULL;
}

/* FIXME: This is sorely broken.

  UPDATE: I changed the list_for_each_entry to
  list_for_each_entry_safe, does this make it less broken?
  (rcyeske@vcn.bc.ca 20040125) */
static void
sync_wins (rp_screen *s)
{
  rp_window *win, *wintmp;
  XWindowAttributes attr;
  unsigned int i, nwins;
  Window dw1, dw2, *wins;
  XQueryTree(dpy, s->root, &dw1, &dw2, &wins, &nwins);

  /* Remove any windows in our cached lists that aren't in the query
     tree. These windows have been destroyed. */
  list_for_each_entry_safe (win, wintmp, &rp_mapped_window, node)
    {
      int found;

      found = 0;
      for (i=0; i<nwins; i++)
	{
	  if (win->w == wins[i])
	    {
	      found = 1;
	      break;
	    }
	}
      if (!found)
	{
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
		  if (frame->number == win->scr->current_frame) 
		    set_active_frame (frame);
		}
	      withdraw_window (win);
	    }

	  /* Now that the window is guaranteed to be in the unmapped window
	     list, we can safely stop managing it. */
	  unmanage (win);
	  ignore_badwindow--;
	}
    }


  for (i=0; i<nwins; i++)
    {
      XGetWindowAttributes(dpy, wins[i], &attr);
      if (wins[i] == s->bar_window 
	  || wins[i] == s->key_window 
	  || wins[i] == s->input_window
	  || wins[i] == s->frame_window
	  || wins[i] == s->help_window
	  || attr.override_redirect == True) continue;
      
      /* Find the window in our mapped window list. */
      win = find_window_in_list (wins[i], &rp_mapped_window);
      if (win)
	{
	  rp_frame *frame;
	  /* If the window is viewable and it is in a frame, then
	     maximize it and go to the next window. */
	  if (attr.map_state == IsViewable)
	    {
	      frame = find_windows_frame (win);
	      if (frame)
		{
		  maximize (win);
		}
	      else
		{
		  hide_window (win);
		}
	    }
	  else if (attr.map_state == IsUnmapped
		   && get_state (win) == IconicState)
	    {
	      frame = find_windows_frame (win);
	      if (frame)
		{
		  unhide_window (win);
		  maximize (win);
		}
	    }
	  else
	    {
	      PRINT_DEBUG (("I don't know what to do...\n"));
	    }
	  
	  /* We've handled the window. */
	  continue;
	}

      /* Try the unmapped window list. */
      win = find_window_in_list (wins[i], &rp_unmapped_window);
      if (win)
	{
	  /* If the window is viewable and it is in a frame, then
	     maximize it and go to the next window. */
	  if (attr.map_state == IsViewable)
	    {
	      /* We need to map it since it's visible now. */
	      map_window (win);
	    }
	  else if (attr.map_state == IsUnmapped
		   && get_state (win) == IconicState)
	    {
	      /* We need to map the window and then hide it. */
	      map_window (win);
	      hide_window (win);
	    }
	  else
	    {
	      PRINT_DEBUG (("I think it's all sync'd up...\n"));
	    }
	  
	  /* We've handled the window. */
	  continue;
	}

      /* The window isn't in the mapped or unmapped window list so add
	 it. */
      win = add_to_window_list (s, wins[i]);
      
      /* If it's visible or iconized. "Map" it. */
      if (attr.map_state == IsViewable
	  || (attr.map_state == IsUnmapped
	      && get_state (win) == IconicState))
	map_window (win);
    }

}

/* Temporarily give control over to another window manager, reclaiming
   control when that WM terminates. */
char *
cmd_tmpwm (int interactive, char *data)
{
  struct list_head *tmp, *iter;
  rp_window *win = NULL;
  int child;
  int status;
  int pid;
  int i;

  if (data == NULL)
    {
      message (" tmpwm: one argument required ");
      return NULL;
    }

  /* Release event selection on the root windows, so the new WM can
     have it. */
  for (i=0; i<num_screens; i++)
    {
      XSelectInput(dpy, RootWindow (dpy, screens[i].screen_num), 0);
      /* Unmap its key window */
      XUnmapWindow (dpy, screens[i].key_window);
    }

  /* Don't listen for any events from any window. */
  list_for_each_safe_entry (win, iter, tmp, &rp_mapped_window, node)
    {
      unhide_window (win);
      maximize (win);
      XSelectInput (dpy, win->w, 0);
    }

  list_for_each_safe_entry (win, iter, tmp, &rp_unmapped_window, node)
    XSelectInput (dpy, win->w, 0);

  XSync (dpy, False);

  /* Launch the new WM and wait for it to terminate. */
  pid = spawn (data);
  do
    {
      child = waitpid (pid, &status, 0);
    } while (child != pid);

  /* This xsync seems to be needed. Otherwise, the following code dies
     because X thinks another WM is running. */
  XSync (dpy, False);

  /* Enable the event selection on the root window. */
  for (i=0; i<num_screens; i++)
    {
      XSelectInput(dpy, RootWindow (dpy, screens[i].screen_num),
		   PropertyChangeMask | ColormapChangeMask
		   | SubstructureRedirectMask | SubstructureNotifyMask);
      /* Map its key window */
      XMapWindow (dpy, screens[i].key_window);
    }
  XSync (dpy, False);

  /* Sort through all the windows in each group and pick out the ones
     that are unmapped or destroyed. */
  for (i=0; i<num_screens; i++)
    {
      sync_wins (&screens[i]);
    }

  /* If no window has focus, give the key_window focus. */
  if (current_window() == NULL)
    set_window_focus (current_screen()->key_window);

  /* And we're back in ratpoison. */
  return NULL;
}

/* Return a new string with the frame selector or it as a string if no
   selector exists for the number. */
static char *
frame_selector (int n)
{
  if (n < strlen (defaults.frame_selectors))
    {
      return xsprintf (" %c ", defaults.frame_selectors[n]);
    }
  else
    {
      return xsprintf (" %d ", n);
    }
}

/* Return true if ch is nth frame selector. */
static int
frame_selector_match (char ch)
{
  int i;

  /* Is it in the frame selector string? */
  for (i=0; i<strlen (defaults.frame_selectors); i++)
    {
      if (ch == defaults.frame_selectors[i])
	return i;
    }

  /* Maybe it's a number less than 9 and the frame selector doesn't
     define that many selectors. */
  if (ch >= '0' && ch <= '9'
      && ch - '0' >= strlen (defaults.frame_selectors))
    {
      return ch - '0';
    }

  return -1;
}

/* Select a frame by number. */
char *
cmd_fselect (int interactive, char *data)
{
  rp_frame *frame;
  int fnum = -1;

  /* If the command was specified on the command line or an argument
     was supplied to it, then try to read that argument. */
  if (!interactive || data != NULL)
    {
      if (data == NULL)
	{
	  /* The command was from the command line, but they didn't
	     specify an argument. FIXME: give some indication of
	     failure. */
	  return NULL;
	}

      /* Attempt to read the argument. */
      if (sscanf (data, "%d", &fnum) < 1)
	{
	  message (" fselect: numerical argument required ");
	  return NULL;
	}
    }
  /* The command was called interactively and no argument was
     supplied, so we show the frames' numbers and read a number from
     the keyboard. */
  else
    {
      KeySym c;
      char keysym_buf[513];
      int keysym_bufsize = sizeof (keysym_buf);
      unsigned int mod;
      Window *wins;
      int i, j;
      rp_frame *cur;
      int frames;

      frames = 0;
      for (j=0; j<num_screens; j++)
	frames += num_frames(&screens[j]);

      wins = xmalloc (sizeof (Window) * frames);

      /* Loop through each frame and display its number in it's top
	 left corner. */
      i = 0;
      for (j=0; j<num_screens; j++)
	{
	  XSetWindowAttributes attr;
	  rp_screen *s = &screens[j];
      
	  /* Set up the window attributes to be used in the loop. */
	  attr.border_pixel = s->fg_color;
	  attr.background_pixel = s->bg_color;
	  attr.override_redirect = True;

	  list_for_each_entry (cur, &s->frames, node)
	    {
	      int width, height;
	      char *num;

	      /* Create the string to be displayed in the window and
		 determine the height and width of the window. */
/* 	      num = xsprintf (" %d ", cur->number); */
	      num = frame_selector (cur->number);
	      width = defaults.bar_x_padding * 2 + XTextWidth (defaults.font, num, strlen (num));
	      height = (FONT_HEIGHT (defaults.font) + defaults.bar_y_padding * 2);

	      /* Create and map the window. */
	      wins[i] = XCreateWindow (dpy, s->root, s->left + cur->x, s->top + cur->y, width, height, 1, 
				       CopyFromParent, CopyFromParent, CopyFromParent,
				       CWOverrideRedirect | CWBorderPixel | CWBackPixel,
				       &attr);
	      XMapWindow (dpy, wins[i]);
	      XClearWindow (dpy, wins[i]);

	      /* Display the frame's number inside the window. */
	      XDrawString (dpy, wins[i], s->normal_gc, 
			   defaults.bar_x_padding, 
			   defaults.bar_y_padding + defaults.font->max_bounds.ascent,
			   num, strlen (num));

	      free (num);
	      i++;
	    }
	}
      XSync (dpy, False);

      /* Read a key. */
      XGrabKeyboard (dpy, current_screen()->key_window, False, GrabModeSync, GrabModeAsync, CurrentTime);
      read_key (&c, &mod, keysym_buf, keysym_bufsize);
      XUngrabKeyboard (dpy, CurrentTime);

      /* Destroy our number windows and free the array. */
      for (i=0; i<frames; i++)
	XDestroyWindow (dpy, wins[i]);

      free (wins);

      /* FIXME: We only handle one character long keysym names. */
      if (strlen (keysym_buf) == 1)
	{
	  fnum = frame_selector_match (keysym_buf[0]);
	  if (fnum == -1)
	    return NULL;
	}
      else
	{
	  return NULL;
	}
    }

  /* Now that we have a frame number to go to, let's try to jump to
     it. */
  frame = find_frame_number (fnum);
  if (frame)
    set_active_frame (frame);
  else
    marked_message_printf (0, 0, " fselect: No such frame (%d) ", fnum);

  return NULL;
}

char *
cmd_fdump (int interactively, char *data)
{
  struct sbuf *s;
  char *tmp;
  rp_frame *cur;

  s = sbuf_new (0);

  /* FIXME: Oooh, gross! there's a trailing comma, yuk! */
  list_for_each_entry (cur, &current_screen()->frames, node)
    {
      char *tmp;

      tmp = frame_dump (cur);
      sbuf_concat (s, tmp);
      sbuf_concat (s, ",");
      free (tmp);
    }

  tmp = sbuf_get (s);
  free (s);
  return tmp;
}

char *
cmd_frestore (int interactively, char *data)
{
  rp_screen *s = current_screen();
  char *token;
  char *dup;
  rp_frame *new, *cur;
  rp_window *win;
  struct list_head fset;
  int max = -1;

  if (data == NULL)
    {
      message (" frestore: one argument required ");
      return NULL;
    }

  INIT_LIST_HEAD (&fset);

  dup = xstrdup (data);
  token = strtok (dup, ",");
  if (token == NULL)
    {
      message (" frestore: invalid frame format ");
      free (dup);
      return NULL;
    }

  /* Build the new frame set. */
  while (token != NULL)
    {
      new = frame_read (token);
      if (new == NULL)
	{
	  message (" frestore: invalid frame format ");
	  free (dup);
	  return NULL;
	}
      list_add_tail (&new->node, &fset);
      token = strtok (NULL, ",");
    } 

  free (dup);

  /* Clear all the frames. */
  list_for_each_entry (cur, &s->frames, node)
    {
      PRINT_DEBUG (("blank %d\n", cur->number));
      blank_frame (cur);
    }

  /* Get rid of the frames' numbers */
  screen_free_nums (s);

  /* Splice in our new frameset. */
  screen_restore_frameset (s, &fset);
/*   numset_clear (s->frames_numset); */

  /* Process the frames a bit to make sure everything lines up. */
  list_for_each_entry (cur, &s->frames, node)
    {
      rp_window *win;

      PRINT_DEBUG (("restore %d %d\n", cur->number, cur->win_number));

      /* Find the current frame based on last_access. */
      if (cur->last_access > max)
	{
	  s->current_frame = cur->number;
	  max = cur->last_access;
	}

      /* Grab the frame's number, but if it already exists request a
	 new one. */
      if (!numset_add_num (s->frames_numset, cur->number)) {
	cur->number = numset_request (s->frames_numset);
      }

      /* Update the window the frame points to. */
      if (cur->win_number != EMPTY)
	{
	  win = find_window_number (cur->win_number);
	  set_frames_window (cur, win);
	}
    }

  /* Show the windows in the frames. */
  list_for_each_entry (win, &rp_mapped_window, node)
    {
      if (win->frame_number != EMPTY)
	{
	  maximize (win);
	  unhide_window (win);
	}
    }

  set_active_frame (current_frame());  
  update_bar (s);
  show_frame_indicator();

  PRINT_DEBUG (("Done.\n"));
  return NULL;
}

char *
cmd_verbexec (int interactive, char *data)
{
  char msg[100]="Running ";
  strncat(msg, data, 100-strlen(msg));
  
  if(data) cmd_echo(interactive, msg);
  return cmd_exec(interactive, data);
}

char *
cmd_defwinliststyle (int interactive, char *data)
{
  if (data == NULL && !interactive)
    return xsprintf ("%s", defaults.window_list_style ? "column":"row");

  if (data == NULL)
    {
      message (" defwinliststyle: one argument required ");
      return NULL;
     }

  if (!strcmp ("column", data))
    {
      defaults.window_list_style = STYLE_COLUMN;
    }
  else if (!strcmp ("row", data))
    {
      defaults.window_list_style = STYLE_ROW;
    }
  else
    {
      message (" defwinliststyle: invalid argument ");
    }

   return NULL;    
}

char *
cmd_gnext (int interactive, char *data)
{
  set_current_group (group_next_group ());
  return NULL;
}

char *
cmd_gprev (int interactive, char *data)
{
  set_current_group (group_prev_group ());
  return NULL;
}

char *
cmd_gnew (int interactive, char *data)
{
  set_current_group (group_add_new_group (data));
  return NULL;
}

char *
cmd_gnewbg (int interactive, char *data)
{
  group_add_new_group (data);
  return NULL;
}

/* Given a string, find a matching group. First check if the string is
   a number, then check if it's the name of a group. */
static rp_group *
find_group (char *str)
{
  rp_group *group;
  int n;

  /* Check if the user typed a group number. */
  n = string_to_window_number (str);
  if (n >= 0)
    {
      group = groups_find_group_by_number (n);
      if (group)
	return group;
    }

  group = groups_find_group_by_name (str);
  return group;
}

struct list_head *
group_completions (char *str)
{
  struct list_head *list;
  rp_group *cur;

  /* Initialize our list. */
  list = xmalloc (sizeof (struct list_head));
  INIT_LIST_HEAD (list);

  /* Grab all the group names. */
  list_for_each_entry (cur, &rp_groups, node)
    {
      struct sbuf *s;

      s = sbuf_new (0);
      /* A group may not have a name, so if it doesn't, use it's
	 number. */
      if (cur->name)
	{
	  sbuf_copy (s, cur->name);
	}
      else
	{
	  sbuf_printf (s, "%d", cur->number);
	}

      list_add_tail (&s->node, list);
    }

  return list;
}

char *
cmd_gselect (int interactive, char *data)
{
  char *str;
  rp_group *g;

  if (data == NULL)
    str = get_input (MESSAGE_PROMPT_SWITCH_TO_GROUP, group_completions);
  else
    str = xstrdup (data);

  /* User aborted. */
  if (str == NULL)
    return NULL;

  g = find_group (str);

  if (g)
    set_current_group (g);
  else
    return cmd_groups (interactive, NULL);

  free (str);

  return NULL;
}

/* Show all the groups, with the current one highlighted. */
char *
cmd_groups (int interactive, char *data)
{
  rp_group *cur;
  int mark_start = 0, mark_end = 0;
  struct sbuf *buffer;

  buffer = sbuf_new (0);

  /* Generate the string. */
  list_for_each_entry (cur, &rp_groups, node)
    {
      char *fmt;
      char separator;

      if (cur == rp_current_group)
	mark_start = strlen (sbuf_get (buffer));

      /* Pad start of group name with a space for row
	 style. non-Interactive always gets a column.*/
      if (defaults.window_list_style == STYLE_ROW && interactive)
	  sbuf_concat (buffer, " ");	

      if(cur == rp_current_group)
	separator = '*';
      else
	separator = '-';

      fmt =  xsprintf ("%d%c%s", cur->number, separator, cur->name);
      sbuf_concat (buffer, fmt);

      /* Pad end of group name with a space for row style. */
      if (defaults.window_list_style == STYLE_ROW && interactive)
	{
	  sbuf_concat (buffer, " ");
	}
      else
	{
	  if (cur->node.next != &rp_groups)
	    sbuf_concat (buffer, "\n");
	}

      if (cur == rp_current_group)
	mark_end = strlen (sbuf_get (buffer));
    }

  /* Display it or return it. */
  if (interactive)
    {
      marked_message (sbuf_get (buffer), mark_start, mark_end);
      sbuf_free (buffer);
      return NULL;
    }
  else
    {
      char* tmp = sbuf_get(buffer);
      free(buffer);
      return tmp;
    }
}

/* Move a window to a different group. */
char *
cmd_gmove (int interactive, char *data)
{
  char *str;
  rp_group *g;

  if (current_window() == NULL)
    {
      message (" gmove: no focused window ");
      return NULL;
    }

  /* Prompt for a group */
  if (data == NULL)
    str = get_input (MESSAGE_PROMPT_SWITCH_TO_GROUP, group_completions);
  else
    str = xstrdup (data);

  /* User aborted. */
  if (str == NULL)
    return NULL;

  g = find_group (str);
  if (g == NULL)
    {
      message (" gmove: cannot find group ");
      return NULL;
    }

  group_move_window (g, current_window());
  return NULL;
}

char *
cmd_gmerge (int interactive, char *data)
{
  rp_group *g;

  if (data == NULL)
    {
      message (" gmerge: one argument required ");
      return NULL;
    }

  g = find_group (data);

  if (g)
    groups_merge (g, rp_current_group);
  else
    message (" gmerge: cannot find group ");

  return NULL;
}

char *
cmd_addhook (int interactive, char *data)
{
  char *dup;
  char *token;
  struct list_head *hook;
  struct sbuf *cmd;

  if (data == NULL)
    {
      message (" addhook: two arguments required ");
      return NULL;
    }

  dup = xstrdup (data);
  token = strtok (dup, " ");
  
  hook = hook_lookup (token);
  if (hook == NULL)
    {
      marked_message_printf (0, 0, " addhook: unknown hook \"%s\"", token);
      free (dup);
      return NULL;
    }

  token = strtok (NULL, "\0");
  
  if (token == NULL)
    {
      message (" addhook: two arguments required ");
      free (dup);
      return NULL;
    }

  /* Add the command to the hook  */
  cmd = sbuf_new (0);
  sbuf_copy (cmd, token);
  hook_add (hook, cmd);

  free (dup);
  return NULL;
}

char *
cmd_remhook (int interactive, char *data)
{
  char *dup;
  char *token;
  struct list_head *hook;
  struct sbuf *cmd;

  if (data == NULL)
    {
      message (" remhook: two arguments required ");
      return NULL;
    }

  dup = xstrdup (data);
  token = strtok (dup, " ");
  
  hook = hook_lookup (token);
  if (hook == NULL)
    {
      marked_message_printf (0, 0, " remhook: unknown hook \"%s\"", token);
      free (dup);
      return NULL;
    }

  token = strtok (NULL, "\0");
  
  if (token == NULL)
    {
      message (" remhook: two arguments required ");
      free (dup);
      return NULL;
    }

  /* Add the command to the hook  */
  cmd = sbuf_new (0);
  sbuf_copy (cmd, token);
  hook_remove (hook, cmd);

  free (dup);
  return NULL;
}

struct list_head *
hook_completions (char* str)
{
  struct list_head *list;
  struct rp_hook_db_entry *entry;

  /* Initialize our list. */
  list = xmalloc (sizeof (struct list_head));
  INIT_LIST_HEAD (list);
 
  for (entry = rp_hook_db; entry->name; entry++)
    {
      struct sbuf *hookname;

      hookname = sbuf_new(0);
      sbuf_copy (hookname, entry->name);
      list_add_tail (&hookname->node, list);
    }
  
  return list;
}

char *
cmd_listhook (int interactive, char *data)
{
  struct sbuf *buffer;
  struct list_head *hook;
  struct sbuf *cur;
  char *str;
  
  if (data == NULL)
    str = get_input (" Hook: ", hook_completions);
  else
    str = xstrdup (data);

  /* User aborted. */
  if (str == NULL)
    return NULL;

  hook = hook_lookup (str);
  if (hook == NULL)
    {
      marked_message_printf (0, 0, " listhook: unknown hook \"%s\" ", str);
      return NULL;
    }

  buffer = sbuf_new(0);

  if (list_empty(hook))
    {
      sbuf_printf(buffer, " Nothing defined for %s ", str);
    }
  else
    {
      list_for_each_entry (cur, hook, node)
	{
	  sbuf_printf_concat(buffer, "%s", sbuf_get (cur));
	  if (cur->node.next != hook)
	    sbuf_printf_concat(buffer, "\n");
	}
    }

  /* Display it or return it. */
  if (interactive)
    {
      marked_message (sbuf_get (buffer), 0, 0);
      sbuf_free (buffer);
      return NULL;
    }
  else
    {
      char* tmp = sbuf_get(buffer);
      free(buffer);
      return tmp;
    }
}

char *
cmd_gdelete (int interactive, char *data)
{
  rp_group *g; 

  if (data == NULL) 
    g = rp_current_group;
  else
    {
      g = find_group (data);
      if (!g)
	{
	  message (" gdelete: cannot find group ");
	  return NULL;
	}
    }
  
  switch (group_delete_group (g))
    {
    case GROUP_DELETE_GROUP_OK:
      break;
    case GROUP_DELETE_GROUP_NONEMPTY:
      message (" gdelete: non-empty group ");
      break;
    default:
      message (" gdelete: Unknown return code (this shouldn't happen) ");
    }

  return NULL;
}

static void
grab_rat ()
{
  XGrabPointer (dpy, current_screen()->root, True, 0, 
		GrabModeAsync, GrabModeAsync, 
		None, current_screen()->rat, CurrentTime);
}

static void
ungrab_rat ()
{
  XUngrabPointer (dpy, CurrentTime);
}

char *
cmd_readkey (int interactive, char *data)
{
  char *keysym_name;
  rp_action *key_action;
  KeySym keysym;		/* Key pressed */
  unsigned int mod;		/* Modifiers */
  int rat_grabbed = 0;
  rp_keymap *map;

  if (data == NULL)
    {
      message (" readkey: keymap expected ");
      return NULL;
    }

  map = find_keymap (data);
  if (map == NULL)
    {
      marked_message_printf (0, 0, " readkey: Unknown keymap '%s' ", data);
      return NULL;
    }

  XGrabKeyboard (dpy, current_screen()->key_window, False, GrabModeSync, GrabModeAsync, CurrentTime);

  /* Change the mouse icon to indicate to the user we are waiting for
     more keystrokes */
  if (defaults.wait_for_key_cursor)
    {
      grab_rat();
      rat_grabbed = 1;
    }

  read_key (&keysym, &mod, NULL, 0);
  XUngrabKeyboard (dpy, CurrentTime);

  if (rat_grabbed)
    ungrab_rat();

  if ((key_action = find_keybinding (keysym, x11_mask_to_rp_mask (mod), map)))
    {
      char *result;
      result = command (1, key_action->data);
      
      /* Gobble the result. */
      if (result)
	free (result);
    }
  else
    {
      /* No key match, notify user. */
      keysym_name = keysym_to_string (keysym, x11_mask_to_rp_mask (mod));
      marked_message_printf (0, 0, " %s unbound key ", keysym_name);
      free (keysym_name);
    }

  return NULL;
}

char *
cmd_newkmap (int interactive, char *data)
{
  rp_keymap *map;

  if (data == NULL)
    {
      message (" newkmap: one argument required ");
      return NULL;
    }

  map = find_keymap (data);
  if (map)
    {
      marked_message_printf (0, 0, " newkmap: '%s' already exists ", data);
      return NULL;
    }

  map = keymap_new (data);
  list_add_tail (&map->node, &rp_keymaps);

  return NULL;
}

char *
cmd_delkmap (int interactive, char *data)
{
   rp_keymap *map;

  if (data == NULL)
    {
      message (" delkmap: one argument required ");
      return NULL;
    }

  if (!strcmp (data, ROOT_KEYMAP) || !strcmp (data, TOP_KEYMAP))
    {
      marked_message_printf (0, 0, " delkmap: Cannot delete '%s' keymap ", data);
      return NULL;
    }

  map = find_keymap (data);
  if (map == NULL)
    {
      marked_message_printf (0, 0,  " delkmap: Unknown keymap '%s' ", data);
      return NULL;
    }

  list_del (&map->node);
  keymap_free (map);

  return NULL;
}

char *
cmd_defframesels (int interactive, char *data)
{
  if (data == NULL)
    {
      marked_message_printf (0, 0, " %s ", defaults.frame_selectors);
      return NULL;
    }

  free (defaults.frame_selectors);
  defaults.frame_selectors = xstrdup (data);
  return NULL;
}
