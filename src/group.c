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

#include "ratpoison.h"

#include <string.h>

static struct numset *group_numset;

static void
set_current_group_1 (rp_group *g)
{
  static int counter = 1;
  rp_current_group = g;
  if (g)
    g->last_access = counter++;
}

void
init_groups(void)
{
  rp_group *g;

  group_numset = numset_new();
  INIT_LIST_HEAD (&rp_groups);

  /* Create the first group in the list (We always need at least
     one). */
  g = group_new (numset_request (group_numset), DEFAULT_GROUP_NAME);
  set_current_group_1 (g);
  list_add_tail (&g->node, &rp_groups);
}

void
free_groups(void)
{
  rp_group *cur;
  struct list_head *iter, *tmp;

  list_for_each_safe_entry (cur, iter, tmp, &rp_groups, node)
    {
      group_free (cur);
    }
}

rp_group *
group_new (int number, char *name)
{
  rp_group *g;

  g = xmalloc (sizeof (rp_group));

  if (name)
    g->name = xstrdup (name);
  else
    g->name = NULL;
  g->last_access = 0;
  g->number = number;
  g->numset = numset_new();
  INIT_LIST_HEAD (&g->unmapped_windows);
  INIT_LIST_HEAD (&g->mapped_windows);

  return g;
}

void
group_free (rp_group *g)
{
  if (g->name)
    free (g->name);
  numset_free (g->numset);
  numset_release (group_numset, g->number);
  free (g);
}

rp_group *
group_add_new_group (char *name)
{
  rp_group *g;

  g = group_new (numset_request (group_numset), name);
  list_add_tail (&g->node, &rp_groups);

  return g;
}

rp_group *
group_next_group (void)
{
  return list_next_entry (rp_current_group, &rp_groups, node);
}

rp_group *
group_prev_group (void)
{
  return list_prev_entry (rp_current_group, &rp_groups, node);
}

rp_group *
group_last_group (void)
{
  int last_access = 0;
  rp_group *most_recent = NULL;
  rp_group *cur;

  list_for_each_entry (cur, &rp_groups, node)
    {
      if (cur != rp_current_group && cur->last_access > last_access) {
        most_recent = cur;
	last_access = cur->last_access;
      }
    }
  return most_recent;
}

rp_group *
groups_find_group_by_name (char *s)
{
  rp_group *cur;

  list_for_each_entry (cur, &rp_groups, node)
    {
      if (cur->name)
        {
          if (str_comp (s, cur->name, strlen (s)))
            return cur;
        }
    }

  return NULL;
}

rp_group *
groups_find_group_by_number (int n)
{
  rp_group *cur;

  list_for_each_entry (cur, &rp_groups, node)
    {
      if (cur->number == n)
        return cur;
    }

  return NULL;
}

/* Return the first group that contains the window. */
rp_group *
groups_find_group_by_window (rp_window *win)
{
  rp_group *cur;
  rp_window_elem *elem;

  list_for_each_entry (cur, &rp_groups, node)
    {
      elem = group_find_window (&cur->mapped_windows, win);
      if (elem)
        return cur;
    }

  return NULL;
}


/* Return the first group that is g. */
rp_group *
groups_find_group_by_group (rp_group *g)
{
  rp_group *cur;

  list_for_each_entry (cur, &rp_groups, node)
    {
      if (cur == g)
        return cur;
    }

  return NULL;
}

rp_window_elem *
group_find_window (struct list_head *list, rp_window *win)
{
  rp_window_elem *cur;

  list_for_each_entry (cur, list, node)
    {
      if (cur->win == win)
        return cur;
    }

  return NULL;
}

rp_window_elem *
group_find_window_by_number (rp_group *g, int num)
{
  rp_window_elem *cur;

  list_for_each_entry (cur, &g->mapped_windows, node)
    {
      if (cur->number == num)
        return cur;
    }

  return NULL;

}


/* Insert a window_elem into the correct spot in the group's window
   list to preserve window number ordering. */
static void
group_insert_window (struct list_head *h, rp_window_elem *w)
{
  rp_window_elem *cur;

  list_for_each_entry (cur, h, node)
    {
      if (cur->number > w->number)
        {
          list_add_tail (&w->node, &cur->node);
          return;
        }
    }

  list_add_tail(&w->node, h);
}

static int
group_in_list (struct list_head *h, rp_window_elem *w)
{
  rp_window_elem *cur;

  list_for_each_entry (cur, h, node)
    {
      if (cur == w)
        return 1;
    }

  return 0;
}

