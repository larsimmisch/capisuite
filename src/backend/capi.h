/** @file capi.h
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

#ifndef CAPI_H
#define CAPI_H

#include <capi20.h>
#include <string>
#include <map>  
#include <vector>
#include "capiexception.h"

class Connection;
class ApplicationInterface;

/** @brief Thread exec handler for Capi class

    This is a handler which will call this->run() for the use in pthread_create().
*/
void* capi_exec_handler(void* args);

/** @brief Main Class for communication with CAPI

    This class is the main encapsulation to use the CAPI ISDN interface.

    There are only a small subset of methods which are of use for the 
    application layer. These are for general purposes like enabling
    listening to calls and getting some nice formatted info strings.

    The biggest amount are shadow methods for nearly all CAPI messages,
    which do the dumb stuff like increasing message numbers, testing for 
    errors, building message structures and so on. Not each parameter of these
    is described in every detail here.  For more details please have a look in 
    the CAPI 2.0 specification, available from http://www.capi.org.

    There's also a big message handling routine (readMessage()) which calls 
    special handlers for incoming messages of the CAPI.

    A Capi object creates a new thread (with body run()) which waits for 
    incoming messages in an endless loop and hands them to readMessage().

    This class only does the general things - for handling single connections
    see Connection. Connection objects will be automatically created by this 
    class for incoming connections and can be created manually to initiate an 
    outgoing connection.

    The application is supposed to create one single object of this class.

    To communicate with the application via callback functions, the application
    must provide an implementation of the ApplicationInterface.
    The methods of this interface are called when some special events are received.

    @author Gernot Hillier
*/
class Capi {
	friend class Connection; 
	friend void* capi_exec_handler(void*);

	public:
		/** @brief Constructor. Registers our App at CAPI and start the communication thread.

		    @param debug reference to a ostream object where debug info should be written to
		    @param debug_level verbosity level for debug messages
		    @param error reference to a ostream object where errors should be written to
		    @param DDILength if ISDN interface is in PtP mode, the length of the DDI must be set here. 0 means disabled (PtMP)
		    @param DDIBaseLength the base number length w/o extension (and w/o 0) if DDI is used
		    @param DDIStopNumbers list of DDIs shorter than DDILength we will accept
		    @param maxLogicalConnection max. number of logical connections we will handle. 0 means autodetect.
        	    @param maxBDataBlocks max. number of unconfirmed B3-datablocks, 7  is the maximum supported by CAPI
	 	    @param maxBDataLen max. B3-Datablocksize, 2048 is the maximum supported by CAPI
		    @throw CapiError Thrown if no ISDN controller is reported by CAPI
		    @throw CapiMsgError Thrown if registration at CAPI wasn't successful.
		*/
		Capi (ostream &debug, unsigned short debug_level, ostream &error, 
		  unsigned short DDILength=0, unsigned short DDIBaseLength=0, 
		  vector<string> DDIStopNumbers=vector<string>(), 
		  unsigned maxLogicalConnection=0, unsigned maxBDataBlocks=7,
		  unsigned maxBDataLen=2048) throw (CapiError, CapiMsgError);

		/** @brief Destructor. Unregister App at CAPI

		    @throw CapiMsgError Thrown if deregistration at CAPI failed.
		*/
		~Capi();

		/** @brief Register the instance implementing the ApplicationInterface

		    @param application_in pointer to a class implementing (derived from) ApplicationInterface
		*/
		void registerApplicationInterface(ApplicationInterface* application_in);

		/** @brief Tell capi that we want to _additionally_ listen to Fax G3 calls

		    This method enables listening to fax calls from the analog network (coded as Bearer Capability
		    3.1kHz audio and from ISDN coded as fax group 3. The previously set listen mask is not cleared,
		    so that the application can call this method after another listen request w/o loosing the other
		    services.

		    It will also check if this controller is able to handle this service and throw an error otherwise.

		    @param Controller Nr. of Controller (0 = all available controllers)
		    @throw CapiMsgError Thrown by listen_req, see there for details
		    @throw CapiError Thrown if the given controller can't handle fax group 3
		*/
  		void setListenFaxG3 (_cdword Controller=0) throw (CapiMsgError,CapiError);

  		/** @brief Tell capi that we want to _additionally_ listen to Telephony calls

		    This method enables listening to speech calls from the analog network (coded as Bearer Capability
		    3.1kHz audio) and from ISDN (coded as Bearer Capability Speech or High Layer Compatibility telephony).
		    The previously set listen mask is not cleared, so that the application can call this method after
		    another listen request w/o loosing the other services.

		    It will also check if this controller is able to handle this service and throw an error otherwise.

		    @param Controller Nr. of Controller (0 = all available controllers)
		    @throw CapiMsgError Thrown by listen_req, see there for details
		    @throw CapiError Thrown if the given controller can't handle fax group 3
		*/
  		void setListenTelephony (_cdword Controller=0) throw (CapiMsgError,CapiError);

