/** @file disconnectmodule.h
    @brief Contains DisconnectModule - Call Module for call clearing

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

#ifndef DISCONNECTMODULE_H
#define DISCONNECTMODULE_H

#include "callmodule.h"
#include "../backend/connection.h"


using namespace std;

/** @brief Call Module for call clearing

    This module initiates disconnection or rejection of connection and
    waits until the physical connection is cleared completely. It's
    no problem to call it when the connection is already (partly or completely)
    cleared.

    There exists nothing like a wrong connection state to disconnect, therefore
    CapiWrongState is never thrown.

    @author Gernot Hillier
*/
class DisconnectModule: public CallModule
{
	public:
 		/** @brief Constructor. Create object.

      		    @param conn reference to Connection object
		    @param reject_reason reason to give for rejecting a waiting call (1=ignore call, default; 
		           for other values see Connection::rejectWaiting()), ignored for normal disconnect 
		    @param quick_disconnect disconnect physical immediately, this will lead to a protocol error in Layer 3. Use for error cases.
  		*/
		DisconnectModule(Connection *conn, int reject_reason=1, bool quick_disconnect=false);

		/** @brief Initiate call clearing and wait for successful physical disconnection.
		
		    @throw CapiMsgError Thrown by Connection::disconnectCall() or Connection::rejectWaiting()
		    @throw CapiExternalError Thrown by Connection::rejectWaiting().
  		*/
		void mainLoop() throw (CapiMsgError,CapiExternalError);

		/** @brief Do nothing as we're waiting for physical disconnection.
		*/
  		void callDisconnectedLogical ();

	private:
		int reject_reason; ///< saving reject reason given in constructor
		bool quick_disconnect; ///< disconnect physical immediately w/o disconnection logical before
};

#endif

/* History

Old Log (for new changes see ChangeLog):

Revision 1.2  2003/10/03 14:56:40  gernot
- partly implementation of a bigger semantic change: don't throw
  call finished exceptions in normal operation any longer; i.e. we only
  test for the connection at the begin of a command. This allows return
  values, e.g. for commands like capisuite.fax_receive() which were
  interrupted by an exception always in former CapiSuite versions and thus
  never returned. This is also a better and more logical use of exceptions
  IMO. ATTN: this is *far from stable*

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.3  2002/12/11 13:40:22  ghillie
- added support for quick disconnect (immediate physical disconnect)

Revision 1.2  2002/12/06 15:26:30  ghillie
- supports rejecting of call now, too

Revision 1.1  2002/12/06 12:48:38  ghillie
inital checkin

*/
