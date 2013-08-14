# -*- mode: python ; coding: iso_8859_15 -*-
#
#                  idle.py - default script for capisuite
#              ---------------------------------------------
#    copyright            : (C) 2002 by Gernot Hillier
#    email                : gernot@hillier.de
#    version              : $Revision: 1.13 $
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#

import os, time, pwd, fcntl

# capisuite stuff
#import capisuite
import capisuite.fax
from capisuite.config import NoOptionError
import capisuite.core as core

# todo: eliminate this
import capisuite.helpers as helpers

# sendfax is now imported from capisuite.fax

from capisuite.fileutils import _releaseLock, _getLock, LockTakenError

def idle(capi):

    capi = capisuite.core.Capi(capi)
    config = capisuite.config.readGlobalConfig()
    try:
        spool = config.get('GLOBAL', "spool_dir")
        max_tries = config.getint('GLOBAL', "send_tries")
        delays = config.getList('GLOBAL', "send_delays")
    except NoOptionError, err:
        core.error("global option %s not found." % err.option)
        return

    # todo: implement config.getQueue(queue,user=None)
    doneQ = os.path.join(spool, "done")
    failedQ = os.path.join(spool, "failed")

    if not os.access(doneQ, os.W_OK) or not os.access(failedQ, os.W_OK):
        core.error("Can't read/write to the necessary spool dirs below %s" % spool)
        return

    # search in all user-specified sendq's
    for user in config.listUsers():
        outgoing_num = config.getUser(user, "outgoing_msn")
        if not outgoing_num:
            incoming_nums = config.getUser(user, "fax_numbers")
            if not incoming_nums:
                continue
            outgoing_num = incoming_nums.split(',')[0].strip()

        mailaddress = config.getUser(user, "fax_email", user)
        fromaddress = config.getUser(user, "fax_email_from", user)

        for jobnum, controlfile in capisuite.fax.getQueueFiles(config, user):
            core.log("checking job %s %s" % (jobnum, controlfile), 3)
            assert controlfile == os.path.abspath(controlfile)
            try:
                # lock the job so that it isn't deleted while sending
                lock = _getLock('dummy', forfile=controlfile, blocking=0)
            except LockTakenError:
                # if we didn't get the lock, continue with next job
                continue
            try:
                control = capisuite.config.JobDescription(controlfile)

                fax_file = control.get('filename')
                assert fax_file == os.path.abspath(fax_file)

                # both the job control file and the fax file must have
                # the users uid
                uid = pwd.getpwnam(user).pw_uid
                try:
                    if os.stat(controlfile).st_uid != uid or \
                       os.stat(fax_file).st_uid != uid:
                        core.error("job %s seems to be manipulated "
                                   "(wrong uid)! Ignoring..." % controlfile)
                        #_releaseLock(lock)
                        continue
                except OSError, e:
                    core.error("job %s seems to be manipulated! "
                               "%s Ignoring..." % (controlfile, e))
                    #_releaseLock(lock)
                    continue

                # todo: describe what is tested here
                # perhaps it was cancelled?
                if not os.access(controlfile, os.W_OK):
                    #_releaseLock(lock)
                    continue

                # set DST value to -1 (unknown), as strptime sets it wrong
                # for some reason
                starttime = time.strptime(control.get("starttime"))[:-1]+(-1, )
                starttime = time.mktime(starttime)
                if starttime > time.time():
                    #_releaseLock(lock)
                    continue

                sendinfo = {
                    'outgoing_num': outgoing_num,
                    'dialstring':   control.get("dialstring")
                    }
                # these options may overwrite the global settings per job
                for n in ('stationID', 'headline'):
                    if control.has_option(n):
                        sendinfo[n] = control.get(n)

                core.log("job %s from %s to %s initiated" %
                         (jobnum, user, sendinfo['dialstring']), 1)
                result, faxinfo = capisuite.fax.sendfax(config, user, capi,
                                                        fax_file, **sendinfo)
                result, resultB3 = result
                core.log("job %s: result was 0x%x, 0x%x" % \
                         (jobnum, result, resultB3), 1)
                sendinfo['result'] = result
                sendinfo['resultB3'] = resultB3
                sendinfo['hostname'] = os.uname()[1]
                if faxinfo:
                    sendinfo.update(faxinfo.as_dict())
                
                # todo: use symbolic names for these results to be more
                # meaningfull
                send_ok = resultB3 == 0 \
                          and result in (0, 0x3400, 0x3480, 0x3490, 0x349f)
                tries = control.getint("tries") +1
                control.set('tries', tries)
                if send_ok:
                    core.log("job %s: finished successfully" % jobnum, 1)
                    control = capisuite.fax.moveJob(controlfile, doneQ, user)
                    sendinfo.update(control.items())
                    helpers.sendSimpleMail(
                        fromaddress, mailaddress,
                        config.get('MailFaxSent', 'subject') % sendinfo,
                        config.get('MailFaxSent', 'text') % sendinfo)
                elif tries >= max_tries:
                    # too many ties, send failed
                    core.log("job %s: failed finally" % jobnum, 1)
                    control = capisuite.fax.moveJob(controlfile, failedQ, user)
                    sendinfo.update(control.items())
                    helpers.sendSimpleMail(
                        fromaddress, mailaddress,
                        config.get('MailFaxFailed', 'subject') % sendinfo,
                        config.get('MailFaxFailed', 'text') % sendinfo)
                else:
                    # delay next try
                    next_delay = int(delays[ min(len(delays),tries) -1 ])
                    core.log("job %s: delayed for %i seconds" % \
                             (jobnum, next_delay), 2)
                    starttime = time.time() + next_delay
                    control.set('starttime', time.ctime(starttime))
                    control.write(controlfile)
            finally:
                _releaseLock(lock)

