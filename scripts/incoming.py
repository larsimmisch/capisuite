# -*- mode: python ; coding: iso_8859_15 -*-
#
#              incoming.py - standard incoming script for capisuite
#              ----------------------------------------------------
#    copyright            : (C) 2002 by Gernot Hillier
#    email                : gernot@hillier.de
#    version              : $Revision: 1.18 $
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#

# general imports
import time, os

# CapiSuite imports
import capisuite
import capisuite.helpers as helpers
from capisuite.config import NoOptionError
import capisuite.fileutils as fileutils
import capisuite.fax
import capisuite.voice
from capisuite import core
say = capisuite.voice.say # shortcut

def callIncoming(call, service, call_from, call_to):
    """
    Main function called by CapiSuite when an incoming call is received.

    It will decide if this call should be accepted, with which service
    and for which user. The real call handling is done in faxIncoming
    and voiceIncoming.

    'call' reference to the call. Needed by all capisuite functions
    'service' one of SERVICE_FAXG3, SERVICE_VOICE, SERVICE_OTHER
    'call_from' string containing the number of the calling party
    'call_to' string containing the number of the called party
    """
    # convert into a python call handle
    # TODO-gh: can't we get rid of this line?
    call = core.Call(call, service, call_from, call_to)
    # read config file and search for call.to_nr in the user sections
    try:
        config = capisuite.config.readGlobalConfig()
    except IOError, e:
        core.error("Error occured during config file reading: %s "
                   "Disconnecting..." % e)
        call.reject(0x34A9)
        return

    for user in config.listUsers():
        # accept a voice call on 'voice_numbers'
        if config.has_option(user, 'voice_numbers'):
            numbers = config.getList(user, 'voice_numbers')
            if numbers == ["*"] or call.to_nr in numbers:
                if service in (core.SERVICE_VOICE, ):
                    break
        # accept a voice or fax call on 'fax_numbers'
        if config.has_option(user, 'fax_numbers'):
            numbers = config.getList(user, 'fax_numbers')
            if numbers == ["*"] or call.to_nr in numbers:
                if service in (core.SERVICE_FAXG3, core.SERVICE_VOICE):
                    # set service type to 'fax'
                    service = core.SERVICE_FAXG3
                    break
    else:
        # no matching entry found (no users as this number)
        call.log("call from %s to %s ignoring" % (call.from_nr, call.to_nr), 1)
        call.reject(1)
        return

    # answer the call with the right service
    try:
        if service == core.SERVICE_VOICE:
            voiceIncoming(config, user, call)
        elif service == core.SERVICE_FAXG3:
            faxIncoming(config, user, call, 0)
        else:
            raise RuntimeError
    except NoOptionError, err:
        core.error("global option %s not found -> rejecting call" %
                   err.option)
        call.reject(0x34A9)
    except fileutils.UnknownUserError:
        core.error("user %s is not a valid system user. Disconnecting." % user,
                   call)
        call.reject(0x34A9)
    except core.CallGoneError:
        causes = call.disconnect()
        call.log("connection lost with cause 0x%x, 0x%x" % causes, 1)


