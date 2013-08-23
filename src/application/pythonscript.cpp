/*  @file pythonscript.cpp
    @brief Contains PythonScript - Read a python script and call a function in own thread

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

#include "pythonscript.h"
#include <cStringIO.h>
#include <sstream> 

PythonScript::PythonScript(ostream &debug, unsigned short debug_level, ostream &error, string filename, string functionname, PycStringIO_CAPI* cStringIO)
:debug(debug),debug_level(debug_level),error(error),filename(filename),functionname(functionname),args(NULL), cStringIO(cStringIO)
{
	if (debug_level>=3)
		debug << prefix() << "PythonScript created." << endl;
}

PythonScript::~PythonScript()
{
	if (debug_level>=3)
		debug << prefix() << "PythonScript deleted." << endl;
}

string
PythonScript::prefix(bool verbose)
{
	stringstream s;
	time_t t=time(NULL);
	char* ct=ctime(&t);
	ct[24]='\0';
	if (verbose)
		s << ct << " Pythonscript " << filename << "," << functionname << "," << hex << this << ": ";
	else
		s << ct << " Pythonscript " << hex << this << ": ";
	return (s.str());
}

void 
PythonScript::run() throw (ApplicationError)
{
	PyObject *module=NULL, *module_dict=NULL, *function_ref=NULL, *result=NULL;

	FILE* scriptfile=NULL;
	try {
		if (!(scriptfile=fopen(filename.c_str(),"r") ) )
			throw ApplicationError("unable to open "+filename,"PythonScript::run()");

		// get __main__
		if ( ! ( module=PyImport_AddModule("__main__"))) // module = borrowed ref
			throw ApplicationError("unable to get __main__ namespace","PythonScript::run()");
		if ( ! ( module_dict=PyModule_GetDict(module) ) )  // module_dict = borrowed ref
			throw ApplicationError("unable to get __main__ dictionary","PythonScript::run()");

		// read control script. It must define a function callIncoming. For description see run()
		if (PyRun_SimpleFile(scriptfile,const_cast<char*>(filename.c_str()))==-1)
			throw ApplicationError("syntax error while executing python script","PythonScript::run()");

		fclose(scriptfile);
		scriptfile=NULL;

		// now let's get the user defined function
		PyObject* function_ref=PyDict_GetItemString(module_dict,const_cast<char*>(functionname.c_str())); // borrowed ref
		if (! function_ref || !PyCallable_Check(function_ref) )
			throw ApplicationError("control script does not define function "+functionname,"PythonScript::run()");

		if (!args)
			throw ApplicationError("no arguments given","PythonScript::run()");
		if (!PyTuple_Check(args))
			throw ApplicationError("args must be a tuple","PythonScript::run()");

		result=PyObject_CallObject(function_ref,args);
		if (!result) {
			PyObject *catch_stderr;
			// redirect sys.stderr and then print exception
			if ( ! ( module=PyImport_AddModule("sys"))) // module = borrowed ref
				throw ApplicationError("unable to get sys namespace","PythonScript::run()");
			if ( ! ( module_dict=PyModule_GetDict(module) ) )  // module_dict = borrowed ref
				throw ApplicationError("unable to get sys dictionary","PythonScript::run()");

			catch_stderr=cStringIO->NewOutput(128); // create StringIO object for collecting stderr messages
			if ( PyDict_SetItemString(module_dict,"stderr",catch_stderr)!=0 ) {
				Py_DECREF(catch_stderr);
				throw ApplicationError("unable to redirect sys.stderr","PythonScript::run()");
			}
			Py_DECREF(catch_stderr);

			PyErr_Print();
                        PyObject *py_traceback;
			if ( !(py_traceback=cStringIO->cgetvalue(catch_stderr)) )
				throw ApplicationError("unable to get traceback","PythonScript::run()");
			
			Py_ssize_t length;
			char *traceback;
			if (PyString_AsStringAndSize(py_traceback, &traceback, &length))
				throw ApplicationError("unable to convert traceback to char*","PythonScript::run()");

                        error << prefix() << "A python error occured. See traceback below." << endl;
			error << prefix(false) << "Python traceback: ";
			for (Py_ssize_t i=0;i<length-1;i++) {
				error << traceback[i];
				if (traceback[i]=='\n')
					error << prefix(false) << "Traceback: ";
			}
			error << endl;

			// undo redirection
			if ( PyDict_SetItemString(module_dict,"stderr",PyDict_GetItemString(module_dict,"__stderr__"))!=0 ) {
				throw ApplicationError("unable to reset sys.stderr","PythonScript::run()");
			}
		} else {
			if (result!=Py_None)
				error << prefix() << "WARNING: "+functionname+" shouldn't return anything";

			Py_DECREF(result);
			result=NULL;
		}
	}
	catch(ApplicationError e) {
		if (scriptfile)
			fclose(scriptfile);
		if (result)
			Py_DECREF(result);
		throw;
	}
}

void 
PythonScript::final()
{
	delete this;
}
