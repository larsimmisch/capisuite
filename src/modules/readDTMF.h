/** @file readDTMF.h
    @brief Contains ReadDTMF - Call Module for waiting for DTMF signals

    @author Gernot Hillier <gernot@hillier.de>
    $Revision: 1.4 $
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef READDTMF_H
#define READDTMF_H

#include "callmodule.h"

class Connection;

/** @brief Call Module for waiting for DTMF signals

    This module allows the user to specify how much DTMF digits he wants to
    read and how long to wait for them. It doesn't do the actual read, just
    waits for the given conditions to be fulfilled.

    To use it, create an object and call mainLoop(). After mainLoop() finished,
    call Connection::getDTMF() to read the received signals.

    CapiWrongState will only be thrown if connection is not up at startup,
    not later on. We see a later disconnect as normal event, no error.

    @author Gernot Hillier

*/
class ReadDTMF: public CallModule
{
	public:
		/** @brief Constructor. Create Object and read the current digit count from Connection.

		    @param conn reference to Connection object
		    @param timeout timeout in seconds after which reading is terminated (only terminates when min_digits are reached!), restarts after each digit
		    @param min_digits minimum number of digits which must be read in ANY case without respect to timout. Only set to value >0 if you're sure the user will input a digit.
		    @param max_digits maximum number of digits to read, we abort immediately if this number is reached (0=infinite, only timeout counts)
		    @throw CapiWrongState Thrown if connection not up (thrown by base class)
		*/
		ReadDTMF(Connection *conn, int timeout, int min_digits, int max_digits) throw (CapiWrongState);

		/** @brief mainLoop: Waits until the given conditions (see constructor) have been fulfilled

		    The module will finish if one of these conditions are true:

                    - max_digits is fulfilled
		    - timeout was reached AND min_digits is fulfilled
  		*/
		void mainLoop() throw ();

		/** @brief finish if max_digits is reached, otherwise restart timeout when DTMF signal is received
  		*/
		void gotDTMF();

		/** @brief set call_finished

		This method overwrites CallModule::callDisconnectedLogical and sets the flag call_finished
		in addition to the other one. This is needed for our mainLoop to differ between a received
		DTMF (then we should continue) and a disconnect (then we should really finish)
		*/
		void callDisconnectedLogical();

		/** @brief set call_finished

		This method overwrites CallModule::callDisconnectedPhysical and sets the flag call_finished
		in addition to the other one. This is needed for our mainLoop to differ between a received
		DTMF (then we should continue) and a disconnect (then we should really finish)
		*/
		void callDisconnectedPhysical();

	private:
		int digit_count, ///< save the current number of digits in receive buffer
		    min_digits, ///< save min_digits parameter
		    max_digits; ///< save max_digits parameter
		bool call_finished; ///< set additionally at disconnect as CallModule::finish is used otherwise here
};

#endif

/* History

Old Log (for new changes see ChangeLog):

Revision 1.2  2003/10/03 14:56:40  gernot
- partly implementation of a bigger semantic change: don't throw
  call finished exceptions in normal operation any longer; i.e. we only
  test for the connection at the begin of a command. This allows return
  values, e.g. for commands like capisuite.fax_receive() which were
  interrupted by an exception always in former CapiSuite versions and thus
  never returned. This is also a better and more logical use of exceptions
  IMO. ATTN: this is *far from stable*

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.2  2002/11/29 10:28:34  ghillie
- updated comments, use doxygen format now
- removed unnecessary attribute again

Revision 1.1  2002/11/25 11:42:07  ghillie
initial checkin

*/
