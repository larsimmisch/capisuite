/** @file main.cpp
    @brief Contains main().

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

#include "application/capisuite.h"
#include "application/applicationexception.h"
#include "backend/capiexception.h"

/** @brief main application function

    This is the main() of CapiSuite.
*/
int main(int argc, char** argv)
{
	CapiSuite cs(argc,argv);
	cs.mainLoop();
}

/*  History

Old Log (for new changes see ChangeLog):
Revision 1.1.1.1  2003/02/19 08:19:52  gernot
initial checkin of 0.4

Revision 1.13  2003/02/05 15:59:38  ghillie
- moved error handling to capisuite.cpp, so it's logged

Revision 1.12  2003/01/19 16:36:07  ghillie
- added catch blocks

Revision 1.11  2003/01/07 14:48:44  ghillie
- added support for commandline parsing (only --help and --config yet)

Revision 1.10  2003/01/05 12:26:19  ghillie
- renamed fro capisuite.cpp to main.cpp
- moved all code to the CapiSuite class (application/capisuite.*)

Revision 1.9  2003/01/04 15:54:18  ghillie
- append log files, don't recreate them on every new start
- support for log_level
- added timestamp to log messages

Revision 1.8  2002/12/18 14:35:39  ghillie
- added setListenFaxG3(). Oops ;)

Revision 1.7  2002/12/16 13:10:41  ghillie
- added support for using log files

Revision 1.6  2002/12/14 14:00:04  ghillie
- added code for configfile parsing

Revision 1.5  2002/12/13 09:22:19  ghillie
- use config.h from automake/autoconf
- error classes support direct output now using operator<<
- began implementing support for config file

Revision 1.4  2002/12/09 15:19:08  ghillie
- given debug and error streams to FlowControl and Capi objects

Revision 1.3  2002/12/05 14:47:42  ghillie
- small changes in the constructor parameters for Capi and FlowControl, added call to capi->registerApplicationInterface()

Revision 1.2  2002/12/02 12:12:56  ghillie
added script names to FlowControl contructor parameters

Revision 1.1  2002/11/29 11:06:22  ghillie
renamed CapiCom to CapiSuite (name conflict with MS crypto API :-( )

Revision 1.5  2002/11/29 10:20:44  ghillie
- updated docs, use doxygen format now

Revision 1.4  2002/11/25 20:55:27  ghillie
-changed to set Listen on all controllers

Revision 1.3  2002/11/20 17:14:03  ghillie
fixed small scope problem

Revision 1.2  2002/11/19 15:57:18  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.1  2002/10/25 13:23:38  ghillie
renamed main.cpp

Revision 1.8  2002/10/09 11:18:59  gernot
cosmetic changes (again...) and changed info function of CAPI class

Revision 1.7  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.6  2002/10/04 13:27:15  gernot
some restructuring to get it to a working state ;-)

does not do anything useful yet nor does it even compile...

Revision 1.5  2002/09/22 20:06:36  gernot
deleted autocreated file

Revision 1.4  2002/09/22 14:55:21  gernot
adding audio send module

Revision 1.3  2002/09/22 14:22:53  gernot
some cosmetic comment improvements ;-)

Revision 1.2  2002/09/19 12:08:19  gernot
added magic CVS strings

* Sun Sep 15 2002 - gernot@hillier.de
- put under CVS, cvs log follows above

* Sun May 20 2002 - gernot@hillier.de
- first version

*/
