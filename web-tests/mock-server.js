const express = require('express');
const path = require('path');
const app = express();

app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Silence favicon 404
app.get('/favicon.ico', (req, res) => res.status(204).end());

// Serve static webpanel assets from the root webpanel folder
app.use('/', express.static(path.join(__dirname, '../webpanel')));

// Initial mockup configuration matching ConfigOnofre structure
const initialConfig = {
  nodeId: "test-node",
  chipId: "123456",
  mcu: "ESP32",
  firmware: "9.202",
  buildDate: "Sep 01 2026",
  wifiSSID: "Home_WiFi",
  wifiSecret: "hide_password",
  dhcp: true,
  wifiIp: "192.168.1.100",
  wifiMask: "255.255.255.0",
  wifiGw: "192.168.1.254",
  mqttIpDns: "192.168.1.10",
  mqttPort: 1883,
  mqttUsername: "admin",
  mqttPassword: "hide_password",
  outInPins: [4, 5, 12, 13, 14],
  inPins: [17],
  usablePins: [4, 5, 12, 13, 14, 17],
  freeHeap: 180000,
  heapFrag: 3,
  maxFreeBlock: 120000,
  resetReason: "Power on",
  uptime: 3600,
  signal: -65,
  mqttConnected: true,
  cloudConfigured: false,
  clockSynced: true,
  clockNow: "2026-09-01 18:00:00",
  features: [
    {
      group: "ACTUATOR",
      driver: "GARDEN_VALVE",
      id: "vlv1",
      name: "Jato Central",
      typeControl: 0,
      state: 0,
      inputs: [12],
      outputs: [13]
    },
    {
      group: "ACTUATOR",
      driver: "GARDEN_VALVE",
      id: "vlv2",
      name: "Jato Anel",
      typeControl: 0,
      state: 0,
      inputs: [14],
      outputs: [15]
    },
    {
      group: "ACTUATOR",
      driver: "LIGHT_RGBW",
      id: "light1",
      name: "Foco RGBW",
      typeControl: 0,
      state: 0,
      inputs: [18],
      outputs: [19]
    },
    {
      group: "SENSOR",
      driver: "LD2450",
      id: "radar1",
      name: "Radar Sala",
      inputs: [16, 17],
      outputs: [],
      state: JSON.stringify({
        occupancy: "detected",
        motion: "detected",
        count: 2,
        t1_x: -650,
        t1_y: 2200,
        t1_s: 14,
        t1_r: 200,
        t2_x: 800,
        t2_y: 3500,
        t2_s: 0,
        t2_r: 200
      })
    },
    {
      group: "SENSOR",
      driver: "LD2410",
      id: "radar2",
      name: "Radar Quarto",
      inputs: [21, 22],
      outputs: [],
      state: JSON.stringify({
        occupancy: "detected",
        motion: "detected",
        movingTargetDistance: 180,
        movingTargetEnergy: 75,
        stationaryTargetDistance: 120,
        stationaryTargetEnergy: 60
      })
    }
  ]
};

let mockIrrigation = {
  enabled: true,
  skipOnRain: true,
  maxConcurrentZones: 1,
  programs: [
    {
      id: 1,
      enabled: true,
      weekdays: 127,
      startMinute: 420,
      zones: [
        { uniqueId: "vlv1", minutes: 10 },
        { uniqueId: "vlv2", minutes: 15 }
      ]
    }
  ],
  running: null
};

let mockAquadance = {
  shows: [
    {
      id: 1,
      name: "Sinfonia das Águas",
      tempoMs: 300,
      totalSteps: 16,
      loop: false,
      tracks: [
        { uniqueId: "vlv1", trackType: 0, steps: [1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0], rgbw: [], posX: 35, posY: 45 },
        { uniqueId: "vlv2", trackType: 0, steps: [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1], rgbw: [], posX: 65, posY: 45 },
        { uniqueId: "light1", trackType: 2, steps: [100, 100, 0, 0, 100, 100, 0, 0, 100, 100, 0, 0, 100, 100, 0, 0], rgbw: [0x00e5ff, 0x00e5ff, 0, 0, 0xffcc00, 0xffcc00, 0, 0, 0x00ff88, 0x00ff88, 0, 0, 0xff00aa, 0xff00aa, 0, 0], posX: 50, posY: 65 }
      ]
    }
  ]
};

initialConfig.irrigation = mockIrrigation;
let config = JSON.parse(JSON.stringify(initialConfig));

// Requests log so the test runner can verify actions
let requestsLog = [];

app.use((req, res, next) => {
  if (req.path !== '/config' || req.method !== 'GET') {
    requestsLog.push({
      method: req.method,
      path: req.path,
      body: req.body,
      query: req.query
    });
  }
  next();
});

// REST API Endpoints
app.get('/config', (req, res) => {
  res.json(config);
});

app.post('/config', (req, res) => {
  config = { ...config, ...req.body };
  res.json(config);
});

app.get('/irrigation', (req, res) => {
  res.json(mockIrrigation);
});

app.post('/irrigation', (req, res) => {
  mockIrrigation = { ...mockIrrigation, ...req.body };
  config.irrigation = mockIrrigation;
  res.json(mockIrrigation);
});