def faxIncoming(config, user, call, already_connected):
    """
    Called by callIncoming when an incoming fax call is received

    'call' a python Call handle referencing to the call
    'user' name of the user who is responsible for this
    'config' ConfigParser instance holding the config data
    'already_connected' ture if we're already connected (that means we must
    switch to fax mode)
    """
    # todo: use config.getQueueFiles + _mkdir here
    receivedQ = fileutils._mkuserdir(user,
                                     config.get('GLOBAL', "fax_user_dir"),
                                     user, "received")

    # assure these variables are defined
    filename = faxInfo = None

    stationID = config.getUser(user, "fax_stationID", default="")
    if not stationID:
        core.error("Warning: fax_stationID not found for user %s "
                   "-> using empty string" % user)
    # empty string is no problem for headline
    headline = config.getUser(user, "fax_headline", default="")

    call.log("call from %s to %s for %s connecting with fax" % \
             (call.from_nr, call.to_nr, user), 1)
    try:
        if already_connected:
            faxInfo = call.switch_to_faxG3(stationID, headline)
        else:
            faxInfo = call.connect_faxG3(stationID, headline)
        filename = fileutils.uniqueName(receivedQ, "fax", faxInfo.format)[1]
        call.fax_receive(filename)
        causes = call.disconnect()
        call.log("connection finished with cause 0x%x, 0x%x" % causes, 1)
    except core.CallGoneError:
        # catch this here to get the cause info into the mail
        causes = call.disconnect()
        call.log("connection lost with cause 0x%x, 0x%x" % causes, 1)
        # todo: send error mail here? Don't think it makes sense to send
        # a mail on each try, which would mean sending 10 mails for one fax...
        # If the user wants to know the current status he should use "capisuitefax -l"
    else:
        assert filename
        if os.access(filename, os.R_OK):
            faxInfo = faxInfo.as_dict()
            faxInfo.update({
                'filename' : filename,
                'call_from': call.from_nr,
                'call_to'  : call.to_nr,
                'causes'   : causes,
                'hostname' : os.uname()[1]
                })
            capisuite.fax.createReceivedJob(user, **faxInfo)
            action = _getAction(config, user, "fax_action",
                                ("saveonly", "mailandsave"))
            if action == "mailandsave":
                fromaddress = config.getUser(user, "fax_email_from", user)
                mailaddress = config.getUser(user, "fax_email", user)

                helpers.sendMIMEMail(
                    fromaddress, mailaddress,
                    config.get('MailFaxReceived', 'subject') % faxInfo,
                    faxInfo['format'],
                    config.get('MailFaxReceived', 'text') % faxInfo,
                    filename)


def _getAction(config, user, action_name, allowed_actions):
    action = config.getUser(user, action_name, "").lower()
    if action not in allowed_actions:
        action = allowed_actions[0]
        core.error("Warning: No valid %s definition found for user %s -> "
                   "assuming %s" % action_name, user, action)
    return action


