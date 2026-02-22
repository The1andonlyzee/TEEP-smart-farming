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

# Create logs directory
echo "Creating logs directory..."
mkdir -p "$SCRIPT_DIR/logs"

# Make healthcheck script executable
echo "Setting up healthcheck script..."
chmod +x "$SCRIPT_DIR/healthcheck.sh"

# Option 1: Systemd Service (recommended)
echo ""
echo "Choose installation method:"
echo "1) Systemd service (recommended - auto-start on boot)"
echo "2) Cron job (simpler - manual start required)"
read -p "Enter choice (1 or 2): " choice

if [ "$choice" = "1" ]; then
    echo ""
    echo "Installing as systemd service..."
    
    # Update service file with actual paths
    sed -i "s|/home/k410-2nd-ubuntu-pc/Documents/TEEP-smart-farming/camera-utils|$SCRIPT_DIR|g" smart-farm-logger.service
    sed -i "s|k410-2nd-ubuntu-pc|$USER_NAME|g" smart-farm-logger.service
    
    # Copy service file
    sudo cp smart-farm-logger.service /etc/systemd/system/
    
    # Reload systemd
    sudo systemctl daemon-reload
    
    # Enable service
    sudo systemctl enable smart-farm-logger.service
    
    # Start service
    sudo systemctl start smart-farm-logger.service
    
    echo ""
    echo "Service installed! Check status with:"
    echo "  sudo systemctl status smart-farm-logger"
    echo ""
    echo "Useful commands:"
    echo "  sudo systemctl start smart-farm-logger    # Start"
    echo "  sudo systemctl stop smart-farm-logger     # Stop"
    echo "  sudo systemctl restart smart-farm-logger  # Restart"
    echo "  sudo journalctl -u smart-farm-logger -f   # View logs"
    
    # Set up healthcheck cron
    echo ""
    read -p "Install healthcheck cron job? (runs every 5 minutes) [y/N]: " install_cron
    if [ "$install_cron" = "y" ] || [ "$install_cron" = "Y" ]; then
        # Add cron job
        (crontab -l 2>/dev/null; echo "*/5 * * * * $SCRIPT_DIR/healthcheck.sh") | crontab -
        echo "Healthcheck cron job installed (runs every 5 minutes)"
    fi
    
elif [ "$choice" = "2" ]; then
    echo ""
    echo "Installing cron jobs..."
    
    # Add cron jobs
    (
        crontab -l 2>/dev/null
        echo "# Smart Farm Logger - Auto-start on reboot"
        echo "@reboot cd $SCRIPT_DIR && /usr/bin/node smart_farm_logger.js >> logs/smart_farm.log 2>> logs/smart_farm_error.log &"
        echo ""
        echo "# Smart Farm Logger - Healthcheck every 5 minutes"
        echo "*/5 * * * * $SCRIPT_DIR/healthcheck.sh"
    ) | crontab -
    
    echo "Cron jobs installed!"
    echo ""
    echo "Starting server now..."
    cd "$SCRIPT_DIR"
    nohup node smart_farm_logger.js >> logs/smart_farm.log 2>> logs/smart_farm_error.log &
    sleep 2
    echo "Server started with PID: $(pgrep -f smart_farm_logger.js)"
    
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
    echo "Auto-suspend disabled"
fi

echo ""
echo "================================================"
echo "Installation complete!"
echo "================================================"
echo ""
echo "Monitor your server:"
echo "  Web dashboard: http://localhost:3000"
echo "  Logs: tail -f $SCRIPT_DIR/logs/smart_farm.log"
echo "  Healthcheck log: tail -f $SCRIPT_DIR/logs/healthcheck.log"
echo ""
