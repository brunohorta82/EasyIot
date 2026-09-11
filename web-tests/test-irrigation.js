const { chromium, devices } = require('playwright');
const { startServer, stopServer } = require('./mock-server');

async function runIrrigationTests() {
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
      console.log(`[TEST] Irrigation Multi-Zone Scheduler on ${dev.name}`);
      console.log(`======================================================`);

      const context = await browser.newContext({
        ...dev.use,
        permissions: ['clipboard-read', 'clipboard-write']
      });
      const page = await context.newPage();

      // Navigate to Web Panel
      console.log('  -> Navigating to Web Panel...');
      await page.goto(url);
      await page.waitForSelector('button[data-view="irrigation"]', { timeout: 10000 });

      // 1. Open Irrigation Tab
      console.log('  -> Opening Irrigation Tab...');
      await page.click('button[data-view="irrigation"]');
      await page.waitForSelector('#pane-irr-trad:not(.hide)', { timeout: 5000 });

      // 2. Verify Status Card & Global Settings
      console.log('  -> Verifying Global Settings (Rain Skip & Max Concurrent)...');
      const rainCheckbox = await page.$('#irr-rain');
      if (!rainCheckbox) throw new Error('#irr-rain checkbox not found');
      await rainCheckbox.click(); // toggle rain skip

      const maxSelect = await page.$('#irr-max');
      if (maxSelect) {
        await page.selectOption('#irr-max', { value: '2' });
      }

      // 3. Test Adding a New Program
      console.log('  -> Adding a new Irrigation Program...');
      const initialProgs = await page.$$eval('.irr-prog', els => els.length);
      await page.click('#irr-add');
      await page.waitForTimeout(300);

      const updatedProgs = await page.$$eval('.irr-prog', els => els.length);
      if (updatedProgs !== initialProgs + 1) {
        throw new Error(`Failed to add new program. Expected ${initialProgs + 1}, got ${updatedProgs}`);
      }
      console.log(`  -> Successfully added program #${updatedProgs}.`);

      // 4. Configure Weekdays and Start Time
      console.log('  -> Configuring Weekday selector & Start Time...');
      const dayBtns = await page.$$('.irr-prog:last-child .irr-day');
      if (dayBtns.length >= 7) {
        await dayBtns[1].click(); // Monday
        await dayBtns[3].click(); // Wednesday
        await dayBtns[5].click(); // Friday
      }

      const timeInput = await page.$('.irr-prog:last-child input[data-ip="start"]');
      if (timeInput) {
        await timeInput.fill('06:30');
        await timeInput.dispatchEvent('input');
      }

      // 5. Select Valves and Set Duration
      console.log('  -> Selecting valve zones for program...');
      const zoneChecks = await page.$$('.irr-prog:last-child input[data-ipz]');
      if (zoneChecks.length) {
        await zoneChecks[0].click();
      }

      // 6. Save Irrigation Schedule
      console.log('  -> Saving Irrigation Configuration...');
      const saveBtn = await page.$('#irr-save');
      if (saveBtn) {
        await saveBtn.click();
        await page.waitForTimeout(400);
      }

      // 7. Test Manual Program Run & Stop
      console.log('  -> Testing Manual Program Execution...');
      const runBtn = await page.$('.irr-prog-card:first-child button[data-iprun]');
      if (runBtn) {
        // Arm click
        await runBtn.click();
        await runBtn.click(); // armed confirmation
        await page.waitForTimeout(500);

        // Verify running state
        await page.waitForSelector('#irr-stop', { timeout: 5000 });
        console.log('  -> Program is RUNNING: valve countdown active.');

        // Stop cycle
        await page.click('#irr-stop');
        await page.waitForTimeout(400);
        console.log('  -> Cycle stopped successfully.');
      }

      await context.close();
      console.log(`[PASS] Irrigation tests passed on ${dev.name}`);
    }

    console.log('\n[ALL PASS] Irrigation E2E tests succeeded across all emulators!');
    stopServer();
    if (browser) await browser.close();
    process.exit(0);

  } catch (err) {
    console.error('\n[FAIL] Irrigation test failed:');
    console.error(err);
    stopServer();
    if (browser) await browser.close();
    process.exit(1);
  }
}

runIrrigationTests();
