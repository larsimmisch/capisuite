"""capisuite.fileutils

File handling utility function.

"""

__author__    = "Hartmut Goebel <h.goebel@crazy-compilers.com>"
__copyright__ = "Copyright (c) 2004 by Hartmut Goebel"
__version__   = "$Revision: 0.0 $"
__credits__   = "This file is part of www.capisuite.de; some ideas taken from Gernot Hillier"
__license__ = """
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
"""

import fcntl, os, re, errno
from types import IntType

import capisuite.core as core

class UnknownUserError(KeyError): pass
class LockTakenError(Exception): pass


def lockname(path):
    return "%s.lock" % os.path.splitext(path)[0]
def controlname(path):
    return "%s.txt" % os.path.splitext(path)[0]


def _getLock(name, forfile=None, blocking=0):
    if forfile:
        name = lockname(forfile)
    elif not name:
        raise ValueError, "missing lock name"
    lockfile = open(name, "w")
    try:
        if blocking:
            fcntl.lockf(lockfile, fcntl.LOCK_EX)
        else:
            fcntl.lockf(lockfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except IOError, err: # can't get the lock
        if err.errno in (errno.EACCES, errno.EAGAIN):
            lockfile.close()
            raise LockTakenError
        else:
            raise
    # currently log is only available if running within capisuite
    if hasattr(core, 'log'):
        core.log("lock taken %s" % name, 3)
    return (name, lockfile)


def _releaseLock((lockname, lockfile)):
    fcntl.lockf(lockfile, fcntl.LOCK_UN)
    lockfile.close()
    try:
        os.unlink(lockname)
    except OSError, err:
        # as we don't hold the lock any more, the other thread can be quicker
        # in deleting than we; this doesn't harm, so ignore it
        if (err.errno!=2): 
            raise
    # currently log is only available if running within capisuite
    if hasattr(core, 'log'):
        core.log("lock released %s" % lockname, 3)


def _setProtection(user, mode=0600, *files):
    import pwd
    try:
        userdata = pwd.getpwnam(user)
    except KeyError:
        raise UnknownUserError(user)
    uid = os.getuid()
    if uid == 0:
        # running as root, change ownership
        uid = userdata.pw_uid
    for f in files:
        os.chmod(f, mode)
        os.chown(f, uid, userdata.pw_gid)


def _mkuserdir(user, parrentdir, *dirs):
    import pwd
    try:
        userdata = pwd.getpwnam(user)
    except KeyError:
        raise UnknownUserError(user)
    path = parrentdir
    for d in dirs:
       path = os.path.join(path, d)
       # todo: use os.path.exists?
       if not os.access(path, os.F_OK):
           os.mkdir(path, 0700)
           os.chown(path, userdata.pw_uid, userdata.pw_gid)
    return path

###--- counter files ---###

def readCounter(default=0, *fileparts):
    assert isinstance(default, IntType)
    filename = os.path.join(*fileparts)
    if os.path.exists(filename):
        lastfile = open(filename, "r")
        count = int(lastfile.readline())
        lastfile.close()
        return count
    else:
        return default

def writeCounter(count, *fileparts):
    assert isinstance(count, IntType)
    filename = os.path.join(*fileparts)
    lastfile = open(filename, "w")
    print >> lastfile, count
    lastfile.close()


###--- ... ---###

def __makeCountedFilePattern(basename, suffix):
    # todo: group should be digits only
    return re.compile("%s-([0-9]+)\.%s" % (re.escape(basename),
                                           re.escape(suffix)))

# @brief thread-safe creation of a unique filename in a directory
#
# This function reads the nextnumber from then "nextnr"-file in the given
# directory and updates it. It holds the next free file number.
#
# If nextnr doesn't exist, it's created.
#
# The filenames created will have the format
#
# basename-number.suffix
#
# @param directory name of the directory to work in
# @param basename the basename of the filename
# @param suffix the suffix of the filename (without ".")
#
# @return job number, new file name
def uniqueName(directory, basename, suffix):
    nextfile = "%s-nextnr" % os.path.join(directory, basename)
    lock = _getLock(os.path.join(directory,"cs_lock"), blocking=1)
    try:
        nextnum = readCounter(-10, nextfile)
        if nextnum < 0:
            # search for next free sequence number
            pattern = __makeCountedFilePattern(basename, suffix)
            numbers = [0]
            for f in os.listdir(directory):
                m = pattern.match(f)
                if m:
                    numbers.append(int(m.group(1)))
            # take number of last file and increase it by one
            nextnum = max(numbers)+1 
        writeCounter(nextnum+1, nextfile)
    except:
        _releaseLock(lock)
        raise
    else:
        _releaseLock(lock)
    newname = "%s-%03i.%s" % (os.path.join(directory,basename),
                              nextnum, suffix)
    return nextnum, newname
