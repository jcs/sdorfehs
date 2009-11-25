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
##
## $Id: Makefile.am,v 1.12 2006/11/21 17:03:31 brl Exp $

PACKAGE = @PACKAGE@
VERSION = @VERSION@

CLEANFILES		= *~ gmon.out
MAINTAINERCLEANFILES	= Makefile.in aclocal.m4 \
			config.guess config.h.in config.sub configure \
			install-sh missing mkinstalldirs stamp-h.in depcomp

SUBDIRS			= src doc contrib

doc_DATA		= AUTHORS COPYING ChangeLog NEWS README

EXTRA_DIST		= $(doc_DATA)

dist-hook:
	chmod -R go-w $(distdir)

sense:
	@echo 42

.PHONY: bin-dist strip-bin-dist dist-hook sense
