/** @file incomingscript.h
    @brief Contains IncomingScript - Incoming call handling. One object for each incoming call is created.

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

#ifndef INCOMINGSCRIPT_H
#define INCOMINGSCRIPT_H

#include "applicationexception.h"
#include "pythonscript.h"

class Connection;
class PycStringIO_CAPI;

/** @brief Thread exec handler for IncomingScript class

    This is a handler which will call this->run() for the use in pthread_create().
    It will also register incomingscript_cleanup_handler
*/
void* incomingscript_exec_handler(void* arg);

/** @brief Thread clean handler for IncomingScript class

    This is a handler which is called by pthreads at cleanup.
    It will call this->final().
*/
void incomingscript_cleanup_handler(void* arg);

/** @brief Incoming call handling. One object for each incoming call is created.

    IncomingScript handels an incoming connection. For each connection, one object
    of it is created by FlowControl. It mainly creates a new thread with an own
    python subinterpreter, initializes the capisuitemodule, and calls run() of
    PythonScript which will execute the defined function in the script.

    @author Gernot Hillier
*/
class IncomingScript: public PythonScript
{
	friend void* incomingscript_exec_handler(void*);
	friend void incomingscript_cleanup_handler(void*);

	public:
		/** @brief Constructor. Create Object and start detached thread

		    @param debug stream for debugging info
		    @param debug_level verbosity level for debug messages
		    @param error stream for error messages
		    @param conn reference to according connection (disconnected if error occurs)
		    @param incoming_script file name of the python script to use as incoming script
		    @param cStringIO pointer to the Python cStringIO C API
		    @throw ApplicationError Thrown if thread can't be started
		*/
		IncomingScript(ostream &debug, unsigned short debug_level, ostream &error, Connection *conn, string incoming_script, PycStringIO_CAPI* cStringIO) throw (ApplicationError);

		/** @brief Destructor. Destruct object and assure the call is disconnected.
		*/
		virtual ~IncomingScript();

	private:
		/** @brief Thread body. Calls the python function callIncoming() which will handle the call.

		    Create python sub-interpreter, read script for incoming calls,

		    The read Python script for incoming calls must provide a function named callIncoming with the following signature:

		    def callIncoming(call, service, callingParty, calledParty):
		    	# function body

		    The parameters given to the python function are:
			- call: reference to the incoming call. Must be given to all capisuite-provided python functions as first parameter.
			- service (integer): service as signalled by ISDN, set to one of the SERVICE_* constants defined in capisuitemodule_init
			- callingParty (string): the number of the calling party (source of the call)
			- calledParty (string): the number of the called party (destination of the call)

		    At the moment callIncoming() is called, the call is waiting for an answer, so the first thing the script must do
		    is to call connect_*() or reject(). It must also disconnect the call in any case (even in the exception handlers!)
		    before finishing using disconnect().

		    If the call is disconnected by the other party, the Python exception CallGoneError is raised and should be caught 
		    by the script (but even there you must call disconnect()).

		    The python global lock will be acquired while the function runs.
    		*/
    		virtual void run(void) throw();

		Connection *conn; ///< reference to according connection object      
		
		pthread_t thread_handle; ///< handle for the created pthread thread
};

#endif

/* History

Old Log (for new changes see ChangeLog):
Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.5  2003/02/10 14:17:09  ghillie
merged from NATIVE_PTHREADS to HEAD

Revision 1.4.2.2  2003/02/10 14:04:57  ghillie
- made destructors virtual, otherwise wrong destructor is called!

Revision 1.4.2.1  2003/02/09 15:03:41  ghillie
- rewritten to use native pthread_* calls instead of CommonC++ Thread

Revision 1.4  2003/01/18 12:53:06  ghillie
- pass on reference to Python C API to PythonScript

Revision 1.3  2003/01/04 16:00:53  ghillie
- log improvements: log_level, timestamp

Revision 1.2  2002/12/14 14:03:27  ghillie
- added throw() declaration to run() method

Revision 1.1  2002/12/10 15:01:08  ghillie
- class IncomingScript now takes over the functionality of the old CallControl
  class defined in callcontrol.*, but uses a base class now

Revision 1.17  2002/12/09 15:24:21  ghillie
- new parameter debug to constructor
- doc changes

Revision 1.16  2002/12/07 22:31:37  ghillie
- remove unnecessary attributes py_state, py_dict, isRunning
- added attribute incoming_script

Revision 1.15  2002/12/05 15:53:41  ghillie
- began restructuring for COnnection to self-delete after getting OK from FlowControl / CallControl
- callCompleted() removed, not needed any more

Revision 1.14  2002/12/02 12:23:06  ghillie
- incoming_script is now a parameter to constructor
- service parameter now uses constants from Connection::service_t

Revision 1.13  2002/11/29 11:09:04  ghillie
renamed CapiCom to CapiSuite (name conflict with MS crypto API :-( )

Revision 1.12  2002/11/29 10:20:44  ghillie
- updated docs, use doxygen format now

Revision 1.11  2002/11/27 15:56:14  ghillie
updated comments for doxygen

Revision 1.10  2002/11/23 15:55:09  ghillie
added missing (?) include

Revision 1.9  2002/11/21 11:34:33  ghillie
- new methods final() and callCompleted()

Revision 1.8  2002/11/18 14:21:07  ghillie
- moved global severity_t to ApplicationError::severity_t
- added throw() declarations to header files

Revision 1.7  2002/11/14 17:05:19  ghillie
major structural changes - much is easier, nicer and better prepared for the future now:
- added DisconnectLogical handler to CallInterface
- DTMF handling moved from CallControl to Connection
- new call module ConnectModule for establishing connection
- python script reduced from 2 functions to one (callWaiting, callConnected
  merged to callIncoming)
- call modules implement the CallInterface now, not CallControl any more
  => this freed CallControl from nearly all communication stuff

Revision 1.6  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.5  2002/11/12 15:48:07  ghillie
added data in handler

Revision 1.4  2002/10/31 12:35:58  ghillie
added DTMF support

Revision 1.3  2002/10/30 14:24:41  ghillie
added support for python call handling before call is connected

Revision 1.2  2002/10/27 12:47:20  ghillie
- added multithread support for python
- changed callcontrol reference to stay in the python namespace
- changed ApplicationError to support differen severity

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.6  2002/10/23 15:40:15  ghillie
added python integration...

Revision 1.5  2002/10/23 14:17:41  ghillie
added registerCallModule()

Revision 1.4  2002/10/09 14:36:22  gernot
added CallModule base class for all call handling modules

Revision 1.3  2002/10/05 20:43:32  gernot
quick'n'dirty, but WORKS

Revision 1.2  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.1  2002/10/04 13:28:43  gernot
CallControll class added

*/
