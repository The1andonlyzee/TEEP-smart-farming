#!/usr/bin/env node

/**
 * ThingsBoard Configuration Helper
 * 
 * This script helps you get the device IDs needed for the dataset logger.
 */

const axios = require('axios');
const readline = require('readline');

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

function question(query) {
  return new Promise(resolve => rl.question(query, resolve));
}

async function getThingsBoardConfig() {
  console.log('\n╔════════════════════════════════════════════════╗');
  console.log('║   ThingsBoard Configuration Helper            ║');
  console.log('╚════════════════════════════════════════════════╝\n');
  
  // Get ThingsBoard connection details
  const host = await question('ThingsBoard URL (e.g., http://localhost:8080): ');
  const username = await question('Username/Email: ');
  const password = await question('Password: ');
  
  try {
    console.log('\n🔐 Logging in to ThingsBoard...');
    
    // Login
    const loginResponse = await axios.post(`${host}/api/auth/login`, {
      username,
      password
    });
    
    const token = loginResponse.data.token;
    console.log('✅ Login successful!\n');
    
    // Get devices
    console.log('📡 Fetching devices...\n');
    
    const devicesResponse = await axios.get(
      `${host}/api/tenant/devices?pageSize=100`,
      {
        headers: {
          'X-Authorization': `Bearer ${token}`
        }
      }
    );
    
    const devices = devicesResponse.data.data;
    
    if (devices.length === 0) {
      console.log('⚠️  No devices found in ThingsBoard.');
      console.log('   Please add devices first and run this script again.\n');
      rl.close();
      return;
    }
    
    console.log(`Found ${devices.length} device(s):\n`);
    console.log('╔═══╦══════════════════════════════════════╦══════════════════════════════════════════╗');
    console.log('║ # ║ Device Name                          ║ Device ID                                ║');
    console.log('╠═══╬══════════════════════════════════════╬══════════════════════════════════════════╣');
    
    devices.forEach((device, idx) => {
      const name = device.name.padEnd(38);
      const id = device.id.id;
      console.log(`║ ${(idx + 1).toString().padStart(2)} ║ ${name} ║ ${id} ║`);
    });
    
    console.log('╚═══╩══════════════════════════════════════╩══════════════════════════════════════════╝\n');
    
    // Generate configuration code
    console.log('📋 Copy this configuration to your jsmpeg_snapshot_server.js:\n');
    console.log('─'.repeat(80));
    console.log('\nconst TB_CONFIG = {');
    console.log(`  host: '${host}',`);
    console.log(`  username: '${username}',`);
    console.log(`  password: '${password}',`);
    console.log('  devices: [');
    
    devices.forEach((device, idx) => {
      console.log('    {');
      console.log(`      name: '${device.name}',`);
      console.log(`      deviceId: '${device.id.id}'`);
      console.log(`    }${idx < devices.length - 1 ? ',' : ''}`);
    });
    
    console.log('  ]');
    console.log('};\n');
    console.log('─'.repeat(80));
    
    // Get device credentials (optional)
    console.log('\n📌 Device Credentials (for direct MQTT/HTTP publishing):');
    console.log('─'.repeat(80));
    
    for (const device of devices) {
      try {
        const credResponse = await axios.get(
          `${host}/api/device/${device.id.id}/credentials`,
          {
            headers: {
              'X-Authorization': `Bearer ${token}`
            }
          }
        );
        
        const credentials = credResponse.data;
        console.log(`\n${device.name}:`);
        console.log(`  Device ID: ${device.id.id}`);
        console.log(`  Access Token: ${credentials.credentialsId || 'N/A'}`);
      } catch (error) {
        console.log(`\n${device.name}: Could not fetch credentials`);
      }
    }
    
    console.log('\n─'.repeat(80));
    
    // Test telemetry fetch
    console.log('\n🧪 Testing telemetry fetch...\n');
    
    for (const device of devices) {
      try {
        const telemetryResponse = await axios.get(
          `${host}/api/plugins/telemetry/DEVICE/${device.id.id}/values/timeseries`,
          {
            headers: {
              'X-Authorization': `Bearer ${token}`
            }
          }
        );
        
        const telemetry = telemetryResponse.data;
        const keys = Object.keys(telemetry);
        
        console.log(`${device.name}:`);
        if (keys.length > 0) {
          console.log(`  ✅ ${keys.length} telemetry key(s): ${keys.join(', ')}`);
          
          // Show latest values
          keys.slice(0, 3).forEach(key => {
            if (telemetry[key] && telemetry[key].length > 0) {
              const latest = telemetry[key][0];
              console.log(`     - ${key}: ${latest.value} (${new Date(latest.ts).toISOString()})`);
            }
          });
        } else {
          console.log('  ⚠️  No telemetry data found');
        }
        console.log();
      } catch (error) {
        console.log(`${device.name}: Could not fetch telemetry`);
        console.log();
      }
    }
    
    console.log('✅ Configuration complete!\n');
    
  } catch (error) {
    console.error('\n❌ Error:', error.message);
    
    if (error.response) {
      console.error('Status:', error.response.status);
      console.error('Details:', error.response.data);
    }
    
    console.log('\nTroubleshooting:');
    console.log('  1. Check ThingsBoard URL is correct');
    console.log('  2. Verify username/password');
    console.log('  3. Ensure ThingsBoard is running');
    console.log('  4. Check network connectivity\n');
  }
  
  rl.close();
}

// Run
getThingsBoardConfig().catch(console.error);
