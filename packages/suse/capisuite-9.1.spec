#
# spec file for package capisuite
#
# Copyright (c) 2004 SUSE LINUX AG, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://www.suse.de/feedback/
#

# norootforbuild
# neededforbuild  capi4linux gcc-c++ libstdc++-devel libxml2-devel python python-devel sfftobmp

BuildRequires: aaa_base acl attr bash bind-utils bison bzip2 coreutils cpio cpp cracklib cvs cyrus-sasl db devs diffutils e2fsprogs file filesystem fillup findutils flex gawk gdbm-devel glibc glibc-devel glibc-locale gpm grep groff gzip info insserv kbd less libacl libattr libgcc libselinux libstdc++ libxcrypt m4 make man mktemp module-init-tools ncurses ncurses-devel net-tools netcfg openldap2-client openssl pam pam-modules patch permissions popt procinfo procps psmisc pwdutils rcs readline sed strace syslogd sysvinit tar tcpd texinfo timezone unzip util-linux vim zlib zlib-devel binutils capi4linux gcc gcc-c++ gdbm gettext libstdc++-devel libtool libxml2-devel perl python python-devel rpm sfftobmp libjpeg libtiff

Name:         capisuite
License:      GPL
Group:        Hardware/ISDN
Autoreqprov:  on
Version:      0.4.5
Release:      0
%define pyver %(python -c 'import sys; print sys.version[:3]')
Requires:     sfftobmp sox tiff ghostscript-library python >= %pyver, python < %{pyver}.99
Summary:      ISDN telecommunication suite providing fax and voice services
Source0:      capisuite-%{version}.tar.gz
URL:          http://www.capisuite.de
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
PreReq:       %insserv_prereq

%description
CapiSuite is an ISDN telecommunication suite providing easy to use
telecommunication functions which can be controlled from Python
scripts.

It uses a CAPI-compatible driver for accessing the ISDN-hardware, so
you'll need an AVM card with the according driver.

CapiSuite is distributed with two example scripts for call incoming
handling and fax sending. See /usr/share/doc/packages/capisuite for
further information.



Authors:
--------
    Gernot Hillier

%prep
%setup  
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --with-docdir=%{_defaultdocdir}/capisuite --mandir=/usr/share/man

%build
make 

%install
make DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT/etc/init.d
mkdir -p $RPM_BUILD_ROOT/usr/sbin
mkdir -p $RPM_BUILD_ROOT/etc/cron.daily
install -m 644 cronjob.conf $RPM_BUILD_ROOT/etc/capisuite/cronjob.conf
install -m 755 rc.capisuite $RPM_BUILD_ROOT/etc/init.d/capisuite
install -m 755 capisuite.cron $RPM_BUILD_ROOT/etc/cron.daily/capisuite
ln -sf ../../etc/init.d/capisuite $RPM_BUILD_ROOT/usr/sbin/rccapisuite

%clean
rm -rf $RPM_BUILD_ROOT

%preun
%{stop_on_removal capisuite}

%postun
%{restart_on_update capisuite}
%{insserv_cleanup}

%files
%defattr(-,root,root)
%dir /etc/capisuite
%config /etc/capisuite/cronjob.conf
%config /etc/capisuite/capisuite.conf
%config /etc/capisuite/fax.conf
%config /etc/capisuite/answering_machine.conf
/usr/sbin/capisuite
/usr/bin/capisuitefax
%doc %{_defaultdocdir}/capisuite
/usr/share/capisuite
/usr/lib/capisuite
/var/spool/capisuite
/usr/%{_lib}/python2.?/site-packages/cs_helpers.py
/etc/init.d/capisuite
/etc/cron.daily/capisuite
/usr/sbin/rccapisuite
%{_mandir}/man1/capisuitefax.1.gz
%{_mandir}/man5/answering_machine.conf.5.gz
%{_mandir}/man5/capisuite.conf.5.gz
%{_mandir}/man5/fax.conf.5.gz
%{_mandir}/man8/capisuite.8.gz

