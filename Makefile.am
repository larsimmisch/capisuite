spooldir = @localstatedir@/spool/capisuite
pkgsysconfdir = @sysconfdir@/capisuite
docdir = @docdir@
doc_DATA = COPYING NEWS README
EXTRA_DIST = rc.capisuite.in capisuite.cronin cronjob.conf

SUBDIRS = src scripts docs

all: capisuite.cron rc.capisuite

capisuite.cron: capisuite.cronin
	rm -f $@
	sed -e 's,@pkgsysconfdir\@,$(pkgsysconfdir),g' \
            -e 's,@spooldir\@,$(spooldir),g' $< >$@
	chmod a+x $@

rc.capisuite: rc.capisuite.in
	rm -f $@
	sed -e 's,@pkgsysconfdir\@,$(pkgsysconfdir),g' \
	    -e 's,@sbindir\@,$(sbindir),g' $< >$@
	chmod a+x $@

clean-local:
	rm -f rc.capisuite capisuite.cron 

