#!/bin/bash

# Configuration
CONFIG="Release"
BINARY="./build/Pholio"

# Initial Build if binary doesn't exist
if [ ! -f "$BINARY" ]; then
    echo "Binary not found, performing initial build..."
    python3 build.py --config $CONFIG
fi

while true; do
    echo "Starting Pholio..."
    export PHOLIO_RESTART_VIA_EXIT_CODE=1
    "$BINARY" "$@"
    EXIT_CODE=$?
    
    if [ $EXIT_CODE -eq 42 ]; then
        echo "Restart requested by application (Exit Code 42)."
        # Just continue the loop
    elif [ $EXIT_CODE -eq 43 ]; then
        echo "Rebuild and Restart requested by application (Exit Code 43)."
        python3 build.py --config $CONFIG
        if [ $? -ne 0 ]; then
            echo "Build failed. Stopping."
            exit 1
        fi
    else
        echo "Application exited normally (Exit Code $EXIT_CODE)."
        exit $EXIT_CODE
    fi
    
    echo "----------------------------------------"
    sleep 1
done
