/* manage.h */

#ifndef _MANAGE_H
#define _MANAGE_H

#include "data.h"

void grab_keys ();
void scanwins(screen_info *s);
void manage (rp_window *w, screen_info *s);
void unmanage (rp_window *w);

#endif /* _MANAGE_H */
