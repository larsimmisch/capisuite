/*  @file callmodule.cpp
    @brief Contains CallModule - Base class for all call handling modules

    @author Gernot Hillier <gernot@hillier.de>
    $Revision: 1.6 $
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <unistd.h>
#include <sys/time.h>
#include "../backend/connection.h"
#include "callmodule.h"

CallModule::CallModule(Connection *connection, int timeout, bool DTMF_exit, bool checkConnection) throw (CapiWrongState)
:finish(false),timeout(timeout),conn(connection),DTMF_exit(DTMF_exit)
{
	if (conn)
		conn->registerCallInterface(this); // Connection needs to know who we are...
		if (checkConnection && conn->getState()!=Connection::UP)
			throw CapiWrongState("call abort detected","CallModule::CallModule()");
}

CallModule::~CallModule()
{
	if (conn)
		conn->registerCallInterface(NULL); // tell Connection that we've finished...
}

void
CallModule::callDisconnectedPhysical()
{
	finish=true;
}

void
CallModule::callDisconnectedLogical()
{
	finish=true;
}

void
CallModule::mainLoop() throw (CapiWrongState,CapiMsgError,CapiExternalError,CapiError)
{
	if (! (DTMF_exit && (conn->getDTMF()!="") ) ) {
		exit_time=getTime()+timeout;
		timespec delay_time;
		delay_time.tv_sec=0; delay_time.tv_nsec=100000000;  // 100 msec
		while(!finish && ( (timeout==-1) || (getTime() <= exit_time) ) )
			nanosleep(&delay_time,NULL);
	}
}

void
CallModule::resetTimer(int new_timeout)
{
	exit_time=getTime()+new_timeout;
	timeout=new_timeout;
}

long
CallModule::getTime()
{
	struct timeval curr_time;
	gettimeofday(&curr_time,NULL);
	return curr_time.tv_sec;
}

// will be overloaded by subclasses if necessary

void
CallModule::transmissionComplete()
{
}

void
CallModule::callConnected()
{
}

void
CallModule::alerting()
{
}

void
CallModule::dataIn(unsigned char* data, unsigned length)
{
}

void
CallModule::gotDTMF()
{
	if (DTMF_exit)
		finish=true;
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

Revision 1.2  2003/04/17 10:40:32  gernot
- support new ALERTING notification feature of backend

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.14  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.13  2002/12/09 15:43:10  ghillie
- changed WARNING to ERROR severity in exception

Revision 1.12  2002/12/05 15:06:21  ghillie
- do registering and unregistering only if conn pointer is set (used for CallOutgoing)

Revision 1.11  2002/11/29 10:27:44  ghillie
- updated comments, use doxygen format now

Revision 1.10  2002/11/25 11:56:21  ghillie
- changed semantics of timeout parameter: -1 = infinite now, 0 = 0 seconds (i.e. abort immediately)

Revision 1.9  2002/11/22 15:18:06  ghillie
- added support for DTMF_exit
- de-register Connection object uncondionally in destructor (checking for abort removed)

Revision 1.8  2002/11/21 15:34:50  ghillie
- mainLoop() doesn't return any value any more, but throws CapiWrongState when connection is lost

Revision 1.7  2002/11/21 11:37:09  ghillie
make sure that we don't use Connection object after call was finished

Revision 1.6  2002/11/15 13:51:49  ghillie
fix: call module wasn't finished when call was only connected/disconnected physically

Revision 1.5  2002/11/14 17:05:19  ghillie
major structural changes - much is easier, nicer and better prepared for the future now:
- added DisconnectLogical handler to CallInterface
- DTMF handling moved from CallControl to Connection
- new call module ConnectModule for establishing connection
- python script reduced from 2 functions to one (callWaiting, callConnected
  merged to callIncoming)
- call modules implement the CallInterface now, not CallControl any more
  => this freed CallControl from nearly all communication stuff

Revision 1.4  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.3  2002/11/12 15:52:08  ghillie
added data in handler

Revision 1.2  2002/10/31 12:40:57  ghillie
changed sleep to nanosleep to make module destruction more quick

Revision 1.1  2002/10/25 13:29:39  ghillie
grouped files into subdirectories

Revision 1.3  2002/10/23 14:25:29  ghillie
- a callmodule has to register itself at CallControl so it can be aborted if call is gone
- added distinction between "exited normally" and "aborted because call is gone" -> different results of mainLoop()

Revision 1.2  2002/10/10 12:45:40  gernot
added AudioReceive module, some small details changed

Revision 1.1  2002/10/09 14:34:22  gernot
added CallModule class as base class for all call handling modules

*/
