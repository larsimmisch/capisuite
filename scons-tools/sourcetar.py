# -*- python -*-
"""
SCons builder for creating source tars.
"""

import SCons
from SCons.Node.FS import Base

def _e_dist(target, source, env):
                    
    def collect_sources(sources, collected, done, dbg=''):
        for s in sources:
            if s in done.keys(): continue
            done[s] = None
            if not s.is_derived(): # this node is not built (nor side-effect)
                #assert not s.all_children()
                if isinstance(s, Base) and s.is_under(env.fs.Dir('#')):
                    #print s
                    collected[s] = None
            else:
                collect_sources(s.all_children(scan=1), collected,done)
        return collected

    def cmp_path(a, b):
        return cmp(a.get_path(), b.get_path())

    collected = collect_sources(source, {}, {})
    collected = [ c.srcnode() for c in collected.keys() ]
    for c in collected:
        if not c.exists():
            print 'warning: file', c, 'is missing'
    collected = [c for c in collected if c.exists()]
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
    multi = 0,
    
    )

def generate(env):
    env['BUILDERS']['SourceTar'] = _builder
    env['BUILDERS']['DistTar'] = _builder_dist

def exists(env):
    import SCons.Tools.tar
    return SCons.Tools.tar.exists(env)
