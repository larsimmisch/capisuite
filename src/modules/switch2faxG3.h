/** @file switch2faxG3.h
    @brief Contains Switch2FaxG3 - Call Module for switching to FAXG3 service from another one.

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

#ifndef SWITCH2FAXG3_H
#define SWITCH2FAXG3_H

#include "callmodule.h"

class Connection;

using namespace std;

/** @brief Call Module for switching to FAXG3 service from another one.

    This module does all the necessary steps to switch from another service (mostly VOICE)
    to FaxG3 service (see Connection::service_t). The steps are:
    
    - disconnect logical connection
    - wait for completion of disconnect
    - call Connection::changeProtocol() to switch to faxG3
    - wait for logical connection to re-establish

    We throw CapiWrongState whenever disconnection occurs.
*/
class Switch2FaxG3: public CallModule
{
	public:
		/** @brief Constructor. Create object.

		    @param conn reference to Connection object
		    @param faxStationID fax station ID to use
		    @param faxHeadline fax headline to use
		    @throw CapiWrongState Thrown if connection not up (thrown by base class)
		*/
		Switch2FaxG3(Connection *conn, string faxStationID, string faxHeadline) throw (CapiWrongState);

		/** @brief Do all needed steps (disconnect logical, wait, switch to fax, wait).

		    @throw CapiWrongState Thrown by Connection::changeProtocol
		    @throw CapiExternalError Thrown by Connection::changeProtocol
		    @throw CapiMsgError Thrown by Connection::changeProtocol, Connection::disconnectCall
		*/
		void mainLoop() throw (CapiWrongState, CapiExternalError, CapiMsgError);

		/** @brief Finish first wait if the logical disconnection succeeded.
		*/
		void callDisconnectedLogical();

		/** @brief Finish second wait if logical connection has been re-established
		*/
		void callConnected();


	private:     
		string faxStationID, ///< fax station ID to use
		       faxHeadline; ///< fax headline to use
};

#endif

/* History

Old Log (for new changes see ChangeLog):

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.3  2002/12/02 12:32:54  ghillie
renamed Connection::SPEECH to Connection::VOICE

Revision 1.2  2002/11/29 10:29:12  ghillie
- updated comments, use doxygen format now

Revision 1.1  2002/11/22 14:59:36  ghillie
initial checkin

*/
