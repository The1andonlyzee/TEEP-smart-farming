# Troubleshooting Flowchart

## Is the system working?

```
START
  │
  ├─→ Open browser → Go to http://localhost:3000
  │
  ├─→ Can you see camera feeds?
  │   │
  │   ├─→ YES → System is working! ✅
  │   │         Check data files to confirm logging
  │   │
  │   └─→ NO → Continue below ↓

  ├─→ Open terminal
  │
  ├─→ Type: cd ~/Documents/TEEP-smart-farming/camera-utils
  │
  ├─→ Type: ./status.sh
  │
  ├─→ What does it say?
      │
      ├─→ "Server Status: RUNNING" ✅
      │   │
      │   ├─→ But website doesn't work?
      │   │   → Firewall issue
      │   │   → Try: sudo ufw allow 3000
      │   │
      │   └─→ "HTTP Server: NOT RESPONDING" ❌
      │       → Type: sudo systemctl restart smart-farm-logger
      │       → Wait 30 seconds
      │       → Check website again
      │
      └─→ "Server Status: NOT RUNNING" ❌
          │
          ├─→ Type: sudo systemctl start smart-farm-logger
          │
          ├─→ Type: sudo systemctl status smart-farm-logger
          │
          ├─→ Does it say "active (running)" now?
              │
              ├─→ YES → Fixed! ✅
              │
              └─→ NO → Read error message
                  │
                  ├─→ Says "permission denied"
                  │   → Type: sudo chmod +x smart_farm_logger.js
                  │   → Try starting again
                  │
                  ├─→ Says "port already in use"
                  │   → Type: sudo lsof -i :3000
                  │   → Type: sudo kill -9 [PID from above]
                  │   → Try starting again
                  │
                  ├─→ Says "module not found"
                  │   → Type: npm install
                  │   → Try starting again
                  │
                  └─→ Other error → CALL SUPPORT
                      Copy the error message and send it
```

---

## No sensor data being logged?

```
START
  │
  ├─→ Check if today's CSV exists
  │   Location: farm_data/sensor_logs/2026-02-22.csv
  │
  ├─→ Does the file exist?
      │
      ├─→ NO → Server might not be running
      │   │    → Go to "Is the system working?" above
      │   │
      │   └─→ File exists but empty/old?
      │       │
      │       ├─→ Check ThingsBoard
      │       │   1. Open ThingsBoard in browser
      │       │   2. Go to Devices
      │       │   3. Click on Arduino_Mega_Sensors
      │       │   4. Check "Latest telemetry"
      │       │   │
      │       │   ├─→ No data / Old data?
      │       │   │   → Arduino/Sensor hardware problem
      │       │   │   → Check physical connections
      │       │   │   → Check Arduino power/WiFi
      │       │   │
      │       │   └─→ Has recent data?
      │       │       → Server can't connect to ThingsBoard
      │       │       → Check network connection
      │       │       → Restart server
      │
      └─→ YES, file has recent data → System working! ✅
```

---

## Cameras not capturing images?

```
START
  │
  ├─→ Check latest image
  │   Location: farm_data/timelapse_images/Camera_Zone_1/latest.jpg
  │
  ├─→ Does latest.jpg exist and is recent?
      │
      ├─→ NO or OLD → Check camera connection
      │   │
      │   ├─→ Type: ping 192.168.0.237
      │   │   │
      │   │   ├─→ No response → Camera is OFF or disconnected
      │   │   │                 → Check camera power
      │   │   │                 → Check network cable
      │   │   │
      │   │   └─→ Gets response → Camera is on
      │   │                        → Check if RTSP stream works
      │   │                        → Type: ffmpeg -i rtsp://192.168.0.237:554/stream1 -frames:v 1 test.jpg
      │   │                        │
      │   │                        ├─→ Works? → Server problem
      │   │                        │             → Restart server
      │   │                        │
      │   │                        └─→ Fails? → Camera RTSP issue
      │   │                                     → Reboot camera
      │   │                                     → Check camera settings
      │
      └─→ YES → System working! ✅
```

