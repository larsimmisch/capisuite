/** @file connection.h
    @brief Contains Connection - Encapsulates a CAPI connection with all its states and methods.

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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <capi20.h>
#include <string>
#include <fstream>
#include "capiexception.h"

class CallInterface;
class Capi;

using namespace std;

/** @brief Encapsulates a CAPI connection with all its states and methods.

    This class encapsulates one ISDN connection (physical and logical). It has two groups of methods:
    methods which are supposed to be called from the application (declared public) which serve to
    access the connection, send data, clear it etc. Also there are many message handlers implemented
    (declared as private) which are called by Capi::readMessage() and do the most message handling stuff.

    Only one logical connection per physical connection is supported.

    To communicate with the application by callback functions, the application must implement the CallInterface.
    The methods of this class are called if different events occur like call completion or a finished data transfer.

    There are two ways how Connection objects are created:
      - automatically for each incoming connection
      - manually to initiate a outgoing connection

   @author Gernot Hillier
*/
class Connection
{
	friend class Capi;

	public:
		/** @brief Type for describing the service of incoming and outgoing calls.

		    Several similar services are grouped together so the application doesn't have to distinct between speech calls
		    originating from the analog or digital network, between devices providing a High Layer compatibility and devices
		    which only provide bearer capabilities and so on.

		    For outgoing calls, the most specific value will be used.

		    For exact details concerning the different services see the parameter CIP in the CAPI spec.
		*/
		enum service_t {
			VOICE, ///< connections for speech services originated from the analog or digital network (CIP values 1,4,16)
			FAXG3,  ///< connections for fax Group2/3 services originating from the digital network (as analog networks don't provide service indicator) (CIP value 17)
			OTHER   ///< all other service types not included in the above values
		};

		/** @brief Constructor. Create an object to initiate an outgoing call

		    This constructor is used when the application creates a Connection object manually to initiate an outgoing connection.

		    It constructs the necessary information elements (B channel protocol settings, Number elements) and calls Capi::connect_req
		    to initiate a call.

		    @param capi pointer to the Capi Object
		    @param controller number of the controller which should initiate the call
		    @param call_from our number
		    @param clir set to TRUE to hide our number (so that the called party can't see it)
		    @param call_to the number which we want to call
		    @param service service to use (currently only VOICE and FAXG3 are supported), see service_t
		    @param faxStationID fax station ID, only used with service faxG3
		    @param faxHeadline fax headline (written on each fax page), only used with service faxG3
		    @throw CapiExternalError thrown when parameters are missing or wrong
		    @throw CapiMsgError thrown by Capi::connect_req, see there
		*/
		Connection (Capi* capi, _cdword controller, string call_from, bool clir, string call_to, service_t service, string faxStationID, string faxHeadline) throw (CapiExternalError, CapiMsgError);

		/** @brief. Destructor. Deletes the connection object.

		    Can block if file transmission is still in progress and/or if connection is not cleared already.
		    
		    Please call as soon as you don't need the object any more as this will also free some
		    CAPI resources associated to the call.

		    Please disconnect before deleting a Connection object! The auto-disconnect here is only supported
		    to prevent fatal errors and won't work very nicely!!
		*/
		~Connection();

		/** @brief Register the instance implementing the CallInterface

		    @param call_if - pointer to the instance to use
		*/
		void registerCallInterface(CallInterface *call_if);

