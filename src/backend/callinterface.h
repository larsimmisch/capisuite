/** @file callinterface.h
    @brief Contains CallInterface - Interface class for all signals specific to a certain call.

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

#ifndef CALLINTERFACE_H
#define CALLINTERFACE_H

#include <string>

using namespace std;

/** @brief Interface class for all signals specific to a certain call.

    While ApplicationInterface contains the methods to inform the application about general events, this
    interface has all the signals which describe events of a certain connection like DTMF signal received,
    call is disconnected logical, etc.

    The application is supposed to create objects for each call which implement this interface and register
    them with Connection::registerCallInterface(). It's possible to use different modules for different tasks
    during one connection and to dynamically register/unregister them. If no object is registered, the callbacks
    are simply not called. However, there are certain events which need a registered CallInterface implementing 
    object - otherwise Connection will throw exceptions.

    @author Gernot Hillier
*/
class CallInterface
{
	public:
		/** @brief Called if the other party is alerted, i.e. it has started "ringing" there
		*/
		virtual void alerting (void) = 0;

		/** @brief Called if the connection is completely established (physical + logical)
		*/
  		virtual void callConnected (void) = 0;

		/** @brief called if logical connection is finished
		*/
  		virtual void callDisconnectedLogical (void) = 0;

		/** @brief called if physical connection is finished.

		    This is called when the connection has been cleared down completely.
		    
		    Attention: You must delete the Connection object yourself if you don't need
		    it any more!
		*/
  		virtual void callDisconnectedPhysical (void) = 0;

  		/** @brief called if the file requested for sending is sent completely
		*/
  		virtual void transmissionComplete (void) = 0;

  		/** @brief called by Connection object if DMTF characters were received.

		    It is necessary to enable DTMF receiving with Connection::enableDTMF
		    before any DTMFs are signalled. DTMF chars can be read with Connection::getDTMF().
		*/
		virtual void gotDTMF (void) = 0;

  		/** @brief called by Connection object for each received data packet.

		    You can either use this to save your data manually and/or tell connection
		    to save it to a file (with start_file_reception)() )

		    But please not that this is a performance issue: calling an application function
		    for each received function should only be done if really necessary.

      		    @param data pointer to data as received by CAPI
		    @param length length of data in bytes
		*/
		virtual void dataIn (unsigned char* data, unsigned length) = 0;
};

#endif

/* History

Old Log (for new changes see ChangeLog):
Revision 1.2  2003/04/17 10:39:42  gernot
- support ALERTING notification (to know when it's ringing on the other side)
- cosmetical fixes in capi.cpp

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.9  2002/12/06 12:55:04  ghillie
- updated docs

Revision 1.8  2002/11/29 10:20:44  ghillie
- updated docs, use doxygen format now

Revision 1.7  2002/11/27 15:58:13  ghillie
updated comments for doxygen

Revision 1.6  2002/11/15 13:49:10  ghillie
fix: callmodule wasn't aborted when call was only connected/disconnected physically

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

Revision 1.3  2002/11/12 15:48:54  ghillie
added data in handler

Revision 1.2  2002/10/31 12:37:34  ghillie
added DTMF support

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.5  2002/10/23 15:40:51  ghillie
typo...

Revision 1.4  2002/10/09 14:36:22  gernot
added CallModule base class for all call handling modules

Revision 1.3  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.2  2002/10/04 13:27:15  gernot
some restructuring to get it to a working state ;-)

does not do anything useful yet nor does it even compile...

Revision 1.1  2002/10/02 14:10:07  gernot
first version

*/
