#ifndef UTF8_H
#define UTF8_H

/* UTF-8 handling macros */
#define RP_IS_UTF8_CHAR(c) (utf8_locale && (c) & 0xC0)
#define RP_IS_UTF8_START(c) (utf8_locale && ((c) & 0xC0) == 0xC0)
#define RP_IS_UTF8_CONT(c) (utf8_locale && ((c) & 0xC0) == 0x80)

extern int utf8_locale;

int utf8_check_locale(void);

#endif