		/** @brief Change the used B protcols (e.g. switch from speech to faxG3)

		    You have to disconnect the logical connection before calling this method. So to change from speech to fax do:
		    	- disconnect(LOGICAL_ONLY);
			- wait for CallInterface::CallDisconnectedLogical() to be called
			- changeProtocol(FAXG3,"stationID","headline")
			- wait for CallInterface::callConnected() to be called
			
		    Does nothing if the requested service is already active. Otherwise the necessary information elements are built
		    and Capi::select_b_protocol_req is called.

		    @param desired_service service to switch to
		    @param faxStationID Only needed when switching to FaxG3. The fax station ID to use.
		    @param faxHeadline Only needed when switching to FaxG3. The fax headline to use.
		    @throw CapiExternalError Thrown by Connection::buildBconfiguration. See there.
		    @throw CapiMsgError Thrown by Connection::select_b_protocol_req. See there.
		    @throw CapiWrongState Connection is in wrong state. Either it was finished by the partner or you didn't disconnect the logical connection before calling changeProtocol
		*/
		void changeProtocol (service_t desired_service, string faxStationID, string faxHeadline) throw (CapiMsgError, CapiExternalError, CapiWrongState);

		/** @brief called to start sending of a file
		    
		    The transmission will be controlled automatically by Connection - no further user interaction is necessary
		    if the whole file should be sent.

		    The file has to be in the correct format expected by CAPI, i.e. bit-reversed A-Law, 8 khz, mono (".la" for sox) for speech, SFF for faxG3

 		    @param filename the name of the file which should be sent
		    @throw CapiWrongState Thrown if Connection isn't up completely (physical & logical)
		    @throw CapiExternalError Thrown if file transmission is already in progress or the file couldn't be opened
		    @throw CapiMsgError Thrown by send_block(). See there.
		*/
		void start_file_transmission(string filename) throw (CapiWrongState, CapiExternalError, CapiMsgError);

		/** @brief called to stop sending of the current file, will block until file is really finished

		    If you stop the file transmission manually, CallInterface::transferCompleted won't be called.
		*/
		void stop_file_transmission();

		/** @brief called to activate receive mode

		    This method doen't do anything active, it will only set the receive mode for this connection
		    If you don't call this, all received data is discarded. If called, the data B3 stream received
		    is written to this file w/o changes. So it's in the native format given by CAPI (i.e. inserved A-Law
		    for speech, SFF for FaxG3).

 		    @param filename name of the file to which to save the incoming data
		    @throw CapiWrongState Thrown if Connection isn't up completely (physical & logical)
		    @throw CapiExternalError Thrown if file reception is already in progress or the file couldn't be opened
		*/
		void start_file_reception(string filename) throw (CapiWrongState, CapiExternalError);

		/** @brief called to stop receive mode

		    This closes the reception file and tells us to ignore further incoming B3 data.
		*/
		void stop_file_reception();

		/** @brief Tells disconnectCall() method how to disconnect.
		*/
		enum disconnect_mode_t {
			ALL, ///< do the necessary steps to finish the call nicely (first logical, then physical)
			PHYSICAL_ONLY, ///< quick'n'dirty disconnect (physical only, logical will auto-disconnect with protocol error)
			LOGICAL_ONLY ///< only disconnect logical connection, physical will stay up
		};

		/** @brief Terminate this connection

		    According to the current state and the given mode, the needed steps to disconnect will be executed.

		    If you don't specify a mode, the connection will be completely cleared (first logical, then physical).

		    It doesn't throw any error if you call it for an already cleared connection, so it's safe to call it more
		    than one time.

		    To reject a waiting call you haven't accepted earlier, use rejectWaiting(), not disconnectCall().

		    Attention: Connection objects will not be deleted after the disconnection. You must delete the Connection
		    object explicitly. The reason for this behavior is to allow you to get connection related information
		    (like disconnect cause, ...) after the connection has cleared. See also CallInterface::callDisconnectedPhysical().

		    @param disconnect_mode see description of disconnect_mode_t
		    @throw CapiMsgError Thrown by Capi::disconnect_b3_req() or Capi::disconnect_req(). See there.
		*/
		void disconnectCall(disconnect_mode_t disconnect_mode=ALL) throw (CapiMsgError);

