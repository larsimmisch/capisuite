/** @file idlescript.h
    @brief Contains IdleScript - Implements calling of python script in regular intervals for user defined activity (e.g. sending faxes).

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

#ifndef IDLESCRIPT_H
#define IDLESCRIPT_H

#include <string>
#include "applicationexception.h"
#include "pythonscript.h"

class Capi;
class PycStringIO_CAPI;

/** @brief Thread exec handler for IdleScript class

    This is a handler which will call this->run() for the use in pthread_create().
    It will also register idlescript_cleanup_handler
*/
void* idlescript_exec_handler(void* arg);

/** @brief Thread clean handler for IdleScript class

    This is a handler which is called by pthreads at cleanup.
    It will call this->final().
*/
void idlescript_cleanup_handler(void* arg);

/** @brief Implements calling of python script in regular intervals for user defined activity (e.g. sending faxes).

    Executes a given idle script at regular intervals thus giving the user the ability to
    do arbitrary things. The main use is surely initiating outgoing calls, e.g. to send faxes.

    It creates one new thread which will execute the idle script over and over...
    
    If the script fails too often, it's deactivated. After fixing the script, it can be reactivated
    with activate().

    @author Gernot Hillier
*/

class IdleScript: public PythonScript
{
	friend void* idlescript_exec_handler(void*);
	friend void idlescript_cleanup_handler(void*);
	public:
		/** @brief Constructor. Create Object and start a detached thread

		    @param debug stream for debugging info
		    @param debug_level verbosity level for debug messages
		    @param error stream for error messages
		    @param capi reference to Capi object
		    @param idlescript file name of the python script to use as incoming script
		    @param idlescript_interval interval between two subsequent calls to the idle script in seconds
		    @param py_state thread state of the main python interpreter which must be initialized an Py_SaveThread()'d before.
		    @param cStringIO pointer to the Python cStringIO C API
		    @throw ApplicationError Thrown if thread can't be started
		*/
		IdleScript(ostream &debug, unsigned short debug_level, ostream &error, Capi *capi, string idlescript, int idlescript_interval, PyThreadState *py_state, PycStringIO_CAPI* cStringIO) throw (ApplicationError);

		/** @brief Destructor. Destruct object.
		*/
		virtual ~IdleScript(); 
		
		/** @brief terminate thread
		*/
		void requestTerminate(void);

		/** @brief reactivate the script execution in the case it was deactivated by too much errors
		*/
		void activate(void);

	private:
		/** @brief Thread body. Calls the python function idle().

		    The read Python idle script must provide a function named idle with the following signature:

		    def idle(capi):
		    	# function body

		    The parameters given to the python function are:
			- capi: reference to the capi providing the interface to the ISDN hardware (meeded for the call_*() function class)

		    The script is responsible for clearing each call it initiates, even in the exception handlers!

		    If the call is disconnected by the other party, the Python exception CallGoneError is raised and should be caught
		    by the script (don't forget to call disconnect() there).     
		    
		    If the script produces too much errors in a row, it will be deactivated. Use activate() to re-enable.

		    The python global lock will be acquired while the function runs.
		*/
		virtual void run(void) throw();
		
		PyThreadState *py_state; ///< py_state of the main python interpreter used for run().   
		string idlescript; ///< name of the python script which is called at regular intervals
		int idlescript_interval; ///< interval between subsequent executions of idle script
		Capi *capi; ///< reference to Capi object
		bool active; ///< used to disable IdleScript in case of too much errors
		
		pthread_t thread_handle; ///< handle for the created pthread thread
};

#endif

/* History

Old Log (for new changes see ChangeLog):
Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.9  2003/02/10 14:17:09  ghillie
merged from NATIVE_PTHREADS to HEAD

Revision 1.8.2.2  2003/02/10 14:04:57  ghillie
- made destructors virtual, otherwise wrong destructor is called!

Revision 1.8.2.1  2003/02/09 15:03:41  ghillie
- rewritten to use native pthread_* calls instead of CommonC++ Thread

Revision 1.8  2003/01/18 12:53:06  ghillie
- pass on reference to Python C API to PythonScript

Revision 1.7  2003/01/13 21:25:13  ghillie
- improved comment for finish flag

Revision 1.6  2003/01/06 21:02:56  ghillie
- added support for deactivating/activating script execution w/o exiting the
  thread (new method activate, comment changes)

Revision 1.5  2003/01/06 16:22:24  ghillie
- renamed terminate() to requestTerminate() to avoid name-conflict
- added finish flag

Revision 1.4  2003/01/04 16:00:53  ghillie
- log improvements: log_level, timestamp

Revision 1.3  2002/12/14 14:02:51  ghillie
- added terminate() method
- added error counting code to run() to deactivate after 10 errors

Revision 1.2  2002/12/11 13:04:35  ghillie
- minor improvements in comments, ...

Revision 1.1  2002/12/10 15:54:08  ghillie
- initial checkin, will take over functionality from FlowControl::executeIdleScript()

*/
