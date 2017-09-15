#!/bin/bash

export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

case "$1" in
    "c" | "C" | "clean" | "Clean" ) rm -rf ../log/* ; rm -rf core.* core ;;
    * ) ;;
esac

cd ../bin
#for ((i =0; i <= $1; i++))
for i in {0..$1}
do
    ./daemon.sh stop
    sleep 1
    ./daemon.sh start
done
