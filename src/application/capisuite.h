/** @file capisuite.h
    @brief Contains CapiSuite - Main application class, implements ApplicationInterface

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

#ifndef CAPISUITE_H
#define CAPISUITE_H

#include <Python.h>
#include <map>
#include <queue>
#include <fstream>
#include "../backend/applicationinterface.h"
#include "applicationexception.h"
#include "capisuitemodule.h"
class Capi;
class IdleScript;
class PycStringIO_CAPI;

/** @brief Main application class, implements ApplicationInterface

    This class realizes the main application and thus implements the
    ApplicationInterface. It firstly generates the necessary objects from the CAPI
    abstraction layer (one object of class Capi) and enables call listening.

    It contains the mainLoop() and creates handling objects of IncomingScript for 
    all incoming calls and one object of IdleScript which will start the idle script
    at regular intervals. The scripts are informed about disconnection / program termination
    but will delete themselves.

    main() should create one CapiSuite object and then call mainLoop().

    @author Gernot Hillier
*/
class CapiSuite: public ApplicationInterface
{
	public:
		/** @brief Constructor. General initializations.

		    Creates a Capi object, enables listening, calls readConfiguration,
		    and initializes the Python interpreter in multithreading mode.
		    It immediately releases the global Python lock after doing initialization.

		    Also an IdleScript object is created and will regularly call the given script.

		    @param argc commandline argument count as given to main()
		    @param argv commandline arguments as given to main()
		*/
		CapiSuite(int argc, char** argv);

		/** @brief Destructor. Kill Python interpreter.

		    Request finishing of IdleThread and Python interpreter.
		*/
		~CapiSuite();

		/** @brief Callback: enqueue Connection in waiting
	   	*/
  		virtual void callWaiting (Connection *conn);

		/** @brief Main Loop. Event Loop (handling incoming connections)

		    For each incoming connection, an object of IncomingScript is created
		    which handles this call in an own thread.

		    This loop will run until the program is finished.
		*/
		void mainLoop();

		/** @brief Request finish of mainLoop
		*/
		void finish();

		/** @brief Parse a given configuration file

		This function reads the given configuration file. It must consist of key=value pairs
		separated on different lines.

		Lines beginning with "#" are treated as comments. Leading and trailing whitespaces
		and quotation marks (") surrounding the values will be ignored.
		*/
		void parseConfigFile(ifstream &configfile);

		/** @brief Read configuration and set default values for options not found

		The configuration is read from PREFIX/etc/capisuite.conf (should exist), ~/.capisuite.conf (optional) and perhaps
		a given custom config file (optional), while the latter has higher priority. After that all configuration options
		are checked and set to default values if not found.
		*/
		void readConfiguration();

		/** @brief Read commandline options
		*/
		void readCommandline(int argc, char**argv);

		/** @brief Print help message
		*/
		void help();

		/** @brief restart some aspects if the process gets a SIGHUP

		    Currently, this only reactivates the idle script if it was deactivated by too much errors in a row.
		*/
		void reload();

		/** @brief print a message to the log

		    Prints message to the log if it's level is high enough.

		    @param message the message
		    @param level level of the message
		*/
		void logMessage(string message, int level);

		/** @brief print a message to the error log

		    Prints message to the error log

		    @param message the message
		*/
		void errorMessage(string message);

	private:
  		/** @brief return a prefix containing this pointer and date for log messages

      		    @return constructed prefix as stringstream
  		*/
		string prefix();

  		/** @brief Test a configuration variable and set default if undefined

    		    @param key name of the config variable
		    @param value default value to set if key is not defined in config map
  		*/
		void checkOption(string key, string value);

		queue <Connection*> waiting; ///< queue for waiting connection instances
		IdleScript *idle; ///< reference to the IdleScript object created

		PyThreadState *py_state; ///< saves the created thread state of the main python interpreter
		PycStringIO_CAPI* save_cStringIO; ///< holds a pointer to the Python cStringIO C API
		Capi* capi; ///< reference to Capi object to use, set in constructor
		ostream  *debug, ///< debug stream
			 *error; ///< stream for error messages

		unsigned short debug_level; ///< verbosity level for debug stream

		bool finish_flag; ///< flag to finish mainLoop()

		bool daemonmode; ///< flag set when we're running as daemon

		map<string,string> config; ///< holds the configuration read from the configfile
		string custom_configfile; ///< holds the name of the custom config file if given

};

#endif

/* History

Old Log (for new changes see ChangeLog):
Revision 1.3  2003/02/25 13:23:19  gernot
- comment fix
- remove old, unused attribute

Revision 1.2  2003/02/21 23:21:44  gernot
- follow some a little bit stricter rules of gcc-2.95.3

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.8  2003/01/31 11:25:53  ghillie
- moved capisuiteInstance from header to cpp (mustn't be defined in
  each file including capisuite.h, use extern there instead!)

Revision 1.7  2003/01/19 12:06:25  ghillie
- new methods logMessage() and errorMessage()

Revision 1.6  2003/01/18 12:51:48  ghillie
- added save_cStringIO attribute for Python cStringIO C API

Revision 1.5  2003/01/13 21:24:47  ghillie
- added new method checkOption

Revision 1.4  2003/01/07 14:52:36  ghillie
- added support for custom config files
- added support for parsing commandline options

Revision 1.3  2003/01/06 21:00:48  ghillie
- added SIGHUP support (new method reload)

Revision 1.2  2003/01/06 16:20:51  ghillie
- updated comment

Revision 1.1  2003/01/05 12:28:09  ghillie
- renamed FlowControl to CapiSuite
- the code from main() was moved to this class

Revision 1.13  2003/01/04 15:58:38  ghillie
- log improvements: log_level, timestamp
- added finish() method
- added static FlowControl pointer

Revision 1.12  2002/12/11 13:02:56  ghillie
- executeIdleScript() removed, its function is now done by IdleScript
  object (changes in constructor and mainLoop())
- removed getCapi()
- minor docu bugs fixed

Revision 1.11  2002/12/09 15:29:13  ghillie
- debug stream given in constructor
- doc update for callWaiting() and mainLoop()
- obsolete debug() method removed

Revision 1.10  2002/12/06 12:54:30  ghillie
-removed callCompleted()

Revision 1.9  2002/12/05 14:54:15  ghillie
- constructor gets Capi* now
- new method getCapi()
- python idle script gets called with pointer to FlowControl now

Revision 1.8  2002/12/02 12:30:30  ghillie
- constructor now takes 3 additional arguments for the scripts to use
- added support for an idle script which is started in regular intervals

Revision 1.7  2002/11/29 10:20:44  ghillie
- updated docs, use doxygen format now

Revision 1.6  2002/11/27 15:58:13  ghillie
updated comments for doxygen

Revision 1.5  2002/11/19 15:57:18  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.4  2002/11/18 14:21:07  ghillie
- moved global severity_t to ApplicationError::severity_t
- added throw() declarations to header files

Revision 1.3  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.2  2002/10/27 12:47:20  ghillie
- added multithread support for python
- changed callcontrol reference to stay in the python namespace
- changed ApplicationError to support differen severity

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.6  2002/10/24 09:55:52  ghillie
many fixes. Works for one call now

Revision 1.5  2002/10/23 15:40:15  ghillie
added python integration...

Revision 1.4  2002/10/09 14:36:22  gernot
added CallModule base class for all call handling modules

Revision 1.3  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.2  2002/10/04 13:27:15  gernot
some restructuring to get it to a working state ;-)

does not do anything useful yet nor does it even compile...

Revision 1.1  2002/10/02 14:10:07  gernot
first version

*/
