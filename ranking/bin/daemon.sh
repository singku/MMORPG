#!/bin/sh
echo "ser shell exec ..."
if [ "$1" = "stop" ] ; then
	ps -ef | grep "\<kranking\>" | awk '{print "kill " $2}'|sh
elif [ "$1" = "start" ]; then
	./kranking ../etc/bench.conf ./librank.so
elif [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "daemon.sh start|stop|restart"
fi
#ls ./ppseer ../etc/bench.conf ./libppser.so