		/** @brief Accept a waiting incoming call.

		    Will update the saved service to the desired_service. Calls Capi::connect_resp().

		    @param desired_service to determine with which service we should connect (e.g. VOICE or FAXG3)
		    @param faxStationID my fax station ID - only needed in FAXG3 mode
		    @param faxHeadline my fax headline - only needed in FAXG3 mode
		    @throw CapiWrongState Thrown if call is not in necessary state (waiting for acception or rejection)
		    @throw CapiExternalError Thrown by buildBconfiguration. See there.
		    @throw CapiMsgError Thrown by Capi::connect_resp(). See there.
		*/
		void connectWaiting(service_t desired_service, string faxStationID="", string faxHeadline="")  throw (CapiWrongState, CapiExternalError, CapiMsgError);

		/** @brief Reject a waiting incoming call

		    Only use for waiting calls - to disconnect an already connected call use disconnectCall().

		    @param reject reject cause, see CAPI spec ch 5.6 for details
		    @throw CapiWrongState Thrown if call is not in necessary state (waiting for acception or rejection)
		    @throw CapiExternalError Thrown if reject cause is not set (cause 0 not allowed).
		    @throw CapiMsgError Thrown by Capi::connect_resp(). See there.
		*/
		void rejectWaiting(_cword reject) throw (CapiWrongState, CapiMsgError, CapiExternalError);

		/** @brief disable timeout for a pending call (i.e. send ALERT)

		    Normally, a call is indicated from ISDN and timeouts after 4 (or 8) seconds if no answer is received.
		    If you want to accept the call, not immediately, but some secods later, you must call acceptWaiting().
		    This tells ISDN that we will accept it some times later (i.e. send ALERT_REQ)

		    @throw CapiWrongState Thrown if call is not in necessary state (waiting for acception or rejection)
		    @throw CapiMsgError Thrown by Capi::calert_req(). See there.
		*/
		void acceptWaiting() throw (CapiMsgError, CapiWrongState);

		/** @brief Enable indication for DTMF signals

		    Normally you get no indication when the CAPI recognizes a DTMF signal. With enableDTMF()
		    you enable these indications. DTMF signals will be saved locally and signalled by CallInterface::gotDTMF().
		    You can read the saved DTMF signales with getDTMF().

		    @throw CapiWrongState Thrown if Connection isn't up completely (physical & logical)
		    @throw CapiMsgError Thrown by Capi::facility_req(). See there.
		*/
		void enableDTMF() throw (CapiWrongState, CapiMsgError);

		/** @brief Disable indication for DTMF signals

		    @throw CapiWrongState Thrown if Connection isn't up completely (physical & logical)
		    @throw CapiMsgError Thrown by Capi::facility_req(). See there.
		*/
		void disableDTMF() throw (CapiWrongState, CapiMsgError);

		/** @brief read the saved DTMF characters

		    DTMF characters will only be saved if you called enableDTMF() before.

		    @return string containing received DTMF coded as characters ('0'..'9','A'..'D','*','#','X'=fax CNG,'Y'=fax CED)
		*/
		string getDTMF();

		/** @brief Delete the saved DTMF characters
		*/
		void clearDTMF();

		/** @brief Return number of the called party (the source of the call)

		    @return CalledPartyNumber
		*/
		string getCalledPartyNumber();

		/** @brief Return number of calling party (the destination of the call)

		    @return callingPartyNumber
		*/
		string getCallingPartyNumber();

		/** @brief Return currently used service mode

		    @return service as described in Connection::service_t
		*/
		service_t getService();

		/** @brief Return disconnection cause given by the CAPI

		    0x33xx=see CAPI spec
		    0x34xx=ISDN cause, for coding of xx see ETS 300102-1.

		    @return Returns the disconnection cause, 0=no cause available
		*/
		_cword getCause();

		/** @brief Return disconnection cause given by the CAPI

		    0x33xx=see CAPI spec
		    0x34xx=ISDN cause, for coding of xx see ETS 300102-1.

		    @return Returns the disconnection cause of Layer3, 0=no cause available or normal clearing
		*/
		_cword getCauseB3();


