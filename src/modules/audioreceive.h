/** @file audioreceive.h
    @brief Contains AudioReceive - Call Module for receiving audio.

    @author Gernot Hillier <gernot@hillier.de>
    $Revision: 1.4 $
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AUDIORECEIVE_H
#define AUDIORECEIVE_H

#include <string>
#include "callmodule.h"

class Connection;

using namespace std;

/** @brief Call Module for receiving audio.

    This module handles the reception of an audio wave file. It can recognize
    silence in the signal and timeout after a given period of silence, after
    a general timeout or after the reception of a DTMF signal.

    If DTMF abort is enabled, the module will abort immediately if the DTMF
    receiving buffer (see Connection::getDTMF) isn't empty when it is created.
    That allows the user to abort subsequent audio receive and send commands
    with one DTMF signal w/o needing to check for received DTMF after each
    command.

    The call must be in audio mode (by connecting with service VOICE),
    otherwise an exception will be caused.

    CapiWrongState will only be thrown if connection is not up at startup,
    not later on. We see a later disconnect as normal event, no error.

    The created file will be saved in the format given by Capi, that is
    bit-reversed A-Law (or u-Law), 8 kHz, mono.

    @author Gernot Hillier
*/
class AudioReceive: public CallModule
{
	public:
 		/** @brief Constructor. Create an object and test for audio mode.
		
	  	    The constructor also converts the given silence_timeout from seconds to number of samples (bytes).

      		    @param conn reference to Connection object
		    @param file name of file to save received audio stream to.
		    @param timeout timeout in seconds after which record is finished, 0=record forever (until call is finished)
		    @param silence_timeout duration of silence in seconds after which record is finished, 0=no silence detection
		    @param DTMF_exit true: abort if we receive DTMF during mainLoop() or if DTMF was received before
		    @throw CapiExternalError Thrown if connection is not in speech mode
		    @throw CapiWrongState Thrown if connection is not up (thrown by base class constructor)
		*/
		AudioReceive(Connection *conn, string file, int timeout, int silence_timeout, bool DTMF_exit) throw (CapiExternalError,CapiWrongState);

 		/** @brief Start file reception, wait for one of the timeouts or disconnection and stop the reception.

		    If the recording was finished because of silence, the silence is truncated away from the recorded file

		    @throw CapiExternalError Thrown by Connection::start_file_reception().
		    @throw CapiWrongState Thrown if connection is not up at start of transfer (thrown by Connection::start_file_reception)
  		*/
		void mainLoop() throw (CapiWrongState,CapiExternalError);

 		/** @brief Test all received audio packets for silence and count silent packets

		    All bytes of a received packages (i.e. 2048 bytes) are partly A-Law decoded, added and
		    compared to a threshhold. If silence is found, silence_count is increased, otherwise the
		    counter is reset to 0.

		    If the silence_timeout value is reached, the mainLoop is signalled to finish.
  		*/
		void dataIn(unsigned char* data, unsigned length);

 		/** @brief Return the time in seconds since start of mainLoop()

		    @return time in seconds since start of mainLoop()
  		*/
		long duration();

	private:
		unsigned int silence_count; ///< counter how many consecutive samples (bytes) have been silent
		unsigned int silence_timeout; ///< amount of silence samples after which record is finished
		string file; ///< file name to save audio data to
		long start_time, ///< time in seconds since the epoch when the recording was started
			end_time; ///< time in seconds since the epoch when the recording was finished
};

#endif

/* History

Old Log (for new changes see ChangeLog):

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.14  2003/01/16 13:03:07  ghillie
- added attribute end_time
- updated comment to reflect new truncation of silence

Revision 1.13  2002/12/04 11:38:50  ghillie
- added time measurement: save time in start_time at the begin of mainLoop() and return difference to getTime() in duration()

Revision 1.12  2002/12/02 12:32:21  ghillie
renamed Connection::SPEECH to Connection::VOICE

Revision 1.11  2002/11/29 10:26:10  ghillie
- updated comments, use doxygen format now
- removed transmissionComplete() method as this makes no sense in receiving!

Revision 1.10  2002/11/25 11:54:21  ghillie
- tests for speech mode before receiving now
- small performance improvement (use string::empty() instead of comparison to "")

Revision 1.9  2002/11/22 15:16:20  ghillie
added support for finishing when DTMF is received

Revision 1.8  2002/11/21 15:32:40  ghillie
- moved code from constructor/destructor to overwritten mainLoop() method

Revision 1.7  2002/11/19 15:57:19  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.6  2002/11/14 17:05:19  ghillie
major structural changes - much is easier, nicer and better prepared for the future now:
- added DisconnectLogical handler to CallInterface
- DTMF handling moved from CallControl to Connection
- new call module ConnectModule for establishing connection
- python script reduced from 2 functions to one (callWaiting, callConnected
  merged to callIncoming)
- call modules implement the CallInterface now, not CallControl any more
  => this freed CallControl from nearly all communication stuff

Revision 1.5  2002/11/13 15:24:25  ghillie
finished silence detection code

Revision 1.4  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.3  2002/11/12 15:52:08  ghillie
added data in handler

Revision 1.2  2002/10/29 14:28:22  ghillie
added stop_file_* calls to make sure transmission is cancelled when it's time...

Revision 1.1  2002/10/25 13:29:39  ghillie
grouped files into subdirectories

Revision 1.2  2002/10/24 09:55:52  ghillie
many fixes. Works for one call now

Revision 1.1  2002/10/23 09:47:30  ghillie
added audioreceive module

*/
