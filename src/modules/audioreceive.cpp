/*  @file audioreceive.cpp
    @brief Contains AudioReceive - Call Module for receiving audio.

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

#define conf_silence_limit 10

#include "../backend/connection.h"
#include "audioreceive.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* Lookup table to reverse the bit order of a byte. ie MSB become LSB */
/* taken from Sox 12.17.3 */
unsigned char cswap[256] = {
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0,
  0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
  0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4,
  0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
  0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
  0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA,
  0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6,
  0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
  0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1,
  0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9,
  0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
  0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD,
  0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3,
  0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
  0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7,
  0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF,
  0x3F, 0xBF, 0x7F, 0xFF
};


AudioReceive::AudioReceive(Connection *conn, string file, int timeout, int silence_timeout, bool DTMF_exit) throw (CapiExternalError,CapiWrongState)
	:CallModule(conn, timeout, DTMF_exit),silence_count(0),file(file),start_time(0),end_time(0),
	silence_timeout(silence_timeout*8000) // ISDN audio sample rate = 8000Hz
{
	if (conn->getService()!=Connection::VOICE)
	 	throw CapiExternalError("Connection not in speech mode","AudioReceive::AudioReceive()");
}

void
AudioReceive::mainLoop() throw (CapiWrongState,CapiExternalError)
{
	start_time=getTime();
	if (!(DTMF_exit && (!conn->getDTMF().empty()) ) ) {
		conn->start_file_reception(file);
		CallModule::mainLoop();
		conn->stop_file_reception();
		// truncate the silence away if it's more than one second
		if (silence_timeout>8000 && silence_count > silence_timeout) {
			struct stat filestat;
			if (stat(file.c_str(),&filestat)==-1)
				throw CapiExternalError("can't stat output file","AudioReceive::mainLoop");
			if (truncate(file.c_str(),filestat.st_size-silence_timeout+8000)==-1) 
				throw CapiExternalError("can't truncate output file","AudioReceive::mainLoop");
		}
	}
	end_time=getTime();
}

void
AudioReceive::dataIn(unsigned char* data, unsigned length)
{
	if (silence_timeout) {
		unsigned int sum=0;
		for (int i=0;i<length;i++)
			sum+=(cswap[data[i]]^0x55) & 0x7f;  // swap bits, inversion of odd bits, stripping of sign
		if (sum < conf_silence_limit*length) {
			conn->debugMessage("silence",3);
			silence_count+=length;
			if (silence_count > silence_timeout)
				finish=true;
		} else
			silence_count=0;
	}
}

long
AudioReceive::duration()
{
	return end_time-start_time;
}

/*  History

Old Log (for new changes see ChangeLog):

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.18  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.17  2003/01/16 13:02:30  ghillie
- truncate silence away if silence_timeout enabled
- improve duration() to only include real duration of recording

Revision 1.16  2003/01/04 16:08:48  ghillie
- use new debugMessage method of Connection

Revision 1.15  2002/12/16 15:06:14  ghillie
use right debug stream now

Revision 1.14  2002/12/04 11:38:50  ghillie
- added time measurement: save time in start_time at the begin of mainLoop() and return difference to getTime() in duration()

Revision 1.13  2002/12/02 12:32:21  ghillie
renamed Connection::SPEECH to Connection::VOICE

Revision 1.12  2002/11/29 10:26:10  ghillie
- updated comments, use doxygen format now
- removed transmissionComplete() method as this makes no sense in receiving!

Revision 1.11  2002/11/25 11:54:21  ghillie
- tests for speech mode before receiving now
- small performance improvement (use string::empty() instead of comparison to "")

Revision 1.10  2002/11/22 15:16:20  ghillie
added support for finishing when DTMF is received

Revision 1.9  2002/11/21 15:32:40  ghillie
- moved code from constructor/destructor to overwritten mainLoop() method

Revision 1.8  2002/11/21 11:37:09  ghillie
make sure that we don't use Connection object after call was finished

Revision 1.7  2002/11/19 15:57:19  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.6  2002/11/14 17:05:19  ghillie
major structural changes - much is easier, nicer and better prepared for the future now:
- added DisconnectLogical handler to CallInterface
- DTMF handling moved from CallControl to Connection
- new call module ConnectModule for establishing connection
- python script reduced from 2 functions to one (callWaiting, callConnected
  merged to callIncoming)
- call modules implement the CallInterface now, not CallControl any more
  => this freed CallControl from nearly all communication stuff

Revision 1.5  2002/11/13 15:24:25  ghillie
finished silence detection code

Revision 1.4  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.3  2002/11/12 15:52:08  ghillie
added data in handler

Revision 1.2  2002/10/29 14:28:22  ghillie
added stop_file_* calls to make sure transmission is cancelled when it's time...

Revision 1.1  2002/10/25 13:29:39  ghillie
grouped files into subdirectories

Revision 1.2  2002/10/24 09:55:52  ghillie
many fixes. Works for one call now

Revision 1.1  2002/10/23 09:47:30  ghillie
added audioreceive module

*/
