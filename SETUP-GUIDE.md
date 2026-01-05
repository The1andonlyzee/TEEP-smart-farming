# Smart Farm Camera System - Setup Guide

## Option 1: JSMpeg (Recommended for Quality)

### Installation
```bash
npm install express node-rtsp-stream
```

**Download JSMpeg manually:**
```bash
# Download JSMpeg to your project folder
curl -o jsmpeg.min.js https://cdn.jsdelivr.net/gh/phoboslab/jsmpeg@master/jsmpeg.min.js
```

Or download from: https://github.com/phoboslab/jsmpeg/blob/master/jsmpeg.min.js
(Right-click "Raw" → Save as `jsmpeg.min.js` in your camera-utils folder)

### Run Server
```bash
node camera-server-jsmpeg.js
```

### ThingsBoard Widgets
- Single camera: `thingsboard-widget-jsmpeg-single.html`
- Multi-camera: `thingsboard-widget-jsmpeg-multi.html`

### Advantages
✅ Better compression (lower bandwidth)
✅ Smooth playback
✅ WebGL acceleration support

### Disadvantages
❌ Requires JSMpeg library loading
❌ More complex setup

---

## Option 2: MJPEG (Recommended for Simplicity)

### Installation
```bash
npm install express
```

### Run Server
```bash
node camera-server-mjpeg.js
```

### ThingsBoard Widgets
- Single camera: `thingsboard-widget-mjpeg-single.html`
- Multi-camera: `thingsboard-widget-mjpeg-multi.html`

### Advantages
✅ Dead simple - just `<img>` tag
✅ Works everywhere
✅ No external libraries
✅ Easy debugging

### Disadvantages
❌ Higher bandwidth (2-4 Mbps per camera)
❌ Less efficient compression

---

## Configuration

### Update Camera URLs
In both server files, edit:
```javascript
const cameras = [
  {
    name: 'Camera_Zone_1',
    rtspUrl: 'rtsp://192.168.0.237:554/stream1',  // ⚠️ Change this
    // ...
  }
];
```

### Update ThingsBoard Widgets
In all widget files, change:
```javascript
var PC_IP = '192.168.0.100';  // ⚠️ Your PC's IP address
```

### Find Your PC IP
**Windows:**
```cmd
ipconfig
```
Look for "IPv4 Address" (e.g., 192.168.0.100)

**Linux:**
```bash
ip addr show
```

---

## Testing

1. **Test server locally:**
   ```
   http://localhost:3000
   ```

2. **Test from another device:**
   ```
   http://YOUR_PC_IP:3000
   ```

3. **Verify JSMpeg (if using JSMpeg):**
   ```
   http://YOUR_PC_IP:3000/jsmpeg/jsmpeg.min.js
   ```

4. **Check status:**
   ```
   http://YOUR_PC_IP:3000/status
   ```

---

## Troubleshooting

### FFmpeg not found
```bash
# Windows: Download from https://ffmpeg.org/download.html
# Add to PATH

# Verify installation
ffmpeg -version
```

### Port already in use
```bash
# Find what's using the port
netstat -ano | findstr "3000"
netstat -ano | findstr "9001"
netstat -ano | findstr "9002"

# Kill the process or change ports in server.js
```

### Firewall blocking
```powershell
# Run PowerShell as Administrator
New-NetFirewallRule -DisplayName "Node 3000" -Direction Inbound -LocalPort 3000 -Protocol TCP -Action Allow
New-NetFirewallRule -DisplayName "WebSocket 9001" -Direction Inbound -LocalPort 9001 -Protocol TCP -Action Allow
New-NetFirewallRule -DisplayName "WebSocket 9002" -Direction Inbound -LocalPort 9002 -Protocol TCP -Action Allow
```

### Camera connection issues
Test RTSP directly:
```bash
ffplay -rtsp_transport tcp rtsp://192.168.0.237:554/stream1
```

---

## Bandwidth Usage

**MJPEG (1280x720, 20 FPS):**
- 1 camera: ~3 Mbps
- 2 cameras: ~6 Mbps
- 4 cameras: ~12 Mbps

**JSMpeg (1280x720, 20 FPS):**
- 1 camera: ~1.5 Mbps
- 2 cameras: ~3 Mbps
- 4 cameras: ~6 Mbps

Your 100 Mbps network can handle both easily.

---

## Recommendation

**For your use case (local farm monitoring, 2-4 cameras):**

Use **MJPEG** because:
- Simplest setup
- Most reliable
- Easy to debug
- Your bandwidth is plenty

Use **JSMpeg** if:
- You need lower bandwidth
- You want remote access over internet
- You have 10+ cameras