/* If a window_elem's number has changed then the list has to be
   resorted. */
void
group_resort_window (rp_group *g, rp_window_elem *w)
{
  /* Only a mapped window can be resorted. */
  if (!group_in_list (&g->mapped_windows, w))
    {
      PRINT_DEBUG (("Attempting to restort an unmapped window!\n"));
      return;
    }

  list_del (&w->node);
  group_insert_window (&g->mapped_windows, w);
}

void
group_add_window (rp_group *g, rp_window *w)
{
  rp_window_elem *we;

  /* Create our container structure for the window. */
  we = malloc (sizeof (rp_window_elem));
  we->win = w;
  we->number = -1;

  /* Finally, add it to our list. */
  list_add_tail (&we->node, &g->unmapped_windows);
}

void
group_map_window (rp_group *g, rp_window *win)
{
  rp_window_elem *we;

  we = group_find_window (&g->unmapped_windows, win);

  if (we)
    {
      we->number = numset_request (g->numset);
      list_del (&we->node);
      group_insert_window (&g->mapped_windows, we);
    }
}

void
groups_map_window (rp_window *win)
{
  rp_group *cur;

  list_for_each_entry (cur, &rp_groups, node)
    {
      group_map_window (cur, win);
    }
}

void
group_unmap_window (rp_group *g, rp_window *win)
{
  rp_window_elem *we;

  we = group_find_window (&g->mapped_windows, win);

  if (we)
    {
      numset_release (g->numset, we->number);
      list_move_tail (&we->node, &g->unmapped_windows);
    }
}

void
groups_unmap_window (rp_window *win)
{
  rp_group *cur;

  list_for_each_entry (cur, &rp_groups, node)
    {
      group_unmap_window (cur, win);
    }
}

void
group_del_window (rp_group *g, rp_window *win)
{
  rp_window_elem *cur;
  struct list_head *iter, *tmp;

  /* The assumption is that a window is unmapped before it's deleted. */
  list_for_each_safe_entry (cur, iter, tmp, &g->unmapped_windows, node)
    {
      if (cur->win == win)
        {
          list_del (&cur->node);
          free (cur);
        }
    }

  /* Make sure the window isn't in the list of mapped windows. This
     would mean there is a bug. */
#ifdef DEBUG
  list_for_each_entry (cur, &g->mapped_windows, node)
    {
      if (cur->win == win)
        PRINT_DEBUG (("This window wasn't removed from the mapped window list.\n"));
    }
#endif
}

/* Remove the window from any groups in resides in. */
void
groups_del_window (rp_window *win)
{
  rp_group *cur;

  list_for_each_entry (cur, &rp_groups, node)
    {
      group_del_window (cur, win);
    }
}

rp_window *
group_last_window (rp_group *g, rp_screen *s)
{
  int last_access = 0;
  rp_window_elem *most_recent = NULL;
  rp_window_elem *cur;

  list_for_each_entry (cur, &g->mapped_windows, node)
    {
      if (cur->win->last_access >= last_access
          && cur->win != current_window()
          && !find_windows_frame (cur->win)
          && (cur->win->scr == s || rp_have_xinerama))
        {
          most_recent = cur;
          last_access = cur->win->last_access;
        }
    }

  if (most_recent)
    return most_recent->win;

  return NULL;
}

rp_window *
group_next_window (rp_group *g, rp_window *win)
{
  rp_window_elem *cur, *we;

  /* If there is no window, then get the last accessed one. */
  if (win == NULL)
    return group_last_window (g, current_screen());

  /* If we can't find the window, then it's in a different group, so
     get the last accessed one in this group. */
  we = group_find_window (&g->mapped_windows, win);
  if (we == NULL)
    return group_last_window (g, win->scr);

  /* The window is in this group, so find the next one in the list
     that isn't already displayed. */
  for (cur = list_next_entry (we, &g->mapped_windows, node);
       cur != we;
       cur = list_next_entry (cur, &g->mapped_windows, node))
    {
      if (!find_windows_frame (cur->win) && (cur->win->scr == win->scr || rp_have_xinerama))
        {
          return cur->win;
        }
    }

  return NULL;
}

