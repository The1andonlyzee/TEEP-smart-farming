const express = require('express');
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');
const app = express();

// Camera configurations
const cameras = [
  {
    name: 'Camera_Zone_1',
    rtspUrl: 'rtsp://192.168.0.237:554/stream1',
    hlsPath: '/hls/camera1'
  },
  {
    name: 'Camera_Zone_2',
    rtspUrl: 'rtsp://192.168.0.142:554/11',
    hlsPath: '/hls/camera2'
  }
];

// Create HLS directory
const hlsDir = path.join(__dirname, 'hls');
if (!fs.existsSync(hlsDir)) {
  fs.mkdirSync(hlsDir);
}

// Serve HLS files
app.use('/hls', express.static(hlsDir));

// Enable CORS
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');
  next();
});

// Start FFmpeg for each camera to generate HLS
cameras.forEach(camera => {
  const cameraDir = path.join(hlsDir, path.basename(camera.hlsPath));
  if (!fs.existsSync(cameraDir)) {
    fs.mkdirSync(cameraDir);
  }

  const playlistPath = path.join(cameraDir, 'stream.m3u8');
  
  const ffmpeg = spawn('ffmpeg', [
    '-rtsp_transport', 'tcp',
    '-i', camera.rtspUrl,
    '-c:v', 'libx264',          // H.264 codec
    '-preset', 'ultrafast',
    '-tune', 'zerolatency',
    '-f', 'hls',                // HLS format
    '-hls_time', '2',           // 2-second segments
    '-hls_list_size', '3',      // Keep 3 segments
    '-hls_flags', 'delete_segments+append_list',
    '-hls_segment_filename', path.join(cameraDir, 'segment%03d.ts'),
    playlistPath
  ]);

  ffmpeg.stderr.on('data', (data) => {
    // Suppress output or log if needed
  });

  ffmpeg.on('close', (code) => {
    console.log(`${camera.name} FFmpeg exited with code ${code}`);
  });

  console.log(`✅ ${camera.name} streaming HLS at: /hls/${path.basename(camera.hlsPath)}/stream.m3u8`);
});

// Status endpoint
app.get('/status', (req, res) => {
  res.json({
    status: 'running',
    cameras: cameras.map(c => ({
      name: c.name,
      hlsUrl: `http://YOUR_PC_IP:3000${c.hlsPath}/stream.m3u8`
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
        <title>Smart Farm HLS Cameras</title>
        <script src="https://cdn.jsdelivr.net/npm/hls.js@latest"></script>
      </head>
      <body style="background: #1a1a1a; color: white; font-family: Arial; padding: 20px;">
        <h1>🌱 Smart Farm Camera System (HLS)</h1>
        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px;">
          ${cameras.map((cam, idx) => `
            <div style="background: #2a2a2a; padding: 15px; border-radius: 8px;">
              <h3 style="color: #4CAF50;">${cam.name}</h3>
              <video id="video${idx}" controls style="width: 100%; background: #000; border-radius: 4px;"></video>
            </div>
          `).join('')}
        </div>
        
        <script>
          const cameras = ${JSON.stringify(cameras.map((c, i) => ({
            index: i,
            hlsUrl: `http://${hostname}:3000${c.hlsPath}/stream.m3u8`
          })))};
          
          cameras.forEach(cam => {
            const video = document.getElementById('video' + cam.index);
            
            if (Hls.isSupported()) {
              const hls = new Hls();
              hls.loadSource(cam.hlsUrl);
              hls.attachMedia(video);
              hls.on(Hls.Events.MANIFEST_PARSED, () => {
                video.play();
              });
            } else if (video.canPlayType('application/vnd.apple.mpegurl')) {
              // Native HLS support (Safari)
              video.src = cam.hlsUrl;
              video.addEventListener('loadedmetadata', () => {
                video.play();
              });
            }
          });
        </script>
      </body>
    </html>
  `);
});

const PORT = 3000;
app.listen(PORT, () => {
  console.log('\n=================================');
  console.log('Smart Farm HLS Camera Server');
  console.log('=================================');
  console.log(`🌐 View cameras: http://localhost:${PORT}`);
  console.log('\nHLS Streams:');
  cameras.forEach(c => {
    console.log(`  - ${c.name}: http://localhost:${PORT}${c.hlsPath}/stream.m3u8`);
  });
  console.log('=================================\n');
});