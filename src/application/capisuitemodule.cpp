/*  @file capisuitemodule.cpp
    @brief Contains the Python module and integration routines

    @author Gernot Hillier <gernot@hillier.de>
    $Revision: 1.10 $
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
#include <string>
#include <unistd.h> // for sleep()
#include "../backend/connection.h"
#include "../modules/audiosend.h"
#include "../modules/audioreceive.h"
#include "../modules/faxreceive.h"
#include "../modules/faxsend.h"
#include "../modules/connectmodule.h"
#include "../modules/disconnectmodule.h"
#include "../modules/switch2faxG3.h"
#include "../modules/readDTMF.h"
#include "../modules/calloutgoing.h"
#include "capisuitemodule.h"   
#include "capisuite.h"

#define TEMPORARY_FAILURE 0x34A9    // see ETS 300 102-1, Table 4.13 (cause information element)

extern CapiSuite* capisuiteInstance;

static PyObject* CallGoneError=NULL;
static PyObject* BackendError=NULL;

/** @defgroup python C/Python wrapper functions
    @brief These functions define the python commands you can use in your scripts.

    All CapiSuite-commands available in Python will stay in a python
    module called capisuite. This module and all its functions are defined here.

    There's a general scheme for mapping the names of the C wrapper functions
    to python names:

    Python command "capisuite.command()" will be defined in the wrapper function
    "capisuite_command()".

    So you can use this document as reference to all available CapiSuite Python
    commands. For example, if you read the documentation for the
    capisuite_audio_send function here, you can use it as capisuite.audio_send in
    your Python scripts.

*/

void
capisuitemodule_destruct_connection(void* ptr)
{
	Connection *conn=(static_cast<Connection*>(ptr));
	conn->debugMessage("Python: deleting connection object",2);
	if (conn->getState()!=Connection::DOWN) {
		try {
			conn->errorMessage("Warning: Connection still established in capisuitemodule_desctruct_conn(). Disconnecting.");
			DisconnectModule active(conn,TEMPORARY_FAILURE,true);
			active.mainLoop();
		}
		catch (CapiError e) {
			conn->errorMessage("ERROR: disconnection also failed. Too bad...");
		}
	}
	delete conn;
}

/** @brief Private converter function to extract the contained Connection* from a PyCObject

    This function is defined for the use in PyArg_ParseTuple() calls.

    @param conn_ref - PyCObject pointer
    @param conn address of the Connection pointer where the result will be stored
    @return 1=successful, 0=error
*/
bool
convertConnRef(PyObject *conn_ref, Connection** conn)
{
	if (!PyCObject_Check(conn_ref)) {
		PyErr_SetString(PyExc_TypeError,"Invalid call reference given.");
		return 0;
	}

	if (! ( *conn=static_cast<Connection*>(PyCObject_AsVoidPtr(conn_ref)) ) ) {
		PyErr_SetString(PyExc_TypeError,"Call reference is NULL. This is not allowed.");
		return 0;
	}
	return 1;
}

/** @brief Private converter function to extract the contained Capi* from a PyCObject

    This function is defined for the use in PyArg_ParseTuple() calls.

    @param capi_ref - PyCObject pointer
    @param capi address of the Capi pointer where the result will be stored
    @return 1=successful, 0=error
*/
bool
convertCapiRef(PyObject *capi_ref, Capi** capi)
{
	if (!PyCObject_Check(capi_ref)) {
		PyErr_SetString(PyExc_TypeError,"Invalid Capi reference given.");
		return 0;
	}

	if (! ( *capi=static_cast<Capi*>(PyCObject_AsVoidPtr(capi_ref)) ) ) {
		PyErr_SetString(PyExc_TypeError,"Capi reference is NULL. This is not allowed.");
		return 0;
	}
	return 1;
}
                          
/** @brief Write an informational message to the CapiSuite log.
    @ingroup python

    This function writes a message to the CapiSuite log. It's helpful if you want to write
    messages in the debug log in your scripts. 
    
    The message can be either logged with the general CapiSuite prefix if they are of global
    nature or with the Connection prefix if they're associated with a special connection.

    @param args Contains the python parameters. These are:
    	- <b>message (string)</b> the log message
    	- <b>level (integer)</b> parameter for log_level
	- <b>call (optional)</b> call reference - if given, the message is logged with Connection prefix
    @return None
*/
static PyObject*
capisuite_log(PyObject*, PyObject *args)
{

	Connection* conn=NULL;
	char *message;
	int level;

	if (!PyArg_ParseTuple(args,"si|O&:log",&message,&level,convertConnRef,&conn))
		return NULL;

	if (conn)
		conn->debugMessage(message,level);
	else if (capisuiteInstance)
        	capisuiteInstance->logMessage(message,level);

	Py_XINCREF(Py_None);
	return (Py_None);
}

/** @brief Write an error message to the CapiSuite error log.
    @ingroup python

    This function writes a message to the CapiSuite error log. It
    should be used to output error messages so they appear in the
    normal error log.

    @param args Contains the python parameter:
    	- <b>message (string)</b> the log message
    @return None
*/
static PyObject*
capisuite_error(PyObject*, PyObject *args)
{

	Connection* conn=NULL;
	char *message;

	if (!PyArg_ParseTuple(args,"s|O&:error",&message,convertConnRef,&conn))
		return NULL;

	if (conn)
		conn->errorMessage(message);
	else if (capisuiteInstance)
		capisuiteInstance->errorMessage(message);
		
	Py_XINCREF(Py_None);
	return (Py_None);
}

