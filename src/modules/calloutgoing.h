/** @file calloutgoing.h
    @brief Contains CallOutgoing - Call Module for establishment of an outgoing connection and wait for successful connect

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

#ifndef CALLOUTGOINGMODULE_H
#define CALLOUTGOINGMODULE_H

#include "callmodule.h"
#include "../backend/connection.h"


using namespace std;

/** @brief Call Module for establishment of an outgoing connection and wait for successful connect

    This module serves to initiate an outgoing call and wait for the connection
    establishment. The module can finish for three reasons:
    	- timeout for connecting exceeded
	- error during call setup
	- successful connect

    The timeout will be counted from the moment the other party is alerted,
    not from the moment we initiate the call!

    This call module does never throw CapiWrongState! see getResult() if you
    need to know if conneciton succeeded.

    @author Gernot Hillier
*/
class CallOutgoing: public CallModule
{
	public:
		/** @brief Constructor. Create object.

		    @param capi reference to object of Capi to use
		    @param controller controller number to use
		    @param call_from string containing the number to call
		    @param call_to string containing the own number to use
		    @param service service to call with as described in Connection::service_t
		    @param timeout timeout to wait for connection establishment
		    @param faxStationID fax station ID, only necessary when connecting in FAXG3 mode
		    @param faxHeadline fax headline, only necessary when connecting in FAXG3 mode
		    @param clir set to true to disable sending of own number
		*/
		CallOutgoing(Capi *capi, _cdword controller, string call_from, string call_to, Connection::service_t service, int timeout, string faxStationID, string faxHeadline, bool clir);

		/** @brief Initiate connection, wait for it to succeed

		    @throw CapiExternalError Thrown by Connection::Connection(Capi*,_cdword,string,bool,string,service_t,string,string)
		    @throw CapiMsgError Thrown by Connection::Connection(Capi*,_cdword,string,bool,string,service_t,string,string)
		*/
		void mainLoop() throw (CapiExternalError,CapiMsgError);

		/** @brief Finish if we got connection

		*/
		void callConnected();

		/** @brief activate the timeout in the moment the other party starts getting alerted
		*/
		void alerting();

		/** @brief return reference to the established connection

		    @return reference to the established connection, NULL if timout exceeded w/o successful connection
		*/
		Connection* getConnection(); 
		
		/** @brief return result of connection establishment
		
		    0: connection is up, call was successful
		    1: call timeout exceeded
		    2: connection not successful, no reason available
		    0x3301-0x34FF: connection not successful, reason given in CAPI coding. See CAPI spec for details.
		    @return result, see above
		*/                           
		int getResult();

	private:
		Connection::service_t service;   ///< service with which we should connect
		string call_from, ///< CallingPartyNumber
		       call_to, ///< CalledPartyNumber
		       faxStationID, ///< fax Station ID to use
		       faxHeadline; ///< fax headlint to use
		Capi *capi; ///< reference to object of Capi to use
		_cdword controller; ///< controller to use
		bool clir; ///< enable CLIR? (don't show own number to called party)
		int result; ///< result of the call establishment process (0=success, 1=timeout exceeded, 2=aborted w/o reason, 0x3301-0x34FF=CAPI errors)
		int saved_timeout; ///< we'll save the given timeout for later as first phase (wait for alerting) doesn't need timeout
};

#endif

/* History

Old Log (for new changes see ChangeLog):

Revision 1.2  2003/04/17 10:52:12  gernot
- timeout value is now measured beginning at the moment the other party is
  signalled

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.2  2002/12/06 13:12:23  ghillie
- mainLoop() doesn't throw CapiWrongState any more
- added getResult()

Revision 1.1  2002/12/05 15:07:44  ghillie
- initial checking

*/
