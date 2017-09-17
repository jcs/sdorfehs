#include "ratpoison.h"

#ifdef HAVE_LANGINFO_CODESET
# include <langinfo.h>
#endif

int utf8_locale;

int
utf8_check_locale(void)
{
#ifdef HAVE_LANGINFO_CODESET
  utf8_locale = !strcmp (nl_langinfo (CODESET), "UTF-8");
#endif
  return utf8_locale;
}