/** @brief Send an audio file in a speech mode connection.
    @ingroup python

    This function sends an audio file. The audio file must be in bit-inversed A-Law format. It can be created for example
    with sox using the suffix ".la". It supports abortion if DTMF signal is received.

    If DTMF abort is enabled, the command will also abort immediately if DTMF was received before it is called. That allows
    you to abort subsequent audio receive and send commands with one DTMF signal w/o needing to check for received DTMF
    after each command.

    The connction must be in audio mode (use capisuite_connect_voice()), otherwise an exception will be caused.

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    	- <b>filename (string)</b> file to send
    	- <b>exit_DTMF (integer, optional)</b> if set to 1, sending is aborted when a DTMF signal is received (0=off, default)
    @return int containing duration of send in seconds
*/
static PyObject*
capisuite_audio_send(PyObject*, PyObject *args)
{
	Connection* conn;
	char *filename;
	PyThreadState *_save;
	int exit_DTMF=0;
	long duration=0;

	if (!PyArg_ParseTuple(args,"O&s|i:audio_send",convertConnRef,&conn,&filename,&exit_DTMF))
		return NULL;

	try {
		Py_UNBLOCK_THREADS
		AudioSend active(conn,filename,exit_DTMF);
		active.mainLoop();
		duration=active.duration();
		Py_BLOCK_THREADS
	}
	catch (CapiWrongState e) {
		Py_BLOCK_THREADS
		PyErr_SetString(CallGoneError,"Call was finished from partner.");
		return NULL;
	}
	catch (CapiMsgError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}
	catch (CapiExternalError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}
	catch (CapiError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}

	PyObject *r=PyInt_FromLong(duration);
	return (r);
}

/** @brief Receive an audio file in a speech mode connection.
    @ingroup python

    This functions receives an audio file. It can recognize silence in the signal and timeout after
    a given period of silence, after a general timeout or after the reception of a DTMF signal.

    If the recording was finished because of silence_timeout, the silence will be truncated away.

    If DTMF abort is enabled, the command will also abort immediately if DTMF was received before it is called. That allows
    you to abort subsequent audio receive and send commands with one DTMF signal w/o needing to check for received DTMF
    after each command.

    The connction must be in audio mode (use capisuite_connect_voice()), otherwise an exception will be caused.

    The created file will be saved in bit-reversed A-Law format, 8 kHz mono. Use sox to convert it to a normal wav file.

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    	- <b>filename (string)</b> where to save received file
	- <b>timeout (integer)</b> receive length in seconds (-1 = infinite)
	- <b>silence_timeout (integer, optional)</b> abort after x seconds of silence (0=off, default)
	- <b>exit_DTMF (integer, optional)</b> if set to 1, sending is aborted when a DTMF signal is received (0=off, default)
    @return int containing duration of receive in seconds
*/
static PyObject*
capisuite_audio_receive(PyObject *, PyObject *args)
{
	Connection* conn;
	char *filename;
	int timeout, silence_timeout=0;
	PyThreadState *_save;
	int exit_DTMF=0;
	long duration=0;

	if (!PyArg_ParseTuple(args,"O&si|ii:audio_receive",convertConnRef,&conn,&filename, &timeout, &silence_timeout,&exit_DTMF))
		return NULL;

	try {
		Py_UNBLOCK_THREADS
		AudioReceive active(conn,filename,timeout,silence_timeout,exit_DTMF);
		active.mainLoop();
		duration=active.duration();
		Py_BLOCK_THREADS
	}
	catch (CapiWrongState e) {
		Py_BLOCK_THREADS
		PyErr_SetString(CallGoneError,"Call was finished from partner.");
		return NULL;
	}
	catch (CapiExternalError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}

	PyObject *r=PyInt_FromLong(duration);
	return (r);
}

/** @brief Receive a fax in a fax mode connection
    @ingroup python

    This command receives an analog fax (fax group 3). It starts the reception and waits for the end of the connection.
    So it should be the last command before capisuite_disconnect.

    The connction must be in fax mode (use capisuite_connect_faxG3 or capisuite_switch_to_faxG3), otherwise an exception will be caused.

    The created file will be saved in the Structured Fax File (SFF) format.

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    	- <b>filename (string)</b> where to save received fax
    @return None or a tuple (stationID,rate,hiRes,format,pages) containing the values:
    	- fax station ID from the calling party (String)
	- bit rate which was used for connecting (Integer)
	- high (1) or low (0) resolution (Integer)
	- transmit format: 0=SFF,black&white, 1=ColorJPEG (Integer)
	- number of received pages (Integer)
*/
static PyObject*
capisuite_fax_receive(PyObject *, PyObject *args)
{
	Connection *conn;
	char *filename;
	PyThreadState *_save;

	if (!PyArg_ParseTuple(args,"O&s:fax_receive",convertConnRef,&conn,&filename))
		return NULL;

	try {
		Py_UNBLOCK_THREADS
		FaxReceive active(conn,filename);
		active.mainLoop();
		Py_BLOCK_THREADS
	}
	catch (CapiWrongState e) {
		Py_BLOCK_THREADS
		PyErr_SetString(CallGoneError,"Call was finished from partner.");
		return NULL;
	}
	catch (CapiExternalError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}

	Connection::fax_info_t* fax_info = conn->getFaxInfo();
	if (fax_info) {
		PyObject *r=Py_BuildValue("siiii",fax_info->stationID.c_str(),fax_info->rate,fax_info->hiRes,fax_info->format,fax_info->pages);
		return (r);
	} else {
		Py_XINCREF(Py_None);
		return (Py_None);
	}
}

