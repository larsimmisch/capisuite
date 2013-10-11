/*  @file incomingscript.cpp
    @brief Contains IncomingScript - Incoming call handling. One object for each incoming call is created.

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

#include <Python.h>
#include "incomingscript.h"
#include "../modules/disconnectmodule.h"
#include "capisuitemodule.h"

#define TEMPORARY_FAILURE 0x34A9    // see ETS 300 102-1, Table 4.13 (cause information element)
       
void* incomingscript_exec_handler(void* arg)
{
	if (!arg) {
                cerr << "FATAL ERROR: no IncomingScript reference given in incomingscript_exec_handler" << endl;
		exit(1);
	}
	pthread_cleanup_push(incomingscript_cleanup_handler,arg);
	IncomingScript *instance=static_cast<IncomingScript*>(arg);
	instance->run();
	pthread_cleanup_pop(1); // run the cleanup_handler and then deregister it
	return NULL;
}

void incomingscript_cleanup_handler(void* arg)
{
	if (!arg) {
                cerr << "FATAL ERROR: no IncomingScript reference given in incomingscript_exec_handler" << endl;
		exit(1);
	}
	IncomingScript *instance=static_cast<IncomingScript*>(arg);
	instance->final();
}

IncomingScript::IncomingScript(ostream &debug, unsigned short debug_level, ostream &error, Connection *conn, string incoming_script, PycStringIO_CAPI* cStringIO) throw (ApplicationError)
:PythonScript(debug,debug_level,error,incoming_script,"callIncoming",cStringIO),conn(conn)
{
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
        int ret=pthread_create(&thread_handle, &attr, incomingscript_exec_handler, this);   // start thread as detached
	if (ret)
		throw ApplicationError("error while creating thread","PythonScript::PythonScript()");

	if (debug_level>=2)
		debug << prefix() << "Connection " << conn << " created IncomingScript" << endl;
}

IncomingScript::~IncomingScript()
{
	if (conn) {
		error << prefix() << "Warning: Connection still established in IncomingScript destructor. Disconnecting." << endl;
		try {
			DisconnectModule active(conn,TEMPORARY_FAILURE);
			active.mainLoop();
		}
		catch (CapiError e) {
			error << prefix() << "ERROR: disconnection also failed. Too bad..." << endl;
		}
		delete conn;
	}
	if (debug_level>=2)
		debug << prefix() << "IncomingScript deleted" << endl;
}

void
IncomingScript::run() throw()
{
	PyObject *conn_ref=NULL;
	PyThreadState *py_state=NULL;

	try {
		// thread safe Python init, taken out of PyApache 4.26
		PyEval_AcquireLock();

		if (!(py_state=Py_NewInterpreter() )) {
			PyEval_ReleaseLock();
			capisuitemodule_destruct_connection(conn);
			throw ApplicationError("error while creating new python interpreter","IncomingScript::run()");
		}

		capisuitemodule_init();

		conn_ref=PyCObject_FromVoidPtr(conn,capisuitemodule_destruct_connection); // new ref
		if (!conn_ref) {
			capisuitemodule_destruct_connection(conn);
			throw ApplicationError("unable to create CObject from Connection reference","IncomingScript::run()");
		}

		args=Py_BuildValue("Oiss",conn_ref,conn->getService(),conn->getCallingPartyNumber().c_str(),conn->getCalledPartyNumber().c_str());
		if (!args)
			throw ApplicationError("error during argument building","IncomingScript::run()");

		PythonScript::run();

		Py_DECREF(args);
		args=NULL;

		Py_DECREF(conn_ref);
		conn_ref=NULL;
		conn=NULL; // Connection object will be deleted by Python destruction handler...

		Py_EndInterpreter(py_state);
		py_state=NULL;
		PyEval_ReleaseLock(); // release lock
	}
	catch(ApplicationError e) {
		error << prefix() << "Error occured. message was: " << e << endl;

		if (args)
			Py_DECREF(args);
		if (conn_ref) {
			Py_DECREF(conn_ref);
			conn=NULL;
		}
		if (py_state) {
			Py_EndInterpreter(py_state);
			py_state=NULL;
			PyEval_ReleaseLock();
		}
	}
}

/* History

Old Log (for new changes see ChangeLog):
Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.9  2003/02/10 14:17:09  ghillie
merged from NATIVE_PTHREADS to HEAD

Revision 1.8.2.1  2003/02/09 15:03:41  ghillie
- rewritten to use native pthread_* calls instead of CommonC++ Thread

Revision 1.8  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.7  2003/01/19 12:08:47  ghillie
- changed some debug_levels

Revision 1.6  2003/01/18 12:53:06  ghillie
- pass on reference to Python C API to PythonScript

Revision 1.5  2003/01/04 16:00:53  ghillie
- log improvements: log_level, timestamp

Revision 1.4  2002/12/14 14:03:27  ghillie
- added throw() declaration to run() method

Revision 1.3  2002/12/13 09:57:10  ghillie
- error message formatting done by exception classes now

Revision 1.2  2002/12/10 15:52:32  ghillie
- removed debug output

Revision 1.1  2002/12/10 15:01:08  ghillie
- class IncomingScript now takes over the functionality of the old CallControl
  class defined in callcontrol.*, but uses a base class now

Revision 1.29  2002/12/09 15:23:49  ghillie
- moved start() out of constructor in creator (was unspecified this way!)
- moved disconnection to destructor so it's assured it happens
- exception severity cleanup (no more WARNING exceptions)
- python reference counting cleanup
- added debug stream as constructor parameter, debug output improvement

Revision 1.28  2002/12/07 22:30:48  ghillie
- removed copying of filename to new char*, used const_cast from (const char*)
  to (char*) instead
- moved python initialization code from constructor to run(), makes some
  attributes obsolete
- getting __main__ namespace now taken out of capisuitemodule_init(), done
  here instead
- use DisconnectModule for error handling now

Revision 1.27  2002/12/06 15:23:14  ghillie
- wait for successful disconnection when an error in the script occured

Revision 1.26  2002/12/06 12:50:02  ghillie
- passed the destruction function capisuitemodule_destruct_connection to the PyCObject containing connection reference

Revision 1.25  2002/12/05 15:52:48  ghillie
- begin restructuring for self deletion of Connection object after it gets its OK from CallControl/FlowControl

Revision 1.24  2002/12/05 14:48:25  ghillie
- cleaned up some python reference counting

Revision 1.23  2002/12/02 12:21:56  ghillie
- incoming script name is now a parameter to constructor, not #define'd any more
- service parameter to python script now uses constants defined in Connection::service_t
- SEGV FIX: isRunning is now set to false before ending Python interpreter in run(), callCompleted() acquires
  python global lock _before_ reading isRunning -> race condition fixed
- exception handler in run() ends python interpreter correctly now

Revision 1.22  2002/11/29 11:09:04  ghillie
renamed CapiCom to CapiSuite (name conflict with MS crypto API :-( )

Revision 1.21  2002/11/29 10:20:44  ghillie
- updated docs, use doxygen format now

Revision 1.20  2002/11/25 11:44:48  ghillie
removed CIP value from application, use service type instead

Revision 1.19  2002/11/23 15:54:40  ghillie
pcallcontrol was renamed to capicommodule

Revision 1.18  2002/11/21 11:34:08  ghillie
- changed Reject cause when we have a problem from "Destination Out Of Order" to "Temporary Failure"
- moved Py_EndInterpreter from destructor to run()
- new method callCompleted() which throws CallGoneError into Python
- new method final() which is called automatically after thread has finished, now CallControl objects will delete themselves
  to allow cleanup routines in Python scripts which may take some time to finish w/o freezing the whole application

Revision 1.17  2002/11/20 17:16:24  ghillie
- SEGV-Fix: CallGoneError only triggered if python script is still running in CallControl::~CallControl
- changed sleep in mainLoop() to nanosleep
- small typo fixed

Revision 1.16  2002/11/19 15:57:18  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.15  2002/11/18 14:21:07  ghillie
- moved global severity_t to ApplicationError::severity_t
- added throw() declarations to header files

Revision 1.14  2002/11/14 17:05:19  ghillie
major structural changes - much is easier, nicer and better prepared for the future now:
- added DisconnectLogical handler to CallInterface
- DTMF handling moved from CallControl to Connection
- new call module ConnectModule for establishing connection
- python script reduced from 2 functions to one (callWaiting, callConnected
  merged to callIncoming)
- call modules implement the CallInterface now, not CallControl any more
  => this freed CallControl from nearly all communication stuff

Revision 1.13  2002/11/13 15:21:22  ghillie
added some error handling for python states

Revision 1.12  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.11  2002/11/12 15:47:27  ghillie
added dataIn-handler

Revision 1.10  2002/11/10 17:02:22  ghillie
changed to pass CallControl reference to the called python functions

Revision 1.9  2002/11/07 08:19:04  ghillie
some improvements and fixes in Python global lock and thread state handling

Revision 1.8  2002/11/06 16:16:07  ghillie
added code to raise CallGoneError in any case so the script is cancelled when the call is gone surely

Revision 1.7  2002/10/31 12:35:58  ghillie
added DTMF support

Revision 1.6  2002/10/30 16:05:20  ghillie
cosmetic fixes...

Revision 1.5  2002/10/30 14:24:41  ghillie
added support for python call handling before call is connected

Revision 1.4  2002/10/30 10:45:51  ghillie
added #define for value which should go to config file later

Revision 1.3  2002/10/29 14:06:36  ghillie
several fixes in run method

Revision 1.2  2002/10/27 12:47:20  ghillie
- added multithread support for python
- changed callcontrol reference to stay in the python namespace
- changed ApplicationError to support differen severity

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.9  2002/10/24 09:55:52  ghillie
many fixes. Works for one call now

Revision 1.8  2002/10/23 15:40:15  ghillie
added python integration...

Revision 1.7  2002/10/10 12:45:40  gernot
added AudioReceive module, some small details changed

Revision 1.6  2002/10/09 14:36:22  gernot
added CallModule base class for all call handling modules

Revision 1.5  2002/10/09 11:18:59  gernot
cosmetic changes (again...) and changed info function of CAPI class

Revision 1.4  2002/10/05 20:43:32  gernot
quick'n'dirty, but WORKS

Revision 1.3  2002/10/05 13:53:00  gernot
changed to use thread class of CommonC++ instead of the threads-package
some cosmetic improvements (indentation...)

Revision 1.2  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.1  2002/10/04 13:28:43  gernot
CallControll class added

*/
