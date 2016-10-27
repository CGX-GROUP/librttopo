ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src headers

EXTRA_DIST = \
  ChangeLog \
  CREDITS \
  doc/BUILDING-ON-WINDOWS.md \
  doc/DATASTORES.md \
  doc/g_serialized.txt \
  makefile.vc \
  nmake.opt \
	README.md

AUTOMAKE_OPTIONS = dist-zip foreign

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rttopo.pc

ChangeLog:
	cd $(srcdir) && git2cl > $@