/** @brief Send a fax in a fax mode connection
    @ingroup python

    This command sends an analog fax (fax group 3). It starts the send and waits for the end of the connection.
    So it should be the last command before capisuite_disconnect.

    The connction must be in fax mode (use capisuite_call_faxG3 or capisuite_switch_to_faxG3), otherwise an exception will be caused.

    The created file will be saved in the Structured Fax File (SFF) format.

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    	- <b>filename (string)</b> file to send
    @return None or a tuple (stationID,rate,hiRes,format,pages) containing the values:
    	- fax station ID from the called party (String)
	- bit rate which was used for connecting (Integer)
	- high (1) or low (0) resolution (Integer)
	- transmit format: 0=SFF,black&white, 1=ColorJPEG (Integer)
	- number of sent pages (Integer)
    @return None
*/
static PyObject*
capisuite_fax_send(PyObject *, PyObject *args)
{
	Connection *conn;
	char *filename;
	PyThreadState *_save;

	if (!PyArg_ParseTuple(args,"O&s:fax_send",convertConnRef,&conn,&filename))
		return NULL;

	try {
		Py_UNBLOCK_THREADS
		FaxSend active(conn,filename);
		active.mainLoop();
		Py_BLOCK_THREADS
	}
	catch (CapiWrongState e) {
		Py_BLOCK_THREADS
		PyErr_SetString(CallGoneError,"Call was finished from partner.");
		return NULL;
	}
	catch (CapiMsgError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}
	catch (CapiExternalError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}
	catch (CapiError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}

	Connection::fax_info_t* fax_info = conn->getFaxInfo();
	if (fax_info) {
		PyObject *r=Py_BuildValue("siiii",fax_info->stationID.c_str(),fax_info->rate,fax_info->hiRes,fax_info->format,fax_info->pages);
		return (r);
	} else {
		Py_XINCREF(Py_None);
		return (Py_None);
	}
}

/** @brief Disconnect connection.
    @ingroup python

    This will cause an immediate disconnection. It should be always the last command in every flow of a script.
    It will return a tuple of two result values. The first is the disconnect cause of the physical connection,
    the second the disconnect cause of the logical connection. See CAPI spec for the logical causes and
    ETS 300 102-01 for the physical causes.

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    @return Tuple containing (ReasonPhysical,ReasonLogical)
*/
static PyObject*
capisuite_disconnect(PyObject *, PyObject *args)
{
	Connection *conn;
	PyThreadState *_save;

	if (!PyArg_ParseTuple(args,"O&:disconnect",convertConnRef,&conn))
		return NULL;

	try {
		Py_UNBLOCK_THREADS
		DisconnectModule active(conn);
		active.mainLoop();
		Py_BLOCK_THREADS
	}
	catch (CapiMsgError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}
	catch (CapiExternalError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}

	PyObject *r=Py_BuildValue("ii",conn->getCause(),conn->getCauseB3());
	return (r);
}

/** @brief Reject an incoming call.
    @ingroup python

    If you don't want to accept an incoming call for any reason (e.g. if it has a service or comes from a number
    you don't want to accept), use this command. There are several reasons you can give when rejecting a call.
    Some important ones are:
	- 1=ignore call
	- 2=normal call clearing
	- 3=user busy
	- 7=incompatible destination
	- 8=destination out of order
	- 0x34A9=temporary failure

    You can find many more reasons in the ETS 300 201-01 specification or on the web (search for ISDN cause)
    if you really need them.

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    	- <b>rejectCause (integer)</b> which cause to signal when rejecting call (see above)
    @return None
*/
static PyObject*
capisuite_reject(PyObject *, PyObject *args)
{
	Connection *conn;
	int rejectCause;
	PyThreadState *_save;

	if (!PyArg_ParseTuple(args,"O&i:reject",convertConnRef,&conn,&rejectCause) )
		return NULL;

	try {
		Py_UNBLOCK_THREADS
		DisconnectModule active(conn,rejectCause);
		active.mainLoop();
		Py_BLOCK_THREADS
	}
	catch (CapiMsgError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}
	catch (CapiExternalError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}

	Py_XINCREF(Py_None);
	return (Py_None);
}


/** @brief helper function for capisuite_connect_voice() and capisuite_connect_faxG3()

    @param delay delay in seconds before connection will be established
    @param service with which service we should connect
    @param faxStationID only used for fax connections
    @param faxHeadline only used for fax connections
    @return false if exception was rased -> calling function must return NULL then
*/
bool
capisuite_connect(Connection *conn, int delay, Connection::service_t service, string faxStationID, string faxHeadline)
{
	PyThreadState *_save;
	try {
		Py_UNBLOCK_THREADS
		if (delay) {
			conn->acceptWaiting(); // so that connection doesn't timeout
			sleep(delay);
		}
		ConnectModule active(conn,service, faxStationID, faxHeadline);
		active.mainLoop();
		Py_BLOCK_THREADS
	}
	catch (CapiMsgError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return false;
	}
	catch (CapiWrongState e) {
		Py_BLOCK_THREADS
		PyErr_SetString(CallGoneError,(e.message()).c_str());
		return false;
	}
	catch (CapiExternalError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return false;
	}
	return true;
}

