#!/bin/sh

CUR_PATH=`pwd`
echo $CUR_PATH

REDIS_PATH="/home/toby/dplan/servers/redis-2.8.13/src"
cd $REDIS_PATH

key_file="$CUR_PATH/redis_sets"
t_key_file="$CUR_PATH/t_redis_sets"

echo "KEYS *" | ./redis-cli > $key_file
cat $key_file | grep "rankings:3:\|rankings:2:0" > $t_key_file

cat $t_key_file | xargs ./redis-cli del
cat $t_key_file

echo "delete done"

