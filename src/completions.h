/* Function prototypes.
 */

#ifndef _RATPOISON_COMPLETIONS_H
#define _RATPOISON_COMPLETIONS_H 1

char *completions_next_completion (rp_completions *c, char *partial);
void completions_update (rp_completions *c, char *partial);
void completions_assign (rp_completions *c, struct list_head *new_list);
rp_completions *completions_new (completion_fn list_fn);
void completions_free (rp_completions *c);

#endif /* ! _RATPOISON_COMPLETIONS_H */
