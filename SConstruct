# -*- python -*-
"""
Main SCons build script for CapiSuite

(c) Copyright 2004 by Hartmut Goebel <h.goebel@crazy-compilers.com>

CapiSuite is (c) Copyright by Gernot Hiller <gernot@hiller.de>

Use 'scons --help' for a list of available options.

Options have only to be given once, since they are cached in a file
'options.cache'. To change an option, simply pass it again with a
different value.

Example:

  scons prefix=/     # build for prefix=/
  scons              # prefix=/ is taken from 'options.cache' 
  scons prefix=/usr/local  # build for prefix=/usr/local
                     # since this is the default, the entry in options.cache
                     # will be renmoved
  scons              # default prefix is used
  
WARNING: This build method is not officially supported for CapiSuite. Please
use the usual configure;make;make install triplet instead!
"""

__targets__ = """
Additional targets:
  configure : build the 'configure' script
               (this is automatically done if 'config.h' is missing)
  pycheck    : check Python sources with PyChecker (not yet implemented)

  install    : install all files
  install-pylib   : install only the python library
  install-scripts : install only the python scripts
  install-exec    : install only the executables

  For all install-targets base may be set with INSTALL_BASE=...
"""


# File-Content Substitution will (hopefully ) be part of SCons 0.95
def _file_subst(target, source, env):
    import os, re
    import SCons.Util

    def _substitute(matchobj, env=env):
        sym = matchobj.group(1)
        try:
            return env.subst(str(env[sym]))
        except: # TypeError: # sym not a string
            print 'Not substituting', sym
            return matchobj.group(0) # matched 

    delim = re.escape(env.get('SUBST_DELIM', '@'))
    subst_pattern = re.compile('%s(.*?)%s' % (delim, delim))
    for t, s in zip(target, source):
        t = str(t)
        s = s.rstr()
        text = open(s, 'rb').read()
        text = subst_pattern.sub(_substitute, text)
        open(t, 'wb').write(text)
        os.chmod(t, os.stat(s)[0])
    return None

def _fs_strfunc(target, source, env):
    return "generating '%s' from '%s'" % (target[0], source[0])

_fs_builder = Builder(action = Action(_file_subst, strfunction = _fs_strfunc))

import sys, os, os.path
import SCons.Util
import SCons.Node.FS

EnsurePythonVersion(2,2) # capisuite requires this
#EnsureSConsVersion(0,94)

build_dir = Dir('#/build')

class InstallableEnv(Environment):
    def Install(self, dir, source):
        """Install specified files in the given directory."""
        if self.has_key('INSTALL_BASE'):
            def _Dir(name):
                return SCons.Node.FS.Dir(name, parent, self.fs)
            parent = self['INSTALL_BASE']
            dir = self.arg2nodes(dir, _Dir)
        return Environment.Install(self, dir, source)

    def InstallAs(self, target, source):
        """Install sources as targets."""
        def _File(name):
            return SCons.Node.FS.File(name, dir, self.fs)
            
        if self.has_key('INSTALL_BASE'):
            dir = self['INSTALL_BASE']
            targets = self.arg2nodes(target, _File)
        return Environment.InstallAs(self, target, source)

env = InstallableEnv()
env.Append(
    BUILDERS={'FileSubst' : _fs_builder},
    PACKAGE    = 'capisuite',
    VERSION    = '0.5.cvs',
    srcdir     = build_dir,

    pkgdatadir    = '${datadir}/${PACKAGE}',
    pkglibdir     = '${libdir}/${PACKAGE}',
    pkgincludedir = '${includedir}/${PACKAGE}',

    pkgbindir     = '${bindir}',
    pkgsbindir    = '${sbindir}',
    pkgsysconfdir = '${sysconfdir}/${PACKAGE}',
    #pkglibdir     = '${prefix}/lib',
    spooldir      = '${localstatedir}/spool/${PACKAGE}',
    docdir        = '${pkgdatadir}/doc/${PACKAGE}',
    )
env.SConscript('SConscript-Options', exports=['env', '__targets__'])

if env.has_key('INSTALL_BASE'):
    env.Replace(INSTALL_BASE=env.Dir('$INSTALL_BASE'))
Export('env')

env.BuildDir(build_dir=build_dir, src_dir='.')


###---####---###---####---###---####---###---####---###---####---###---###
# call configure if required

# if config.h does not exist, build it using Scons' conftest
if not os.path.exists(str(File('config.h', build_dir))) \
   or 'configure' in COMMAND_LINE_TARGETS:
    env.SConscript('SConscript-Config', build_dir=build_dir)

###---####---###---####---###---####---###---####---###---####---###---###