app.post('/irrigation-run', (req, res) => {
  mockIrrigation.running = {
    programId: req.body.programId || 1,
    zone: "vlv1",
    secondsLeft: 600,
    zones: [{ zone: "vlv1", secondsLeft: 600 }]
  };
  res.json(mockIrrigation);
});

app.post('/irrigation-stop', (req, res) => {
  mockIrrigation.running = null;
  res.json(mockIrrigation);
});

app.get('/aquadance', (req, res) => {
  res.json(mockAquadance);
});

app.post('/aquadance', (req, res) => {
  mockAquadance = req.body;
  res.json(mockAquadance);
});

app.post('/aquadance-run', (req, res) => {
  res.json({ status: "running", showId: req.body.showId || 1 });
});

app.post('/aquadance-stop', (req, res) => {
  res.json({ status: "stopped" });
});

app.post('/sensors/radar-config', (req, res) => {
  res.json({ status: "ok", config: req.body });
});

app.get('/backup', (req, res) => {
  res.json({
    format: "easyiot-backup",
    version: 1,
    target: { mcu: config.mcu || "ESP32" },
    configuration: { nodeId: config.nodeId, wifiSSID: config.wifiSSID },
    irrigation: mockIrrigation,
    features: config.features
  });
});

app.post('/restore', (req, res) => {
  res.json({ status: "ok" });
});

app.get('/logs', (req, res) => {
  res.send("ESP32 system boot ok\nWiFi connected to Home_WiFi (192.168.1.100)\nMQTT connected\n");
});

app.all('/reboot', (req, res) => {
  res.json({ status: "rebooting" });
});

app.post('/load-defaults', (req, res) => {
  config.features = [];
  res.json({ status: "defaults_loaded" });
});

app.post('/templates/change', (req, res) => {
  res.json({ status: "template_changed" });
});

const DRIVER_MAP = {
  1: "LIGHT_PUSH",
  2: "LIGHT_LATCH",
  3: "COVER_PUSH",
  4: "COVER_LATCH",
  5: "COVER_PUSH_TOGGLE",
  7: "LIGHT_PUSH_VIRTUAL",
  8: "LIGHT_LATCH_VIRTUAL",
  9: "GARAGE_PUSH",
  10: "GARDEN_VALVE",
  15: "RGB_LIGHT",
  16: "ANALOG_DIMMER",
  60: "SENSOR",
  71: "DHT_11",
  72: "DHT_22",
  82: "PIR",
  93: "HCSR04",
  94: "LD2410",
  96: "LD2450",
  97: "LD2460",
  105: "LDC1612"
};

// SSE stream for real-time live events and radar simulation
app.get('/events', (req, res) => {
  res.writeHead(200, {
    'Content-Type': 'text/event-stream',
    'Cache-Control': 'no-cache',
    'Connection': 'keep-alive'
  });
  res.write('event: mqtt_health\ndata: online\n\n');
  const timer = setInterval(() => {
    const t1x = Math.round(Math.sin(Date.now() / 2000) * 1200);
    const t1y = Math.round(2500 + Math.cos(Date.now() / 2500) * 1000);
    const payload = JSON.stringify({
      occupancy: "detected",
      motion: "detected",
      count: 1,
      t1_x: t1x,
      t1_y: t1y,
      t1_s: 18,
      t1_r: 200
    });
    res.write(`event: radar1\ndata: ${payload}\n\n`);
  }, 1000);
  req.on('close', () => clearInterval(timer));
});

app.post('/features', (req, res) => {
  const driverVal = parseInt(req.body.driver, 10);
  const newFeature = {
    id: req.body.id || `feature_${Date.now()}`,
    name: req.body.name,
    driver: DRIVER_MAP[driverVal] || `UNKNOWN_${driverVal}`,
    group: driverVal >= 60 ? "SENSOR" : "ACTUATOR",
    inputs: [req.body.input1, req.body.input2].filter(p => p !== undefined && !isNaN(p)),
    outputs: [],
    state: 0
  };
  config.features.push(newFeature);
  res.json(config);
});

app.delete('/features', (req, res) => {
  const { id } = req.body;
  config.features = config.features.filter(f => f.id !== id);
  res.json(config);
});

// Endpoint to retrieve request logs for test validation
app.get('/test/logs', (req, res) => {
  res.json(requestsLog);
});

app.post('/test/reset-logs', (req, res) => {
  requestsLog = [];
  res.json({ status: "logs_reset" });
});

app.post('/test/reset-all', (req, res) => {
  config = JSON.parse(JSON.stringify(initialConfig));
  requestsLog = [];
  res.json({ status: "all_reset" });
});

let server;
function startServer(port = 3000) {
  return new Promise((resolve) => {
    server = app.listen(port, () => {
      console.log(`[MOCK] ESP WebServer running at http://localhost:${port}`);
      resolve(`http://localhost:${port}`);
    });
  });
}

function stopServer() {
  if (server) {
    server.close();
    console.log('[MOCK] Server stopped.');
  }
}

module.exports = { startServer, stopServer, getConfig: () => config };
