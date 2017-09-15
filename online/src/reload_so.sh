#!/bin/bash

if [ $# -eq 0 ] 
    then echo "need arg1"
    exit
fi

#for ((i =1; i <= $1; i++))
#do
    ./reload_so ./reload_so.conf dp_ol_sk $1 ../bin/libonline.so 
#echo $i
#sleep 3
#done
