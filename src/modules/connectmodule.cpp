/*  @file connectmodule.cpp
    @brief Contains ConnectModule - Call Module for connection establishment at incoming connection

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

#include "connectmodule.h"

ConnectModule::ConnectModule(Connection *conn_in, Connection::service_t service, string faxStationID, string faxHeadline)
:CallModule(conn_in),service(service),faxStationID(faxStationID),faxHeadline(faxHeadline)
{}

void
ConnectModule::mainLoop() throw (CapiWrongState, CapiExternalError, CapiMsgError)
{
	conn->connectWaiting(service,faxStationID,faxHeadline);
	CallModule::mainLoop();
}

void
ConnectModule::callConnected()
{
	finish=true;
}

/*  History

$Log: connectmodule.cpp,v $
Revision 1.1  2003/02/19 08:19:53  gernot
Initial revision

Revision 1.6  2002/11/29 10:27:44  ghillie
- updated comments, use doxygen format now

Revision 1.5  2002/11/25 11:57:19  ghillie
- use service_type instead of CIP value in application layer

Revision 1.4  2002/11/22 15:18:56  ghillie
added faxStationID, faxHeadline parameters

Revision 1.3  2002/11/21 15:33:44  ghillie
- moved code from constructor/destructor to overwritten mainLoop() method

Revision 1.2  2002/11/19 15:57:19  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.1  2002/11/14 17:05:58  ghillie
initial checkin

*/
