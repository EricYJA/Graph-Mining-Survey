#!/bin/bash

# Start your program in the background and redirect its output to a log file
bin/fsm data/my-graph-flat-1-1/ 3 20000 16 v > >(tee -a log.txt) &

# Get the PID of your program
PID=$!

# Monitor memory usage every second and append to the log file
while kill -0 $PID 2>/dev/null; do
    echo "$(date +'%Y-%m-%d %H:%M:%S') Memory Usage: $(ps -o rss= -p $PID) KB" | tee -a log.txt
    sleep 1
done

# Wait for the program to finish
wait $PID

