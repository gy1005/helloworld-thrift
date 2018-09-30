#!/bin/bash

QPS=(1000 10000 20000 30000 40000 50000 60000 70000 75000 80000)

for qps in "${QPS[@]}"; do
  mkdir -p ../logs/qps_$qps
  sudo stap -v -x $(pgrep server) -g --suppress-time-limits ../stap_scripts/thrift-time.stp > ../logs/qps_$qps/stap.txt &
  sleep 10
  ~/Github/helloworld-thrift/build/client -q $qps -t 50 -d 30 > ../logs/qps_$qps/client.txt
  sudo kill -SIGINT $(pgrep stap)
done
