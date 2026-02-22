#!/bin/bash

# Smart Farm Logger Status Monitor
# Shows current status of the logging system

SCRIPT_DIR="/home/k410-2nd-ubuntu-pc/Documents/TEEP-smart-farming/camera-utils"

echo "╔════════════════════════════════════════════════════╗"
echo "║     Smart Farm Logger - Status Monitor            ║"
echo "╚════════════════════════════════════════════════════╝"
echo ""

# Check if process is running
if pgrep -f "smart_farm_logger.js" > /dev/null; then
    PID=$(pgrep -f "smart_farm_logger.js")
    echo "✅ Server Status: RUNNING (PID: $PID)"
    
    # Get process uptime
    STARTED=$(ps -p "$PID" -o lstart=)
    echo "   Started: $STARTED"
    
    # Get memory usage
    MEM=$(ps -p "$PID" -o rss= | awk '{printf "%.1f MB", $1/1024}')
    echo "   Memory: $MEM"
else
    echo "❌ Server Status: NOT RUNNING"
fi

echo ""

# Check HTTP response
if curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:3000/status | grep -q "200"; then
    echo "✅ HTTP Server: RESPONDING"
else
    echo "❌ HTTP Server: NOT RESPONDING"
fi

echo ""

# Check today's data
TODAY=$(date +%Y-%m-%d)

# Sensor logs
SENSOR_LOG="$SCRIPT_DIR/farm_data/sensor_logs/${TODAY}.csv"
if [ -f "$SENSOR_LOG" ]; then
    LINES=$(wc -l < "$SENSOR_LOG")
    LAST_MOD=$(stat -c %y "$SENSOR_LOG" | cut -d. -f1)
    echo "📊 Sensor Log (${TODAY}.csv):"
    echo "   Entries: $((LINES - 1))"  # Minus header
    echo "   Last updated: $LAST_MOD"
else
    echo "⚠️  No sensor log for today"
fi

echo ""

# Time-lapse images
for camera in Camera_Zone_1 Camera_Zone_2; do
    IMG_DIR="$SCRIPT_DIR/farm_data/timelapse_images/$camera"
    if [ -d "$IMG_DIR" ]; then
        IMG_COUNT=$(find "$IMG_DIR" -name "frame_*.jpg" -type f | wc -l)
        LATEST="$IMG_DIR/latest.jpg"
        if [ -f "$LATEST" ]; then
            LAST_IMG=$(stat -c %y "$LATEST" | cut -d. -f1)
            echo "📸 $camera:"
            echo "   Total frames: $IMG_COUNT"
            echo "   Last capture: $LAST_IMG"
        else
            echo "📸 $camera: $IMG_COUNT frames (no recent capture)"
        fi
    fi
done

echo ""

# Disk usage
echo "💾 Disk Usage:"
DISK_USAGE=$(df -h "$SCRIPT_DIR" | awk 'NR==2 {print $5}')
DISK_AVAIL=$(df -h "$SCRIPT_DIR" | awk 'NR==2 {print $4}')
echo "   Used: $DISK_USAGE"
echo "   Available: $DISK_AVAIL"

echo ""

# Data directory size
DATA_SIZE=$(du -sh "$SCRIPT_DIR/farm_data" 2>/dev/null | cut -f1)
echo "   farm_data/: $DATA_SIZE"

echo ""

# Recent errors
ERROR_LOG="$SCRIPT_DIR/logs/smart_farm_error.log"
if [ -f "$ERROR_LOG" ] && [ -s "$ERROR_LOG" ]; then
    ERROR_COUNT=$(wc -l < "$ERROR_LOG")
    echo "⚠️  Error log has $ERROR_COUNT entries"
    echo "   Last 3 errors:"
    tail -n 3 "$ERROR_LOG" | sed 's/^/   /'
else
    echo "✅ No errors logged"
fi

echo ""
echo "──────────────────────────────────────────────────────"
echo "Logs: $SCRIPT_DIR/logs/"
echo "Dashboard: http://localhost:3000"
echo "──────────────────────────────────────────────────────"
