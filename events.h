/* Function prototypes */

#ifndef _EVENTS_H
#define _EVENTS_H

void handle_events ();
void delegate_event (XEvent *ev);
void key_press (XEvent *ev);
void map_request (XEvent *ev);
void unmap_notify (XEvent *ev);

#endif _EVENTS_H
