"""capisuite.storage

This module implements routines for file storage and queue management.
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


class ItemStorage:
    """
    General base class for realizing the item storage. This class defines the
    API for the storage of received messages, given jobs and static files (like
    user announcements). One non-static item is always in one of the four
    places/states: sendq (job to send), done (successfully sent job), failed
    (finally failed job), received (received message). This class only defines
    the API, the actual implementation is done in derived classes which could
    e.g. store the files on disk or in a database (latter not implemented yet).
    """

    def __init__(self,config):
        """
        Constructor. Create an instance of the ItemStorage class and do
        inital checks like correctness of permissions. 'config' contains the
        configuration file object where the necessary settings for
        setting up the storage can be found.
        """
        self.config=config
        pass

    def setDone(self,id):
        """
        Move an item from the sendq to the done state/dir.
        """
        pass

    def setFailed(self,id):
        """
        Move an item from the sendq to the finally failed state/dir.
        """
        pass

    # Hartmut: What about returning an open filehandle? This would allow 
    # passing file-like-objects if some day the coure will be implemented
    # in Python.
    def getItem(self,queue,id):
        """
        Get entry with name 'id' from the queue 'queue'. Must return a file name
	in the local filesystem because the core can only handle files. It should
        also check file permissions if necessary.
        """
        pass
    
    def addItem(self,queue,id,description):
        """
        Add an item to a certain queue with the given description.
        """
        pass

    # Hartmut: Is this really needed? Do we need 'stale' IDs?
    def getNewItemId(self,queue):
        """
        Return a new uniqe name for an item to be stored in the 'queue'.
        """
        pass

    def getStaticItem(self,name):
        """
        Return the full path of a static item like a user announcement.
        """
        pass


class FileStorage(ItemStorage):
    """
    Default implementation of an ItemStorage which stores all
    messages as files on the local filesystem.
    """
    pass
