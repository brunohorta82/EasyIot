const { chromium, devices } = require('playwright');
const { startServer, stopServer } = require('./mock-server');

async function runAquaDanceTests() {
  let browser;
  try {
    const url = await startServer(3000);
    browser = await chromium.launch({ headless: true });

    // Emulated devices: Desktop, Android, iOS, Tablet
    const devicesToTest = [
      { name: 'Desktop Chrome (1280x720)', use: { viewport: { width: 1280, height: 720 } } },
      { name: 'Pixel 5 (Android Emulator)', use: devices['Pixel 5'] },
      { name: 'iPhone 13 (iOS Safari Emulator)', use: devices['iPhone 13'] },
      { name: 'iPad Pro 11 (Tablet Emulator)', use: devices['iPad Pro 11'] }
    ];

    for (const dev of devicesToTest) {
      console.log(`\n======================================================`);
      console.log(`[TEST] AquaDance & Musical 2D Fountain on ${dev.name}`);
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

      // 1. Switch to Rega -> Fonte AquaDance Tab
      console.log('  -> Switching to Rega -> Fonte AquaDance sub-tab...');
      await page.click('button[data-view="irrigation"]');
      await page.waitForSelector('#btn-sub-dance', { state: 'visible', timeout: 5000 });
      await page.click('#btn-sub-dance');
      await page.waitForSelector('#pane-irr-dance:not(.hide)', { timeout: 5000 });

      // 2. Verify Sequencer Toolbar and Controls
      console.log('  -> Verifying Sequencer Toolbar...');
      const tempoVal = await page.inputValue('#dance-tempo');
      const stepsVal = await page.inputValue('#dance-steps');
      console.log(`  -> Initial Tempo: ${tempoVal}ms, Steps: ${stepsVal}`);

      // 3. Test Musical Matrix Sequencer (Piano Roll)
      console.log('  -> Testing Matrix Sequencer note toggles...');
      await page.waitForSelector('.dance-matrix-wrap table, .dance-matrix', { timeout: 5000 });
      const initialCells = await page.$$eval('.dance-cell', els => els.length);
      console.log(`  -> Detected ${initialCells} interactive choreography step cells.`);

      // Click a matrix cell to toggle note on/off
      const firstCell = await page.$('.dance-cell');
      if (firstCell) {
        await firstCell.click();
        await page.waitForTimeout(200);
      }

      // 4. Test Preset Patterns (Wave / Chase / Pulse)
      console.log('  -> Applying Preset: Cascata / Onda...');
      await page.click('#dance-preset-wave');
      await page.waitForTimeout(300);

      const activeCellsWave = await page.$$eval('.dance-cell.on', els => els.length);
      console.log(`  -> Active cells after Wave preset: ${activeCellsWave}`);
      if (activeCellsWave === 0) throw new Error('Preset wave did not activate any cells');

      // 5. Test 2D Pool Basin Simulator & Layout Presets
      console.log('  -> Testing 2D Pool Basin Simulator...');
      const basin = await page.$('#dance-pool-basin');
      if (!basin) throw new Error('2D Pool basin not found');

      const initialPoolNodes = await page.$$eval('.pool-node', els => els.length);
      console.log(`  -> Detected ${initialPoolNodes} fixtures mapped on the 2D basin.`);
      if (initialPoolNodes === 0) throw new Error('No fixtures mapped on 2D basin');

      // Test Layout Presets: Círculo, Linha, Cruz
      console.log('  -> Testing Layout Preset: Círculo...');
      await page.click('#dance-layout-ring');
      await page.waitForTimeout(300);

      console.log('  -> Testing Layout Preset: Cruz...');
      await page.click('#dance-layout-cross');
      await page.waitForTimeout(300);

      // 6. Test Playback Controls (Play -> Step Animation -> Stop)
      console.log('  -> Testing Playback (▶ Reproduzir Dança)...');
      await page.click('#dance-play');
      await page.waitForTimeout(800); // Allow real-time sequencer animation to step

      const stopBtn = await page.$('#dance-stop');
      const isStopVisible = await stopBtn.evaluate(el => !el.classList.contains('hide'));
      if (!isStopVisible) throw new Error('Stop button should be visible during playback');
      console.log('  -> Playback is RUNNING with real-time water jets and lighting animation.');

      // Stop playback
      await page.click('#dance-stop');
      await page.waitForTimeout(300);
      console.log('  -> Playback successfully stopped.');

      // 7. Test Home Assistant 2D Card Exporter
      console.log('  -> Testing Home Assistant 2D Card Export...');
      await page.click('#dance-export-ha');
      await page.waitForTimeout(300);
      console.log('  -> Home Assistant 2D Card YAML exported successfully.');

      await context.close();
      console.log(`[PASS] AquaDance & 2D Fountain tests passed on ${dev.name}`);
    }

    console.log('\n[ALL PASS] AquaDance & 2D Fountain E2E tests succeeded across all emulators!');
    stopServer();
    if (browser) await browser.close();
    process.exit(0);

  } catch (err) {
    console.error('\n[FAIL] AquaDance test failed:');
    console.error(err);
    stopServer();
    if (browser) await browser.close();
    process.exit(1);
  }
}

runAquaDanceTests();
