docdir = @docdir@
EXTRA_DIST = Doxyfile.in mainpage.doxy manual.docbook manual.README

# dist-hook may be not portable partly, so non-SuSE-users please
# forgive me and don't call "make dist" or send me a patch ;-)

dist-hook: manual-html manual-pdf reference-html
	mkdir $(distdir)/manual
	cp -r $(srcdir)/manual/* $(distdir)/manual/
	mkdir $(distdir)/reference
	cp $(srcdir)/reference/* $(distdir)/reference/
	cp manual.pdf $(distdir)/

manual-html:
	sed -e 's,<title>CapiSuite [0-9a-z\.]*</title>,<title>CapiSuite $(VERSION)</title>,g' \
	  manual.docbook > manual.new # (not another *.in file...)
	mv manual.new manual.docbook
	xmllint --noout --valid manual.docbook
	xsltproc -o manual/ /usr/share/sgml/docbook/docbook-xsl-stylesheets/xhtml/chunk.xsl manual.docbook # create HTML
	cp -r /usr/share/sgml/docbook/docbook-xsl-stylesheets/images/ manual/ # missing images...
	if test ! -d manual/images/callouts; then \
		echo "manual pages weren't created succesful." ;\
		exit 1; \
	fi

manual-pdf: manual-html
	ln -fs manual.docbook manual.xml # unfortunately db2pdf wants this...
	db2pdf manual.xml 
	-rm CATALOG.local manual.aux manual.log manual.out manual.tex suse-*.dsl

reference-html: Doxyfile.in
	sed -e 's,@version\@,$(VERSION),g' \
	    -e 's,@capisuite_sources\@,$(top_srcdir)/src,g' \
	    -e 's,@srcdir\@,$(srcdir),g' Doxyfile.in > Doxyfile
	$(doxygen) Doxyfile
	if test ! -d reference; then \
		echo "Creation of reference didn't succeed. Please install doxygen and try again" ;\
		exit 1; \
	fi
	rm Doxyfile
	
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
	if test -f manual.pdf; then \
		cp manual.pdf $(DESTDIR)$(docdir)/ ; \
	fi

uninstall-local:
	rm -rf $(DESTDIR)$(docdir) 

maintainer-clean-local:
	rm -rf reference manual manual.pdf
 