/** @brief Accept an incoming call and connect with voice service.
    @ingroup python

    This will accept an incoming call and choose voice as service, so you can use the audio commands
    (like audio_receive and audio_send) with this connection. After this command has finished, the call
    is connected successfully.

    It's also possible to accept a call with some delay. This is for example useful for an answering
    machine if you want to have the chance to get a call with your phone before your computer answers it.

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
	- <b>delay (integer, optional)</b> delay in seconds _before_ connection will be established (default: 0=immediate connect)
    @return None
*/
static PyObject*
capisuite_connect_voice(PyObject *, PyObject *args)
{
	Connection *conn;
	int delay=0;

	if (!PyArg_ParseTuple(args,"O&|i:connect_voice",convertConnRef,&conn,&delay))
		return NULL;

	if (capisuite_connect(conn,delay,Connection::VOICE,"","")) {
		Py_XINCREF(Py_None);
		return (Py_None);
	} else
		return NULL;
}

/** @brief Accept an incoming call and connect with fax (analog, group 3) service.
    @ingroup python

    This will accept an incoming call and choose fax group 3 as service, so you can use the fax commands
    (like fax_receive) with this connection. After this command has finished, the call is connected successfully.

    It's also possible to accept a call with some delay. This is for example useful if you want to have the chance
    to get a call with your phone before your computer answers it.

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    	- <b>faxStationID (string)</b> the station ID to use
	- <b>faxHeadline (string)</b> the fax headline to use
	- <b>delay (integer, optional)</b> delay in seconds _before_ connection will be established (default: 0=immediate connect)
    @return If faxInfo is available, a tuple (stationID,rate,hiRes,format) otherwise None. Tuple contains:
    	- fax station ID from the calling party (String)
	- bit rate which was used for connecting
	- high (1) or low (0) resolution
	- transmit format: 0=SFF,black&white, 1=ColorJPEG
*/
static PyObject*
capisuite_connect_faxG3(PyObject *, PyObject *args)
{
	Connection *conn;
	int delay=0;
	char *faxStationID, *faxHeadline;

	if (!PyArg_ParseTuple(args,"O&ss|i:connect_faxG3",convertConnRef,&conn,&faxStationID,&faxHeadline,&delay))
		return NULL;

	if (capisuite_connect(conn,delay,Connection::FAXG3,faxStationID,faxHeadline)) {
		Connection::fax_info_t* fax_info = conn->getFaxInfo();
		if (fax_info) {
			PyObject *r=Py_BuildValue("siii",fax_info->stationID.c_str(),fax_info->rate,fax_info->hiRes,fax_info->format);
			return (r);
		} else {
			Py_XINCREF(Py_None);
			return (Py_None);
		}
	} else
		return NULL;
}

/** @brief helper function for capisuite_call_voice() and capisuite_call_faxG3()

    @param capi reference to object of Capi to use
    @param controller controller number to use
    @param call_from string containing the own number to use
    @param call_to string containing the number to call
    @param service service to call with as described in Connection::service_t
    @param timeout timeout to wait for connection establishment
    @param faxStationID fax station ID, only necessary when connecting in FAXG3 mode
    @param faxHeadline fax headline, only necessary when connecting in FAXG3 mode
    @param clir set to true to disable sending of own number
    @return tuple (call,result) - call=reference to the created call object / result(int)=result of the call establishment
*/
static PyObject*
capisuite_call(Capi *capi, unsigned short controller, string call_from, string call_to, Connection::service_t service, int timeout, string faxStationID, string faxHeadline, bool clir)
{
	PyThreadState *_save;
	Connection* conn=NULL;
	int result;
	try {
		Py_UNBLOCK_THREADS
		CallOutgoing active(capi,controller,call_from,call_to,service,timeout,faxStationID,faxHeadline,clir);
		active.mainLoop();
		conn=active.getConnection();
		result=active.getResult();
		Py_BLOCK_THREADS
	}
	catch (CapiMsgError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}
	catch (CapiExternalError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}

	PyObject *r=Py_BuildValue("Ni",PyCObject_FromVoidPtr(conn,capisuitemodule_destruct_connection),result);
	return (r);
}

/** @brief Initiate an outgoing call with service voice and wait for successful connection
    @ingroup python

    This will initiate an outgoing call and choose voice as service, so you can
    use the audio commands (like audio_receive and audio_send) with this 
    connection. After this command has finished, the call is connected 
    successfully or the given timeout has exceeded. The timeout is measured 
    beginning at the moment when the call is signalled (it's "ringing") to the
    called party. 
 
    A python tuple (call,result) is returned by this function. It contains:
	- <b>call</b> reference to the created call object - use this for subsequent calls like audio_send
	- <b>result (int)</b> result of the call establishment process.
		- 0 = connection established
		- 1 = connection timeout exceeded, no connection was established
		- 2 = connection wasn't successful and no reason for this failure is available
		- 0x3301-0x34FF: Error reported by CAPI. For a complete description see 
		  the annex of the user manual

    @param args Contains the python parameters. These are:
	- <b>capi</b> reference to object of Capi to use (given to the idle function as parameter)
    	- <b>controller (int)</b> ISDN controller ID to use (1=first controller)
    	- <b>call_from (string)</b>own number to use
    	- <b>call_to (string)</b>the number to call
    	- <b>timeout (int)</b>timeout to wait for connection establishment in seconds
    	- <b>clir (int, optional)</b>set to 1 to disable sending of own number (0=default)
    @return tuple (call,result) - see above.
*/
static PyObject*
capisuite_call_voice(PyObject *, PyObject *args)
{
	Capi *capi;
	int controller, timeout, clir=0;
	char *call_from,*call_to;

	if (!PyArg_ParseTuple(args,"O&issi|i:call_voice",convertCapiRef,&capi,&controller,&call_from,&call_to,&timeout,&clir))
		return NULL;

	return capisuite_call(capi,controller,call_from,call_to,Connection::VOICE,timeout,"","",clir);
}

