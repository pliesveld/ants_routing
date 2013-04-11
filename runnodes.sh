#!/bin/bash
rm rpcnode.*

killall rpcnode
PORT=8888; while [[ $PORT -lt 8899 ]] ; do ./rpcnode $PORT > node.$PORT & ((PORT++)) ; done 
