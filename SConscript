# -*- python -*-

# build top level files
# this is a SConscript, too, to support really build-dirs

Import('env')

cronfile = env.FileSubst('capisuite.cron', 'capisuite.cronin')
rcfile   = env.FileSubst('rc.capisuite', 'rc.capisuite.in')
env.AddPostAction([cronfile, rcfile], Chmod('$TARGETS', 0755))

env.Alias('install', [
    env.Install('$docdir', Split('COPYING NEWS README AUTHORS')),
    ])

# Since these are not installed, we need to list explicitly
# them for distribuition
env.DistSourcesOf([cronfile, rcfile])
env.ExtraDist('cronjob.conf')
