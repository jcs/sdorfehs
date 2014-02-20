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
CLEANFILES    = ratpoison.1
EXTRA_DIST    = $(srcdir)/ratpoison.mdoc.1 $(srcdir)/ratpoison.man.1 sample.ratpoisonrc ipaq.ratpoisonrc fdl.texi
MAINTAINERCLEANFILES = $(srcdir)/ratpoison.man.1 Makefile.in texinfo.tex

ratpoison.1: $(srcdir)/ratpoison.$(manpage_format).1 Makefile
	tmp=`mktemp` && \
	sed -e 's|%%sysconfdir%%|$(sysconfdir)|g' \
	    -e 's|%%docdir%%|$(docdir)|g' \
	    < $(srcdir)/ratpoison.$(manpage_format).1 > "$$tmp" && \
	chmod 0644 "$$tmp" && \
	mv "$$tmp" ratpoison.1

$(srcdir)/ratpoison.man.1: $(srcdir)/ratpoison.mdoc.1
	tmp=`mktemp` && \
	$(MANDOC) -Tman < $(srcdir)/ratpoison.mdoc.1 > "$$tmp" && \
	chmod 0644 "$$tmp" && \
	mv "$$tmp" $(srcdir)/ratpoison.man.1
