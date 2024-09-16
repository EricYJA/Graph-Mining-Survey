#!/bin/bash

# Load the TBB environment variables
source tbb2020/bin/tbbvars.sh intel64

# Loop through each directory under the graph directory
for dir in data/*; do
    # Check if it's a directory
    if [ -d "$dir" ]; then
        # Extract the directory name without the path for the log file
        dir_name=$(basename "$dir")

        # Start your program in the background and redirect its output to a log file
        bin/fsm "$dir" 3 20 48 v > >(tee -a "log_$dir_name.txt") &

        # Get the PID of your program
        PID=$!

        # Monitor memory usage every second and append to the log file
        while kill -0 $PID 2>/dev/null; do
            echo "$(date +'%Y-%m-%d %H:%M:%S') Memory Usage: $(ps -o rss= -p $PID) KB" | tee -a "log_$dir_name.txt"
            sleep 1
        done

        # Wait for the program to finish
        wait $PID
    fi
done
