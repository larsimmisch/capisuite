"""capisuite.consts

Constant defintions for the capisuite Python package.

"""

__author__    = "Hartmut Goebel <h.goebel@crazy-compilers.com>"
__copyright__ = "Copyright (c) 2004 by Hartmut Goebel"
__version__   = "$Revision: 0.0 $"
__credits__   = "This part of www.capisuite.de; thanks to Gernot Hiller"

SEND_Q = 'sendq'
RECEIVED_Q = 'received'

__known_sections__ = ('GLOBAL',
                      'Mail Fax Sent',
                      'Mail Fax Failed',
                      'Mail Fax Received',
                      'Mail Voice Received')

# capi return codes:
# 34D8 = Incompatible destination

# returncodes from call_voice and call_faxG3
CONNECTION_ESTABLISHED = 0
CONNECTION_TIMEOUT_EXCEEDED = 1
CONNECTION_FAILED_UNKNONW_REASON = 2

"""
reject causes:
  1 = ignore call
  2 = normal call clearing
  3 = user busy
  7 = incompatible destination
  8 = destination out of order
  0x34A9 = temporary failure
"""
                                                            
