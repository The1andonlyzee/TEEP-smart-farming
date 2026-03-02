# Configuration Guide - For Non-Technical Users

All system settings are stored in **easy-to-edit JSON files** in the `system/config/` directory.

**You DON'T need to edit any code!** Just edit these simple text files.

---

## 📁 Configuration Files

```
system/config/
├── cameras.json       ← Camera settings
├── schedules.json     ← When to capture images/log data
└── thingsboard.json   ← Sensor database connection
```

---

## 📹 Editing Camera Settings

**File:** `system/config/cameras.json`

### Example:
```json
{
  "cameras": [
    {
      "name": "Camera_Zone_1",
      "rtspUrl": "rtsp://192.168.0.237:554/stream1",
      "wsPort": 9001,
      "enabled": true,
      "description": "Main growing area camera"
    }
  ]
}
```

### What each setting means:

- **name**: What to call this camera (shows on dashboard)
- **rtspUrl**: Camera's network address
  - Format: `rtsp://IP_ADDRESS:PORT/STREAM_PATH`
  - Get this from your camera's manual
- **wsPort**: Port number for streaming (9001, 9002, 9003, etc.)
  - Each camera needs a different port
- **enabled**: `true` = active, `false` = disabled
- **description**: Notes for yourself

### Adding a new camera:

```json
{
  "cameras": [
    {
      "name": "Camera_Zone_1",
      "rtspUrl": "rtsp://192.168.0.237:554/stream1",
      "wsPort": 9001,
      "enabled": true
    },
    {
      "name": "Camera_Zone_3",           ← NEW CAMERA
      "rtspUrl": "rtsp://192.168.0.240:554/stream1",
      "wsPort": 9003,                    ← Different port!
      "enabled": true
    }
  ]
}
```

**Remember:** Add a comma after each `}` except the last one!

---

## ⏰ Editing Schedules

**File:** `system/config/schedules.json`

### Example:
```json
{
  "timelapse": {
    "schedule": "0 * * * *",
    "enabled": true,
    "imageQuality": 5,
    "imageWidth": 1920,
    "imageHeight": 1080
  },
  "sensors": {
    "schedule": "*/10 * * * *",
    "enabled": true
  },
  "dataDirectory": "../data"
}
```

### What each setting means:

**Time-lapse settings:**
- **schedule**: When to capture images (see schedule guide below)
- **enabled**: `true` = active, `false` = turn off
- **imageQuality**: 2 = best quality (big files), 20 = lower quality (small files)
  - **Recommended:** 5 for time-lapse videos
- **imageWidth**: Image width in pixels (1920 = Full HD)
- **imageHeight**: Image height in pixels (1080 = Full HD)

**Sensor settings:**
- **schedule**: When to log sensor data
- **enabled**: `true` = active, `false` = turn off

**Data location:**
- **dataDirectory**: Where to save files (`../data` = data folder)

### Common Schedules:

Copy one of these into the "schedule" field:

```
"0 * * * *"       Every hour
"*/30 * * * *"    Every 30 minutes
"*/10 * * * *"    Every 10 minutes
"*/5 * * * *"     Every 5 minutes
"0 */2 * * *"     Every 2 hours
"0 9-17 * * *"    Every hour from 9 AM to 5 PM
"0 8,12,16 * * *" At 8 AM, 12 PM, and 4 PM only
```

### Examples:

**Capture time-lapse every 2 hours:**
```json
"timelapse": {
  "schedule": "0 */2 * * *",
  "enabled": true
}
```

**Log sensors every 5 minutes:**
```json
"sensors": {
  "schedule": "*/5 * * * *",
  "enabled": true
}
```

**Turn off time-lapse temporarily:**
```json
"timelapse": {
  "schedule": "0 * * * *",
  "enabled": false    ← Changed to false
}
```

---

## 🔌 Editing ThingsBoard Connection

**File:** `system/config/thingsboard.json`

### Example:
```json
{
  "host": "http://localhost:8080",
  "username": "tenant@thingsboard.org",
  "password": "tenant",
  "devices": [
    {
      "name": "Arduino_Mega_Sensors",
      "deviceId": "a8046cd0-0726-11f1-b87c-d9ef95b7b8d0",
      "description": "Main Arduino with soil sensors"
    }
  ]
}
```

### What each setting means:

- **host**: ThingsBoard server address
  - `http://localhost:8080` if running on same computer
  - `http://IP_ADDRESS:8080` if running on another computer
- **username**: Your ThingsBoard login email
- **password**: Your ThingsBoard password
- **devices**: List of sensor devices
  - **name**: What to call this device
  - **deviceId**: Unique ID from ThingsBoard
  - **description**: Notes for yourself

