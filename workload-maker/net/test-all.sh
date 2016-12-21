#!/bin/bash
SIZE=11
client_num[1]=0
client_num[2]=20
client_num[3]=40
client_num[4]=60
client_num[5]=80
client_num[6]=100
client_num[7]=120
client_num[8]=140
client_num[9]=160
client_num[10]=180
client_num[11]=200

RUN=3

for((i=1; i<=$SIZE; i++))
do
    mkdir ./result/${client_num[$i]}

    for((j=1; j<=$RUN; j++))
    do
        echo "test of client_num ${client_num[$i]} run $j"
        # a test
        ./test-user.sh ${client_num[$i]} $j
    done
done