/*  @file capisuite.cpp
    @brief Contains CapiSuite - Main application class, implements ApplicationInterface

    @author Gernot Hillier <gernot@hillier.de>
    $Revision: 1.8 $
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <Python.h>
#include <cStringIO.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include <sstream>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include "../backend/capi.h"
#include "../backend/connection.h"
#include "incomingscript.h"
#include "idlescript.h"
#include "capisuite.h"

/** @brief Global Pointer to current CapiSuite instance
*/
CapiSuite* capisuiteInstance=NULL;

void exit_handler(int)
{
	if (capisuiteInstance)
		capisuiteInstance->finish();
}

void hup_handler(int)
{
	if (capisuiteInstance)
		capisuiteInstance->reload();
	signal(SIGHUP,hup_handler);
}
 
CapiSuite::CapiSuite(int argc,char **argv)
:capi(NULL),waiting(),config(),idle(NULL),py_state(NULL),debug(NULL),error(NULL),finish_flag(false),custom_configfile(),daemonmode(false)
{
	if (capisuiteInstance!=NULL) {
		cerr << "FATAL error: More than one instances of CapiSuite created" << endl;
		exit(1);
	}
	capisuiteInstance=this;

	readCommandline(argc,argv);
	readConfiguration();

	try {
		if (daemonmode)
                        // set daemon mode
			if (daemon(0,0)!=0) {
				(*error) << prefix() <<  "FATAL error: Can't fork off daemon" << endl;
				exit(1);
			}

		debug_level=atoi(config["log_level"].c_str());

		(*debug) << prefix() << "CapiSuite " << VERSION << " started." << endl;
		(*error) << prefix() << "CapiSuite " << VERSION << " started." << endl;

		string DDIStopListS=config["DDI_stop_numbers"];
		vector<string> DDIStopList;
		DDIStopList.push_back("");

		int j=0;		
		for (int i=0;i<DDIStopListS.length();i++) {
		    if (DDIStopListS[i]==',') {
			DDIStopList.push_back("");
			j++;
		    } else {
			DDIStopList[j]+=DDIStopListS[i];
		    }
		}

		// backend init
		capi=new Capi(*debug,debug_level,*error,atoi(config["DDI_length"].c_str()),atoi(config["DDI_base_length"].c_str()),DDIStopList);
		capi->registerApplicationInterface(this);

                string info;
		if (debug_level>=2)
			info=capi->getInfo(true);
		else
			info=capi->getInfo(false);
		(*debug) << prefix();
		for (int i=0;i<info.size();i++) {
			(*debug) << info[i];
			if (i!=info.size()-1 && info[i]=='\n')
				(*debug) << prefix();
		}

		capi->setListenTelephony(0); // TODO: 0 = all, evtl. einstellbar?
		capi->setListenFaxG3(0); // TODO: 0 = all, evtl. einstellbar?

		// initialization of the Python interpreter to be thread safe (taken out of PyApache 4.26)
		Py_Initialize();
		PycString_IMPORT; // initialize cStringIO module
		if (!(save_cStringIO=PycStringIO)) { // save it as it will be overwritten by each #include<cStringIO.h> with NULL (sic)
			(*error) << prefix() << "FATAL error: error during Python interpreter initialization (cString)" << endl;
			exit(1);
		}
		PyEval_InitThreads();  // init and acquire lock
		py_state=PyEval_SaveThread();  // release lock, save thread context
		if (!py_state) {
			(*error) << prefix() << "FATAL error: can't release python lock" << endl;
			exit(1);
		}

		// idle script object
		int interval=atoi(config["idle_script_interval"].c_str());
		if (interval && config["idle_script"]!="")
			idle=new IdleScript(*debug,debug_level,*error,capi,config["idle_script"],interval,py_state,save_cStringIO);

		// signal handling
		signal(SIGTERM,exit_handler);
		signal(SIGINT,exit_handler);  // this must be located after pyhton initialization
		signal(SIGHUP,hup_handler);
	}
        catch (CapiError e) {
		capisuiteInstance=NULL;
		if (idle) {
			idle->requestTerminate();
		}
		if (py_state) {
			PyEval_RestoreThread(py_state); // switch to right thread context, acquire lock
			py_state=NULL;
			Py_Finalize();
		}
                if (capi)
			delete capi;
                (*error) << prefix() << "Can't start Capi abstraction. The given error message was: " << e << endl << endl;
                exit(1);
        }
        catch (ApplicationError e) {
		capisuiteInstance=NULL;
		if (idle) {
			idle->requestTerminate();
		}
		if (py_state) {
			PyEval_RestoreThread(py_state); // switch to right thread context, acquire lock
			py_state=NULL;
			Py_Finalize();
		}
                if (capi)
			delete capi;
                (*error) << prefix() << "Can't start application. The given error message was: " << e << endl;
                exit(1);
        }
}