		/** @brief Static Returns some info about the installed Controllers

		     The returned string has the following format (UPPERCASE words replaced):

		     "NUM controllers found.
		     CAPI_MANUFACTURER, version VERSION_OF_CAPI/SUBVERSION_OF_CAPI
		     Controller X: CONTROLLER_MANUFACTURER (NUM B channels, SERVICE1, SERVICE2, ..., driver version DRIVER_VERSION/DRIVER_SUBVERSION
		     Controller X+1: CONTROLLER_MANUFACTURER (NUM B channels, SERVICE1, SERVICE2, ..., driver version DRIVER_VERSION/DRIVER_SUBVERSION
		     ..."

		     If verbose is set to false, only the first line is returned.

		     The following services are checked currently and printed as SERVICEX if found: DTMF, SuppServ, transparent, FaxG3, FaxG3ext

		     The method uses values stored by readProfile which must have been called before (constructor does it).

		     @param verbose controls verbosity of output (see above).
		     @return string containing details (see above).
		*/
	  	string getInfo(bool verbose=false);

	private:

		/** @brief erase Connection object in connections map

		    This method is used by Connection::~Connection()
		*/
		void unregisterConnection (_cdword plci);  

		/** @brief Get informations about CAPI driver and installed controllers

		     Fills the members profiles, capiVersion, capiManufacturer, numControllers
		     @throw CapiMsgError Thrown when requesting information fails
		*/
		static void readProfile() throw (CapiMsgError);


		/********************************************************************************/
    		/*	    methods to send CAPI messages - called by the Connection class	*/
		/********************************************************************************/

		/*************************** REQUESTS *******************************************/

		/** @brief Send LISTEN_REQ to CAPI

      	    	    @param Controller Nr. of Controller
	 	    @param InfoMask see CAPI 2.0 spec, ch 5.37, default = 0x03FF     -> all available info elements
		    @param CIPMask see CAPI 2.0 spec, ch 5.37, default = 0x1FFF03FF -> all available services
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
		*/
  		void listen_req (_cdword Controller, _cdword InfoMask=0x03FF, _cdword CIPMask=0x1FFF03FF) throw (CapiMsgError);

		/** @brief Send ALERT_REQ to CAPI

      	    	    @param plci reference to physical connection
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
		*/
  		void alert_req (_cdword plci) throw (CapiMsgError);

		/** @brief Send CONNECT_REQ to CAPI

		    To be able to see which CONNECT_CONF corresponds to this CONNECT_REQ, the Connection object
		    will be saved in the connections map under a pseudo PLCI (0xFACE & messageNumber). This will
		    be corrected at the moment the CONNECT_CONF is received.

      	    	    @param conn reference to the Connection object which calls connect_req()
		    @param Controller Nr. of controller to use for connection establishment
		    @param CIPvalue CIP (service indicator) to use for the connection
		    @param calledPartyNumber The number of the party which is called (i.e. the destination of the call)
		    @param callingPartyNumber The number of the party which calls (i.e. the source of the call)
		    @param B1protocol see CAPI spec for details
		    @param B2protocol see CAPI spec for details
		    @param B3protocol see CAPI spec for details
		    @param B1configuration see CAPI spec for details
		    @param B2configuration see CAPI spec for details
		    @param B3configuration see CAPI spec for details
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
		*/
  		void connect_req (Connection *conn, _cdword Controller, _cword CIPvalue, _cstruct calledPartyNumber, _cstruct callingPartyNumber, _cword B1protocol, _cword B2protocol, _cword B3protocol, _cstruct B1configuration, _cstruct B2configuration, _cstruct B3configuration) throw (CapiMsgError);

		/** @brief send SELECT_B_PROTOCOL_REQ to CAPI

      	    	    @param plci reference to physical connection
		    @param B1protocol see CAPI spec for details
		    @param B2protocol see CAPI spec for details
		    @param B3protocol see CAPI spec for details
		    @param B1configuration see CAPI spec for details
		    @param B2configuration see CAPI spec for details
		    @param B3configuration see CAPI spec for details
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
		*/
  		void select_b_protocol_req (_cdword plci, _cword B1protocol, _cword B2protocol, _cword B3protocol, _cstruct B1configuration, _cstruct B2configuration, _cstruct B3configuration) throw (CapiMsgError);

		/** @brief send CONNECT_B3_REQ to CAPI

      	    	    @param plci reference to physical connection
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
		*/
  		void connect_b3_req (_cdword plci) throw (CapiMsgError);

