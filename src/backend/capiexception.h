/** @file capiexception.h
    @brief Contains exception classes for errors in the CAPI abstraction layer
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

#ifndef CAPIEXCEPTION_H
#define CAPIEXCEPTION_H

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

/** @brief General and base class for errors in the Capi abstraction layer.

    This is the general class for all Capi errors. It serves as base class for the more specific exceptions
    and also as one-size-fits-all throwable object if the other errors doesn't fit. ;-)

    Each exception gets a severity (Warning, Error or Fatal), a message and the name of the function where it occurred.
    If you need further data, please derive a sub-class or format it into the errormsg.

    @author Gernot Hillier
*/
class CapiError
{
	public:
		/** @brief Constructor. Create an object, print error message and abort if severity FATAL was chosen.

		    @param errormsg some informal message describing the error
		    @param function_name name of the function which throws this exception
		*/
		CapiError(string errormsg,string function_name):
		errormsg(errormsg),function_name(function_name)
		{}

		/** @brief Return nice formatted error message

                    Returns the string "Classname: error message occured in function()"
		    @return error message
		*/
		virtual string message()
		{
                	return ("CapiError: "+errormsg+" occured in "+function_name);
		}

	protected:
    		string errormsg;      ///< textual error message
		string function_name; ///< function/method where this error occured
};

/** @brief Capi Abstraction Layer exception class thrown if something should be done in a wrong state.

    This exception is thrown if the Connection is in a wrong state (see Connection::ncci_state and Connection::plci_state)
    when something connection related should be done. This usually means the call was disconnected by the other party
    and should be handled by clearing the ressources controlling the call.

    @author Gernot Hillier
*/
class CapiWrongState : public CapiError
{
	public:
		/** @brief Constructor. Create an object, print error message and abort if severity FATAL was chosen.

		    @param errormsg some informal message describing the error
		    @param function_name name of the function which throws this exception
		*/
 		CapiWrongState(string errormsg,string function_name):
			CapiError("CapiWrongstate: "+errormsg,function_name)
		{}

		/** @brief Return nice formatted error message

                    Returns the string "Classname: error message occured in function()"
		    @return error message
		*/
		virtual string message()
		{
                	return ("CapiWrongState: "+errormsg+" occured in "+function_name);
		}
};

/** @brief Capi Abstraction Layer exception class thrown if an error is indicated by Capi

    This exception serves as a way to communicate errors indicated by CAPI to the application. These
    are mostly errors indicated from CAPI_PUT_MESSAGE or *_CONF messages.

    It includes also the error code given by CAPI in the info attribute.

    @author Gernot Hillier
*/
class CapiMsgError : public CapiError
{
	public:
		/** @brief Constructor. Create an object, print error message and abort if severity FATAL was chosen.

		    @param info error code given by CAPI
		    @param errormsg informal message describing the error
		    @param function_name name of the function which throws this exception
		*/
 		CapiMsgError(unsigned info, string errormsg ,string function_name):
			CapiError(errormsg,function_name),info(info)
		{}
		
		/** @brief Return nice formatted error message

                    Returns the string "Classname: error message occured in function()"
		    @return error message
		*/
		virtual string message()
		{
			stringstream m;
			m << "CapiMsgError: " << errormsg << " (error code 0x" << hex << info << ") occured in " << function_name;
			return (m.str());
		}

	protected:
		unsigned info; ///< error code given by CAPI
};

/** @brief Capi Abstraction Layer exception class thrown if an error was caused by the application.

    This ecxeption should be raised by methods in the CAPI abstraction layer when an error was detected
    that was clearly caused by the application (e.g. giving an invalid file to send, ...).

    @author Gernot Hillier
*/
class CapiExternalError : public CapiError
{
	public:
		/** @brief Constructor. Create an object, print error message and abort if severity FATAL was chosen.

		    @param errormsg informal message describing the error
		    @param function_name name of the function which throws this exception
		*/
 		CapiExternalError(string errormsg,string function_name):
			CapiError("CapiExternalError: "+errormsg,function_name)
		{}

		/** @brief Return nice formatted error message

                    Returns the string "Classname: error message occured in function()"
		    @return error message
		*/
		virtual string message()
		{
                	return ("CapiExternalError: "+errormsg+" occured in "+function_name);
		}
};

/** @brief Overloaded operator for output of error classes
*/
inline ostream& operator<<(ostream &s, CapiError &e)
{
	s << e.message();
	return s;
}

#endif

/* History

Old Log (for new changes see ChangeLog):
Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.9  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.8  2002/12/13 09:57:10  ghillie
- error message formatting done by exception classes now

Revision 1.7  2002/12/11 13:05:34  ghillie
- minor comment improvements

Revision 1.6  2002/12/09 15:39:01  ghillie
- removed severity WARNING
- exception class doesn't print error message any more

Revision 1.5  2002/11/29 10:24:09  ghillie
- updated comments, use doxygen format now
- changed some parameter names in constructor of CapiMsgError

Revision 1.4  2002/11/27 16:00:02  ghillie
updated comments for doxygen

Revision 1.3  2002/11/19 15:57:18  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.2  2002/11/18 14:24:09  ghillie
- moved global severity_t to CapiError::severity_t
- added throw() declarations

Revision 1.1  2002/11/17 14:42:22  ghillie
initial checkin

*/
