pkgsysconfdir = @sysconfdir@/capisuite

sbin_PROGRAMS = capisuite
capisuite_LDADD=application/libccapplication.a modules/libccmodules.a \
		backend/libccbackend.a
capisuite_SOURCES=main.cpp
SUBDIRS = application backend modules capisuite-py

pkgsysconf_DATA = capisuite.conf
EXTRA_DIST = capisuite.conf.in

capisuite.conf: capisuite.conf.in
	rm -f $@ 
	sed -e 's,@pkgdatadir\@,$(pkgdatadir),g' \
	  -e 's,@pkglibdir\@,$(pkglibdir),g' \
	  -e 's,@localstatedir\@,$(localstatedir),g' \
	  -e 's,@spooldir\@,$(spooldir),g' $< >$@

install-data-local:
	mkdir -p $(DESTDIR)$(localstatedir)/log

clean-local:
	rm -f capisuite.conf