/** @brief Initiate an outgoing call with service faxG3 and wait for successful connection
    @ingroup python

    This will initiate an outgoing call and choose fax group 3 as service, so 
    you can use the fax commands (like fax_send and fax_receive) with this 
    connection. After this command has finished, the call is connected
    successfully or the given timeout has exceeded. The timeout is measured
    beginning at the moment when the call is signalled (it's "ringing") to the
    called party.

    A python tuple (call,result) is returned by this function. It contains:
	- <b>call</b> reference to the created call object - use this for subsequent calls like audio_send
	- <b>result (int)</b> result of the call establishment process.
		- 0 = connection established
		- 1 = connection timeout exceeded, no connection was established
		- 2 = connection wasn't successful and no reason for this failure is available
		- 0x3301-0x34FF: Error reported by CAPI. For a complete 
		  description see the annex of the user manual

    @param args Contains the python parameters. These are:
	- <b>capi</b> reference to object of Capi to use (given to the idle function as parameter)
    	- <b>controller (int)</b> ISDN controller ID to use (1=first controller)
    	- <b>call_from (string)</b>own number to use
    	- <b>call_to (string)</b>the number to call
    	- <b>timeout (int)</b>timeout to wait for connection establishment in seconds
    	- <b>faxStationID (string)</b>fax station ID
    	- <b>faxHeadline (string)</b> fax headline to print on every page
    	- <b>clir (int, optional)</b>set to 1 to disable sending of own number (0=default)
    @return tuple (call,result) - see above.
*/
static PyObject*
capisuite_call_faxG3(PyObject *, PyObject *args)
{
	Capi *capi;
	int controller, timeout, clir=0;
	char *call_from,*call_to,*faxStationID,*faxHeadline;

	if (!PyArg_ParseTuple(args,"O&ississ|i:call_faxG3",convertCapiRef,&capi,&controller,&call_from,&call_to,&timeout,&faxStationID,&faxHeadline,&clir))
		return NULL;

	return capisuite_call(capi,controller,call_from,call_to,Connection::FAXG3,timeout,faxStationID,faxHeadline,clir);
}

/** @brief Switch a connection from voice mode to fax mode.
    @ingroup python

    This will switch from voice mode to fax group 3 after you have connected, so you can use the fax commands afterwards.

    <em><b>Attention:</b></em> This command isn't supported by every ISDN card / CAPI driver!

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    	- <b>faxStationID (string)</b> the station ID to use
	- <b>faxHeadline (string)</b> the fax headline to use
    @return If faxInfo is available, a tuple (stationID,rate,hiRes,format) otherwise None. Tuple contains:
    	- fax station ID from the calling party (String)
	- bit rate which was used for connecting
	- high (1) or low (0) resolution
	- transmit format: 0=SFF,black&white, 1=ColorJPEG
*/
static PyObject*
capisuite_switch_to_faxG3(PyObject *, PyObject *args)
{
	Connection *conn;
	char *faxStationID, *faxHeadline;
	PyThreadState *_save;

	if (!PyArg_ParseTuple(args,"O&ss:switch_to_faxG3",convertConnRef,&conn,&faxStationID,&faxHeadline))
		return NULL;

	try {
		Py_UNBLOCK_THREADS
		Switch2FaxG3 active(conn,faxStationID,faxHeadline);
		active.mainLoop();
		Py_BLOCK_THREADS
	}
	catch (CapiWrongState e) {
		Py_BLOCK_THREADS
		PyErr_SetString(CallGoneError,"Call was finished from partner.");
		return NULL;
	}
	catch (CapiExternalError e) {
		Py_BLOCK_THREADS
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}

	Connection::fax_info_t* fax_info = conn->getFaxInfo();
	if (fax_info) {
		PyObject *r=Py_BuildValue("siii",fax_info->stationID.c_str(),fax_info->rate,fax_info->hiRes,fax_info->format);
		return (r);
	} else {
		Py_XINCREF(Py_None);
		return (Py_None);
	}
}

/** @brief Enable recognition of DTMF tones.
    @ingroup python

    You have to enable the recognition of DTMF tones if you want to use them in your script.

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    @return None
*/
static PyObject*
capisuite_enable_DTMF(PyObject *, PyObject *args)
{
	Connection *conn;

	if (!PyArg_ParseTuple(args,"O&:enable_DTMF",convertConnRef,&conn) )
		return NULL;

	try {
		conn->enableDTMF();
	}
	catch  (CapiWrongState) { // issued when we have no connection
		PyErr_SetString(CallGoneError,"Call was finished from partner.");
		return NULL;
	}
	catch  (CapiMsgError e) {
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}

	Py_XINCREF(Py_None);
	return (Py_None);
}