%changelog -n capisuite
* Sun Nov 28 2004 - gernot@hillier.de
- update to 0.4.5
* Mon Mar 08 2004 - kkeil@suse.de
- update to 0.4.4
* Sun Jan 11 2004 - kkeil@suse.de
- fix install -o (typo)
* Sun Jan 11 2004 - adrian@suse.de
- add %%defattr
* Tue Sep 02 2003 - ro@suse.de
- fix specfile typo
* Mon Sep 01 2003 - kkeil@suse.de
- fix automatic voice-->fax switching (#29703)
* Fri Aug 29 2003 - mcihar@suse.cz
- require same python version as it was built with
* Mon Jul 28 2003 - kkeil@suse.de
- update to 0.4.3
  * support color fax reception
  * fax headline is now converted to the right charset internally
  * scripts: capisuitefax now handles filenames and dialstrings
  with special chars (spaces etc.) correctly
  * lot of fixes; see file NEWS in the package doc dir
* Thu Jun 12 2003 - kkeil@suse.de
- fix filelist
* Tue Apr 08 2003 - kkeil@suse.de
- update to 0.4.1a because bug 26169 and 25838
  * fixed a bug in the rc-script of CapiSuite which made
  "capisuite start" unfunctional in 0.4.1 (path of executable changed)
  * included tests and changes for gcc-2.95, no patch necessary
  any more to build CapiSuite with it
  * CapiSuite now checks controller abilities and respects them
  when connecting (e.g. use extended fax protocols if available)
-> this also works around a severe crash with AVM drivers when
  transmitting a fax to/from some analog devices
  * fixed SECURITY bug in cronjob which allowed every CapiSuite user
  to become root. Sorry... :-((
* Wed Apr 02 2003 - ro@suse.de
- use wildcard in filelist for python-dir
* Thu Mar 20 2003 - ghillie@suse.de
- updated to 0.4.1, thrown away all patches which are already
  included in this release
* Thu Mar 13 2003 - ghillie@suse.de
- SECURITY FIX: permissions of saved documents and waves are 0600
  now instead of 0644 and dirs have 0700 instead of 0755. This
  fixes critical bug #25242. Bug severity was approved by kkeil.
* Mon Mar 10 2003 - ghillie@suse.de
- added current documentation as .tar.bz2 to avoid change of
  Makefile so late (old Makefiles won't install new docs correctly)
* Mon Mar 03 2003 - ghillie@suse.de
- added capisuite-faxid.diff: fixes sending of fax station ID, fax
  headline works now
* Sun Feb 23 2003 - ghillie@suse.de
- added capisuite-cron.diff: cron-script errors to /dev/null
* Fri Feb 21 2003 - ghillie@suse.de
- capisuite-removesetuid.diff: fixes Bugzilla 23732 (freeze because
  of usage of setuid() which isn't allowed in threads)
- capisuite-cosmetical.diff: cosmetical fixes (examples in conf files
  were wrong, removed debug output)
- added tiff & ghostscript-library to Requires: fixes Bug 23962
* Tue Feb 18 2003 - ghillie@suse.de
- fixed Bugzilla 23731 (lock files weren't deleted in idle.py)
* Mon Feb 17 2003 - ghillie@suse.de
- updated to 0.4 (new/old messages in remote inquiry, capisuitefax
  can show sendqueue and abort jobs)
* Tue Feb 11 2003 - ghillie@suse.de
- included cron job for cleaning up
- rc.capisuite was moved into tar ball
* Mon Feb 10 2003 - ghillie@suse.de
- updated to 0.3.2 (got rid of CommonC++, using native pthreads now,
  fixed some major bugs)
* Mon Feb 03 2003 - ghillie@suse.de
- updated to 0.3.1: (bugfixes, use different sendqueue for each user,
  script improvements, e.g. sayNumber supports 0-99 well now)
* Wed Jan 29 2003 - ghillie@suse.de
- added sox to Require:
* Wed Jan 29 2003 - ghillie@suse.de
- don't start if no user configured for default scripts
- added insserv to %%post (spec file)
* Mon Jan 27 2003 - ghillie@suse.de
- included startup script
* Mon Jan 27 2003 - ghillie@suse.de
- updated to 0.3 (split configuration files into fax and
  answering machine config)
* Thu Jan 23 2003 - ghillie@suse.de
- updated to 0.2.1 (mainly documentation improvements, has an own
  manual now)
* Mon Jan 20 2003 - ghillie@suse.de
- updated to 0.2 (see included NEWS for changes)
- added correct docdir to configure call in specfile
* Mon Dec 16 2002 - gernot@hillier.de
- first package
* Mon Dec 16 2002 - gernot@hillier.de
- fixed 2 small bugs (physical disconnect was missing in some cases,
  file delete in remote inquiry didn't work)
