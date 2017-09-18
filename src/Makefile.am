## Process this file with automake to produce Makefile.in
## Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
##
## This file is part of ratpoison.
##
## ratpoison is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## ratpoison is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

bin_PROGRAMS		= ratpoison
MAINTAINERCLEANFILES = Makefile.in config.h.in

AM_CPPFLAGS=-DSYSCONFDIR='"$(sysconfdir)"' ${X_CFLAGS} ${XFT_CFLAGS}
ratpoison_LDADD=${XFT_LIBS} ${X_LIBS} ${X_EXTRA_LIBS} ${HISTORY_LIBS}

ratpoison_SOURCES	= actions.c		\
			actions.h		\
			bar.c			\
			bar.h			\
			completions.c		\
			completions.h		\
			communications.c	\
			communications.h	\
			conf.h			\
			data.h			\
			editor.c		\
			editor.h		\
			events.c		\
			events.h		\
			format.c		\
			format.h		\
			frame.c			\
			frame.h			\
			getopt.c		\
			getopt.h		\
			getopt1.c		\
			globals.h		\
			globals.c		\
			group.h			\
			group.c			\
			history.h		\
			history.c		\
			hook.c			\
			hook.h			\
			input.c			\
			input.h			\
			linkedlist.h		\
			linkedlist.c		\
			main.c			\
			manage.c		\
			manage.h		\
			messages.h		\
			number.c		\
			number.h		\
			ratpoison.h		\
			sbuf.c			\
			sbuf.h			\
			screen.h		\
			screen.c		\
			split.c			\
			split.h			\
			utf8.c			\
			utf8.h			\
			util.c			\
			util.h			\
			window.c		\
			window.h		\
			xrandr.c 		\
			xrandr.h

TESTS =			test-sbuf-utf8-nconcat
check_PROGRAMS =	test-sbuf-utf8-nconcat

test_sbuf_utf8_nconcat_SOURCES = 		\
			test-sbuf-utf8-nconcat.c \
			sbuf.c			\
			sbuf.h			\
			utf8.c			\
			utf8.h			\
			util.c			\
			util.h