		/** @brief send DATA_B3_REQ to CAPI

	    	    @param ncci reference to physical connection
		    @param Data pointer to transmission data
		    @param DataLength length of transmission data
		    @param DataHandle some word value which will be referred to in DATA_B3_CONF (to see which data packet was sent successful)
		    @param Flags see CAPI 2.0 spec
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
		*/
  		void data_b3_req (_cdword ncci, void* Data, _cword DataLength,_cword DataHandle,_cword Flags) throw (CapiMsgError);

  		/** @brief send DISCONNECT_B3_REQ to CAPI

	      	    @param ncci reference to physical connection
		    @param ncpi protocol specific info
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
		*/
  		void disconnect_b3_req (_cdword ncci, _cstruct ncpi=NULL) throw (CapiMsgError);

  		/** @brief send DISCONNECT_REQ to CAPI

      	    	    @param plci reference to physical connection
		    @param Keypadfacility see CAPI spec
		    @param Useruserdata see CAPI spec
		    @param Facilitydataarray see CAPI spec
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
		*/
  		void disconnect_req (_cdword plci, _cstruct Keypadfacility=NULL, _cstruct Useruserdata=NULL, _cstruct Facilitydataarray=NULL) throw (CapiMsgError);

  		/** @brief send FACILITY_REQ to CAPI

      	    	    @param address Nr. of connection (Controller/PLCI/NCCI)
		    @param FacilitySelector see CAPI spec (1=DTMF)
		    @param FacilityRequestParameter see CAPI spec (too long to describe it here...)
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
		*/
  		void facility_req (_cdword address, _cword FacilitySelector, _cstruct FacilityRequestParameter) throw (CapiMsgError);

		/*************************** RESPONSES *******************************************/

  		/** @brief send CONNECT_RESP to CAPI

      	    	    @param messageNumber number of the referred INDICATION message
      	    	    @param plci reference to physical connection
		    @param reject tell CAPI if we want to accept (0) or reject (!=0, for details see CAPI spec) the incoming call
		    @param B1protocol see CAPI spec for details
		    @param B2protocol see CAPI spec for details
		    @param B3protocol see CAPI spec for details
		    @param B1configuration see CAPI spec for details
		    @param B2configuration see CAPI spec for details
		    @param B3configuration see CAPI spec for details
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
  		*/
  		void connect_resp (_cword messageNumber, _cdword plci, _cword reject, _cword B1protocol, _cword B2protocol, _cword B3protocol, _cstruct B1configuration, _cstruct B2configuration, _cstruct B3configuration) throw (CapiMsgError);

  		/** @brief send CONNECT_ACTIVE_RESP to CAPI

      	    	    @param messageNumber number of the referred INDICATION message
      	    	    @param plci reference to physical connection
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
	  	*/
	  	void connect_active_resp (_cword messageNumber, _cdword plci) throw (CapiMsgError);

	  	/** @brief send CONNECT_B3_RESP to CAPI

      	    	    @param messageNumber number of the referred INDICATION message
	      	    @param ncci reference to physical connection
		    @param reject tell CAPI if we want to accept (0) or reject (2) the incoming call
		    @param ncpi protocol specific info
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
  		*/
	   	void connect_b3_resp (_cword messageNumber, _cdword ncci, _cword reject, _cstruct ncpi) throw (CapiMsgError);

	  	/** @brief send CONNECT_B3_ACTIVE_RESP to CAPI

      	    	    @param messageNumber number of the referred INDICATION message
	      	    @param ncci reference to physical connection
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
  		*/
  		void connect_b3_active_resp (_cword messageNumber, _cdword ncci) throw (CapiMsgError);

  		/** @brief send DATA_B3_RESP to CAPI

      	    	    @param messageNumber number of the referred INDICATION message
	      	    @param ncci reference to physical connection
		    @param dataHandle Data Handle given by the referred DATA_B3_IND
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
  		*/
  		void data_b3_resp (_cword messageNumber, _cdword ncci, _cword dataHandle) throw (CapiMsgError);

  		/** @brief send FACILITY_RESP to CAPI

      	    	    @param messageNumber number of the referred INDICATION message
       	    	    @param address Nr. of connection (Controller/PLCI/NCCI)
		    @param facilitySelector see CAPI spec (1=DTMF)
		    @param facilityResponseParameter see CAPI spec
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
 		*/
  		void facility_resp (_cword messageNumber, _cdword address, _cword facilitySelector, _cstruct facilityResponseParameter=NULL) throw (CapiMsgError);

  		/** @brief send INFO_RESP to CAPI

      	    	    @param messageNumber number of the referred INDICATION message
       	    	    @param address Nr. of connection (Controller/PLCI)
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
 		*/
  		void info_resp (_cword messageNumber, _cdword address) throw (CapiMsgError);

