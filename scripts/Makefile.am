spooldir = @localstatedir@/spool/capisuite
pkgsysconfdir = @sysconfdir@/capisuite

dist_pkglib_DATA = idle.py incoming.py README
python_module_DATA = cs_helpers.py
EXTRA_DIST = cs_helpers.pyin fax.confin answering_machine.confin

pkgsysconf_DATA = fax.conf answering_machine.conf

dist_bin_SCRIPTS = capisuitefax

SUBDIRS = waves

.pyin.py:
	rm -f $@
	sed -e 's,@\pkgsysconfdir@,$(pkgsysconfdir),g' $< >$@

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
	rm -f cs_helpers.py
	rm -f fax.conf answering_machine.conf

