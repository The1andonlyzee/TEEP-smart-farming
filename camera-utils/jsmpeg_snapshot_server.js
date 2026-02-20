const express = require('express');
const Stream = require('node-rtsp-stream');
const path = require('path');
const cron = require('node-cron');
const fs = require('fs');
const { exec } = require('child_process');
const axios = require('axios');

const app = express();

// ===================================
// CONFIGURATION
// ===================================

// ThingsBoard Configuration (for sensor data logging)
const TB_CONFIG = {
  host: 'http://localhost:8080',
  username: 'your_username@domain.com',
  password: 'your_password',
  devices: [
    {
      name: 'Arduino_Mega_Sensors',
      deviceId: 'YOUR_DEVICE_ID_HERE'
    },
    {
      name: 'NodeMCU_Sensors',
      deviceId: 'YOUR_DEVICE_ID_HERE'
    }
  ]
};

// Camera configurations
const cameras = [
  {
    name: 'Camera_Zone_1',
    rtspUrl: 'rtsp://192.168.0.237:554/stream1',
    wsPort: 9001
  },
  {
    name: 'Camera_Zone_2',
    rtspUrl: 'rtsp://192.168.0.172:554/profile1',
    wsPort: 9002
  }
];

// Schedules
const SCHEDULE_CONFIG = {
  // Time-lapse image capture (for making videos later)
  timelapseSchedule: '0 * * * *',  // Every hour at minute 0
  // Examples:
  // '0 * * * *'      - Every hour (for time-lapse)
  // '*/30 * * * *'   - Every 30 minutes
  // '0 9-17 * * *'   - Every hour from 9 AM to 5 PM
  
  // Sensor data logging (more frequent for research data)
  sensorSchedule: '*/10 * * * *',  // Every 10 minutes
  // Examples:
  // '*/10 * * * *'   - Every 10 minutes
  // '*/5 * * * *'    - Every 5 minutes
  // '*/15 * * * *'   - Every 15 minutes
  
  baseDir: './farm_data',
  
  // Time-lapse image settings
  imageQuality: 5,   // 2-31, lower = better quality (5 is good for time-lapse)
  imageWidth: 1920,  // Full HD for time-lapse
  imageHeight: 1080
};

// ===================================
// DIRECTORY SETUP
// ===================================

function ensureDirectories() {
  const dirs = [
    SCHEDULE_CONFIG.baseDir,
    path.join(SCHEDULE_CONFIG.baseDir, 'timelapse_images'),
    path.join(SCHEDULE_CONFIG.baseDir, 'sensor_logs')
  ];
  
  // Create camera-specific directories
  cameras.forEach(camera => {
    dirs.push(path.join(SCHEDULE_CONFIG.baseDir, 'timelapse_images', camera.name));
  });
  
  dirs.forEach(dir => {
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
      console.log(`📁 Created: ${dir}`);
    }
  });
}

// ===================================
// THINGSBOARD CLIENT
// ===================================

class ThingsBoardClient {
  constructor(config) {
    this.config = config;
    this.token = null;
  }

  async login() {
    try {
      const response = await axios.post(`${this.config.host}/api/auth/login`, {
        username: this.config.username,
        password: this.config.password
      });
      
      this.token = response.data.token;
      console.log('✅ ThingsBoard authenticated');
      return true;
    } catch (error) {
      console.error('❌ ThingsBoard login failed:', error.message);
      return false;
    }
  }

  async getLatestTelemetry(deviceId) {
    if (!this.token) await this.login();

    try {
      const response = await axios.get(
        `${this.config.host}/api/plugins/telemetry/DEVICE/${deviceId}/values/timeseries`,
        {
          headers: { 'X-Authorization': `Bearer ${this.token}` }
        }
      );
      return response.data;
    } catch (error) {
      if (error.response?.status === 401) {
        await this.login();
        return this.getLatestTelemetry(deviceId);
      }
      console.error(`Failed to get telemetry:`, error.message);
      return null;
    }
  }

  async getAllSensorData() {
    const allData = {};
    
    for (const device of this.config.devices) {
      const telemetry = await this.getLatestTelemetry(device.deviceId);
      
      if (telemetry) {
        allData[device.name] = {};
        for (const [key, valueArray] of Object.entries(telemetry)) {
          if (valueArray?.length > 0) {
            allData[device.name][key] = {
              value: valueArray[0].value,
              timestamp: valueArray[0].ts
            };
          }
        }
      }
    }
    
    return allData;
  }
}

