const express = require('express');
const { spawn } = require('child_process');
const app = express();

// Camera configurations
// const cameras = [
//   {
//     name: 'Camera_Zone_1',
//     rtspUrl: 'rtsp://192.168.0.237:554/stream1',
//     endpoint: '/camera1'
//   },
//   {
//     name: 'Camera_Zone_2',
//     rtspUrl: 'rtsp://192.168.0.142:554/11',
//     endpoint: '/camera2'
//   }
// ];
const cameras = [
  {
    name: 'Camera_Zone_1',
    rtspUrl: 'rtsp://192.168.0.142:554/11',  // Swap!
    endpoint: '/camera1'
  },
  {
    name: 'Camera_Zone_2',
    rtspUrl: 'rtsp://192.168.0.237:554/stream1',  // Swap!
    endpoint: '/camera2'
  }
];

// CORS
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', '*');
  next();
});

// Track active streams
const activeStreams = new Map();

// Create MJPEG endpoint for each camera
cameras.forEach(camera => {
  app.get(camera.endpoint, (req, res) => {
    console.log(`${camera.name} stream requested`);
    
    // Set response headers
    res.writeHead(200, {
      'Content-Type': 'multipart/x-mixed-replace; boundary=--myboundary',
      'Cache-Control': 'no-cache, no-store, must-revalidate',
      'Pragma': 'no-cache',
      'Connection': 'keep-alive',
      'X-Content-Type-Options': 'nosniff'
    });

    // Disable buffering
    res.socket.setNoDelay(true);
    res.socket.setKeepAlive(true);

    const ffmpegArgs = [
      '-rtsp_transport', 'tcp',
      '-i', camera.rtspUrl,
      '-f', 'image2pipe',
      '-vcodec', 'mjpeg',
      '-q:v', '3',              // Quality (2-31, lower is better)
      '-r', '20',               // 10 FPS - lower for stability
      '-s', '640x480',          // Lower resolution for testing
      '-an',                    // No audio
      '-preset', 'ultrafast',
      'pipe:1'
    ];

    console.log(`Starting FFmpeg for ${camera.name}`);
    const ffmpeg = spawn('ffmpeg', ffmpegArgs);

    // Store reference
    activeStreams.set(camera.endpoint + req.socket.remoteAddress, ffmpeg);

    let frameCount = 0;
    let isFirstFrame = true;

    // Handle FFmpeg stdout (video frames)
    ffmpeg.stdout.on('data', (chunk) => {
      try {
        if (res.writableEnded) {
          console.log(`${camera.name} response already ended`);
          ffmpeg.kill();
          return;
        }

        // Write frame to response
        res.write('--myboundary\r\n');
        res.write('Content-Type: image/jpeg\r\n');
        res.write('Content-Length: ' + chunk.length + '\r\n\r\n');
        res.write(chunk);

        frameCount++;

        if (isFirstFrame) {
          console.log(`${camera.name} first frame sent (${chunk.length} bytes)`);
          isFirstFrame = false;
        }

        if (frameCount % 100 === 0) {
          console.log(`${camera.name} frames sent: ${frameCount}`);
        }

      } catch (error) {
        console.error(`${camera.name} write error:`, error.message);
        ffmpeg.kill();
      }
    });

    // Handle FFmpeg stderr (logs and errors)
    ffmpeg.stderr.on('data', (data) => {
      const message = data.toString();
      
      // Only log errors, not regular FFmpeg output
      if (message.includes('error') || message.includes('Error') || 
          message.includes('failed') || message.includes('Invalid')) {
        console.error(`${camera.name} FFmpeg error:`, message.substring(0, 200));
      }
    });

    // Handle FFmpeg exit
    ffmpeg.on('exit', (code, signal) => {
      console.log(`${camera.name} FFmpeg exited (code: ${code}, signal: ${signal})`);
      activeStreams.delete(camera.endpoint + req.socket.remoteAddress);
      
      if (!res.writableEnded) {
        res.end();
      }
    });

    // Handle FFmpeg errors
    ffmpeg.on('error', (error) => {
      console.error(`${camera.name} FFmpeg spawn error:`, error.message);
      if (!res.writableEnded) {
        res.end();
      }
    });

    // Cleanup when client disconnects
    req.on('close', () => {
      console.log(`${camera.name} client disconnected (${frameCount} frames sent)`);
      
      if (ffmpeg && !ffmpeg.killed) {
        ffmpeg.kill('SIGTERM');
        
        // Force kill if still alive after 2 seconds
        setTimeout(() => {
          if (!ffmpeg.killed) {
            console.log(`${camera.name} force killing FFmpeg`);
            ffmpeg.kill('SIGKILL');
          }
        }, 2000);
      }
      
      activeStreams.delete(camera.endpoint + req.socket.remoteAddress);
    });

    req.on('error', (error) => {
      console.error(`${camera.name} request error:`, error.message);
      if (ffmpeg && !ffmpeg.killed) {
        ffmpeg.kill();
      }
    });
  });
  
  console.log(`✅ ${camera.name} available at: http://localhost:3000${camera.endpoint}`);
});

