#!/bin/bash

# Check if logs directory exists, if not create it
mkdir -p logs

# Load the TBB environment variables
source tbb2020/bin/tbbvars.sh intel64

# Get the command input from the terminal
COMMAND="$@"

# Extract the base command name (for the log file)
BASE_COMMAND=$(basename "$1")

# Generate the log file name based on the base command and the current timestamp
LOG_FILE="logs/${BASE_COMMAND}_$(date +'%Y-%m-%d_%H-%M-%S').log"

# Record the full command at the first line of the log file
echo "Command: $COMMAND" | tee -a $LOG_FILE

# Start the provided program in the background and redirect its output to the log file
$COMMAND > >(tee -a $LOG_FILE) 2>&1 &

# Get the PID of the program
PID=$!

# Monitor memory usage every 3 seconds and append it to the log file
while kill -0 $PID 2>/dev/null; do
    echo "$(date +'%Y-%m-%d %H:%M:%S') Memory Usage: $(ps -o rss= -p $PID) KB" | tee -a $LOG_FILE
    sleep 3
done

# Wait for the program to finish
wait $PID
