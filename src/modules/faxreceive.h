/** @file faxreceive.h
    @brief Contains FaxReceive - Call Module for receiving an analog fax (group 3)

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

#ifndef FAXRECEIVE_H
#define FAXRECEIVE_H

#include <string>
#include "callmodule.h"

class Connection;

using namespace std;

/** @brief Call Module for receiving an analog fax (group 3).

    This module handles the reception of an analog fax (fax group 3). It starts
    the reception and waits for the end of the connection.

    Fax polling isn't supported yet.

    Fax mode must have been established before using this (by connecting in fax
    mode or switching to fax with Switch2FaxG3), otherwise an exception is
    caused.

    CapiWrongState will only be thrown if connection is not up at startup,
    not later on. We see a later disconnect as normal event, no error.

    The created file will be saved in the format received by Capi, i.e. as
    Structured Fax File (SFF).

    @author Gernot Hillier
*/
class FaxReceive: public CallModule
{
	public:
 		/** @brief Constructor. Test if we are in fax mode and create an object.

      		    @param conn reference to Connection object
		    @param file name of file to save recorded stream to
		    @throw CapiWrongState Thrown if connection not up (thrown by base class)
		    @throw CapiExternalError Thrown if we are not in fax mode.
  		*/
		FaxReceive(Connection *conn, string file) throw (CapiWrongState,CapiExternalError);

 		/** @brief Start file reception, wait for disconnect and stop the reception afterwards
		
		    @throw CapiExternalError Thrown by Connection::start_file_reception. See there for explanation.
		    @throw CapiWrongState Thrown if connection is not up at start of transfer (thrown by Connection::start_file_reception)
  		*/
		void mainLoop() throw (CapiWrongState,CapiExternalError);

 		/** @brief finish main loop if file is completely received
  		*/
		void transmissionComplete();
	
	private:
		string file; ///< file name to save file to
};

#endif

/* History

Old Log (for new changes see ChangeLog):

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.11  2002/12/13 11:47:40  ghillie
- added comment about fax polling

Revision 1.10  2002/11/29 10:27:44  ghillie
- updated comments, use doxygen format now

Revision 1.9  2002/11/25 21:00:53  ghillie
- improved documentation, now doxygen-readabl

Revision 1.8  2002/11/25 11:58:04  ghillie
- test for fax mode before receiving now

Revision 1.7  2002/11/21 15:32:40  ghillie
- moved code from constructor/destructor to overwritten mainLoop() method

Revision 1.6  2002/11/19 15:57:19  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.5  2002/11/14 17:05:19  ghillie
major structural changes - much is easier, nicer and better prepared for the future now:
- added DisconnectLogical handler to CallInterface
- DTMF handling moved from CallControl to Connection
- new call module ConnectModule for establishing connection
- python script reduced from 2 functions to one (callWaiting, callConnected
  merged to callIncoming)
- call modules implement the CallInterface now, not CallControl any more
  => this freed CallControl from nearly all communication stuff

Revision 1.4  2002/11/13 15:26:28  ghillie
removed unnecessary member attribute filename

Revision 1.3  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.2  2002/10/29 14:28:22  ghillie
added stop_file_* calls to make sure transmission is cancelled when it's time...

Revision 1.1  2002/10/25 13:29:39  ghillie
grouped files into subdirectories

Revision 1.7  2002/10/23 14:34:26  ghillie
call modules must register itself at CallControl now

Revision 1.6  2002/10/23 09:46:08  ghillie
changed to fit into new architecture

*/
