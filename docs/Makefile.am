docdir = @docdir@
EXTRA_DIST = Doxyfile.in mainpage.doxy manual.docbook manual.README 

all: docs

docs: Doxyfile
	if test "x$(doxygen)" != "x"; then \
		$(doxygen) Doxyfile ;\
	fi

dist-hook: Doxyfile 
	$(doxygen) Doxyfile
	mkdir $(distdir)/reference
	cp $(srcdir)/reference/* $(distdir)/reference/ 
	mkdir $(distdir)/manual
	cp $(srcdir)/manual/* $(distdir)/manual/

Doxyfile: Doxyfile.in
	sed -e 's,@version\@,$(VERSION),g' \
	    -e 's,@capisuite_sources\@,$(top_srcdir)/src,g' \
	    -e 's,@srcdir\@,$(srcdir),g' $< >$@

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/reference ; \
	(cd reference; for i in *; do \
		$(INSTALL_DATA) $$i $(DESTDIR)$(docdir)/reference/$$i  ;\
        done;) 
	$(mkinstalldirs) $(DESTDIR)$(docdir)/manual ; \
	(cd manual; for i in *; do \
                $(INSTALL_DATA) $$i $(DESTDIR)$(docdir)/manual/$$i  ;\
        done;)

uninstall-local:
	rm -rf $(DESTDIR)$(docdir) 

clean-local:
	rm -f Doxyfile

maintainer-clean-local:
	rm -rf reference manual;
 
