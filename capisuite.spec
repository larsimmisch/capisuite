#
# spec file for package capisuite 
#
# Author: Gernot Hillier <gernot@hillier.de>
#
# This spec file was developed for the use with SuSE Linux.  But it
# should also work for any other distribution with slight changes.
# If you created your own RPM, please tell me and I'll happily include
# the spec or a link to your RPM on the homepage.

# neededforbuild  capi4linux gcc-c++ libstdc++-devel libxml2-devel python python-devel

Name:         capisuite
License:      GPL
Group:        Applications/Communications
Autoreqprov:  on
Version:      0.5  
Release:      0
Requires:     sfftobmp sox tiff ghostscript-library
Summary:      ISDN telecommunication suite providing fax and voice services 
Source0:      capisuite-%{version}.tar.gz
Source1:      rc.capisuite
URL:          http://www.capisuite.de 
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
PreReq:       %insserv_prereq

%description
CapiSuite is an ISDN telecommunication suite providing easy to use
telecommunication functions which can be controlled from Python scripts.

It uses a CAPI-compatible driver for accessing the ISDN-hardware, so you'll
need an Eicon or AVM card with the according driver.

CapiSuite is distributed with two example scripts for call incoming handling
and fax sending. See /usr/share/doc/packages/capisuite for further information.

Authors:
--------
    Gernot Hillier

%prep
%setup  
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --with-docdir=/usr/share/doc/packages/capisuite --mandir=/usr/share/man

%build
make 

%install
make DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT/etc/init.d
mkdir -p $RPM_BUILD_ROOT/usr/sbin
install -g root -m 755 -o root %{SOURCE1} $RPM_BUILD_ROOT/etc/init.d/capisuite
ln -sf ../../etc/init.d/capisuite $RPM_BUILD_ROOT/usr/sbin/rccapisuite

%clean
rm -rf $RPM_BUILD_ROOT

%files
%config /etc/capisuite/capisuite.conf
%config /etc/capisuite/fax.conf
%config /etc/capisuite/answering_machine.conf
/usr/sbin/capisuite
/usr/bin/capisuitefax
%doc /usr/share/doc/packages/capisuite
/usr/share/capisuite
/usr/lib/capisuite
/var/spool/capisuite
/usr/%{_lib}/python2.?/site-packages/cs_helpers.py
/etc/init.d/capisuite
/usr/sbin/rccapisuite

