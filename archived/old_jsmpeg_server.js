const express = require('express');
const Stream = require('node-rtsp-stream');
const path = require('path');
const app = express();

// Serve static files (if you manually download jsmpeg.min.js)
app.use(express.static(__dirname));

// CORS
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', '*');
  next();
});

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

// Create WebSocket streams for each camera
const streams = [];

console.log('\n=================================');
console.log('Initializing Camera Streams...');
console.log('=================================\n');

cameras.forEach(camera => {
  try {
    console.log(`Starting ${camera.name}...`);
    
    const stream = new Stream({
      name: camera.name,
      streamUrl: camera.rtspUrl,
      wsPort: camera.wsPort,
      ffmpegOptions: {
        '-stats': '',
        '-rtsp_transport': 'tcp',
        
        // Video encoding
        '-f': 'mpegts',
        '-codec:v': 'mpeg1video',
        '-b:v': '1500k',
        '-r': '20',
        '-bf': '0',
        '-s': '1280x720',
        
        // No audio
        '-an': ''
      }
    });
    
    streams.push(stream);
    console.log(`✅ ${camera.name} streaming on ws://localhost:${camera.wsPort}`);
    
  } catch (error) {
    console.error(`❌ Failed to start ${camera.name}:`, error.message);
  }
});

// Status endpoint
app.get('/status', (req, res) => {
  const host = req.get('host').split(':')[0];
  res.json({
    status: 'running',
    jsmpegUrl: `http://${host}:3000/jsmpeg.min.js`,
    cameras: cameras.map(c => ({
      name: c.name,
      wsUrl: `ws://${host}:${c.wsPort}`,
      wsPort: c.wsPort
    }))
  });
});

// Test page
app.get('/', (req, res) => {
  const hostname = req.get('host').split(':')[0];
  
  res.send(`
    <!DOCTYPE html>
    <html>
      <head>
        <title>Smart Farm Cameras</title>
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
            margin-bottom: 15px;
            color: #4CAF50;
            font-size: 2.5em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
          }
          h2 {
            text-align: center;
            margin-bottom: 15px;
            color: #4CAF50;
            font-size: 2em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
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
          .info {
            margin-top: 10px;
            padding: 10px;
            background: #1a1a1a;
            border-radius: 6px;
            font-size: 12px;
            color: #aaa;
          }
        </style>
      </head>
      <body>
        <h1>🌱 Smart Farm Camera System</h1>
        <h2> vibe coded </h2>
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
              <div class="info">
                Stream: ws://${hostname}:${cam.wsPort}
              </div>
            </div>
          `).join('')}
        </div>

        <script>
          // Load JSMpeg with CDN fallbacks
          (function loadJSMpeg() {
            const cdns = [
              'https://cdn.jsdelivr.net/gh/phoboslab/jsmpeg@master/jsmpeg.min.js',
              'https://unpkg.com/jsmpeg@0.2.0/jsmpeg.min.js',
              'static/jsmpeg.min.js'  // Local fallback
            ];
            
            let currentCDN = 0;
            
            function tryLoad() {
              if (currentCDN >= cdns.length) {
                console.error('Failed to load JSMpeg from all sources');
                return;
              }
              
              const script = document.createElement('script');
              script.src = cdns[currentCDN];
              script.onload = function() {
                console.log('JSMpeg loaded from:', cdns[currentCDN]);
                initPlayers();
              };
              script.onerror = function() {
                console.warn('Failed to load from:', cdns[currentCDN]);
                currentCDN++;
                tryLoad();
              };
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
          const players = {};

          cameras.forEach(cam => {
            const wsUrl = 'ws://' + hostname + ':' + cam.wsPort;
            const canvasId = 'canvas' + cam.index;
            const statusId = 'status' + cam.index;

            console.log('Connecting to:', wsUrl);

            players[cam.index] = new JSMpeg.Player(wsUrl, {
              canvas: document.getElementById(canvasId),
              autoplay: true,
              audio: false,
              videoBufferSize: 512 * 1024,
              disableGl: false,
              
              onSourceEstablished: function() {
                console.log(cam.name + ' connected');
                const status = document.getElementById(statusId);
                status.textContent = '● Live';
                status.className = 'status online';
              },
              
              onSourceCompleted: function() {
                console.log(cam.name + ' disconnected');
                const status = document.getElementById(statusId);
                status.textContent = '● Disconnected';
                status.className = 'status offline';
                
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

const PORT = 3000;
app.listen(PORT, () => {
  console.log('\n=================================');
  console.log('Smart Farm Camera Server');
  console.log('=================================');
  console.log(`🌐 View cameras: http://localhost:${PORT}`);
  console.log(`📚 JSMpeg: http://localhost:${PORT}/jsmpeg.min.js`);
  console.log(`📊 Status: http://localhost:${PORT}/status`);
  console.log('\n📹 WebSocket Streams:');
  cameras.forEach(c => {
    console.log(`  - ${c.name}: ws://localhost:${c.wsPort}`);
  });
  console.log('=================================\n');
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n\nShutting down...');
  streams.forEach(stream => {
    if (stream && stream.stop) stream.stop();
  });
  process.exit();
});