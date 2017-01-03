if [ $# != 6 ]
then
    echo "Wrong Arg! Usage: ./test-case -result [FOLDER_PATH] -run [RUN_NUM] -stress [true/false]"
    exit
fi

source ./prepare.sh

while [ -n "$1" ]
do
    case "$1" in
        -result) RESULTPATH="$2"
            shift;;
        
        -run) RUN="$2"
            shift;;
            
        -stress) STRESS="$2"
            shift;;
        
        --help) echo "Usage: ./test-case -result [FOLDER_PATH] -run [RUN_NUM] -stress [true/false]"
            exit;;
        
        *) echo "$1 is not an option"   
            exit;;
    esac
    shift
done

echo "Test Case: RESULTPATH = $RESULTPATH, RUN = $RUN, STRESS = $STRESS" | tee ./$RESULTPATH/readme

for((i=1; i<=$SIZE; i++))
do
    mkdir $RESULTPATH/${cli_num[$i]}

    for((j=1; j<=$RUN; j++))
    do  
        echo "result=$RESULTPATH stress=$STRESS with client_num ${cli_num[$i]} run $j"
        # a test
        ./test-kern.sh $RESULTPATH ${cli_num[$i]} $j $STRESS
    done
done
