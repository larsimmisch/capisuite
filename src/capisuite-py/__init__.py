"""capisuite

The main package of CapiSuite. This package provides the interface between
the user programmable Python scripts for handling incoming and outgoing calls
and the CapiSuite core which makes the ISDN hardware available.

This package consists of several files containing classes for certain aspects:

- core: provides the basic access to all functions of the CapiSuite core
- fax: contains all routines necessary for fax connection handling
- voice: contains all routines necessary for voice connection handling
- config: allows to read and write configuration and job description files
- fileutils: contains general file handling routines typically needed
  by CapiSuite scripts
- consts: some constant definitions
- exceptions: collection of exception classes used by the classes and functions
  in the other files

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
