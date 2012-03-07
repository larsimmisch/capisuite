CapiSuite 
=========

http://www.capisuite.org/

CapiSuite is an ISDN telecommunication suite providing easy to use
telecommunication functions which can be controlled from Python
scripts. Currently, mainly voice functions and fax sending/receiving
are supported.

It uses a CAPI-compatible driver for accessing the ISDN-hardware, so
you'll need an Eicon or AVM card or any mISDN supported adapter with the 
according driver.

CapiSuite is distributed with scripts providing a multi-user answering
machine with automatic fax recognition, remote inquiry and fax
send/receive functions.

For further information, please have a look at the manual section
available online or in the download packages. Please test it and post
your feedback to the bug tracker and/or the mailing lists. Thank you!

For the documentation see the created HTML documents situated in
docs/manual/index.html or in the installed version see
$PREFIX/share/doc/capisuite/manual/index.html. There's also a german
translation of the manual available at http://www.capisuite.org/.


Building capisuite from git
---------------------------


	git clone https://github.com/larsimmisch/capisuite.git
	cd capisuite
	./configure ...options...
	make
	sudo make install

Please see Makefile.git for other targets which may be useful for
core-developers.

Side notes
----------

This git repository was created from the svn dumps that Gernot Hillier
kindly provided on http://www.capisuite.org/ and contains his and his 
collaborators work.

All credit should go to the original team. I (larsimmisch) have only made 
minor bugfixes, in particular small changes that make capisuite compatible 
with mISDN (the socket branches from http://git.misdn.eu/).
