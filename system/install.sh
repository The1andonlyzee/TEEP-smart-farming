#!/bin/bash

# Installation script for Smart Farm Logger as a system service

echo "================================================"
echo "Smart Farm Logger - Service Installation"
echo "================================================"
echo ""

# Get the current directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
USER_NAME=$(whoami)

echo "Installation directory: $SCRIPT_DIR"
echo "Running as user: $USER_NAME"
echo ""

# Check if service file exists
if [ ! -f "$SCRIPT_DIR/smart-farm-logger.service" ]; then
    echo "❌ Error: smart-farm-logger.service not found in $SCRIPT_DIR"
    echo "Please make sure the service file is in the same directory as this script."
    exit 1
fi

# Create logs directory
echo "Creating logs directory..."
mkdir -p "$SCRIPT_DIR/logs"

# Make scripts executable
echo "Setting up scripts..."
chmod +x "$SCRIPT_DIR/healthcheck.sh" 2>/dev/null
chmod +x "$SCRIPT_DIR/status.sh" 2>/dev/null

# Option 1: Systemd Service (recommended)
echo ""
echo "Choose installation method:"
echo "1) Systemd service (recommended - auto-start on boot)"
echo "2) Cron job (simpler - manual start required)"
read -p "Enter choice (1 or 2): " choice

if [ "$choice" = "1" ]; then
    echo ""
    echo "Installing as systemd service..."
    
    # Create temporary service file with updated paths
    TMP_SERVICE=$(mktemp)
    cat "$SCRIPT_DIR/smart-farm-logger.service" > "$TMP_SERVICE"
    
    # Update paths in the temporary file
    sed -i "s|User=.*|User=$USER_NAME|g" "$TMP_SERVICE"
    sed -i "s|WorkingDirectory=.*|WorkingDirectory=$SCRIPT_DIR|g" "$TMP_SERVICE"
    sed -i "s|ExecStart=.*|ExecStart=/usr/bin/node $SCRIPT_DIR/smart_farm_logger.js|g" "$TMP_SERVICE"
    sed -i "s|StandardOutput=.*|StandardOutput=append:$SCRIPT_DIR/logs/smart_farm.log|g" "$TMP_SERVICE"
    sed -i "s|StandardError=.*|StandardError=append:$SCRIPT_DIR/logs/smart_farm_error.log|g" "$TMP_SERVICE"
    
    echo "Updated service file:"
    echo "  User: $USER_NAME"
    echo "  Working Directory: $SCRIPT_DIR"
    echo ""
    
    # Copy service file
    sudo cp "$TMP_SERVICE" /etc/systemd/system/smart-farm-logger.service
    rm "$TMP_SERVICE"
    
    # Reload systemd
    echo "Reloading systemd..."
    sudo systemctl daemon-reload
    
    # Stop if already running
    sudo systemctl stop smart-farm-logger.service 2>/dev/null
    
    # Enable service
    echo "Enabling service..."
    sudo systemctl enable smart-farm-logger.service
    
    # Start service
    echo "Starting service..."
    sudo systemctl start smart-farm-logger.service
    
    # Wait a moment for service to start
    sleep 3
    
    # Check status
    if sudo systemctl is-active --quiet smart-farm-logger.service; then
        echo ""
        echo "✅ Service installed and running successfully!"
    else
        echo ""
        echo "⚠️  Service installed but may have failed to start."
        echo "Check status with: sudo systemctl status smart-farm-logger"
    fi
    
    echo ""
    echo "Useful commands:"
    echo "  sudo systemctl status smart-farm-logger   # Check status"
    echo "  sudo systemctl start smart-farm-logger    # Start"
    echo "  sudo systemctl stop smart-farm-logger     # Stop"
    echo "  sudo systemctl restart smart-farm-logger  # Restart"
    echo "  sudo journalctl -u smart-farm-logger -f   # View logs"
    echo "  $SCRIPT_DIR/status.sh                     # Quick status check"
    
    # Set up healthcheck cron
    echo ""
    read -p "Install healthcheck cron job? (runs every 5 minutes) [y/N]: " install_cron
    if [ "$install_cron" = "y" ] || [ "$install_cron" = "Y" ]; then
        # Remove existing cron job if present
        crontab -l 2>/dev/null | grep -v "healthcheck.sh" | crontab - 2>/dev/null
        
        # Add new cron job
        (crontab -l 2>/dev/null; echo "*/5 * * * * $SCRIPT_DIR/healthcheck.sh") | crontab -
        echo "✅ Healthcheck cron job installed (runs every 5 minutes)"
    fi
    
elif [ "$choice" = "2" ]; then
    echo ""
    echo "Installing cron jobs..."
    
    # Remove existing cron jobs if present
    crontab -l 2>/dev/null | grep -v "smart_farm_logger.js\|healthcheck.sh\|Smart Farm Logger" | crontab - 2>/dev/null
    
    # Add new cron jobs
    (
        crontab -l 2>/dev/null
        echo ""
        echo "# Smart Farm Logger - Auto-start on reboot"
        echo "@reboot cd $SCRIPT_DIR && /usr/bin/node smart_farm_logger.js >> logs/smart_farm.log 2>> logs/smart_farm_error.log &"
        echo ""
        echo "# Smart Farm Logger - Healthcheck every 5 minutes"
        echo "*/5 * * * * $SCRIPT_DIR/healthcheck.sh"
    ) | crontab -
    
    echo "✅ Cron jobs installed!"
    echo ""
    echo "Starting server now..."
    cd "$SCRIPT_DIR"
    nohup node smart_farm_logger.js >> logs/smart_farm.log 2>> logs/smart_farm_error.log &
    sleep 2
    
    if pgrep -f smart_farm_logger.js > /dev/null; then
        PID=$(pgrep -f smart_farm_logger.js)
        echo "✅ Server started with PID: $PID"
    else
        echo "⚠️  Server may have failed to start. Check logs:"
        echo "  tail -f $SCRIPT_DIR/logs/smart_farm_error.log"
    fi
    
else
    echo "Invalid choice. Exiting."
    exit 1
fi

# Disable auto-suspend (optional)
echo ""
read -p "Disable automatic suspend? (recommended for lab server) [y/N]: " disable_suspend
if [ "$disable_suspend" = "y" ] || [ "$disable_suspend" = "Y" ]; then
    echo "Disabling automatic suspend..."
    sudo systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target
    echo "✅ Auto-suspend disabled"
fi

echo ""
echo "================================================"
echo "Installation complete!"
echo "================================================"
echo ""
echo "📊 Monitor your server:"
echo "  Web dashboard:    http://localhost:3000"
echo "  Quick status:     $SCRIPT_DIR/status.sh"
echo ""
echo "📁 Logs location:"
echo "  Application log:  tail -f $SCRIPT_DIR/logs/smart_farm.log"
echo "  Error log:        tail -f $SCRIPT_DIR/logs/smart_farm_error.log"
echo "  Healthcheck log:  tail -f $SCRIPT_DIR/logs/healthcheck.log"
echo ""
echo "📝 Configuration:"
echo "  Edit config files in: $SCRIPT_DIR/config/"
echo "  After changes run:    sudo systemctl restart smart-farm-logger"
echo ""