CapiSuite::~CapiSuite()
{
	if (idle)
		idle->requestTerminate(); // will self-delete!

	// thread-safe shutdown of the Python interpreter (taken out of PyApache 4.26)
	if (py_state) {
		PyEval_RestoreThread(py_state); // switch to right thread context, acquire lock
		py_state=NULL;
		Py_Finalize();
	}

	delete capi;

	(*debug) << prefix() << "CapiSuite finished." << endl;
	(*error) << prefix() << "CapiSuite finished." << endl;

	capisuiteInstance=NULL;
}


void
CapiSuite::finish()
{
	if (debug_level >= 2)
		(*debug) << prefix() << "requested finish" << endl;
	finish_flag=true;
}

void CapiSuite::reload()
{
	if (debug_level >= 2)
		(*debug) << prefix() << "requested reload" << endl;
	if (idle)
		idle->activate();
}

void
CapiSuite::callWaiting (Connection *conn)
{
	waiting.push(conn);
}

void
CapiSuite::mainLoop()
{
	timespec delay_time;
	delay_time.tv_sec=0; delay_time.tv_nsec=100000000;  // 100 msec
	int count,errorcount=0;
	while (!finish_flag) {
		nanosleep(&delay_time,NULL);
		count++;
		while (waiting.size()) {
			Connection* conn=waiting.front();
			waiting.pop();

			IncomingScript *instance;
			try {
				instance=new IncomingScript(*debug,debug_level,*error,conn,config["incoming_script"],save_cStringIO);
			}
			catch (ApplicationError e)
			{
				(*error) << prefix() << "ERROR: can't start IncomingScript thread, message was: " << e << endl;
				delete instance;
			}
			// otherwise it will self-delete!
		}
	}
}

string
CapiSuite::prefix()
{
	time_t t=time(NULL);
	char* ct=ctime(&t);
	ct[24]='\0';
	stringstream s;
	s << ct << " CapiSuite " << hex << this << ": ";
	return (s.str());
}

void
CapiSuite::parseConfigFile(ifstream &configfile)
{
        while (!configfile.eof()) {
                string l;
                getline(configfile,l);
                if (l.size() && l[0]!='#') {
                        int pos=l.find("=");
                        if (pos>0) {
                                int key_f=l.find_first_not_of("\" \t"); // strip blanks and " chars...
                                int key_l=l.find_last_not_of("\" \t",pos-1);
                                int value_f=l.find_first_not_of("\" \t",pos+1);
                                int value_l=l.find_last_not_of("\" \t");
                                string key,value;
                                if (key_f>=0 && key_f<=key_l) { // if we have a valid key
                                        key=l.substr(key_f,key_l-key_f+1);
                                        if (value_f>0 && value_f<=value_l)
                                                value=l.substr(value_f,value_l-value_f+1);
                                        else
                                                value="";
                                        config[key]=value;
                                }
                        }
                }
        }
}

void
CapiSuite::checkOption(string key, string value)
{
	if (!config.count(key)) {
		cerr << "Warning: Can't find " << key << " variable. Using default (\"" << value << "\")." << endl;
		config[key]=value;
        }
}

void
CapiSuite::logMessage(string message, int level)
{
	if (debug_level >= level)
		(*debug) << prefix() << message << endl;
}