def GetPythonModuleSetup(env):
    """
    Get configuration for linking with the Python library.
    """
    import distutils.sysconfig
    from distutils.sysconfig import get_python_lib, get_config_vars, \
         get_config_var
    env.Append(
        python_version       = distutils.sysconfig.get_python_version(),
        python_prefix        = distutils.sysconfig.PREFIX,
        python_execprefix    = distutils.sysconfig.EXEC_PREFIX,
        python_libdir        = get_python_lib(plat_specific=1, standard_lib=1),
        python_moduledir     = get_python_lib(plat_specific=0, standard_lib=0),
        python_moduleexecdir = get_python_lib(plat_specific=1, standard_lib=0),
        python_includespec   = get_config_vars('INCLUDEPY', 'CONFINCLUDEPY'),
        python_linkforshared = get_config_var('LINKFORSHARED'),

        pkgpython_moduledir     = '${python_moduledir}/${PACKAGE}',
        pkgpython_moduleexecdir = '${python_moduleexecdir}/${PACKAGE}',

        LINKFLAGS = ['${python_linkforshared}'],
        LIBS      = ['python${python_version}'],
        )
    env.Append(
        CPPPATH   = env['python_includespec'],
        PYTHON    = sys.executable,
        )
    
def GetPythonEmbeddedSetup(env):
    """
    Get configuration for linking an embedded Python application.
    """
    # Base idea on which variable to check are from the INN 2.3.1 package
    import distutils.sysconfig
    print 'Checking how to link an embedded Python application:',
    libvars = Split("LIBS LIBC LIBM LOCALMODLIBS BASEMODLIBS")
    python_libspec = []
    for libvar in distutils.sysconfig.get_config_vars(*libvars):
        libvar = libvar.split()
        for lib in libvar:
            # sanity check
            if not lib.startswith('-l'):
                print "Error in Python Makefile: shared lib spec does ", \
                      "not start with '-l':", lib
            python_libspec.append(lib[2:])
    env.Append(python_libspec   = python_libspec,
               python_configdir = distutils.sysconfig.get_config_var('LIBPL'),
               # preferable these should be '$python..', but SCons
               # doe not yet support this (SCons 0.94)
               LIBS    = python_libspec,
               LIBPATH = ['${python_configdir}'],
               )
    print ' '.join(python_libspec)
    #print '>>>', env['LIBS']

GetPythonModuleSetup(env)
GetPythonEmbeddedSetup(env)

env.Append(
    CCFLAGS = Split('-g -O2'),
    LIBS      = Split('pthread capi20'),
    CPPDEFINES={'LOCALSTATEDIR': r'\"${localstatedir}\"',
                'PKGDATADIR'   : r'\"${pkgdatadir}\"',
                'PKGSYSCONFDIR': r'\"${pkgsysconfdir}\"',
                'PKGLIBDIR'    : r'\"${pkglibdir}\"',
                },
    )

## # build debug?
## if ARGUMENTS.get('debug', 0):
##     env.Append(CXXFLAGS = ['-g', '-DDEBUG'])
## else:
##     env.Append(CXXFLAGS = ['-O2'])
##
## if env['CXX'] in ('g++', 'c++'):
##     env.Append(CXXFLAGS = ['-Wall', '-Wno-non-virtual-dtor'])


###---####---###---####---###---####---###---####---###---####---###---###

# snippet for unittest
#mytest = env.program(...)
#Alias( 'unittest', mytest )
#Alias( 'all', 'unittest' )
#unittetsInstall = env.Install(...)
#Alias('unittest', unitteststInstall)

# now build the subdirectories' stuff
env.SConscript(dirs=[Dir('.', build_dir),
                     Dir('src', build_dir),
                     Dir('scripts', build_dir),
                     Dir('scripts/waves', build_dir),
                     Dir('docs', build_dir),
                     ])

#env.SourceCode('.',
#      env.CVS('pserver:anonymous@cvs.capisuite.berlios.de:/cvsroot/capisuite',
#              'capisuite'))

"""
make DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT/usr/sbin

ln -sf ../../etc/init.d/capisuite $RPM_BUILD_ROOT/usr/sbin/rccapisuite
"""
#EXTRA_DIST = rc.capisuite.in capisuite.cronin cronjob.conf
#install-data-local:
#	mkdir -p $(DESTDIR)$(localstatedir)/log
#	$(mkinstalldirs) $(DESTDIR)$(spooldir)/sendq
#	$(mkinstalldirs) $(DESTDIR)$(spooldir)/done
#	$(mkinstalldirs) $(DESTDIR)$(spooldir)/failed
#	$(mkinstalldirs) $(DESTDIR)$(spooldir)/users
