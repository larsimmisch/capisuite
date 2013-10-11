/*  @file capi.h
    @brief Contains Capi - Main Class for communication with CAPI

    @author Gernot Hillier <gernot@hillier.de>
    $Revision: 1.7 $
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <iostream>
#include <sstream>
#include <cstdlib>
#include "connection.h"
#include "applicationinterface.h"
#include "capi.h"
#include "../../config.h"

// initialize static members
short Capi::numControllers=0;
string Capi::capiManufacturer, Capi::capiVersion;
vector <Capi::CardProfileT> Capi::profiles;

void* capi_exec_handler(void* arg)
{
        if (!arg) {
                cerr << "FATAL ERROR: no Capi reference given in capi_exec_handler" << endl;
		exit(1);
	}

	Capi *instance=static_cast<Capi*>(arg);
	instance->run();
	return NULL;
}

Capi::Capi (ostream& debug, unsigned short debug_level, ostream &error, unsigned short DDILength, unsigned short DDIBaseLength, vector<string> DDIStopNumbers, unsigned maxLogicalConnection, unsigned maxBDataBlocks,unsigned maxBDataLen) throw (CapiError, CapiMsgError)
:debug(debug),debug_level(debug_level),error(error),messageNumber(0),usedInfoMask(0x10),usedCIPMask(0),
DDILength(DDILength),DDIBaseLength(DDIBaseLength),DDIStopNumbers(DDIStopNumbers)
{
	if (debug_level >= 2)
		debug << prefix() << "Capi object created" << endl;
	Capi::readProfile(); // can throw CapiMsgError. Just propagate...

	if (Capi::numControllers==0)
		throw (CapiError("No ISDN-Controller installed","Capi::Capi()"));

	if (!maxLogicalConnection) {
		for (unsigned i=1;i<=Capi::numControllers;i++) {
			maxLogicalConnection+=profiles[i-1].bChannels;
		}
	}
	if (debug_level >= 2)
		debug << prefix() << "Registering for handling max. " << maxLogicalConnection << " logical connections" << endl;

	unsigned info = capi20_register(maxLogicalConnection, maxBDataBlocks, maxBDataLen, &applId);
	if (applId == 0 || info!=0)
        	throw (CapiMsgError(info,"Error while registering application: "+describeParamInfo(info),"Capi::Capi()"));

	if (DDILength)
		usedInfoMask|=0x80; // enable Called Party Number Info Element for PtP configuration

	for (int i=1;i<=Capi::numControllers;i++)
		listen_req(i, usedInfoMask, usedCIPMask); // can throw CapiMsgError

	int erg=pthread_create(&thread_handle, NULL, capi_exec_handler, this); // create a normal thread
	if (erg!=0)
		throw (CapiMsgError(erg,"Error while starting message thread","Capi::Capi()"));
}

Capi::~Capi ()
{
	int ret=pthread_cancel(thread_handle); // tell run() to end
	if (ret)
		throw (CapiMsgError(ret,"Error while cancelling Capi thread","Capi::~Capi()"));
	ret=pthread_join(thread_handle,NULL);
	if (ret)
		throw (CapiMsgError(ret,"Error while joining Capi thread","Capi::~Capi()"));

	unsigned info = capi20_release(applId); // this will abort capi20_waitformessage
	if (info != 0)
		throw (CapiMsgError(info,"Error while unregistering application: "+describeParamInfo(info),"Capi::~Capi()"));

	if (debug_level >= 2)
		debug << prefix() << "Capi object deleted. Let's go to bed..." << endl;
}

void
Capi::registerApplicationInterface(ApplicationInterface* application_in)
{
	application=application_in;
	if (debug_level >= 1) {
		debug << prefix() << "Registered successful at CAPI with ApplId " << applId << endl;
	}
}

void
Capi::unregisterConnection(_cdword plci)
{
	connections.erase(plci);
}

void
Capi::listen_req(_cdword Controller, _cdword InfoMask, _cdword CIPMask) throw (CapiMsgError)
{
   	_cmsg CMSG;  // Nachrichten-Struktur
	usedInfoMask=InfoMask;
	usedCIPMask=CIPMask;

	if (debug_level >= 2) {
    		debug << prefix() << ">LISTEN_REQ ApplID 0x" << hex << applId << " msgNum 0x" << messageNumber << " Controller 0x" << Controller << " InfoMask 0x"
		 << InfoMask << " CIPMask 0x" << CIPMask << " 0x0 NULL NULL" << endl;
	}
	unsigned info=LISTEN_REQ(&CMSG, applId, messageNumber++, Controller, InfoMask,CIPMask,0,NULL,NULL);
	if (debug_level >= 2) {
		debug << prefix() << "info: " << info << endl;
	}

    	if (info != 0)
       		throw(CapiMsgError(info,"Error while LISTEN_REQ: "+Capi::describeParamInfo(info),"Capi::listen_req()"));
}

void
Capi::alert_req(_cdword plci) throw (CapiMsgError)
{
   	_cmsg CMSG;  // Nachrichten-Struktur

	if (debug_level >= 2) {
	    	debug << prefix() << ">ALERT_REQ: ApplId 0x" << hex << applId << ", MsgNr 0x" << messageNumber << ", PLCI 0x" << plci << endl;
	}
	unsigned info=ALERT_REQ(&CMSG, applId, messageNumber++, plci, 
	    NULL, NULL, NULL, NULL
#ifndef HAVE_CAPI_LIBRARY_V2
	    , NULL
#endif
	    );
	if (debug_level >= 2) {
	    	debug << prefix() << "info: " << info << endl;
	}

    	if (info != 0)
       		throw(CapiMsgError(info,"Error while ALERT_REQ: "+Capi::describeParamInfo(info),"Capi::alert_req()"));
}


void
Capi::connect_req(Connection *conn, _cdword controller, _cword CIPValue, _cstruct calledPartyNumber, _cstruct callingPartyNumber, _cword B1protocol, _cword B2protocol, _cword B3protocol, _cstruct B1configuration, _cstruct B2configuration, _cstruct B3configuration) throw (CapiMsgError)
{
	_cmsg CMSG;  // Nachrichten-Struktur

	messageNumber++;

	connections[0xFACE0000 | messageNumber]=conn; // pseudo PLCI to see which CONNECT_CONF corresponds to which CONNECT_REQ
						      // this can't be a valid NCCI as a valid ncci & 0xFF000000 = 0

	if (debug_level >= 2) {
		debug << prefix() << ">CONNECT_REQ: ApplId 0x" << hex << applId << ", MsgNr 0x" << messageNumber << ", Controller 0x" << controller
		<< " CIPValue 0x" << CIPValue << ", B1proto 0x" << B1protocol << ", B2proto 0x" << B2protocol <<", B3proto 0x" << B3protocol << endl;
	}
	unsigned info=CONNECT_REQ(
		&CMSG, applId, messageNumber, controller, CIPValue, 
		calledPartyNumber, callingPartyNumber, NULL, NULL,
		B1protocol, B2protocol, B3protocol, B1configuration, B2configuration, 
		B3configuration,
#ifndef HAVE_CAPI_LIBRARY_V2
		NULL,
#endif
		NULL, NULL, NULL, NULL, NULL, NULL, NULL
#ifndef HAVE_CAPI_LIBRARY_V2
		, NULL
#endif
		);
	if (debug_level >= 2) {
		debug << prefix() << "info: " << info << endl;
	}

	if (info != 0)
		throw(CapiMsgError(info,"Error while CONNECT_REQ: "+Capi::describeParamInfo(info),"Capi::connect_req()"));
}

void
Capi::connect_b3_req(_cdword plci) throw (CapiMsgError)
{
   	_cmsg CMSG;  // Nachrichten-Struktur

	if (debug_level >= 2) {
		debug << prefix() << ">CONNECT_B3_REQ: ApplId 0x" << hex << applId << ", MsgNr 0x" << messageNumber << ", PLCI 0x" << plci << endl;
	}
	unsigned info=CONNECT_B3_REQ(&CMSG, applId, messageNumber++, plci, NULL);
	if (debug_level >= 2) {
	    	debug << prefix() << "info: " << info << endl;
	}

    	if (info != 0)
       		throw(CapiMsgError(info,"Error while CONNECT_B3_REQ: "+Capi::describeParamInfo(info),"Capi::connect_b3_req()"));
}

void
Capi::select_b_protocol_req (_cdword plci, _cword B1protocol, _cword B2protocol, _cword B3protocol, _cstruct B1configuration, _cstruct B2configuration, _cstruct B3configuration) throw (CapiMsgError)
{
   	_cmsg CMSG;  // Nachrichten-Struktur

	if (debug_level >= 2)	    	debug << prefix() << ">SELECT_B_PROTOCOL_REQ: ApplId 0x" << hex << applId << ", MsgNr 0x" << messageNumber << ", PLCI 0x" << plci
	 		     << ", B1protocol " << B1protocol << ", B2protocol " << B2protocol << ", B3protocol " << B3protocol << endl;
	unsigned info=SELECT_B_PROTOCOL_REQ(
		&CMSG, applId, messageNumber++, plci, B1protocol, B2protocol, 
		B3protocol, B1configuration, B2configuration, B3configuration
#ifndef HAVE_CAPI_LIBRARY_V2
		, NULL
#endif
		);
	if (debug_level >= 2)
			debug << prefix() << "info: " << info << endl;

    	if (info != 0)
       		throw(CapiMsgError(info,"Error while SELECT_B_PROTOCOL_REQ: "+Capi::describeParamInfo(info),"Capi::select_b_protocol_req()"));
}

void
Capi::data_b3_req (_cdword ncci, void* Data, _cword DataLength,_cword DataHandle,_cword Flags) throw (CapiMsgError)
{
	_cmsg    CMSG;  // Nachrichten-Struktur
	if (debug_level >= 3)
		debug << prefix() << ">DATA_B3_REQ ApplId 0x" << hex << applId << ", msgNum 0x" << messageNumber << ", NCCI 0x" << ncci << dec
	 		 << ", DataLen " << DataLength << ", DataHandle " << DataHandle << hex << ", Flags 0x" << Flags << endl;
	unsigned info=DATA_B3_REQ(&CMSG, applId, messageNumber++, ncci, Data, DataLength, DataHandle, Flags);
	if (debug_level >= 3)
			debug << prefix() << "info: " << info << endl;

	if (info != 0)
		throw(CapiMsgError(info,"Error while DATA_B3_REQ: "+Capi::describeParamInfo(info),"Capi::data_b3_req()"));
}

void
Capi::disconnect_b3_req (_cdword ncci, _cstruct ncpi) throw (CapiMsgError)
{
	_cmsg    CMSG;  // Nachrichten-Struktur
	if (debug_level >= 2)
		debug << prefix() << ">DISCONNECT_B3_REQ ApplId 0x" << hex << applId << " MsgNum 0x" << messageNumber << " NCCI 0x" << ncci << endl;
	unsigned info=DISCONNECT_B3_REQ(&CMSG, applId, messageNumber++, ncci, ncpi);
	if (debug_level >= 2)
		debug << prefix() << "info: " << info << endl;

	if (info != 0)
		throw(CapiMsgError(info,"Error while DISCONNECT_B3_REQ: "+Capi::describeParamInfo(info),"Capi::disconnect_b3_req()"));
}

void
Capi::disconnect_req (_cdword plci, _cstruct Keypadfacility, _cstruct Useruserdata, _cstruct Facilitydataarray) throw (CapiMsgError)
{
	_cmsg CMSG;  // Nachrichten-Struktur
	if (debug_level >= 2) {
		debug << prefix() << ">DISCONNECT_REQ ApplId 0x" << hex << applId << " MsgNum 0x" << messageNumber << " PLCI 0x" << plci << endl;
	}
	unsigned info=DISCONNECT_REQ(&CMSG, applId, messageNumber++, plci, NULL, Keypadfacility, Useruserdata, Facilitydataarray);
	if (debug_level >= 2) {
		debug << prefix() << "info: " << info << endl;
	}

	if (info != 0)
		throw(CapiMsgError(info,"Error while DISCONNECT_REQ: "+Capi::describeParamInfo(info),"Capi::disconnect_req()"));
}

void
Capi::facility_req (_cdword address, _cword FacilitySelector, _cstruct FacilityRequestParameter) throw (CapiMsgError)
{
	_cmsg CMSG;	// Nachrichten-Struktur
	if (debug_level >= 2) {
		debug << prefix() << ">FACILITY_REQ ApplId 0x" << hex << applId << ", MsgNr 0x" << messageNumber << ", Address 0x" << address << ", FacilitySelector 0x" << FacilitySelector << endl;
	}
	unsigned info=FACILITY_REQ(&CMSG, applId, messageNumber++, address, FacilitySelector, FacilityRequestParameter);
	if (debug_level >= 2) {
		debug << prefix() << "info: " << info << endl;
	}

    	if (info != 0)
       		throw(CapiMsgError(info,"Error while FACILITY_REQ: "+Capi::describeParamInfo(info),"Capi::facility_req()"));
}


void
Capi::setListenFaxG3 (_cdword controller) throw (CapiMsgError,CapiError)
{
	usedCIPMask|=0x00020010;
	if (!controller) {
		unsigned char buf[64];
		for (int i=1;i<=Capi::numControllers;i++)
			if (profiles[i-1].fax || profiles[i-1].faxExt)
				listen_req(i, usedInfoMask, usedCIPMask); // can throw CapiMsgError
	}
	else {
		if (profiles[controller-1].fax || profiles[controller-1].faxExt)
			listen_req(controller, usedInfoMask, usedCIPMask); // can throw CapiMsgError
		else
			throw(CapiError("Chosen controller doesn't support fax services","Capi::setListenFaxG3"));
	}
}

void
Capi::setListenTelephony (_cdword controller) throw (CapiMsgError,CapiError)
{
	usedCIPMask|=0x00010012;
	if (!controller) {
		unsigned char buf[64];
		for (int i=1;i<=Capi::numControllers;i++)
			if (profiles[i-1].transp)
				listen_req(i, usedInfoMask, usedCIPMask); // can throw CapiMsgError
	}
	else {
		if (profiles[controller-1].transp)
			listen_req(controller, usedInfoMask, usedCIPMask); // can throw CapiMsgError
		else
			throw(CapiError("Chosen controller doesn't support voice (transparent) services","Capi::setListenTelephony"));
	}
}

void
Capi::connect_resp (_cword messageNumber, _cdword plci, _cword reject, _cword B1protocol, _cword B2protocol, _cword B3protocol, _cstruct B1configuration, _cstruct B2configuration, _cstruct B3configuration) throw (CapiMsgError)
{
	if (debug_level >= 2)
		debug << prefix() << ">CONNECT_RESP ApplId 0x" << hex << applId << ", msgNum 0x" << messageNumber << ", PLCI 0x" << plci << ", Reject 0x"
		 << reject << ", B1proto 0x" << B1protocol << ", B2proto 0x" << B2protocol << ", B3proto 0x" << B3protocol << endl;

	_cmsg new_message;
	unsigned info=CONNECT_RESP(
		&new_message, applId, messageNumber, plci, reject, B1protocol, 
		B2protocol, B3protocol, B1configuration, B2configuration, 
		B3configuration, 
#ifndef HAVE_CAPI_LIBRARY_V2
		NULL,
#endif
		NULL, NULL, NULL, NULL, NULL, NULL, NULL
		);
	if (debug_level >= 2)
		debug << prefix() << "info: " << info << endl;

	if (info != 0)
   		throw(CapiMsgError(info,"Error while CONNECT_RESP: "+Capi::describeParamInfo(info),"Capi::connect_resp()"));

}

void
Capi::connect_active_resp (_cword messageNumber, _cdword plci) throw (CapiMsgError)
{
	if (debug_level >= 2)
		debug << prefix() << ">CONNECT_ACTIVE_RESP ApplId 0x" << hex << applId << " MsgNum 0x" << messageNumber << " PLCI 0x" << plci << endl;

	_cmsg new_message;
	unsigned info=CONNECT_ACTIVE_RESP(&new_message, applId, messageNumber, plci);
	if (debug_level >= 2)
		debug << prefix() << "info: " << info << endl;

	if (info != 0)
   		throw(CapiMsgError(info,"Error while CONNECT_ACTIVE_RESP: "+Capi::describeParamInfo(info),"Capi::connect_active_resp()"));
}

void 
Capi::connect_b3_resp (_cword messageNumber, _cdword ncci, _cword reject, _cstruct ncpi) throw (CapiMsgError)
{
	if (debug_level >= 2)
		debug << prefix() << ">CONNECT_B3_RESP ApplId 0x" << hex << applId << " MsgNum 0x" << messageNumber << " NCCI 0x" << ncci << " Reject 0x" << reject << endl;

	_cmsg new_message;
	unsigned info=CONNECT_B3_RESP(&new_message, applId, messageNumber, ncci, reject, ncpi);
	if (debug_level >= 2)
		debug << prefix() << "info: " << info << endl;

	if (info != 0)
   		throw(CapiMsgError(info,"Error while CONNECT_B3_RESP: "+Capi::describeParamInfo(info),"Capi::connect_b3_resp()"));
}

void 
Capi::connect_b3_active_resp (_cword messageNumber, _cdword ncci) throw (CapiMsgError)
{
	if (debug_level >= 2)
		debug << prefix() << ">CONNECT_B3_ACTIVE_RESP ApplId 0x" << hex << applId << " MsgNum 0x" << messageNumber << " NCCI 0x" << ncci << endl;

	_cmsg new_message;
	unsigned info=CONNECT_B3_ACTIVE_RESP(&new_message, applId, messageNumber, ncci);
	if (debug_level >= 2)
		debug << prefix() << "info: " << info << endl;

	if (info != 0)
   		throw(CapiMsgError(info,"Error while CONNECT_B3_ACTIVE_RESP: "+Capi::describeParamInfo(info),"Capi::connect_b3_active_resp()"));
}


void 
Capi::data_b3_resp (_cword messageNumber, _cdword ncci, _cword dataHandle) throw (CapiMsgError)
{
	if (debug_level >= 3)
		debug << prefix() << ">DATA_B3_RESP, ApplId 0x" << hex << applId << ", msgNum 0x" << messageNumber << ", NCCI 0x" << ncci << ", DataHandle 0x" << dataHandle << endl;

	_cmsg new_message;
	unsigned info=DATA_B3_RESP(&new_message, applId, messageNumber, ncci, dataHandle);
	if (debug_level >= 3)
		debug << prefix() << "info: " << info << endl;

	if (info != 0)
		throw(CapiMsgError(info,"Error while DATA_B3_RESP: "+Capi::describeParamInfo(info),"Capi::data_b3_resp()"));
}

void
Capi::facility_resp (_cword messageNumber, _cdword address, _cword facilitySelector, _cstruct facilityResponseParameter) throw (CapiMsgError)
{
	if (debug_level >= 2)
		debug << prefix() << ">FACILITY_RESP ApplId 0x" << hex << applId << ", MsgNr 0x" << messageNumber << ", Address 0x" << address
		     << ", FacilitySelector 0x" << facilitySelector << endl;

	_cmsg new_message;
	unsigned info=FACILITY_RESP(&new_message, applId, messageNumber, address, facilitySelector, facilityResponseParameter);
	if (debug_level >= 2)
		debug << prefix() << "info: " << info << endl;

	if (info != 0)
		throw(CapiMsgError(info,"Error while FACILITY_RESP: "+Capi::describeParamInfo(info),"Capi::facility_resp()"));
}

void
Capi::info_resp (_cword messageNumber, _cdword address) throw (CapiMsgError)
{
	if (debug_level >= 2)
		debug << prefix() << ">INFO_RESP ApplId 0x" << hex << applId << ", MsgNr 0x" << messageNumber << ", Address 0x" << address << endl;

	_cmsg new_message;
	unsigned info=INFO_RESP(&new_message, applId, messageNumber, address);
	if (debug_level >= 2)
		debug << prefix() << "info: " << info << endl;

	if (info != 0)
		throw(CapiMsgError(info,"Error while INFO_RESP: "+Capi::describeParamInfo(info),"Capi::info_resp()"));
}

void
Capi::disconnect_b3_resp (_cword messageNumber, _cdword ncci) throw (CapiMsgError)
{
	if (debug_level >= 2)
		debug << prefix() << ">DISCONNECT_B3_RESP ApplId 0x" << hex << applId << " MsgNum 0x" << messageNumber << " NCCI 0x" << ncci << endl;

	_cmsg new_message;
	unsigned info=DISCONNECT_B3_RESP(&new_message, applId, messageNumber, ncci);
	if (debug_level >= 2)
		debug << prefix() << "info: " << info << endl;

	if (info != 0)
		throw(CapiMsgError(info,"Error while DISCONNECT_B3_RESP: "+Capi::describeParamInfo(info),"Capi::disconnect_b3_resp()"));
}


void
Capi::disconnect_resp (_cword messageNumber, _cdword plci) throw (CapiMsgError)
{
	if (debug_level >= 2)
		debug << prefix() << ">DISCONNECT_RESP ApplId 0x" << hex << applId << " MsgNum 0x" << messageNumber << " PLCI 0x" << plci << endl;

	_cmsg new_message;
	unsigned info=DISCONNECT_RESP(&new_message, applId, messageNumber, plci);
	if (debug_level >= 2)
		debug << prefix() << "info: " << info << endl;

	if (info != 0)
		throw(CapiMsgError(info,"Error while DISCONNECT_RESP: "+Capi::describeParamInfo(info),"Capi::disconnect_resp()"));
}



void
Capi::readMessage (void) throw (CapiMsgError, CapiError, CapiWrongState, CapiExternalError)
{
	_cmsg nachricht;
 	unsigned info=CAPI_GET_CMSG(&nachricht, applId);  // don't use capi20_get_message here as CAPI_GET_CMSG does disassembling of message parameters for us
	switch (info) {
		case CapiNoError:           //----- a message has been read -----
      			switch (nachricht.Subcommand) {
         			case CAPI_CONF:    // confirmation
					switch (nachricht.Command) {
						case CAPI_ALERT: {
							_cdword plci=ALERT_CONF_PLCI(&nachricht);
							if (debug_level >= 2)
								debug << prefix() << "<ALERT_CONF, PLCI: 0x" << hex << ALERT_CONF_PLCI(&nachricht) << ", Info 0x" << ALERT_CONF_INFO(&nachricht) << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in ALERT_CONF","Capi::readMessage()"));
							else
								connections[plci]->alert_conf(nachricht);
						} break;

						case CAPI_CONNECT: {
							_cdword pseudoPLCI=0xFACE0000 | nachricht.Messagenumber; // pseudo PLCI as set by connect_req
							_cdword plci=CONNECT_CONF_PLCI(&nachricht);
							if (debug_level >= 2)
								debug << prefix() << "<CONNECT_CONF, PLCI: 0x" << hex << CONNECT_CONF_PLCI(&nachricht) << ", Info 0x" << CONNECT_CONF_INFO(&nachricht) << endl;
							if (connections.count(pseudoPLCI)==0)
								throw(CapiError("MessageNumber unknown in CONNECT_CONF","Capi::readMessage()"));
							else {
								connections[plci]=connections[pseudoPLCI];
								connections.erase(pseudoPLCI);
								connections[plci]->connect_conf(nachricht);
							}
						} break;

						case CAPI_CONNECT_B3: {
							_cdword plci=CONNECT_B3_CONF_NCCI(&nachricht) & 0xFFFF; // PLCI is coded in the least 2 octets of NCCI
							if (debug_level >= 2)
								debug << prefix() << "<CONNECT_B3_CONF, NCCI: 0x" << hex << CONNECT_B3_CONF_NCCI(&nachricht) << ", Info 0x" << CONNECT_B3_CONF_INFO(&nachricht) << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in CONNECT_B3_CONF","Capi::readMessage()"));
							else
								connections[plci]->connect_b3_conf(nachricht);
						} break;

						case CAPI_SELECT_B_PROTOCOL: {
							_cdword plci=SELECT_B_PROTOCOL_CONF_PLCI(&nachricht);
							if (debug_level >= 2)
								debug << prefix() << "<SELECT_B_PROTOCOL_CONF, PLCI: 0x" << hex << SELECT_B_PROTOCOL_CONF_PLCI(&nachricht) << ", Info 0x" << SELECT_B_PROTOCOL_CONF_INFO(&nachricht) << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in SELECT_B_PROTOCOL_CONF","Capi::readMessage()"));
							else
								connections[plci]->select_b_protocol_conf(nachricht);
						} break;

						case CAPI_LISTEN:
							if (debug_level >= 2)
								debug << prefix() << "<LISTEN_CONF Controller 0x" << hex << LISTEN_CONF_CONTROLLER(&nachricht) << " Info 0x" << LISTEN_CONF_INFO(&nachricht) << endl;

							if (LISTEN_CONF_INFO(&nachricht)!=0)
								throw CapiMsgError(LISTEN_CONF_INFO(&nachricht),"LISTEN_REQ was unsuccesful "+Capi::describeParamInfo(LISTEN_CONF_INFO(&nachricht)),"Capi::readMessage()");

						break;

						case CAPI_DATA_B3: {
							_cdword plci=DATA_B3_CONF_NCCI(&nachricht) & 0xFFFF; // PLCI is coded in the least 2 octets of NCCI
							if (debug_level >= 3)
								debug << prefix() << "<DATA_B3_CONF, NCCI 0x" << hex << DATA_B3_CONF_NCCI(&nachricht) << dec << ", DataHandle " << DATA_B3_CONF_DATAHANDLE(&nachricht)
								      << ", Info 0x" << DATA_B3_CONF_INFO(&nachricht) << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in DATA_B3_CONF","Capi::readMessage()"));
							else
								connections[plci]->data_b3_conf(nachricht);
						} break;


						case CAPI_FACILITY:
							switch (FACILITY_CONF_FACILITYSELECTOR(&nachricht)) {
								case 1: { // DTMF
									_cdword plci=FACILITY_CONF_PLCI(&nachricht) & 0xFFFF; // this *should* be PLCI but who knows, so let's mask it to be sure
									if (debug_level >= 2)
										debug << prefix() << "<FACILITY_CONF PLCI 0x" << hex << FACILITY_CONF_PLCI(&nachricht) << " Info 0x" << FACILITY_CONF_INFO(&nachricht)
								                     << " FacilitySelector 0x" << FACILITY_CONF_FACILITYSELECTOR(&nachricht) << endl;

		     							if (connections.count(plci)==0)
										throw(CapiError("PLCI unknown in FACILITY_CONF","Capi::readMessage()"));
									else
										connections[plci]->facility_conf_DTMF(nachricht);
								} break;

								default:
									error << prefix() << "WARNING: PLCI " << hex << FACILITY_CONF_PLCI(&nachricht) << ": unsupported facility selector " << FACILITY_CONF_FACILITYSELECTOR(&nachricht) << " in FACILITY_CONF" << endl;
								break;
							}
						break;

						case CAPI_DISCONNECT_B3: {
							_cdword plci=DISCONNECT_B3_CONF_NCCI(&nachricht) & 0xFFFF; // PLCI is coded in the least 2 octets of NCCI
							if (debug_level >= 2)
								debug << prefix() << "<DISCONNECT_B3_CONF NCCI 0x" << hex << DISCONNECT_B3_CONF_NCCI(&nachricht) << " Info 0x" << DISCONNECT_B3_CONF_INFO(&nachricht)
							              << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in DISCONNECT_B3_CONF","Capi::readMessage()"));
							else
								connections[plci]->disconnect_b3_conf(nachricht);
						} break;

						case CAPI_DISCONNECT: { // TODO: perhaps we should handle NCPI telling us fax infos here??
							_cdword plci=DISCONNECT_CONF_PLCI(&nachricht);
							if (debug_level >= 2)
								debug << prefix() << "<DISCONNECT_CONF PLCI 0x" << hex << DISCONNECT_CONF_PLCI(&nachricht) << " Info 0x" << DISCONNECT_CONF_INFO(&nachricht)
						                     << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in DISCONNECT_CONF","Capi::readMessage()"));
							else
								connections[plci]->disconnect_conf(nachricht);
						} break;
					}
            			break;

				case CAPI_IND:     // indication
            				switch (nachricht.Command) {
						case CAPI_CONNECT: { // call for us
							_cdword plci=CONNECT_IND_PLCI(&nachricht);
							if (debug_level >= 2)
								debug << prefix() << "<CONNECT_IND PLCI 0x" << hex << plci << " CIP 0x" << CONNECT_IND_CIPVALUE(&nachricht) << endl;

							if (connections.count(plci)>0)
								throw(CapiError("PLCI used twice from CAPI in CONNECT_IND","Capi::readMessage()"));
							else {
								Connection *c=new Connection(nachricht,this,DDILength,DDIBaseLength,DDIStopNumbers);
								connections[plci]=c;
								if (!DDILength) // if we have PtP then wait until DDI is complete
									application->callWaiting(c);
							}
						} break;

						case CAPI_CONNECT_ACTIVE: {
							_cdword plci=CONNECT_IND_PLCI(&nachricht);
							if (debug_level >= 2)
								debug << prefix() << "<CONNECT_ACTIVE_IND PLCI 0x" << hex << plci << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in CONNECT_ACTIVE_IND","Capi::readMessage()"));
							else
								connections[plci]->connect_active_ind(nachricht);
						} break;

						case CAPI_CONNECT_B3: {
							_cdword plci=CONNECT_B3_IND_NCCI(&nachricht) & 0xFFFF; // PLCI is coded in the least 2 octets of NCCI
							if (debug_level >= 2)
								debug << prefix() << "<CONNECT_B3_IND NCCI 0x" << hex << CONNECT_B3_IND_NCCI(&nachricht) << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in CONNECT_B3_IND","Capi::readMessage()"));
							else
								connections[plci]->connect_b3_ind(nachricht);
						} break;

						case CAPI_CONNECT_B3_ACTIVE: {
							_cdword plci=CONNECT_B3_ACTIVE_IND_NCCI(&nachricht) & 0xFFFF; // PLCI is coded in the least 2 octets of NCCI
							if (debug_level >= 2)
								debug << prefix() << "<CONNECT_B3_ACTIVE_IND NCCI 0x" << hex << CONNECT_B3_ACTIVE_IND_NCCI(&nachricht) << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in CONNECT_B3_ACTIVE_IND","Capi::readMessage()"));
							else
								connections[plci]->connect_b3_active_ind(nachricht);
						} break;

						case CAPI_DISCONNECT: {  // call gone, we'll confirm to CAPI
							_cdword plci=DISCONNECT_IND_PLCI(&nachricht);
							if (debug_level >= 2)
								debug << prefix() << "<DISCONNECT_IND PLCI 0x" << hex << plci << " Reason 0x" << DISCONNECT_IND_REASON(&nachricht) << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in DISCONNECT_IND","Capi::readMessage()"));
							else {
								connections[plci]->disconnect_ind(nachricht);
							}
						} break;

						case CAPI_DISCONNECT_B3: {
							_cdword plci=DISCONNECT_B3_IND_NCCI(&nachricht) & 0xFFFF; // PLCI is coded in the least 2 octets of NCCI
							if (debug_level >= 2)
								debug << prefix() << "<DISCONNECT_B3_IND NCCI 0x" << hex << DISCONNECT_B3_IND_NCCI(&nachricht) << " Reason 0x" << DISCONNECT_B3_IND_REASON_B3(&nachricht) << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in DISCONNECT_B3_IND","Capi::readMessage()"));
							else
								connections[plci]->disconnect_b3_ind(nachricht);
						} break;

						case CAPI_DATA_B3: {
							_cdword plci=DATA_B3_IND_NCCI(&nachricht) & 0xFFFF; // PLCI is coded in the least 2 octets of NCCI
							if (debug_level >= 3)
								debug << prefix() << "<DATA_B3_IND: NCCI 0x" << hex << DATA_B3_IND_NCCI(&nachricht) << dec << ", DataLength " << DATA_B3_IND_DATALENGTH(&nachricht)
							      		<< hex << ", DataHandle 0x" << DATA_B3_IND_DATAHANDLE(&nachricht) << ", Flags 0x" << DATA_B3_IND_FLAGS(&nachricht) << endl;
							if (connections.count(plci)==0)
								throw(CapiError("PLCI unknown in DATA_B3_IND","Capi::readMessage()"));
							else
								connections[plci]->data_b3_ind(nachricht);
						} break;

						case CAPI_FACILITY:
							switch (FACILITY_IND_FACILITYSELECTOR(&nachricht)) {
								case 1: { // DTMF
									_cdword plci=FACILITY_IND_PLCI(&nachricht) & 0xFFFF; // we *should* get PLCI but just to be sure we mask the NCCI-part out...
									if (debug_level >= 2)
										debug << prefix() << "<FACILITY_IND: PLCI 0x" << hex << FACILITY_IND_PLCI(&nachricht) << ", FacilitySelector 0x" << FACILITY_IND_FACILITYSELECTOR(&nachricht) << endl;

		     							if (connections.count(plci)==0)
										throw(CapiError("PLCI unknown in FACILITY_IND","Capi::readMessage()"));
									else
										connections[plci]->facility_ind_DTMF(nachricht);
								} break;

								default:
									error << prefix() << "WARNING: PLCI " << hex << (FACILITY_IND_PLCI(&nachricht) & 0xFFFF) << ": Unsupported facility selector " << FACILITY_IND_FACILITYSELECTOR(&nachricht) << " in FACILITY_IND" << endl;
								break;
							}
						break;

						case CAPI_INFO:
							switch (INFO_IND_INFONUMBER(&nachricht)) {
								case 0x8001: { // ALERTING
									_cdword plci=INFO_IND_PLCI(&nachricht);
									if (debug_level >= 2)
										debug << prefix() << "<INFO_IND: PLCI 0x" << hex << plci << ", InfoNumber ALERTING " << endl;
		     							if (connections.count(plci)==0)
										throw(CapiError("PLCI unknown in INFO_IND","Capi::readMessage()"));
									else
										connections[plci]->info_ind_alerting(nachricht);
								} break;

								case 0x70: { // Called Party Number
									_cdword plci=INFO_IND_PLCI(&nachricht);
									if (debug_level >= 2)
										debug << prefix() << "<INFO_IND: PLCI 0x" << hex << plci << ", InfoNumber CalledPartyNr " << endl;
									
									bool nrComplete;
									if (connections.count(plci)==0)
										throw(CapiError("PLCI unknown in INFO_IND","Capi::readMessage()"));
									else {
										nrComplete=connections[plci]->info_ind_called_party_nr(nachricht);
										if (nrComplete && DDILength)
											application->callWaiting(connections[plci]);
									}
								} break;

								default:
									if (debug_level >= 2)
										debug << prefix() << "<INFO_IND: Controller/PLCI 0x" << hex << INFO_IND_PLCI(&nachricht) << ", InfoNumber " << INFO_IND_INFONUMBER(&nachricht) << " (ignoring)" << endl;
									info_resp(nachricht.Messagenumber,INFO_IND_PLCI(&nachricht));
								break;
							}
						break;


						default:
							stringstream err;
							err << "Indication 0x" << hex << static_cast<int>(nachricht.Command) << " not handled" << ends;
                  					throw (CapiError(err.str(),"Capi::readMessage()"));
						break;
      					}
   				break;

            			default:       //----- neither indication nor confirmation ???? -----
                			throw(CapiError("Unknown subcommand in function Handle_CAPI_Msg","Capi::readMessage()"));
				break;
            		}
		break;
        	case CapiReceiveQueueEmpty:
            		throw (CapiError("readMessage called but no message available?","Capi::readMessage()"));
		break;

        	default:
            		throw (CapiMsgError(info,"Error while CAPI_GET_CMSG: "+Capi::describeParamInfo(info),"Capi::readMessage()"));
		break;
   	}
}

void
Capi::run()
{       
	while (1) {
		pthread_testcancel();
		unsigned info=capi20_waitformessage (applId, NULL);   // will block until message is available or capi20_release called
		try {
			if (info==CapiNoError) {
				if (debug_level >= 3)
					debug << prefix() << "*" << endl;
				readMessage();  // trigger message reading
				if (debug_level >= 3)
					debug << prefix() << "**" << endl;
			}
		}
		catch (CapiMsgError e) {
		 	error << prefix() << "ERROR: Connection " << this << ": Error in readMessage(), message: " << e << endl;
		}
		catch (CapiError e) {
		 	error << prefix() << "ERROR: Connection " << this << ": Error in readMessage(), message: " << e << endl;
		}
	}
}

string
Capi::describeParamInfo (unsigned int info)
{
	switch (info)
 	{
		// class 0x00xx: Informative values, no error (see CAPI 2.0 4th ed. PartI, chapter 6.1.26)
		case 0x0000:
			return "request accepted.";
		case 0x0001:
			return "warning: NCPI not supported by current protocol, NCPI ignored.";
		case 0x0002:
			return "warnung: Flags not supported by current protocol, flags ignored.";
		case 0x0003:
			return "warnung: Alert already sent by another application.";
		// class 0x10xx: Error information concerning CAPI_REGISTER
		case 0x1001:
			return "Too many applications.";
		case 0x1002:
			return "Logical Block size too small; must be at least 128 bytes.";
		case 0x1003:
			return "Buffer exceeds 64 kbytes.";
		case 0x1004:
			return "Message buffer size too small, must be at least 1024 bytes.";
		case 0x1005:
			return "Max. number of logical connections not supported.";
		case 0x1006:
			return "reserved (unknown error).";
		case 0x1007:
			return "The message could not be accepted because of an internal busy condition.";
		case 0x1008:
			return "OS Resource error (out of memory?).";
		case 0x1009:
			return "CAPI not installed.";
		case 0x100A:
			return "Controller does not support external equipment.";
		case 0x100B:
			return "Controller does only support external equipment.";
		// class 0x11xx: Error information concerning message exchange functions
		case 0x1101:
			return "Illegal application number.";
		case 0x1102:
			return "Illegal command or subcommand, or message length less than 12 octets.";
		case 0x1103:
			return "The message could not be accepted because of a queue full condition.";
		case 0x1104:
			return "Queue is empty.";
		case 0x1105:
			return "Queue overflow: a message was lost!!";
		case 0x1106:
			return "Unknown notification parameter.";
		case 0x1107:
			return "The message could not be accepted because on an internal busy condition.";
		case 0x1108:
			return "OS resource error (out of memory?).";
		case 0x1109:
			return "CAPI not installed.";
		case 0x110A:
			return "Controller does not support external equipment.";
		case 0x110B:
			return "Controller does only support external equipment.";
		// class 0x20xx: Error information concerning resource/coding problems
		case 0x2001:
		   return "Message not supported in current state.";
		case 0x2002:
		   return "Illegal Controller/PLCI/NCCI.";
		case 0x2003:
		   return "No PLCI available.";
		case 0x2004:
		   return "No NCCI available.";
		case 0x2005:
		   return "No Listen resources available.";
		case 0x2006:
		   return "No fax resources available (protocol T.30 not supported).";
		case 0x2007:
		   return "Illegal message parameter coding.";
		case 0x2008:
		   return "No interconnection resources available.";
		// class 0x30xx: Error information concerning requested services
		case 0x3001:
		   return "B1 protocol not supported.";
		case 0x3002:
		   return "B2 protocol not supported.";
		case 0x3003:
		   return "B3 protocol not supported.";
		case 0x3004:
		   return "B1 protocol parameter not supported.";
		case 0x3005:
		   return "B2 protocol parameter not supported.";
		case 0x3006:
		   return "B3 protocol parameter not supported.";
		case 0x3007:
		   return "B protocol combination not supported.";
		case 0x3008:
		   return "NCPI not supported.";
		case 0x3009:
		   return "CIP Value unknown.";
		case 0x300A:
		   return "Flags nor supported (reserved bits used!).";
		case 0x300B:
		   return "Facility not supported.";
		case 0x300C:
		   return "Data length not supported by current protocol.";
		case 0x300D:
		   return "Reset procedure not supported by current protocol.";
		case 0x300E:
		   return "TEI assignment failed / overlapping channel masks.";
		case 0x300F:
		   return "Unsupported interoperability (see CAPI 2.0 specification, Part IV).";
		case 0x3010:
		   return "Request not allowed in this state.";
		case 0x3011:
		   return "Facility specific function not supported.";

		default:
			stringstream message;
			message << "CAPI returned an unknown error! Please ask your manufacturer for assistance (error code=0x" << hex << info << ")";
			return message.str();
	}
}
      
string
Capi::prefix()
{
	stringstream s;
	time_t t=time(NULL);
	char* ct=ctime(&t);
	ct[24]='\0';
	s << ct << " Capi " << hex << this << ": ";
	return (s.str());
}
    
void
Capi::readProfile() throw (CapiMsgError)
{
	unsigned char buf[64];
 	_cdword buf2[4]; 

	// is CAPI correctly installed?
	unsigned info=CAPI20_ISINSTALLED();
    	if (info!=0)
      		throw (CapiMsgError(info,"Error in CAPI20_ISINSTALLED: "+describeParamInfo(info),"Capi::getCapiInfo()"));

    	// retrieve number of installed controllers and create array for storing the descriptions
    	info=CAPI20_GET_PROFILE (0, buf);
    	if (info!=0)
      		throw (CapiMsgError(info,"Error in CAPI20_GET_PROFILE: "+describeParamInfo(info),"Capi::getCapiInfo()"));

	Capi::numControllers=buf[0]+(buf[1] << 8);

	// retrieve general information (kernel driver manufacturer, version of kernel driver)
	if (capi20_get_manufacturer(0,buf))
		capiManufacturer=reinterpret_cast<char *> (buf);
	else
		capiManufacturer="unknown";

	if (capi20_get_version(0, reinterpret_cast<unsigned char *>(buf2))) {
		stringstream tmp;
		tmp << buf2[0] << "." << buf2[1] << "/" << buf2[2] << "." << buf2[3] << ends;
		capiVersion=tmp.str();
    	} else
		capiVersion="unknown";

	// retrieve controller specific information (manufacturer, version)
	for (unsigned i=1;i<=Capi::numControllers;i++) {

		profiles.push_back(CardProfileT());
		
		if (capi20_get_manufacturer(i,buf))
			profiles[i-1].manufacturer=reinterpret_cast<char *> (buf);
		else
			profiles[i-1].manufacturer="unknown";
			
		info = CAPI20_GET_PROFILE(i, buf);
		if (info!=0)
			throw (CapiMsgError(info,"Error in CAPI20_GET_PROFILE/2: "+Capi::describeParamInfo(info),"Capi::getCapiInfo()"));

		profiles[i-1].bChannels=buf[2] + (buf[3]<<8);

		if (buf[4] & 0x08)
			profiles[i-1].dtmf=true;
		else
			profiles[i-1].dtmf=false;

		if (buf[4] & 0x10)
			profiles[i-1].suppServ=true;
		else
			profiles[i-1].suppServ=false;

		if (buf[8] & 0x02 && buf[12] & 0x02 && buf[16] & 0x01)
			profiles[i-1].transp=true;
		else
			profiles[i-1].transp=false;

		if (buf[8] & 0x10 && buf[12] & 0x10 && buf[16] & 0x10)
			profiles[i-1].fax=true;
		else
			profiles[i-1].fax=false;

		if (buf[8] & 0x10 && buf[12] & 0x10 && buf[16] & 0x20)
			profiles[i-1].faxExt=true;
		else
			profiles[i-1].faxExt=false;

		if (capi20_get_version(i,reinterpret_cast<unsigned char*>(buf2))) {
			stringstream tmp;
			tmp << buf2[0] << "." << buf2[1] << "/" << buf2[2] << "." << buf2[3];
			profiles[i-1].version=tmp.str();
		} else
			profiles[i-1].version="unknown";
	}
}

string
Capi::getInfo(bool verbose)
{
	stringstream tmp;
	tmp << Capi::numControllers << " controllers found" << endl;

	if (verbose)
	{
		tmp << "Capi driver: " << capiManufacturer << ", version " << capiVersion << endl;

		// retrieve controller specific information (manufacturer, version)
		for (unsigned i=1;i<=Capi::numControllers;i++) {
			tmp << "Controller " << i << ": " << profiles[i-1].manufacturer;

			tmp << " (" << profiles[i-1].bChannels << " B channels";

			if (profiles[i-1].dtmf)
				tmp << ", DTMF";
			if (profiles[i-1].suppServ)
				tmp << ", SuppServ";
			if (profiles[i-1].transp)
				tmp << ", transparent";
			if (profiles[i-1].fax)
				tmp << ", FaxG3";
			if (profiles[i-1].faxExt)
				tmp << ", FaxG3ext";

			tmp << "), driver version " << profiles[i-1].version << endl;
		}
	}
	return tmp.str();
}

/* History

Old Log (for new changes see ChangeLog):

Revision 1.5.2.4  2003/11/06 18:32:15  gernot
- implemented DDIStopNumbers

Revision 1.5.2.3  2003/11/02 14:58:16  gernot
- use DDI_base_length instead of DDI_base
- added DDI_stop_numbers option
- use DDI_* options in the Connection class
- call the Python script if number is complete

Revision 1.5.2.2  2003/11/01 22:59:33  gernot
- read CalledPartyNr InfoElements

Revision 1.5.2.1  2003/10/26 16:51:55  gernot
- begin implementation of DDI, get DDI Info Elements

Revision 1.5  2003/04/17 10:39:42  gernot
- support ALERTING notification (to know when it's ringing on the other side)
- cosmetical fixes in capi.cpp

Revision 1.4  2003/04/04 09:14:02  gernot
- setListenTelephony() and setListenFaxG3 now check if the given controller
  supports this service and throw an error otherwise

Revision 1.3  2003/04/03 21:16:03  gernot
- added new readProfile() which stores controller profiles in attributes
- getInfo() only creates the string out of the stored values and doesn't
  do the real inquiry any more
- getInfo() and numControllers aren't static any more

Revision 1.2  2003/02/21 23:21:44  gernot
- follow some a little bit stricter rules of gcc-2.95.3

Revision 1.1.1.1  2003/02/19 08:19:53  gernot
initial checkin of 0.4

Revision 1.28  2003/02/10 14:20:52  ghillie
merged from NATIVE_PTHREADS to HEAD

Revision 1.27.2.1  2003/02/09 15:05:36  ghillie
- rewritten to use native pthread_* calls instead of CommonC++ Thread

Revision 1.27  2003/01/19 16:50:27  ghillie
- removed severity in exceptions. No FATAL-automatic-exit any more.
  Removed many FATAL conditions, other ones are exiting now by themselves

Revision 1.26  2003/01/06 16:29:25  ghillie
- added debug output for constructor/destructor
- added call to terminate() in destructor
- missing break added in readMessage (oops... :-|)
- consider CapiReceiveQueueEmpty as error in readMessage()
- support finishing in run()
- only call readMessage() when waitformessage returned CapiNoError

Revision 1.25  2003/01/04 16:07:42  ghillie
- log improvements: log_level, timestamp

Revision 1.24  2002/12/18 14:40:23  ghillie
- removed this nasty listen_state. It had no sense than making problems.
- small change in getInfo()
- added check for abilities in setListen* if you say 0 for all controllers.
  Do nothing if this ability isn't provided for this controller

Revision 1.23  2002/12/13 09:57:10  ghillie
- error message formatting done by exception classes now

Revision 1.22  2002/12/09 15:32:58  ghillie
- debug stream now given in constructor
- debug output improvements
- exception cleanup (removed WARNING exceptions, output messages, ...)

Revision 1.21  2002/12/06 12:55:28  ghillie
- don't delete Connection objects any more

Revision 1.20  2002/12/05 15:00:06  ghillie
- new methods registerApplicationInterface(), unregisterConnection()
- use debug stream given in constructor, not from ApplicationInterface any more
- connect_req gets Connection* now, enters it with pseudoPLCI in connections map
- readMessage(): when receiving CONNECT_CONF: searches for pseudoPLCI in connections and moves this entry to real plci
- readMessage(): when receiving DISCONNECT_INF: don't erase Connection object from connections map, this will be done in Connection::~Connection

Revision 1.19  2002/12/02 16:53:51  ghillie
- minor debug output change

Revision 1.18  2002/11/29 11:11:12  ghillie
- moved communication thread from own class (CapiCommThread) to Capi class

Revision 1.17  2002/11/29 10:22:19  ghillie
- updated comments, use doxygen format now
- fixed small typo in setListen* methods

Revision 1.16  2002/11/27 15:58:52  ghillie
caught exceptions from disconnect_ind()

Revision 1.15  2002/11/25 20:57:21  ghillie
setListen* methods can now set listen state on all found controllers

Revision 1.14  2002/11/22 15:08:22  ghillie
- new method select_b_protocol_req()
- added SELECT_B_PROTOCOL_CONF case in readMessage()

Revision 1.13  2002/11/19 15:57:18  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.12  2002/11/18 14:24:09  ghillie
- moved global severity_t to CapiError::severity_t
- added throw() declarations

Revision 1.11  2002/11/17 14:38:34  ghillie
- improved exception throwing: using different exception types with severity parameter (FATAL,ERROR,WARNING) now
- changed getInfo() to use macros from /usr/include/capiutils.h now

Revision 1.10  2002/11/15 15:25:53  ghillie
added ALERT_REQ so we don't loose a call when we wait before connection establishment

Revision 1.9  2002/11/15 13:49:10  ghillie
fix: callmodule wasn't aborted when call was only connected/disconnected physically

Revision 1.8  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.7  2002/11/10 17:04:21  ghillie
improved some debug output

Revision 1.6  2002/11/08 07:57:07  ghillie
added functions to initiate a call
corrected FACILITY calls to use PLCI instead of NCCI in DTMF processing as told by Mr. Ortmann on comp.dcom.isdn.capi

Revision 1.5  2002/10/31 15:39:04  ghillie
added missing FACILITY_RESP message (oops...)

Revision 1.4  2002/10/31 12:37:34  ghillie
added DTMF support

Revision 1.3  2002/10/29 15:42:29  ghillie
typos...

Revision 1.2  2002/10/29 14:09:12  ghillie
fixed order in DISCONNECT_ROUTINE: first call callCompleted, then delete connection object!
cosmetic fixes

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.12  2002/10/09 11:18:59  gernot
cosmetic changes (again...) and changed info function of CAPI class

Revision 1.11  2002/10/05 20:43:32  gernot
quick'n'dirty, but WORKS

Revision 1.10  2002/10/05 13:53:00  gernot
changed to use thread class of CommonC++ instead of the threads-package
some cosmetic improvements (indentation...)

Revision 1.9  2002/10/04 15:56:34  gernot
reactivated error handling for Capi::getInfo()

Revision 1.8  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.7  2002/10/04 13:27:15  gernot
some restructuring to get it to a working state ;-)

does not do anything useful yet nor does it even compile...

Revision 1.6  2002/10/02 14:10:07  gernot
first version

Revision 1.5  2002/10/01 09:02:04  gernot
changes for compilation with gcc3.2

Revision 1.4  2002/09/22 14:22:53  gernot
some cosmetic comment improvements ;-)

Revision 1.3  2002/09/19 12:08:19  gernot
added magic CVS strings

* Sun Sep 15 2002 - gernot@hillier.de
- put under CVS, cvs changelog follows

* Sun May 19 2002 - gernot@hillier.de
- changed to not using QT libs any more

* Sun Apr 1 2002 - gernot@hillier.de
- first version

*/
