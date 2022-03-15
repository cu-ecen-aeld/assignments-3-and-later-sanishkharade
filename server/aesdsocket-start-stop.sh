#! /bin/sh

# Reference - Mastering Embedded Linux Programming Chapter 10, pg no 457

case "$1" in
	start)
		echo "Starting aesdsocket"
		start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
		;;
	stop)
		echo "Stopping aesdsocket"
		start-stop-daemon -K -n aesdsocket -s SIGTERM
		;;
	*)
		echo "Usage: $0 {start|stop}"
	exit 1
esac

exit 0