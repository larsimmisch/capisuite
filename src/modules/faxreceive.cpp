/* @file faxreceive.cpp
   @brief Contains FaxReceive - Call Module for receiving an analog fax (group 3)

   @author Gernot Hillier <gernot@hillier.de>
   $Revision: 1.3 $
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
#include "faxreceive.h"


FaxReceive::FaxReceive(Connection *conn, string file) throw (CapiWrongState,CapiExternalError)
:CallModule(conn),file(file)
{
	if (conn->getService()!=Connection::FAXG3)
	 	throw CapiExternalError("Connection not in fax mode","FaxReceive::FaxReceive()");
}


void
FaxReceive::mainLoop() throw (CapiWrongState,CapiExternalError)
{
	conn->start_file_reception(file);
	CallModule::mainLoop();
	conn->stop_file_reception();
}

void 
FaxReceive::transmissionComplete()
{
	finish=true;
}

/*  History

Old Log (for new changes see ChangeLog):

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.12  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.11  2002/12/09 15:43:45  ghillie
- small typo...

Revision 1.10  2002/11/29 10:27:44  ghillie
- updated comments, use doxygen format now

Revision 1.9  2002/11/25 11:58:04  ghillie
- test for fax mode before receiving now

Revision 1.8  2002/11/21 15:32:40  ghillie
- moved code from constructor/destructor to overwritten mainLoop() method

Revision 1.7  2002/11/21 11:37:09  ghillie
make sure that we don't use Connection object after call was finished

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

Revision 1.7  2002/10/23 14:34:26  ghillie
call modules must register itself at CallControl now

Revision 1.6  2002/10/23 09:45:00  ghillie
changed to fit into new architecture

*/
