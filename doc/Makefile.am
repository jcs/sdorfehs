# Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
#
# This file is part of ratpoison.
#
# ratpoison is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# ratpoison is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

info_TEXINFOS = ratpoison.texi
man_MANS      = ratpoison.1
EXTRA_DIST    = ratpoison.mdoc.1 \
		ratpoison.man.1 \
		sample.ratpoisonrc \
		ipaq.ratpoisonrc \
		fdl.texi

if NO_MDOC_MANPAGE
ratpoison.1: ratpoison.man.1
	cp ratpoison.man.1 ratpoison.1
else
ratpoison.1: ratpoison.mdoc.1
	cp ratpoison.mdoc.1 ratpoison.1
endif

ratpoison.man.1: ratpoison.mdoc.1
	tmp=$$(mktemp) && \
	mandoc -Tman ratpoison.mdoc.1 > "$$tmp" && \
	mv "$$tmp" ratpoison.man.1

MAINTAINERCLEANFILES = Makefile.in ratpoison.man.1 texinfo.tex