void
CapiSuite::errorMessage(string message)
{
	(*error) << prefix() << message << endl;
}

void
CapiSuite::readConfiguration()
{
	ifstream configf;

	if (custom_configfile.size()) {
		configf.open(custom_configfile.c_str());
		if (configf)
			parseConfigFile(configf);
		else
			cerr << "Warning: Can't open custom file " << custom_configfile << "."<< endl;
		configf.close();
	} else {
		configf.open((string(PKGSYSCONFDIR)+"/capisuite.conf").c_str());
		if (configf)
			parseConfigFile(configf);
		else
			cerr << "Warning: Can't open " << PKGSYSCONFDIR <<"/capisuite.conf." << endl;
		configf.close();
	}

	checkOption("incoming_script",string(PKGLIBDIR)+"/incoming.py");
	checkOption("idle_script",string(PKGLIBDIR)+"idle.py");
	checkOption("idle_script_interval","60");
	checkOption("log_file",string(LOCALSTATEDIR)+"/log/capisuite.log");
	checkOption("log_level","2");
	checkOption("log_error",string(LOCALSTATEDIR)+"/log/capisuite.error");
	checkOption("DDI_length","0");
	checkOption("DDI_base_length","0");
	checkOption("DDI_stop_numbers","");
	
	string t(config["idle_script_interval"]);
	for (int i=0;i<t.size();i++)
		if (t[i]<'0' || t[i]>'9')
			throw ApplicationError("Invalid idle_script_interval given.","readConfiguration()");

	if (config["log_file"]!="" && config["log_file"]!="-") {
		debug = new ofstream(config["log_file"].c_str(),ios::app);
		if (! (*debug)) {
			cerr << "Can't open log file. Writing to stdout." << endl;
			delete debug;
			debug = &cout;
		}
	} else
		debug=&cout;

	t=config["log_level"];
	if (t.size()!=1 && (t[0]<'0' || t[0]>'3'))
		throw ApplicationError("Invalid log_level given.","main()");

	if (config["log_error"]!="" && config["log_error"]!="-") {
		error = new ofstream(config["log_error"].c_str(),ios::app);
		if (! (*error)) {
			cerr << "Can't open error log file. Writing to stderr." << endl;
			delete error;
			error = &cerr;
		}
	} else
		error=&cerr;

	t=config["DDI_length"];
	for (int i=0;i<t.size();i++)
                if (t[i]<'0' || t[i]>'9')
                        throw ApplicationError("Invalid DDI_length given.","readConfiguration()");

        t=config["DDI_base_length"];
        for (int i=0;i<t.size();i++)
                if (t[i]<'0' || t[i]>'9')
                        throw ApplicationError("Invalid DDI_base_length given.","readConfiguration()");

        t=config["DDI_stop_numbers"];
	for (int i=0;i<t.size();i++)
                if ((t[i]<'0' || t[i]>'9') && t[i]!=',')
                        throw ApplicationError("Invalid DDI_stop_numbers given.","readConfiguration()");
			
	if (daemonmode) {
		if (debug==&cout) {
			cerr << "FATAL error: not allowed to write to stdout in daemon mode." << endl;
			exit(1);
		}
		if (error==&cerr) {
			cerr << "FATAL error: not allowed to write to stderr in daemon mode." << endl;
			exit(1);
		}
	}
}


void
CapiSuite::readCommandline(int argc, char** argv)
{
	struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"config", required_argument, NULL, 'c'},
		{"daemon", no_argument, NULL, 'd'},
		{0, 0, 0, 0}
	};

	int result=0;
	do {
		result=getopt_long(argc,argv,"hdc:",long_options,NULL);
		switch (result) {
			case -1: // end
			break;

			case 'c':
				custom_configfile=optarg;
			break;

			case 'd':
				daemonmode=true;
				break;
			case 'h':
			default:
				help();
				exit(1);
			break;
		}
	} while (result!=-1);
}

