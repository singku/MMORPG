#!/bin/bash

ps aux | grep dplan_cache_agent | grep -v grep | awk '{print $2}' | xargs -I^ kill -9 ^

case "$1" in
    "c" | "C" | "clean" | "Clean" ) ./clean_log ; rm -rf bin/core.* rm -f ./core.* ;;
    * ) ;;
esac

./bin/dplan_cache_agent ../etc/bench.conf

