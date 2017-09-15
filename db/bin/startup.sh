#!/bin/bash
rm -rf ../log/*

cd $(dirname $0 ) 
./daemon.sh stop
./daemon.sh start 
