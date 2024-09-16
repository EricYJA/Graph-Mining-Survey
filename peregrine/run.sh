#!/bin/bash

# Load the TBB environment variables
source tbb2020/bin/tbbvars.sh intel64

# Start your program in the background and redirect its output to a log file
bin/fsm data/cite-flat-normalized/ 3 500 16 > >(tee -a pa-flat-fsm-500.log) &

# Get the PID of your program
PID=$!

# Monitor memory usage every second and append to the log file
while kill -0 $PID 2>/dev/null; do
    echo "$(date +'%Y-%m-%d %H:%M:%S') Memory Usage: $(ps -o rss= -p $PID) KB" | tee -a log.txt
    sleep 1
done

# Wait for the program to finish
wait $PID

