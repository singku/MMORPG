#!/bin/sh

uid=$1;
svrid=$2;

if [[ $uid == "" ]] 
    then exit 
fi

if [[ $svrid == "" ]] 
    then exit 
fi
echo "$uid, $svrid"

u_create_tm=`/home/svc/server/db/sql/get_create_tm.sh $uid $svrid`
echo $u_create_tm

if [[ $u_create_tm == "" ]]
    then exit
fi

role_key=`php /home/svc/rankrm/shit.php $uid $u_create_tm`
echo $role_key
echo "hello01"

echo "
SELECT $svrid
KEYS *" | redis-cli | grep rankings | 
while read row
do
    echo "select $svrid
    zrem $row $role_key" | redis-cli
done

#删缓存
echo "DEL dplan-player-cache-u:${uid}_$u_create_tm" | redis-cli
echo "DEL cache_player_head_item:${uid}_$u_create_tm" | redis-cli
