# Smart Farm Dataset Logger - Setup Guide

Complete system for automated camera snapshots + sensor data logging synchronized with ThingsBoard.

## 📋 Features

✅ Automated periodic snapshots from all IP cameras  
✅ Synchronized sensor data logging from ThingsBoard  
✅ Organized dataset structure by date  
✅ Both JSON and CSV output formats  
✅ Manual capture trigger via web interface  
✅ Live camera streaming (JSMpeg)

---

## 🚀 Quick Start

### 1. Install Dependencies

```bash
npm install
```

Required packages:

- `express` - Web server
- `node-rtsp-stream` - RTSP to WebSocket streaming
- `node-cron` - Scheduled tasks
- `axios` - ThingsBoard API calls

### 2. Configure ThingsBoard Connection

Edit `jsmpeg_snapshot_server.js` and update the `TB_CONFIG` section:

```javascript
const TB_CONFIG = {
  host: "http://localhost:8080", // Your ThingsBoard URL
  username: "your_username@domain.com", // Your TB login
  password: "your_password",
  devices: [
    {
      name: "Arduino_Mega_Sensors",
      deviceId: "YOUR_DEVICE_ID_HERE",
      accessToken: "YOUR_ACCESS_TOKEN", // Optional
    },
    {
      name: "NodeMCU_Sensors",
      deviceId: "YOUR_DEVICE_ID_HERE",
      accessToken: "YOUR_ACCESS_TOKEN",
    },
  ],
};
```

#### How to Get Device IDs from ThingsBoard:

**Method 1: Via Web UI**

1. Go to ThingsBoard → Devices
2. Click on your device
3. Look at URL: `http://your-tb:8080/devices/<DEVICE_ID>`
4. Copy the UUID (e.g., `a1b2c3d4-e5f6-7890-abcd-ef1234567890`)

**Method 2: Via REST API**

```bash
# Login and get token
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"your@email.com","password":"yourpassword"}'

# List all devices
curl -X GET http://localhost:8080/api/tenant/devices?pageSize=100 \
  -H "X-Authorization: Bearer YOUR_TOKEN"
```

### 3. Configure Capture Schedule

Modify the schedule in `SNAPSHOT_CONFIG`:

```javascript
const SNAPSHOT_CONFIG = {
  schedule: "*/10 * * * *", // Every 10 minutes
  baseDir: "./datasets",
  imageQuality: 2, // 2 = high quality, 31 = low quality
  imageWidth: 1280,
  imageHeight: 720,
};
```

**Cron Schedule Examples:**

```
'*/5 * * * *'    - Every 5 minutes
'*/10 * * * *'   - Every 10 minutes
'*/30 * * * *'   - Every 30 minutes
'0 * * * *'      - Every hour
'0 */2 * * *'    - Every 2 hours
'0 9-17 * * *'   - Every hour from 9 AM to 5 PM
'0 8,12,16 * * *' - At 8 AM, 12 PM, and 4 PM
```

### 4. Run the Server

```bash
node jsmpeg_snapshot_server.js
```

Or with auto-reload during development:

```bash
npm run dev
```

---

## 📁 Dataset Structure

```
datasets/
├── snapshots/
│   ├── 2026-02-20/
│   │   ├── Camera_Zone_1_2026-02-20T10-00-00.jpg
│   │   ├── Camera_Zone_1_2026-02-20T10-10-00.jpg
│   │   ├── Camera_Zone_2_2026-02-20T10-00-00.jpg
│   │   └── Camera_Zone_2_2026-02-20T10-10-00.jpg
│   └── 2026-02-21/
│       └── ...
│
├── sensor_logs/
│   ├── 2026-02-20/
│   │   ├── sensors_2026-02-20T10-00-00.json
│   │   ├── sensors_2026-02-20T10-10-00.json
│   │   └── sensors_2026-02-20.csv  (daily aggregated)
│   └── 2026-02-21/
│       └── ...
│
└── summaries/
    ├── summary_2026-02-20T10-00-00.json
    └── summary_2026-02-20T10-10-00.json
```

### Example Files

**sensors_2026-02-20T10-00-00.json:**

```json
{
  "capture_timestamp": "2026-02-20T10:00:00.000Z",
  "unix_timestamp": 1708502400000,
  "devices": {
    "Arduino_Mega_Sensors": {
      "soil_moisture_1": {
        "value": 450,
        "timestamp": 1708502398234
      },
      "soil_moisture_2": {
        "value": 523,
        "timestamp": 1708502398234
      },
      "soil_ph_1": {
        "value": 6.8,
        "timestamp": 1708502398234
      }
    },
    "NodeMCU_Sensors": {
      "temperature": {
        "value": 28.5,
        "timestamp": 1708502399102
      },
      "humidity": {
        "value": 65.2,
        "timestamp": 1708502399102
      }
    }
  }
}
```

**sensors_2026-02-20.csv:**

```csv
timestamp,unix_timestamp,Arduino_Mega_Sensors_soil_moisture_1,Arduino_Mega_Sensors_soil_moisture_2,Arduino_Mega_Sensors_soil_ph_1,NodeMCU_Sensors_temperature,NodeMCU_Sensors_humidity
2026-02-20T10:00:00.000Z,1708502400000,450,523,6.8,28.5,65.2
2026-02-20T10:10:00.000Z,1708502700000,448,520,6.9,28.7,64.8
```

---

## 🌐 Web Interface

Access at: **http://localhost:3000**

Features:

- Live camera streams
- Manual capture button
- Connection status indicators
- Schedule information

### API Endpoints

**Status Check:**

```bash
curl http://localhost:3000/status
```

**Manual Trigger:**

