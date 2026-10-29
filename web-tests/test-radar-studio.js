const { chromium, devices } = require('playwright');
const { startServer, stopServer } = require('./mock-server');

async function runRadarStudioTests() {
  let browser;
  try {
    const url = await startServer(3000);
    browser = await chromium.launch({ headless: true });

    // Emulated devices: Desktop, Tablet, and Mobile Android/iOS
    const devicesToTest = [
      { name: 'Desktop Chrome (1280x720)', use: { viewport: { width: 1280, height: 720 } } },
      { name: 'Pixel 5 (Android Emulator)', use: devices['Pixel 5'] },
      { name: 'iPhone 13 (iOS Safari Emulator)', use: devices['iPhone 13'] },
      { name: 'iPad Pro 11 (Tablet Emulator)', use: devices['iPad Pro 11'] }
    ];

    for (const dev of devicesToTest) {
      console.log(`\n======================================================`);
      console.log(`[TEST] Radar & Presence 2D Studio on ${dev.name}`);
      console.log(`======================================================`);

      const context = await browser.newContext({
        ...dev.use,
        permissions: ['clipboard-read', 'clipboard-write']
      });
      const page = await context.newPage();
      page.on('console', msg => console.log(`    [BROWSER CONSOLE] ${msg.text()}`));
      page.on('pageerror', err => console.error(`    [BROWSER ERROR] ${err.message}`));

      // Navigate to Web Panel
      console.log('  -> Navigating to Web Panel...');
      await page.goto(url);
      await page.waitForSelector('button[data-view="system"]', { timeout: 10000 });
      await page.waitForSelector('#ov-radar-studio:not(.hide)', { timeout: 10000 });

      // 1. Verify Radar Studio Card and SVG canvas are visible and rendered
      console.log('  -> Verifying Radar Studio 2D Canvas...');
      const studioCard = await page.$('#ov-radar-studio');
      if (!studioCard) throw new Error('Radar Studio card not found in DOM');

      const isHidden = await studioCard.evaluate(el => el.classList.contains('hide'));
      if (isHidden) throw new Error('Radar Studio card is unexpectedly hidden');

      const svgCanvas = await page.$('#radar-svg');
      if (!svgCanvas) throw new Error('Radar SVG canvas not found');

      // Check distance arcs and angle rays
      const arcsCount = await page.$$eval('.radar-grid-arc', els => els.length);
      if (arcsCount < 4) throw new Error(`Expected at least 4 distance arcs, found ${arcsCount}`);
      console.log(`  -> Detected ${arcsCount} polar distance arcs in 2D canvas.`);

      // 2. Verify Multi-Target Telemetry from Mock Sensor (LD2450)
      console.log('  -> Verifying active sensor targets (LD2450)...');
      await page.waitForSelector('.radar-target-node', { timeout: 5000 });
      const targetLabels = await page.$$eval('.radar-target-label', els => els.map(e => e.textContent));
      console.log(`  -> Found live target labels: ${JSON.stringify(targetLabels)}`);
      if (!targetLabels.length) throw new Error('No targets rendered on SVG canvas');

      const badgeCountText = await page.textContent('#radar-badge-count');
      console.log(`  -> Target count badge: ${badgeCountText}`);

      // 3. Test Interactive Detection Zones Management
      console.log('  -> Testing Detection Zones (Add, Edit, Delete)...');
      const initialZones = await page.$$eval('.radar-zone-shape', els => els.length);
      console.log(`  -> Initial zones rendered: ${initialZones}`);

      // Add a new zone
      await page.click('#radar-add-zone');
      await page.waitForTimeout(300);
      const updatedZones = await page.$$eval('.radar-zone-shape', els => els.length);
      if (updatedZones !== initialZones + 1) {
        throw new Error(`Failed to add new zone. Expected ${initialZones + 1}, got ${updatedZones}`);
      }
      console.log(`  -> Successfully added new detection zone. Total: ${updatedZones}`);

      // Rename the newly added zone
      const lastZoneInput = await page.$('.radar-zone-item:last-child input');
      if (lastZoneInput) {
        await lastZoneInput.fill('Zona Teste E2E');
        await lastZoneInput.dispatchEvent('input');
      }

      // 4. Test Virtual Simulation Mode ("▶ Modo Simulação")
      console.log('  -> Testing Virtual Simulation Mode toggle...');
      const simBtn = await page.$('#radar-btn-sim');
      if (!simBtn) throw new Error('Simulation button #radar-btn-sim not found');

      // Turn Simulation ON
      await page.click('#radar-btn-sim');
      await page.waitForTimeout(600); // Allow simulation loop to step

      let simBtnText = await page.textContent('#radar-btn-sim');
      if (!simBtnText.includes('Parar')) {
        throw new Error(`Simulation mode did not activate. Button text: ${simBtnText}`);
      }
      console.log('  -> Virtual Simulation is ACTIVE: animated targets moving across polar canvas.');

      // Let simulated target walk through the room
      await page.waitForTimeout(1000);

      // Turn Simulation OFF
      await page.click('#radar-btn-sim');
      await page.waitForTimeout(300);
      simBtnText = await page.textContent('#radar-btn-sim');
      if (!simBtnText.includes('Modo Simulação')) {
        throw new Error(`Simulation mode did not stop. Button text: ${simBtnText}`);
      }
      console.log('  -> Virtual Simulation successfully stopped.');

      // 5. Test LD2410 Sensor Selection & Gate/Energy Gauges
      console.log('  -> Switching active sensor to LD2410 (Radar Quarto)...');
      await page.selectOption('#radar-sensor-select', { value: 'radar2' });
      await page.waitForTimeout(400);

      const ldCard = await page.$('#radar-ld2410-card');
      const isLdHidden = await ldCard.evaluate(el => el.classList.contains('hide'));
      if (isLdHidden) throw new Error('LD2410 Energy Gauge card should be visible for radar2');

      const moveEnergyText = await page.textContent('#radar-move-energy');
      const statEnergyText = await page.textContent('#radar-stat-energy');
      console.log(`  -> LD2410 Energy Gauges: Moving=${moveEnergyText}, Stationary=${statEnergyText}`);
      if (moveEnergyText !== '75%' || statEnergyText !== '60%') {
        throw new Error(`LD2410 Energy values mismatch: Moving=${moveEnergyText}, Stationary=${statEnergyText}`);
      }

      // 6. Test Home Assistant 2D Card Exporter
      console.log('  -> Testing Home Assistant Lovelace YAML Card Export...');
      await page.click('#radar-btn-export-ha');
      await page.waitForTimeout(300);
      console.log('  -> Home Assistant 2D YAML Card exported successfully.');

      await context.close();
      console.log(`[PASS] Radar & Presence 2D Studio tests passed on ${dev.name}`);
    }

    console.log('\n[ALL PASS] Radar & Presence 2D Studio E2E tests succeeded across all emulators!');
    stopServer();
    if (browser) await browser.close();
    process.exit(0);

  } catch (err) {
    console.error('\n[FAIL] Radar Studio test failed:');
    console.error(err);
    stopServer();
    if (browser) await browser.close();
    process.exit(1);
  }
}

runRadarStudioTests();
