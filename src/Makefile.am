MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = gpasman
gpasman_SOURCES = \
	gpasman.c \
	gpasman.h \
	file.c \
	file.h \
	librc2.c \
	rc2.h \
	i18n-support.h \
	gtk-patch.c \
	gtk-patch.h

OPT = $(GTK_PATCH)
WARN = #-Wall -Werror
DEBUG = #-g3
EXTRA_CFLAGS = $(OPT) $(WARN) $(DEBUG)

INCLUDES = \
	$(GTK_CFLAGS)

CFLAGS = \
	$(EXTRA_CFLAGS)

gpasman_LDADD = \
	$(GTK_LIBS)

DEFS = -DLOCALEDIR=\"$(datadir)/locale\" @DEFS@

