#!/bin/bash
cd `dirname $0`
pkill -9 proxy
#rm -rf ../log/*
sleep 1

case "$1" in
    "c" | "C" | "clean" | "Clean" ) rm -rf log/* ;;
    * ) ;;
esac
./bin/proxy ./etc/bench.lua
