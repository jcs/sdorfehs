#ifndef _SBUF_H
#define _SBUF_H

#include <stdlib.h>

struct 
sbuf
{
  char *data;
  size_t len;
  size_t maxsz;
};

struct sbuf *sbuf_new (size_t initsz);
void sbuf_free (struct sbuf *b);
char *sbuf_concat (struct sbuf *b, const char *str);
char *sbuf_copy (struct sbuf *b, const char *str);
char *sbuf_clear (struct sbuf *b);
char *sbuf_get (struct sbuf *b);

#endif /* _SBUF_H */
