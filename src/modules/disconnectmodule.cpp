/** @file disconnectmodule.cpp
    @brief Contains DisconnectModule - Call Module for call clearing

    @author Gernot Hillier <gernot@hillier.de>
    $Revision: 1.1 $
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
:CallModule(conn),reject_reason(reject_reason),quick_disconnect(quick_disconnect)
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

void DisconnectModule::callDisconnectedPhysical()
{
 	finish=true;
}

/*  History

$Log: disconnectmodule.cpp,v $
Revision 1.1  2003/02/19 08:19:53  gernot
Initial revision

Revision 1.3  2002/12/11 13:40:22  ghillie
- added support for quick disconnect (immediate physical disconnect)

Revision 1.2  2002/12/06 15:26:30  ghillie
- supports rejecting of call now, too

Revision 1.1  2002/12/06 12:48:38  ghillie
inital checkin

*/
