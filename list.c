/* functions for handling the window list */

#include <stdio.h>
#include <stdlib.h>

#include "ratpoison.h"

rp_window *rp_window_head, *rp_window_tail;
rp_window *rp_current_window;

rp_window *
add_to_window_list (screen_info *s, Window w)
{
  rp_window *new_window;

  new_window = malloc (sizeof (rp_window));
  if (new_window == NULL)
    {
      fprintf (stderr, "list.c:add_to_window_list():Out of memory!\n");
      exit (EXIT_FAILURE);
    }
  new_window->w = w;
  new_window->scr = s;
  new_window->last_access = 0;
  new_window->prev = NULL;
  if ((new_window->name = malloc (strlen ("Unnamed") + 1)) == NULL)
    {
      fprintf (stderr, "list.c:add_to_window_list():Out of memory.\n");
      exit (EXIT_FAILURE);
    }
  strcpy (new_window->name, "Unnamed");

  if (rp_window_head == NULL)
    {
      /* The list is empty. */
      rp_window_head = new_window;
      rp_window_tail = new_window;
      new_window->next = NULL;
      return new_window;
    }

  /* Add the window to the head of the list. */
  new_window->next = rp_window_head;
  rp_window_head->prev = new_window;
  rp_window_head = new_window;

  return new_window;
}

/* Check to see if the window is already in our list of managed windows. */
rp_window *
find_window (Window w)
{
  rp_window *cur;

  for (cur = rp_window_head; cur; cur = cur->next)
    if (cur->w == w) return cur;

  return NULL;
}
      
void
remove_from_window_list (rp_window *w)
{
  if (rp_window_head == w) rp_window_head = w->next;
  if (rp_window_tail == w) rp_window_tail = w->prev;

  if (w->prev != NULL) w->prev->next = w->next;
  if (w->next != NULL) w->next->prev = w->prev;

  if (rp_current_window == w) rp_current_window = rp_window_head;
  if (rp_current_window && rp_current_window->state == STATE_UNMAPPED) next_window ();

  free (w);
#ifdef DEBUG
  printf ("Removed window from list.\n");
#endif
}

void
set_current_window (rp_window *win)
{
  rp_current_window = win;  
}

void
init_window_list ()
{
  rp_window_head = rp_window_tail = NULL;
  rp_current_window = NULL;
}

void
next_window ()
{
  if (rp_current_window != NULL)
    {
      rp_current_window = rp_current_window->next;
      if (rp_current_window == NULL) 
	{
	  rp_current_window = rp_window_head;
	}
      if (rp_current_window->state == STATE_UNMAPPED) next_window ();
      set_active_window (rp_current_window);
    }
}

void
prev_window ()
{
  if (rp_current_window != NULL)
    {
      set_current_window (rp_current_window->prev);
      if (rp_current_window == NULL) 
	{
	  rp_current_window = rp_window_tail;
	}
      if (rp_current_window->state == STATE_UNMAPPED) prev_window ();
      set_active_window (rp_current_window);
    }
}

rp_window *
find_window_by_number (int n)
{
  int i;
  rp_window *cur;

  for (i=0, cur=rp_window_head; cur; cur=cur->next)
    {
      if (cur->state != STATE_MAPPED) continue;

      if (i == n) return cur;
      else i++;
    }

  return NULL;
}

void
goto_window_number (int n)
{
  rp_window *win;

  if ((win = find_window_by_number (n)) == NULL)
    {
      return;
    }

  rp_current_window = win;
  set_active_window (rp_current_window);
}

rp_window *
find_last_accessed_window ()
{
  int last_access = 0;
  rp_window *cur, *most_recent = NULL;

  for (cur=rp_window_head; cur; cur=cur->next)
    {
      if (cur->last_access >= last_access 
	  && cur != rp_current_window 
	  && cur->state == STATE_MAPPED) 
	{
	  most_recent = cur;
	  last_access = most_recent->last_access;
	}
    }

  return most_recent;
}

void
last_window ()
{
  rp_current_window = find_last_accessed_window ();
  set_active_window (rp_current_window);
}

void
set_active_window (rp_window *rp_w)
{
  static int counter = 1;	/* increments every time this function
                                   is called. This way we can track
                                   which window was last accessed.  */

  if (rp_w == NULL) return;

  counter++;
  rp_w->last_access = counter;

  if (rp_w->scr->bar_is_raised) update_window_names (rp_w->scr);

  XSetInputFocus (dpy, rp_w->w, 
		  RevertToPointerRoot, CurrentTime);
  XRaiseWindow (dpy, rp_w->w);
      
  /* Make sure the program bar is always on the top */
  update_window_names (rp_w->scr);
}
