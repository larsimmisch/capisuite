"""capisuite.delivery

This module implements routines for delivering received files and status
information to the user.
"""

__author__    = "Gernot Hillier <gernot@hillier.de>"
__copyright__ = "Copyright (c) 2005 by Gernot Hillier"
__version__   = "$Revision: 0.0 $"
__credits__   = "This file is part of www.capisuite.de"
__license__ = """
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
"""


class UserDelivery:
    """
    General base class for realizing the delivery of received messages
    (e.g. voice or fax files) and status information (e.g. fax job status
    reports) to the user. This class only defines the API. The actual
    implementation using e.g. mail delivery can be found in derived classes.
    """

    def __init__(self,config):
        """
        Constructor. Create an instance of the UserDelivery class. 'config'
        contains the configuration file object where the necessary
        settings for setting up the delivery can be found.
        """
        self.config=config
        pass

    def sendNotice(self,recipient,subject,information):
        """
        Send the 'information' text (multi-line string) to the given
        'recipient'.
        """
        pass

    # Hartmut: We should define and document all allowed convertions.
    # Otherwise this will become hell.
    def sendFile(self,recipient,subject,information,file,format):
        """
        Send the 'file' together with the given 'information' (multi-line
        string) to 'recipient'. If necessary, the file is converted to the
        given 'format' (e.g. PDF or WAV) before.
        """
        pass

class MailDelivery(UserDelivery):
    """
    Default implementation of a UserDelivery which delivers all messages via
    mail.
    """
    pass