/** @brief Disable recognition of DTMF tones.
    @ingroup python

    You can disable the recognition of DTMF tones again if you want to.

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    @return None
*/
static PyObject*
capisuite_disable_DTMF(PyObject *, PyObject *args)
{
	Connection *conn;

	if (!PyArg_ParseTuple(args,"O&:disable_DTMF",convertConnRef,&conn) )
		return NULL;

	try {
		conn->disableDTMF();
	}
	catch (CapiWrongState) { // issued when we have no connection
		PyErr_SetString(CallGoneError,"Call was finished from partner.");
		return NULL;
	}
	catch (CapiMsgError e) {
		PyErr_SetString(BackendError,(e.message()).c_str());
		return NULL;
	}

	Py_XINCREF(Py_None);
	return (Py_None);
}

/** @brief Read the received DTMF tones or wait for a certain amount of them.
    @ingroup python

    This function allows to just read in the DTMF tones which were already received. But it also supports to
    wait for a certain amount of DTMF tones if you want the user to always input some digits at a certain
    step in your script.

    You can specify how much DTMF tones you want in several ways - see the parameter description. To just
    see if something was entered before, use capisuite.read_DTMF(0). If you want to get at least 1 and mostly 4 digits
    and want to wait 5 seconds for additional digits, you'll use capisuite.read_DTMF(5,1,4).

    Valid DTMF characters are '0'...'9','A'...'D' and two special fax tones: 'X' (CNG), 'Y' (CED)

    @param args Contains the python parameters. These are:
    	- <b>call</b> Reference to the current call
    	- <b>timeout (integer)</b> timeout in seconds after which reading is terminated, only applied when min_digits are reached! (-1 = infinite)
	- <b>min_digits (integer, optional)</b> minimum number of digits which must be read in ANY case, i.e. timout doesn't count here (default: 0)
  	- <b>max_digits (integer, optional)</b> maximum number of digits to read (aborts immediately if this number is reached) (0=infinite, i.e. only timeout counts, default)
    @return python string containing the received DTMF characters
*/
static PyObject*
capisuite_read_DTMF(PyObject *, PyObject *args)
{
	Connection *conn;
	PyThreadState *_save;
	int timeout, min_digits=0, max_digits=0;

	if (!PyArg_ParseTuple(args,"O&i|ii:read_DTMF",convertConnRef,&conn, &timeout, &min_digits, &max_digits) )
		return NULL;

	string dtmf_received;
	try {
		Py_UNBLOCK_THREADS
		ReadDTMF active(conn,timeout,min_digits,max_digits);
		active.mainLoop();
		dtmf_received=conn->getDTMF();
		conn->clearDTMF();
		Py_BLOCK_THREADS
	}
	catch (CapiWrongState e) {
		Py_BLOCK_THREADS
		PyErr_SetString(CallGoneError,"Call was finished from partner.");
		return NULL;
	}

	PyObject* result=Py_BuildValue("s",dtmf_received.c_str());
	return (result);
}


/** PCallControlMethods - array of functions in module capisuite
*/
static PyMethodDef PCallControlMethods[] = {
        {"audio_receive", 	capisuite_audio_receive, 	METH_VARARGS, "Receive audio. For further details see capisuite module reference."},
        {"audio_send", 		capisuite_audio_send, 		METH_VARARGS, "Send audio. For further details see capisuite module reference."},
 	{"fax_receive",		capisuite_fax_receive, 		METH_VARARGS, "Receive fax. For further details see capisuite module reference."},
 	{"fax_send",		capisuite_fax_send, 		METH_VARARGS, "Send fax. For further details see capisuite module reference."},
	{"disconnect", 		capisuite_disconnect, 		METH_VARARGS, "Disconnect call. For further details see capisuite module reference."},
	{"connect_voice",	capisuite_connect_voice,	METH_VARARGS, "Connect pending call with Telephony services. Arguments: call, delay"},
	{"connect_faxG3",	capisuite_connect_faxG3,	METH_VARARGS, "Connect pending call with FaxG3 services. For further details see capisuite module reference."},
	{"call_voice",		capisuite_call_voice,		METH_VARARGS, "Initiate an outgoing call with service voice. For further details see capisuite module reference."},
	{"call_faxG3",		capisuite_call_faxG3,		METH_VARARGS, "Initiate an outgoing call with service FaxG3. For further details see capisuite module reference."},
	{"switch_to_faxG3",	capisuite_switch_to_faxG3,	METH_VARARGS, "Switch from telephony to FaxG3 services. For further details see capisuite module reference."},
	{"reject",		capisuite_reject, 		METH_VARARGS, "Reject waiting call. For further details see capisuite module reference."},
	{"enable_DTMF",		capisuite_enable_DTMF,		METH_VARARGS, "Enable DTMF recognition. For further details see capisuite module reference."},
	{"disable_DTMF",	capisuite_disable_DTMF,		METH_VARARGS, "Disable DTMF recognition. For further details see capisuite module reference."},
	{"read_DTMF",		capisuite_read_DTMF,		METH_VARARGS, "Read and clear received DTMF. For further details see capisuite module reference."},
	{"log",			capisuite_log,			METH_VARARGS, "Write log message. For further details see capisuite module reference."},
	{"error",		capisuite_error,		METH_VARARGS, "Write error message. For further details see capisuite module reference."},
        {NULL,NULL,0,NULL}
};