const tbClient = new ThingsBoardClient(TB_CONFIG);

// ===================================
// TIME-LAPSE IMAGE CAPTURE
// ===================================

function captureTimelapseImage(camera) {
  return new Promise((resolve, reject) => {
    const timestamp = new Date();
    const timeStr = timestamp.toISOString().replace(/[:.]/g, '-').split('.')[0];
    
    // Save with sequential numbering for easy time-lapse creation
    const frameNumber = Date.now();
    const filename = `frame_${frameNumber}.jpg`;
    
    const cameraDir = path.join(SCHEDULE_CONFIG.baseDir, 'timelapse_images', camera.name);
    const filepath = path.join(cameraDir, filename);
    
    // Also save a "latest.jpg" for easy monitoring
    const latestPath = path.join(cameraDir, 'latest.jpg');
    
    const cmd = `ffmpeg -rtsp_transport tcp -i "${camera.rtspUrl}" \
      -frames:v 1 \
      -q:v ${SCHEDULE_CONFIG.imageQuality} \
      -s ${SCHEDULE_CONFIG.imageWidth}x${SCHEDULE_CONFIG.imageHeight} \
      -y "${filepath}" 2>&1`;
    
    exec(cmd, (error) => {
      if (error) {
        console.log(`   ✗ ${camera.name} failed`);
        reject(error);
      } else {
        // Copy to latest.jpg
        fs.copyFileSync(filepath, latestPath);
        
        // Save metadata
        const metaPath = filepath.replace('.jpg', '.txt');
        fs.writeFileSync(metaPath, `Captured: ${timestamp.toISOString()}\nCamera: ${camera.name}\n`);
        
        console.log(`   ✓ ${camera.name}: ${filename}`);
        resolve(filepath);
      }
    });
  });
}

async function captureAllTimelapseImages() {
  const timestamp = new Date();
  console.log(`\n📸 [${timestamp.toLocaleString()}] Capturing time-lapse frames...`);
  
  const promises = cameras.map(camera => captureTimelapseImage(camera));
  const results = await Promise.allSettled(promises);
  
  const successful = results.filter(r => r.status === 'fulfilled').length;
  console.log(`✅ Captured ${successful}/${cameras.length} frames for time-lapse\n`);
}

// ===================================
// SENSOR DATA LOGGING
// ===================================

async function logSensorData() {
  const timestamp = new Date();
  const dateStr = timestamp.toISOString().split('T')[0];
  
  console.log(`📊 [${timestamp.toLocaleString()}] Logging sensor data...`);
  
  const sensorData = await tbClient.getAllSensorData();
  
  if (Object.keys(sensorData).length === 0) {
    console.log('   ⚠️  No sensor data available\n');
    return;
  }

  const logDir = path.join(SCHEDULE_CONFIG.baseDir, 'sensor_logs');
  const csvPath = path.join(logDir, `${dateStr}.csv`);

  // Create CSV header if file doesn't exist
  if (!fs.existsSync(csvPath)) {
    const headers = ['timestamp', 'date', 'time'];
    
    for (const [deviceName, readings] of Object.entries(sensorData)) {
      for (const sensorKey of Object.keys(readings)) {
        headers.push(`${deviceName}_${sensorKey}`);
      }
    }
    
    fs.writeFileSync(csvPath, headers.join(',') + '\n');
    console.log(`   📄 Created new log file: ${dateStr}.csv`);
  }

  // Append data row
  const timeOnly = timestamp.toTimeString().split(' ')[0];
  const row = [
    timestamp.toISOString(),
    dateStr,
    timeOnly
  ];
  
  for (const [deviceName, readings] of Object.entries(sensorData)) {
    for (const data of Object.values(readings)) {
      row.push(data.value);
    }
  }

  fs.appendFileSync(csvPath, row.join(',') + '\n');
  
  // Print summary
  let totalReadings = 0;
  for (const readings of Object.values(sensorData)) {
    totalReadings += Object.keys(readings).length;
  }
  console.log(`   ✓ Logged ${totalReadings} readings to ${dateStr}.csv\n`);
}

// ===================================
// EXPRESS SERVER (STREAMING)
// ===================================

app.use(express.static(__dirname));

app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', '*');
  next();
});

// Create WebSocket streams for live monitoring
const streams = [];

console.log('\n═══════════════════════════════════════');
console.log('Starting Camera Streams...');
console.log('═══════════════════════════════════════\n');