		/**  @brief Represents the current connection state in an easy way
		*/
		enum connection_state_t {
			DOWN, ///< this means the connection is completely down, i.e. physical and logical down
			WAITING, ///< this means an incoming connection is waiting for our decision (accept or reject)
			UP, ///< this means the connection is completely up, i.e. physical and logical up
			OTHER_STATE ///< connection is in some other state (e.g. physical up, logical down, ...)
		};

		/** @brief Return the connection state

		    @return current connection state as specified in Connection::connection_state_t
		*/
		connection_state_t getState();

		/** @brief Output error message

		    This is intended for external use if some other part of the application wants to make a error-log entry.
		    For internal use in this class just output to the stream "error".

		    @param message the message to print
		*/
		void errorMessage(string message);


		/** @brief Output debug message

		    This is intended for external use if some other part of the application wants to make a log entry.
		    For internal use in this class just output to the stream "debug".

		    @param message the message to print
		    @param level the debug level of the given message (see capisuite.conf for explanation)
		*/
		void debugMessage(string message, unsigned short level);

	protected:

		/** @brief Constructor. Create an object for an incoming call

		    This one is used by Capi when an incoming connection triggers the automatic creation of a Connection object, i.e.
		    when we receive a CONNECT_IND message.

		    It only extracts some data (numbers, services, etc.) from the message and saves it in private attributes.

		    @param message the received CONNECT_IND message
		    @param capi pointer to the Capi Object
		*/
		Connection (_cmsg& message, Capi *capi);

		/********************************************************************************/
    		/*	    methods handling CAPI messages - called by the Capi class		*/
		/********************************************************************************/

		/*************************** INDICATIONS ****************************************/

		/** @brief Called when we get CONNECT_ACTIVE_IND from CAPI

		    This method will also send a response to Capi and initiate
		    a B3 connection if necessary.

		    @param message the received CONNECT_ACTIVE_IND message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong plci_state)
		    @throw CapiMsgError Thrown by Capi::connect_b3_req
		*/
		void connect_active_ind(_cmsg& message) throw (CapiWrongState, CapiMsgError);

		/** @brief called when we get CONNECT_B3_IND from CAPI

		    This method will also send a response to Capi.

		    @param message the received CONNECT_B3_IND message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong ncci_state)
		    @throw CapiMsgError Thrown by Capi::connect_b3_resp()
		*/
		void connect_b3_ind(_cmsg& message) throw (CapiWrongState, CapiMsgError);

		/** @brief called when we get CONNECT_B3_ACTIVE_IND from CAPI

		    This method will also send a response to Capi and call CallInterface::callConnected().

		    @param message the received CONNECT_B3_ACTIVE_IND message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong ncci_state)
		    @throw CapiExternalError Thrown if no CallInterface is registered
		*/
		void connect_b3_active_ind(_cmsg& message) throw (CapiWrongState, CapiExternalError);

		/** @brief called when we get DATA_B3_IND from CAPI

		    This method will also save the received data, send a response to Capi and call CallInterface::dataIn().

		    @param message the received DATA_B3_IND message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong ncci_state)
		    @throw CapiMsgError Thrown by Capi::data_b3_resp()
		*/
		void data_b3_ind(_cmsg& message) throw (CapiWrongState,CapiMsgError);

		/** @brief called when we get FACILITY_IND from CAPI with facility selector saying it's DTMF

		    This method will save the received DTMF to received_dtmf, send a response to Capi and call 
		    CallInterface::gotDTMF().

		    @param message the received FACILITY_IND message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong ncci_state)
		*/
		void facility_ind_DTMF(_cmsg& message) throw (CapiWrongState);

