# -*- python -*-
"""
SCons builder for creating source tars.
"""

import SCons
from SCons.Node.FS import Base
from SCons.Node.Python import Value

class DistSourceOfFile(Value): pass

def _e_dist(target, source, env):
                    
    def collect_sources(sources, collected, done):
        for s in sources:
            if done.has_key(s): continue
            done[s] = None
            if not s.is_derived(): # this node is not built (nor side-effect)
                #assert not s.all_children()
                if isinstance(s, Base) and s.is_under(env.fs.Dir('#')):
                    if not s.srcnode().exists():
                        print 'warning: file', s.srcnode(), 'is missing'
                    else:
                        collected[s] = None
            else:
                collect_sources(s.all_children(scan=1), collected, done)

    def cmp_path(a, b):
        return cmp(a.get_path(), b.get_path())

    collected = {}; done = {}
    for s in source:
        if isinstance(s, DistSourceOfFile):
            collect_sources([s.value], collected, done)
        else:
            collected[s] = None
    collected = [ c.srcnode() for c in collected.keys() ]
    collected.sort(cmp_path)
    return (target, collected)

def DistTar(target, source, env):
    import tarfile
    tar = tarfile.open(str(target[0]), "w:gz")
    for name in source:
        name = str(name)
        tar.add(name, env.subst('${PACKAGE}-${VERSION}/') + name)
    tar.close()

def _str_DistTar(target, source, env):
    return env.subst('tar czf $TARGET $SOURCES')

_builder = SCons.Builder.Builder(
    action="$TARCOM",
    emitter = _e_dist,
    multi = 1,
    )

_builder_dist = SCons.Builder.Builder(
    action=SCons.Action.Action(DistTar,strfunction=_str_DistTar),
    emitter = _e_dist,
    multi = 1,
    )

def generate(env):
    env['BUILDERS']['SourceTar'] = _builder
    env['BUILDERS']['DistTar'] = _builder_dist
    env.DistSourceOfFile = DistSourceOfFile

def exists(env):
    import SCons.Tools.tar
    return SCons.Tools.tar.exists(env)
