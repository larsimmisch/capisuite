/*  @file calloutgoing.cpp
    @brief Contains CallOutgoingModule - Call Module for establishment of an outgoing connection and wait for successful connect

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

#include "calloutgoing.h"

CallOutgoing::CallOutgoing(Capi *capi, _cdword controller, string call_from, string call_to, Connection::service_t service, int timeout, string faxStationID, string faxHeadline, bool clir)
:CallModule(NULL,-1,false,false),capi(capi),controller(controller)
,call_from(call_from),call_to(call_to),service(service),
faxStationID(faxStationID),faxHeadline(faxHeadline),clir(clir)
,saved_timeout(timeout)
{}

void
CallOutgoing::mainLoop() throw (CapiExternalError, CapiMsgError)
{
	conn=new Connection(capi,controller,call_from,clir,call_to,service,faxStationID,faxHeadline);
	conn->registerCallInterface(this);

	// first, we have no timeout, timeout is activated in alerting()!
	CallModule::mainLoop();

	if (conn->getState()==Connection::UP)
		result=0;
	else if (finish) { // error during connection setup
		result=conn->getCause();
		if (!result)
			result=2; // no reason available
	} else { // timeout exceeded
		result=1;
		conn->disconnectCall();
		timespec delay_time;
		delay_time.tv_sec=0; delay_time.tv_nsec=100000000;  // 100 msec
		while(conn->getState()!=Connection::DOWN)
			nanosleep(&delay_time,NULL);
	}
}

void
CallOutgoing::callConnected()
{
	finish=true;
}

void
CallOutgoing::alerting()
{
	// now activate the timeout!
	resetTimer(saved_timeout);
}

Connection*
CallOutgoing::getConnection()
{
	return conn;
}

int
CallOutgoing::getResult()
{
	return result;
}

/*  History

Old Log (for new changes see ChangeLog):

Revision 1.3  2003/10/03 14:56:40  gernot
- partly implementation of a bigger semantic change: don't throw
  call finished exceptions in normal operation any longer; i.e. we only
  test for the connection at the begin of a command. This allows return
  values, e.g. for commands like capisuite.fax_receive() which were
  interrupted by an exception always in former CapiSuite versions and thus
  never returned. This is also a better and more logical use of exceptions
  IMO. ATTN: this is *far from stable*

Revision 1.2  2003/04/17 10:52:12  gernot
- timeout value is now measured beginning at the moment the other party is
  signalled

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.3  2002/12/06 13:10:34  ghillie
- corrected some wrong semantics, don't throw CapiWrongState any more
- added waiting for disconnection after timeout
- added result value for reason of abortion or success (getResult() added)
- always return Connection object, don't delete it in any case

Revision 1.2  2002/12/05 15:55:54  ghillie
- small typo fixed

Revision 1.1  2002/12/05 15:07:44  ghillie
- initial checking

*/
