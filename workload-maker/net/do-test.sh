#/bin/bash 
nohup sh -c "./test-case.sh -result result.stress.8 -run 500 -stress true -datasize 8" &
nohup sh -c "./test-case.sh -result result.nostress.8 -run 500 -stress false -datasize 8" &
nohup sh -c "./test-case.sh -result result.stress.128 -run 500 -stress true -datasize 128" &
nohup sh -c "./test-case.sh -result result.nostress.128 -run 500 -stress false -datasize 128" &
