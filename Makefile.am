ACLOCAL_AMFLAGS = -I m4

SUBDIRS = conf src
EXTRA_DIST = Makefile.cvs debian

dist-hook:
	-rm -Rf `find $(distdir)/debian -name CVS`

include $(top_srcdir)/coverage.mk

COVERDIR = $(top_builddir)/coverage/html

$(COVERDIR):
	mkdir -p $@

coverage-report: $(notdir $(CURDIR)).info | $(COVERDIR)
	@tagrel=`echo $Name:  $ " | awk '{print $$2}'`; \
	genhtml --show-details --title "$$tagrel" --output-directory $(COVERDIR)/html $<
