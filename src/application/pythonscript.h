/** @file pythonscript.h
    @brief Contains PythonScript - Read a python script and call a function in own thread

    @author Gernot Hillier <gernot@hillier.de>
    $Revision: 1.5 $
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PYTHONSCRIPT_H
#define PYTHONSCRIPT_H

#include <Python.h>

#include "../../config.h"
#ifdef HAVE_OSTREAM
  #include <ostream>
#else
  #include <ostream.h>
#endif
#include "applicationexception.h"
class PycStringIO_CAPI;

using namespace std;

/** @brief Read a python script and call a function

    This class reads a given python script which
    must define one function with given name. This function is called
    with arbitrary parameters.

    @author Gernot Hillier
*/
class PythonScript
{      
	public:
		/** @brief Constructor. Create Object.

		    @param debug stream for debugging info
		    @param debug_level verbosity level for debug messages
		    @param error stream for error messages
		    @param filename file name of the python script to read
		    @param functionname name of the function to call
		    @param cStringIO pointer to the Python cStringIO C API
		*/
		PythonScript(ostream &debug, unsigned short debug_level, ostream &error, string filename, string functionname, PycStringIO_CAPI* cStringIO);

		/** @brief Destructor.
		*/
		virtual ~PythonScript();

	protected:
		/** @brief Reads the given python script and calls the given function.

		    The arguments for the function must be given in the constructor.

		    @throw ApplicationError Thrown when script can't be executed for any reason.
		*/
		virtual void run() throw (ApplicationError);

		/** @brief Called by pscript_cleanup_handler(), will delete the current object.
		*/
		virtual void final();
		
  		/** @brief return a prefix containing this pointer, Python script name and date for log messages
		    @param verbose controls verbosity, default is true which means to log filename etc., false means only this pointer

      		    @return constructed prefix as stringstream
  		*/
		string prefix(bool verbose=true);

		string filename, ///< name of the python script to read
		       functionname; ///< name of the function to call
		PyObject *args; ///< python tuple containing the args for the called python function
		ostream &debug, ///< debug stream
			&error; ///< error stream
		unsigned short debug_level; ///< debug level 
		PycStringIO_CAPI* cStringIO; ///< holds a pointer to the Python cStringIO C API
};

#endif

/* History

Old Log (for new changes see ChangeLog):
Revision 1.2  2003/03/21 23:09:59  gernot
- included autoconf tests for gcc-2.95 problems so that it will compile w/o
  change for good old gcc-2.95 and gcc3

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.6  2003/02/10 14:17:09  ghillie
merged from NATIVE_PTHREADS to HEAD

Revision 1.5.2.2  2003/02/10 14:04:57  ghillie
- made destructors virtual, otherwise wrong destructor is called!

Revision 1.5.2.1  2003/02/09 15:03:42  ghillie
- rewritten to use native pthread_* calls instead of CommonC++ Thread

Revision 1.5  2003/01/18 12:55:39  ghillie
- run handles python script errors now on its own and prints tracebacks
  to error log file correctly (solves TODO)

Revision 1.4  2003/01/04 16:00:53  ghillie
- log improvements: log_level, timestamp

Revision 1.3  2002/12/14 14:04:20  ghillie
- run throws ApplicationError now so that derived classes can catch
  and handle it on their behalf

Revision 1.2  2002/12/10 15:05:45  ghillie
- finished pythonscript class definition

Revision 1.1  2002/12/09 18:07:59  ghillie
- initial checkin, not finished!

*/
