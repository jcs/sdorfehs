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

#include "ratpoison.h"
#include <string.h>
#include <X11/cursorfont.h>

static void init_screen (rp_screen *s, int screen_num);

int
screen_width (rp_screen *s)
{
  return s->width - defaults.padding_right - defaults.padding_left;
}

int
screen_height (rp_screen *s)
{
  return s->height - defaults.padding_bottom - defaults.padding_top;
}

int
screen_left (rp_screen *s)
{
  return s->left + defaults.padding_left;
}

int
screen_right (rp_screen *s)
{
  return screen_left (s) + screen_width (s);
}

int
screen_top (rp_screen *s)
{
  return s->top + defaults.padding_top;
}

int
screen_bottom (rp_screen *s)
{
  return screen_top (s) + screen_height (s);
}

/* Returns a pointer to a list of frames. */
struct list_head *
screen_copy_frameset (rp_screen *s)
{
  struct list_head *head;
  rp_frame *cur;

  /* Init our new list. */
  head = xmalloc (sizeof (struct list_head));
  INIT_LIST_HEAD (head);

  /* Copy each frame to our new list. */
  list_for_each_entry (cur, &s->frames, node)
    {
      list_add_tail (&(frame_copy (cur))->node, head);
    }

  return head;
}

/* Set head as the frameset, deleting the existing one. */
void
screen_restore_frameset (rp_screen *s, struct list_head *head)
{
  frameset_free (&s->frames);
  INIT_LIST_HEAD (&s->frames);

  /* Hook in our new frameset. */
  list_splice (head, &s->frames);
}


/* Given a screen, free the frames' numbers from the numset. */
void
screen_free_nums (rp_screen *s)
{
  rp_frame *cur;

  list_for_each_entry (cur, &s->frames, node)
    {
      numset_release (s->frames_numset, cur->number);
    }
}

/* Given a list of frames, free them, but don't remove their numbers
   from the numset. */
void
frameset_free (struct list_head *head)
{
  rp_frame *frame;
  struct list_head *iter, *tmp;

  list_for_each_safe_entry (frame, iter, tmp, head, node)
    {
      /* FIXME: what if frames has memory inside its struct
	 that needs to be freed? */
      free (frame);
    }
}

rp_frame *
screen_get_frame (rp_screen *s, int frame_num)
{
  rp_frame *cur;

  list_for_each_entry (cur, &s->frames, node)
    {
      if (cur->number == frame_num)
	return cur;
    }
  
  return NULL;
}

/* Given a root window, return the rp_screen struct */
rp_screen *
find_screen (Window w)
{
  int i;

  for (i=0; i<num_screens; i++)
    if (screens[i].root == w) return &screens[i];

   return NULL;
 }

/* Return 1 if w is a root window of any of the screens. */
int
is_a_root_window (int w)
{
  int i;
  for (i=0; i<num_screens; i++)
    if (screens[i].root == w) return 1;

  return 0;
}

void
init_screens (int screen_arg, int screen_num)
{
  int i;
  
  /* Get the number of screens */
  if (rp_have_xinerama)
      num_screens = xine_screen_count;
  else
      num_screens = ScreenCount (dpy);
  
  /* make sure the screen specified is valid. */
  if (screen_arg)
    {
      /* Using just a single Xinerama screen doesn't really make sense. So we
       * disable Xinerama in this case.
       */
      if (rp_have_xinerama)
	{
	    fprintf (stderr, "Warning: selecting a specific Xinerama screen is not implemented.\n");
	    rp_have_xinerama = 0;
	    screen_num = 0;
	    num_screens = ScreenCount(dpy);
	}
      
      if (screen_num < 0 || screen_num >= num_screens)
	{
	  fprintf (stderr, "%d is an invalid screen for the display\n", screen_num);
	  exit (EXIT_FAILURE);
	}

      /* we're only going to use one screen. */
      num_screens = 1;
    }

  /* Create our global frame numset */
  rp_frame_numset = numset_new();

  /* Initialize the screens */
  screens = (rp_screen *)xmalloc (sizeof (rp_screen) * num_screens);
  PRINT_DEBUG (("%d screens.\n", num_screens));

  if (screen_arg)
    {
      init_screen (&screens[0], screen_num);
    }
  else
    {
      for (i=0; i<num_screens; i++)
	{
	  init_screen (&screens[i], i);
	}
    }

}

static void
init_rat_cursor (rp_screen *s)
{
  s->rat = XCreateFontCursor( dpy, XC_icon );
}

