#! /bin/sh
#
# Script to start/stop capisuite daemon during boot/shutdown or from
# cardmgr via script from a /etc/pcmcia/<script>
#
#		Written by Miquel van Smoorenburg <miquels@cistron.nl>.
#		Modified for Debian GNU/Linux
#		by Ian Murdock <imurdock@gnu.ai.mit.edu>.
#		Modified for capisuite package
#		by Achim Bohnet <ach@mpe.mpg.de>
#
# Version:	@(#)skeleton  1.8  03-Mar-1998  miquels@cistron.nl
#
# This file was automatically customized by dh-make on Mon, 10 Mar 2003 21:37:06 +0100

PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/capisuite
NAME=capisuite
DESC="capisuite daemon"

test -f $DAEMON || exit 0

test -f /etc/default/capisuite || exit 0
run_capisuite_daemon=n
. /etc/default/capisuite
if [ "y" != "$run_capisuite_daemon" ]; then
  echo " 
/etc/init.d/capisuite: To use the fax and/or voice box services of capisuite
                       configure files in '/etc/capisuite/'. Then set
		       'run_capisuite_daemon' to 'y' in /etc/default/capisuite.
"
fi

set -e

case "$1" in
  start)
	echo -n "Starting $DESC: "
	start-stop-daemon --start --quiet --background --make-pidfile --pidfile /var/run/$NAME.pid \
		--exec $DAEMON
	echo "$NAME."
	;;
  stop)
	echo -n "Stopping $DESC: "
	start-stop-daemon --oknodo --stop --quiet --pidfile /var/run/$NAME.pid \
		--exec $DAEMON
	rm -f /var/run/$NAME.pid
	echo "$NAME."
	;;
  #reload)
	#
	#	If the daemon can reload its config files on the fly
	#	for example by sending it SIGHUP, do it here.
	#
	#	If the daemon responds to changes in its config file
	#	directly anyway, make this a do-nothing entry.
	#
	# echo "Reloading $DESC configuration files."
	# start-stop-daemon --stop --signal 1 --quiet --pidfile \
	#	/var/run/$NAME.pid --exec $DAEMON
  #;;
  restart|force-reload)
	#
	#	If the "reload" option is implemented, move the "force-reload"
	#	option to the "reload" entry above. If not, "force-reload" is
	#	just the same as "restart".
	#
	echo -n "Restarting $DESC: "
	start-stop-daemon --oknodo --stop --quiet --pidfile /var/run/$NAME.pid \
		--exec $DAEMON
	sleep 1
	start-stop-daemon --start --quiet --background --make-pidfile --pidfile /var/run/$NAME.pid \
		--exec $DAEMON
	echo "$NAME."
	;;
  *)
	N=/etc/init.d/$NAME
	# echo "Usage: $N {start|stop|restart|reload|force-reload}" >&2
	echo "Usage: $N {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0
