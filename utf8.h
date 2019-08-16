#ifndef UTF8_H
#define UTF8_H

extern int isu8char(char c);
extern int isu8start(char c);
extern int isu8cont(char c);

extern int utf8_locale;

int utf8_check_locale(void);

#endif