		/** @brief called when we get DISCONNECT_B3_IND from CAPI

		    This method will also send a response to Capi and stop_file_transmission and stop_file_reception().
		    It will call CallInterface::callDisconnectedLogical() and initiate termination of physical connection.

		    @param message the received DISCONNECT_B3_IND message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong ncci_state)
		*/
		void disconnect_b3_ind(_cmsg& message) throw (CapiWrongState);

		/** @brief called when we get DISCONNECT_IND from CAPI

		    This method will also send a response to Capi and call CallInterface::callDisconnectedPhysical().

		    @param message the received DISCONNECT_IND message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong ncci_state or plci_state)
		    @throw CapiMsgError Thrown by Capi::disconnect_resp()
		*/
		void disconnect_ind(_cmsg& message) throw (CapiWrongState, CapiMsgError);

		/*************************** CONFIRMATIONS **************************************/

		/** @brief called when we get CONNECT_CONF from CAPI

		    @param message the received CONNECT_CONF message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong plci_state)
		    @throw CapiMsgError Thrown if the info InfoElement indicates an error
		*/
		void connect_conf(_cmsg& message) throw (CapiWrongState, CapiMsgError);

		/** @brief called when we get CONNECT_B3_CONF from CAPI

		    @param message the received CONNECT_B3_CONF message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong ncci_state)
		    @throw CapiMsgError Thrown if the info InfoElement indicates an error
		*/
		void connect_b3_conf(_cmsg& message) throw (CapiWrongState, CapiMsgError);

		/** @brief called when we get SELECT_B_PROTOCOL_CONF from CAPI

		    @param message the received SELECT_B_PROTOCOL_CONF message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong plci_state)
		    @throw CapiMsgError Thrown if the info InfoElement indicates an error
		*/
		void select_b_protocol_conf(_cmsg& message) throw (CapiWrongState, CapiMsgError);

		/** @brief called when we get ALERT_CONF from CAPI

		    @param message the received ALERT_CONF message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong plci_state)
		    @throw CapiMsgError Thrown if the info InfoElement indicates an error
		*/
		void alert_conf(_cmsg& message) throw (CapiWrongState, CapiMsgError);

		/** @brief called when we get DATA_B3_CONF from CAPI

		   This will trigger new send_block().

		    @param message the received DATA_B3_CONF message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong plci_state)
		    @throw CapiMsgError Thrown if the info InfoElement indicates an error
		    @throw CapiExternalError Thrown by Connection::send_block()
		*/
		void data_b3_conf(_cmsg& message) throw (CapiWrongState, CapiMsgError, CapiExternalError);

		/** @brief called when we get FACILITY_CONF from CAPI with facility selector saying it's DTMF

		    @param message the received FACILITY_CONF message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong plci_state)
		    @throw CapiMsgError Thrown if the info InfoElement indicates an error
		*/
		void facility_conf_DTMF(_cmsg& message) throw (CapiWrongState, CapiMsgError);

		/** @brief called when we get DISCONNECT_B3_CONF from CAPI

		    @param message the received DISCONNECT_B3_CONF message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong plci_state)
		    @throw CapiMsgError Thrown if the info InfoElement indicates an error
		*/
		void disconnect_b3_conf(_cmsg& message) throw (CapiWrongState, CapiMsgError);

		/** @brief called when we get DISCONNECT_CONF from CAPI

		    @param message the received DISCONNECT_CONF message
		    @throw CapiWrongState Thrown when the message is received unexpected (i.e. in a wrong plci_state)
		    @throw CapiMsgError Thrown if the info InfoElement indicates an error
		*/
		void disconnect_conf(_cmsg& message) throw (CapiWrongState, CapiMsgError);

		/********************************************************************************/
    		/*	                       internal methods					*/
		/********************************************************************************/

  		/** @brief return a prefix containing this pointer and date for log messages

      		    @return constructed prefix as stringstream
  		*/
		string prefix();