// Status endpoint
app.get('/status', (req, res) => {
  const host = req.get('host').split(':')[0];
  res.json({
    status: 'running',
    activeStreams: activeStreams.size,
    cameras: cameras.map(c => ({
      name: c.name,
      url: `http://${host}:3000${c.endpoint}`
    }))
  });
});

// Test page with better error handling
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
          .camera-wrapper {
            position: relative;
            background: #000;
            border-radius: 4px;
            min-height: 360px;
          }
          .camera img {
            width: 100%;
            height: auto;
            display: block;
          }
          .status {
            margin-top: 10px;
            padding: 8px;
            background: #1a1a1a;
            border-radius: 4px;
            font-size: 12px;
            color: #aaa;
          }
          .loading {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            color: #ff9800;
            font-size: 14px;
          }
        </style>
      </head>
      <body>
        <h1>🌱 Smart Farm Camera System (MJPEG)</h1>
        <div class="grid">
          ${cameras.map((cam, idx) => `
            <div class="camera">
              <h3>${cam.name}</h3>
              <div class="camera-wrapper">
                <div class="loading" id="loading${idx}">● Loading stream...</div>
                <img id="img${idx}" 
                     data-src="http://${host}:3000${cam.endpoint}" 
                     alt="${cam.name}"
                     style="display: none;">
              </div>
              <div class="status" id="status${idx}">
                Initializing...
              </div>
            </div>
          `).join('')}
        </div>

        <script>
          ${cameras.map((cam, idx) => `
            (function() {
              const img = document.getElementById('img${idx}');
              const loading = document.getElementById('loading${idx}');
              const status = document.getElementById('status${idx}');
              const src = img.dataset.src;
              let retryCount = 0;
              const maxRetries = 3;

              function loadStream() {
                status.textContent = 'Connecting... (attempt ' + (retryCount + 1) + ')';
                img.src = src + '?t=' + Date.now();
              }

              img.onload = function() {
                loading.style.display = 'none';
                img.style.display = 'block';
                status.textContent = '● Live - Stream active';
                status.style.color = '#4CAF50';
                retryCount = 0;
                console.log('${cam.name} stream loaded');
              };

              img.onerror = function() {
                console.error('${cam.name} stream error');
                loading.textContent = '● Connection Failed';
                loading.style.color = '#f44336';
                status.textContent = '● Error - Retrying...';
                status.style.color = '#f44336';
                
                retryCount++;
                if (retryCount < maxRetries) {
                  setTimeout(loadStream, 3000);
                } else {
                  status.textContent = '● Failed after ' + maxRetries + ' attempts';
                  loading.textContent = '● Stream Unavailable';
                }
              };

              // Start loading
              loadStream();
            })();
          `).join('\n')}
        </script>
      </body>
    </html>
  `);
});

const PORT = 3000;
const server = app.listen(PORT, () => {
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

// Increase timeout
server.timeout = 0;
server.keepAliveTimeout = 0;
server.headersTimeout = 0;

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n\nShutting down gracefully...');
  
  // Kill all active FFmpeg processes
  activeStreams.forEach((ffmpeg, key) => {
    console.log(`Killing stream: ${key}`);
    ffmpeg.kill('SIGTERM');
  });
  
  server.close(() => {
    console.log('Server closed');
    process.exit(0);
  });
  
  // Force exit after 5 seconds
  setTimeout(() => {
    console.log('Forcing exit...');
    process.exit(1);
  }, 5000);
});

process.on('uncaughtException', (err) => {
  console.error('Uncaught exception:', err);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('Unhandled rejection at:', promise, 'reason:', reason);
});