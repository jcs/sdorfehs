## Process this file with automake to produce Makefile.in
## Copyright (C) 2000, 2001 Shawn Betts <sabetts@users.sourceforge.net>
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
## $Id: Makefile.am,v 1.9 2001/09/16 09:48:43 sabetts Exp $

PACKAGE = @PACKAGE@
VERSION = @VERSION@

CLEANFILES		= *~ gmon.out
MAINTAINERCLEANFILES	= Makefile.in aclocal.m4 \
			config.guess config.h.in config.sub configure \
			install-sh missing mkinstalldirs stamp-h.in

SUBDIRS			= src doc contrib

docdir			= $(datadir)/doc/$(PACKAGE)
doc_DATA		= AUTHORS COPYING ChangeLog NEWS README

EXTRA_DIST		= $(doc_DATA)

dist-hook:
	chmod -R go-w $(distdir)

bin-dist strip-bin-dist: all
	-rm -rf $(PACKAGE)-$(VERSION).bin.$(host_triplet)
	$(MAKE) $(subst strip-,-strip,install$(subst bin-dist,,$@)) \
		prefix=`pwd`/$(PACKAGE)-$(VERSION).bin.$(host_triplet)/$(prefix)
	$(TAR) chof - $(PACKAGE)-$(VERSION).bin.$(host_triplet) \
		 | GZIP=$(GZIP_ENV) gzip -c > $(PACKAGE)-$(VERSION).bin.$(host_triplet).tar.gz
	-rm -rf $(PACKAGE)-$(VERSION).bin.$(host_triplet)

sense:
	@echo 42

.PHONY: bin-dist strip-bin-dist dist-hook sense