rp_window *
group_prev_window (rp_group *g, rp_window *win)
{
  rp_window_elem *cur, *we;

  /* If there is no window, then get the last accessed one. */
  if (win == NULL)
    return group_last_window (g, current_screen());

  /* If we can't find the window, then it's in a different group, so
     get the last accessed one in this group. */
  we = group_find_window (&g->mapped_windows, win);
  if (we == NULL)
    return group_last_window (g, win->scr);

  /* The window is in this group, so find the previous one in the list
     that isn't already displayed. */
  for (cur = list_prev_entry (we, &g->mapped_windows, node);
       cur != we;
       cur = list_prev_entry (cur, &g->mapped_windows, node))
    {
      if (!find_windows_frame (cur->win) && (cur->win->scr == win->scr || rp_have_xinerama))
        {
          return cur->win;
        }
    }

  return NULL;

}

void
group_move_window (rp_group *to, rp_window *win)
{
  rp_group *cur, *from = NULL;
  rp_window_elem *we = NULL;

  /* Find the group that the window belongs to. FIXME: If the window
     exists in multiple groups, then we're going to find the first
     group with this window in it. */
  list_for_each_entry (cur, &rp_groups, node)
    {
      we = group_find_window (&cur->mapped_windows, win);
      if (we)
        {
          from = cur;
          break;
        }
    }

  if (we == NULL || from == NULL)
    {
      PRINT_DEBUG (("Unable to find window in mapped window lists.\n"));
      return;
    }

  /* Manually remove the window from one group...*/
  numset_release (from->numset, we->number);
  list_del (&we->node);

  /* and shove it into the other one. */
  we->number = numset_request (to->numset);
  group_insert_window (&to->mapped_windows, we);
}

void
groups_merge (rp_group *from, rp_group *to)
{
  rp_window_elem *cur;
  struct list_head *iter, *tmp;

  /* Merging a group with itself makes no sense. */
  if (from == to)
    return;

  /* Move the unmapped windows. */
  list_for_each_safe_entry (cur, iter, tmp, &from->unmapped_windows, node)
    {
      list_del (&cur->node);
      list_add_tail (&cur->node, &to->unmapped_windows);
    }

  /* Move the mapped windows. */
  list_for_each_safe_entry (cur, iter, tmp, &from->mapped_windows, node)
    {
      numset_release (from->numset, cur->number);
      list_del (&cur->node);

      cur->number = numset_request (to->numset);
      group_insert_window (&to->mapped_windows, cur);
    }
}

void
set_current_group (rp_group *g)
{
  if (rp_current_group == g || g == NULL)
    return;

  set_current_group_1 (g);

  /* Call the switch group hook. */
  hook_run (&rp_switch_group_hook);
}

int
group_delete_group (rp_group *g)
{
  if (list_empty (&(g->mapped_windows))
      && list_empty (&(g->unmapped_windows)))
    {
      /* we can safely delete the group */
      if (g == rp_current_group)
        {
          set_current_group_1 (group_next_group ());
        }

      list_del (&(g->node));
      group_free (g);

      if (list_empty (&rp_groups))
        {
          /* Create the first group in the list (We always need at least
             one). */
          g = group_new (numset_request (group_numset), DEFAULT_GROUP_NAME);
          set_current_group_1 (g);
          list_add_tail (&g->node, &rp_groups);
        }
      return GROUP_DELETE_GROUP_OK;
    }
  else
    {
      return GROUP_DELETE_GROUP_NONEMPTY;
    }
}

/* Used by :cother / :iother  */
rp_window *
group_last_window_by_class (rp_group *g, char *class)
{
  int last_access = 0;
  rp_window_elem *most_recent = NULL;
  rp_window_elem *cur;
  rp_screen *s = current_screen();

  list_for_each_entry (cur, &g->mapped_windows, node)
    {
      if (cur->win->last_access >= last_access
          && cur->win != current_window()
          && !find_windows_frame (cur->win)
          && (cur->win->scr == s || rp_have_xinerama)
          && strcmp(class, cur->win->res_class))
        {
          most_recent = cur;
          last_access = cur->win->last_access;
        }
    }

  if (most_recent)
    return most_recent->win;

  return NULL;
}

/* Used by :cother / :iother  */
rp_window *
group_last_window_by_class_complement (rp_group *g, char *class)
{
  int last_access = 0;
  rp_window_elem *most_recent = NULL;
  rp_window_elem *cur;
  rp_screen *s = current_screen();

  list_for_each_entry (cur, &g->mapped_windows, node)
    {
      if (cur->win->last_access >= last_access
          && cur->win != current_window()
          && !find_windows_frame (cur->win)
          && (cur->win->scr == s || rp_have_xinerama)
          && !strcmp(class, cur->win->res_class))
        {
          most_recent = cur;
          last_access = cur->win->last_access;
        }
    }

  if (most_recent)
    return most_recent->win;

  return NULL;
}