cameras.forEach(camera => {
  try {
    const stream = new Stream({
      name: camera.name,
      streamUrl: camera.rtspUrl,
      wsPort: camera.wsPort,
      ffmpegOptions: {
        '-stats': '',
        '-rtsp_transport': 'tcp',
        '-f': 'mpegts',
        '-codec:v': 'mpeg1video',
        '-b:v': '1500k',
        '-r': '20',
        '-bf': '0',
        '-s': '1280x720',
        '-an': ''
      }
    });
    
    streams.push(stream);
    console.log(`✅ ${camera.name} streaming on ws://localhost:${camera.wsPort}`);
    
  } catch (error) {
    console.error(`❌ Failed to start ${camera.name}:`, error.message);
  }
});

// API endpoints
app.get('/status', (req, res) => {
  const host = req.get('host').split(':')[0];
  res.json({
    status: 'running',
    cameras: cameras.map(c => ({
      name: c.name,
      wsUrl: `ws://${host}:${c.wsPort}`
    })),
    schedules: {
      timelapse: SCHEDULE_CONFIG.timelapseSchedule,
      sensors: SCHEDULE_CONFIG.sensorSchedule
    },
    dataDir: SCHEDULE_CONFIG.baseDir
  });
});

app.get('/capture-now', async (req, res) => {
  try {
    await captureAllTimelapseImages();
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ success: false, error: error.message });
  }
});

app.get('/log-now', async (req, res) => {
  try {
    await logSensorData();
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ success: false, error: error.message });
  }
});

// Monitoring dashboard
app.get('/', (req, res) => {
  const hostname = req.get('host').split(':')[0];
  
  res.send(`
    <!DOCTYPE html>
    <html>
      <head>
        <title>Smart Farm Monitoring & Data Logger</title>
        <meta charset="UTF-8">
        <style>
          * { margin: 0; padding: 0; box-sizing: border-box; }
          body { 
            font-family: 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #1a1a1a 0%, #2d2d2d 100%);
            color: white;
            padding: 20px;
            min-height: 100vh;
          }
          h1 {
            text-align: center;
            margin-bottom: 10px;
            color: #4CAF50;
            font-size: 2.5em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
          }
          .controls {
            text-align: center;
            margin: 20px 0;
            padding: 20px;
            background: #2a2a2a;
            border-radius: 12px;
            border: 2px solid #3a3a3a;
          }
          .controls h3 {
            color: #4CAF50;
            margin-bottom: 15px;
          }
          button {
            background: #4CAF50;
            color: white;
            border: none;
            padding: 12px 24px;
            margin: 5px;
            font-size: 16px;
            border-radius: 6px;
            cursor: pointer;
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
          }
          button:hover { background: #45a049; }
          .schedule-info {
            margin-top: 15px;
            padding: 10px;
            background: #1a1a1a;
            border-radius: 6px;
            font-size: 14px;
          }
          .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(600px, 1fr));
            gap: 20px;
            max-width: 1600px;
            margin: 0 auto;
          }
          .camera-container {
            background: #2a2a2a;
            border-radius: 12px;
            padding: 20px;
            box-shadow: 0 8px 16px rgba(0,0,0,0.4);
            border: 2px solid #3a3a3a;
          }
          h3 {
            margin-bottom: 15px;
            color: #4CAF50;
            font-size: 1.4em;
          }
          .camera-wrapper {
            position: relative;
            width: 100%;
            padding-bottom: 56.25%;
            background: #000;
            border-radius: 8px;
            overflow: hidden;
          }
          canvas {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            object-fit: contain;
          }
          .status {
            margin-top: 15px;
            padding: 10px;
            border-radius: 6px;
            text-align: center;
            font-weight: bold;
            font-size: 14px;
          }
          .online { background: linear-gradient(135deg, #4CAF50 0%, #45a049 100%); }
          .offline { background: linear-gradient(135deg, #f44336 0%, #da190b 100%); }
          .connecting { background: linear-gradient(135deg, #ff9800 0%, #f57c00 100%); }
        </style>
      </head>
      <body>
        <h1>🌱 Smart Farm Monitor & Logger</h1>
        
        <div class="controls">
          <h3>Manual Controls</h3>
          <button onclick="captureNow()">📸 Capture Time-lapse Frame Now</button>
          <button onclick="logNow()">📊 Log Sensor Data Now</button>
          
          <div class="schedule-info">
            <strong>Automatic Schedules:</strong><br>
            📸 Time-lapse: ${SCHEDULE_CONFIG.timelapseSchedule}<br>
            📊 Sensor logging: ${SCHEDULE_CONFIG.sensorSchedule}
          </div>
        </div>
        
        <div class="grid">
          ${cameras.map((cam, idx) => `
            <div class="camera-container">
              <h3>${cam.name}</h3>
              <div class="camera-wrapper">
                <canvas id="canvas${idx}"></canvas>
              </div>
              <div class="status connecting" id="status${idx}">
                Connecting...
              </div>
            </div>
          `).join('')}
        </div>

        <script>
          function captureNow() {
            fetch('/capture-now')
              .then(res => res.json())
              .then(() => alert('✅ Time-lapse frame captured!'))
              .catch(err => alert('❌ Capture failed'));
          }
          
          function logNow() {
            fetch('/log-now')
              .then(res => res.json())
              .then(() => alert('✅ Sensor data logged!'))
              .catch(err => alert('❌ Logging failed'));
          }
        
          (function loadJSMpeg() {
            const cdns = [
              'https://cdn.jsdelivr.net/gh/phoboslab/jsmpeg@master/jsmpeg.min.js',
              'https://unpkg.com/jsmpeg@0.2.0/jsmpeg.min.js'
            ];
            
            let currentCDN = 0;
            
            function tryLoad() {
              if (currentCDN >= cdns.length) {
                console.error('Failed to load JSMpeg');
                return;
              }
              
              const script = document.createElement('script');
              script.src = cdns[currentCDN];
              script.onload = () => initPlayers();
              script.onerror = () => { currentCDN++; tryLoad(); };
              document.head.appendChild(script);
            }
            
            tryLoad();
          })();
          
          function initPlayers() {
            const cameras = ${JSON.stringify(cameras.map((c, i) => ({ 
              wsPort: c.wsPort, 
              name: c.name,
              index: i 
            })))};
            
            const hostname = window.location.hostname;

            cameras.forEach(cam => {
              const wsUrl = 'ws://' + hostname + ':' + cam.wsPort;
              const canvasId = 'canvas' + cam.index;
              const statusId = 'status' + cam.index;

              new JSMpeg.Player(wsUrl, {
                canvas: document.getElementById(canvasId),
                autoplay: true,
                audio: false,
                
                onSourceEstablished: function() {
                  document.getElementById(statusId).textContent = '● Live';
                  document.getElementById(statusId).className = 'status online';
                },
                
                onSourceCompleted: function() {
                  document.getElementById(statusId).textContent = '● Disconnected';
                  document.getElementById(statusId).className = 'status offline';
                  setTimeout(() => location.reload(), 3000);
                }
              });
            });
          }
        </script>
      </body>
    </html>
  `);
});

