/*  @file switch2faxG3.cpp
    @brief Contains Switch2FaxG3 - Call Module for switching to FAXG3 service from another one.

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

#include "../backend/connection.h"
#include "switch2faxG3.h"

Switch2FaxG3::Switch2FaxG3(Connection *conn_in, string faxStationID, string faxHeadline) throw (CapiWrongState)
:CallModule(conn_in), faxStationID(faxStationID), faxHeadline(faxHeadline)
{}

void
Switch2FaxG3::mainLoop() throw (CapiWrongState, CapiExternalError, CapiMsgError)
{
	conn->debugMessage("switching to fax protocol",1);
	conn->disconnectCall(Connection::LOGICAL_ONLY);
        CallModule::mainLoop();  // wait for DISCONNECT_B3_IND
	finish=false;
	conn->changeProtocol(Connection::FAXG3,faxStationID,faxHeadline); // change to FaxG3
        CallModule::mainLoop();  // wait for CONNECT_B3_IND
	conn->debugMessage("connection re-established, switching to fax protocol finished",1);
}

void
Switch2FaxG3::callDisconnectedLogical()
{
	finish=true;
}

void
Switch2FaxG3::callConnected()
{
	finish=true;
}

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

Revision 1.8  2003/02/10 14:08:21  ghillie
- cosmetical log improvement

Revision 1.7  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.6  2002/12/18 14:46:23  ghillie
- removed done TODO

Revision 1.5  2002/12/06 13:13:06  ghillie
- use Connection::getState() insted of Connection::isUp()

Revision 1.4  2002/12/05 15:56:24  ghillie
- added checks for connection state to throw exception if connection is down

Revision 1.3  2002/11/29 10:29:12  ghillie
- updated comments, use doxygen format now

Revision 1.2  2002/11/25 11:58:53  ghillie
use service_type instead of hardcoded CIP values now

Revision 1.1  2002/11/22 14:59:36  ghillie
initial checkin

*/
