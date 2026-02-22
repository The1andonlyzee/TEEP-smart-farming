#!/bin/bash

# Smart Farm Logger Healthcheck Script
# Checks if the server is running and responding, restarts if needed

# Configuration
SERVER_URL="http://localhost:3000/status"
LOG_FILE="/home/k410-2nd-ubuntu-pc/Documents/TEEP-smart-farming/camera-utils/logs/healthcheck.log"
SCRIPT_DIR="/home/k410-2nd-ubuntu-pc/Documents/TEEP-smart-farming/camera-utils"
SCRIPT_NAME="smart_farm_logger.js"
MAX_RETRIES=3

# Create log directory if it doesn't exist
mkdir -p "$(dirname "$LOG_FILE")"

# Function to log messages
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

# Function to check if process is running
is_process_running() {
    pgrep -f "$SCRIPT_NAME" > /dev/null
    return $?
}

# Function to check if server responds
is_server_responding() {
    response=$(curl -s -o /dev/null -w "%{http_code}" --max-time 10 "$SERVER_URL" 2>/dev/null)
    if [ "$response" = "200" ]; then
        return 0
    else
        return 1
    fi
}

# Function to start the server
start_server() {
    log "Starting smart farm logger..."
    cd "$SCRIPT_DIR" || exit 1
    nohup node "$SCRIPT_NAME" >> "$SCRIPT_DIR/logs/smart_farm.log" 2>> "$SCRIPT_DIR/logs/smart_farm_error.log" &
    sleep 5
    log "Server started with PID: $(pgrep -f "$SCRIPT_NAME")"
}

# Function to stop the server
stop_server() {
    log "Stopping smart farm logger..."
    pkill -f "$SCRIPT_NAME"
    sleep 2
}

# Main healthcheck logic
log "========================================="
log "Starting healthcheck..."

# Check if process is running
if ! is_process_running; then
    log "ERROR: Process not running!"
    start_server
    
    # Verify it started
    if is_process_running; then
        log "SUCCESS: Server restarted successfully"
    else
        log "CRITICAL: Failed to restart server!"
        exit 1
    fi
else
    log "OK: Process is running (PID: $(pgrep -f "$SCRIPT_NAME"))"
    
    # Check if server responds
    if is_server_responding; then
        log "OK: Server responding correctly"
    else
        log "WARNING: Server not responding, attempting restart..."
        
        # Try stopping and starting
        stop_server
        start_server
        
        # Wait and verify
        sleep 10
        
        if is_server_responding; then
            log "SUCCESS: Server recovered after restart"
        else
            log "ERROR: Server still not responding after restart"
            exit 1
        fi
    fi
fi

# Check disk space
disk_usage=$(df -h "$SCRIPT_DIR" | awk 'NR==2 {print $5}' | sed 's/%//')
if [ "$disk_usage" -gt 90 ]; then
    log "WARNING: Disk usage is ${disk_usage}%"
fi

# Check if data is being logged (optional)
today=$(date +%Y-%m-%d)
sensor_log="$SCRIPT_DIR/farm_data/sensor_logs/${today}.csv"
if [ -f "$sensor_log" ]; then
    # Check if file was modified in last 30 minutes
    last_modified=$(stat -c %Y "$sensor_log")
    current_time=$(date +%s)
    diff=$((current_time - last_modified))
    
    if [ $diff -gt 1800 ]; then  # 30 minutes = 1800 seconds
        log "WARNING: Sensor log not updated in last 30 minutes"
    else
        log "OK: Sensor logging active (last update: $((diff/60)) minutes ago)"
    fi
fi

log "Healthcheck completed"
log "========================================="
