#include "sbuf.h"

/* ------------------------------------ move to separate file */
#include <stdlib.h>
#include <stdio.h>
void
fatal (const char *msg)
{
  fprintf (stderr, "%s", msg);
  abort ();
}

void *
xmalloc (size_t size)
{
  register void *value = malloc (size);
  if (value == 0)
    fatal ("virtual memory exhausted");
  return value;
}

void *
xrealloc (void *ptr, size_t size)
{
  register void *value = realloc (ptr, size);
  if (value == 0)
    fatal ("Virtual memory exhausted");
  fprintf (stderr, "realloc: %d\n", size);
  return value;
}
/*------------------------------------------------------------*/

struct sbuf *
sbuf_new (size_t initsz)
{
  struct sbuf *b = (struct sbuf*) xmalloc (sizeof (struct sbuf));

  if (initsz < 1) 
    initsz = 1;

  b->data = (char*) xmalloc (initsz);
  b->maxsz = initsz;

  b->data[0] = '\0';
  b->len = 0;

  return b;
}

void
sbuf_free (struct sbuf *b)
{
  if (b != NULL)
    {
      if (b->data != NULL)
	free (b->data);
      
      free (b);
    }
}

char *
sbuf_concat (struct sbuf *b, const char *str)
{
  size_t minsz = b->len + strlen (str) + 1;

  if (b->maxsz < minsz)
    {
      b->data = (char*) xrealloc (b->data, minsz);
      b->maxsz = minsz;
    }
  
  memcpy (b->data + b->len, str, minsz - b->len - 1);
  b->len = minsz - 1;
  *(b->data + b->len) = 0;

  return b->data;
}

char *
sbuf_copy (struct sbuf *b, const char *str)
{
  b->len = 0;
  return sbuf_concat (b, str);
}

char *
sbuf_clear (struct sbuf *b)
{
  b->len = 0;
  b->data[0] = '\0';
  return b->data;
}

char *
sbuf_get (struct sbuf *b)
{
  return b->data;
}

