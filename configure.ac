dnl Process this file with autoconf to produce a configure script.
dnl Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
dnl
dnl This file is part of ratpoison.
dnl
dnl ratpoison is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl ratpoison is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

AC_INIT([ratpoison], [1.4.10-beta], [ratpoison-devel@nongnu.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])

AM_MAINTAINER_MODE([enable])

AC_CONFIG_HEADER(src/config.h)

dnl by default turn off debugging
AC_MSG_CHECKING([whether to enable debugging])
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],[build with extra debugging messages])],
  [], [enable_debug=no])
AS_CASE([$enable_debug],
  [yes], [AC_MSG_RESULT([yes])
   	  AC_DEFINE([DEBUG], 1, [Define this to enable debugging messages])],
  [no], [AC_MSG_RESULT([no])],
  [AC_MSG_ERROR([invalid --enable-debug="$enable_debug" parameter])])

AC_ARG_WITH([electric-fence],
  [AS_HELP_STRING([--with-electric-fence],
    [Use the Electric Fence library (memory allocation debugging).])])

AC_ARG_WITH([xterm],
  [AS_HELP_STRING([--with-xterm=PROG],
    [set the x terminal emulator used by ratpoison])],
  [term_prog=$withval],
  [term_prog=xterm])
AC_DEFINE_UNQUOTED([TERM_PROG], ["$term_prog"], [X terminal emulator to use])

