# Smart Farm Data Logger - Usage Guide

Simple system for:
1. **Time-lapse images** - Capture frames periodically for making time-lapse videos
2. **Sensor data logging** - Log sensor readings to CSV files

---

## 🚀 Quick Start

### 1. Install Dependencies
```bash
npm install
```

### 2. Get ThingsBoard Device IDs
```bash
node tb_config_helper.js
```

Copy the generated config into `smart_farm_logger.js` at the `TB_CONFIG` section.

### 3. Configure Schedules

Edit `smart_farm_logger.js`:

```javascript
const SCHEDULE_CONFIG = {
  // Time-lapse: Every hour (for making videos)
  timelapseSchedule: '0 * * * *',
  
  // Sensor logging: Every 10 minutes (for research data)
  sensorSchedule: '*/10 * * * *',
  
  // Other settings...
};
```

**Common Schedules:**
```javascript
// Time-lapse options:
'0 * * * *'       // Every hour (24 frames/day)
'*/30 * * * *'    // Every 30 minutes (48 frames/day)
'0 */2 * * *'     // Every 2 hours (12 frames/day)

// Sensor logging options:
'*/10 * * * *'    // Every 10 minutes (144 readings/day)
'*/5 * * * *'     // Every 5 minutes (288 readings/day)
'*/15 * * * *'    // Every 15 minutes (96 readings/day)
```

### 4. Run
```bash
node smart_farm_logger.js
```

### 5. Monitor
Open: **http://localhost:3000**

---

## 📁 Output Structure

```
farm_data/
├── timelapse_images/
│   ├── Camera_Zone_1/
│   │   ├── frame_1708502400000.jpg  (timestamped frames)
│   │   ├── frame_1708502400000.txt  (metadata)
│   │   ├── frame_1708506000000.jpg
│   │   └── latest.jpg               (most recent frame)
│   └── Camera_Zone_2/
│       └── ...
│
└── sensor_logs/
    ├── 2026-02-20.csv  (one file per day)
    ├── 2026-02-21.csv
    └── ...
```

---

## 🎬 Creating Time-lapse Videos

### Using FFmpeg (Recommended)

**Basic time-lapse:**
```bash
cd farm_data/timelapse_images/Camera_Zone_1

# Create time-lapse at 24 fps (1 second = 24 hours if 1 frame/hour)
ffmpeg -pattern_type glob -i 'frame_*.jpg' \
  -r 24 \
  -vf "scale=1920:1080" \
  -c:v libx264 \
  -pix_fmt yuv420p \
  timelapse_zone1.mp4
```

**With date overlay:**
```bash
ffmpeg -pattern_type glob -i 'frame_*.jpg' \
  -vf "scale=1920:1080, \
       drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf:\
       textfile=frame_%d.txt:fontsize=48:fontcolor=white:x=10:y=10" \
  -r 24 \
  -c:v libx264 \
  timelapse_with_date.mp4
```

**High quality:**
```bash
ffmpeg -pattern_type glob -i 'frame_*.jpg' \
  -r 24 \
  -c:v libx264 \
  -preset slow \
  -crf 18 \
  timelapse_hq.mp4
```

**Frame rate guide:**
- `12 fps` = 1 second video = 12 hours (if capturing every hour)
- `24 fps` = 1 second video = 24 hours (if capturing every hour)
- `30 fps` = 1 second video = 30 hours (if capturing every hour)

### Using Python (Optional)

```python
import cv2
import glob
from pathlib import Path

def create_timelapse(input_dir, output_file, fps=24):
    """Create time-lapse video from images."""
    
    # Get all frame images
    images = sorted(glob.glob(f"{input_dir}/frame_*.jpg"))
    
    if not images:
        print("No images found!")
        return
    
    # Read first image to get dimensions
    first = cv2.imread(images[0])
    height, width = first.shape[:2]
    
    # Create video writer
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out = cv2.VideoWriter(output_file, fourcc, fps, (width, height))
    
    print(f"Creating time-lapse from {len(images)} frames...")
    
    for img_path in images:
        frame = cv2.imread(img_path)
        out.write(frame)
    
    out.release()
    print(f"✅ Time-lapse saved: {output_file}")

# Usage
create_timelapse(
    'farm_data/timelapse_images/Camera_Zone_1',
    'timelapse_zone1.mp4',
    fps=24
)
```

---

## 📊 Analyzing Sensor Data

### Load in Python

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load sensor data for a specific date
df = pd.read_csv('farm_data/sensor_logs/2026-02-20.csv')

# Parse timestamp
df['timestamp'] = pd.to_datetime(df['timestamp'])

# Display first few rows
print(df.head())
print(f"\nTotal readings: {len(df)}")
print(f"Time range: {df['timestamp'].min()} to {df['timestamp'].max()}")
```

### Plotting Trends

```python
# Plot moisture readings
plt.figure(figsize=(12, 6))

plt.subplot(2, 1, 1)
plt.plot(df['timestamp'], df['Arduino_Mega_Sensors_soil_moisture_1'], 
         marker='o', label='Moisture Sensor 1')
plt.plot(df['timestamp'], df['Arduino_Mega_Sensors_soil_moisture_2'], 
         marker='o', label='Moisture Sensor 2')
plt.ylabel('Soil Moisture')
plt.legend()
plt.grid(True, alpha=0.3)

plt.subplot(2, 1, 2)
plt.plot(df['timestamp'], df['NodeMCU_Sensors_temperature'], 
         marker='o', color='red', label='Temperature')
plt.plot(df['timestamp'], df['NodeMCU_Sensors_humidity'], 
         marker='o', color='blue', label='Humidity')
plt.xlabel('Time')
plt.ylabel('Value')
plt.legend()
plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('sensor_trends.png', dpi=300)
plt.show()
```

### Multi-day Analysis

```python
import glob

