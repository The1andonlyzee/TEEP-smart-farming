const express = require('express');
const Stream = require('node-rtsp-stream');
const app = express();

// Camera configurations
const cameras = [
  {
    name: 'Camera_Zone_1',
    rtspUrl: 'rtsp://192.168.0.237:554/stream1',
    wsPort: 9001
  },
  {
    name: 'Camera_Zone_2',
    rtspUrl: 'rtsp://192.168.0.142:554/11',
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
        '-r': '20',
        '-s': '1280x720',
        '-q:v': '5',
        '-b:v': '1000k',
        '-bf': '0',
        '-f': 'mpegts',
        '-codec:v': 'mpeg1video',
        '-an': ''
      }
    });
    
    streams.push(stream);
    console.log(`✅ ${camera.name} streaming configured on port ${camera.wsPort}`);
    
  } catch (error) {
    console.error(`❌ Failed to start ${camera.name}:`, error.message);
  }
});

// CORS middleware
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');
  next();
});

// Health check endpoint
app.get('/status', (req, res) => {
  const status = cameras.map((c, idx) => ({
    name: c.name,
    wsPort: c.wsPort,
    wsUrl: `ws://${req.get('host').split(':')[0]}:${c.wsPort}`,
    rtsp: c.rtspUrl.replace(/:[^:]*@/, ':****@')
  }));
  
  res.json({
    status: 'running',
    cameras: status
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
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
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
            margin-bottom: 30px;
            color: #4CAF50;
            font-size: 2.5em;
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
          .error { background: linear-gradient(135deg, #9c27b0 0%, #7b1fa2 100%); }
          .info {
            margin-top: 10px;
            padding: 10px;
            background: #1a1a1a;
            border-radius: 6px;
            font-size: 12px;
            color: #aaa;
          }
          .info-row {
            display: flex;
            justify-content: space-between;
            margin: 5px 0;
          }
          .debug-log {
            margin-top: 10px;
            padding: 10px;
            background: #0a0a0a;
            border-radius: 6px;
            font-size: 11px;
            color: #888;
            max-height: 100px;
            overflow-y: auto;
            font-family: 'Courier New', monospace;
          }
        </style>
      </head>
      <body>
        <h1>🌱 Smart Farm Camera System</h1>
        <div class="grid">
          ${cameras.map((cam, idx) => `
            <div class="camera-container">
              <h3>${cam.name}</h3>
              <div class="camera-wrapper">
                <canvas id="canvas${idx}"></canvas>
              </div>
              <div class="status connecting" id="status${idx}">
                Loading player...
              </div>
              <div class="info">
                <div class="info-row">
                  <span>Stream:</span>
                  <span>ws://${hostname}:${cam.wsPort}</span>
                </div>
                <div class="info-row">
                  <span>RTSP:</span>
                  <span>${cam.rtspUrl.replace(/:[^:]*@/, ':****@')}</span>
                </div>
              </div>
              <div class="debug-log" id="log${idx}">
                Initializing...
              </div>
            </div>
          `).join('')}
        </div>

        <!-- Load JSMpeg from multiple sources as fallback -->
        <script>
          // Try multiple CDN sources
          function loadJSMpeg() {
            const sources = [
              'https://unpkg.com/jsmpeg@0.2.0/jsmpeg.min.js',
              'https://cdn.jsdelivr.net/gh/phoboslab/jsmpeg@master/jsmpeg.min.js',
              'https://cdn.jsdelivr.net/npm/jsmpeg@0.2.0/jsmpeg.min.js'
            ];
            
            let sourceIndex = 0;
            
            function tryLoad() {
              if (sourceIndex >= sources.length) {
                console.error('Failed to load JSMpeg from all sources');
                document.querySelectorAll('.status').forEach(el => {
                  el.textContent = '❌ Failed to load video player library';
                  el.className = 'status error';
                });
                return;
              }
              
              const script = document.createElement('script');
              script.src = sources[sourceIndex];
              script.onload = function() {
                console.log('✅ JSMpeg loaded from:', sources[sourceIndex]);
                initializePlayers();
              };
              script.onerror = function() {
                console.warn('Failed to load from:', sources[sourceIndex]);
                sourceIndex++;
                tryLoad();
              };
              document.head.appendChild(script);
            }
            
            tryLoad();
          }
          
          function initializePlayers() {
            const cameras = ${JSON.stringify(cameras.map((c, i) => ({ 
              wsPort: c.wsPort, 
              name: c.name,
              index: i 
            })))};
            
            const hostname = window.location.hostname;
            const players = {};

            function log(index, message) {
              const logEl = document.getElementById('log' + index);
              const time = new Date().toLocaleTimeString();
              logEl.innerHTML = time + ': ' + message + '<br>' + logEl.innerHTML;
              console.log('[' + cameras[index].name + '] ' + message);
            }

            function updateStatus(index, status, message) {
              const statusEl = document.getElementById('status' + index);
              statusEl.textContent = message;
              statusEl.className = 'status ' + status;
            }

            cameras.forEach(cam => {
              const wsUrl = 'ws://' + hostname + ':' + cam.wsPort;
              const canvasId = 'canvas' + cam.index;

              log(cam.index, 'Connecting to: ' + wsUrl);
              updateStatus(cam.index, 'connecting', 'Connecting to camera...');

              try {
                players[cam.index] = new JSMpeg.Player(wsUrl, {
                  canvas: document.getElementById(canvasId),
                  autoplay: true,
                  audio: false,
                  videoBufferSize: 512 * 1024,
                  
                  onSourceEstablished: function() {
                    log(cam.index, '✅ Stream established!');
                    updateStatus(cam.index, 'online', '● Online - Streaming');
                  },
                  
                  onSourceCompleted: function() {
                    log(cam.index, '❌ Stream disconnected');
                    updateStatus(cam.index, 'offline', '● Offline - Reconnecting...');
                    
                    setTimeout(() => {
                      location.reload();
                    }, 5000);
                  }
                });

                // Check connection after 3 seconds
                setTimeout(() => {
                  if (players[cam.index].client) {
                    const readyState = players[cam.index].client.readyState;
                    if (readyState === WebSocket.CLOSED || readyState === WebSocket.CLOSING) {
                      log(cam.index, '❌ WebSocket closed - Port ' + cam.wsPort + ' not accessible');
                      updateStatus(cam.index, 'error', '● Connection Failed - Check server');
                    }
                  }
                }, 3000);

              } catch (error) {
                log(cam.index, '❌ Error: ' + error.message);
                updateStatus(cam.index, 'error', '● Error - ' + error.message);
              }
            });
          }
          
          // Start loading
          loadJSMpeg();
        </script>
      </body>
    </html>
  `);
});

const PORT = 3000;
app.listen(PORT, () => {
  console.log('\n=================================');
  console.log('Smart Farm Camera Proxy Server');
  console.log('=================================');
  console.log(`🌐 View cameras: http://localhost:${PORT}`);
  console.log(`📊 Status API: http://localhost:${PORT}/status`);
  console.log('\nWebSocket Streams:');
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