#!/bin/bash

MODULE_NAME=$1
MP2_PROCFS_FILE="/proc/mp2/status"
MP2_GRADE_DIR="test4_grade"
REGISTRATION_OUT="$MP2_GRADE_DIR/registration_out.txt"

MP2_DEBUG_FILE="$MP2_GRADE_DIR/run_test.debug"
MP2_LOG_FILE="$MP2_GRADE_DIR/run_test.log"

# create grading directory
mkdir $MP2_GRADE_DIR

# sanity check
if [ -z "$MODULE_NAME" ]; then
  echo "Module name not provided" >> $MP2_LOG_FILE;
  exit;
fi

# first check that the module is inserted
MODCHECK=`lsmod | grep -c $MODULE_NAME`
if [ "$MODCHECK" -eq "0" ]; then
  echo "MP2 module is not inserted. Grading script exiting.";
  exit;
fi

# handle debug redirection
exec 19>$MP2_DEBUG_FILE
BASH_XTRACEFD=19

## design a set of tasks that will stress the scheduler

# First set: tasks with computation time > period. Should fail
set -x
./userapp -c 176 -p 1200 -n 330 -j 10 > $MP2_GRADE_DIR/app1.out 2> $MP2_GRADE_DIR/app1.err &
PID1=$!
./userapp -c 450 -p 1400 -n 490 -j 10 > $MP2_GRADE_DIR/app2.out 2> $MP2_GRADE_DIR/app2.err &
PID2=$!
./userapp -c 235 -p 1100 -n 360 -j 10 > $MP2_GRADE_DIR/app3.out 2> $MP2_GRADE_DIR/app3.err &
PID3=$!

# rename the appropriate gold files
cp gold1.txt gold_$PID1.txt
cp gold2.txt gold_$PID2.txt
cp gold3.txt gold_$PID3.txt

set +x 

while [ "$(ps -p $PID1 $PID2 $PID3 | grep -c $PID2)" != "0" ]
do
  ps -u -p $PID1 $PID2 $PID3 >> $MP2_LOG_FILE
  sleep 0.5s
done

# wait for the processes to complete and leave
wait

# remove extra files
rm gold_*.txt

mv test4.grade $MP2_GRADE_DIR/

echo "Goodbye from grading script!" >> $MP2_LOG_FILE