---

## Disk is full?

```
START
  │
  ├─→ Type: df -h
  │
  ├─→ Look at the row with your home directory
  │
  ├─→ Is "Use%" over 90%?
      │
      ├─→ YES → Clean up old data
      │   │
      │   ├─→ Option 1: Delete old sensor logs (>90 days)
      │   │   Type: find farm_data/sensor_logs -name "*.csv" -mtime +90 -delete
      │   │
      │   ├─→ Option 2: Delete old images (>90 days)
      │   │   Type: find farm_data/timelapse_images -name "*.jpg" -mtime +90 -delete
      │   │
      │   ├─→ Option 3: Backup and delete manually
      │   │   1. Copy farm_data folder to external drive
      │   │   2. Delete old folders using file manager
      │   │
      │   └─→ Check disk again: df -h
      │
      └─→ NO → Disk space is fine ✅
```

---

## After power outage / computer restart?

```
START
  │
  ├─→ Wait 2 minutes for system to fully boot
  │
  ├─→ Check if auto-suspend is still disabled
  │   Type: systemctl status sleep.target
  │   │
  │   ├─→ Says "masked" → Good ✅
  │   │
  │   └─→ Says "loaded" → Re-disable it
  │       Type: sudo systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target
  │
  ├─→ Check if service auto-started
  │   Type: sudo systemctl status smart-farm-logger
  │   │
  │   ├─→ Says "active (running)" → Good ✅
  │   │
  │   └─→ Says "inactive" or "failed"
  │       Type: sudo systemctl start smart-farm-logger
  │
  └─→ Open browser → http://localhost:3000
      │
      └─→ Cameras showing? → Everything recovered! ✅
```

---

## Weekly Health Check (Monday Morning)

```
1. Type: cd ~/Documents/TEEP-smart-farming/camera-utils

2. Type: ./status.sh

3. Look for problems:
   ├─→ All ✅ green checkmarks? → Everything is fine!
   │
   └─→ Any ❌ red X marks?
       ├─→ "Server Status: NOT RUNNING"
       │   → Type: sudo systemctl start smart-farm-logger
       │
       ├─→ "HTTP Server: NOT RESPONDING"  
       │   → Type: sudo systemctl restart smart-farm-logger
       │
       ├─→ "WARNING: Disk usage is XX%"
       │   → If > 90%, clean up old data (see "Disk is full?" above)
       │
       └─→ "WARNING: Sensor log not updated"
           → Check ThingsBoard (see "No sensor data?" above)

4. Open http://localhost:3000 in browser
   ├─→ Cameras showing live feed? → Good ✅
   └─→ Black screen or error? → Restart cameras

5. Check yesterday's data exists:
   Type: ls farm_data/sensor_logs/
   → Should see yesterday's CSV file

6. Done! System is healthy ✅
```

---

## Contact Tree

```
Problem Level 1: Basic Issues
  ├─→ Server not running
  ├─→ Website not loading
  └─→ Need to create time-lapse video
      → Use QUICK_REFERENCE.md
      → Try yourself first

Problem Level 2: Hardware Issues  
  ├─→ Camera physically disconnected
  ├─→ Arduino not sending data
  └─→ Network connectivity problems
      → Contact: Lab Technician
      → Phone: [FILL IN]

Problem Level 3: Software/Configuration
  ├─→ Need to change settings
  ├─→ Error messages you don't understand
  └─→ System crashes repeatedly
      → Contact: IT Support / Original Developer
      → Email: [FILL IN]
      → Include: Output of ./status.sh and error logs

EMERGENCY: Data Loss / Critical Failure
      → Contact: Lab Supervisor immediately
      → Phone: [FILL IN]
```

---

**Print this document and keep near the computer!**
