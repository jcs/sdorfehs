#include "ratpoison.h"

static struct numset *group_numset;

void init_groups()
{
  rp_group *g;

  group_numset = numset_new();
  INIT_LIST_HEAD (&rp_groups);

  /* Create the first group in the list (We always need at least
     one). */
  g = group_new (numset_request (group_numset));
  rp_current_group = g;
  list_add_tail (&g->node, &rp_groups);
}

rp_group *
group_new (int number)
{
  rp_group *g;

  g = xmalloc (sizeof (rp_group));

  g->number = numset_request (group_numset);
  g->numset = numset_new();
  INIT_LIST_HEAD (&g->unmapped_windows);
  INIT_LIST_HEAD (&g->mapped_windows);
  
  return g;
}

void
group_free (rp_group *g)
{
  /* free (g->name); */
  numset_free (g->numset);
  numset_release (group_numset, g->number);
  free (g);
}

rp_group *
group_add_new_group ()
{
  rp_group *g;

  g = group_new (numset_request (group_numset));
  list_add_tail (&g->node, &rp_groups);

  return g;
}

rp_group *
group_next_group ()
{
  return list_next_entry (rp_current_group, &rp_groups, node);
}

rp_group *
group_prev_group ()
{
  return list_prev_entry (rp_current_group, &rp_groups, node);
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

rp_window *
group_find_window_by_number (rp_group *g, int num)
{
  rp_window_elem *cur;

  list_for_each_entry (cur, &g->mapped_windows, node)
    {
      if (cur->number == num)
	return cur->win;
    }

  return NULL;
  
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
      list_move_tail (&we->node, &g->mapped_windows);
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
	  numset_release (g->numset, cur->number);
	  list_del (&cur->node);
	  free (cur);
	}
    }
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
group_last_window (rp_group *g)
{
  int last_access = 0;
  rp_window_elem *most_recent = NULL;
  rp_window_elem *cur;

  list_for_each_entry (cur, &g->mapped_windows, node)
    {
      if (cur->win->last_access >= last_access 
	  && cur->win != current_window()
	  && !find_windows_frame (cur->win))
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
    return group_last_window (g);

  /* If we can't find the window, then it's in a different group, so
     get the last accessed one in this group. */
  we = group_find_window (&g->mapped_windows, win);
  if (we == NULL)
    return group_last_window (g);

  /* The window is in this group, so find the next one in the list
     that isn't already displayed. */
  for (cur = list_next_entry (we, &g->mapped_windows, node); 
       cur != we; 
       cur = list_next_entry (cur, &g->mapped_windows, node))
    {
      if (!find_windows_frame (cur->win))
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
    return group_last_window (g);

  /* If we can't find the window, then it's in a different group, so
     get the last accessed one in this group. */
  we = group_find_window (&g->mapped_windows, win);
  if (we == NULL)
    return group_last_window (g);

  /* The window is in this group, so find the previous one in the list
     that isn't already displayed. */
  for (cur = list_prev_entry (we, &g->mapped_windows, node); 
       cur != we; 
       cur = list_prev_entry (cur, &g->mapped_windows, node))
    {
      if (!find_windows_frame (cur->win))
	{
	  return cur->win;
	}
    }

  return NULL;

}
