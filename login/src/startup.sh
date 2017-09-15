ps aux | grep  login | grep -v grep | awk '{print $2}' | xargs -I^ kill -9 ^
cd ../bin && ./login ../conf/bench.conf

