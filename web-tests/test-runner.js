const { chromium, devices } = require('playwright');
const { startServer, stopServer } = require('./mock-server');

// Helper to fetch request logs from mock server
async function getLogs() {
  const res = await fetch('http://localhost:3000/test/logs');
  return res.json();
}

async function resetLogs() {
  await fetch('http://localhost:3000/test/reset-logs', { method: 'POST' });
}

async function resetAll() {
  await fetch('http://localhost:3000/test/reset-all', { method: 'POST' });
}

async function runTests() {
  let browser;
  try {
    // 1. Start Mock Server
    const url = await startServer(3000);

    // 2. Launch headless browser
    browser = await chromium.launch({ headless: true });

    // Devices list to test (Desktop, iPhone, and Android)
    const devicesToTest = [
      { name: 'Desktop Chrome', use: {} },
      { name: 'iPhone 13 (iOS)', use: devices['iPhone 13'] },
      { name: 'Pixel 5 (Android)', use: devices['Pixel 5'] }
    ];

    for (const dev of devicesToTest) {
      console.log(`\n[TEST] Running E2E Suite on device: ${dev.name}...`);
      
      // Reset the mock server state before each run
      await resetAll();
      
      // Create a context emulating the device configurations (viewport, user agent)
      const context = await browser.newContext(dev.use);
      const page = await context.newPage();
      page.on('console', msg => console.log(`    [BROWSER CONSOLE] ${msg.text()}`));
      page.on('pageerror', err => console.error(`    [BROWSER ERROR] ${err.message}`));

      console.log(`  -> Navigating to Web Panel on ${dev.name}...`);
      await page.goto(url);
      await page.waitForLoadState('load');
      await page.waitForSelector('button[data-view="system"]', { timeout: 10000 });

      // SCENARIO 1: Verify Initial Config Load
      console.log('  -> Verifying initial configuration populated in UI...');
      await page.click('button[data-view="system"]'); // Switch to system tab
      const nodeIdVal = await page.inputValue('#s-nodeId');
      
      await page.click('button[data-system-view="network"]'); // Switch to network sub-tab
      const wifiSSIDVal = await page.inputValue('#s-ssid');
      const mqttIpVal = await page.inputValue('#s-mqttHost');
      
      if (nodeIdVal !== 'test-node' || wifiSSIDVal !== 'Home_WiFi' || mqttIpVal !== '192.168.1.10') {
        throw new Error(`Config mismatch in UI on ${dev.name}: nodeId=${nodeIdVal}, wifiSSID=${wifiSSIDVal}, mqttIp=${mqttIpVal}`);
      }

      // SCENARIO 2: Change Config and Save
      console.log('  -> Modifying System Configuration...');
      const safeNodeId = `Living-Room-${dev.name.replace(/[^a-zA-Z0-9]/g, '-')}`;
      
      await page.click('button[data-system-view="general"]');
      await page.fill('#s-nodeId', safeNodeId);
      
      await page.click('button[data-system-view="network"]');
      await page.fill('#s-ssid', 'My_New_WiFi');
      await page.fill('#s-mqttHost', '192.168.1.50');
      
      await resetLogs();
      console.log('  -> Saving changes...');
      await page.click('#save-btn');
      await page.waitForTimeout(500); // Wait for request to complete

      const saveLogs = await getLogs();
      const configSaveRequest = saveLogs.find(l => l.path === '/config' && l.method === 'POST');
      if (!configSaveRequest) {
        throw new Error(`No POST /config request recorded on ${dev.name}`);
      }
      if (configSaveRequest.body.nodeId !== safeNodeId || configSaveRequest.body.wifiSSID !== 'My_New_WiFi' || configSaveRequest.body.mqttIpDns !== '192.168.1.50') {
        throw new Error(`Save payload mismatch on ${dev.name}: ${JSON.stringify(configSaveRequest.body)}`);
      }

      // SCENARIO 3: Add Virtual Feature
      console.log('  -> Adding virtual feature...');
      await page.click('button[data-view="features"]'); // Open features tab
      await page.waitForSelector('#nf-name', { state: 'visible' });

      await page.fill('#nf-name', `Luz ${dev.name}`);
      await page.selectOption('#nf-driver', { value: '7' }); // Driver 7: Iluminação Pulsador Virtual
      
      await resetLogs();
      await page.click('#nf-add'); // Click Add Feature button
      await page.waitForTimeout(500);

      const featureLogs = await getLogs();
      const addFeatureRequest = featureLogs.find(l => l.path === '/features' && l.method === 'POST');
      if (!addFeatureRequest) {
        throw new Error(`No POST /features request recorded on ${dev.name}`);
      }
      if (addFeatureRequest.body.name !== `Luz ${dev.name}` || parseInt(addFeatureRequest.body.driver, 10) !== 7) {
        throw new Error(`Add feature payload mismatch on ${dev.name}: ${JSON.stringify(addFeatureRequest.body)}`);
      }

      // SCENARIO 4: Reboot system
      console.log('  -> Triggering system reboot...');
      await page.click('button[data-view="system"]'); // Back to system tab
      await page.click('button[data-system-view="firmware"]'); // Firmware / Actions sub-tab
      await resetLogs();
      
      // #a-reboot uses armed() - double click to confirm
      await page.click('#a-reboot');
      await page.click('#a-reboot');
      await page.waitForTimeout(500);

      const rebootLogs = await getLogs();
      const rebootRequest = rebootLogs.find(l => l.path === '/reboot');
      if (!rebootRequest) {
        throw new Error(`No /reboot request recorded on ${dev.name}`);
      }

      await context.close();
      console.log(`[SUCCESS] E2E Suite passed on device: ${dev.name}`);
    }

    console.log('\n[SUCCESS] All multi-device Web E2E Integration tests passed successfully!');
    stopServer();
    if (browser) await browser.close();
    process.exit(0);

  } catch (error) {
    console.error('\n[FAILED] E2E Integration test failed:');
    console.error(error.message);
    stopServer();
    if (browser) await browser.close();
    process.exit(1);
  }
}

runTests();
