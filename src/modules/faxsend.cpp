/* @file faxsend.cpp
   @brief Contains FaxSend - Call Module for sending an analog fax (group 3)

   @author Gernot Hillier <gernot@hillier.de>
   $Revision: 1.2 $
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
#include "faxsend.h"


FaxSend::FaxSend(Connection *conn, string file) throw (CapiWrongState,CapiExternalError)
:CallModule(conn),file(file)
{
	if (conn->getService()!=Connection::FAXG3)
	 	throw CapiExternalError("Connection not in fax mode","FaxSend::FaxSend()");
}


void
FaxSend::mainLoop() throw (CapiWrongState,CapiExternalError,CapiMsgError)
{
	conn->start_file_transmission(file);
	CallModule::mainLoop();
	conn->stop_file_transmission();
}

void 
FaxSend::transmissionComplete()
{
	finish=true;
}

/*  History

$Log: faxsend.cpp,v $
Revision 1.2  2003/12/28 15:00:35  gernot
* rework of exception handling stuff; many modules were not
  declaring thrown exceptions correctly any more after the
  re-structuring to not throw exceptions on any disconnect

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.2  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.1  2002/12/13 11:44:34  ghillie
added support for fax send

*/
