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
                                                            
