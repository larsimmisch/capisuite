# -*- python -*-

# build top level files
# this is a SConscript, too, to support really build-dirs

Import('env')

cronfile = env.FileSubst('capisuite.cron', 'capisuite.cronin')
rcfile   = env.FileSubst('rc.capisuite', 'rc.capisuite.in')
env.AddPostAction([cronfile, rcfile], Chmod('$TARGETS', 0755))

env.Alias('install', [
    env.Install('$docdir', Split('COPYING NEWS README AUTHORS')),
    #env.InstallAs('$sysconfdir/init.d/capisuite', rcfile),
    #env.InstallAs('$sysconfdir/cron.daily/capisuite', cronfile),
    #env.InstallAs('$pkgsysconfdir/cronjob.conf','cronjob.conf'),
    ])

# Since these are not installed, we need to list them for
# distribuition explicitly
env.ExtraDist([cronfile, rcfile, 'cronjob.conf'])
