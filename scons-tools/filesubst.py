# -*- python -*-
"""
File-Content Substitution builder for SCons
"""

__author__ = "Hartmut Goebel <h.goebel@crazy-compilers.com>"

import os, re
import SCons

def _action(target, source, env):

    def _substitute(matchobj, env=env):
        sym = matchobj.group(1)
        try:
            return env.subst(str(env[sym]))
        except: # TypeError: # sym not a string
            txt = matchobj.group(0) # the string matched
            print 'Not substituting', txt
            return txt

    delim = re.escape(env['FILESUBSTDELIM'])
    # compile a non-greedy pattern
    subst_pattern = re.compile('%s(.*?)%s' % (delim, delim))
    for t, s in zip(target, source):
        t = str(t)
        s = s.rstr()
        text = open(s, 'rb').read()
        text = subst_pattern.sub(_substitute, text)
        open(t, 'wb').write(text)
        os.chmod(t, os.stat(s)[0])
    return None

def _strfunc(target, source, env):
    return "generating '%s' from '%s'" % (target[0], source[0])

_builder = SCons.Builder.Builder(
    action = SCons.Action.Action(_action, _strfunc),
    src_suffix = '.in',
    )

def generate(env):
    env['BUILDERS']['FileSubst'] = _builder
    env['FILESUBSTDELIM'] = '@'

def exists(env):
    return 1
