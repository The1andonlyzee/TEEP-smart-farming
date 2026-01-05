const express = require('express');
const { spawn } = require('child_process');
const app = express();

// Camera configurations
const cameras = [
  {
    name: 'Camera_Zone_1',
    rtspUrl: 'rtsp://192.168.0.237:554/stream1',
    endpoint: '/camera1'
  },
  {
    name: 'Camera_Zone_2',
    rtspUrl: 'rtsp://192.168.0.142:554/11',
    endpoint: '/camera2'
  }
];

// CORS
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', '*');
  next();
});

// Create MJPEG endpoint for each camera
cameras.forEach(camera => {
  app.get(camera.endpoint, (req, res) => {
    console.log(`${camera.name} stream requested`);
    
    res.writeHead(200, {
      'Content-Type': 'multipart/x-mixed-replace; boundary=--myboundary',
      'Cache-Control': 'no-cache, no-store, must-revalidate',
      'Pragma': 'no-cache',
      'Connection': 'close'
    });

    const ffmpeg = spawn('ffmpeg', [
      '-rtsp_transport', 'tcp',
      '-i', camera.rtspUrl,
      '-f', 'image2pipe',
      '-vcodec', 'mjpeg',
      '-q:v', '3',           // Quality: 2-5 (2=best, 5=lower)
      '-r', '15',            // 15 FPS - good balance
      '-s', '1280x720',      // Resolution
      'pipe:1'
    ]);

    ffmpeg.stdout.on('data', (chunk) => {
      res.write('--myboundary\r\n');
      res.write('Content-Type: image/jpeg\r\n');
      res.write('Content-Length: ' + chunk.length + '\r\n\r\n');
      res.write(chunk);
    });

    ffmpeg.stderr.on('data', (data) => {
      // Suppress FFmpeg logs
    });

    ffmpeg.on('close', (code) => {
      console.log(`${camera.name} stream ended (code ${code})`);
      res.end();
    });

    // Cleanup when client disconnects
    req.on('close', () => {
      console.log(`${camera.name} client disconnected`);
      ffmpeg.kill('SIGTERM');
    });
  });
  
  console.log(`✅ ${camera.name} available at: http://localhost:3000${camera.endpoint}`);
});

// Status endpoint
app.get('/status', (req, res) => {
  const host = req.get('host').split(':')[0];
  res.json({
    status: 'running',
    cameras: cameras.map(c => ({
      name: c.name,
      url: `http://${host}:3000${c.endpoint}`
    }))
  });
});

// Test page
app.get('/', (req, res) => {
  const host = req.get('host').split(':')[0];
  
  res.send(`
    <!DOCTYPE html>
    <html>
      <head>
        <title>Smart Farm Cameras - MJPEG</title>
        <style>
          body { 
            background: #1a1a1a; 
            color: white; 
            font-family: Arial; 
            padding: 20px;
            margin: 0;
          }
          h1 { 
            text-align: center; 
            color: #4CAF50; 
          }
          .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(600px, 1fr));
            gap: 20px;
            max-width: 1600px;
            margin: 20px auto;
          }
          .camera {
            background: #2a2a2a;
            padding: 15px;
            border-radius: 8px;
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
          }
          .camera h3 {
            margin: 0 0 10px 0;
            color: #4CAF50;
          }
          .camera img {
            width: 100%;
            height: auto;
            border-radius: 4px;
            background: #000;
            min-height: 360px;
          }
          .status {
            margin-top: 10px;
            padding: 8px;
            background: #1a1a1a;
            border-radius: 4px;
            font-size: 12px;
            color: #aaa;
          }
        </style>
      </head>
      <body>
        <h1>🌱 Smart Farm Camera System (MJPEG)</h1>
        <div class="grid">
          ${cameras.map(cam => `
            <div class="camera">
              <h3>${cam.name}</h3>
              <img id="img${cam.endpoint}" src="http://${host}:3000${cam.endpoint}" 
                   alt="${cam.name}" 
                   onerror="this.style.border='2px solid red'"
                   onload="this.style.border='2px solid green'">
              <div class="status">
                Stream: http://${host}:3000${cam.endpoint}
              </div>
            </div>
          `).join('')}
        </div>
      </body>
    </html>
  `);
});

const PORT = 3000;
app.listen(PORT, () => {
  console.log('\n=================================');
  console.log('Smart Farm MJPEG Camera Server');
  console.log('=================================');
  console.log(`🌐 View cameras: http://localhost:${PORT}`);
  console.log(`📊 Status API: http://localhost:${PORT}/status`);
  console.log('\nCamera Streams:');
  cameras.forEach(c => {
    console.log(`  - ${c.name}: http://localhost:${PORT}${c.endpoint}`);
  });
  console.log('=================================\n');
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down...');
  process.exit();
});