void
capisuitemodule_init () throw (ApplicationError)
{
	PyObject *mod,*d;
	try {
		if ( ! ( mod=Py_InitModule3("_capisuite", PCallControlMethods, "Python module for controlling CapiSuite") ) )  // m=borrowed ref
			throw ApplicationError("unable to init python module capisuite (InitModule failed)","capisuite_init()");

		if ( ! ( d=PyModule_GetDict(mod) ) )  // d=borrowed ref
			throw ApplicationError("unable to init python module capisuite (GetDict(mod) failed)","capisuite_init()");

		if (!CallGoneError)
			if (! (CallGoneError=PyErr_NewException("capisuite.CallGoneError", NULL, NULL) ) )
				throw ApplicationError("unable to init python module capisuite (NewException for CallGoneError failed)","capisuite_init()");
		if (!BackendError)
			if (! (BackendError=PyErr_NewException("capisuite.BackendError", NULL, NULL) ) )
				throw ApplicationError("unable to init python module capisuite (NewException for BackendError failed)","capisuite_init()");
		
		if (PyDict_SetItemString(d, "CallGoneError", CallGoneError)!=0)
			throw ApplicationError("unable to init python module capisuite (SetItemString for CallGoneError failed)","capisuite_init()");
		if (PyDict_SetItemString(d, "BackendError", BackendError)!=0)
			throw ApplicationError("unable to init python module capisuite (SetItemString for BackendError failed)","capisuite_init()");

		if (PyModule_AddIntConstant(mod, "SERVICE_VOICE", Connection::VOICE) != 0)
			throw ApplicationError("unable to init python module capisuite (adding constant SERVICE_VOICE failed)","capisuite_init()");
		if (PyModule_AddIntConstant(mod, "SERVICE_FAXG3", Connection::FAXG3) != 0)
			throw ApplicationError("unable to init python module capisuite (adding constant SERVICE_FAXG3 failed)","capisuite_init()");
		if (PyModule_AddIntConstant(mod, "SERVICE_OTHER", Connection::OTHER) != 0)
			throw ApplicationError("unable to init python module capisuite (adding constant SERVICE_OTHER failed)","capisuite_init()");
	}
	catch(ApplicationError) {
 		if (CallGoneError)
			Py_DECREF(CallGoneError);
		if (BackendError)
			Py_DECREF(BackendError);
		throw;
	}
}


