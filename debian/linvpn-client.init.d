#! /bin/sh

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/linvpn
NAME=linvpn
DESC=linvpn-client

test -x $DAEMON || exit 0

# Include linvpn-client defaults if available
if [ -f /etc/default/linvpn-client ] ; then
	. /etc/default/linvpn-client
else
    exit 0
fi

if [ "$START" != "1" -o ! "$VPNS" ]; then
    echo "linvpn-client is disabled in /etc/default/linvpn-client"
    exit 0
fi

set -e

case "$1" in
  start)
	echo -n "Starting $DESC: "
        for i in $VPNS; do
	    start-stop-daemon --start --quiet \
                --pidfile /var/run/${NAME}-${i}.pid \
                --exec $DAEMON -- -c $i $OPTS
        done
	echo "$NAME."
	;;
  stop)
	echo -n "Stopping $DESC: "
        for i in $VPNS; do
	    start-stop-daemon --stop --quiet \
                --pidfile /var/run/${NAME}-${i}.pid --exec $DAEMON
        done
	echo "$NAME."
	;;
  restart|force-reload)
	echo -n "Restarting $DESC: "
        for i in $VPNS; do
	    start-stop-daemon --stop --quiet \
                --pidfile /var/run/${NAME}-${i}.pid --exec $DAEMON
        done
        sleep 1
        for i in $VPNS; do
	    start-stop-daemon --start --quiet \
                --pidfile /var/run/${NAME}-${i}.pid \
                --exec $DAEMON -- -c $i $OPTS
        done
	echo "$NAME."
	;;
  *)
	N=/etc/init.d/$NAME
	echo "Usage: $N {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0
