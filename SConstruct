# -*- mode: python ; coding: latin-1 -*-
#
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
"""
Main SCons build script for CapiSuite

(c) Copyright 2004 by Hartmut Goebel <h.goebel@crazy-compilers.com>

CapiSuite is (c) Copyright by Gernot Hillier <gernot@hillier.de>

Use 'scons --help' for a list of available options.

Options have only to be given once, since they are cached in a file
'options.cache'. To change an option, simply pass it again with a
different value.

Example:

  scons prefix=/     # build for prefix=/
  scons              # prefix=/ is taken from 'options.cache' 
  scons prefix=/usr/local  # build for prefix=/usr/local
                     # since this is the default, the entry in
                     # options.cache will be removed
  scons              # default prefix is used
  
WARNING: This build method is not yet officially supported for
CapiSuite. Please use the usual 'configure; make; make install'
triplet instead!
"""

__targets__ = """
Additional targets:
  configure : build the 'configure' script
              (this is automatically done if 'config.h' is missing)
  pycheck    : check Python sources with PyChecker (not yet implemented)

  install         : install all files
  install-pylib   : install only the python library
  install-scripts : install only the python scripts
  install-exec    : install only the executables
  install-man     : install only the man pages
  install-data-local: create spool- and state-dirs

  For all 'install'-targets the installation base may be set with
  INSTALL_BASE=...

  dist      : build distribution archive (.tar.gz)
  distcheck : test whether distribution archive is slef-contained
  rpms      : build source and binay rpms

"""
## install-strip: strip the _installed_ binaries
## all == .
## clean == -c: clean all file that could be made by building
## distclean: clean all file that are made by configuring and building
##            config.h, .sconf.temp, ...
## uninstall == -c install
## mostlyclean: läßt selten gebaute File stehen
## maintainer-clean: clean + bison-output, info-files, tag tables, etc.
## check: perform self-test
##
## TAGS, info, dvi,
## installcheck, installdirs

###---####---###---####---###---####---###---####---###---####---###---###

# some Pythonic setup
import sys, os, os.path, glob
import SCons.Util, SCons.Script
import SCons.Node.FS

# Store signatures in ".sconsign.dbmlite" in the top-level directory.
SConsignFile()

class InstallableEnv(Environment):
    """Extended Environment which supports INSTALL_BASE and ExtraDist."""
    def BasedFile(self, name):
        if self.get('INSTALL_BASE'): name = ('$INSTALL_BASE/%s' % name)
        return self.File(name)
    def BasedDir(self, name):
        if self.get('INSTALL_BASE'): name = ('$INSTALL_BASE/%s' % name)
        return self.Dir(name)

    def Install(self, dir, source):
        """Install specified files in the given directory."""
        dir = self.arg2nodes(dir, self.BasedDir)
        files = Environment.Install(self, dir, source)
        self.DistSourcesOf(files)
        return files

    def InstallAs(self, target, source):
        """Install sources as targets."""
        target = self.arg2nodes(target, self.BasedFile)
        files = Environment.InstallAs(self, target, source)
        self.DistSourcesOf(files)
        return files

    def DistSourcesOf(self, files):
        """
        Add all sourcefiles of the given files to the distribution archive.
        """
        files = self.arg2nodes(files, self.File)
        files = map(self.DistSourceOfFile, files)
        #self.DistTar('$DIST_ARCHIVE', files)
        self.Append(DIST_FILES=files)
        return files

    def ExtraDist(self, files):
        """Additional files to be distributed."""
        files = self.arg2nodes(files, self.File)
        #self.DistTar('$DIST_ARCHIVE', files)
        self.Append(DIST_FILES=files)
        return files

    _valid_man_extensions = '0123456789ln' # from info automake

    def mandir(self, section):
        section = str(section)
        assert section in self._valid_man_extensions, section # todo: error msg
        return os.path.join('$mandir', 'man%s' % section)

    def InstallMan(self, source, section=None):
        """
        Install man pages.
        If 'section' is given, file extensions are changed to 'section'.
        Otherwise, the section is taken from file extension.
        """
        source = self.arg2nodes(source, self.File)
        result = []
        for man in source:
            base = os.path.basename(man.name)
            base, ext = os.path.splitext(base)
            sect = section
            if sect is None:
                sect = ext[1] # determine section from name/extension
            else:
                ext = '.%s' % sect # overwrite extension
            res = self.InstallAs(os.path.join(self.mandir(sect), base + ext),
                                 man)
            result.extend(res)
        return result

#
# Support functions for linking with Python
#

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

