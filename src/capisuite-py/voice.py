"""capisuite.voice

Module for voice interfacing and interactions of capisuite.

"""

__author__    = "Hartmut Goebel <h.goebel@crazy-compilers.com>"
__copyright__ = "Copyright (c) 2004 by Hartmut Goebel"
__version__   = "$Revision: 0.0 $"
__credits__   = "This file is part of www.capisuite.de; thanks to Gernot Hillier"
__license__ = """
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
"""

import os, re, errno

# capisuite stuff
import fileutils
from capisuite.config import JobDescription, createDescriptionFor
from capisuite.exceptions import InvalidJob, JobLockedError
#from capisuite.consts import *

_job_pattern = re.compile("voice-([0-9]+)\.txt")
_la_pattern = re.compile("voice-([0-9]+)\.la")


def _userQ(config, user, Q):
    userdir= config.get('GLOBAL', "voice_user_dir")
    # todo: enable this, iff using config.getUser()
    #if not userdir:
    #    raise NoOptionError('', 'fax_user_dir')
    return os.path.abspath(os.path.join(userdir, user, Q))


def getAudio(config, user, filename):
    """
    Search for an audio file first in user_dir, than in audio_dir

    'config' is the ConfigParser object containing the configuration,
    'user' the name of the user, 'filename' the filename of the wave
    file.

    Returns the found file with full path
    """

    userdir = config.get('GLOBAL', "voice_user_dir")
    userdir = os.path.join(userdir, user)
    if config.has_option('GLOBAL', "user_audio_files") and \
       config.getboolean('GLOBAL', "user_audio_files") and \
       os.access(os.path.join(userdir, filename),os.R_OK):
        return os.path.join(userdir, filename) 
    else:
        systemdir = config.get('GLOBAL', "audio_dir")
        return os.path.join(systemdir, filename)


def getNumberFiles(number, gender="-"):
    number = str(number)
    if number == "-" or number == "??":
        # "??" is needed for backward compatibility to versions <= 0.4.1a
        yield 'unbekannt'
    elif gender != "-" and number in ("1", "01"):
        if gender in ("n", "m"):
            yield 'ein'
        else:
            yield 'eine'
    elif len(number) == 2 and number[0] != "0":
        digit10, digit01 = number
        if digit10 == "1" or digit01 == "0":
            # for 10, 11...19, 20, 30, ... we have seperate voice files
            yield number
        else:
            if digit01 == "1":
                yield 'ein'
            else:
                yield digit01
            yield 'und'
            yield '%s0' % digit10
    else:
        for digit in list(number):
            yield digit


# @brief say a german number
#
# All numbers from 0 to 99 are said correctly, while all larger ones are
# split into numbers and only the numbers are said one after another.
# An input of "-" produces the word "unbekannt" (unknown)
#
# @param call reference to the call
# @param number the number to say
# @param curr_user the current user named
# @param config the ConfigParser instance holding the configuration info
# @param gender if the number is used in connection with a singular noun ("f" --> "eine Nachricht")
def sayNumber(config, user, call, number, gender="-"):
    for f in getNumberFiles(number, gender=gender):
        say(config, user, call, "%s.la" % f)

def say(config, user, call, *files):
    for f in files:
        call.audio_send(getAudio(config, user, f),1)


###---- Queue handling ---###

def getInquiryCounter(config, user):
    return fileutils.readCounter(-1, 
                                 _userQ(config, user, "received"),
                                 "last_inquiry")

def setInquiryCounter(config, user, count):
    return fileutils.writeCounter(count,
                                  _userQ(config, user, "received"),
                                  "last_inquiry")


def getQueueFiles(config, user):
    """
    Generated a list of all Queue files, where each entry consists of
    a tuple (job-number, filename).

    filename is an absolute path
    
    """
    receivedQ = _userQ(config, user, "received")
    for filename in os.listdir(receivedQ):
        m = _job_pattern.match(filename)
        if m:
            yield (m.group(1), # job number
                   filename)


def abortJob(controlfile):
    """
    Abort a fax job defined by it's controlfile.

    This will remove the job from respective queue and delete both the
    controlfile and the file defined in the controlfile.
    """
    # todo: security: May this be missused for deleting other users files?
    # todo: security: should ensure controlfile and filename are in the same
    #                 directory
    lockname = fileutils.lockname(controlfile)
    if not os.access(controlfile, os.W_OK):
        raise InvalidJob(None, controlfile)
    try:
        lock = fileutils._getLock(lockname, blocking=0)
    except IOError, err:
        if err.errno in (errno.EACCES, errno.EAGAIN):
            raise JobLockedError(None, controlfile)
        else:
            raise
    else:
        try:
            control = JobDescription(controlfile)
            filename = control.get('filename')
            os.unlink(filename)
            os.unlink(controlfile)
        finally:
            fileutils._releaseLock(lock)


def createReceivedJob(user, filename, call_from, call_to, causes):
    """
    Create a file description for a received fax. The description file
    is written to the same directory as the filename.
    """
    import time
    control = {
        'call_from': call_from,
        'call_to': call_to,
        'date': time.ctime(),
        'cause': "0x%x/0x%x" % causes,
        # we return this dict, thus set the filename here, too
        'filename': filename,
        }
    controlfile = createDescriptionFor(**control)
    fileutils._setProtection(user, 0600, filename, controlfile)
    return control
