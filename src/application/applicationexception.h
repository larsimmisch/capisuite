/** @file applicationexception.h
    @brief Contains ApplicationError - Exception class for errors in the application layer

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

#ifndef APPLICATIONEXCEPTION_H
#define APPLICATIONEXCEPTION_H

#include <iostream>
#include <string>

using namespace std;
 
/** @brief Exception class for errors in the application layer

    Each exception gets a severity (Warning, Error or Fatal), a message and the name of the function where it occurred.

    @author Gernot Hillier
*/
class ApplicationError
{
	public:

		/** @brief Constructor. Create an object, print error message and abort if severity FATAL was chosen.

		    @param errormsg some informal message describing the error
		    @param function_name name of the function which throws this exception
		*/
		ApplicationError(string errormsg,string function_name):
		errormsg(errormsg),function_name(function_name)
		{}

		/** @brief Return nice formatted error message

                    Returns the string "Classname: error message occured in function()"

		    @return error message
		*/
		virtual string message()
		{
                	return ("ApplicationError: "+errormsg+" occured in "+function_name);
		}


	protected:
    		string errormsg;      ///< textual error message
		string function_name; ///< function/method where this error occured
};

/** @brief Overloaded operator for output of error classes
*/
inline ostream& operator<<(ostream &s, ApplicationError &e)
{
	s << e.message();
	return s;
}

#endif

/* History

Old Log (for new changes see ChangeLog):
Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.12  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.11  2002/12/13 09:57:10  ghillie
- error message formatting done by exception classes now

Revision 1.10  2002/12/10 15:03:04  ghillie
- added missing include<string>, using namespace std

Revision 1.9  2002/12/09 15:19:53  ghillie
- removed severity WARNING
- removed printing of error message in ERROR severity

Revision 1.8  2002/11/29 10:20:44  ghillie
- updated docs, use doxygen format now

Revision 1.7  2002/11/27 15:54:02  ghillie
updated docu for doxygen

Revision 1.6  2002/11/25 21:00:53  ghillie
- improved documentation, now doxygen-readabl

Revision 1.5  2002/11/18 14:21:07  ghillie
- moved global severity_t to ApplicationError::severity_t
- added throw() declarations to header files

Revision 1.4  2002/11/17 14:34:17  ghillie
small change in header description

Revision 1.3  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.2  2002/10/27 12:47:20  ghillie
- added multithread support for python
- changed callcontrol reference to stay in the python namespace
- changed ApplicationError to support differen severity

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.1  2002/10/24 09:58:12  ghillie
definition of application exceptions will stay here

*/