void
CapiSuite::help()
{
	cout << "CapiSuite " << VERSION << " (c) by Gernot Hillier <gernot@hillier.de>" << endl << endl;
	cout << "CapiSuite is an python-scriptable ISDN Telecommunication Suite providing some" << endl;
	cout << "ISDN services like fax, voice recording/playing, etc. For further documentation" << endl;
	cout << "please have a look at the HTML documents provided with it." << endl << endl;
	cout << "syntax: capisuite [-h] [-c file]" << endl << endl;
	cout << "-h, --help			show this help" << endl;
	cout << "-c file, --config=file		use a custom configuration file" << endl;
	cout << "				(default: " << PKGSYSCONFDIR <<"/capisuite.conf)" << endl;
	cout << "-d, --daemon			run as daemon" << endl;
}


/* History

Old Log (for new changes see ChangeLog):

Revision 1.5.2.3  2003/11/06 18:32:15  gernot
- implemented DDIStopNumbers

Revision 1.5.2.2  2003/11/02 14:58:16  gernot
- use DDI_base_length instead of DDI_base
- added DDI_stop_numbers option
- use DDI_* options in the Connection class
- call the Python script if number is complete

Revision 1.5.2.1  2003/10/26 16:51:55  gernot
- begin implementation of DDI, get DDI Info Elements

Revision 1.5  2003/04/03 21:09:46  gernot
- Capi::getInfo isn't static any longer

Revision 1.4  2003/03/06 09:34:44  gernot
- added missing endl's in error messages

Revision 1.3  2003/02/25 13:22:42  gernot
- remove old, unused attribute
- correct some forgotten references to CallControl to IncomingScript

Revision 1.2  2003/02/21 23:21:44  gernot
- follow some a little bit stricter rules of gcc-2.95.3

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.14  2003/02/17 16:49:24  ghillie
- cosmetic...

Revision 1.13  2003/02/10 14:14:39  ghillie
merged from NATIVE_PTHREADS to HEAD

Revision 1.12.2.1  2003/02/09 14:58:13  ghillie
- rewritten parseConfigFile to use STL strings instead of CommonC++ strtokenizer
- no need to call detach on thread classes any more (because of their new
  usage of native pthreads)

Revision 1.12  2003/02/05 15:57:57  ghillie
- improved error handling, replaced some outputs to cerr with
  correct error stream

Revision 1.11  2003/01/31 11:25:53  ghillie
- moved capisuiteInstance from header to cpp (mustn't be defined in
  each file including capisuite.h, use extern there instead!)

Revision 1.10  2003/01/27 19:25:14  ghillie
- config moved to etc/capisuite/*

Revision 1.9  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.8  2003/01/19 12:06:05  ghillie
- support starting as daemon with "-d" (resolving TODO)
- nicer formatting for getInfo output at startup (resolves TODO)
- new methods logMessage() and errorMessage() for use in python scripts
  (resolves TODO)

Revision 1.7  2003/01/18 12:51:08  ghillie
- initialization of cStringIO needed for printing tracebacks to error log
  (solved one TODO item), pass on reference to *Script

Revision 1.6  2003/01/13 21:24:17  ghillie
- corrected typos
- reverted change to config options

Revision 1.5  2003/01/08 15:59:05  ghillie
- updated for new configuration variables for pathes, new method checkOption()

Revision 1.4  2003/01/07 14:51:19  ghillie
- added commandline parsing (new methods help(), readCommandLine():
  -h/--help and -c/--config (other configuration file)

Revision 1.3  2003/01/06 21:00:24  ghillie
- added SIGHUP support (new functions hup_handler, CapiSuite::reload)

Revision 1.2  2003/01/06 16:20:36  ghillie
- added error check for idle->detach()
- removed superfluous delete idle calls (SEGV)

Revision 1.1  2003/01/05 12:28:09  ghillie
- renamed FlowControl to CapiSuite
- the code from main() was moved to this class

Revision 1.23  2003/01/04 15:57:03  ghillie
- added exit handler for SIGINT and SIGTERM
- added static FlowControl pointer
- added finish method for exit handler
- added timestamp in log files
- log_level support

Revision 1.22  2002/12/14 14:01:06  ghillie
- just terminate() IdleScript instance, don't delete it (will delete self!)

Revision 1.21  2002/12/11 13:01:59  ghillie
- executeIdleScript() removed, its function is now done by IdleScript
  object (changes in constructor and mainLoop())
- removed getCapi()

Revision 1.20  2002/12/10 15:52:18  ghillie
- begin changes for moving functionality from executeIdleScript() to
  IdleScript class

Revision 1.19  2002/12/10 15:05:14  ghillie
- changed CallControl to IncomingScript
- added some missing =NULL statements in executeIdleScript()

Revision 1.18  2002/12/09 15:26:11  ghillie
- callWaiting does add Connection to queue only, real handling moved
  to mainLoop() as it may block in error handling
- error output/exception improvements (WARNING -> ERROR severity, ...)
- removed obsolete debug() method

Revision 1.17  2002/12/07 22:37:56  ghillie
- moved capisuitemodule_init() call from executeIdleScript() to constructor
- get __main__ as it's not returned by capisuitemodule_init() any more
- MEMORY FIX: added missing fclose call

Revision 1.16  2002/12/05 15:55:10  ghillie
- removed callCompleted() as now exception will be thrown in python script any more
- removed instances attribute

Revision 1.15  2002/12/05 14:52:31  ghillie
- in executeIdleScript(): removed ugly strncpy, used const_cast<char*>() instead to satisfy PyRun_SimpleFile()
- cleaned up python reference counting
- new method getCapi()

Revision 1.14  2002/12/02 12:28:09  ghillie
- FlowControl constructor now takes 3 additional parameters for the scripts to be used
- added support for regular execution of an idle script in mainLoop(), added executeIdleScript()

Revision 1.13  2002/11/29 10:20:44  ghillie
- updated docs, use doxygen format now

Revision 1.12  2002/11/27 15:57:25  ghillie
added missing throw() declaration

Revision 1.11  2002/11/23 16:40:19  ghillie
removed unnecessary include

Revision 1.10  2002/11/21 11:35:54  ghillie
- in case of call Completion just call instance->callCompleted() instead of deleting it.
  This allows us to continue w/o waiting for the thread to finish.

Revision 1.9  2002/11/20 17:17:46  ghillie
- FIX: instances weren't erased when exception occurred -> this lead to double used PLCI
- in mainLoop(): changed sleep to nanosleep

Revision 1.8  2002/11/19 15:57:18  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.7  2002/11/18 14:21:07  ghillie
- moved global severity_t to ApplicationError::severity_t
- added throw() declarations to header files

Revision 1.6  2002/11/14 17:05:19  ghillie
major structural changes - much is easier, nicer and better prepared for the future now:
- added DisconnectLogical handler to CallInterface
- DTMF handling moved from CallControl to Connection
- new call module ConnectModule for establishing connection
- python script reduced from 2 functions to one (callWaiting, callConnected
  merged to callIncoming)
- call modules implement the CallInterface now, not CallControl any more
  => this freed CallControl from nearly all communication stuff

Revision 1.5  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.4  2002/11/10 17:02:54  ghillie
mainLoop prepared for Python execution

Revision 1.3  2002/11/07 08:19:04  ghillie
some improvements and fixes in Python global lock and thread state handling

Revision 1.2  2002/10/27 12:47:20  ghillie
- added multithread support for python
- changed callcontrol reference to stay in the python namespace
- changed ApplicationError to support differen severity

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.8  2002/10/24 09:55:52  ghillie
many fixes. Works for one call now

Revision 1.7  2002/10/23 15:40:15  ghillie
added python integration...

Revision 1.6  2002/10/09 14:36:22  gernot
added CallModule base class for all call handling modules

Revision 1.5  2002/10/05 20:43:32  gernot
quick'n'dirty, but WORKS

Revision 1.4  2002/10/05 13:53:00  gernot
changed to use thread class of CommonC++ instead of the threads-package
some cosmetic improvements (indentation...)

Revision 1.3  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.2  2002/10/04 13:27:15  gernot
some restructuring to get it to a working state ;-)

does not do anything useful yet nor does it even compile...

Revision 1.1  2002/10/02 14:10:07  gernot
first version

*/
