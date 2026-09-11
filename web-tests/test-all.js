const { spawn } = require('child_process');
const path = require('path');

const suites = [
  { name: 'System & Feature Management E2E', script: 'test-runner.js' },
  { name: 'Radar & Presence 2D Studio E2E', script: 'test-radar-studio.js' },
  { name: 'AquaDance Musical 2D Fountain E2E', script: 'test-aquadance.js' },
  { name: 'Irrigation Multi-Zone Scheduler E2E', script: 'test-irrigation.js' },
  { name: 'Diagnostics, Sparkline & Backups E2E', script: 'test-diagnostics.js' },
  { name: 'Network Resiliency & Reconnect E2E', script: 'test-resiliency.js' }
];

async function runSuite(suite) {
  return new Promise((resolve, reject) => {
    console.log(`\n================================================================`);
    console.log(`>>> RUNNING TEST SUITE: ${suite.name} (${suite.script})`);
    console.log(`================================================================`);

    const child = spawn('node', [path.join(__dirname, suite.script)], {
      stdio: 'inherit',
      shell: process.platform === 'win32'
    });

    child.on('close', (code) => {
      if (code === 0) {
        resolve();
      } else {
        reject(new Error(`Suite ${suite.name} failed with exit code ${code}`));
      }
    });
  });
}

async function runAll() {
  const startTime = Date.now();
  console.log(`Starting EasyIot Web E2E Multi-Device Emulation Test Suite...\n`);

  for (const suite of suites) {
    try {
      await runSuite(suite);
    } catch (err) {
      console.error(`\n[FATAL] ${err.message}`);
      process.exit(1);
    }
  }

  const durationSec = ((Date.now() - startTime) / 1000).toFixed(1);
  console.log(`\n================================================================`);
  console.log(`🎉 ALL ${suites.length} E2E TEST SUITES PASSED SUCCESSFULLY in ${durationSec}s!`);
  console.log(`Tested across Desktop, Pixel 5 (Android), iPhone 13 (iOS), and iPad Pro (Tablet).`);
  console.log(`================================================================\n`);
}

runAll();
