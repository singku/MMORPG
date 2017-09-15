#!/bin/bash

ps aux | grep home_svr | grep -v grep | awk '{print $2}' | xargs -I^ kill -9 ^

case "$1" in
    "c" | "C" | "clean" | "Clean" ) rm -rf ../log/* ; rm -rf ./core.*;;
    * ) ;;
esac

cd ../bin && ./home_svr ../etc/bench.conf

