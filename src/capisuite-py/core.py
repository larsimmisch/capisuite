"""capisuite.core

This module exposes the built-in core of capisuite.
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


# _capisuite may only be imported when running within capisuite
try:
   # import all capisuite symbols in the namespace "_capisuite.symbol"
   import _capisuite 
   # now add symbols directly used by the scripts to our namespace
   from _capisuite import log,error,SERVICE_VOICE,SERVICE_FAXG3,CallGoneError
except ImportError:
    pass


#########
###
###  ATTENTION: This interface is not yet stable. You may expect
###  changes until capisuite 0.5 is released!
###
#########

class Capi:
    def __init__(self, handle):
        """
        handle: a capi handle as received from _capisuite (given to the idle
                function as parameter)
        """
        self._handle = handle

    def __repr__(self):
        return 'Capi(%(_handle)s)' % self.__dict__

    def call_voice(self, controller, call_from, call_to,
                   timeout, clir=0):
        """
        Initiate an outgoing call with service voice and wait for
        successful connection.

        This will initiate an outgoing call and choose voice as
        service, so you can use the audio commands (like audio_receive()
        and audio_send()) with this connection. After this command has
        finished, the call is connected successfully or the given
        timeout has exceeded. The timeout is measured beginning at the
        moment when the call is signalled (it's "ringing") to the
        called party.
        
        Parameters:
        controller: ISDN controller ID to use
        call_from: own number to use (string)
        call_to: the number to call (string)
        timeout: timeout in seconds to wait for connection establishment
        clir: disable sending of own number (default=0, send number)

        On success returns a call object; on failure returns an
        error_code.
        """
        call, result = _capisuite.call_voice(self._handle, controller,
                                       call_from, call_to,
                                       timeout, clir)
        if result:
            return None, result
        return Call(call, SERVICE_VOICE, call_from, call_to), None


    def call_faxG3(self, controller, call_from, call_to,
                   timeout, stationID, headline, clir=0):
        """
        Initiate an outgoing call with service faxG3 and wait for
        successful connection.

        This will initiate an outgoing call and choose fax group 3 as
        service, so you can use the fax commands (like fax_send() and
        fax_receive()) with this connection. After this command has
        finished, the call is connected successfully or the given
        timeout has exceeded. The timeout is measured beginning at the
        moment when the call is signalled (it's "ringing") to the
        called party.
        
        Parameters:
        controller: ISDN controller ID to use
        call_from: own number to use (string)
        call_to: the number to call (string)
        timeout: timeout in seconds to wait for connection establishment
        faxStationID: fax station ID (string)
        faxHeadline: fax headline to print on every page (string)
        clir: disable sending of own number (default=0, send number)

        On success returns a call object; on failure returns an
        error_code.
        """
        call, result = _capisuite.call_faxG3(self._handle, controller,
                                             call_from, call_to,
                                             timeout, stationID, headline)
        if result:
            return None, result
        return Call(call, SERVICE_FAXG3, call_from, call_to), None


class Call:
    def __init__(self, handle, service, call_from, call_to):
        """
        handle: a call handle as received from _capisuite

        NB: A Call instance is never True to ease testing results from
        Capi.call_...()
        """
        self._handle = handle
        self.service = service
        self.from_nr = call_from
        self.to_nr = call_to

    ###--- python stuff --###

    def __nonzero__(self):
        # 'if Call()' must never be true to allow easier results from
        # Capi.call_...()
        return 0

    def __str__(self):
        return str(self._handle)
    
    def __repr__(self):
        # todo: add service, call_from, call_to
        return ('Call(%(_handle)s, service=%(service)s, '
                'from_nr=%(from_nr)s, to_nr%(to_nr)s)') % self.__dict__


    ###--- general --###

    def disconnect(self):
        """
        Disconnect connection.

        This will cause an immediate disconnection. It should be
        always the last command in every flow of a script.

        Returns a tuple of two result values. The first is the
        disconnect cause of the physical connection, the second the
        disconnect cause of the logical connection. See CAPI spec for
        the logical causes and ETS 300 102-01 for the physical causes.
        """
        result = _capisuite.disconnect(self._handle)
        return result


    def reject(self, rejectCause):
        """
        Reject an incoming call. 

        If you don't want to accept an incoming call for any reason
        (e.g. if it has a service or comes from a number you don't
        want to accept), use this command. There are several reasons
        you can give when rejecting a call. Some important ones are:

        rejectCause: cause to signal when rejecting call. This may be one of
          1 = ignore call
          2 = normal call clearing
          3 = user busy
          7 = incompatible destination
          8 = destination out of order
          0x34A9 = temporary failure
        """
        _capisuite.reject(self._handle, rejectCause)

    def log(self, message, level):
    	"""
        Log a connection dependent message.

        This function writes a message to the CapiSuite log. As all messages
        written with it are prefixed with the current call reference, you
        should use it for connection-dependant messages (e.g. information about
        handling *this* call).

        If you want to log messages of general nature not associated with a
        certain call (e.g. problem in reading configuration files), please use
        core.log instead.

        message: the log message to be written
        level: parameter for CapiSuite log_level used (0=vital .. 3=debug info)
        """
        _capisuite.log(message, level, self._handle)

    ###--- DTMF support --###

    def enable_DTMF(self):
        """
        Enable recognition of DTMF tones.
        """
        _capisuite.enable_DTMF(self._handle)

    def disable_DTMF(self):
        """
        Disable recognition of DTMF tones.
        """
        _capisuite.disable_DTMF(self._handle)


    def read_DTMF(self, timeout, min_digits=0, max_digits=0):
        """
        Read the received DTMF tones or wait for a certain amount of
        them.

        This function allows to just read in the DTMF tones which were
        already received. But it also supports to wait for a certain
        amount of DTMF tones if you want the user to always input some
        digits at a certain step in your script.

        You can specify how much DTMF tones you want in several ways -
        see the parameter description. To just see if something was
        entered before, use capisuite.read_DTMF(0). If you want to get
        at least 1 and mostly 4 digits and want to wait 5 seconds for
        additional digits, you'll use capisuite.read_DTMF(5,1,4).

        Valid DTMF characters are '0'...'9','A'...'D' and two special
        fax tones: 'X' (CNG), 'Y' (CED)

        timeout: timeout in seconds after which reading is terminated;
                 only applied after min_digits have been read! (-1 =
                 infinite)
        min_digits: minimum number of digits which must be read in ANY
                    case, i.e. timout doesn't count here (default: 0)
        max_digits: maximum number of digits to read; aborts
                    immediately enough digits are read) (default:
                    0=infinite, i.e. wait until timeout is reached)

        Returns a string containing the characters read.
        """
        # todo: descibe what A...D means and where '#' and '*' go
        return _capisuite.read_DTMF(self._handle, timeout,
                                    min_digits, max_digits)


    ###--- voice calls ---###

    def connect_voice (self, delay=0):
        """
        Accept an incoming call and connect with voice service.

        This will accept an incoming call and choose voice as service,
        so you can use the audio commands (like audio_receive() and
        audio_send()) with this connection. After this command has
        finished, the call is connected successfully.

        It's also possible to accept a call with some delay. This is
        useful for an answering machine if you want to fetch a call
        with your phone before your computer answers it.

        delay: delay in seconds _before_ connection will be established
               (default: 0=immediate connect)
        """
        _capisuite.connect_voice(self._handle, delay)


    def audio_receive(self, filename, timeout, silence_timeout=0,
                      exit_DTMF=0):
        """
        Receive an audio file in a speech mode connection.

        This functions receives an audio file. It can recognize
        silence in the signal and timeout after a given period of
        silence, after a general timeout or after the reception of a
        DTMF signal.

        If the recording was finished because of silence_timeout, the
        silence will be truncated away.

        If DTMF abort is enabled, the command will also abort
        immediately if DTMF was received before it is called. This
        allows to abort subsequent audio receive and send commands
        with one DTMF signal w/o the need to check for received DTMF
        after each command.

        The connction must be in audio mode (by connect_voice()),
        otherwise an exception will be caused.

        The created file will be saved in bit-reversed A-Law format, 8
        kHz mono. Use sox to convert it to a normal wav file.
        
        filename: where to save the received message.
        timeout: receive length in seconds (-1 = infinite).
        silence_timeout: abort after x seconds of silence (default: no timeout)
        exit_DTMF: abort sending when a DTMF signal is received (default: 0)

        Returns duration of receiving in seconds.
        """
        return _capisuite.audio_receive(self._handle, filename, timeout,
                                        silence_timeout, exit_DTMF)


    def audio_send(self, filename, exit_DTMF=0):
        """
        Send an audio file in a speech mode connection.

        This function sends an audio file, which must be in
        bit-inversed A-Law format. Thus files can be created with eg.
        sox using the suffix ".la". It supports abortion if DTMF
        signal is received.

        If DTMF abort is enabled, the command will also abort
        immediately if DTMF was received before it is called. That
        allows you to abort subsequent audio receive and send commands
        with one DTMF signal w/o the need to check for received DTMF
        after each command.

        The connction must be in audio mode (use connect_voice()),
        otherwise an exception will be caused.

        filename: file to send
        exit_DTMF: abort sending when a DTMF signal is received (default: 0)

        Returns duration of send in seconds.
        """
        return _capisuite.audio_send(self._handle, filename, exit_DTMF)


    def switch_to_faxG3(self, faxStationID, faxHeadline):
        """
        Switch a connection from voice mode to fax mode. 

        This will switch from voice mode to fax group 3 after you have
        connected, so you can use the fax commands afterwards.

        Attention: Not all ISDN cards or CAPI driver support this
                   command.
 
        faxStationID: the station ID to use (string)
        faxHeadline: the fax headline to use (string)

        Returns a FaxInfo instance.
        """
        faxInfo = _capisuite.switch_to_faxG3(self._handle,
                                             faxStationID, faxHeadline)
        self.service = SERVICE_FAXG3
        log('faxinfo: %s' % repr(faxInfo), 3)
        if not faxInfo:
            return FaxInfo()
        return FaxInfo(*faxInfo)


    ###--- fax calls --###

    def connect_faxG3(self, faxStationID, faxHeadline, delay=0):
        """
        Accept an incoming call and connect with fax (analog, group 3) service.

        This will accept an incoming call and choose fax group 3 as
        service, so you can use the fax commands (like fax_receive())
        with this connection. After this command has finished, the
        call is connected successfully.

        It's also possible to accept a call with some delay. This is
        useful if eg. you want to have to fetch a call with your phone
        before your computer answers it.

        faxStationID: the station ID to use (string)
        faxHeadline: the fax headline to use (string)
        delay: delay in seconds _before_ connection will be established
               (default: 0=immediate connect)

        Returns a FaxInfo instance.
        """
        faxInfo = _capisuite.connect_faxG3(self._handle, faxStationID,
                                           faxHeadline, delay)
        log('faxinfo: %s' % repr(faxInfo), 3)
        if not faxInfo:
            return FaxInfo()
        return FaxInfo(*faxInfo)


    def fax_send(self, faxfilename):
        """
        Send a fax in a fax mode connection. 

        This command sends an analog fax (fax group 3). It starts the
        send and waits for the end of the connection. So it should be
        the last command before disconnect().

        The connction must be in fax mode (use capi.call_faxG3() or
        call.switch_to_faxG3()), otherwise an exception will be caused.

        The file to be sent must be in the Structured Fax File (SFF)
        format.

        faxfilename: file to send
        """
        faxInfo = _capisuite.fax_send(self._handle, faxfilename)
        log('faxinfo: %s' % repr(faxInfo), 3)
        if not faxInfo:
            return FaxInfo()
        return FaxInfo(*faxInfo)


    def fax_receive(self, filename):
        """
        Receive a fax in a fax mode connection.

        This command receives an analog fax (fax group 3). It starts
        the reception and waits for the end of the connection. So it
        should be the last command before disconnect().

        The connction must be in fax mode (use capi.call_faxG3() or
        call.switch_to_faxG3()), otherwise an exception will be caused.

        The created file will be saved in the Structured Fax File
        (SFF) format.

        filename: where to save the received fax.
        """
        faxInfo = _capisuite.fax_receive(self._handle, filename)
        log('faxinfo: %s' % repr(faxInfo), 3)
        if not faxInfo:
            return FaxInfo()
        return FaxInfo(*faxInfo)


class FaxInfo:
    def __init__(self, stationID='', rate=0, hiRes=0, format=0, numPages=0):
        self.stationID  = stationID
        self.bitRate    = rate
        self.hiRes      = hiRes
        self.resolution = hiRes and "hiRes" or "loRes"
        # cff: color fax; sff: normal black-and-white fax 
        self.format   = format and 'cff' or 'sff'
        self.color    = format and 'color' or 'b&w'
        self.numPages = numPages

    def as_dict(self):
        d = {}
        for a in ('stationID', 'bitRate', 'resolution',
                  'hiRes', 'format', 'color', 'numPages'):
            d[a] = getattr(self, a)
        return d

# implemented in _capisuite:
#
#def error(...):
#    pass
#def log(...):
#    pass