def voiceIncoming(config, user, call):
    """
    Called by callIncoming when an incoming voice call is received

    'call' a python Call handle referencing to the call
    'user' name of the user who is responsible for this
    'config' ConfigParser instance holding the config data
    """
    try:
        if not config.has_option(user, 'voice_delay'):
            core.error("voice_delay not found for user %s! -> "
                    "rejecting call" % user)
            call.reject(0x34A9)
            return
        delay = config.getint(user, "voice_delay")
        call.log("call from %s to %s for %s connecting with voice" % \
            (call.from_nr, call.to_nr, user), 1)
        call.connect_voice(delay)

        userdir = config.get('GLOBAL', "voice_user_dir")
        action = _getAction(config, user, "voice_action",
                            ("saveonly", "mailandsave", "none"))
        receivedQ = fileutils._mkuserdir(user, userdir, user, "received")
        userannouncement = os.path.join(
            userdir, user,
            config.getUser(user, "announcement", "announcement.la"))
        pin = config.getUser(user, "pin", "")
        filename = None # assure it's defined

        call.enable_DTMF()
        if os.access(userannouncement, os.R_OK):
            call.audio_send(userannouncement, 1)
        else:
            if call.to_nr != "-":
                say(config, user, call, "anrufbeantworter-von.la")
                capisuite.voice.sayNumber(config, user, call, call.to_nr)
            say(config, user, call, "bitte-nachricht.la")

        if action != "none":
            say(config, user, call, "beep.la")
            length = config.getUser(user, "record_length", 60)
            # todo: put this into voice.getNameForRecord
            filename = fileutils.uniqueName(receivedQ, "voice", 'la')[1]
            silence_timeout = config.getUser(user, "record_silence_timeout", 5)
            msg_length = call.audio_receive(filename, int(length),
                                                 int(silence_timeout), 1)
        dtmf_list = call.read_DTMF(0)
        if dtmf_list == "X":
            if os.access(filename, os.R_OK):
                os.unlink(filename)
            faxIncoming(config, user, call, 1)
        elif dtmf_list and pin:
            # wait 5 seconds for input
            dtmf_list += call.read_DTMF(3)
            count = 1
            # allow three tries
            while count < 3 and pin != dtmf_list:
                call.log("wrong PIN entered...", 1)
                say(config, user, call, "beep.la")
                dtmf_list = call.read_DTMF(3)
                count += 1
            else:
                # failed three time
                # todo: hang up?
                # gh: Yes.
                pass
            if pin == dtmf_list:
                if os.access(filename, os.R_OK):
                    os.unlink(filename)
                call.log("Starting remote inquiry...", 1)
                remoteInquiry(config, user, call, receivedQ)

    except core.CallGoneError:
        # catch this here to get the cause info in the mail
        causes = call.disconnect()
        call.log("connection lost with cause 0x%x, 0x%x" % causes, 1)
    else:
        causes = call.disconnect()
        call.log("connection finished with cause 0x%x, 0x%x" % causes, 1)

    if (filename and os.access(filename, os.R_OK)):    
        info = capisuite.voice.createReceivedJob(user, filename, call.from_nr,
                                                 call.to_nr, causes)
        fromaddress = config.getUser(user, "voice_email_from", user)
        mailaddress = config.getUser(user, "voice_email", user)
        if action == "mailandsave":
            info['hostname'] = os.uname()[1]
            info['msg_length'] = msg_length
            helpers.sendMIMEMail(
                fromaddress, mailaddress,
                config.get('MailVoiceReceived', 'subject') % info,
                "la",
                config.get('MailVoiceReceived', 'text') % info,
                filename)

def remoteInquiry(config, user, call, receivedQ):
    """
    Remote inquiry function (uses german wave snippets!)

    Implemented commands for remote inquiry are:
      delete message - 1
      next message - 4
      last message - 5
      repeat current message - 6

    'user' name of the user who is responsible for this
    'config' ConfigParser instance holding the config data
    'call' reference to the call. Needed by all capisuite functions
    'receivedQ' the received queue dir of the user
    """
    try:
        lock = fileutils._getLock(
            os.path.join(receivedQ, 'inquiry_lock'), blocking=0)
    except fileutils.LockTakenError:
        say(config, user, call, "fernabfrage-aktiv.la")
        return
    try:
        # read directory contents
        messages = capisuite.voice.getQueueFiles(config, user)

        # read the number of the message heard last at the last inquiry
        lastinquiry = capisuite.voice.getInquiryCounter(config, user)
        # filter out old messages
        oldmessages = [m for m in messages if m[0] <= lastinquiry]
        messages    = [m for m in messages if m[0] >  lastinquiry]
        oldmessages.sort()
        messages.sort()

        announceNumMessages(config, user, call, len(messages), new=1)

        # menu for record new announcement
        cmd = ""
        while cmd not in ("1", "9"):
            if len(messages) or len(oldmessages):
                say(config, user, call, "zum-abhoeren-1.la")
            say(config, user, call, "fuer-neue-ansage-9.la")
            cmd = call.read_DTMF(0, 1)
        if cmd == "9":
            cmd_recordNewAnnouncement(config, user, call, receivedQ)
            return

        # start inquiry
        cmd_inquiry(config, user, call, oldmessages, messages, lastinquiry)

    finally:
        fileutils._releaseLock(lock)


def announceNumMessages(config, user, call, numMessages, new):
    if numMessages == 1:
        msg = "nachricht.la"
    else:
        msg = "nachrichten.la"
    if new:
        msg = 'neue-' + msg
    capisuite.voice.sayNumber(config, user, call, numMessages, gender="f")
    say(config, user, call, msg)


