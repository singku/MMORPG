#!/bin/sh
./stop.sh
sleep 2
mkdir -p dirty_log
rm -f core.*
rm -rf dirty_log/*
cp /home/svc/dirty.dat ./data/
./dirty_agent ./dirty_agent.conf
