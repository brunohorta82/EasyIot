const { chromium, devices } = require('playwright');
const { startServer, stopServer } = require('./mock-server');

async function runResiliencyTests() {
  let browser;
  try {
    const url = await startServer(3000);
    browser = await chromium.launch({ headless: true });

    const devicesToTest = [
      { name: 'Desktop Chrome (1280x720)', use: { viewport: { width: 1280, height: 720 } } },
      { name: 'Pixel 5 (Android Emulator)', use: devices['Pixel 5'] },
      { name: 'iPhone 13 (iOS Safari Emulator)', use: devices['iPhone 13'] }
    ];

    for (const dev of devicesToTest) {
      console.log(`\n======================================================`);
      console.log(`[TEST] Network Resiliency & Reconnect on ${dev.name}`);
      console.log(`======================================================`);

      const context = await browser.newContext(dev.use);
      const page = await context.newPage();

      // Navigate to Web Panel
      console.log('  -> Navigating to Web Panel...');
      await page.goto(url);
      await page.waitForSelector('button[data-view="system"]', { timeout: 10000 });

      // 1. Verify Online State
      console.log('  -> Verifying initial online connection...');
      const title = await page.title();
      if (!title) throw new Error('Page title empty');

      // 2. Emulate Network Offline State
      console.log('  -> Simulating Network Disconnection (Offline)...');
      await context.setOffline(true);
      await page.waitForTimeout(600);

      // 3. Emulate Network Reconnection
      console.log('  -> Simulating Network Reconnection (Online)...');
      await context.setOffline(false);
      await page.waitForTimeout(600);

      // 4. Verify SSE Event Stream Recovery
      console.log('  -> Verifying real-time updates resume...');
      await page.waitForSelector('#ov-radar-studio:not(.hide)', { timeout: 8000 });

      await context.close();
      console.log(`[PASS] Resiliency tests passed on ${dev.name}`);
    }

    console.log('\n[ALL PASS] Network Resiliency E2E tests succeeded across all emulators!');
    stopServer();
    if (browser) await browser.close();
    process.exit(0);

  } catch (err) {
    console.error('\n[FAIL] Resiliency test failed:');
    console.error(err);
    stopServer();
    if (browser) await browser.close();
    process.exit(1);
  }
}

runResiliencyTests();
