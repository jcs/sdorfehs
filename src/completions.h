/* Function prototypes.
 */

#ifndef _RATPOISON_COMPLETIONS_H
#define _RATPOISON_COMPLETIONS_H 1

char *completions_complete (rp_completions *c, char *partial, int direction);
rp_completions *completions_new (completion_fn list_fn);
void completions_free (rp_completions *c);

#endif /* ! _RATPOISON_COMPLETIONS_H */
