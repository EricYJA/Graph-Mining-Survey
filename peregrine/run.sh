#!/bin/bash

# Load the TBB environment variables
source tbb2020/bin/tbbvars.sh intel64

# log file name
LOG_FILE=bin-pa-flat-fsm-600.log

# Start your program in the background and redirect its output to a log file
bin/fsm data/cite-flat-normalized 3 600 16 > >(tee -a $LOG_FILE) 2>&1 &

# Get the PID of your program
PID=$!

# Monitor memory usage every second and append to the log file
while kill -0 $PID 2>/dev/null; do
    echo "$(date +'%Y-%m-%d %H:%M:%S') Memory Usage: $(ps -o rss= -p $PID) KB" | tee -a $LOG_FILE
    sleep 3
done

# Wait for the program to finish
wait $PID