  		/** @brief format the CallingPartyNr or CalledPartyNr to readable string

		    @param capi_input the CallingPartyNumber resp. CalledPartyNumber struct
                    @param isCallingNr true = CallingPartyNr / false = CalledPartyNr
      		    @return the number formatted as string
  		*/
  		string getNumber (_cstruct capi_input, bool isCallingNr);

		/** @brief called to send next block (2048 bytes) of file

		    The transmission will be controlled automatically by Connection, so you don't
		    need to call this method directly. send_block() will automatically send as much
		    packets as the configured window size (conf_send_buffers) permits.

		    Will call CallInterface::transmissionComplete() if the file was transferred completely.

		    @throw CapiWrongState Thrown when the the connection is not up completely (physical & logical)
 		    @throw CapiExternalError Thrown when no CallInterface is registered
		    @throw CapiMsgError Thrown by Capi::data_b3_req().
		*/
		void send_block() throw (CapiWrongState, CapiExternalError, CapiMsgError);

		/** @brief called to build the B Configuration info elements out of given service
		
		    This is a convenience functin to do the quite annoying enconding stuff for the 6 B configuration parameters.

		    @param service value indicating service to be used as described in service_t
		    @param faxStationID my fax station ID
		    @param faxHeadline the fax headline
		    @param B1proto return value: B1protocol value for CAPI, see CAPI spec
		    @param B2proto return value: B2protocol value for CAPI, see CAPI spec
		    @param B3proto return value: B3protocol value for CAPI, see CAPI spec
		    @param B1config return value: B1configuration element for CAPI, see CAPI spec
		    @param B2config return value: B2configuration element for CAPI, see CAPI spec
		    @param B3config return value: B3configuration element for CAPI, see CAPI spec
		*/
		void buildBconfiguration(service_t service, string faxStationID, string faxHeadline, _cword& B1proto, _cword& B2proto, _cword& B3proto, _cstruct& B1config, _cstruct& B2config, _cstruct& B3config) throw (CapiExternalError);

		/********************************************************************************/
    		/*	                       attributes					*/
		/********************************************************************************/

		/** @brief State for the physical connection as defined in the CAPI spec.

		    For complete diagrams and state definition see CAPI 2.0 spec, chapter 7.2

		*/
		enum plci_state_t {
			P0,	///< default state. no connection.
			P01, 	///< CONNECT_REQ sent, waiting for CONNECT_CONF
			P1,	///< CONNECT_CONF received, waiting for CONNECT_ACTIVE_IND
			P2,	///< CONNECT_IND received, no CONNECT_RESP given yet
			P3,	///< only needed for handsets
			P4,	///< CONNECT_RESP sent, waiting for CONNECT_ACTIVE_IND
			P5,	///< DISCONNECT_REQ sent, waiting for DISCONNECT_IND
			P6,	///< DISCONNECT_IND received, DISCONNECT_RESP not sent yet
			PACT	///< active physical connection
		} plci_state;

		/** @brief State for logical connection as defined in the CAPI spec

		    For complete diagrams and state definition see CAPI 2.0 spec, chapter 7.2
		*/
		enum ncci_state_t {
			N0,	///< default state, no connection.
			N01,	///< CONNECT_B3_REQ sent, waiting for CONNECT_B3_CONF
			N1,	///< CONNECT_B3_IND received, CONNECT_B3_RESP not sent yet
			N2,	///< CONNECT_B3_CONF received / CONNECT_B3_RESP sent, waiting for CONNECT_B3_ACTIVE_IND
			N3,	///< RESET_B3_REQ sent, waiting for RESET_B3_IND
			N4,	///< DISCONNECT_B3_REQ sent, waiting for DISCONNECT_B3_IND
			N5,	///< DISCONNECT_B3_IND received
			NACT	///< active logical connection
		} ncci_state;

		_cdword plci;    ///< CAPI id for call
		_cdword ncci;    ///< id for logical connection

		service_t service; ///< as described in Connection::service_t, set to the last known service (either got from ISDN or set explicitly)

