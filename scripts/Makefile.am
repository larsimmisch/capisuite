spooldir = @localstatedir@/spool/capisuite
pkgsysconfdir = @sysconfdir@/capisuite

dist_pkgdata_DATA = idle.py incoming.py README
pkgpython_PYTHON = helpers.py
EXTRA_DIST = helpers.pyin fax.confin answering_machine.confin capisuitefax.in

pkgsysconf_DATA = fax.conf answering_machine.conf

bin_SCRIPTS = capisuitefax capisuite-checkconfig

SUBDIRS = waves

capisuite-checkconfig: capisuite-checkconfig.in
	rm -f $@
	sed 's,@\pkgsysconfdir@,$(pkgsysconfdir),g' $< >$@
	chmod 755 $@

capisuitefax: capisuitefax.in
	rm -f $@
	sed 's,@\PYTHON@,$(PYTHON),g' $< >$@
	chmod 755 $@

.pyin.py:
	rm -f $@
	sed 's,@sfftobmp_major_version\@,$(sfftobmp_major_version),g' $< >$@

.confin.conf: 
	rm -f $@
	sed -e 's,@pkgdatadir\@,$(pkgdatadir),g' \
            -e 's,@spooldir\@,$(spooldir),g' $< >$@

uninstall-hook:
	-rmdir $(DESTDIR)$(pkglibdir)
	-rmdir $(DESTDIR)$(spooldir)/sendq $(DESTDIR)$(spooldir)/done \
               $(DESTDIR)$(spooldir)/failed $(DESTDIR)$(spooldir)/users $(DESTDIR)$(spooldir)

install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(spooldir)/sendq
	$(mkinstalldirs) $(DESTDIR)$(spooldir)/done
	$(mkinstalldirs) $(DESTDIR)$(spooldir)/failed
	$(mkinstalldirs) $(DESTDIR)$(spooldir)/users

clean-local:
	rm -f helpers.py
	rm -f fax.conf answering_machine.conf