```bash
curl http://localhost:3000/capture
```

---

## 🔧 Troubleshooting

### Camera Snapshots Not Working

**Check FFmpeg:**

```bash
ffmpeg -version
which ffmpeg
```

**Test RTSP stream manually:**

```bash
ffmpeg -rtsp_transport tcp -i "rtsp://192.168.0.237:554/stream1" \
  -frames:v 1 -q:v 2 test_snapshot.jpg
```

### ThingsBoard Connection Issues

**Test login:**

```bash
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"your@email.com","password":"yourpassword"}'
```

**Check device ID:**

```bash
# After getting token from login
curl -X GET http://localhost:8080/api/tenant/devices?pageSize=100 \
  -H "X-Authorization: Bearer YOUR_TOKEN"
```

### No Sensor Data

1. **Verify devices are sending data to ThingsBoard**
   - Check ThingsBoard UI → Devices → Latest Telemetry
2. **Check device names match**
   - Device names in `TB_CONFIG.devices` must match actual device names
3. **Check authentication**
   - Username/password correct
   - User has permission to access devices

### Permission Errors

```bash
# If you get permission errors creating directories:
sudo chown -R $USER:$USER ./datasets
chmod -R 755 ./datasets
```

---

## 📊 Using the Dataset

### Load in Python

```python
import json
import pandas as pd
from PIL import Image
from pathlib import Path

# Load sensor data
sensor_file = 'datasets/sensor_logs/2026-02-20/sensors_2026-02-20T10-00-00.json'
with open(sensor_file) as f:
    data = json.load(f)

# Load camera snapshot
img_file = 'datasets/snapshots/2026-02-20/Camera_Zone_1_2026-02-20T10-00-00.jpg'
img = Image.open(img_file)

# Load daily CSV
csv_file = 'datasets/sensor_logs/2026-02-20/sensors_2026-02-20.csv'
df = pd.read_csv(csv_file)
```

### Batch Processing

```python
from pathlib import Path
import json

dataset_dir = Path('datasets/summaries')

for summary_file in sorted(dataset_dir.glob('*.json')):
    with open(summary_file) as f:
        summary = json.load(f)

    timestamp = summary['capture_time']
    snapshot_paths = [s['filepath'] for s in summary['snapshots']]
    sensor_data = summary['sensor_data']

    # Your processing here
    print(f"Processing {timestamp}")
```

---

## 🎯 Advanced Configuration

### Different Schedules for Cameras and Sensors

If you want separate schedules:

```javascript
// Separate camera snapshot schedule
cron.schedule("*/5 * * * *", async () => {
  await captureAllSnapshots();
});

// Separate sensor logging schedule
cron.schedule("*/10 * * * *", async () => {
  await logSensorData(new Date());
});
```

### Adjust Image Quality

```javascript
const SNAPSHOT_CONFIG = {
  imageQuality: 2, // 2-31, lower = better quality, larger file
  imageWidth: 1920, // Higher resolution
  imageHeight: 1080,
};
```

Quality recommendations:

- `2` = Publication quality (~500KB per image)
- `5` = High quality (~200KB per image)
- `10` = Good quality (~100KB per image)
- `15` = Medium quality (~50KB per image)

### Add More Cameras

```javascript
const cameras = [
  {
    name: "Camera_Zone_1",
    rtspUrl: "rtsp://192.168.0.237:554/stream1",
    wsPort: 9001,
  },
  {
    name: "Camera_Zone_2",
    rtspUrl: "rtsp://192.168.0.172:554/profile1",
    wsPort: 9002,
  },
  {
    name: "Camera_Zone_3",
    rtspUrl: "rtsp://192.168.0.240:554/stream1",
    wsPort: 9003,
  },
];
```

### Add More Sensor Devices

```javascript
const TB_CONFIG = {
  // ...
  devices: [
    {
      name: "Arduino_Mega_Sensors",
      deviceId: "uuid-1",
    },
    {
      name: "NodeMCU_Sensors",
      deviceId: "uuid-2",
    },
    {
      name: "ESP32_Environmental",
      deviceId: "uuid-3",
    },
  ],
};
```

---

## 🐳 Docker Deployment (Optional)

If you want to run this alongside ThingsBoard in Docker:

**Dockerfile:**

```dockerfile
FROM node:18-alpine

RUN apk add --no-cache ffmpeg

WORKDIR /app
COPY package*.json ./
RUN npm install

COPY . .

EXPOSE 3000 9001 9002

CMD ["node", "jsmpeg_snapshot_server.js"]
```

**docker-compose.yml addition:**

```yaml
camera-logger:
  build: .
  ports:
    - "3000:3000"
    - "9001:9001"
    - "9002:9002"
  volumes:
    - ./datasets:/app/datasets
  depends_on:
    - thingsboard
  restart: unless-stopped
```

---

## 📝 Notes

- **Timestamps are synchronized** - snapshots and sensor readings use the same timestamp for each capture cycle
- **CSV files are daily** - one CSV per day with all readings appended
- **JSON files are per-capture** - individual files for each capture event
- **Summary files** - link snapshots to sensor data for easy batch processing
- **Automatic directory creation** - date-based folders created automatically
- **Error handling** - continues running even if individual captures fail

---

## 📞 Support

For issues or questions:

1. Check logs in terminal
2. Verify ThingsBoard connection
3. Test camera RTSP URLs manually
4. Check file permissions on `datasets/` directory

---

## 🚀 Next Steps

After setup:

1. Run a manual test capture: http://localhost:3000 → Click "Capture Dataset Now"
2. Check `datasets/` directory for output
3. Verify both snapshots and sensor logs are created
4. Let it run on schedule
5. Use the data for your research!