def cmd_inquiry(config, user, call, oldmessages, messages, lastinquiry):
    """
    Remote inquiry function (uses german wave snippets!)

    Implemented commands for remote inquiry are:
      delete message - 1
      next message - 4
      last message - 5
      repeat current message - 6

    'user' name of the user who is responsible for this
    'config' ConfigParser instance holding the config data
    'call' reference to the call. Needed by all capisuite functions
    'userdir' spool_dir of the current_user
    """
    for curr_msgs in (messages, oldmessages):
        capisuite.voice.sayNumber(config, user, call, len(curr_msgs), "f")
        announceNumMessages(config, user, call, len(messages),
                            new = (curr_msgs == messages))
        i = 0
        while i < len(curr_msgs):
            msgnum, controlfile = curr_msgs[i]
            descr = config.JobDescription(controlfile)
            # play the announcement
            for f in _getSoundsForAnnounceMessages(i, descr):
                say(config, user, call, "%s.la" % f)
            # play the recorded file
            call.audio_send(descr.get('filename'), 1)
            cmd = ""
            while cmd not in ("1", "4", "5", "6"):
                say(config, user, call, "erklaerung.la")
                cmd = call.read_DTMF(0, 1)
            if cmd == "1":
                cmd_deleteMessage(config, user, call, controlfile)
                del curr_msgs[i]
            elif cmd == "4":
                if msgnum > lastinquiry:
                    lastinquiry = msgnum
                    capisuite.voice.setInquiryCounter(config, user, msgnum)
                i += 1
            elif cmd == "5":
                i -= 1
    say(config, user, call, "keine-weiteren-nachrichten.la")
    # todo: say 'hauptmenu'

def _getSoundsForAnnounceMessages(msgnum, descr):
    """
    generated a list of sound to be played before replaying a recored message
    """
    getNumberFiles = capisuite.voice.getNumberFiles
    yield 'nachricht'
    for f in getNumberFiles(msgnum+1): yield f
    yield 'von'
    for f in getNumberFiles(descr.get('call_from')): yield f
    yield 'fuer'
    for f in getNumberFiles(descr.get('call_to')): yield f
    yield 'am'
    calltime = time.strptime(descr.get('time'))
    for f in getNumberFiles(calltime[2]): yield f
    yield '.'
    for f in getNumberFiles(calltime[1]): yield f
    yield '.'
    yield 'um'
    for f in getNumberFiles(calltime[3], 'n'): yield f
    yield 'uhr'
    for f in getNumberFiles(calltime[4]): yield f

def cmd_deleteMessage(config, user, call, controlfile):
    capisuite.fax.abortJob(controlfile)
    say(config, user, call, "nachricht-geloescht.la")


def cmd_recordNewAnnouncement(config, user, call, userdir):
    """
    remote inquiry command: record new announcement (uses german wave snippets!)
    'config' ConfigParser instance holding the config data
    'user' name of the user who is responsible for this
    'call' reference to the call. Needed by all capisuite functions
    'userdir' spool_dir of the current_user
    """
    say(config, user, call, "bitte-neue-ansage-komplett.la", "beep.la")

    tmpfile = os.path.join(userdir, "announcement-tmp.la")
    while 1:
        call.audio_receive(tmpfile, 60, 3)
        say(config, user, call, "neue-ansage-lautet.la")
        call.audio_send(tmpfile)
        say(config, user, call, "wenn-einverstanden-1.la")
        cmd = call.read_DTMF(0, 1)
        # todo: allow eg. '9' for cancel and go back to menu
        if cmd == "1":
            break
        else:
            say(config, user, call, "bitte-neue-ansage-kurz.la", "beep.la")

    userannouncement = os.path.join(userdir,
                       config.getUser(user, "announcement", "announcement.la"))
    os.rename(tmpfile, userannouncement)
    fileutils._setProtection(user, userannouncement, mode=0666)
    say(config, user, call, "ansage-gespeichert.la")

