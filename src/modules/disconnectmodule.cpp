/** @file disconnectmodule.cpp
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

#include "disconnectmodule.h"

DisconnectModule::DisconnectModule(Connection *conn, int reject_reason, bool quick_disconnect)
:CallModule(conn,-1,false,false),reject_reason(reject_reason),quick_disconnect(quick_disconnect)
{}

void
DisconnectModule::mainLoop() throw (CapiMsgError,CapiExternalError)
{
	Connection::connection_state_t state=conn->getState();
	if (state!=Connection::DOWN) {
		try {
			if (state==Connection::WAITING)
				conn->rejectWaiting(reject_reason);
			else
				conn->disconnectCall(quick_disconnect ? Connection::PHYSICAL_ONLY : Connection::ALL);
			CallModule::mainLoop();
		}
		catch (CapiWrongState) {} // shouldn't throw CapiWrongState as abort won't get true, but who knows...
	}
}

void DisconnectModule::callDisconnectedLogical()
{}

/*  History

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
