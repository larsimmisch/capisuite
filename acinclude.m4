#
# Autoconf macros for configuring the build of Python extension modules
#
# $Header: /root/cvs2svn/capisuite/capisuite/acinclude.m4,v 1.1 2003/02/19 08:19:52 gernot Exp $
#
# taken out of Postgres CVS by Gernot Hillier 
#

# CS_SET_DOCDIR
# -------------
# Set the name of the docdir to the given value. This is not nice, but I 
# found no other name to do it than with AC_ARG_WITH. Please tell me if 
# you have better ideas...
AC_DEFUN(CS_SET_DOCDIR,
[AC_ARG_WITH(docdir,
	     AC_HELP_STRING([--with-docdir=DOCDIR],
			    [use DOCDIR to install documentation to (default is PREFIX/share/doc/capisuite)]),
	docdir=$withval, docdir=$datadir/doc/capisuite)
AC_SUBST(docdir)
])

# PGAC_CHECK_PYTHON_DIRS
# -----------------------
# Determine the name of various directory of a given Python installation.
AC_DEFUN([PGAC_CHECK_PYTHON_DIRS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING([Python installation directories])
python_version=`${PYTHON} -c "import sys; print sys.version[[:3]]"`
python_prefix=`${PYTHON} -c "import sys; print sys.prefix"`
python_execprefix=`${PYTHON} -c "import sys; print sys.exec_prefix"`
python_libdir=`${PYTHON} -c "from distutils import sysconfig; print sysconfig.get_python_lib(1,1)"`
python_configdir="${python_libdir}/config"
python_moduledir="${python_libdir}/site-packages"
python_moduleexecdir="${python_libdir}/site-packages"
python_includespec="-I${python_prefix}/include/python${python_version}"
python_linkforshared=`${PYTHON} -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('LINKFORSHARED')"`
if test "$python_prefix" != "$python_execprefix"; then
  python_includespec="-I${python_execprefix}/include/python${python_version} $python_includespec"
fi

AC_SUBST(python_version)[]dnl
AC_SUBST(python_prefix)[]dnl
AC_SUBST(python_execprefix)[]dnl
AC_SUBST(python_configdir)[]dnl
AC_SUBST(python_moduledir)[]dnl
AC_SUBST(python_moduleexecdir)[]dnl
AC_SUBST(python_includespec)[]dnl
AC_SUBST(python_linkforshared)[]dnl
# This should be enough of a message.
if test "$python_prefix" != "$python_execprefix"; then
  AC_MSG_RESULT([$python_libdir and $python_execprefix])
else
  AC_MSG_RESULT([$python_libdir])
fi
])# _PGAC_CHECK_PYTHON_DIRS


# PGAC_CHECK_PYTHON_MODULE_SETUP
# ------------------------------
# Finds things required to build a Python extension module.
# This used to do more, that's why it's separate.
#
# It would be nice if we could check whether the current setup allows
# the build of the shared module. Future project.
AC_DEFUN([PGAC_CHECK_PYTHON_MODULE_SETUP],
[
  AC_REQUIRE([PGAC_CHECK_PYTHON_DIRS])
])# PGAC_CHECK_PYTHON_MODULE_SETUP


# PGAC_CHECK_PYTHON_EMBED_SETUP
# -----------------------------
# Courtesy of the INN 2.3.1 package...
AC_DEFUN([PGAC_CHECK_PYTHON_EMBED_SETUP],
[AC_REQUIRE([PGAC_CHECK_PYTHON_DIRS])
AC_MSG_CHECKING([how to link an embedded Python application])

if test ! -f "$python_configdir/Makefile"; then
  AC_MSG_RESULT(no)
  AC_MSG_ERROR([Python Makefile not found])
fi

_python_libs=`grep '^LIBS=' $python_configdir/Makefile | sed 's/^.*=//'`
_python_libc=`grep '^LIBC=' $python_configdir/Makefile | sed 's/^.*=//'`
_python_libm=`grep '^LIBM=' $python_configdir/Makefile | sed 's/^.*=//'`
_python_liblocalmod=`grep '^LOCALMODLIBS=' $python_configdir/Makefile | sed 's/^.*=//'`
_python_libbasemod=`grep '^BASEMODLIBS=' $python_configdir/Makefile | sed 's/^.*=//'`

pgac_tab="	" # tab character
python_libspec=`echo X"$_python_libs $_python_libc $_python_libm -lpython$python_version $_python_liblocalmod $_python_libbasemod" | sed -e 's/^X//' -e "s/[[ $pgac_tab]][[ $pgac_tab]]*/ /g"`
LIBS="$LIBS $python_libspec"
LDFLAGS="$LDFLAGS -L$python_configdir $python_linkforshared"
AC_MSG_RESULT([${python_libspec}])

AC_SUBST(LIBS)[]dnl
AC_SUBST(LDFLAGS)
])# PGAC_CHECK_PYTHON_EMBED_SETUP

