/** @file connectmodule.h
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

#ifndef CONNECTMODULE_H
#define CONNECTMODULE_H

#include "callmodule.h"
#include "../backend/connection.h"


using namespace std;

/** @brief Call Module for connection establishment at incoming connection

    This module serves to accept an incoming call and wait for the connection
    establishment. It is the first module you should call when an incoming 
    call is signalled and you want to accept it. 
    
    @author Gernot Hillier
*/
class ConnectModule: public CallModule
{
	public:
 		/** @brief Constructor. Create object.

      		    @param conn reference to Connection object
		    @param service service to connect with as described in Connection::service_t
		    @param faxStationID fax station ID, only necessary when connecting in FAXG3 mode
		    @param faxHeadline fax headline, only necessary when connecting in FAXG3 mode
  		*/
		ConnectModule(Connection *conn, Connection::service_t service, string faxStationID, string faxHeadline);

 		/** @brief Accept connection and wait for complete establishment

		    @throw CapiWrongState Thrown by CallModule::mainLoop()
		    @throw CapiExternalError Thrown by Connection::connectWaiting()
		    @throw CapiMsgError Thrown by Connection::connectWaiting()
  		*/
		void mainLoop() throw (CapiWrongState, CapiExternalError, CapiMsgError);

 		/** @brief Finish mainLoop() if call is completely established
  		*/
		void callConnected();

	private: 	
		Connection::service_t service;   ///< service with which we should connect
		string faxStationID, ///< fax Station ID to use
		       faxHeadline; ///< fax headlint to use
};

#endif

/* History

$Log: connectmodule.h,v $
Revision 1.1  2003/02/19 08:19:53  gernot
Initial revision

Revision 1.7  2002/11/29 10:27:44  ghillie
- updated comments, use doxygen format now

Revision 1.6  2002/11/25 11:57:19  ghillie
- use service_type instead of CIP value in application layer

Revision 1.5  2002/11/22 15:18:56  ghillie
added faxStationID, faxHeadline parameters

Revision 1.4  2002/11/21 15:33:44  ghillie
- moved code from constructor/destructor to overwritten mainLoop() method

Revision 1.3  2002/11/20 17:25:29  ghillie
added missing throw() declaration

Revision 1.2  2002/11/19 15:57:19  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.1  2002/11/14 17:05:58  ghillie
initial checkin

*/
