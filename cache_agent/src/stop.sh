#!/bin/bash

BIN_NAME="dplan_cache_agent"

pkill -SIGTERM $BIN_NAME ; sleep 1; pkill -9 $BIN_NAME
