docdir = @docdir@
EXTRA_DIST = Doxyfile.in mainpage.doxy manual.docbook manual.README

dist-hook: Doxyfile.in
	sed -e 's,@version\@,$(VERSION),g' \
	    -e 's,@capisuite_sources\@,$(top_srcdir)/src,g' \
	    -e 's,@srcdir\@,$(srcdir),g' Doxyfile.in > Doxyfile
	$(doxygen) Doxyfile
	if test ! -d reference; then \
		echo "Creation of reference didn't succeed. Please install doxygen and try again" ;\
		exit 1; \
	fi
	rm Doxyfile
	if test ! -d manual/images/callouts; then \
		echo "You didn't create the manual pages or forgot to copy images to manual/. Please read docs/manual.README" ;\
		exit 1; \
	fi
	mkdir $(distdir)/reference
	cp $(srcdir)/reference/* $(distdir)/reference/
	mkdir $(distdir)/manual
	cp -r $(srcdir)/manual/* $(distdir)/manual/

install-data-local:
	if test -d reference; then \
		$(mkinstalldirs) $(DESTDIR)$(docdir)/reference ; \
		(cd reference; for i in *; do \
			$(INSTALL_DATA) $$i $(DESTDIR)$(docdir)/reference/$$i  ;\
		done;) ;\
	fi

	if test -d manual; then \
		$(mkinstalldirs) $(DESTDIR)$(docdir) ; \
		cp -r manual $(DESTDIR)$(docdir)/ ; \
	fi

uninstall-local:
	rm -rf $(DESTDIR)$(docdir) 

clean-local:
	rm -f Doxyfile

maintainer-clean-local:
	rm -rf reference manual;
 
