/** @file faxsend.h
    @brief Contains FaxSend - Call Module for sending an analog fax (group 3)

    @author Gernot Hillier <gernot@hillier.de>
    $Revision: 1.1 $
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FAXSEND_H
#define FAXSEND_H

#include <string>
#include "callmodule.h"

class Connection;

using namespace std;

/** @brief Call Module for sending an analog fax (group 3).

    This module handles the send of an analog fax (fax group 3). It starts the send and waits for the end of the connection.
    
    Fax polling isn't supported yet.

    Fax mode must have been established before using this (by connecting in fax mode or switching to fax with Switch2FaxG3),
    otherwise an exception is caused.
    
    The given file must be in the format used by Capi, i.e. Structured Fax File (SFF).

    @author Gernot Hillier
*/
class FaxSend: public CallModule
{
	public:
 		/** @brief Constructor. Test if we are in fax mode and create an object.

      		    @param conn reference to Connection object
		    @param file name of file to send
		    @throw CapiExternalError Thrown if we are not in fax mode.
  		*/
		FaxSend(Connection *conn, string file) throw (CapiExternalError);

 		/** @brief Start file send, wait for disconnect and stop the send afterwards
		
		    @throw CapiWrongState Thrown when disconnection takes place.
		    @throw CapiExternalError Thrown by Connection::start_file_transmission. See there for explanation.
    		    @throw CapiMsgError Thrown by Connection::start_file_transmission. See there for explanation.
  		*/
		void mainLoop() throw (CapiWrongState, CapiExternalError, CapiMsgError);

 		/** @brief finish main loop if file is completely sent
  		*/
		void transmissionComplete();

	private:
		string file; ///< file name to send
};

#endif

/* History

$Log: faxsend.h,v $
Revision 1.1  2003/02/19 08:19:53  gernot
Initial revision

Revision 1.1  2002/12/13 11:44:34  ghillie
added support for fax send

*/
