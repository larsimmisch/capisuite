"""
Exceptions hierarchy for capisuite

Exception
+ MissingConfigEntry
+ FaxError
  + JobError
    + InvalidJob
    + JobLockedError
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

class Error(Exception): pass
class FaxError(Exception): pass
class VoiceError(Exception): pass

class JobError(Error):
    # todo: distinguish fax/voice
    def __init__(self, jobnum, jobfile):
        Error.__init__(self)
        self.jobnum  = jobnum
        self.jobfile = jobfile

class JobLockedError(JobError): pass
class InvalidJob(JobError): pass

from ConfigParser import NoOptionError, NoSectionError
from ConfigParser import Error as ConfigError

class NoGlobalSectionError(NoSectionError):
    """Raised when the GLOBAL section is missing
    in a configuration file."""

    def __init__(self):
        NoSectionError.__init__(self,
                                "Invalid config file: section GLOBAL missing")
        self.section = 'GLOBAL'