# Load all CSV files
all_files = sorted(glob.glob('farm_data/sensor_logs/*.csv'))

# Combine into single DataFrame
df_list = []
for file in all_files:
    df = pd.read_csv(file)
    df_list.append(df)

df_all = pd.concat(df_list, ignore_index=True)
df_all['timestamp'] = pd.to_datetime(df_all['timestamp'])

print(f"Total readings: {len(df_all)}")
print(f"Date range: {df_all['date'].min()} to {df_all['date'].max()}")

# Daily statistics
daily_stats = df_all.groupby('date').agg({
    'Arduino_Mega_Sensors_soil_moisture_1': ['mean', 'min', 'max'],
    'NodeMCU_Sensors_temperature': ['mean', 'min', 'max']
})

print("\nDaily Statistics:")
print(daily_stats)
```

### Export for Research

```python
# Calculate hourly averages
df_all['hour'] = pd.to_datetime(df_all['timestamp']).dt.floor('H')

hourly = df_all.groupby('hour').mean(numeric_only=True)
hourly.to_csv('hourly_averages.csv')

print("✅ Exported hourly averages")
```

---

## 🎥 Time-lapse Examples

### Example 1: Plant Growth (24 hours)
- **Capture interval**: Every 1 hour
- **Duration**: 24 hours
- **Frames**: 24 images
- **Video settings**: 24 fps → 1 second video

### Example 2: Weekly Growth
- **Capture interval**: Every 2 hours
- **Duration**: 7 days
- **Frames**: 84 images
- **Video settings**: 24 fps → 3.5 second video

### Example 3: Monthly Overview
- **Capture interval**: Every 6 hours
- **Duration**: 30 days
- **Frames**: 120 images
- **Video settings**: 30 fps → 4 second video

---

## 💡 Tips

### For Time-lapse Videos

1. **Consistent lighting** - Best results during daylight hours only
   - Schedule: `'0 8-18 * * *'` (every hour, 8 AM - 6 PM)
   
2. **Camera placement** - Keep cameras stationary

3. **Frame rate calculation**:
   - Total frames ÷ Desired video duration (sec) = FPS
   - Example: 100 frames ÷ 5 seconds = 20 fps

4. **Storage estimate**:
   - 1 frame/hour @ 500KB = ~12 MB/day
   - 1 frame/hour @ 200KB = ~5 MB/day

### For Sensor Logging

1. **Choose appropriate interval**:
   - Fast-changing (temperature): Every 5-10 minutes
   - Slow-changing (soil moisture): Every 10-15 minutes

2. **CSV organization**:
   - One file per day keeps files manageable
   - Easy to archive old data

3. **Backup regularly**:
   ```bash
   # Compress and backup weekly
   tar -czf backup_$(date +%Y%m%d).tar.gz farm_data/
   ```

---

## 🔧 Manual Controls

### Via Web Interface
Open http://localhost:3000 and use buttons to:
- Capture time-lapse frame immediately
- Log sensor data immediately

### Via API
```bash
# Capture time-lapse frame now
curl http://localhost:3000/capture-now

# Log sensor data now
curl http://localhost:3000/log-now

# Check status
curl http://localhost:3000/status
```

---

## 📈 Research Workflow

### Daily Routine
1. Check live camera streams on dashboard
2. Download yesterday's CSV for quick analysis
3. Verify images are being captured

### Weekly Routine
1. Create time-lapse video from past week
2. Analyze sensor trends
3. Archive old data
4. Check disk space

### Monthly Routine
1. Create monthly time-lapse compilation
2. Generate statistical reports
3. Backup all data to external storage

---

## 🐛 Troubleshooting

### No Images Captured

```bash
# Test FFmpeg manually
ffmpeg -rtsp_transport tcp -i "rtsp://YOUR_CAMERA_IP:554/stream1" \
  -frames:v 1 test.jpg

# Check if FFmpeg is installed
ffmpeg -version
```

### No Sensor Data

1. Run config helper: `node tb_config_helper.js`
2. Check ThingsBoard UI → Devices → Latest Telemetry
3. Verify device IDs in config match

### CSV Missing Columns

Delete the CSV file and let the system recreate it with correct headers on next logging cycle.

---

## 📝 Example Data Analysis Script

Save as `analyze_farm_data.py`:

```python
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime

# Load today's data
today = datetime.now().strftime('%Y-%m-%d')
df = pd.read_csv(f'farm_data/sensor_logs/{today}.csv')
df['timestamp'] = pd.to_datetime(df['timestamp'])

# Create report
fig, axes = plt.subplots(3, 1, figsize=(12, 10))

# Soil moisture
axes[0].plot(df['timestamp'], df['Arduino_Mega_Sensors_soil_moisture_1'])
axes[0].set_title('Soil Moisture')
axes[0].set_ylabel('Moisture Level')
axes[0].grid(True)

# Temperature
axes[1].plot(df['timestamp'], df['NodeMCU_Sensors_temperature'], color='red')
axes[1].set_title('Temperature')
axes[1].set_ylabel('°C')
axes[1].grid(True)

# Humidity
axes[2].plot(df['timestamp'], df['NodeMCU_Sensors_humidity'], color='blue')
axes[2].set_title('Humidity')
axes[2].set_ylabel('%')
axes[2].grid(True)

plt.tight_layout()
plt.savefig(f'farm_report_{today}.png', dpi=300)
print(f"✅ Report saved: farm_report_{today}.png")
```

Run with: `python analyze_farm_data.py`

---

**Ready to start?**

```bash
npm install
node tb_config_helper.js
# Update config in smart_farm_logger.js
node smart_farm_logger.js
```

Then visit http://localhost:3000 to monitor your farm! 🌱
