#!/usr/bin/bash

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
INPUT_DIR=$SCRIPT_DIR/input
OUTPUT_DIR=$SCRIPT_DIR/output
SOLVER_LOGS_DIR=$SCRIPT_DIR/solver_logs

if [ ! -d "$INPUT_DIR" ]; then
    echo "No input dir"
    exit
fi

if [ ! -d "$OUTPUT_DIR" ]; then
    mkdir $OUTPUT_DIR
fi

EXEC_FILE="$(cd ../ && echo $PWD)/SourceCode/bin/main"

if [ ! -f $EXEC_FILE ]; then
    echo "EXEC FILE DOESN'T EXISTS: $EXEC_FILE"
    exit 
fi

ALGOS=("APPROX" "SM" "MIP")

for infile in $INPUT_DIR/*.txt; do
    tmp=$(basename -- "$infile")
    test_name="${tmp%.*}"
    
    for algo in ${ALGOS[@]}; do
    solver_log="$SOLVER_LOGS_DIR/$test_name""_""$algo"".log"
    base="$OUTPUT_DIR/$test_name""_""$algo"
    resultfilename="$base""_res.txt"
    time_filename="$base""_time.txt"
    echo $resultfilename $time_filename
    { time $EXEC_FILE -f $infile -o $resultfilename -a $algo > $solver_log ;} 2> $time_filename 
    done

done