XFT_CFLAGS=
XFT_LIBS=
AC_ARG_WITH(xft,AS_HELP_STRING([--without-xft],[Don't use the Xft library even if available]),
                xft=$withval, xft=default)
if test "x$xft" != "xno"; then
    dnl Configure Xft.
    PKG_CHECK_MODULES([XFT], [xft],,[noop=noop])
    AC_MSG_CHECKING(whether to use Xft)
    echo "pkg_failed is $pkg_failed" >&5
dnl pkg_failed can be yes, no or untried (if there is no pkg-config or that does not work).
    if test "$pkg_failed" = "no" ; then
       AC_DEFINE_UNQUOTED(USE_XFT_FONT, 1, [Define this to use Xft])
       AC_MSG_RESULT(yes)
    elif test "x$xft" != "xdefault" ; then
dnl if explicitly requested but not found, bail out:
    	AC_MSG_RESULT(no)
    	if test "$pkg_failed" = "untried" ; then
	    AC_MSG_ERROR([Not Xft library found (this needs a working pkg-config)!])
	else
	    AC_MSG_ERROR([No Xft library found!])
	fi
    else
    	AC_MSG_RESULT(no)
    fi
else
    AC_MSG_CHECKING(whether to use Xft)
    AC_MSG_RESULT(no)
fi
AC_SUBST(XFT_CFLAGS)
AC_SUBST(XFT_LIBS)

AC_ARG_WITH([xkb],
  [AS_HELP_STRING([--without-xkb], [Don't build XKB support for keyboard input.])],
  [xkb=$withval],
  [xkb=yes])
AS_IF([test "X$xkb" = "Xyes"],
  [AC_DEFINE([WANT_XKB], 1,
    [Define to 1 if you want to build XKB support for keyboard input.])])

AC_MSG_CHECKING([manpage format])
AC_ARG_ENABLE([mdoc],
  [AS_HELP_STRING([--disable-mdoc],
    [install man(7) manpages, useful if you don't have a mdoc(7) formatter\
     such as mandoc(1) or groff(1)])],
  [], [enable_mdoc=yes])
AS_CASE([$enable_mdoc],
  [yes], [manpage_format=mdoc],
  [no],  [manpage_format=man],
  [AC_MSG_ERROR([invalid --enable-mdoc="$enable_mdoc" parameter])])
AC_MSG_RESULT([$manpage_format])
AC_SUBST([manpage_format])

AC_MSG_CHECKING([Xrandr support])
AC_ARG_WITH([xrandr],
  [AS_HELP_STRING([--without-xrandr],
    [Build without Xrandr support, default is to require and use it.])],
  [with_xrandr=$withval],
  [with_xrandr=check])
AC_MSG_RESULT([$with_xrandr])

dnl Checks for programs.
AC_PROG_CC
AM_MISSING_PROG([MANDOC], [mandoc])
AC_ARG_VAR([MANDOC],
  [path to mandoc(1), only used when regenerating doc/ratpoison.man.1 \
   after editing doc/ratpoison.mdoc.1])

AX_CHECK_COMPILE_FLAG([-Wall], [CFLAGS="-Wall $CFLAGS"], [], [], [])

dnl check for an x terminal emulator
AC_MSG_CHECKING(terminal emulator)
AC_MSG_RESULT($term_prog)

dnl Check for the X libs
AC_PATH_X
AC_PATH_XTRA

if test "x$no_x" = "xyes"; then
	AC_MSG_ERROR([*** Can't find X11 headers and libs])
fi


dnl Those who do not learn the lessons of history
dnl are doomed to delete it...  yuk yuk.  --ttn
AC_ARG_ENABLE([history],
  AS_HELP_STRING([--enable-history],
    [Link with libhistory (default: disabled. Note: only used for ! expansion)]),
  [], [enable_history=no])

HISTORY_LIBS=
have_history=no
AS_CASE([$enable_history],
  [no], [],
  [yes],
    [AC_CHECK_HEADERS([readline/history.h],
      [AC_CHECK_LIB([history], [add_history], [have_history=yes], [])],
      [])],
   [AC_MSG_ERROR([invalid --disable-history="$enable_history" parameter])])

AC_MSG_CHECKING([whether to use libhistory])
AS_CASE([$enable_history:$have_history],
  [yes:no], [AC_MSG_ERROR([Can't find history headers/lib. \
Install readline dev libs for history expansion.])],
  [no:*], [AC_MSG_RESULT([no])],
  [yes:yes], [AC_MSG_RESULT([yes])
            AC_DEFINE_UNQUOTED(HAVE_HISTORY, 1, Define this to enable history)
  	    HISTORY_LIBS="-lhistory"])

AC_SUBST(HISTORY_LIBS)

AC_CHECK_LIB(X11, XOpenDisplay, [X_LIBS="-lX11 $X_LIBS"],
	AC_MSG_ERROR([*** Can't find libX11]),$X_LIBS $X_EXTRA_LIBS)

mysavedCPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $X_CFLAGS"
AC_CHECK_HEADERS([X11/XKBlib.h], [], [], [
#include <X11/Xlib.h>
])
CPPFLAGS="$mysavedCPPFLAGS"

AC_CHECK_LIB(Xtst, XTestFakeButtonEvent, [X_LIBS="-lXtst $X_LIBS"; AC_DEFINE(HAVE_LIBXTST,1,[Xtst])],,$X_LIBS $X_EXTRA_LIBS)
AC_CHECK_LIB([X11], [XkbKeycodeToKeysym],
  [AC_DEFINE(HAVE_XKBKEYCODETOKEYSYM, 1,
    [Define to 1 if you have the `XkbKeycodeToKeysym' function.])],
  [], [$X_LIBS $X_EXTRA_LIBS])

mysavedCPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $X_CFLAGS"
AC_CHECK_HEADERS([X11/extensions/Xrandr.h], [xrandr_h=yes], [xrandr_h=no], [
#include <X11/Xlib.h>
])
CPPFLAGS="$mysavedCPPFLAGS"

AC_CHECK_LIB(Xrandr, XRRGetScreenResources, [libxrandr=yes], [libxrandr=no], [$X_LIBS $X_EXTRA_LIBS])

AS_CASE(["$with_xrandr:$xrandr_h:$libxrandr"],
  [yes:no:*], [AC_MSG_ERROR([*** Can't enable Xrandr support, header Xrandr.h not found.])],
  [yes:*:no], [AC_MSG_ERROR([*** Can't enable Xrandr support, libXrandr not found.])],
  [check:no:*], [AC_MSG_ERROR([*** Header Xrandr.h not found, use ./configure --without-xrandr to build without multiple screens support.])],
  [check:*:no], [AC_MSG_ERROR([*** libXrandr not found, use ./configure --without-xrandr to build without multiple screens support.])],
  [yes:yes:yes|check:yes:yes], [X_LIBS="-lXrandr $X_LIBS"
  			        AC_DEFINE(HAVE_XRANDR, 1, [Define to 1 if you want Xrandr support.])])

AC_SUBST(X_LIBS)
AC_SUBST(X_EXTRA_LIBS)
AC_SUBST(X_CFLAGS)

AS_IF([test "$with_electric_fence" = yes],
  [AC_CHECK_LIB([efence], [malloc], [],
    [AC_MSG_ERROR([Couldn't find the Electric Fence library (-lefence)])])])

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([pwd.h sys/ioctl.h sys/stat.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_TYPEOF

AH_TEMPLATE([HAVE___BUILTIN_PREFETCH],
  [Define to 1 if your compiler supports the `__builtin_prefetch' function.])
AC_MSG_CHECKING([for __builtin_prefetch()])
AC_LINK_IFELSE([AC_LANG_PROGRAM([],
  [
    int a = 9;
    __builtin_prefetch(&a);
    return 0;
  ])],
  [AC_DEFINE([HAVE___BUILTIN_PREFETCH], 1)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])

dnl Checks for library functions.
AC_CHECK_FUNCS(getline getopt_long getpwuid setenv setpgid setpgrp setsid)
AC_CHECK_FUNCS(unsetenv vsnprintf)

AM_LANGINFO_CODESET

lispdir='${datarootdir}/emacs/site-lisp'
AC_SUBST([lispdir])

AC_CONFIG_FILES([Makefile doc/Makefile src/Makefile contrib/Makefile])
AC_OUTPUT
