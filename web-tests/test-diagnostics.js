const { chromium, devices } = require('playwright');
const { startServer, stopServer } = require('./mock-server');

async function runDiagnosticsTests() {
  let browser;
  try {
    const url = await startServer(3000);
    browser = await chromium.launch({ headless: true });

    const devicesToTest = [
      { name: 'Desktop Chrome (1280x720)', use: { viewport: { width: 1280, height: 720 } } },
      { name: 'Pixel 5 (Android Emulator)', use: devices['Pixel 5'] },
      { name: 'iPhone 13 (iOS Safari Emulator)', use: devices['iPhone 13'] },
      { name: 'iPad Pro 11 (Tablet Emulator)', use: devices['iPad Pro 11'] }
    ];

    for (const dev of devicesToTest) {
      console.log(`\n======================================================`);
      console.log(`[TEST] Diagnostics, Telemetry & Backups on ${dev.name}`);
      console.log(`======================================================`);

      const context = await browser.newContext({
        ...dev.use,
        permissions: ['clipboard-read', 'clipboard-write']
      });
      const page = await context.newPage();

      // Navigate to Web Panel
      console.log('  -> Navigating to Web Panel...');
      await page.goto(url);
      await page.waitForSelector('button[data-view="diag"]', { timeout: 10000 });

      // 1. Open Diagnostics Tab
      console.log('  -> Opening Diagnostics Tab...');
      await page.click('button[data-view="diag"]');
      await page.waitForSelector('#v-diag.on', { timeout: 5000 });

      // 2. Verify Heap Memory & Sparkline SVG
      console.log('  -> Verifying Free Heap and Sparkline Graph...');
      const heapText = await page.textContent('#d-heap');
      console.log(`  -> Reported Heap: ${heapText}`);

      const sparkSvg = await page.$('#d-heap-spark');
      if (!sparkSvg) throw new Error('#d-heap-spark SVG element not found');

      // 3. Verify Network Status Card & MQTT
      console.log('  -> Verifying Network Status & Broker...');
      const netCard = await page.$('#d-net-card');
      if (!netCard) throw new Error('#d-net-card not found');

      const brokerText = await page.textContent('#d-broker');
      console.log(`  -> MQTT Broker: ${brokerText}`);

      // 4. Test Live Log Controls (Pause / Resume / Copy)
      console.log('  -> Testing Live Terminal Log controls...');
      const pauseBtn = await page.$('#d-log-pause');
      if (pauseBtn) {
        await pauseBtn.click();
        await page.waitForTimeout(200);
        let btnText = await pauseBtn.textContent();
        if (btnText !== 'Retomar') throw new Error('Log pause failed');

        await pauseBtn.click();
        await page.waitForTimeout(200);
        btnText = await pauseBtn.textContent();
        if (btnText !== 'Pausar') throw new Error('Log resume failed');
      }

      const copyBtn = await page.$('#d-log-copy');
      if (copyBtn) {
        await copyBtn.click();
        await page.waitForTimeout(200);
      }

      // 5. Test Pinout & Board View
      console.log('  -> Navigating to PINOUT & Templates tab...');
      await page.click('button[data-view="pinout"]');
      await page.waitForSelector('#v-pinout.on', { timeout: 5000 });

      const pinLeftCount = await page.$$eval('#pin-left .pin', els => els.length);
      const pinRightCount = await page.$$eval('#pin-right .pin', els => els.length);
      console.log(`  -> Board Pinout Rendered: ${pinLeftCount} left pins, ${pinRightCount} right pins.`);

      // 6. Test Backup JSON Export
      console.log('  -> Testing Configuration Backup Export...');
      await page.click('button[data-view="system"]');
      await page.waitForSelector('#v-system.on', { timeout: 5000 });
      await page.click('button[data-system-view="access"]');
      await page.waitForTimeout(200);

      const exportBtn = await page.$('#a-export');
      if (exportBtn) {
        await exportBtn.click();
        await page.waitForTimeout(300);
        console.log('  -> Backup JSON export triggered.');
      }

      await context.close();
      console.log(`[PASS] Diagnostics & Telemetry tests passed on ${dev.name}`);
    }

    console.log('\n[ALL PASS] Diagnostics & Backups E2E tests succeeded across all emulators!');
    stopServer();
    if (browser) await browser.close();
    process.exit(0);

  } catch (err) {
    console.error('\n[FAIL] Diagnostics test failed:');
    console.error(err);
    stopServer();
    if (browser) await browser.close();
    process.exit(1);
  }
}

runDiagnosticsTests();
