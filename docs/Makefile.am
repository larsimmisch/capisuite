docdir = @docdir@
EXTRA_DIST = Doxyfile.in mainpage.doxy manual.docbook manual.README

dist_man_MANS = capisuite.8 capisuite.conf.5 fax.conf.5 answering_machine.conf.5 capisuitefax.1

# dist-hook may be not portable partly, so non-SuSE-users please
# forgive me and don't call "make dist" or send me a patch ;-)

dist-hook: manual-prepare manual-html reference-html capisuite.8 capisuite.conf.5 fax.conf.5 answering_machine.conf.5 capisuitefax.1 manual-pdf
	mkdir $(distdir)/manual
	cp -r $(srcdir)/manual/* $(distdir)/manual/
	mkdir $(distdir)/reference
	cp $(srcdir)/reference/* $(distdir)/reference/
	cp manual.pdf $(distdir)/

# force recreation of complete manual for dist
recreatedoc:
	touch manual.docbook

manual-prepare: recreatedoc
	sed -e 's,<title>CapiSuite [0-9a-z\.]*</title>,<title>CapiSuite $(VERSION)</title>,g' \
	  manual.docbook > manual.docbook.new
	mv manual.docbook.new manual.docbook
	xmllint --noout --valid manual.docbook

manual-html: manual.docbook
	xsltproc --stringparam profile.condition html -o manual-profiled.docbook \
	  /usr/share/xml/docbook/stylesheet/nwalsh/current/profiling/profile.xsl \
	   manual.docbook
	xsltproc --stringparam chunker.output.encoding UTF-8 \
	  --stringparam chunker.output.doctype-public "-//W3C//DTD XHTML 1.0 Transitional//EN" \
	  --stringparam chunker.output.doctype-system "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd" \
	  -o manual/ /usr/share/xml/docbook/stylesheet/nwalsh/current/xhtml/chunk.xsl manual-profiled.docbook
	rm manual-profiled.docbook
	cp -r /usr/share/xml/docbook/stylesheet/nwalsh/current/images/ manual/ # missing images...
	if test ! -d manual/images/callouts; then \
		echo "manual pages weren't created succesful." ;\
		exit 1; \
	fi

capisuite.8 capisuite.conf.5 fax.conf.5 answering_machine.conf.5 capisuitefax.1: manual.docbook
	xsltproc --stringparam profile.condition man -o manual-profiled.docbook \
	  /usr/share/xml/docbook/stylesheet/nwalsh/current/profiling/profile.xsl \
	  manual.docbook
	xsltproc /usr/share/xml/docbook/stylesheet/nwalsh/current/manpages/docbook.xsl \
	  manual-profiled.docbook
	rm manual-profiled.docbook

manual-pdf: manual.docbook
	xsltproc --stringparam profile.condition pdf -o manual.xml \
	  /usr/share/xml/docbook/stylesheet/nwalsh/current/profiling/profile.xsl \
	  manual.docbook
	db2pdf manual.xml 
	-rm CATALOG.local manual.aux manual.log manual.out manual.tex suse-*.dsl manual.xml

reference-html: Doxyfile.in
	sed -e 's,@VERSION\@,$(VERSION),g' \
	    -e 's,@capisuite_sources\@,$(top_srcdir)/src,g' \
	    -e 's,@docbuilddir\@,.,g' \
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
	-rm -rf $(DESTDIR)$(docdir) 

maintainer-clean-local:
	-rm -rf reference manual manual.pdf *.[1-9]