		_cword connect_ind_msg_nr; ///< this is needed as connect_resp is given in another function as connect_ind

		_cword disconnect_cause, ///< the disconnect cause as given by the CAPI in DISCONNECT_IND
			disconnect_cause_b3; ///< the disconnect cause as given by the CAPI in DISCONNECT_B3_IND

		string call_from; ///< CallingPartyNumber, formatted as string with leading '0' or '+' prefix
		string call_to;   ///< CalledPartyNumber, formatted as string

		string received_dtmf; ///< accumulates the received DTMF data, see readDTMF()

		bool keepPhysicalConnection, ///< set to true to disable auto-physical disconnect after logical disconnect for one time
			our_call; ///< set to true if we initiated the call (needed to know as some messages must be sent if we initiated the call)

		CallInterface *call_if; ///< pointer to the object implementing CallInterface
		Capi *capi; ///< pointer to the Capi object

		pthread_mutex_t send_mutex,  ///< to realize critical sections in transmission code
				receive_mutex; ///< to realize critical sections in reception code

		ofstream *file_for_reception; ///< NULL if no file is received, pointer to the file otherwise
		ifstream *file_to_send;  ///< NULL if no file is sent, pointer to the file otherwise
                                     
		ostream &debug, ///< debug stream
		        &error; ///< stream for error messages 
		unsigned short debug_level; ///< debug level

		/** @brief ring buffer for sending

		    7 buffers a 2048 byte mark the maximal window size handled by CAPI

		    is empty: buffer_used==0 / is full: buffers_used==7
		    to forget item: buffers_used--; buffer_start++;
		    to remember item: send_buffer[ (buffer_start+buffers_used)%8 ]=item; buffers_used++
		*/
		char send_buffer[7][2048];

		unsigned short buffer_start, ///< holds the index for the first buffer currently used
			buffers_used; ///< holds the number of currently used buffers
};

#endif

