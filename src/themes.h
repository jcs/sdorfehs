#ifndef THEME_H
#define THEME_H

#define T_CLASSIC 	1
#define T_CHOCOLATE_BAR	2
#define T_MAPLE_LEAF	3
#define T_CHRISTMAS 	4

#ifndef THEME
#  define FOREGROUND    "black"
#  define BACKGROUND    "white"
#  define FONT      	"9x15bold"
#else
#  if THEME == T_RATPOISON_CLASSIC
#    define FOREGROUND 	"black"
#    define BACKGROUND 	"lightgreen"
#    define FONT    	"fixed"
#  elif THEME == T_CHOCOLATE_BAR
#    define FOREGROUND 	"chocolate4"
#    define BACKGROUND 	"chocolate1"
#    define FONT    	"8x13bold"
#  elif THEME == T_MAPLE_LEAF
#    define FOREGROUND 	"red"
#    define BACKGROUND 	"white"
#    define FONT    	"12x24"
#  elif THEME == T_CHRISTMAS
#    define FOREGROUND 	"red"
#    define BACKGROUND 	"green"
#    define FONT    	"12x24"
#  else
#    error unknown theme
#  endif
#endif

#endif
