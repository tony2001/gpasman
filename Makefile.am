MAINTAINERCLEANFILES = Makefile.in \
	gpasman.desktop \
	configure config.guess config.sub aclocal.m4 autom4te.cache \
	@PACKAGE@-@VERSION@.tar.gz

SUBDIRS = src pixmaps

DESKTOP_IN_FILES = gpasman.desktop.in
DESKTOP_FILES = $(DESKTOP_IN_FILES:.desktop.in=.desktop)

launcherdir = $(datadir)/applications
launcher_DATA = $(DESKTOP_FILES)

EXTRA_DIST = \
	$(DESKTOP_IN_FILES) \
	$(DESKTOP_FILES) \
	debian/ \
	gpasman.spec.in \
	gpasman.spec \
	rc2-1.1.0/ \
	LICENCE.Artistic \
	LICENCE.GPLv2 \
	autogen.sh

