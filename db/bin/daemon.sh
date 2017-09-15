#!/bin/sh
echo "dplan shell exec ..."
cd `dirname $0`
if [ "$1" = "stop" ] ; then
#	ps -ef |grep ./ser | awk '{print "kill -15 " $2}'|sh
	ps -ef |grep "\<dplan_db\>" | awk '{print "kill -9 " $2}'|sh
elif [ "$1" = "restart" ]; then
	killall -HUP ./dplan_db
	./dplan_db ./bench.lua ./libdb.so 
elif [ "$1" = "start" ]; then
#	 valgrind --leak-check=full ./dplan_db ../etc/bench.conf ./libtest.so -s 4096
	./dplan_db ../etc/bench.lua ./libdb.so 
elif [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
	echo "daemon.sh start|stop|restart"
fi
#ls ./dplan_db ./bench.lua ./libdb.so