def Get_sfftobmp_Version(context):
    """
    Test for the sfftobmp version installed as different versions
    need different parameters. :-(
    """
    import commands, re
    print 'Checking for sfftobmp version ...',
    status, text = commands.getstatusoutput('sfftobmp -v')
    if status:
        print 'failed'
        print text
        Exit(1)
    res = re.search(r'Version ([0-9,.]+)', text, re.M) or 0
    if not res:
        print 'failed'
        print
        print '** It seams like the output of sfftobmp has changed. Please'
        print '** contact the authors of capisuite to fix this.'
        Exit(10)
    res = res.group(1)
    env.Replace(sfftobmp_major_version = res.split('.')[0])
    print res


# A Shortcut
is_dist = ('dist' in COMMAND_LINE_TARGETS or
           'distcheck' in COMMAND_LINE_TARGETS or
           'rpms' in COMMAND_LINE_TARGETS)

###---####---###---####---###---####---###---####---###---####---###---###

# capisuite requires Python 2.2
EnsurePythonVersion(2,2)
if is_dist:
    # 'dist' target requires module tarfile which is new in Python 2.3
    EnsurePythonVersion(2,3) 
EnsureSConsVersion(0,96)

###---####---###---####---###---####---###---####---###---####---###---###
#
# Setting up build-environment
#

# Build all files in this subdir
build_dir = Dir('#/build')

env = InstallableEnv(tools=Split('default sourcetar filesubst'),
                     toolpath=['scons-tools'],

    # set some build variables
    PACKAGE    = 'capisuite',
    VERSION    = '0.5.cvs',
    RELEASE    = '1',

    SVNREPOSITORY = 'https://h3281.serverkompetenz.net/repos/capisuite/',

    # required for some building the docs (doxygen)
    srcdir     = build_dir, 

    # default pathes
    pkgdatadir    = '${datadir}/${PACKAGE}',
    pkglibdir     = '${libdir}/${PACKAGE}',
    pkgincludedir = '${includedir}/${PACKAGE}',
    pkgbindir     = '${bindir}',
    pkgsbindir    = '${sbindir}',
    pkgsysconfdir = '${sysconfdir}/${PACKAGE}',
    #pkglibdir     = '${prefix}/lib',
    spooldir      = '${localstatedir}/spool/${PACKAGE}',
    docdir        = '${pkgdatadir}/doc/${PACKAGE}',

    # Distribution of files and sources:
    DIST_ARCHIVE  = '#/dist/${PACKAGE}-${VERSION}.tar.gz',
    DIST_FILES    = [],

    # used for distcheck
    SCONS         = 'scons',
    DISTCHECK_DIR = Dir('#/dist/check'),
    )

###---####---###---####---###---####---###---####---###---####---###---###
#
# Handling of command line options
#

# Some commonly used make targets have to be done another way.
# Inform the used if he tries to use such a target:
for ct, t in (
    ('clean', '-c'),
    ('uninstall', '-c install'),
    ('distclean', '-c dist'),
    ):
    if ct in COMMAND_LINE_TARGETS:
        print 
        print "Please use 'scons %s' instead of the pseudo-target '%s'." \
              % (t, ct)
        print
        Exit(1)

# Handle options
env.SConscript('SConscript-Options', exports=['env', '__targets__'])

### Some more setup
env.BuildDir(build_dir=build_dir, src_dir='.')

Export('env', 'is_dist')

###---####---###---####---###---####---###---####---###---####---###---###
#
# Configure build (only if neither cleaning nor 'dist'-ing)
# 
if not GetOption('clean') and not is_dist:
    # "configure" only if necessary or requested
    if not File('config.h', build_dir).exists() \
           or 'configure' in COMMAND_LINE_TARGETS:
        env.SConscript('SConscript-Config', build_dir=build_dir)

    GetPythonEmbeddedSetup(env)
    Get_sfftobmp_Version(env)

# get some build variables we always need to evaluate,
# even for non-building targets.
GetPythonModuleSetup(env)

###---####---###---####---###---####---###---####---###---####---###---###

