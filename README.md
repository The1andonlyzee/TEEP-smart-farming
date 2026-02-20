# Smart Farm Dataset Logger

Automated system for capturing synchronized camera snapshots and sensor readings from your smart farm IoT setup.

## 🎯 What This Does

- **Periodically captures** snapshots from all IP cameras (every 10 min by default)
- **Logs sensor data** from ThingsBoard-connected devices (Arduino Mega, NodeMCU, etc.)
- **Synchronizes timestamps** between cameras and sensors
- **Organizes data** in research-ready format (JSON + CSV)
- **Provides live streaming** dashboard for monitoring
- **Exports for ML** training with proper structure

## 📁 What You Get

```
datasets/
├── snapshots/           # Camera images organized by date
├── sensor_logs/         # JSON + CSV sensor readings
└── summaries/           # Links cameras + sensors per capture
```

Perfect for:
- Building ML training datasets
- Time-series analysis
- Computer vision + sensor fusion research
- Agricultural monitoring studies

---

## 🚀 Quick Start

### 1. Install Dependencies

```bash
npm install
```

**Required:**
- Node.js (v16+)
- FFmpeg (for snapshot capture)
- ThingsBoard (running with your devices)

### 2. Get Your ThingsBoard Configuration

Run the helper script to get device IDs:

```bash
node tb_config_helper.js
```

This will:
- Connect to your ThingsBoard
- List all your devices
- Generate the configuration code
- Test telemetry access

Copy the output and paste into `jsmpeg_snapshot_server.js` at the `TB_CONFIG` section.

### 3. Update Camera URLs

Edit `jsmpeg_snapshot_server.js` and update camera configurations:

```javascript
const cameras = [
  {
    name: 'Camera_Zone_1',
    rtspUrl: 'rtsp://192.168.0.237:554/stream1',  // Your camera IP
    wsPort: 9001
  },
  // Add more cameras...
];
```

### 4. Run the Server

```bash
node jsmpeg_snapshot_server.js
```

Or for development with auto-reload:
```bash
npm run dev
```

### 5. Access Dashboard

Open browser: **http://localhost:3000**

- View live camera streams
- Trigger manual captures
- Monitor system status

---

## 📊 Using the Dataset

### Python Analysis

```python
from dataset_loader import SmartFarmDataset

# Initialize
dataset = SmartFarmDataset('./datasets')

# List available data
dates = dataset.list_dates()
print(f"Available dates: {dates}")

# Get latest sample
sample = dataset.get_sample(0)  # 0 = most recent

# Access data
timestamp = sample['timestamp']
images = sample['images']  # Dict of PIL Images
sensors = sample['sensor_data']  # Dict of sensor readings

# Visualize
dataset.visualize_sample(0)

# Analyze daily trends
dataset.analyze_daily_trends('2026-02-20')

# Export for ML training
df = dataset.export_for_ml('./ml_dataset')
```

### Load in Notebook

```python
import pandas as pd
from PIL import Image

# Load daily CSV
df = pd.read_csv('datasets/sensor_logs/2026-02-20/sensors_2026-02-20.csv')

# Load corresponding image
img = Image.open('datasets/snapshots/2026-02-20/Camera_Zone_1_2026-02-20T10-00-00.jpg')
```

---

## ⚙️ Configuration

### Capture Schedule

Edit in `jsmpeg_snapshot_server.js`:

```javascript
const SNAPSHOT_CONFIG = {
  schedule: '*/10 * * * *',  // Every 10 minutes
  baseDir: './datasets',
  imageQuality: 2,  // 2 = high quality
  imageWidth: 1280,
  imageHeight: 720
};
```

**Schedule Examples:**
- `'*/5 * * * *'` - Every 5 minutes
- `'0 * * * *'` - Every hour
- `'0 9-17 * * *'` - Every hour, 9 AM - 5 PM
- `'0 8,12,16 * * *'` - At 8 AM, 12 PM, 4 PM

### Add More Devices

1. Add device in ThingsBoard
2. Run `node tb_config_helper.js`
3. Update `TB_CONFIG.devices` array
4. Restart server