/*  History

$Log: connection.h,v $
Revision 1.1  2003/02/19 08:19:53  gernot
Initial revision

Revision 1.30  2003/02/10 14:20:52  ghillie
merged from NATIVE_PTHREADS to HEAD

Revision 1.29.2.1  2003/02/10 14:07:54  ghillie
- use pthread_mutex_* instead of CommonC++ Semaphore

Revision 1.29  2003/01/04 16:08:22  ghillie
- log improvements: log_level, timestamp
- added methods debugMessage(), errorMessage(), removed get*Stream()
- added some additional debug output for connection setup / finish

Revision 1.28  2002/12/16 13:13:47  ghillie
- added getCauseB3 to return B3 cause

Revision 1.27  2002/12/13 11:46:59  ghillie
- added attribute our_call to differ outgoing and incoming calls

Revision 1.26  2002/12/11 13:39:05  ghillie
- added support for PHYSICAL_ONLY disconnect in disconnectCall()

Revision 1.25  2002/12/10 15:06:15  ghillie
- new methods get*Stream() for use in capisuitemodule

Revision 1.24  2002/12/09 15:42:24  ghillie
- save debug and error stream in own attributes

Revision 1.23  2002/12/06 15:25:47  ghillie
- new return value for getState(): WAITING

Revision 1.22  2002/12/06 13:07:36  ghillie
- update docs because application is now responsible to delete
  Connection object
- new methods getCause() and getState()

Revision 1.21  2002/12/05 15:05:12  ghillie
- moved constructor for incoming calls to "private:"

Revision 1.20  2002/12/02 12:31:36  ghillie
renamed Connection::SPEECH to Connection::VOICE

Revision 1.19  2002/11/29 10:25:01  ghillie
- updated comments, use doxygen format now

Revision 1.18  2002/11/27 16:03:20  ghillie
updated comments for doxygen

Revision 1.17  2002/11/25 11:51:54  ghillie
- removed the unhandy CIP parameters from the interface to the application layer, use service type instead
- rejectWaiting() tests against cause!=0 now
- removed isUp() method

Revision 1.16  2002/11/22 15:13:44  ghillie
- new attribute keepPhysicalConnection which prevents disconnect_b3_ind() from sending disconnect_req()
- moved the ugly B*configuration, B*protocol settings from some methods to private method buildBconfiguration
- new methods changeProtocol(), select_b_protocol_conf(), clearDTMF()
- disconnect_b3_ind sets ncci_state to N0 before calling the callbacks
- added parameter disconnect_mode to disconnectCall()
- getDTMF() does non-destructive read now

Revision 1.15  2002/11/21 15:30:28  ghillie
- added new method Connection::acceptWaiting() - sends ALERT_REQ
- updated description of Connection::connectWaiting()

Revision 1.14  2002/11/19 15:57:19  ghillie
- Added missing throw() declarations
- phew. Added error handling. All exceptions are caught now.

Revision 1.13  2002/11/18 14:24:09  ghillie
- moved global severity_t to CapiError::severity_t
- added throw() declarations

Revision 1.12  2002/11/18 12:24:33  ghillie
- changed disconnectCall() so that it doesn't throw exceptions any more,
  so that we can call it in any state

Revision 1.11  2002/11/17 14:40:55  ghillie
added isUp()

Revision 1.10  2002/11/15 15:25:53  ghillie
added ALERT_REQ so we don't loose a call when we wait before connection establishment

Revision 1.9  2002/11/15 13:49:10  ghillie
fix: callmodule wasn't aborted when call was only connected/disconnected physically

Revision 1.8  2002/11/14 17:05:19  ghillie
major structural changes - much is easier, nicer and better prepared for the future now:
- added DisconnectLogical handler to CallInterface
- DTMF handling moved from CallControl to Connection
- new call module ConnectModule for establishing connection
- python script reduced from 2 functions to one (callWaiting, callConnected
  merged to callIncoming)
- call modules implement the CallInterface now, not CallControl any more
  => this freed CallControl from nearly all communication stuff

Revision 1.7  2002/11/13 08:34:54  ghillie
moved history to the bottom

Revision 1.6  2002/11/10 17:05:18  ghillie
changed to support multiple buffers -> deadlock in stop_file_transmission!!

Revision 1.5  2002/11/08 07:57:07  ghillie
added functions to initiate a call
corrected FACILITY calls to use PLCI instead of NCCI in DTMF processing as told by Mr. Ortmann on comp.dcom.isdn.capi

Revision 1.4  2002/10/31 12:40:06  ghillie
added DTMF support
small fixes like making some unnecessary global variables local, removed some unnecessary else cases

Revision 1.3  2002/10/30 14:29:25  ghillie
added getCIPvalue

Revision 1.2  2002/10/29 14:27:42  ghillie
added stop_file_*, added semaphores

Revision 1.1  2002/10/25 13:29:38  ghillie
grouped files into subdirectories

Revision 1.10  2002/10/10 12:45:40  gernot
added AudioReceive module, some small details changed

Revision 1.9  2002/10/09 11:18:59  gernot
cosmetic changes (again...) and changed info function of CAPI class

Revision 1.8  2002/10/04 15:48:03  gernot
structure changes completed & compiles now!

Revision 1.7  2002/10/04 13:27:15  gernot
some restructuring to get it to a working state ;-)

does not do anything useful yet nor does it even compile...

Revision 1.6  2002/10/01 09:02:04  gernot
changes for compilation with gcc3.2

Revision 1.5  2002/09/22 14:22:53  gernot
some cosmetic comment improvements ;-)

Revision 1.4  2002/09/19 12:08:19  gernot
added magic CVS strings

* Sun Sep 15 2002 - gernot@hillier.de
- put under CVS, cvs changelog follows above

* Sun May 20 2002 - gernot@hillier.de
- first version

*/
