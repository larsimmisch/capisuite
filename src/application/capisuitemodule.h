/** @file capisuitemodule.h
    @brief Contains the Python module integration routines

    This file contains the implementation of thy python module
    capisuite which contains all commands available in python scripts
    for programming capisuite.

    There are two groups of functions: functions used from C++ to init
    and access the python module and functions used from python implementing
    the functions of the python module. 
    
    Here you'll only find the functions used from C++. If you're interested
    in the commands usable from python, please have a look at the documentation
    found in @ref python.

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

#ifndef PCAPICOMMODULE_H
#define PCAPICOMMODULE_H

#include <Python.h>
#include "applicationexception.h"

class Connection;

/** @brief Initializes and registers C implementation of python module capisuite

    This function creates a new python module named "capisuite" containing the
    functions for the control of capisuite and two exception types: CallGoneError
    and BackendError (see @ref python). Also there are three constants defined:
    SERVICE_VOICE, SERVICE_FAXG3, SERVICE_OTHER, see also Connection::service_t.

    @return <b>borrowed</b> reference to the __main__-Dictionary of the created python interpreter
    @throw ApplicationError Thrown if some step of the module initialization fails. See errormsg for details.
*/
void capisuitemodule_init() throw (ApplicationError);

/** @brief Destructor function for Connection reference given to Python scripts.

    This function will be called by Python if the given connection reference is not used any
    more in the script. This will lead to the destruction of the Connection object.

    This function has the right signature to pass as destructor function for PyCCobject_FromVoidPtr() calls.

    @param conn Connection reference
*/
void capisuitemodule_destruct_connection(void* conn);

#endif

/* History

Old Log (for new changes see ChangeLog):
Revision 1.2  2003/02/25 13:24:21  gernot
- remove old forward declaration

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.5  2002/12/07 22:36:21  ghillie
- capisuitemodule_init: doesn't return __main__ any more

Revision 1.4  2002/12/06 12:54:11  ghillie
- removed capisuitemodule_call_gone() (CallGoneException won't be thrown in
  from somewhere any more)
- added destruction function for Connection objects

Revision 1.3  2002/12/05 14:50:05  ghillie
- comment improvement

Revision 1.2  2002/12/02 12:26:51  ghillie
- update description to new behaviour of service parameter

Revision 1.1  2002/11/29 11:06:22  ghillie
renamed CapiCom to CapiSuite (name conflict with MS crypto API :-( )

Revision 1.2  2002/11/29 10:20:44  ghillie
- updated docs, use doxygen format now

Revision 1.1  2002/11/22 15:44:54  ghillie
renamed pcallcontrol.* to capicommodule.*

Revision 1.7  2002/11/18 14:21:07  ghillie
- moved global severity_t to ApplicationError::severity_t
- added throw() declarations to header files

Revision 1.6  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.5  2002/11/10 17:03:45  ghillie
now CallControl reference is passed directly to the called Pyhton functions

Revision 1.4  2002/11/06 16:16:07  ghillie
added code to raise CallGoneError in any case so the script is cancelled when the call is gone surely

Revision 1.3  2002/10/30 14:25:54  ghillie
added connect,disconnect,reject functions, changed init function to return the module dictionary

Revision 1.2  2002/10/27 12:47:20  ghillie
- added multithread support for python
- changed callcontrol reference to stay in the python namespace
- changed ApplicationError to support differen severity

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.3  2002/10/24 09:55:52  ghillie
many fixes. Works for one call now

Revision 1.2  2002/10/23 15:42:11  ghillie
- added standard headers
- changed initialization code (object references now set in extra function)
- added some missing Py_None

*/