// ===================================
// STARTUP & SCHEDULING
// ===================================

const PORT = 3000;

async function startup() {
  ensureDirectories();
  
  console.log('\n═══════════════════════════════════════');
  console.log('Smart Farm Data Logger');
  console.log('═══════════════════════════════════════\n');
  
  // Connect to ThingsBoard
  console.log('🔐 Connecting to ThingsBoard...');
  await tbClient.login();
  
  // Schedule time-lapse captures
  console.log(`\n⏰ Scheduling time-lapse captures: ${SCHEDULE_CONFIG.timelapseSchedule}`);
  cron.schedule(SCHEDULE_CONFIG.timelapseSchedule, () => {
    captureAllTimelapseImages();
  });
  
  // Schedule sensor logging
  console.log(`⏰ Scheduling sensor logging: ${SCHEDULE_CONFIG.sensorSchedule}`);
  cron.schedule(SCHEDULE_CONFIG.sensorSchedule, () => {
    logSensorData();
  });
  
  // Start web server
  app.listen(PORT, () => {
    console.log('\n═══════════════════════════════════════');
    console.log(`🌐 Dashboard: http://localhost:${PORT}`);
    console.log(`📊 Status API: http://localhost:${PORT}/status`);
    console.log(`\n📁 Data directory: ${SCHEDULE_CONFIG.baseDir}`);
    console.log(`   - Time-lapse: ${SCHEDULE_CONFIG.baseDir}/timelapse_images/`);
    console.log(`   - Sensor logs: ${SCHEDULE_CONFIG.baseDir}/sensor_logs/`);
    console.log('═══════════════════════════════════════\n');
  });
}

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n\nShutting down...');
  streams.forEach(stream => {
    if (stream?.stop) stream.stop();
  });
  process.exit();
});

startup();
