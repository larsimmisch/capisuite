# -*- python -*-

# build top level files
# this is a SConscript, too, to support really build-dirs

Import('env')

cronfile = env.FileSubst('capisuite.cron', 'capisuite.cronin')
rcfile   = env.FileSubst('rc.capisuite', 'rc.capisuite.in')
env.AddPostAction([cronfile, rcfile], 'chmod gu+x $TARGET')

Alias('install',
      env.Install('$docdir', Split('COPYING NEWS README')),
      env.Install('$sysconfdir/init.d/capisuite', rcfile),
      env.InstallAs('$sysconfdir/cron.daily/capisuite', cronfile),
      env.InstallAs('$pkgsysconfdir/cronjob.conf','cronjob.conf'),
      )
