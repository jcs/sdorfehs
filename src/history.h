#ifndef _RATPOISON_HISTORY_H
#define _RATPOISON_HISTORY_H 1

void  history_load ();
void  history_save ();
void  history_resize (int size);
void  history_reset ();
void  history_add (char *item);
char *history_next ();
char *history_previous ();
char *history_list_items ();
int   history_expand_line (char *string, char **output);

#endif /* ! _RATPOISON_HISTORY_H */
