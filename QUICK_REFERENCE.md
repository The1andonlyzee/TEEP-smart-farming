# Smart Farm System - Quick Reference Guide
## For Non-Technical Users (Mechanical Engineers)

---

## 🚨 EMERGENCY: System Not Working?

### Step 1: Check if server is running
Open terminal and type:
```bash
cd ~/Documents/TEEP-smart-farming/camera-utils
./status.sh
```

**What you'll see:**
- ✅ Green checkmarks = Everything is fine
- ❌ Red X marks = Something is broken

### Step 2: Restart the system
```bash
sudo systemctl restart smart-farm-logger
```

Wait 30 seconds, then check status again.

### Step 3: Still broken?
**Call IT support** or check "Common Problems" section below.

---

## 📊 Daily Monitoring (No Programming Needed)

### Check System Status
```bash
cd ~/Documents/TEEP-smart-farming/camera-utils
./status.sh
```

**Look for:**
- Server Status: Should say "RUNNING"
- HTTP Server: Should say "RESPONDING"
- Sensor Log: Should have recent timestamp
- Camera captures: Should have recent timestamp

### View Live Cameras
1. Open web browser (Firefox/Chrome)
2. Go to: `http://localhost:3000`
3. You should see live camera feeds

---

## 📁 Where is My Data?

### Sensor Data (CSV files)
```
~/Documents/TEEP-smart-farming/camera-utils/farm_data/sensor_logs/
```

Each day creates one CSV file: `2026-02-22.csv`

**To open:**
- Right-click file → Open with LibreOffice Calc (Excel equivalent)

### Camera Images (for time-lapse)
```
~/Documents/TEEP-smart-farming/camera-utils/farm_data/timelapse_images/Camera_Zone_1/
~/Documents/TEEP-smart-farming/camera-utils/farm_data/timelapse_images/Camera_Zone_2/
```

**To view:**
- Double-click any `.jpg` file

---

## 🎬 Creating Time-lapse Videos

### Simple Method (Copy-Paste)

1. Open terminal
2. Navigate to camera folder:
```bash
cd ~/Documents/TEEP-smart-farming/camera-utils/farm_data/timelapse_images/Camera_Zone_1
```

3. Create video (copy this exactly):
```bash
ffmpeg -pattern_type glob -i 'frame_*.jpg' -r 12 -c:v libx264 timelapse.mp4
```

4. Video will be saved as `timelapse.mp4` in same folder

**Adjust speed:**
- `-r 12` = 12 frames per second (faster)
- `-r 6` = 6 frames per second (slower)
- `-r 24` = 24 frames per second (very fast)

---

## 🔧 Common Problems & Solutions

### Problem: Camera feed not showing on website

**Solution:**
```bash
# Check if cameras are on
ping 192.168.0.237  # Camera 1
ping 192.168.0.172  # Camera 2

# If ping works, restart server
sudo systemctl restart smart-farm-logger
```

### Problem: No new sensor data

**Check ThingsBoard:**
1. Open browser → Go to ThingsBoard (ask for URL)
2. Login
3. Go to Devices
4. Check if "Latest telemetry" has recent data
5. If yes → Server problem (restart)
6. If no → Arduino/sensor problem (check hardware)

### Problem: Disk space full

**Check space:**
```bash
df -h
```

**Clean old data (BE CAREFUL!):**
```bash
# Delete data older than 90 days
find ~/Documents/TEEP-smart-farming/camera-utils/farm_data -type f -mtime +90 -delete
```

**OR** manually delete old folders in file manager.

### Problem: Computer suspended/shut down

**Check auto-suspend:**
```bash
# Should say "masked"
systemctl status sleep.target

# If not, disable it:
sudo systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target
```

### Problem: System doesn't start after reboot

**Check service:**
```bash
sudo systemctl status smart-farm-logger
```

If says "inactive" or "failed":
```bash
sudo systemctl start smart-farm-logger
```

---

## ⚙️ Changing Settings (Advanced)

### Change Capture Schedule

**DON'T EDIT CODE!** Instead, ask someone with programming knowledge.

**Current settings:**
- Time-lapse: Every 1 hour
- Sensor logging: Every 10 minutes

**To change, edit this file:**
`~/Documents/TEEP-smart-farming/camera-utils/smart_farm_logger.js`

**Find these lines (around line 30):**
```javascript
timelapseSchedule: '0 * * * *',  // Every hour
sensorSchedule: '*/10 * * * *',  // Every 10 minutes
```

**Common schedules:**
- `'0 * * * *'` = Every hour
- `'*/30 * * * *'` = Every 30 minutes
- `'0 */2 * * *'` = Every 2 hours
- `'*/5 * * * *'` = Every 5 minutes

**After editing, restart:**
```bash
sudo systemctl restart smart-farm-logger
```

---

## 📞 Contact Information

**Before calling support, have ready:**
1. Output of `./status.sh`
2. Last 20 lines of error log:
```bash
tail -n 20 ~/Documents/TEEP-smart-farming/camera-utils/logs/smart_farm_error.log
```

**Support contacts:**
- IT Department: [FILL IN]
- Original Developer (Ganteng): [FILL IN]
- Lab Supervisor: [FILL IN]

---

## 📋 Weekly Maintenance Checklist

**Monday morning (5 minutes):**
- [ ] Run `./status.sh` - Everything green?
- [ ] Open http://localhost:3000 - Cameras working?
- [ ] Check yesterday's CSV file exists
- [ ] Check disk space < 80%

**End of month:**
- [ ] Create time-lapse video of the month
- [ ] Backup data to external drive
- [ ] Archive old data (>90 days) to external storage

---

## 🎓 Learning Resources

**Want to understand more?**

1. **Linux basics:** https://ubuntu.com/tutorials/command-line-for-beginners
2. **systemd services:** https://www.digitalocean.com/community/tutorials/systemd-essentials-working-with-services-units-and-the-journal
3. **FFmpeg video creation:** https://trac.ffmpeg.org/wiki/Slideshow

**But honestly:** You don't need to understand the internals. Just use this guide!

---

## 📝 System Architecture (Simple Explanation)

```
Sensors (Arduino/ESP8266) 
    ↓
ThingsBoard (Database)
    ↓
Smart Farm Logger (This software) ← Pulls data every 10 min
    ↓
Saves to: CSV files + Camera images
```

**What runs automatically:**
- ✅ Sensor data collection (every 10 min)
- ✅ Camera snapshots (every hour)
- ✅ Healthcheck (every 5 min)
- ✅ Auto-restart if crashed

**What you need to do manually:**
- Create time-lapse videos
- Backup data
- Monitor system health

---

**Last Updated:** February 22, 2026
**System Version:** 1.0
**Next Review Date:** [FILL IN]