---

## 🔧 API Endpoints

### Check Status
```bash
curl http://localhost:3000/status
```

Returns:
```json
{
  "status": "running",
  "cameras": [...],
  "datasetConfig": {
    "schedule": "*/10 * * * *",
    "baseDir": "./datasets"
  }
}
```

### Trigger Manual Capture
```bash
curl http://localhost:3000/capture
```

Or click the button on the web dashboard.

---

## 📖 Files Overview

| File | Purpose |
|------|---------|
| `jsmpeg_snapshot_server.js` | Main server with snapshot + logging |
| `tb_config_helper.js` | Get ThingsBoard device IDs |
| `dataset_loader.py` | Python utilities for data analysis |
| `SETUP_GUIDE.md` | Detailed setup and troubleshooting |
| `package.json` | Node.js dependencies |

---

## 🐛 Troubleshooting

### Snapshots Not Working

```bash
# Check FFmpeg
ffmpeg -version

# Test RTSP manually
ffmpeg -rtsp_transport tcp -i "rtsp://YOUR_CAMERA_IP:554/stream1" \
  -frames:v 1 test.jpg
```

### ThingsBoard Connection Failed

```bash
# Test login
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"your@email.com","password":"yourpass"}'
```

Run `node tb_config_helper.js` to diagnose.

### No Sensor Data

1. Check devices in ThingsBoard UI → Latest Telemetry
2. Verify device names match in `TB_CONFIG`
3. Run `node tb_config_helper.js` to test telemetry access

### Permission Errors

```bash
sudo chown -R $USER:$USER ./datasets
chmod -R 755 ./datasets
```

---

## 📚 Documentation

- **[SETUP_GUIDE.md](SETUP_GUIDE.md)** - Comprehensive setup guide
- **[dataset_loader.py](dataset_loader.py)** - Python API documentation

---

## 💡 Tips

### For Research

1. **Start with short intervals** (5-10 min) to collect data quickly
2. **Adjust image quality** based on storage - quality `2` = ~500KB, `10` = ~100KB
3. **Use summaries** for quick batch processing - they link cameras + sensors
4. **Export for ML** when ready - creates clean train/val/test structure

### For Production

1. **Increase intervals** for long-term monitoring (hourly)
2. **Set up cron** for automatic cleanup of old data
3. **Add monitoring** - check disk space, log errors
4. **Backup strategy** - rsync datasets to external storage

### For Development

1. **Use manual trigger** to test without waiting for schedule
2. **Check `/status` endpoint** to verify configuration
3. **Monitor logs** for FFmpeg or ThingsBoard errors
4. **Test with one camera** first before adding all

---

## 🎓 Research Applications

This dataset structure supports:

- **Computer Vision**: Object detection, segmentation, tracking in agricultural scenes
- **Time-Series**: Sensor trend analysis, anomaly detection, forecasting
- **Multi-Modal Learning**: Fusion of camera + sensor data for predictions
- **IoT Analytics**: System performance, network reliability studies
- **Agricultural Research**: Growth monitoring, environmental correlation studies

---

## 🤝 Contributing

Suggestions for improvements:

- Add database storage (PostgreSQL/TimescaleDB)
- Implement data retention policies
- Add email/Telegram notifications
- Create dashboard with historical data
- Add GPS coordinates for outdoor deployments
- Implement automatic quality checks

---

## 📝 License

MIT - Use freely for research and projects

---

## 🙏 Acknowledgments

Built for smart farm research at Hasanuddin University & Southern Taiwan University of Science and Technology.

---

## 📞 Support

For issues:
1. Check `SETUP_GUIDE.md` troubleshooting section
2. Run `node tb_config_helper.js` to diagnose ThingsBoard
3. Review logs in terminal
4. Verify camera RTSP URLs work with FFmpeg

---

**Ready to start collecting data?**

```bash
npm install
node tb_config_helper.js  # Get your config
# Update jsmpeg_snapshot_server.js with config
node jsmpeg_snapshot_server.js
```

Then open http://localhost:3000 and watch your smart farm! 🌱
