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

rp_frame *
screen_find_frame_by_frame (rp_screen *s, rp_frame *f)
{
  rp_frame *cur;

  list_for_each_entry (cur, &s->frames, node)
    {
      PRINT_DEBUG (("cur=%p f=%p\n", cur, f));
      if (cur == f)
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
is_a_root_window (unsigned int w)
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
  screens = xmalloc (sizeof (rp_screen) * num_screens);
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
  XGCValues gcv;
  struct sbuf *buf;
  int xine_screen_num;
  char *colon;

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
	       | SubstructureRedirectMask | SubstructureNotifyMask
	       | StructureNotifyMask);
  XSync (dpy, False);

  /* Set the numset for the frames to our global numset. */
  s->frames_numset = rp_frame_numset;

  /* Build the display string for each screen */
  buf = sbuf_new (0);
  sbuf_printf (buf, "DISPLAY=%s", DisplayString (dpy));
  colon = strrchr (sbuf_get (buf), ':');
  if (colon)
    {
      char *dot;

      dot = strrchr (sbuf_get (buf), '.');
      if (!dot || dot < colon)
        {
          /* no dot was found or it belongs to fqdn - append screen_num
             to the end */
          sbuf_printf_concat (buf, ".%d", screen_num);
        }
    }
  s->display_string = sbuf_free_struct (buf);

  PRINT_DEBUG (("display string: %s\n", s->display_string));

  s->screen_num = screen_num;
  s->xine_screen_num = xine_screen_num;
  s->root = RootWindow (dpy, screen_num);
  s->def_cmap = DefaultColormap (dpy, screen_num);

  init_rat_cursor (s);

  s->fg_color = BlackPixel (dpy, s->screen_num);
  s->bg_color = WhitePixel (dpy, s->screen_num);
  s->fw_color = BlackPixel (dpy, s->screen_num);
  s->bw_color = BlackPixel (dpy, s->screen_num);

  /* Setup the GC for drawing the font. */
  gcv.foreground = s->fg_color;
  gcv.background = s->bg_color;
  gcv.function = GXcopy;
  gcv.line_width = 1;
  gcv.subwindow_mode = IncludeInferiors;
  s->normal_gc = XCreateGC(dpy, s->root,
                           GCForeground | GCBackground | GCFunction
                           | GCLineWidth | GCSubwindowMode,
                           &gcv);
  gcv.foreground = s->bg_color;
  gcv.background = s->fg_color;
  s->inverse_gc = XCreateGC(dpy, s->root,
                            GCForeground | GCBackground | GCFunction
                            | GCLineWidth | GCSubwindowMode,
                            &gcv);

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

  activate_screen(s);

  XSync (dpy, 0);

#ifdef USE_XFT_FONT
  {
    s->xft_font = XftFontOpenName (dpy, screen_num, DEFAULT_XFT_FONT);
    if (!s->xft_font)
      {
        PRINT_ERROR(("Failed to open font\n"));
      }
    else
      {
        if (!XftColorAllocName (dpy, DefaultVisual (dpy, screen_num),
                                DefaultColormap (dpy, screen_num),
                                defaults.fgcolor_string, &s->xft_fg_color))
          {
            PRINT_ERROR(("Failed to allocate font fg color\n"));
            XftFontClose (dpy, s->xft_font);
            s->xft_font = NULL;
          }
        if (!XftColorAllocName (dpy, DefaultVisual (dpy, screen_num),
                                DefaultColormap (dpy, screen_num),
                                defaults.bgcolor_string, &s->xft_bg_color))
          {
            PRINT_ERROR(("Failed to allocate font fg color\n"));
            XftFontClose (dpy, s->xft_font);
            s->xft_font = NULL;
          }
      }
  }
#endif
}

void
activate_screen (rp_screen *s)
{
  /* Add netwm support. FIXME: I think this is busted. */
  XChangeProperty (dpy, RootWindow (dpy, s->screen_num),
		   _net_supported, XA_ATOM, 32, PropModeReplace, 
		   (unsigned char*)&_net_wm_pid, 1);

  /* set window manager name */
  XChangeProperty (dpy, RootWindow (dpy, s->screen_num),
		   _net_wm_name, xa_utf8_string, 8, PropModeReplace,
		   (unsigned char*)"ratpoison", 9);
  XMapWindow (dpy, s->key_window);
}

void
deactivate_screen (rp_screen *s)
{
  /* Unmap its key window */
  XUnmapWindow (dpy, s->key_window);

  /* delete everything so noone sees them while we are not there */
  XDeleteProperty (dpy, RootWindow (dpy, s->screen_num),
		   _net_supported);
  XDeleteProperty (dpy, RootWindow (dpy, s->screen_num),
		   _net_wm_name);
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

char *
screen_dump (rp_screen *screen)
{
  char *tmp;
  struct sbuf *s;

  s = sbuf_new (0);
  sbuf_printf (s, "%d %d %d %d %d %d",
               (rp_have_xinerama)?screen->xine_screen_num:screen->screen_num,
               screen->left,
               screen->top,
               screen->width,
               screen->height,
               (current_screen() == screen)?1:0 /* is current? */
               );

  /* Extract the string and return it, and don't forget to free s. */
  tmp = sbuf_get (s);
  free (s);
  return tmp;
}

void
screen_update (rp_screen *s, int width, int height)
{
  rp_frame *f;
  int oldwidth,oldheight;

  PRINT_DEBUG (("screen_update(%d,%d)\n", width, height));
  if (rp_have_xinerama)
    {
      /* TODO: how to do this with xinerama? */
      return;
    }

  if (s->width == width && s->height == height)
    /* nothing to do */
    return;

  oldwidth = s->width; oldheight = s->height;
  s->width = width; s->height = height;

  XResizeWindow (dpy, s->help_window, width, height);

  list_for_each_entry (f, &s->frames, node)
    {
      f->x = (f->x*width)/oldwidth;
      f->width = (f->width*width)/oldwidth;
      f->y = (f->y*height)/oldheight;
      f->height = (f->height*height)/oldheight;
      maximize_all_windows_in_frame (f);
    }
}
