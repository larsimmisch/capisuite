"""capisuite.fax

Module for fax interfacing of capisuite.

Most functions deal about creating and manipulationg job control
files. To actually send the fax out, call sendfax(). This is normaly
done by the idle-script.
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

import os, os.path, time, re, errno
from types import ListType, TupleType

# capisuite stuff
import fileutils
from capisuite.config import JobDescription, createDescriptionFor
from capisuite.exceptions import InvalidJob, JobLockedError
from capisuite.consts import *

_job_pattern = re.compile("fax-([0-9]+)\.txt")

###---- Utility functions ---###

def _userQ(config, user, Q):
    userdir= config.get('GLOBAL', "fax_user_dir")
    # todo: enable this, iff using config.getUser()
    #if not userdir:
    #    raise NoOptionError('', 'fax_user_dir')
    return os.path.abspath(os.path.join(userdir, user, Q))

    
###---- Job handling ---###

def _createSendJob(user, filename, **controlinfo):
    """
    Create a control file for a fax to be send. The control file is
    written to the same directory as the filename.

    NB: This function does not care about the control information to
    be written to the controlfile. This is to keep the definition of
    what is required for sending the fax out of here. It's up to the
    application layer to ensure correct parameters.
    """
    controlfile = createDescriptionFor(filename, **controlinfo)
    fileutils._setProtection(user, 0600, filename, controlfile)


def createReceivedJob(user, filename, call_from, call_to, causes, **kwargs):
    """
    Create a file description for a received fax. The description file
    is written to the same directory as the filename.
    """
    controlinfo = {
        'call_from': call_from,
        'call_to': call_to,
        'time': time.ctime(),
        'cause': "0x%x/0x%x" % causes
        }
    controlinfo.update(kwargs)
    controlfile = createDescriptionFor(filename, **controlinfo)
    fileutils._setProtection(user, 0600, filename, controlfile)
    

def enqueueJob(config, user, infiles, converter, **controlinfo):
    # todo: enable color-suffixes
    """
    Enqueue a file into the fax send queue.

    This sets the spool-filename and calls 'converter(infile,
    spoolfile)' to convert the infile into a faxfile. The later has to
    be written tp 'spoolfile'. If the conversion succeeds, the
    appropriate job controlfile is written .

    NB: This function does not care about the control information to
    be written to the controlfile. This is to keep the definition of
    what is required for sending the fax out of here. It's up to the
    application layer to ensure correct parameters.

    @param user
    @param infile
    @param converter a function for converting the infile into a fax file
           (will be called covnerter(infile, faxfile)
    @controlinfo the fax job description
    """
    assert isinstance(infiles, (ListType, TupleType))
    # ensure some required entries (which may have defaults) exist
    if not controlinfo.has_key('tries'):
        controlinfo['tries'] = 0
    if not controlinfo.has_key('starttime'):
        controlinfo['starttime'] = time.ctime(time.time())
    
    sendQ = _userQ(config, user, SEND_Q)
    jobnum, faxname = fileutils.uniqueName(sendQ, "fax", "sff")
    converter(infiles, faxname)
    _createSendJob(user, faxname, **controlinfo)
    return jobnum


def moveJob(controlfile, newdir, user=None):
    # todo: important: update 'filenmae' in job description!
    control = JobDescription(controlfile)
    filename = control.get('filename')
    cname = os.path.split(controlfile)[1]
    fname = os.path.split(filename)[1]
    if not user:
        cname = os.path.join(newdir, cname)
        fname = os.path.join(newdir, fname)
    else:
        cname = os.path.join(newdir, "%s-%s" % (user, cname))
        fname = os.path.join(newdir, "%s-%s" % (user, fname))

    # move controlfile to keep owner and protection!
    os.rename(controlfile, cname)
    os.rename(filename,    fname)
    # update controlfile
    control.set('filename', fname)
    control.write(cname)

    return control


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


def abortUserJob(config, user, jobnum):
    """
    Abort a fax send job defined by username and job number.

    This will remove the job from respective queue and delete both the
    controlfile and the file defined in the controlfile.
    """
    sendQ = _userQ(config, user, SEND_Q)
    controlfile = fileutils.controlname(os.path.join(sendQ,
                                                     "fax-%03i" % jobnum))
    abortJob(controlfile)



###---- Queue handling ---###

def getQueueFiles(config, user):
    """
    Generate a list of all fax jobs in the send queue.

    Result is a list of (job-number, controlfile) tuples, where
    'controlfile' is an absolut paht.
    """
    sendQ = _userQ(config, user, SEND_Q)
    for filename in os.listdir(sendQ):
        m = _job_pattern.match(filename)
        if m:
            yield (int(m.group(1)), # job number
                   os.path.join(sendQ, filename))

def getQueue(config, user):
    """
    Generate a list of all fax entries in the send queue.

    Result is a list of (job-number, description) tuples, where
    'description' is the content of the job's controlfile as a
    dictionary.
    """
    for num, controlfile in getQueueFiles(config, user):
        jobDesc = JobDescription(controlfile).items()
        yield (num, jobDesc)


def getQueueDetails(config, user):
    """
    Return a list of interesting details about the fax jobs in the
    send queue.

    Result is a list of tuples of (job-num, addresse/dialstring,
    tries, starttime, subject), where starttime it the time when the
    next try for sending is undertaken and addresse/dialstring is the
    adresses (if not empty) or the dialstring otherwise).
    """
    jobs = []
    for num, jobDesc in getQueue(config, user):
        jobs.append( (
            num,
            jobDesc['addressee'] or jobDesc['dialstring'],
            jobDesc['tries'],
            jobDesc['starttime'],
            jobDesc['subject'],
            ))
    return jobs



###--- Send/Receive Fax ---###

def sendfax(config, user, capi, faxfile,
            outgoing_num, dialstring, stationID=None, headline=None):
    """
    Send a fax out via the capi.

    Returns a tuple ((result, resultB3), faxinfo)
    """
    import capisuite.core as core

    controller = config.getint('GLOBAL', "send_controller")
    timeout = int(config.getUser(user, "outgoing_timeout"))

    # get defaults for stationID and headline from config
    if not stationID:
        stationID = config.getUser(user, "fax_stationID")
    if not stationID:
        core.error("Warning: fax_stationID for user %s not set" %user)
    if not headline:
        headline = config.getUser(user, "fax_headline")

    try:
        call, result = capi.call_faxG3(controller, outgoing_num,
                                       dialstring, timeout,
                                       stationID, headline)
        core.log('result from capi.call_faxG3: %s' % result, 2)
        if result:
            # an errror occured
            return (result, 0), None
        faxinfo = call.fax_send(faxfile)
        return call.disconnect(), faxinfo
    except core.CallGoneError:
        return call.disconnect(), None