### How to get Device IDs:

**Easy way:** Run this command:
```bash
cd system
node tb_config_helper.js
```

It will show you all your devices and their IDs automatically!

---

## ✏️ How to Edit Config Files

### Method 1: Text Editor (Simple)

1. Right-click the file → Open With → Text Editor
2. Make your changes
3. Save (Ctrl+S)
4. Restart the server:
   ```bash
   sudo systemctl restart smart-farm-logger
   ```

### Method 2: Command Line

```bash
cd ~/Documents/TEEP-smart-farming/system/config
nano cameras.json       # Edit cameras
nano schedules.json     # Edit schedules
nano thingsboard.json   # Edit ThingsBoard

# After editing:
# Press Ctrl+X, then Y, then Enter to save

# Restart server:
sudo systemctl restart smart-farm-logger
```

---

## ⚠️ Important Rules

### JSON Syntax Rules:

1. **Commas:** Add comma after `}` except the last one:
   ```json
   { "a": 1 },  ← Comma here
   { "b": 2 },  ← Comma here
   { "c": 3 }   ← NO comma on last one
   ```

2. **Quotes:** Always use double quotes `"`, not single `'`:
   ```json
   "schedule": "0 * * * *"  ✅ Correct
   'schedule': '0 * * * *'  ❌ Wrong
   ```

3. **Case-sensitive:** `true` is not the same as `True`:
   ```json
   "enabled": true   ✅ Correct
   "enabled": True   ❌ Wrong
   ```

### After Editing:

**ALWAYS restart the server** after changing config:
```bash
sudo systemctl restart smart-farm-logger
```

Or use the tool:
```bash
cd ~/Documents/TEEP-smart-farming/tools
./restart-server
```

---

## 🔍 Checking If Config Is Valid

After editing, check if you made any syntax errors:

```bash
cd ~/Documents/TEEP-smart-farming/system
node smart_farm_logger.js
```

**If you see errors:** You have a syntax mistake (probably missing comma or quote)

**If you see:** 
```
✓ ThingsBoard: http://localhost:8080
✓ Devices: 2
✓ Cameras: 2 enabled
```

Then your config is valid! ✅

Press Ctrl+C to stop, then restart properly:
```bash
sudo systemctl restart smart-farm-logger
```

---

## 📝 Common Changes

### Change Time-lapse Frequency

Edit `system/config/schedules.json`:
```json
"timelapse": {
  "schedule": "0 */2 * * *",  ← Change this line
  "enabled": true
}
```

Restart:
```bash
sudo systemctl restart smart-farm-logger
```

### Add a Third Camera

Edit `system/config/cameras.json`:
```json
{
  "cameras": [
    { "name": "Camera_Zone_1", "rtspUrl": "rtsp://192.168.0.237:554/stream1", "wsPort": 9001, "enabled": true },
    { "name": "Camera_Zone_2", "rtspUrl": "rtsp://192.168.0.172:554/profile1", "wsPort": 9002, "enabled": true },
    { "name": "Camera_Zone_3", "rtspUrl": "rtsp://192.168.0.240:554/stream1", "wsPort": 9003, "enabled": true }
  ]
}
```

Restart:
```bash
sudo systemctl restart smart-farm-logger
```

### Disable Time-lapse Temporarily

Edit `system/config/schedules.json`:
```json
"timelapse": {
  "schedule": "0 * * * *",
  "enabled": false  ← Change to false
}
```

Restart - done!

---

## 🆘 Help! I Broke Something!

### Config file won't load:

1. Check for syntax errors (missing commas, quotes)
2. Use online JSON validator: https://jsonlint.com
3. Copy-paste your config file there - it will show errors

### Server won't start:

```bash
# Check what's wrong:
sudo systemctl status smart-farm-logger

# View error details:
sudo journalctl -u smart-farm-logger -n 50
```

### Reset to default:

If you completely broke the config, restore from backup:

```bash
cd ~/Documents/TEEP-smart-farming/system/config
cp cameras.json cameras.json.broken
# Then manually fix or ask for help
```

---

## 💡 Pro Tips

1. **Before editing:** Make a backup copy
   ```bash
   cp cameras.json cameras.json.backup
   ```

2. **Test syntax:** Use https://jsonlint.com to validate

3. **Small changes:** Change one thing at a time, test, then change more

4. **Keep it simple:** Don't change things you don't understand

5. **Document changes:** Add comments in "description" fields

---

**Need Help?**

- Check `TROUBLESHOOTING_FLOWCHART.md`
- View current config: http://localhost:3000/config
- Contact: [YOUR CONTACT INFO HERE]
