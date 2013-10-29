/*  @file connection.cpp
    @brief Contains Connection - Encapsulates a CAPI connection with all its states and methods.

    @author Gernot Hillier <gernot@hillier.de>
    $Revision: 1.18 $
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "../../config.h"
#include <fstream>
#include <stdexcept> // for out_of_range
#include <pthread.h>
#include <string.h>
#include <errno.h> // for errno
#include <iconv.h> // for iconv(), iconv_open(), iconv_close()
#include <cstring>
#include "capi.h"
#include "callinterface.h"
#include "connection.h"

#define conf_send_buffers 4

using namespace std;

Connection::Connection (_cmsg& message, Capi *capi, unsigned short DDILength, unsigned short DDIBaseLength, std::vector<std::string> DDIStopNumbers):
	call_if(NULL),capi(capi),plci_state(P2),ncci_state(N0), buffer_start(0), buffers_used(0),
	file_for_reception(NULL), file_to_send(NULL), received_dtmf(""), keepPhysicalConnection(false),
	disconnect_cause(0),debug(capi->debug), debug_level(capi->debug_level), error(capi->error),
	our_call(false), disconnect_cause_b3(0), fax_info(NULL), DDILength(DDILength), 
	DDIBaseLength(DDIBaseLength), DDIStopNumbers(DDIStopNumbers) 
{
	pthread_mutex_init(&send_mutex, NULL);
	pthread_mutex_init(&receive_mutex, NULL);

	plci=CONNECT_IND_PLCI(&message); // Physical Link Connection Identifier
	call_from = getNumber(CONNECT_IND_CALLINGPARTYNUMBER(&message),true);
	if (DDILength)
		call_to=""; // we enable the CalledParty InfoElement when using DDI and will get the number later again
	else
		call_to=getNumber(CONNECT_IND_CALLEDPARTYNUMBER(&message),false);

	if (debug_level >= 1) {
		debug << prefix() << "Connection object created for incoming call PLCI " << plci;
		debug << " from " << call_from << " to " << call_to << " CIP 0x" << hex << CONNECT_IND_CIPVALUE(&message) << endl;
	}
  	switch (CONNECT_IND_CIPVALUE(&message)) {
		case 1:
		case 4:
		case 16:
			service=VOICE;
		break;
		case 17:
			service=FAXG3;
		break;
		default:
			service=OTHER;
		break;
	}
	connect_ind_msg_nr=message.Messagenumber; // this is needed as connect_resp is given later
}

Connection::Connection (Capi* capi, _cdword controller, string call_from, bool clir, string call_to, service_t service, string faxStationID, string faxHeadline)  throw (CapiExternalError, CapiMsgError)
	:call_if(NULL),capi(capi),plci_state(P01),ncci_state(N0),plci(0),service(service),  
	buffer_start(0), buffers_used(0), file_for_reception(NULL), file_to_send(NULL), 
	call_from(call_from), call_to(call_to), connect_ind_msg_nr(0), disconnect_cause(0), 
	debug(capi->debug), debug_level(capi->debug_level), error(capi->error), keepPhysicalConnection(false),
	our_call(true), disconnect_cause_b3(0), fax_info(NULL), DDILength(0), DDIBaseLength(0) 
{
	pthread_mutex_init(&send_mutex, NULL);
	pthread_mutex_init(&receive_mutex, NULL);

	if (debug_level >= 1) {
		debug << prefix() << "Connection object created for outgoing call from " << call_from << " to " << call_to
		  << " service " << dec << service << endl;
	}
	if (debug_level >= 2) {
		debug << prefix() << "using faxStationID " << faxStationID << " faxHeadline " << faxHeadline << " CLIR " << clir << endl;
	}
	_cstruct B1config=NULL, B2config=NULL, B3config=NULL, calledPartyNumber=NULL, callingPartyNumber=NULL;
	_cword B1proto,B2proto,B3proto;

	try {
		_cword CIPvalue;
		switch (service) {
			case VOICE:
				CIPvalue=16;
			break;
			case FAXG3:
				CIPvalue=17;
			break;
			default:
				throw CapiExternalError("unsupported service given","Connection::Connection()");
			break;
		}

		buildBconfiguration(controller, service, faxStationID, faxHeadline, B1proto, B2proto, B3proto, B1config, B2config, B3config);

		if (!call_to.size())
			throw CapiExternalError("calledPartyNumber is required","Connection::Connection()");

		calledPartyNumber=new unsigned char [1+1+call_to.size()]; //struct length, number type/number plan, number
		calledPartyNumber[0]=1+call_to.size(); // length
		calledPartyNumber[1]=0x80;  // as suggested by CAPI spec (unknown number type, unknown number plan, see ETS 300 102-1)
		for (unsigned j=0;j<call_to.size();j++)
			calledPartyNumber[j+2]=call_to[j];

		callingPartyNumber=new unsigned char [1+2+call_from.size()];
		callingPartyNumber[0]=2+call_from.size(); // length
		callingPartyNumber[1]=0x00; // as suggested by CAPI spec (unknown number type, unknown number plan, see ETS 300 102-1)
		if (clir)
			callingPartyNumber[2]=0xA0; // suppress calling id presentation (CLIR)
		else
			callingPartyNumber[2]=0x80; // allow calling id presentation (CLIP)
		for (unsigned j=0;j<call_from.size();j++)  // TODO: does this really work when no number is given?!
			callingPartyNumber[j+3]=call_from[j];

		plci_state=P01;
		capi->connect_req(this,controller,CIPvalue, calledPartyNumber, callingPartyNumber,B1proto,B2proto,B3proto,B1config, B2config, B3config);
	} catch (...) {
		if (B1config)
			delete[] B1config;
		if (B2config)
			delete[] B2config;
		if (B3config)
			delete[] B3config;
		if (calledPartyNumber)
			delete[] calledPartyNumber;
		if (callingPartyNumber)
			delete[] callingPartyNumber;
		throw;
	}
	if (B1config)
		delete[] B1config;
	if (B2config)
		delete[] B2config;
	if (B3config)
		delete[] B3config;
	if (calledPartyNumber)
		delete[] calledPartyNumber;
	if (callingPartyNumber)
		delete[] callingPartyNumber;
}

Connection::~Connection()
{
	stop_file_transmission();
	stop_file_reception();

	if (getState()!=DOWN) {
		error << prefix() << "WARNING: please disconnect yourself before deleting connection object!!" << endl;
		disconnectCall(PHYSICAL_ONLY);
		while (getState()!=DOWN)
			;
	}
	plci_state=P0;

	pthread_mutex_lock(&send_mutex);  // assure the lock is free before destroying it
	pthread_mutex_unlock(&send_mutex);
	pthread_mutex_destroy(&send_mutex);

	pthread_mutex_lock(&receive_mutex); // assure the lock is free before destroying it
	pthread_mutex_unlock(&receive_mutex);
	pthread_mutex_destroy(&receive_mutex);

	if (fax_info)
		delete fax_info;

	if (debug_level >= 1) {
		debug << prefix() << "Connection object deleted" <<  endl;
	}
}

void
Connection::registerCallInterface(CallInterface *call_if_in)
{
	call_if=call_if_in;
}

void
Connection::changeProtocol(service_t desired_service, string faxStationID, string faxHeadline) throw (CapiMsgError, CapiExternalError, CapiWrongState)
{
	if (debug_level >= 2) {
		debug << prefix() << "Protocol change to " << desired_service << " requested" <<  endl;
	}

	if (ncci_state!=N0 || plci_state!=PACT)
		throw CapiWrongState("wrong state for changeProtocol","Connection::changeProtocol()");

	if (desired_service!=service) {
		_cstruct B1config=NULL, B2config=NULL, B3config=NULL;
		_cword B1proto,B2proto,B3proto;

		try {
			buildBconfiguration(plci & 0xff, desired_service, faxStationID, faxHeadline, B1proto, B2proto, B3proto, B1config, B2config, B3config);

			capi->select_b_protocol_req(plci,B1proto,B2proto,B3proto,B1config, B2config, B3config);
		} catch (...) {
			if (B1config)
				delete[] B1config;
			if (B2config)
				delete[] B2config;
			if (B3config)
				delete[] B3config;
			throw;
		}
		if (B1config)
			delete[] B1config;
		if (B2config)
			delete[] B2config;
		if (B3config)
			delete[] B3config;

		service=desired_service;
	}
}

void
Connection::connectWaiting(service_t desired_service, string faxStationID, string faxHeadline) throw (CapiWrongState,CapiExternalError,CapiMsgError)
{
	if (debug_level >= 1) {
		debug << prefix() << "accepting with service " << desired_service <<  endl;
	}
	if (debug_level >= 2) {
		debug << prefix() << "using faxStationID " << faxStationID << " faxHeadline " << faxHeadline <<  endl;
	}
	if (plci_state!=P2)
		throw CapiWrongState("wrong state for connectWaiting","Connection::connectWaiting()");

	if (our_call)
		throw (CapiExternalError("can't accept an outgoing call","Connection::connectWaiting()"));

	_cstruct B1config=NULL, B2config=NULL, B3config=NULL;
	_cword B1proto,B2proto,B3proto;

	try {
		buildBconfiguration(plci & 0xff, desired_service, faxStationID, faxHeadline, B1proto, B2proto, B3proto, B1config, B2config, B3config);

		plci_state=P4;
		capi->connect_resp(connect_ind_msg_nr,plci,0,B1proto,B2proto,B3proto,B1config, B2config, B3config);
	} catch (...) {
		if (B1config)
			delete[] B1config;
		if (B2config)
			delete[] B2config;
		if (B3config)
			delete[] B3config;
		throw;
	}
	if (B1config)
		delete[] B1config;
	if (B2config)
		delete[] B2config;
	if (B3config)
		delete[] B3config;
	service=desired_service;
}

void
Connection::rejectWaiting(_cword reject) throw (CapiWrongState, CapiMsgError, CapiExternalError)
{
	if (debug_level >= 1) {
		debug << prefix() << "rejecting with cause " << reject <<  endl;
	}
	if (plci_state!=P2)
		throw CapiWrongState("wrong state for reject","Connection::reject()");
	if (our_call)
		throw (CapiExternalError("can't reject an outgoing call","Connection::rejectWaiting()"));
	if (!reject)
		throw CapiExternalError("reject cause must not be zero","Connection::reject()");

	plci_state=P5;
	capi->connect_resp(connect_ind_msg_nr,plci,reject,0,0,0,NULL,NULL,NULL); // can throw CapiMsgError. Propagate
}

void
Connection::acceptWaiting() throw (CapiMsgError, CapiWrongState)
{
	if (plci_state!=P2)
		throw CapiWrongState("wrong state for acceptWaiting","Connection::acceptWaiting()");
	capi->alert_req(plci);
}

string
Connection::getCalledPartyNumber()
{
	return call_to;
}

string
Connection::getCallingPartyNumber()
{
	return call_from;
}

string
Connection::prefix()
{
	stringstream s;
	time_t t=time(NULL);
	char* ct=ctime(&t);
	ct[24]='\0';
	s << ct << " Connection " << hex << this << ": ";
	return (s.str());
}

void
Connection::debugMessage(string message, unsigned short level)
{
	if (debug_level >= level)
		debug << prefix() << message << endl;
}

void
Connection::errorMessage(string message)
{
	error << prefix() << message << endl;
}

Connection::service_t
Connection::getService()
{
	return service;
}

Connection::fax_info_t*
Connection::getFaxInfo()
{
	return fax_info;
}

Connection::connection_state_t
Connection::getState()
{
	if (plci_state==PACT && ncci_state==NACT)
		return UP;
	else if (plci_state==P2 && ncci_state==N0)
		return WAITING;
	else if (plci_state==P0 && ncci_state==N0)
		return DOWN;
	else
		return OTHER_STATE;
}

_cword
Connection::getCause()
{
	return disconnect_cause;
}

_cword
Connection::getCauseB3()
{
	return disconnect_cause_b3;
}

void
Connection::connect_active_ind(_cmsg& message) throw (CapiWrongState, CapiMsgError)
{
	if (plci_state!=P4 && plci_state!=P1) {
		throw CapiWrongState("CONNECT_ACTIVE_IND received in wrong state","Connection::connect_active_ind()");
  	} else {
		try {
			capi->connect_active_resp(message.Messagenumber,plci);
		}
		catch (CapiMsgError e) {
			error << prefix() << "WARNING: error detected when trying to send connect_active_resp. Message was:" << e << endl;
		}

		if (plci_state==P1) { // this is an outgoing call, so we have to initiate B3 connection
			ncci_state=N01;
			try {
				capi->connect_b3_req(plci);
			}
			catch (CapiMsgError) {
				plci_state=PACT;
				ncci_state=N0;
				throw;  // this is critical, so propagate
			}
		}
		plci_state=PACT;
	}
}

void
Connection::connect_b3_ind(_cmsg& message) throw (CapiWrongState, CapiMsgError)
{
	if (ncci_state!=N0) {
		throw CapiWrongState("CONNECT_B3_IND received in wrong state","Connection::connect_b3_ind()");
  	} else {
		ncci=CONNECT_B3_IND_NCCI(&message);

		// 0 = we'll accept any call, NULL=no NCPI necessary
		// this can throw CapiMsgError. Propagate.
		ncci_state=N2;
		capi->connect_b3_resp(message.Messagenumber,ncci,0,NULL);
	}
}

void
Connection::connect_b3_active_ind(_cmsg& message) throw (CapiError,CapiWrongState, CapiExternalError)
{
	if (ncci_state!=N2) {
		throw CapiWrongState("CONNECT_B3_ACTIVE_IND received in wrong state","Connection::connect_b3_active_ind()");
  	} else {
		if (ncci!=CONNECT_B3_ACTIVE_IND_NCCI(&message))
			throw CapiError("CONNECT_B3_ACTIVE_IND received with wrong NCCI","Connection::connect_b3_active_ind()");
		try {
			capi->connect_b3_active_resp(message.Messagenumber,ncci);
		}
		catch (CapiMsgError e) {
			error << prefix() << "WARNING: Error deteced when sending connect_b3_active_resp. Message was: " << e << endl;
		}
		ncci_state=NACT;

		if (service==FAXG3 && CONNECT_B3_ACTIVE_IND_NCPI(&message)[0]>=9) {
			_cstruct ncpi=CONNECT_B3_ACTIVE_IND_NCPI(&message);
			if (!fax_info)
				fax_info=new fax_info_t;
			fax_info->rate=ncpi[1]+(ncpi[2]<<8);
			fax_info->hiRes=((ncpi[3] & 0x01) == 0x01);
			fax_info->format=((ncpi[4] & 0x04) == 0x04);
			fax_info->pages=ncpi[7]+(ncpi[8]<<8);
			fax_info->stationID.assign(reinterpret_cast<char*>(&ncpi[10]),static_cast<int>(ncpi[9])); // indx 9 helds the length, string starts at 10
			if (debug_level >= 2) {
				debug << prefix() << "fax connected with rate " << dec << fax_info->rate
				  << (fax_info->hiRes ? ", hiRes" : ", lowRes") << (fax_info->format ? ", JPEG" : "")
				  << ", ID: " << fax_info->stationID << endl;
			}
		}

		if (call_if)
			call_if->callConnected();
		else
			throw CapiExternalError("no call control interface registered!","Connection::connect_b3_active_ind()");
	}
}

void
Connection::disconnect_b3_ind(_cmsg& message) throw (CapiError,CapiWrongState)
{
	if (ncci_state!=NACT && ncci_state!=N1 && ncci_state!=N2 && ncci_state!=N3 && ncci_state!=N4) {
		throw CapiWrongState("DISCONNECT_B3_IND received in wrong state","Connection::disconnect_b3_ind()");
	} else {
		if (ncci!=DISCONNECT_B3_IND_NCCI(&message))
			throw CapiError("DISCONNECT_B3_IND received with wrong NCCI","Connection::disconnect_b3_ind()");

		disconnect_cause_b3=DISCONNECT_B3_IND_REASON_B3(&message);

		if (service==FAXG3 && CONNECT_B3_ACTIVE_IND_NCPI(&message)[0]>=9) {
			_cstruct ncpi=CONNECT_B3_ACTIVE_IND_NCPI(&message);
			if (!fax_info)
				fax_info=new fax_info_t;
			fax_info->rate=ncpi[1]+(ncpi[2]<<8);
			fax_info->hiRes=((ncpi[3] & 0x01) == 0x01);
			fax_info->format=((ncpi[4] & 0x04) == 0x04);
			fax_info->pages=ncpi[7]+(ncpi[8]<<8);
			fax_info->stationID.assign(reinterpret_cast<char*>(&ncpi[10]),static_cast<int>(ncpi[9])); // indx 9 helds the length, string starts at 10
			if (debug_level >= 2) {
				debug << prefix() << "fax finished with rate " << dec << fax_info->rate
				  << (fax_info->hiRes ? ", hiRes" : ", lowRes") << (fax_info->format ? ", JPEG" : "")
				  << ", ID: " << fax_info->stationID << ", " << fax_info->pages << " pages" << endl;
			}
		}

		pthread_mutex_lock(&send_mutex);
		buffers_used=0; // we'll get no DATA_B3_CONF's after DISCONNECT_B3_IND, see Capi 2.0 spec, 5.18, note for DATA_B3_CONF
		pthread_mutex_unlock(&send_mutex);

		stop_file_transmission();
		stop_file_reception();

		bool our_disconnect_req= (ncci_state==N4) ? true : false;

		ncci_state=N5;

		if (call_if)
			call_if->callDisconnectedLogical();

		try {
			ncci_state=N0;
			capi->disconnect_b3_resp(message.Messagenumber,ncci);
		}
		catch (CapiMsgError e) {
			error << prefix() << "WARNING: Can't send disconnect_b3_resp. Message was: " << e << endl;
		}

		if (our_disconnect_req && !keepPhysicalConnection) { // this means *we* initiated disconnect of logical connection with DISCONNECT_B3_REQ before
			try {
				plci_state=P5;
				capi->disconnect_req(plci);  // so we'll continue with the disconnect of physical connection
			}
			catch (CapiMsgError e) {
				// in this application this is fatal. Panic please.
				throw CapiError("Can't disconnect. Please file a bug report. Error message: "+e.message(),"Connection::disconnect_b3_ind()");
			}
		} else
			keepPhysicalConnection=false;
	}
}

void
Connection::disconnect_ind(_cmsg& message) throw (CapiError,CapiWrongState,CapiMsgError)
{
	if (ncci_state!=N0 || (plci_state!=P1 && plci_state!=P2 && plci_state!=P3 && plci_state!=P4 && plci_state!=P5 && plci_state!=PACT)) {
		throw CapiWrongState("DISCONNECT_IND received in wrong state","Connection::disconnect_ind()");
	} else {
		if (plci!=DISCONNECT_IND_PLCI(&message))
			throw CapiError("DISCONNECT_IND received with wrong PLCI","Connection::disconnect_ind()");

		disconnect_cause=DISCONNECT_IND_REASON(&message);

		plci_state=P0;
		capi->disconnect_resp(message.Messagenumber,plci);
		capi->unregisterConnection(plci);

		if (call_if)
			call_if->callDisconnectedPhysical();
	}
}

void
Connection::data_b3_ind(_cmsg& message) throw (CapiError,CapiWrongState,CapiMsgError)
{
	if (ncci_state!=NACT && ncci_state!=N4)
		throw CapiWrongState("DATA_B3_IND received in wrong state","Connection::data_b3_ind()");

	if (ncci!=CONNECT_B3_IND_NCCI(&message))
		throw CapiError("DATA_B3_IND received with wrong NCCI","Connection::data_b3_ind()");

	pthread_mutex_lock(&receive_mutex);
	if (file_for_reception)	{
		for (int i=0;i<DATA_B3_IND_DATALENGTH(&message);i++)
			(*file_for_reception) << DATA_B3_IND_DATA(&message)[i];
	}
	pthread_mutex_unlock(&receive_mutex);

	if (call_if)
		call_if->dataIn(DATA_B3_IND_DATA(&message),DATA_B3_IND_DATALENGTH(&message));

	capi->data_b3_resp(message.Messagenumber,ncci,DATA_B3_IND_DATAHANDLE(&message));
}

void
Connection::facility_ind_DTMF(_cmsg &message) throw (CapiError,CapiWrongState)
{
	if (plci_state!=PACT)
		throw CapiWrongState("FACILITY_IND received in wrong state","Connection::facility_ind_DTMF()");

	if (plci!=(FACILITY_IND_PLCI(&message) & 0xFFFF) ) // this *should* be PLCI, but who knows - so mask NCCI part if it's there...
		throw CapiError("FACILITY_IND received with wrong PLCI","Connection::facility_ind_DTMF()");

	try {
		capi->facility_resp(message.Messagenumber,plci,1);
	}
	catch (CapiMsgError e) {
		error << prefix() << "WARNING: Can't send facility_resp. Message was: " << e << endl;
	}

	_cstruct facilityIndParam=FACILITY_IND_FACILITYINDICATIONPARAMETER(&message);
	received_dtmf.append(reinterpret_cast<char*>(facilityIndParam+1),static_cast<size_t>(facilityIndParam[0]));  //string, length
	if (debug_level >= 2) {
		debug << prefix() << "received DTMF buffer " << received_dtmf << endl;
	}

	if (call_if)
		call_if->gotDTMF();
}

void
Connection::info_ind_alerting(_cmsg &message) throw (CapiError,CapiWrongState)
{
	if (plci_state!=P01 && plci_state!=P1)
		throw CapiWrongState("INFO_IND for ALERTING received in wrong state","Connection::info_ind_alerting()");

	if (plci!=INFO_IND_PLCI(&message))
		throw CapiError("INFO_IND received with wrong PLCI","Connection::info_ind_alerting()");

	try {
		capi->info_resp(message.Messagenumber,plci);
	}
	catch (CapiMsgError e) {
		error << prefix() << "WARNING: Can't send info_resp. Message was: " << e << endl;
	}

	if (call_if)
		call_if->alerting();
}

bool
Connection::info_ind_called_party_nr(_cmsg &message) throw (CapiError,CapiWrongState)
{
	if (plci_state!=P2)
		throw CapiWrongState("INFO_IND for CalledPartyNr received in wrong state",
		  "Connection::info_ind_called_party_nr()");

	if (plci!=INFO_IND_PLCI(&message))
		throw CapiError("INFO_IND received with wrong PLCI",
		  "Connection::info_ind_called_party_nr()");
	try {
		capi->info_resp(message.Messagenumber,plci);
	}
	catch (CapiMsgError e) {
		error << prefix() << "WARNING: Can't send info_resp. Message was: " << 
		  e << endl;
	}

	call_to+=getNumber(INFO_IND_INFOELEMENT(&message),false);

	string currDDI;
	try {
		currDDI=call_to.substr(DDIBaseLength);
	}
	catch (std::out_of_range e) {
		throw CapiError("DDIBaseLength too big - configuration error?",
		  "Connection::info_ind_called_party_nr()");
	}
	for (int i=0;i<DDIStopNumbers.size();i++)
	    if (DDIStopNumbers[i]==currDDI) {
			if (debug_level >= 1)
	    			debug << prefix() << "got DDI, nr is now " << call_to << " (complete,stop_nr)" << endl;
			return true;
	    }

	if (call_to.length()>=DDIBaseLength+DDILength) {
		if (debug_level >=1)
                	debug << prefix() << "got DDI, nr is now " << call_to << " (complete)" << endl;
		return true;
	} else {
		if (debug_level >=1)
                        debug << prefix() << "got DDI, nr is now " << call_to << " (incomplete)" << endl;
                return false;
	}
}

void
Connection::connect_conf(_cmsg& message) throw (CapiWrongState, CapiMsgError)
{
	if (plci_state!=P01)
		throw CapiWrongState("CONNECT_CONF received in wrong state","Connection::connect_conf()");

	if (CONNECT_CONF_INFO(&message))
		throw CapiMsgError(CONNECT_CONF_INFO(&message),"CONNECT_CONF received with Error (Info)","Connection::connect_conf()");
		// TODO: do we have to delete Connection here if Info!=0 or is a DISCONNECT_IND initiated then (think not ...)

	plci=CONNECT_CONF_PLCI(&message);
	if (debug_level >= 2) {
		debug << prefix() << "got PLCI " << plci << endl;
	}

	plci_state=P1;
}

void
Connection::connect_b3_conf(_cmsg& message) throw (CapiWrongState, CapiMsgError)
{
	if (ncci_state!=N01)
		throw CapiWrongState("CONNECT_B3_CONF received in wrong state","Connection::connect_b3_conf()");

	if (CONNECT_B3_CONF_INFO(&message)) {
		ncci_state=N0;
		throw CapiMsgError(CONNECT_B3_CONF_INFO(&message),"CONNECT_B3_CONF received with Error (Info)","Connection::connect_b3_conf()");
	}

	ncci=CONNECT_B3_CONF_NCCI(&message);

	ncci_state=N2;
}

void
Connection::select_b_protocol_conf(_cmsg& message) throw (CapiError,CapiWrongState,CapiMsgError)
{
	if (plci_state!=PACT || ncci_state!=N0)
		throw CapiWrongState("SELECT_B_PROTOCOL_CONF received in wrong state","Connection::select_b_protocol_conf()");

	if (plci!=SELECT_B_PROTOCOL_CONF_PLCI(&message))
		throw CapiError("SELECT_B_PROTOCOL_CONF received with wrong PLCI","Connection::select_b_protocol_conf()");

	if (SELECT_B_PROTOCOL_CONF_INFO(&message))
		throw CapiMsgError(SELECT_B_PROTOCOL_CONF_INFO(&message),"SELECT_B_PROTOCOL_CONF received with Error (Info)","Connection::select_b_protocol_conf()");

	if (our_call) {
		try {
			ncci_state=N01;
			capi->connect_b3_req(plci);
		}
		catch (CapiMsgError) {
			ncci_state=N0;
			throw;  // this is critical, so propagate
		}
	}
}

void
Connection::alert_conf(_cmsg& message) throw (CapiError,CapiWrongState,CapiMsgError)
{
	if (plci_state!=P2 && plci_state!=P5)
		throw CapiWrongState("ALERT_CONF received in wrong state","Connection::alert_conf()");

	if (plci!=ALERT_CONF_PLCI(&message))
		throw CapiError("ALERT_CONF received with wrong PLCI","Connection::alert_conf()");

	if (ALERT_CONF_INFO(&message) && ALERT_CONF_INFO(&message)!=0x0003) // 0x0003 = another application sent ALERT_REQ earlier -> no problem for us
		throw CapiMsgError(ALERT_CONF_INFO(&message),"ALERT_CONF received with Error (Info)","Connection::alert_conf()");
}

void
Connection::data_b3_conf(_cmsg& message) throw (CapiError,CapiWrongState,CapiMsgError,CapiExternalError)
{
	if (ncci_state!=NACT)
		throw CapiWrongState("DATA_B3_CONF received in wrong state","Connection::data_b3_conf()");

	if (ncci!=DATA_B3_CONF_NCCI(&message))
		throw CapiError("DATA_B3_CONF received with wrong NCCI","Connection::data_b3_conf()");

	if (DATA_B3_CONF_INFO(&message))
		throw CapiMsgError(DATA_B3_CONF_INFO(&message),"DATA_B3_CONF received with Error (Info)","Connection::data_b3_conf()");

	pthread_mutex_lock(&send_mutex);

	try {
		if ( (!buffers_used) || (DATA_B3_CONF_DATAHANDLE(&message)!=buffer_start) )
			throw CapiError("DATA_B3_CONF received with invalid data handle","Connection::data_b3_conf()");
		// free one buffer
		buffers_used--;
		buffer_start=(buffer_start+1)%7;
		while (file_to_send && (buffers_used < conf_send_buffers) )
			send_block();
	}
	catch (...) {
		pthread_mutex_unlock(&send_mutex);
		throw;
	}
	pthread_mutex_unlock(&send_mutex);
}

void
Connection::facility_conf_DTMF(_cmsg& message) throw (CapiError,CapiWrongState,CapiMsgError)
{
	if (plci_state!=PACT)
		throw CapiWrongState("FACILITY_CONF for DTMF received in wrong state","Connection::facility_conf_DTMF()");

	if (plci!=(FACILITY_CONF_PLCI(&message) & 0xFFFF)) // this *should* be the PLCI but to be sure we mask out NCCI part
		throw CapiError("FACILITY_CONF received with wrong PLCI","Connection::facility_conf_DTMF()");

	if (FACILITY_CONF_INFO(&message))
		throw CapiMsgError(FACILITY_CONF_INFO(&message),"FACILITY_CONF received with Error (Info)","Connection::facility_conf_DTMF()");

	_cstruct facilityConfParameter=FACILITY_CONF_FACILITYCONFIRMATIONPARAMETER(&message);
	if ((facilityConfParameter[0]==2) && facilityConfParameter[1])
		throw CapiMsgError(FACILITY_CONF_INFO(&message),"FACILITY_CONF received with DTMF Error (DTMF information)","Connection::facility_conf_DTMF()");
}

void
Connection::disconnect_b3_conf(_cmsg& message) throw (CapiError,CapiWrongState, CapiMsgError)
{
	if (ncci_state!=N4)
		throw CapiWrongState("DISCONNECT_B3_CONF received in wrong state","Connection::disconnect_b3_conf()");

	if (ncci!=DISCONNECT_B3_CONF_NCCI(&message))
		throw CapiError("DISCONNECT_B3_CONF received with wrong NCCI","Connection::disconnect_b3_conf()");

	if (DISCONNECT_B3_CONF_INFO(&message))
		throw CapiMsgError(DISCONNECT_B3_CONF_INFO(&message),"DISCONNECT_B3_CONF received with Error (Info)","Connection::disconnect_b3_conf()");
}

void
Connection::disconnect_conf(_cmsg& message) throw (CapiError,CapiWrongState,CapiMsgError)
{
	if (plci_state!=P5)
		throw CapiWrongState("DISCONNECT_CONF received in wrong state","Connection::disconnect_conf()");

	if (plci!=DISCONNECT_CONF_PLCI(&message))
		throw CapiError("DISCONNECT_CONF received with wrong PLCI","Connection::disconnect_conf()");

	if (DISCONNECT_CONF_INFO(&message))
		throw CapiMsgError(DISCONNECT_CONF_INFO(&message),"DISCONNECT_CONF received with Error (Info)","Connection::disconnect_conf()");
}

void
Connection::disconnectCall(disconnect_mode_t disconnect_mode) throw (CapiMsgError)
{
	if (debug_level >= 1) {
		debug << prefix() << "disconnect initiated" << endl;
	}
	if ((ncci_state==N1 || ncci_state==N2 || ncci_state==N3 || ncci_state==NACT) && (disconnect_mode==ALL || disconnect_mode==LOGICAL_ONLY) ) {  // logical connection up
		ncci_state=N4;
		capi->disconnect_b3_req(ncci); // can throw CapiMsgError. Fatal here. Propagate
		if (disconnect_mode==LOGICAL_ONLY)
			keepPhysicalConnection=true;
  	} else if ((plci_state==PACT || plci_state==P1 || plci_state==P2 || plci_state==P3 || plci_state==P4) && (disconnect_mode==ALL || disconnect_mode==PHYSICAL_ONLY) ) { // physical connection up
		plci_state=P5;
		capi->disconnect_req(plci); // can throw CapiMsgError. Fatal here. Propagate
  	}
	// otherwise do nothing
}

void
Connection::send_block() throw (CapiError,CapiWrongState,CapiExternalError,CapiMsgError)
{
	if (ncci_state!=NACT)
		throw CapiWrongState("unable to send file because connection is not established","Connection::send_block()");

	if (!file_to_send)
		throw CapiError("unable to play file because no input file is open","Connection::send_block()");

	if (buffers_used>=7)
		throw CapiError("unable to send file snippet because buffers are full","Connection::send_block()");

	bool file_completed=false;

	unsigned short buff_num=(buffer_start+buffers_used)%7; // buffer to store the next item

	int i=0;
	while (i<2048 && !file_completed) {
		if (!file_to_send->get(send_buffer[buff_num][i]))
			file_completed=true;
		else
	   		i++;
	}

	try {
		if (i>0) {
	  	 	capi->data_b3_req(ncci,send_buffer[buff_num],i,buff_num,0); // can throw CapiMsgError. Propagate.
			buffers_used++;
		}
	}
	catch (CapiMsgError e) {
		error << prefix() << "WARNING: Can't send data_b3_req. Message was: " << e << endl;
		if (file_completed) {
			file_to_send->close();
			delete file_to_send;
			file_to_send=NULL;
		}
	}

  	if (file_completed) {
    		file_to_send->close();
		delete file_to_send;
	 	file_to_send=NULL;
	 	if (call_if)
	 		call_if->transmissionComplete();
		else
			throw CapiExternalError("no call control interface registered!","Connection::send_block()");
  	}
}

void
Connection::start_file_transmission(string filename) throw (CapiError,CapiWrongState,CapiExternalError,CapiMsgError)
{
	if (debug_level >= 2) {
		debug << prefix() << "start_file_transmission " << filename << endl;
	}
	if (ncci_state!=NACT)
		throw CapiWrongState("unable to send file because connection is not established","Connection::start_file_transmission()");

	if (file_to_send)
		throw CapiExternalError("unable to send file because transmission is already in progress","Connection::start_file_transmission()");

	file_to_send=new ifstream(filename.c_str());

	if (! (*file_to_send)) { // we can't open the file
		delete file_to_send;
		file_to_send=NULL;
		throw CapiExternalError("unable to open file to send ("+filename+")","Connection::start_file_transmission()");
	} else {
		pthread_mutex_lock(&send_mutex);
		try {
			while (file_to_send && buffers_used<conf_send_buffers)
				send_block();
		}
		catch (...) {
			pthread_mutex_unlock(&send_mutex);
			throw;
		}
		pthread_mutex_unlock(&send_mutex);
	}
}

void
Connection::stop_file_transmission()
{
	if (debug_level >= 2) {
		debug << prefix() << "stop_file_transmission initiated" << endl;
	}
	pthread_mutex_lock(&send_mutex);
	if (file_to_send) {
		file_to_send->close();
		delete file_to_send;
		file_to_send=NULL;
	}
	pthread_mutex_unlock(&send_mutex);

	timespec delay_time;
	delay_time.tv_sec=0; delay_time.tv_nsec=100000000;  // 100 msec
	while (buffers_used) // wait until all packages are transmitted
		nanosleep(&delay_time,NULL);
	if (debug_level >= 2) {
		debug << prefix() << "stop_file_transmission finished" << endl;
	}
}

void
Connection::start_file_reception(string filename) throw (CapiWrongState, CapiExternalError)
{
	if (debug_level >= 2) {
		debug << prefix() << "start_file_reception " << filename << endl;
	}
	if (ncci_state!=NACT)
		throw CapiWrongState("unable to receive file because connection is not established","Connection::start_file_reception()");

	if (file_for_reception)
		throw CapiExternalError("file reception is already active","Connection::start_file_reception()");

  	file_for_reception=new ofstream(filename.c_str());
  	if (! (*file_for_reception)) { // we can't open the file
  		delete file_for_reception;
		file_for_reception=NULL;
		throw CapiExternalError("unable to open file for reception ("+filename+")","Connection::start_file_reception()");
  	}
}

void
Connection::stop_file_reception()
{
	pthread_mutex_lock(&receive_mutex);

	if (file_for_reception) {
  		file_for_reception->close();
		delete file_for_reception;
		file_for_reception=NULL;
	}

	pthread_mutex_unlock(&receive_mutex);
	if (debug_level >= 2) {
		debug << prefix() << "stop_file_reception finished" << endl;
	}
}

void
Connection::enableDTMF() throw (CapiWrongState, CapiMsgError)
{
 	if (plci_state!=PACT)
		throw CapiWrongState("unable to enable DTMF because connection is not established","Connection::enableDTMF()");

	_cstruct facilityRequestParameter=new unsigned char[1+2+2+2+1+3];
	int i=0;
	facilityRequestParameter[i++]=2+2+2+1+3; // total length
	facilityRequestParameter[i++]=1; facilityRequestParameter[i++]=0;  // start DTMF listen
	facilityRequestParameter[i++]=40; facilityRequestParameter[i++]=0;  // default value for tone-duration
	facilityRequestParameter[i++]=40; facilityRequestParameter[i++]=0;  // default value for gap-duration
	facilityRequestParameter[i++]=0; // we don't want to send DTMF now (=empty struct)
	facilityRequestParameter[i++]=2; // now let's start substruct DTMF Characteristics (length)
	facilityRequestParameter[i++]=0; facilityRequestParameter[i++]=0;  // default value for DTMF Selectivity

	try {
		capi->facility_req(plci,1,facilityRequestParameter);
	}
	catch  (CapiMsgError) {
		delete[] facilityRequestParameter;
		throw;
	}
	delete[] facilityRequestParameter;
}

void
Connection::disableDTMF() throw (CapiWrongState, CapiMsgError)
{
 	if (plci_state!=PACT)
		throw CapiWrongState("unable to disable DTMF because connection is not established","Connection::disableDTMF()");

	_cstruct facilityRequestParameter=new unsigned char[1+2+2+2+1+1];
	int i=0;
	facilityRequestParameter[i++]=2+2+2+1+1; // total length
	facilityRequestParameter[i++]=2; facilityRequestParameter[i++]=0;  // stop DTMF listen
	facilityRequestParameter[i++]=40; facilityRequestParameter[i++]=0;  // default value for tone-duration
	facilityRequestParameter[i++]=40; facilityRequestParameter[i++]=0;  // default value for gap-duration
	facilityRequestParameter[i++]=0; // we don't want to send DTMF now (=empty struct)
	facilityRequestParameter[i++]=0; // no DTMF Characteristics

	try {
		capi->facility_req(plci,1,facilityRequestParameter);
	}
	catch (CapiMsgError) {
		delete[] facilityRequestParameter;
		throw;
	}
	delete[] facilityRequestParameter;
}

string
Connection::getDTMF()
{
	return received_dtmf;
}

void
Connection::clearDTMF()
{
#ifdef HAVE_STRING_CLEAR
 	received_dtmf.clear();
#else
	received_dtmf="";
#endif
}

string
Connection::getNumber(_cstruct capi_input, bool isCallingNr)
{
// CallingNr: byte 0: length (w/o byte 0), Byte 1+2 see ETS 300 102-1, Chapter 4.5, byte 3-end: number (w/o leading "0" or "00")
// CalledNr:  byte 0: length (w/o byte 0), Byte 1 see ETS 300 102-1, Chapter 4, byte 2-end: number w/o leading "0" or "00"
	int length=capi_input[0];

	if (!length) // no info element given
		return "-";

	char *nr=new char[length];
	memcpy (nr,&capi_input[2],length-1); // copy only number
	nr[length-1]='\0';                   // add \0
	string a(nr);
	if (isCallingNr)
		a=a.substr(1);

	// if we are looking at a CallingPartyNumber and it is an international number or a national number
	// (see ETS 300 102-1, chapter 4.5), we'll add the prefix "0" or "+"

	if (a.empty()) {
		a="-";
	} else if (isCallingNr && ((capi_input[1] & 0x70) == 0x20)) {  // national number
		a='0'+a;
	} else if (isCallingNr && ((capi_input[1] & 0x70) == 0x10)) { // international number
		a='+'+a;
	}
	return a;
}

void
Connection::buildBconfiguration(_cdword controller, service_t service, string faxStationID, string faxHeadline, _cword& B1proto, _cword& B2proto, _cword& B3proto, _cstruct& B1config, _cstruct& B2config, _cstruct& B3config) throw (CapiExternalError)
{
	switch (service) {
		case VOICE:
			if (!capi->profiles[controller-1].transp)
				throw (CapiExternalError("controller doesn't support voice (transparent) services","Connection::buildBconfiguration()"));
			B1proto=1;  // bit-transparent
			B2proto=1;  // Transparent
			B3proto=0;  // Transparent
			B1config=NULL; // no configuration for bit-transparent available
			B2config=NULL; // no configuration for transparent available
			B3config=NULL; // no configuration for transparent available
		break;

		case FAXG3: {
			B1proto=4; // T.30 modem for Fax G3
			B2proto=4; // T.30 for Fax G3
			if (capi->profiles[controller-1].faxExt)
				B3proto=5; // T.30 for Fax G3 Extended
			else if (capi->profiles[controller-1].fax)
				B3proto=4; // T.30 for Fax G3
			else
				throw (CapiExternalError("controller doesn't support fax services","Connection::buildBconfiguration()"));

			B1config=NULL; // default configuration (adaptive maximum baud rate, default transmit level)
			B2config=NULL; // no configuration available

			if (faxStationID.size()>20) // stationID mustn't exceed 20 characters
				faxStationID=faxStationID.substr(0,20);
			if (faxHeadline.size()>254)  // if the string would be longer the struct must be coded different, but I think a header > 254 bytes has no sence
				faxHeadline=faxHeadline.substr(0,254);

			// convert faxHeadline to CP437 for AVM drivers as they expect the string in this format
			if (capi->profiles[controller-1].manufacturer.find("AVM")!=std::string::npos)
				convertToCP437(faxHeadline);

			B3config=new unsigned char [1+2+2+1+faxStationID.size()+1+faxHeadline.size()]; // length + 1 byte for the length itself
			int i=0;
			B3config[i++]=2+2+1+faxStationID.size()+1+faxHeadline.size();  // length
			B3config[i++]=0; B3config[i++]=4; // resolution = standard; accept color faxes
			B3config[i++]=0; B3config[i++]=0; // format: SFF
			B3config[i++]=faxStationID.size();
			for (unsigned j=0;j<faxStationID.size();j++)
				B3config[i++]=faxStationID[j];
			B3config[i++]=faxHeadline.size();
			for (unsigned j=0;j<faxHeadline.size();j++)
				B3config[i++]=faxHeadline[j];
		} break;

		default:
			throw CapiExternalError("unsupported service given by application","Connection::buildBconfiguration()");
		break;
	}
}

void
Connection::convertToCP437(string &text)
{
	size_t from_length=text.size()+1;
	size_t to_length=from_length;

	char* from_buf=new char[from_length];
	char* from_buf_tmp=from_buf; // as pointer is changed by iconv()
	char* to_buf = new char[to_length];
	char* to_buf_tmp=to_buf; // as pointer is changed by iconv()

	strncpy(from_buf,text.c_str(),from_length);

	iconv_t conv=iconv_open("CP437","Latin1");

	if (conv==(iconv_t)-1) {
		error << prefix() << "WARNING: string conversion to CP437 not supported by iconv" << endl;
		return;
	}

	if (iconv(conv,&from_buf_tmp,&from_length,&to_buf_tmp,&to_length)==(size_t)-1) {
		char msg[200];
		error << prefix() << "WARNING: error during string conversion (iconv): " << strerror_r(errno,msg,200) << endl;
		return;
	}

	if (iconv_close(conv)!=0)
		throw CapiExternalError("error during string conversion (iconv_close)","Connection::convertToCP437");

	text=to_buf;
        delete[] from_buf;
        delete[] to_buf;
}


/*  History

Old Log (for new changes see ChangeLog):
Revision 1.13  2003/12/21 21:15:10  gernot
* src/backend/connection.cpp (buildBconfiguration): accept
  color faxes now by setting bit 10 in B3configuration

Revision 1.12  2003/07/20 19:08:19  gernot
- added missing include of errno.h

Revision 1.11.2.5  2003/11/06 18:32:15  gernot
- implemented DDIStopNumbers

Revision 1.11.2.4  2003/11/02 14:58:16  gernot
- use DDI_base_length instead of DDI_base
- added DDI_stop_numbers option
- use DDI_* options in the Connection class
- call the Python script if number is complete

Revision 1.11.2.3  2003/11/01 22:59:33  gernot
- read CalledPartyNr InfoElements

Revision 1.11.2.2  2003/10/26 16:52:55  gernot
- begin implementation of DDI; get DDI info elements

Revision 1.11.2.1  2003/07/20 19:08:44  gernot
- added missing include of errno.h

Revision 1.11  2003/06/29 06:18:13  gernot
- don't take a wrong character too serious...

Revision 1.10  2003/06/28 12:49:47  gernot
- convert fax headline to CP437, so that german umlauts and other special
  characters will work now

Revision 1.9  2003/05/25 13:38:30  gernot
- support reception of color fax documents

Revision 1.8  2003/05/24 13:48:54  gernot
- get fax details (calling station ID, transfer format, ...), handle PLCI

Revision 1.7  2003/04/17 10:39:42  gernot
- support ALERTING notification (to know when it's ringing on the other side)
- cosmetical fixes in capi.cpp

Revision 1.6  2003/04/17 10:36:29  gernot
- fix another typo which could probably lead to errors in sending own number...

Revision 1.5  2003/04/10 21:29:51  gernot
- support empty destination number for incoming calls correctly (austrian
  telecom does this (sic))
- core now returns "-" instead of "??" for "no number available" (much nicer
  in my eyes)
- new wave file used in remote inquiry for "unknown number"

Revision 1.4  2003/04/04 09:17:59  gernot
- buildBconfiguration() now checks the abilities of the given controller
  and throws an error if it doesn't support the service
- it also sets the fax protocol setting now the highest available ability
  (fax G3 or fax G3 extended) of the controller, thus preparing fax polling
  and *working around a severe bug in the AVM drivers producing a kernel
  oops* with some analog fax devices. AVM knows about this and analyzes it.

Revision 1.3  2003/03/21 23:09:59  gernot
- included autoconf tests for gcc-2.95 problems so that it will compile w/o
  change for good old gcc-2.95 and gcc3

Revision 1.2  2003/02/28 21:36:51  gernot
- don't allocate new B3config in buildBconfiguration(), fixes bug 532
- limit stationID to 20 characters

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.44  2003/02/10 14:20:52  ghillie
merged from NATIVE_PTHREADS to HEAD

Revision 1.43  2003/02/09 15:16:29  ghillie
- fixed some delete calls to delete[]

Revision 1.42.2.1  2003/02/10 14:10:27  ghillie
- use pthread_mutex_* instead of CommonC++ semaphores

Revision 1.42  2003/01/31 16:33:13  ghillie
- callingParty wasn't set

Revision 1.41  2003/01/31 11:27:50  ghillie
- wrong initialization of debug_level for outgoing connections fixed

Revision 1.40  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.39  2003/01/13 21:30:23  ghillie
- FIX: removed erroneous checking of connect_ind_msg_nr in rejectWaiting()
  and checked for our_call instead (oops, overlooked this one ;-) )

Revision 1.38  2003/01/04 16:08:22  ghillie
- log improvements: log_level, timestamp
- added methods debugMessage(), errorMessage(), removed get*Stream()
- added some additional debug output for connection setup / finish

Revision 1.37  2002/12/18 14:46:07  ghillie
- removed debug output

Revision 1.36  2002/12/18 14:45:13  ghillie
- moved *_state=XY actions direct in front of messages sent to CAPI, so that
  parallel executed threads don't see a wrong state (hopefully)
- removed test for connect_ind_msg_nr!=0 in connectWaiting(). Don't know
  what I intended with this (sigh)
- added missing "{" in select_b_protocol_conf() :-(
- removed unnecessary plci_state=PACT in select_b_protocol_conf

Revision 1.35  2002/12/16 15:05:47  ghillie
- FIX: corrected disconnect behaviour (physical connection is now disconnected
  correctly)

Revision 1.34  2002/12/16 13:13:47  ghillie
- added getCauseB3 to return B3 cause

Revision 1.33  2002/12/13 11:46:19  ghillie
- new attribute our_call to inidicate that we initiated a connection
- send CONNECT_B3_REQ after receiving SELECT_B_PROTOCOL_CONF for outgoing calls

Revision 1.32  2002/12/13 09:57:44  ghillie
- error message formatting done by exception classes now

Revision 1.31  2002/12/11 13:38:43  ghillie
- FIX: added missing init of keepPhysicalConnection in outgoing constructor
- use quick disconnect (PHYSICAL_ONLY) in destructor
- disconnectCall(): added support for PHYSICAL_ONLY disconnect

Revision 1.30  2002/12/10 15:06:15  ghillie
- new methods get*Stream() for use in capisuitemodule

Revision 1.29  2002/12/09 15:42:07  ghillie
- saves debug and error stream in own attributes now
- debug output improvements, error output included
- unregistering at Capi now done as soon as DISCONNECT_IND is received

Revision 1.28  2002/12/06 15:25:39  ghillie
- cleaned up and fixed destructor (wrong order of some calls)
- new return value for getState(): WAITING

Revision 1.27  2002/12/06 13:06:44  ghillie
- added support for saving disconnect cause
- ~Connection does busy wait for disconnect to prevent Connection objects
  going away while the corresponding call is still active
- added error checking for connect_ind_msg_nr
- new methods getState() and getCause()

Revision 1.26  2002/12/05 15:04:29  ghillie
- Capi::connect_req() gets this now
- call capi->unregisterConnection(plci) in destructor
- connect_conf() sets plci attribute
- connect_b3_conf() sets ncci attribute

Revision 1.25  2002/12/04 10:43:43  ghillie
- small FIX in getNumber(): added missing parantheses in if condition -> national number & international number work now

Revision 1.24  2002/12/02 12:31:10  ghillie
- renamed Connection::SPEECH to Connection::VOICE

Revision 1.23  2002/11/29 10:25:01  ghillie
- updated comments, use doxygen format now

Revision 1.22  2002/11/27 16:02:54  ghillie
- added missing throw() declaration in changeProtocol()
- added missing state check in acceptWaiting()
- data_b3_ind and disconnect_ind propagate CapiMsgError now
- DTMF handling routines and select_b_protocol_conf test for state of physical connection instead of logical connection now

Revision 1.21  2002/11/25 11:51:45  ghillie
- removed the unhandy CIP parameters from the interface to the application layer, use service type instead
- rejectWaiting() tests against cause!=0 now
- removed isUp() method

Revision 1.20  2002/11/22 15:13:44  ghillie
- new attribute keepPhysicalConnection which prevents disconnect_b3_ind() from sending disconnect_req()
- moved the ugly B*configuration, B*protocol settings from some methods to private method buildBconfiguration
- new methods changeProtocol(), select_b_protocol_conf(), clearDTMF()
- disconnect_b3_ind sets ncci_state to N0 before calling the callbacks
- added parameter disconnect_mode to disconnectCall()
- getDTMF() does non-destructive read now

Revision 1.19  2002/11/21 15:28:12  ghillie
- removed ALERT_REQ sending from constructor - this is now done by the python functions connect_*()
- new method Connection::acceptWaiting() - sends ALERT_REQ for use by the above mentioned python functions
- connectWaiting changes cipValue now

Revision 1.18  2002/11/20 17:24:58  ghillie
- added check if call_if is set in data_b3_ind before it's called (ouch!)
- changed impossible error to ::FATAL in send_block()

Revision 1.17  2002/11/19 15:57:18  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.16  2002/11/18 14:24:09  ghillie
- moved global severity_t to CapiError::severity_t
- added throw() declarations

Revision 1.15  2002/11/18 12:23:17  ghillie
- fix: set buffers_used to 0 in critical section in Connection::disconnect_b3_ind()
- disconnectCall() doesn't throw exception any more (does nothing if we have wrong state),
  so we can call it w/o knowledge if connection is still up

Revision 1.14  2002/11/17 14:40:47  ghillie
- improved exception throwing, different exception kinds are used now
- added isUp()

Revision 1.13  2002/11/15 15:25:53  ghillie
added ALERT_REQ so we don't loose a call when we wait before connection establishment

Revision 1.12  2002/11/15 13:49:10  ghillie
fix: callmodule wasn't aborted when call was only connected/disconnected physically

Revision 1.11  2002/11/14 17:05:19  ghillie
major structural changes - much is easier, nicer and better prepared for the future now:
- added DisconnectLogical handler to CallInterface
- DTMF handling moved from CallControl to Connection
- new call module ConnectModule for establishing connection
- python script reduced from 2 functions to one (callWaiting, callConnected
  merged to callIncoming)
- call modules implement the CallInterface now, not CallControl any more
  => this freed CallControl from nearly all communication stuff

Revision 1.10  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.9  2002/11/12 15:51:12  ghillie
minor fixes (avoid deadlock, don't wait for DATA_B3_CONF after DISCONNECT_B3_IND) in file_transmission code
added dataIn handler
minor fixes (and reformatting) in getNumber()

Revision 1.8  2002/11/10 17:05:18  ghillie
changed to support multiple buffers -> deadlock in stop_file_transmission!!

Revision 1.7  2002/11/08 07:57:06  ghillie
added functions to initiate a call
corrected FACILITY calls to use PLCI instead of NCCI in DTMF processing as told by Mr. Ortmann on comp.dcom.isdn.capi

Revision 1.6  2002/10/31 15:39:04  ghillie
added missing FACILITY_RESP message (oops...)

Revision 1.5  2002/10/31 12:40:06  ghillie
added DTMF support
small fixes like making some unnecessary global variables local, removed some unnecessary else cases

Revision 1.4  2002/10/30 14:29:25  ghillie
added getCIPvalue

Revision 1.3  2002/10/30 10:47:13  ghillie
added debug output

Revision 1.2  2002/10/29 14:27:09  ghillie
added stop_file_*, added semaphore calls to guarantee right order of execution (I hope ;-) )

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.15  2002/10/24 09:55:52  ghillie
many fixes. Works for one call now

Revision 1.14  2002/10/23 09:43:05  ghillie
small variable name change (stationID->faxStationID)

Revision 1.13  2002/10/10 12:45:40  gernot
added AudioReceive module, some small details changed

Revision 1.12  2002/10/09 14:36:22  gernot
added CallModule base class for all call handling modules

Revision 1.11  2002/10/09 11:18:59  gernot
cosmetic changes (again...) and changed info function of CAPI class

Revision 1.10  2002/10/05 13:53:00  gernot
changed to use thread class of CommonC++ instead of the threads-package
some cosmetic improvements (indentation...)

Revision 1.9  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.8  2002/10/04 13:27:15  gernot
some restructuring to get it to a working state ;-)
does not do anything useful yet nor does it even compile...

Revision 1.7  2002/10/01 09:02:04  gernot
changes for compilation with gcc3.2

Revision 1.6  2002/09/22 14:22:53  gernot
some cosmetic comment improvements ;-)

Revision 1.5  2002/09/19 12:08:19  gernot
added magic CVS strings

Revision 1.4  2002/09/18 16:59:48  gernot
added version info

* Sun Sep 15 2002 - gernot@hillier.de
- put under CVS, cvs log follows above

* Sun May 20 2002 - gernot@hillier.de
- first version

*/