/* History

Old Log (for new changes see ChangeLog):
Revision 1.7  2003/08/24 12:49:21  gernot
- switch_to_faxG3: return faxInfo structure instead of None

Revision 1.6  2003/08/10 12:00:57  gernot
- added better description for controller ID

Revision 1.5  2003/07/20 10:31:52  gernot
- return information about transfers in fax receive/send, audio receive

Revision 1.4  2003/05/25 13:38:30  gernot
- support reception of color fax documents

Revision 1.3  2003/04/17 10:53:54  gernot
- update documentation of capisuite_call_* to the new behaviour (timeout for
  outgoing calls starts when other party gets signalled), moved error code
  description to manual

Revision 1.2  2003/02/21 23:21:44  gernot
- follow some a little bit stricter rules of gcc-2.95.3

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.21  2003/01/31 11:26:46  ghillie
- add "extern capisuiteInstance"
- add two missing Py_BLOCK_THREADS in capisuite_reject()

Revision 1.20  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.19  2003/01/19 12:07:47  ghillie
- new functions capisuite_log and capisuite_error (resolves TODO)

Revision 1.18  2003/01/16 13:01:21  ghillie
- updated comment of audio_receive: truncates silence now

Revision 1.17  2003/01/04 15:54:58  ghillie
- use new *Message functions of Connection

Revision 1.16  2002/12/16 13:12:06  ghillie
- destruct_connection now uses correct debug stream
- changed disconnect() to return cause and causeB3

Revision 1.15  2002/12/13 11:44:34  ghillie
added support for fax send

Revision 1.14  2002/12/13 09:57:10  ghillie
- error message formatting done by exception classes now

Revision 1.13  2002/12/11 13:37:25  ghillie
- use quick_disconnect in destruct when connection is still established
  because this is the result of some error

Revision 1.12  2002/12/11 13:00:19  ghillie
- modified capisuitemodule_call_* and convertFlowControlRef (now named
  convertCapiRef) to use Capi ptr instead of FlowControl ptr

Revision 1.11  2002/12/10 15:03:53  ghillie
- capisuitemodule_destruct_connection does nice disconnection now

Revision 1.10  2002/12/07 22:35:46  ghillie
- capisuite_connect(): removed PyErr_Occured() check
- capisuitemodule_init: doesn't return __main__ namespace any more
- FIX in capisuitemodule_init: don't double create exceptions any more!
- capisuitemodule_init: use shorter PyModule_AddIntConstant() instead of
  manual creation and registration

Revision 1.9  2002/12/06 15:24:25  ghillie
- reject uses DisconnectModule, too, now and waits for disconnection

Revision 1.8  2002/12/06 12:53:08  ghillie
- added destruction function for Connection objects
- removed capisuitemodule_call_gone()
- changed capisuite_disconnect(): use DisconnectModule, wait for disconnect,
  return ISDN disconnect cause
- fixed some typos in PyArg_ParseTuple() calls
- changed capisuite_call* to return a tuple (call,result)

Revision 1.7  2002/12/05 15:54:22  ghillie
- removed checks for PyErr_Occured() as exceptions won't be thrown in from outside any more

Revision 1.6  2002/12/05 14:49:47  ghillie
- added convertFlowControlRef()
- new python functions: call_voice and call_faxG3 for initiating outgoing calls

Revision 1.5  2002/12/04 10:38:14  ghillie
- audio_send and audio_receive do return duration now

Revision 1.4  2002/12/02 16:53:23  ghillie
- some minor fixes in the docu strings
- typo (SERVICE_SPEECH->SERVICE_VOICE)

Revision 1.3  2002/12/02 12:26:31  ghillie
- renamed Connection::SPEECH to Connection::VOICE
- 3 new constants defined in init function: SERVICE_VOICE, SERVICE_FAXG3, SERVICE_OTHER
- moved DECREF's to exception handler where appropriate

Revision 1.2  2002/11/29 11:37:39  ghillie
- missed some changes from CapiCom to CapiSuite

Revision 1.1  2002/11/29 11:06:22  ghillie
renamed CapiCom to CapiSuite (name conflict with MS crypto API :-( )

Revision 1.5  2002/11/29 10:20:44  ghillie
- updated docs, use doxygen format now

Revision 1.4  2002/11/27 15:56:56  ghillie
renamed connect_telephony to connect_voice

Revision 1.3  2002/11/25 11:48:48  ghillie
- improved parameter descriptions for python functions
- made more python parameter optional
- removed CIPvalue from application layer, use service type instead
- renamed get_DTMF() to read_DTMF(), added additional parameters (timeout, min_/max_digits), uses ReadDTMF-module now

Revision 1.2  2002/11/23 15:55:51  ghillie
changed some misleading function names

Revision 1.1  2002/11/22 15:44:54  ghillie
renamed pcallcontrol.* to capicommodule.*

Revision 1.18  2002/11/22 15:07:09  ghillie
- new python function switch_to_faxG3
- new parameter exit_DTMF for audio_send() and audio_receive()
- new parameters faxStationID and faxHeadline for connect_faxG3
- get_DTMF calls conn->clearDTMF() now

Revision 1.17  2002/11/21 15:26:19  ghillie
- removed some unnecessary commands (we don't need result of mainLoop() any more, no need to use pointers for call modules any more)
- removed python command connect(call,CIP), added connect_telephony() and connect_faxG3()
- connect_telephony() and connect_faxG3 take delay in seconds before connect as parameter now
- unified python function names (enableDTMF -> enable_DTMF, disableDTMF -> disable_DTMF, getDTMF -> get_DTMF

Revision 1.16  2002/11/20 17:23:04  ghillie
- fixed locking in the case of an exception. Python locks weren't set-up again when an exception occurred
- removed unnecessary conn->isUp() calls (handled via exceptions now)
- CapiWrongState exception causes CallGoneError to be issued in Python now
- removed unnecessary PyErr_Occured() calls at the end of each function
- added missing Py_XDECREF in _init()

Revision 1.15  2002/11/19 15:57:18  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.14  2002/11/18 14:21:07  ghillie
- moved global severity_t to ApplicationError::severity_t
- added throw() declarations to header files

Revision 1.13  2002/11/18 12:08:46  ghillie
return reference to GetDict(__main__) instead of GetDict(pcallcontrol), so that callIncoming can live in __main__

Revision 1.12  2002/11/17 14:36:32  ghillie
improved error handling: script will now terminate if B3 connection is lost
(each function checks for PyErr_Occurred() and conn->isUp() now)

Revision 1.11  2002/11/14 17:04:05  ghillie
* major structural changes - much is easier, nicer and better prepared for the future now:
  - added DisconnectLogical handler to CallInterface
  - DTMF handling moved from CallControl to Connection
  - new call module ConnectModule for establishing connection
  - python script reduced from 2 functions to one (callWaiting, callConnected
    merged to callIncoming)
  - call modules implement the CallInterface now, not CallControl any more
    => this freed CallControl from nearly all communication stuff

* converter function for PyCObject -> Connection pointer introduced
  -> this saves us many identical code lines in all Python/C integration
     functions

Revision 1.10  2002/11/13 15:23:21  ghillie
added new parameter to audio_receive (silence_timeout)
fixed deadlock: in deletion of call modules must happen w/o holding python global lock

Revision 1.9  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.8  2002/11/10 17:03:45  ghillie
now CallControl reference is passed directly to the called Pyhton functions

Revision 1.7  2002/11/06 16:16:07  ghillie
added code to raise CallGoneError in any case so the script is cancelled when the call is gone surely

Revision 1.6  2002/10/31 12:35:58  ghillie
added DTMF support

Revision 1.5  2002/10/30 16:05:20  ghillie
cosmetic fixes...

Revision 1.4  2002/10/30 14:25:54  ghillie
added connect,disconnect,reject functions, changed init function to return the module dictionary

Revision 1.3  2002/10/29 14:07:38  ghillie
changed to implecete pass call parameter so the user doesn't have to type it in the script...

Revision 1.2  2002/10/27 12:47:20  ghillie
- added multithread support for python
- changed callcontrol reference to stay in the python namespace
- changed ApplicationError to support differen severity

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.3  2002/10/24 09:55:52  ghillie
many fixes. Works for one call now

Revision 1.2  2002/10/23 15:42:11  ghillie
- added standard headers
- changed initialization code (object references now set in extra function)
- added some missing Py_None

*/