  		/** @brief send DISCONNECT_RESP to CAPI

      	    	    @param messageNumber number of the referred INDICATION message
      	    	    @param plci reference to physical connection
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
  		*/
  		void disconnect_resp (_cword messageNumber, _cdword plci) throw (CapiMsgError);

	  	/** @brief send DISCONNECT_B3_RESP to CAPI

      	    	    @param messageNumber number of the referred INDICATION message
	      	    @param ncci reference to physical connection
		    @throw CapiMsgError Thrown when CAPI_PUT_MESSAGE returned an error.
   		*/
	  	void disconnect_b3_resp (_cword messageNumber, _cdword ncci) throw (CapiMsgError);

		/********************************************************************************/
    		/*   main message handling method for incoming msgs                             */
		/********************************************************************************/

		/** @brief read Message from CAPI and process it accordingly
		
		    This method handles all incoming messages. It is called by run() and will call
		    special handler methods of Connection mainly. Prints messages for debug purposes.

	      	    @throw CapiMsgError directly raised when CAPI_GET_MESSAGE or LISTEN_REQ fails, may also be raised by all called *_ind, *_conf handlers
	      	    @throw CapiError directly raised when general error occurs (unknown call references, unknown message, ... received)
	      	    @throw CapiWrongState may be raised by all called *_ind(), *_conf() handlers
	      	    @throw CapiExternalError may be raised by some called *_ind(), *_conf() handlers
		*/
	  	void readMessage (void) throw (CapiMsgError, CapiError, CapiWrongState, CapiExternalError);

		/********************************************************************************/
    		/*	    		methods for internal use				*/
		/********************************************************************************/

	  	/** @brief textual description for Parameter Info

		    This method returns an error string for the given info parameter. The strings were
		    taken out of the CAPI 2.0 spec

	    	    @param info errorcode as given by CAPI
	    	    @return textual description of error
  		*/
  		static string describeParamInfo (unsigned int info);

	  	/** @brief getApplId returns the application id we got from CAPI

	      	    @return Application ID from CAPI
	  	*/
	  	unsigned short getApplId(void) {return applId;}

		/** @brief Thread body - endless loop, will be blocked until message is received and then call readMessage()
    		*/
    		virtual void run(void);

  		/** @brief return a prefix containing this pointer and date for log messages

      		    @return constructed prefix as string
  		*/
		string prefix();

		/********************************************************************************/
		/*	    			attributes					*/
		/********************************************************************************/

		/** @brief type for storing controller profiles
		*/
		class CardProfileT
		{
			public:
			/** @brief default constructor
			*/
			CardProfileT()
			:manufacturer(""),version(""),bChannels(0),fax(false),faxExt(false),dtmf(false)
			{}

			string manufacturer; ///< manufacturer of controller
			string version; ///< version of controller driver
			int bChannels; ///< number of supported B channels
			bool transp; ///< does this controller support transparent protocols?
			bool fax; ///< does this controller support fax?
			bool faxExt; ///< does this controller support extended fax protocols (polling,...)?
			bool dtmf; ///< does this controller support DTMF recognition?
			bool suppServ; ///< does this controller support Supplementary Services?
		};

		static short numControllers;  ///< number of installed controllers, set by readProfile() method
                static string capiManufacturer, ///< manufacturer of the general CAPI driver
		       capiVersion; ///< version of the general CAPI driver

		unsigned short DDILength; ///< length of extension number (DDI) when ISDN PtP mode is used (0=PtMP)
		unsigned short DDIBaseLength; ///< base number length for the ISDN interface if PtP mode is used
		vector<string> DDIStopNumbers; ///< list of DDIs shorten than DDILength we'll accept
		
		static vector <CardProfileT> profiles; ///< vector containing profiles for all found cards (ATTENTION: starts with index 0,
						///< while CAPI numbers controllers starting by 1 (sigh)

		map <_cdword,Connection*> connections; ///< containing pointers to the currently active Connection
							///< objects, referenced by PLCI (or 0xFACE & messageNum when Connection is in plci_state Connection::P01

		_cword messageNumber;  ///< sequencial message number, must be increased for every sent message
		_cdword usedInfoMask;  ///< InfoMask currently used (in last listen_req)
		_cdword usedCIPMask;   ///< CIPMask currently used (in last listen_req)

		unsigned applId;  ///< containing the ID we got from CAPI after successful registration

		ApplicationInterface *application; ///< pointer to the application object implementing ApplicationInterface
		ostream &debug, ///< stream to write debug info to
			&error; ///< stream for error messages
		unsigned short debug_level; ///< debug level

		pthread_t thread_handle; ///< handle for the created message reading thread
};

#endif
