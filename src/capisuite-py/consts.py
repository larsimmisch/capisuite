"""capisuite.consts

Constant defintions for the capisuite Python package.

"""

__author__    = "Hartmut Goebel <h.goebel@crazy-compilers.com>"
__copyright__ = "Copyright (c) 2004 by Hartmut Goebel"
__version__   = "$Revision: 0.0 $"
__credits__   = "This file is part of www.capisuite.de; thanks to Gernot Hillier"
__license__ = """
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
"""

SEND_Q = 'sendq'
RECEIVED_Q = 'received'

__known_sections__ = ('GLOBAL',
                      'MailFaxSent',
                      'MailFaxFailed',
                      'MailFaxReceived',
                      'MailVoiceReceived')

# returncodes from call_voice and call_faxG3
CONNECTION_ESTABLISHED = 0
CONNECTION_TIMEOUT_EXCEEDED = 1
CONNECTION_FAILED_UNKNOWN_REASON = 2

"""
reject causes:
  1 = ignore call
  2 = normal call clearing
  3 = user busy
  7 = incompatible destination
  8 = destination out of order
"""

# For more detials about the error messaged please refere to 
# http://www.capisuite.de/manual/apb.html

error_messages = {
    0x0000: "Normal call clearing, no error",
    
    # Protocol errors
    0x3301: "Protocol error layer 1 (broken line or B-channel removed by signalling protocol)",
    0x3302: "Protocol error layer 2",
    0x3303: "Protocol error layer 3",
    0x3304: "Another application got that call",

    # these are T.30 (fax) errors
    0x3311: "Fax-Error: Remote station is not a G3 fax device",
    0x3312: "Fax-Error: Training error",
    0x3313: "Fax-Error: Remote station doesn't support transfer mode, e.g. wrong resolution",
    0x3314: "Fax-Error: Remote abort",
    0x3315: "Fax-Error: Remote procedure error",
    0x3316: "Fax-Error: Local transmit data underflow",
    0x3317: "Fax-Error: Local receive data overflow",
    0x3318: "Fax-Error: Local abort",
    0x3319: "Fax-Error: Illegal parameter coding (e.g. defective SFF file)",
    
    # ISDN error codes
    0x3400: "Normal termination, no reason available",
    0x3480: "Normal termination",
    0x3481: "Unallocated (unassigned) number",
    0x3482: "No route to specified transit network",
    0x3483: "No route to destination",
    0x3486: "Channel unacceptable",
    0x3487: "Call awarded and being delivered in an established channel",
    0x3490: "Normal call clearing",
    0x3491: "User busy",
    0x3492: "No user responding",
    0x3493: "No answer from user (user alerted)",
    0x3495: "Call rejected",
    0x3496: "Number changed",
    0x349A: "Non-selected user clearing",
    0x349B: "Destination out of order",
    0x349C: "Invalid number format",
    0x349D: "Facility rejected",
    0x349E: "Response to STATUS ENQUIRY",
    0x349F: "Normal, unspecified",
    0x34A2: "No circuit / channel available",
    0x34A6: "Network out of order",
    0x34A9: "Temporary failure",
    0x34AA: "Switching equipment congestion",
    0x34AB: "Access information discarded",
    0x34AC: "Requested circuit / channel not available",
    0x34AF: "Resources unavailable, unspecified",
    0x34B1: "Quality of service unavailable",
    0x34B2: "Requested facility not subscribed",
    0x34B9: "Bearer capability not authorized",
    0x34BA: "Bearer capability not presently available",
    0x34BF: "Service or option not available, unspecified",
    0x34C1: "Bearer capability not implemented",
    0x34C2: "Channel type not implemented",
    0x34C5: "Requested facility not implemented",
    0x34C6: "Only restricted digital information bearer capability is available",
    0x34CF: "Service or option not implemented, unspecified",
    0x34D1: "Invalid call reference value",
    0x34D2: "Identified channel does not exist",
    0x34D3: "A suspended call exists, but this call identity does not",
    0x34D4: "Call identity in use",
    0x34D5: "No call suspended",
    0x34D6: "Call having the requested call identity has been cleared",
    0x34D8: "Incompatible destination",
    0x34DB: "Invalid transit network selection",
    0x34DF: "Invalid message, unspecified",
    0x34E0: "Mandatory information element is missing",
    0x34E1: "Message type non-existent or not implemented",
    0x34E2: "Message not compatible with call state or message type non-existent or not implemented",
    0x34E3: "Information element non-existent or not implemented",
    0x34E4: "Invalid information element contents",
    0x34E5: "Message not compatible with call state",
    0x34E6: "Recovery on timer expiry",
    0x34EF: "Protocol error, unspecified",
    0x34FF: "Interworking, unspecified",
    }
