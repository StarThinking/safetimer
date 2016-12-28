#!/bin/bash

type=$1

rm ./client
rm ./server
mkdir result
echo $type > ./result/readme

if [ $type = "echo" ]
then
    echo "type is echo"
    killall server
    gcc server_echo.c -lpthread -o server
    gcc client_echo.c -o client
elif [ $type = "noecho" ]
then
    echo "type is noecho"
    killall server
    gcc server_noecho.c -lpthread -o server
    gcc client_noecho.c -o client
fi

~/hb-latency/bin/sync.sh
