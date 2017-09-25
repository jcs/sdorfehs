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

ACLOCAL_AMFLAGS = -I m4 --install

CLEANFILES		= *~ gmon.out
MAINTAINERCLEANFILES	= Makefile.in aclocal.m4 configure src/config.h.in \
			build-aux/depcomp build-aux/install-sh  \
			build-aux/missing build-aux/texinfo.tex

SUBDIRS			= src doc contrib

doc_DATA		= AUTHORS COPYING NEWS README

EXTRA_DIST		= $(doc_DATA)
