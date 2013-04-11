#!/bin/bash

if [ -z $1 ]; then
echo "Usage: $0 <topology.dat>"
exit
fi

./net_sim -C localhost:8888:8899 $1
