/** @file callmodule.h
    @brief Contains CallModule - Base class for all call handling modules

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

#ifndef CALLMODULE_H
#define CALLMODULE_H

#include "../backend/callinterface.h"
#include "../backend/capiexception.h"

class Connection;

/** @brief Base class for all call handling modules

    This class implements the CallInterface. It is the base class for all special call handling modules like FaxReceive/FaxSend, AudioReceive/AudioSend, etc.
    It contains basic code for registering with the according Connection object, realizing exits because of timeouts, received DTMF and call clearing from the other party.

    The general usage is: create a CallModule object, then run mainLoop(). mainLoop() will exit if necessary.

    To be able to recognize call clearing of the other party, the CapiWrongState exception is used. It is triggered by mainLoop().

    Sub classes will mainly overwrite mainLoop() and the other signals they need for their tasks.

    If you don't change the semantics in the sub classes, the module will terminate when at least one of the following events occurs:
    	- logical connection is ended
	- physical connection is ended
	- DTMF code is received (depending on the value of DTMF_exit given in the constructor)

    @author Gernot Hillier
*/
class CallModule: public CallInterface
{
	public:
 		/** @brief Constructor. Register this module at the according Connection object

      		    @param connection reference to Connection object
		    @param timeout timeout for this module in seconds (only considered in mainLoop!), -1=infinite (default)
		    @param DTMF_exit if this is set to true, then the current module is exited if we receive a DTMF tone
  		*/
		CallModule(Connection* connection, int timeout=-1, bool DTMF_exit=false);

 		/** @brief Destructor. Deregister this module at the according Connection object.
		*/
		~CallModule();

 		/** @brief Waits in busy loop until the module is completed.

                    Waits in a busy loop (sleeping 100 msecs between each iteration) until a DTMF signal is reached (if enabled) or the timeout is reached (if enabled).

		    This method will likely be overwritten in each sub class. You can call CallModule::mainLoop() there to implement busy loops.
		    @throw CapiWrongState Something is tried in a wrong connection state. This usually means our call was finished (raised directly).
		    @throw CapiMsgError A CAPI function hasn't succeeded for some reason (not thrown by CallModule, but may be thrown in subclasses).
		    @throw CapiExternalError A given command didn't succeed for a reason not caused by the CAPI (not thrown by CallModule, but may be thrown in subclasses)
  		*/
		virtual void mainLoop() throw (CapiWrongState, CapiMsgError, CapiExternalError);

 		/** @brief empty here.

		    empty function to overwrite if necessary
  		*/
		virtual void transmissionComplete(void);

		/** @brief empty here.

		    empty function to overwrite if necessary
		*/
  		virtual void alerting (void);

		/** @brief empty here.

		    empty function to overwrite if necessary
		*/
  		virtual void callConnected (void);

		/** @brief abort current modul if logical connection is lost.

		    will abort currently running module. May be overwritten with complete new behaviour if needed.
		*/
  		virtual void callDisconnectedLogical (void);

		/** @brief abort current module if physical connection is lost.

 		    will abort currently running module. Only overwrite if you know what you do!
		*/
  		virtual void callDisconnectedPhysical (void);

		/** @brief finish current module if DTMF is received.

		    finish current module if DTMF_exit was enabled
		*/
		virtual void gotDTMF (void);

  		/** @brief empty here.

		    empty function to overwrite if necessary
		*/
		virtual void dataIn (unsigned char* data, unsigned length);

	protected:
 		/** @brief get the current time in # of seconds sinc 1/1/1970
  		*/
		virtual long getTime();

 		/** @brief restart the timer with new timeout value
  		*/
		void resetTimer(int new_timeout);

		bool DTMF_exit; ///< if set to true, we will finish when we receive a DTMF signal
		bool finish;  ///< set this if the module should exit nicely for any reason
		bool abort;   ///< set this for hard exit because connection is lost, causes CapiWrongState to be throwed in mainLoop
		Connection* conn; ///< reference to the according Connection object
		long exit_time; ///< time when the timeout should occur
		int timeout; ///< timeout period in seconds
};

#endif

/* History

$Log: callmodule.h,v $
Revision 1.3  2003/04/17 10:40:32  gernot
- support new ALERTING notification feature of backend

Revision 1.2  2003/03/06 09:35:10  gernot
- fixed typo

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.12  2002/12/06 13:08:30  ghillie
minor doc change

Revision 1.11  2002/11/29 10:27:44  ghillie
- updated comments, use doxygen format now

Revision 1.10  2002/11/25 21:00:53  ghillie
- improved documentation, now doxygen-readabl

Revision 1.9  2002/11/25 11:56:21  ghillie
- changed semantics of timeout parameter: -1 = infinite now, 0 = 0 seconds (i.e. abort immediately)

Revision 1.8  2002/11/22 15:18:06  ghillie
- added support for DTMF_exit
- de-register Connection object uncondionally in destructor (checking for abort removed)

Revision 1.7  2002/11/21 15:34:50  ghillie
- mainLoop() doesn't return any value any more, but throws CapiWrongState when connection is lost

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

Revision 1.4  2002/11/13 15:25:08  ghillie
fixed small typo

Revision 1.3  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.2  2002/11/12 15:52:08  ghillie
added data in handler

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