env.Append(
    CCFLAGS = Split('-g -O2'),

    # these should go into src/SConscript:
    LIBS      = Split('pthread capi20'),
    CPPDEFINES={'LOCALSTATEDIR': r'\"${localstatedir}\"',
                'PKGDATADIR'   : r'\"${pkgdatadir}\"',
                'PKGSYSCONFDIR': r'\"${pkgsysconfdir}\"',
                'PKGLIBDIR'    : r'\"${pkglibdir}\"',
                'HAVE_CONFIG_H': 1, # we always have config.h
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
#
# Build everything defined in the 'Sconscript' files.
# 
env.SConscript(dirs=[Dir('.', build_dir),
                     Dir('src', build_dir),
                     Dir('scripts', build_dir),
                     Dir('scripts/waves', build_dir),
                     Dir('docs', build_dir),
                     Dir('packages/rpm'),
                     ])

#--- additional files to be distributed ---
env.ExtraDist(Split("""
    ChangeLog.2003 ChangeLog.2004
    TODO
    SConstruct
    SConscript-Config
    SConscript-Options
    scons-tools/sourcetar.py
    scons-tools/filesubst.py
    scons-tools/textfile.py

    SConscript
    src/SConscript
    scripts/SConscript
    scripts/waves/SConscript
    packages/rpm/SConscript
    docs/SConscript
    """))
# ChangeLog and ChangeLog.complete are build in SConscript


#--- additional files to be distributed (automake/autoconf stuff) ---
# only add these files if they exist
for f in Split("""
    INSTALL
    acinclude.m4
    aclocal.m4
    config.h.in
    configure
    configure.in
    depcomp
    install-sh
    missing
    mkinstalldirs
    Makefile.am
    Makefile.in
    docs/Makefile.am
    docs/Makefile.in
    scripts/Makefile.am
    scripts/Makefile.in
    scripts/waves/Makefile.am
    scripts/waves/Makefile.in
    src/Makefile.am
    src/Makefile.in
    src/application/Makefile.am
    src/application/Makefile.in
    src/backend/Makefile.am
    src/backend/Makefile.in
    src/capisuite-py/Makefile.am
    src/capisuite-py/Makefile.in
    src/modules/Makefile.am
    src/modules/Makefile.in
    """):
    if os.path.exists(f):
        env.ExtraDist(f)

#env.SourceCode('.',
#      env.CVS('pserver:anonymous@cvs.capisuite.berlios.de:/cvsroot/capisuite',
#              'capisuite'))

"""
make DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT/usr/sbin

ln -sf ../../etc/init.d/capisuite $RPM_BUILD_ROOT/usr/sbin/rccapisuite
"""

###---####---###---####---###---####---###---####---###---####---###---###
#
# Install targets
#

for d in (
    '${localstatedir}/log',
    '${spooldir}/sendq',
    '${spooldir}/done',
    '${spooldir}/failed',
    '${spooldir}/users',
    ):
    d = env.BasedDir(d)
    if not d.exists():
        d = env.Command(d, None, Mkdir('$TARGET'))
        env.Alias('install-data-local', d)

# 'install' includes the other parts, too
env.Alias('install', env.Alias('install-pylib'))
env.Alias('install', env.Alias('install-scripts'))
env.Alias('install', env.Alias('install-exec'))
env.Alias('install', env.Alias('install-data-local'))
env.Alias('install', env.Alias('install-man'))

###---####---###---####---###---####---###---####---###---####---###---###
#
# 'Dist' target
#
if is_dist or 'rpms' in COMMAND_LINE_TARGETS:
    dist = env.DistTar('$DIST_ARCHIVE', env['DIST_FILES'])[0]
    env.Alias('dist', dist)

###---####---###---####---###---####---###---####---###---####---###---###
#
# 'Dist' target
#
if 'distcheck' in COMMAND_LINE_TARGETS:
    distcheck = env.Command('distcheck',
                env.Dir('$DISTCHECK_DIR/${PACKAGE}-${VERSION}'),
                [Delete('$SOURCE'),
                 Mkdir('$SOURCE'),
                 '$TAR xz -C ${SOURCE.dir} -f ' + dist.abspath,
                 '$SCONS -C $SOURCE .',
                 '$SCONS -C $SOURCE dist',
                 'echo ; echo checkdist passed ; echo'
                 ])
    env.Clean(distcheck, env['DISTCHECK_DIR'])
    env.Depends(distcheck, dist)

###---####---###---####---###---####---###---####---###---####---###---###
#
# 'rpm' target
#
if 'rpms' in COMMAND_LINE_TARGETS:
    _builddir = build_dir
    _distdir = dist.dir
    _arch = 'i586'
    _rpmbasename = env.subst('capisuite-$VERSION-$RELEASE')
    env.Alias('rpms', 
      env.Command([File('%s.%s.rpm' % (_rpmbasename, _arch), _distdir),
                   File('%s.%s.rpm' % (_rpmbasename, 'src'), _distdir)],
                  ['packages/rpm/capisuite-mdk-9.2.spec', dist], [ \
                  ['rpmbuild',
                   '--define', '_builddir %s'  % _builddir.abspath,
                   '--define', '_sourcedir %s' % _distdir.abspath,
                   '--define', '_srcrpmdir %s' % _distdir.abspath,
                   '--define', '_rpmdir %s'    % _distdir.abspath,
                   '--define', '_rpmfilename $TARGET.name',
                   '-ba', '$SOURCE'],
                   ]))
