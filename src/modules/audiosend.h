/** @file audiosend.h
    @brief Contains AudioSend - Call Module for sending an A-Law file

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

#ifndef AUDIOSEND_H
#define AUDIOSEND_H

#include <string>
#include "callmodule.h"

class Connection;

using namespace std;

/** @brief Call Module for sending an A-Law file.

    This module handles the sending of an audio file. The audio file must be in
    bit-inversed A-Law format. It can be created for example with sox using the
    suffix ".la". It supports abortion if DTMF signal is received.

    If DTMF abort is enabled, the module will abort immediately if the DTMF
    receiving buffer (see Connection::getDTMF) isn't empty when it is created.
    That allows the user to abort subsequent audio receive and send commands
    with one DTMF signal w/o needing to check for received DTMF after each
    command.

    The connction must be in audio mode (by connecting with service VOICE),
    otherwise an exception will be caused.

    CapiWrongState will only be thrown if connection is not up at startup,
    not later on. We see a later disconnect as normal event, no error.

    @author Gernot Hillier
*/
class AudioSend: public CallModule
{
	public:
 		/** @brief Constructor. Test if we are in speech mode and create an object.

                    @param conn reference to Connection object
		    @param file name of file to send
		    @param DTMF_exit set to true, if you want to finish when DTMF signal is received
		    @throw CapiExternalError Thrown if speech mode isn't established before.
		    @throw CapiWrongState Thrown if connection is not up (thrown by base class constructor)
		*/
		AudioSend(Connection *conn, string file, bool DTMF_exit) throw (CapiWrongState,CapiExternalError);

		/** @brief Start file transmission, wait for the end of the file or the connection, stop file transmission

		    @throw CapiExternalError Thrown by Connection::start_file_transmission, see there for explanation.
		    @throw CapiMsgError Thrown by Connection::start_file_transmission, see there for explanation.
		    @throw CapiError Thrown by Connection::start_file_transmission, see there for explanation.
		    @throw CapiWrongState Thrown if connection is not up at start of transfer (thrown by Connection::start_file_transmission)
		*/
		void mainLoop() throw (CapiError,CapiWrongState,CapiExternalError,CapiMsgError);

		/** @brief finish main loop if file is completely received

		*/
		void transmissionComplete();

		/** @brief Return the time in seconds since start of mainLoop()

		    @return time in seconds since start of mainLoop()
		*/
		long duration();

	private:
		string file; ///< name of the file to send
		long start_time; ///< time in seconds since the epoch when the module was started
};

#endif

/* History

Old Log (for new changes see ChangeLog):

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.13  2002/12/04 11:38:50  ghillie
- added time measurement: save time in start_time at the begin of mainLoop() and return difference to getTime() in duration()

Revision 1.12  2002/12/02 12:32:21  ghillie
renamed Connection::SPEECH to Connection::VOICE

Revision 1.11  2002/11/29 10:27:44  ghillie
- updated comments, use doxygen format now

Revision 1.10  2002/11/25 21:00:53  ghillie
- improved documentation, now doxygen-readabl

Revision 1.9  2002/11/25 11:54:35  ghillie
- tests for speech mode before receiving now
- small performance improvement (use string::empty() instead of comparison to "")

Revision 1.8  2002/11/22 15:16:20  ghillie
added support for finishing when DTMF is received

Revision 1.7  2002/11/21 15:32:40  ghillie
- moved code from constructor/destructor to overwritten mainLoop() method

Revision 1.6  2002/11/19 15:57:19  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.5  2002/11/14 17:05:19  ghillie
major structural changes - much is easier, nicer and better prepared for the future now:
- added DisconnectLogical handler to CallInterface
- DTMF handling moved from CallControl to Connection
- new call module ConnectModule for establishing connection
- python script reduced from 2 functions to one (callWaiting, callConnected
  merged to callIncoming)
- call modules implement the CallInterface now, not CallControl any more
  => this freed CallControl from nearly all communication stuff

Revision 1.4  2002/11/13 15:26:28  ghillie
removed unnecessary member attribute filename

Revision 1.3  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.2  2002/10/29 14:28:22  ghillie
added stop_file_* calls to make sure transmission is cancelled when it's time...

Revision 1.1  2002/10/25 13:29:39  ghillie
grouped files into subdirectories

Revision 1.10  2002/10/23 15:37:50  ghillie
typo...

Revision 1.9  2002/10/23 14:10:27  ghillie
callmodules must register itself at connection class now

Revision 1.8  2002/10/10 12:45:40  gernot
added AudioReceive module, some small details changed

Revision 1.7  2002/10/09 14:36:22  gernot
added CallModule base class for all call handling modules

Revision 1.6  2002/10/05 20:43:32  gernot
quick'n'dirty, but WORKS

Revision 1.5  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.4  2002/10/04 13:27:15  gernot
some restructuring to get it to a working state ;-)

does not do anything useful yet nor does it even compile...

Revision 1.3  2002/10/02 14:10:07  gernot
first version

Revision 1.2  2002/10/01 09:02:04  gernot
changes for compilation with gcc3.2

Revision 1.1  2002/09/22 14:55:21  gernot
adding audio send module

*/
