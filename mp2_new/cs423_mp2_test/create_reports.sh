#!/bin/bash

# Run the tests and generate a grade report for the tests in this mp

MODULE_NAME=$1
NETID=$2
TEST=$3
CURR_DIR=`pwd`

if [ -z "$NETID" ]; then
  echo "ERROR: No netid provided"
  exit
fi

MODCHECK=`lsmod | grep -c $MODULE_NAME`
if [ "$MODCHECK" -eq "0" ]; then
  insmod $MODULE_NAME
fi

# check if grade file already exists, unlikely
TEST_REPORT="${NETID}_test_report.txt"

# handle debug redirection
REPORT_DEBUG_FILE="report.debug"
exec 19>$REPORT_DEBUG_FILE
BASH_XTRACEFD=19

# compile source code for the test suite
set -x

# Run the fourth set of tests for a couple of applications to be scheduled
cd $CURR_DIR
cd test4/
./run_test.sh $MODULE_NAME
TEST4_FILE="test4_grade/test4.grade"
APP1_GRADE=`head -n 1 $TEST4_FILE`
APP1_DUMP1=`{ head -n 2 > /dev/null; head -n 1; } < $TEST4_FILE`
APP1_DUMP2=`{ head -n 3 > /dev/null; head -n 1; } < $TEST4_FILE`

APP2_GRADE=`{ head -n 4 > /dev/null; head -n 1; } < $TEST4_FILE`
APP2_DUMP1=`{ head -n 6 > /dev/null; head -n 1; } < $TEST4_FILE`
APP2_DUMP2=`{ head -n 7 > /dev/null; head -n 1; } < $TEST4_FILE`

APP3_GRADE=`{ head -n 8 > /dev/null; head -n 1; } < $TEST4_FILE`
APP3_DUMP1=`{ head -n 10 > /dev/null; head -n 1; } < $TEST4_FILE`
APP3_DUMP2=`{ head -n 11 > /dev/null; head -n 1; } < $TEST4_FILE`

cd $CURR_DIR
echo "## Schedule Three Tasks at Once" >> $TEST_REPORT
echo "" >> $TEST_REPORT
echo " * (a) Application 1 check: $APP1_GRADE" >> $TEST_REPORT
echo "   * Application parameters are 212 ms for computation and 1000 ms for period" >> $TEST_REPORT
echo "   * $APP1_DUMP1" >> $TEST_REPORT
echo "   * $APP1_DUMP2" >> $TEST_REPORT
echo " * (b) Application 2 check: $APP2_GRADE" >> $TEST_REPORT
echo "   * Application parameters are 354 ms for computation and 1500 ms for period" >> $TEST_REPORT
echo "   * $APP2_DUMP1" >> $TEST_REPORT
echo "   * $APP2_DUMP2" >> $TEST_REPORT
echo " * (c) Application 3 check: $APP3_GRADE" >> $TEST_REPORT
echo "   * Application parameters are 280 ms for computation and 1200 ms for period" >> $TEST_REPORT
echo "   * $APP3_DUMP1" >> $TEST_REPORT
echo "   * $APP3_DUMP2" >> $TEST_REPORT
echo "" >> $TEST_REPORT