static void
init_screen (rp_screen *s, int screen_num)
{
  XGCValues gv;
  int xine_screen_num;

  /* We use screen_num below to refer to the real X screen number, but
   * if we're using Xinerama, it will only be the Xinerama logical screen
   * number.  So we shuffle it away and replace it with the real one now,
   * to cause confusion.  -- CP
   */
  if (rp_have_xinerama)
    {
      xine_screen_num = screen_num;
      screen_num = DefaultScreen(dpy);
      xinerama_get_screen_info(xine_screen_num,
		      &s->left, &s->top, &s->width, &s->height);
    }
  else
    {
      xine_screen_num = screen_num;
      s->left = 0;
      s->top = 0;
      s->width = DisplayWidth(dpy, screen_num);
      s->height = DisplayHeight(dpy, screen_num);
    }

  /* Select on some events on the root window, if this fails, then
     there is already a WM running and the X Error handler will catch
     it, terminating ratpoison. */
  XSelectInput(dpy, RootWindow (dpy, screen_num),
	       PropertyChangeMask | ColormapChangeMask
	       | SubstructureRedirectMask | SubstructureNotifyMask );
  XSync (dpy, False);

  /* Set the numset for the frames to our global numset. */
  s->frames_numset = rp_frame_numset;

  /* Build the display string for each screen */
  s->display_string = xmalloc (strlen(DisplayString (dpy)) + 21);
  sprintf (s->display_string, "DISPLAY=%s", DisplayString (dpy));
  if (strrchr (DisplayString (dpy), ':'))
    {
      char *dot;

      dot = strrchr(s->display_string, '.');
      if (dot)
	sprintf(dot, ".%i", screen_num);
    }

  PRINT_DEBUG (("%s\n", s->display_string));

  s->screen_num = screen_num;
  s->xine_screen_num = xine_screen_num;
  s->root = RootWindow (dpy, screen_num);
  s->def_cmap = DefaultColormap (dpy, screen_num);
  
  init_rat_cursor (s);

  s->fg_color = BlackPixel (dpy, s->screen_num);
  s->bg_color = WhitePixel (dpy, s->screen_num);

  /* Setup the GC for drawing the font. */
  gv.foreground = s->fg_color;
  gv.background = s->bg_color;
  gv.function = GXcopy;
  gv.line_width = 1;
  gv.subwindow_mode = IncludeInferiors;
  gv.font = defaults.font->fid;
  s->normal_gc = XCreateGC(dpy, s->root, 
			   GCForeground | GCBackground | GCFunction 
			   | GCLineWidth | GCSubwindowMode | GCFont, 
			   &gv);

  /* Create the program bar window. */
  s->bar_is_raised = 0;
  s->bar_window = XCreateSimpleWindow (dpy, s->root, 0, 0, 1, 1, 
				       defaults.bar_border_width, 
				       s->fg_color, s->bg_color);

  /* Setup the window that will receive all keystrokes once the prefix
     key has been pressed. */
  s->key_window = XCreateSimpleWindow (dpy, s->root, 0, 0, 1, 1, 0, 
				       WhitePixel (dpy, s->screen_num), 
				       BlackPixel (dpy, s->screen_num));
  XSelectInput (dpy, s->key_window, KeyPressMask | KeyReleaseMask);
  XMapWindow (dpy, s->key_window);

  /* Create the input window. */
  s->input_window = XCreateSimpleWindow (dpy, s->root, 0, 0, 1, 1, 
					 defaults.bar_border_width, 
					 s->fg_color, s->bg_color);
  XSelectInput (dpy, s->input_window, KeyPressMask | KeyReleaseMask);

  /* Create the frame indicator window */
  s->frame_window = XCreateSimpleWindow (dpy, s->root, 1, 1, 1, 1, defaults.bar_border_width, 
					 s->fg_color, s->bg_color);

  /* Create the help window */
  s->help_window = XCreateSimpleWindow (dpy, s->root, s->left, s->top, s->width,
					s->height, 0, s->fg_color, s->bg_color);
  XSelectInput (dpy, s->help_window, KeyPressMask);

  XSync (dpy, 0);
}

static int
is_rp_window_for_given_screen (Window w, rp_screen *s)
{
  if (w != s->key_window &&
      w != s->bar_window &&
      w != s->input_window &&
      w != s->frame_window &&
      w != s->help_window)
    return 0;
  return 1;
}

int
is_rp_window_for_screen(Window w, rp_screen *s)
{
  int i;

  if (rp_have_xinerama)
    {
      for (i=0; i<num_screens; i++)
        if (is_rp_window_for_given_screen(w, &screens[i])) return 1;
      return 0;
    }
  else
    {
      return is_rp_window_for_given_screen(w, s);
    }
}

