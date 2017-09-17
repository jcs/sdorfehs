#ifndef UTIL_H
#define UTIL_H

void fatal (const char *msg);
void *xmalloc (size_t size);
void *xrealloc (void *ptr, size_t size);
char *xstrdup (const char *s);
char *xvsprintf (char *fmt, va_list ap);
char *xsprintf (char *fmt, ...);
char *strtok_ws (char *s);
int str_comp (char *s1, char *s2, size_t len);

#endif
