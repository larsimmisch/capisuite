"""
Exceptions hierarchy for capisuite

Exception
+ MissingConfigEntry
+ FaxError
  + JobError
    + InvalidJob
    + JobLockedError
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

from ConfigParser import NoOptionError, NoSectionError, Error

class NoGlobalSectionError(NoSectionError):
    """Raised when the GLOBAL section is missing
    in a configuration file."""

    def __init__(self):
        Error.__init__(self, "Invalid config file: section GLOBAL missing")
        self.section = 'GLOBAL'
