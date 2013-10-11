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
#include "idlescript.h"
#include "capisuitemodule.h"

void* idlescript_exec_handler(void* arg)
{
	if (!arg) {
                cerr << "FATAL ERROR: no IdleScript reference given in idlescript_exec_handler" << endl;
		exit(1);
	}
	pthread_cleanup_push(idlescript_cleanup_handler,arg);
	IdleScript *instance=static_cast<IdleScript*>(arg);
	instance->run();
	pthread_cleanup_pop(1); // run the cleanup_handler and then deregister it
	return NULL;
}

void idlescript_cleanup_handler(void* arg)
{
	if (!arg) {
                cerr << "FATAL ERROR: no IdleScript reference given in idlescript_exec_handler" << endl;
		exit(1);
	}
	IdleScript *instance=static_cast<IdleScript*>(arg);
	instance->final();
}

IdleScript::IdleScript(ostream &debug, unsigned short debug_level, ostream &error, Capi *capi, string idlescript, int idlescript_interval, PyThreadState *py_state, PycStringIO_CAPI* cStringIO) throw (ApplicationError)
:PythonScript(debug,debug_level,error,idlescript,"idle",cStringIO),idlescript_interval(idlescript_interval),py_state(py_state),capi(capi),active(true)
{
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
        int ret=pthread_create(&thread_handle, &attr, idlescript_exec_handler, this);   // start thread as detached
	if (ret)
		throw ApplicationError("error while creating thread","IdleScript::IdleScript()");

	if (debug_level>=3)
		debug << prefix() << "IdleScript created." << endl;
}

IdleScript::~IdleScript()
{
	if (debug_level>=3)
		debug << prefix() << "IdleScript deleted" << endl;
}

void
IdleScript::run() throw()
{
	int count=idlescript_interval*9,errorcount=0;
	timespec delay_time;
	delay_time.tv_sec=0; delay_time.tv_nsec=100000000;  // 100 msec
	while (1) {
		pthread_testcancel(); // cancellation point
		nanosleep(&delay_time,NULL);
		count++;
		if (active && (count>=idlescript_interval*10)) {
			count=0;
			PyObject *capi_ref=NULL;
			try {
				if (debug_level>=3)
					debug << prefix() << "executing idlescript..." << endl;
				PyEval_RestoreThread(py_state); // acquire lock, switch to right thread context

				capisuitemodule_init();

				capi_ref=PyCObject_FromVoidPtr(capi,NULL); // new ref
				if (!capi_ref)
					throw ApplicationError("unable to create CObject from Capi reference","IdleScript::run()");

				args=Py_BuildValue("(O)",capi_ref); // args = new ref
				if (!args)
					throw ApplicationError("can't build arguments","IdleScript::run()");

				PythonScript::run();

				Py_DECREF(args);
				args=NULL;

				Py_DECREF(capi_ref);
				capi_ref=NULL;

				if (PyEval_SaveThread()!=py_state) // release lock
					throw ApplicationError("can't release thread lock","IdleScript::run()");
				errorcount=0;
				if (debug_level>=3)
					debug << prefix() << "idlescript finished..." << endl;
			}
			catch (ApplicationError e) {
				errorcount++;
				error << prefix() << "IdleScript " << this << " Error occured. " << endl;
				error << prefix() << "message was: " << e << endl;
				if (errorcount>9) {
					error << prefix() << "Too much subsequent errors. Disabling idle script." << endl;
					active=false;
				}

				if (args)
					Py_DECREF(args);
				if (capi_ref)
					Py_DECREF(capi_ref);
				if (PyEval_SaveThread()!=py_state) // release lock
					throw ApplicationError("can't release thread lock","IdleScript::run()");
			}
		} 
	}
}

void
IdleScript::requestTerminate()
{
	pthread_cancel(thread_handle);
}

void
IdleScript::activate()
{
	active=true;
}

/* History

Old Log (for new changes see ChangeLog):
Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.12  2003/02/10 14:17:09  ghillie
merged from NATIVE_PTHREADS to HEAD

Revision 1.11.2.1  2003/02/09 15:03:41  ghillie
- rewritten to use native pthread_* calls instead of CommonC++ Thread

Revision 1.11  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.10  2003/01/18 12:52:50  ghillie
- pass on reference to Python C API to PythonScript

Revision 1.9  2003/01/17 15:11:34  ghillie
- added debug output for finish of idlescript

Revision 1.8  2003/01/06 21:02:01  ghillie
- won't exit if script causes too much errors - only temporarily deactivate
  script execution, can be re-enabled with activate()
- added debug output for script execution

Revision 1.7  2003/01/06 16:21:58  ghillie
- renamed terminate() to requestTerminate() to avoid endless recursion
- use finish flag instead of call to terminate() in requestTerminate() and run()

Revision 1.6  2003/01/04 16:00:53  ghillie
- log improvements: log_level, timestamp

Revision 1.5  2002/12/16 13:12:44  ghillie
- removed double output of error message

Revision 1.4  2002/12/14 14:02:22  ghillie
- added terminate() method (make terminate public)
- added error counting code to de-activate idle-script after 10 errors

Revision 1.3  2002/12/13 09:57:10  ghillie
- error message formatting done by exception classes now

Revision 1.2  2002/12/11 13:03:50  ghillie
- finished (use Thred::sleep() instead of nanosleep(), fix in if condition)

Revision 1.1  2002/12/10 15:54:08  ghillie
- initial checkin, will take over functionality from FlowControl::executeIdleScript()

*/
