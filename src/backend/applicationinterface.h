/** @file applicationinterface.h
    @brief Contains ApplicationInterface - Interface class which is implemented by the main application.

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

#ifndef APPLICATIONINTERFACE_H
#define APPLICATIONINTERFACE_H

using namespace std;

#include <iostream>

class Connection;

/** @brief Interface class which is implemented by the main application.

    This interface exposes methods for general communication between the Capi abstraction layer and the application. These methods
    are mainly used for telling the application the begin and end of an incoming call, so it can start and finish call handling
    procedures
    
    For special events during call handling, see the CallInterface.

    @author Gernot Hillier
*/
class ApplicationInterface
{
	public:
  		/** @brief Called by Capi if we get a CONNECT_IND (in case of an incoming call)
        	    
		    @param conn pointer to the according connection object
		*/
  		virtual void callWaiting (Connection *conn) = 0;
};

#endif

/* History

Old Log (for new changes see ChangeLog):
Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.7  2002/12/05 15:55:34  ghillie
- removed callCompleted(), application will self-determine when call is completed

Revision 1.6  2002/12/05 14:54:52  ghillie
- thrown away debug() method, debug stream is given in constructor now

Revision 1.5  2002/11/29 10:20:44  ghillie
- updated docs, use doxygen format now

Revision 1.4  2002/11/25 21:00:53  ghillie
- improved documentation, now doxygen-readabl

Revision 1.3  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.2  2002/10/29 14:10:42  ghillie
updated description of callCompleted: now the given reference is valid

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.9  2002/10/23 15:37:50  ghillie
typo...

Revision 1.8  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.7  2002/10/04 13:27:15  gernot
some restructuring to get it to a working state ;-)

does not do anything useful yet nor does it even compile...

Revision 1.6  2002/10/02 14:10:07  gernot
first version

Revision 1.5  2002/10/01 08:56:09  gernot
some cosmetic improvements, changes for gcc3.2

Revision 1.4  2002/09/22 14:22:53  gernot
some cosmetic comment improvements ;-)

Revision 1.3  2002/09/19 12:08:19  gernot
added magic CVS strings

* Sun Sep 15 2002 - gernot@hillier.de
- put under CVS, cvs changelog follows

* Sun May 20 2002 - gernot@hillier.de
- first version

*/
