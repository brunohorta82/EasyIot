let baseUrl = (location.origin && location.origin !== "null") ? location.origin : "";
/* OnOfre device panel.
   The build strips this first line and injects `let baseUrl = ""` so the embedded
   copy talks to whatever address the device was reached at. */

var config = {};          // last config read from the device
var dirty = false;        // unsaved edits in the forms
var removed = [];         // feature ids queued for removal
var heapHistory = [];     // free heap samples, for the sparkline
var logPaused = false;
var logLines = [];
var source = null;

const $ = (id) => document.getElementById(id);
const esc = (s) => String(s == null ? "" : s).replace(/[&<>"']/g,
  (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));

/* Drivers the firmware can create, grouped for the picker. `pins` is how many
   GPIOs prepareNewFeature() expects — the form follows it. */
const DRIVERS = [
  { g: "Luzes e tomadas", items: [
    { v: 7, t: "LIGHT_PUSH", n: "Luz · botão de pressão", pins: 1 },
    { v: 8, t: "LIGHT_LATCH", n: "Luz · interruptor", pins: 1 },
    { v: 1, t: "SWITCH_PUSH", n: "Tomada · botão de pressão", pins: 1 },
    { v: 2, t: "SWITCH_LATCH", n: "Tomada · interruptor", pins: 1 },
  ]},
  { g: "Estores e portões", items: [
    { v: 3, t: "COVER_SINGLE_PUSH", n: "Estore · um botão", pins: 1 },
    { v: 4, t: "COVER_DUAL_PUSH", n: "Estore · dois botões", pins: 2 },
    { v: 5, t: "COVER_DUAL_LATCH", n: "Estore · dois interruptores", pins: 2 },
    { v: 9, t: "GARAGE_PUSH", n: "Portão", pins: 1 },
    { v: 10, t: "GARDEN_VALVE", n: "Válvula de rega", pins: 1 },
  ]},
  { g: "Sensores", items: [
    { v: 111, t: "DHT_11", n: "DHT11 · temperatura e humidade", pins: 1 },
    { v: 121, t: "DHT_21", n: "DHT21 · temperatura e humidade", pins: 1 },
    { v: 122, t: "DHT_22", n: "DHT22 · temperatura e humidade", pins: 1 },
    { v: 90, t: "DS18B20", n: "DS18B20 · temperatura", pins: 1 },
    { v: 84, t: "DOOR", n: "Sensor de porta", pins: 1 },
    { v: 85, t: "WINDOW", n: "Sensor de janela", pins: 1 },
    { v: 82, t: "PIR", n: "Movimento (PIR)", pins: 1 },
    { v: 83, t: "RAIN", n: "Chuva", pins: 1 },
    { v: 93, t: "HCSR04", n: "Distância (HC-SR04)", pins: 2 },
    { v: 94, t: "LD2410", n: "Presença (LD2410)", pins: 2 },
    { v: 71, t: "PZEM_004T_V03", n: "Contador PZEM v3", pins: 2 },
    { v: 72, t: "PZEM_004T_V01", n: "Contador PZEM v1", pins: 2 },
  ]},
];
const driverInfo = (v) => {
  for (const g of DRIVERS) for (const d of g.items) if (d.v === v) return d;
  return null;
};

/* Drivers a device may report but the panel cannot create — a template or an
   older firmware put them there, and they still deserve a readable name. */
const EXTRA_DRIVER_LABELS = {
  HAN_MODBUS: "Contador de energia HAN",
  HAN_MODBUS_8N2: "Contador de energia HAN (8N2)",
  SHT4X: "SHT4x · temperatura e humidade",
  LTR303: "Sensor de luminosidade",
  TMF882X: "Distância (TMF882x)",
  LOCK_PUSH: "Fechadura",
  INVALID: "desconhecido",
};
/* /config reports the driver as a token; show it in the words the apps use. */
function driverLabel(token) {
  if (!token) return "—";
  for (const g of DRIVERS) for (const d of g.items) if (d.t === token) return d.n;
  return EXTRA_DRIVER_LABELS[token] || token;
}
const TEMPLATES = [
  { v: 1, n: "Duas luzes" }, { v: 2, n: "Duas tomadas" }, { v: 3, n: "Estore" },
  { v: 4, n: "Portão" }, { v: 5, n: "Contador HAN" }, { v: 6, n: "Rega" },
];

/* ---------------- helpers ---------------- */
function toast(msg, kind) {
  const t = $("toast");
  t.textContent = msg;
  t.className = "on " + (kind || "");
  clearTimeout(toast._t);
  toast._t = setTimeout(() => { t.className = ""; }, 3200);
}
function markDirty() { dirty = true; $("dirty").classList.remove("hide"); }
function clearDirty() { dirty = false; $("dirty").classList.add("hide"); }

function duration(sec) {
  sec = Math.floor(sec || 0);
  const d = Math.floor(sec / 86400), h = Math.floor(sec % 86400 / 3600), m = Math.floor(sec % 3600 / 60);
  if (d) return d + "d " + String(h).padStart(2, "0") + "h";
  if (h) return h + "h " + String(m).padStart(2, "0") + "m";
  return m + "m";
}
function rssiText(dbm) {
  if (dbm == null) return "—";
  const q = dbm >= -60 ? "bom" : dbm >= -70 ? "razoável" : dbm >= -80 ? "fraco" : "muito fraco";
  return dbm + " dBm · " + q;
}
function rssiClass(dbm) { return dbm == null ? "" : dbm >= -70 ? "ok" : dbm >= -80 ? "warn" : "bad"; }

async function api(path, opts) {
  const res = await fetch(baseUrl + path, Object.assign(
    { headers: { "Content-Type": "application/json", "Accept": "application/json" } }, opts || {}));
  const txt = await res.text();
  let body = null;
  try { body = txt ? JSON.parse(txt) : null; } catch (e) { /* not JSON */ }
  if (!res.ok) {
    // A refusal carries {result:<code>}; keep it so callers can explain why.
    const err = new Error("HTTP " + res.status);
    err.status = res.status;
    err.body = body;
    throw err;
  }
  return body;
}

/* ---------------- load & render ---------------- */
async function load() {
  try {
    config = await api("/config");
  } catch (e) {
    toast("Não foi possível ler a configuração", "err");
    return;
  }
  removed = [];
  renderHeader();
  renderOverview();
  renderPinout();
  renderFeatures();
  renderDiag();
  fillSystem();
  fillNewFeatureForm();
}

function renderHeader() {
  $("h-name").textContent = config.nodeId || "—";
  $("h-chip").textContent = config.chipId || "";
  $("h-mcu").textContent = config.mcu || "";
  const w = $("h-wifi");
  w.className = "pill " + rssiClass(config.signal);
  w.innerHTML = "WiFi <b>" + esc(rssiText(config.signal)) + "</b>";
  setMqttPill(config.mqttConnected);
  $("h-up").innerHTML = "ligado há <b>" + duration(config.uptime) + "</b>";
}
function setMqttPill(on) {
  const m = $("h-mqtt");
  m.className = "pill " + (on ? "ok" : "bad");
  m.innerHTML = "MQTT <b>" + (on ? "ligado" : "desligado") + "</b>";
}

const isActuator = (f) => f.group === "ACTUATOR";
const isCover = (f) => (f.driver || "").indexOf("COVER") === 0;

function renderOverview() {
  const feats = config.features || [];
  const acts = feats.filter(isActuator);
  const sens = feats.filter((f) => !isActuator(f));

  $("ov-actuators").innerHTML = acts.length ? acts.map((f) => {
    const pins = (f.outputs || []).length ? "OUT " + f.outputs.join(",") : "";
    const ins = (f.inputs || []).length ? "IN " + f.inputs.join(",") : "";
    const state = parseInt(f.state, 10) || 0;
    if (isCover(f)) {
      return '<div class="frow"><div class="fname"><b>' + esc(f.name) + "</b><span>estore · " +
        state + '% aberto</span><input type="range" min="0" max="100" value="' + state +
        '" data-cover="' + esc(f.id) + '"></div>' +
        '<span class="pins">' + esc([pins, ins].filter(Boolean).join(" · ")) + "</span></div>";
    }
    return '<div class="frow"><div class="sw ' + (state > 0 ? "on" : "") + '" data-toggle="' + esc(f.id) + '"><i></i></div>' +
      '<div class="fname"><b>' + esc(f.name) + "</b><span>" + esc(f.family || "") + "</span></div>" +
      '<span class="pins">' + esc([pins, ins].filter(Boolean).join(" · ")) + "</span></div>";
  }).join("") : '<div class="note">Sem acessórios configurados.</div>';

  $("ov-sensors-title").classList.toggle("hide", !sens.length);
  $("ov-sensors").innerHTML = sens.map((f) => isEnergy(f) ? energyCard(f) :
    '<div class="card"><h4>' + esc(f.name) + '</h4><div class="fval" id="sv-' + esc(f.id) + '">' +
    esc(sensorText(f.state)) + "</div></div>").join("");
}

/* Energy meters report a dozen fields; a one-line summary would throw away
   everything that matters, so they get a card of their own. */
const ENERGY_DRIVERS = ["HAN_MODBUS", "HAN_MODBUS_8N2", "PZEM_004T_V03", "PZEM_004T_V01"];
const isEnergy = (f) => ENERGY_DRIVERS.indexOf(f.driver) >= 0;

const ENERGY_FIELDS = [
  { k: "voltage", n: "tensão", u: " V", d: 1 },
  { k: "current", n: "corrente", u: " A", d: 2 },
  { k: "powerFactor", n: "fator de potência", u: "", d: 2 },
  { k: "frequency", n: "frequência", u: " Hz", d: 1 },
  { k: "energy", n: "energia", u: " kWh", d: 2 },
  { k: "powerImport", n: "importado", u: " kWh", d: 1 },
  { k: "powerExport", n: "exportado", u: " kWh", d: 1 },
  { k: "rate1", n: "vazio", u: " kWh", d: 1 },
  { k: "rate2", n: "cheias", u: " kWh", d: 1 },
  { k: "rate3", n: "ponta", u: " kWh", d: 1 },
];
const TARIFF = { 1: "vazio", 2: "cheias", 3: "ponta" };

function parseState(state) {
  if (state == null || state === "") return null;
  let o = state;
  if (typeof o === "string") { try { o = JSON.parse(o); } catch (e) { return null; } }
  return typeof o === "object" ? o : null;
}

/* Importing reads positive; exporting to the grid reads negative and green. */
function energyCard(f) {
  const o = parseState(f.state) || {};
  const imp = Number(o.power || 0);
  const exp = Number(o.export || 0);
  const net = exp > 0 && imp <= 0 ? -exp : imp;
  const rows = ENERGY_FIELDS
    .filter((x) => o[x.k] != null)
    .map((x) => '<div class="kv"><span>' + x.n + "</span><b>" +
      Number(o[x.k]).toFixed(x.d) + x.u + "</b></div>").join("");
  const bad = o.status || (o.error ? "erro de leitura" : "");
  return '<div class="card"><h4>' + esc(f.name) + "</h4>" +
    '<div class="big ' + (net < 0 ? "grn" : "") + '" id="sv-' + esc(f.id) + '">' +
      (net < 0 ? "−" : "") + Math.abs(Math.round(net)) + "<small>W</small></div>" +
    '<div class="note" style="margin:2px 0 8px">' +
      (net < 0 ? "a exportar para a rede" : "a consumir da rede") + "</div>" +
    rows +
    (o.tarif != null ? '<div class="kv"><span>tarifa</span><b>' +
      esc(TARIFF[o.tarif] || o.tarif) + "</b></div>" : "") +
    (bad ? '<div class="note err">' + esc(bad) + "</div>" : "") +
    "</div>";
}

/* Sensor state arrives as JSON for most drivers and as a bare value for others. */
function sensorText(state) {
  if (state == null || state === "") return "—";
  let o = state;
  if (typeof o === "string") { try { o = JSON.parse(o); } catch (e) { return o; } }
  if (typeof o !== "object") return String(o);
  const bits = [];
  if (o.temperature != null) bits.push(Number(o.temperature).toFixed(1) + "°C");
  if (o.humidity != null) bits.push(Math.round(o.humidity) + "%");
  if (o.power != null) bits.push(Math.round(o.power) + "W");
  if (o.distance != null) bits.push(Math.round(o.distance) + "cm");
  if (o.illuminance != null) bits.push(Math.round(o.illuminance) + "lx");
  if (o.state != null && !bits.length) bits.push(o.state ? "ativo" : "livre");
  return bits.length ? bits.join(" · ") : "—";
}

function renderPinout() {
  const usable = config.usablePins || [];
  const inputOnly = config.inputOnlyPins || [];
  const used = {};
  for (const f of config.features || []) {
    for (const o of f.outputs || []) used[o] = { n: f.name, k: "out" };
    for (const i of f.inputs || []) used[i] = { n: f.name, k: "in" };
  }
  const all = usable.concat(inputOnly).sort((a, b) => a - b);
  const half = Math.ceil(all.length / 2);
  const draw = (pins, right) => pins.map((p) => {
    const u = used[p];
    const only = inputOnly.indexOf(p) >= 0;
    const tag = u ? (u.k === "out" ? '<span class="tag t-out">SAÍDA</span>' : '<span class="tag t-in">ENTRADA</span>')
                  : '<span class="tag t-free">LIVRE</span>';
    const what = u ? esc(u.n) : (only ? "livre · só entrada" : "livre");
    return '<div class="pin' + (right ? " r" : "") + '"><span class="n">GPIO' + p + '</span>' +
      '<span class="u">' + what + "</span>" + tag + "</div>";
  }).join("");
  $("pin-left").innerHTML = draw(all.slice(0, half), false);
  $("pin-right").innerHTML = draw(all.slice(half), true);
  $("pin-board").textContent = config.mcu || "—";
  $("pin-mcu").textContent = (usable.length + inputOnly.length) + " pinos";
  $("tpl-btns").innerHTML = TEMPLATES.map((t) =>
    '<button class="btn" data-tpl="' + t.v + '">' + esc(t.n) + "</button>").join("");
}

/* Pins already spoken for by some OTHER feature; those must stay off the menu
   or two drivers would end up fighting over one GPIO. */
function pinsTakenByOthers(selfId) {
  const taken = {};
  for (const o of config.features || []) {
    if (o.id === selfId) continue;
    for (const p of o.outputs || []) taken[p] = 1;
    for (const p of o.inputs || []) taken[p] = 1;
  }
  return taken;
}

function pinSelect(f, fi, kind, slot, current) {
  const taken = pinsTakenByOthers(f.id);
  const opts = (config.usablePins || [])
    .filter((p) => !taken[p] || p === current)
    .map((p) => '<option value="' + p + '"' + (p === current ? " selected" : "") + ">GPIO" + p + "</option>")
    .join("");
  return '<select data-pin="' + kind + '" data-i="' + fi + '" data-k="' + slot + '">' + opts + "</select>";
}

/* Editable wiring. The firmware only re-maps pins it can validate, so a refused
   change simply comes back unchanged on the next read. */
function pinEditor(f, i) {
  const outs = f.outputs || [];
  const ins = f.inputs || [];
  if (!outs.length && !ins.length) return "";
  const block = (label, kind, pins) => !pins.length ? "" :
    '<div class="field"><label>' + label + "</label>" +
    '<div class="row2" style="grid-template-columns:repeat(' + Math.min(pins.length, 3) + ',1fr)">' +
    pins.map((p, k) => pinSelect(f, i, kind, k, p)).join("") + "</div></div>";
  return block("SAÍDAS (relé)", "out", outs) + block("ENTRADAS (botão/sensor)", "in", ins);
}

function renderFeatures() {
  const feats = config.features || [];
  $("feat-list").innerHTML = feats.length ? feats.map((f, i) => {
    const cover = isCover(f);
    return '<div class="card" style="margin-bottom:9px" data-fi="' + i + '">' +
      '<div class="field"><label>NOME</label><input data-f="name" data-i="' + i + '" maxlength="23" value="' + esc(f.name) + '"></div>' +
      '<div class="kv"><span>tipo</span><b>' + esc(driverLabel(f.driver)) + "</b></div>" +
      pinEditor(f, i) +
      (cover ? '<div class="row2"><div class="field"><label>SUBIDA (s)</label>' +
        '<input type="number" min="1" max="300" data-f="upCourseTime" data-i="' + i + '" value="' + (f.upCourseTime || 0) + '"></div>' +
        '<div class="field"><label>DESCIDA (s)</label>' +
        '<input type="number" min="1" max="300" data-f="downCourseTime" data-i="' + i + '" value="' + (f.downCourseTime || 0) + '"></div></div>' : "") +
      (isActuator(f) ? '<div class="field"><label>DESLIGAR SOZINHO (segundos, 0 = nunca)</label>' +
        '<input type="number" min="0" data-f="autoOff" data-i="' + i + '" value="' + (f.autoOff || 0) + '"></div>' +
        '<div class="field"><label>ENDEREÇO KNX (área / linha / membro)</label><div class="row2" style="grid-template-columns:1fr 1fr 1fr">' +
        '<input type="number" min="0" max="31" data-f="area" data-i="' + i + '" value="' + (f.area || 0) + '">' +
        '<input type="number" min="0" max="7" data-f="line" data-i="' + i + '" value="' + (f.line || 0) + '">' +
        '<input type="number" min="0" max="255" data-f="member" data-i="' + i + '" value="' + (f.member || 0) + '">' +
        "</div></div>" : "") +
      '<div class="btns"><button class="btn d" data-del="' + esc(f.id) + '">Remover</button></div></div>';
  }).join("") : '<div class="note">Ainda não há funções configuradas.</div>';
}

function renderDiag() {
  const heap = config.freeHeap;
  if (heap != null) {
    heapHistory.push(heap);
    if (heapHistory.length > 40) heapHistory.shift();
    $("d-heap").innerHTML = (heap / 1024).toFixed(1) + "<small>KB</small>";
    drawSpark("d-heap-spark", heapHistory, "#97d700");
    const frag = config.heapFrag;
    $("d-heap-note").textContent = frag != null
      ? "fragmentação " + frag + "% · maior bloco livre " + Math.round((config.maxFreeBlock || 0) / 1024) + " KB"
      : "";
  }
  $("d-reset").textContent = config.resetReason || "—";
  $("d-uptime").textContent = duration(config.uptime);
  $("d-fw").textContent = (config.firmware || "—") + (config.buildDate ? " · " + config.buildDate : "");
  $("d-sketch").textContent = config.sketchSize
    ? Math.round(config.sketchSize / 1024) + " KB · livre " + Math.round((config.freeSketchSpace || 0) / 1024) + " KB" : "—";
  $("d-ssid").textContent = config.wifiSSID || "—";
  $("d-rssi").textContent = rssiText(config.signal);
  $("d-ip").textContent = config.wifiIp || "—";
  $("d-net").textContent = (config.wifiMask || "—") + " / " + (config.wifiGw || "—");
  $("d-mqtt").textContent = config.mqttConnected ? "ligado" : "desligado";
  $("d-broker").textContent = (config.mqttIpDns || "—") + ":" + (config.mqttPort || "");
  $("d-cloud").textContent = config.cloudIOUsername ? "configurada" : "não configurada";
}

function drawSpark(id, values, colour) {
  const el = $(id);
  if (!el || values.length < 2) return;
  const min = Math.min.apply(null, values), max = Math.max.apply(null, values);
  const span = (max - min) || 1;
  const pts = values.map((v, i) =>
    (i / (values.length - 1) * 200).toFixed(1) + "," + (34 - (v - min) / span * 30).toFixed(1)).join(" ");
  el.innerHTML = '<polyline fill="none" stroke="' + colour + '" stroke-width="1.5" points="' + pts + '"/>';
}

function fillSystem() {
  $("s-nodeId").value = config.nodeId || "";
  $("s-ssid").value = config.wifiSSID || "";
  $("s-wpw").value = "";
  $("s-dhcp").checked = config.dhcp !== false;
  $("s-static").classList.toggle("hide", $("s-dhcp").checked);
  $("s-ip").value = config.wifiIp || "";
  $("s-mask").value = config.wifiMask || "";
  $("s-gw").value = config.wifiGw || "";
  $("s-mqttHost").value = config.mqttIpDns || "";
  $("s-mqttPort").value = config.mqttPort || 1883;
  $("s-mqttUser").value = config.mqttUsername || "";
  $("s-mqttPw").value = "";
  $("s-apiUser").value = config.apiUser || "";
  $("s-apiPw").value = "";
}

function fillNewFeatureForm() {
  const sel = $("nf-driver");
  sel.innerHTML = DRIVERS.map((g) => '<optgroup label="' + esc(g.g) + '">' +
    g.items.map((d) => '<option value="' + d.v + '">' + esc(d.n) + "</option>").join("") + "</optgroup>").join("");
  const used = {};
  for (const f of config.features || []) {
    for (const o of f.outputs || []) used[o] = 1;
    for (const i of f.inputs || []) used[i] = 1;
  }
  const pins = config.usablePins || [];
  const opts = pins.map((p) =>
    '<option value="' + p + '"' + (used[p] ? " disabled" : "") + ">GPIO" + p + (used[p] ? " (ocupado)" : "") + "</option>").join("");
  $("nf-p1").innerHTML = opts;
  $("nf-p2").innerHTML = opts;
  // A select whose options are all disabled just renders blank; say why.
  const free = pins.filter((p) => !used[p]).length;
  const msg = $("nf-msg");
  $("nf-add").disabled = free === 0;
  if (free === 0) {
    msg.className = "note err";
    msg.textContent = "Não há pinos livres. Liberta um pino noutra função para poder criar mais.";
  } else if (msg.textContent.indexOf("pinos livres") >= 0) {
    msg.className = "note";
    msg.textContent = "";
  }
  onDriverChange();
}
function onDriverChange() {
  const d = driverInfo(parseInt($("nf-driver").value, 10));
  $("nf-p2-box").classList.toggle("hide", !d || d.pins < 2);
}

/* ---------------- actions ---------------- */
async function control(id, state) {
  try { await api("/actuators/control", { method: "POST", body: JSON.stringify({ id: id, state: state }) }); }
  catch (e) { toast("Não foi possível comandar", "err"); }
}

async function addFeature() {
  const name = $("nf-name").value.trim();
  const driver = parseInt($("nf-driver").value, 10);
  const d = driverInfo(driver);
  const msg = $("nf-msg");
  if (!name) { msg.className = "note err"; msg.textContent = "Dá um nome à função."; return; }
  const p1 = parseInt($("nf-p1").value, 10);
  const p2 = d && d.pins > 1 ? parseInt($("nf-p2").value, 10) : undefined;
  if (d && d.pins > 1 && p1 === p2) {
    msg.className = "note err"; msg.textContent = "Os dois pinos têm de ser diferentes."; return;
  }
  msg.className = "note"; msg.textContent = "A criar…";
  const body = { name: name, driver: driver, input1: p1 };
  if (p2 !== undefined) body.input2 = p2;
  try {
    config = await api("/features", { method: "POST", body: JSON.stringify(body) });
    $("nf-name").value = "";
    msg.className = "note ok"; msg.textContent = "Criada.";
    renderOverview(); renderPinout(); renderFeatures(); fillNewFeatureForm();
    toast("Função criada", "ok");
  } catch (e) {
    msg.className = "note err";
    msg.textContent = featureError(e);
  }
}

/* prepareNewFeature() refuses with a numeric code; say which one it was. */
function featureError(e) {
  const code = e && e.body && e.body.result;
  if (code === 1) return "Dá um nome à função.";
  if (code === 2) return "Esse pino não serve nesta placa — escolhe outro.";
  if (code === 3) return "Tipo de função inválido para este firmware.";
  if (code === 4) return "Este tipo precisa de dois pinos.";
  return "O dispositivo recusou a função.";
}

async function save() {
  const body = JSON.parse(JSON.stringify(config));
  body.nodeId = $("s-nodeId").value.trim();
  body.wifiSSID = $("s-ssid").value.trim();
  if ($("s-wpw").value) body.wifiSecret = $("s-wpw").value;
  body.dhcp = $("s-dhcp").checked;
  body.wifiIp = $("s-ip").value.trim();
  body.wifiMask = $("s-mask").value.trim();
  body.wifiGw = $("s-gw").value.trim();
  body.mqttIpDns = $("s-mqttHost").value.trim();
  body.mqttPort = parseInt($("s-mqttPort").value, 10) || 1883;
  body.mqttUsername = $("s-mqttUser").value.trim();
  if ($("s-mqttPw").value) body.mqttPassword = $("s-mqttPw").value;
  body.apiUser = $("s-apiUser").value.trim();
  if ($("s-apiPw").value) body.apiPassword = $("s-apiPw").value;
  if (removed.length) body.featuresToRemove = removed;

  $("save-btn").disabled = true;
  $("save-note").textContent = "a guardar…";
  try {
    config = await api("/config", { method: "POST", body: JSON.stringify(body) });
    removed = [];
    clearDirty();
    $("save-note").textContent = "";
    toast("Guardado", "ok");
    renderHeader(); renderOverview(); renderPinout(); renderFeatures(); renderDiag(); fillSystem(); fillNewFeatureForm();
  } catch (e) {
    $("save-note").textContent = "";
    toast("Não foi possível guardar", "err");
  } finally {
    $("save-btn").disabled = false;
  }
}

/* confirm() is silently blocked inside sandboxed frames and is easy to misfire
   on a phone, so destructive buttons ask for a second click on themselves. */
function armed(btn, label) {
  if (btn.dataset.armed === "1") {
    clearTimeout(btn._disarm);
    btn.dataset.armed = "";
    btn.textContent = btn.dataset.label;
    btn.classList.remove("d");
    return true;
  }
  btn.dataset.label = btn.textContent;
  btn.dataset.armed = "1";
  btn.textContent = label;
  btn.classList.add("d");
  clearTimeout(btn._disarm);
  btn._disarm = setTimeout(() => {
    btn.dataset.armed = "";
    btn.textContent = btn.dataset.label;
    btn.classList.remove("d");
  }, 4000);
  return false;
}

async function applyTemplate(v) {
  try {
    await api("/templates/change?t=" + v, { method: "POST" });
    toast("Predefinição aplicada", "ok");
    setTimeout(load, 600);
  } catch (e) { toast("Não foi possível aplicar", "err"); }
}

function exportConfig() {
  const blob = new Blob([JSON.stringify(config, null, 2)], { type: "application/json" });
  const a = document.createElement("a");
  a.href = URL.createObjectURL(blob);
  a.download = (config.nodeId || "onofre") + "-config.json";
  a.click();
  URL.revokeObjectURL(a.href);
}

/* ---------------- live updates ---------------- */
function addLog(kind, text) {
  if (logPaused) return;
  const now = new Date().toTimeString().slice(0, 8);
  logLines.push(now + "  " + text);
  if (logLines.length > 200) logLines.shift();
  const el = $("d-log");
  const atBottom = el.scrollTop + el.clientHeight >= el.scrollHeight - 20;
  el.innerHTML = logLines.map((l) => {
    const k = l.indexOf("[erro]") >= 0 ? "e" : l.indexOf("[aviso]") >= 0 ? "w" : "i";
    return '<div><span class="t">' + esc(l.slice(0, 8)) + '</span> <span class="' + k + '">' +
      esc(l.slice(10)) + "</span></div>";
  }).join("");
  if (atBottom) el.scrollTop = el.scrollHeight;
}

function connectEvents() {
  if (!window.EventSource) return;
  source = new EventSource(baseUrl + "/events");
  source.addEventListener("mqtt_health", (e) => {
    const on = e.data === "online";
    setMqttPill(on);
    $("d-mqtt").textContent = on ? "ligado" : "desligado";
    addLog("i", "[mqtt] " + (on ? "ligado" : "desligado"));
  });
  // Every feature publishes its state under its own id.
  source.onmessage = () => {};
  source.addEventListener("error", () => addLog("i", "[aviso] ligação de eventos interrompida"));
  // Feature events are named after the uniqueId, so they are wired per feature.
  wireFeatureEvents();
}
function wireFeatureEvents() {
  for (const f of config.features || []) {
    (function (feat) {
      source.addEventListener(feat.id, (e) => {
        feat.state = e.data;
        if (isActuator(feat)) {
          const sw = document.querySelector('[data-toggle="' + feat.id + '"]');
          if (sw) sw.classList.toggle("on", (parseInt(e.data, 10) || 0) > 0);
          const rng = document.querySelector('[data-cover="' + feat.id + '"]');
          if (rng) rng.value = parseInt(e.data, 10) || 0;
        } else if (isEnergy(feat)) {
          // The whole card is derived from the payload, so redraw it in place.
          const el = $("sv-" + feat.id);
          const card = el && el.closest(".card");
          if (card) card.outerHTML = energyCard(feat);
        } else {
          const el = $("sv-" + feat.id);
          if (el) el.textContent = sensorText(e.data);
        }
        addLog("i", "[" + feat.name + "] " + String(e.data).slice(0, 90));
      });
    })(f);
  }
}

/* ---------------- wiring ---------------- */
document.addEventListener("click", (ev) => {
  const tab = ev.target.closest("[data-view]");
  if (tab) {
    document.querySelectorAll(".tab").forEach((t) => t.classList.toggle("on", t === tab));
    document.querySelectorAll(".view").forEach((v) => v.classList.toggle("on", v.id === "v-" + tab.dataset.view));
    return;
  }
  const sw = ev.target.closest("[data-toggle]");
  if (sw) { sw.classList.toggle("on"); control(sw.dataset.toggle, 102); return; }
  const tpl = ev.target.closest("[data-tpl]");
  if (tpl) {
    if (armed(tpl, "Substituir tudo?")) applyTemplate(parseInt(tpl.dataset.tpl, 10));
    return;
  }
  const del = ev.target.closest("[data-del]");
  if (del) {
    if (!armed(del, "Confirmar?")) return;
    removed.push(del.dataset.del);
    config.features = (config.features || []).filter((f) => f.id !== del.dataset.del);
    renderFeatures(); renderOverview(); renderPinout(); markDirty();
    return;
  }
});

document.addEventListener("change", (ev) => {
  const cover = ev.target.closest("[data-cover]");
  if (cover) { control(cover.dataset.cover, Math.abs(parseInt(cover.value, 10) - 100)); return; }
  const pin = ev.target.closest("[data-pin]");
  if (pin) {
    const feat = config.features[parseInt(pin.dataset.i, 10)];
    if (feat) {
      const list = pin.dataset.pin === "out" ? feat.outputs : feat.inputs;
      list[parseInt(pin.dataset.k, 10)] = parseInt(pin.value, 10);
      renderFeatures();   // other selects must drop the pin just claimed
      markDirty();
    }
    return;
  }
  const f = ev.target.closest("[data-f]");
  if (f) {
    const i = parseInt(f.dataset.i, 10);
    const key = f.dataset.f;
    const val = f.type === "number" ? (parseInt(f.value, 10) || 0) : f.value;
    if (config.features[i]) { config.features[i][key] = val; markDirty(); }
    return;
  }
  if (ev.target.id === "nf-driver") { onDriverChange(); return; }
  if (ev.target.id === "s-dhcp") { $("s-static").classList.toggle("hide", ev.target.checked); markDirty(); return; }
  if (ev.target.closest("#v-system")) markDirty();
});

window.addEventListener("beforeunload", (e) => {
  if (dirty) { e.preventDefault(); e.returnValue = ""; }
});

document.addEventListener("DOMContentLoaded", () => {
  $("save-btn").onclick = save;
  $("nf-add").onclick = addFeature;
  $("a-export").onclick = exportConfig;
  $("d-log-pause").onclick = (e) => {
    logPaused = !logPaused;
    e.target.textContent = logPaused ? "Retomar" : "Pausar";
  };
  $("d-log-clear").onclick = () => { logLines = []; $("d-log").innerHTML = ""; };
  $("d-log-copy").onclick = () => {
    navigator.clipboard.writeText(logLines.join("\n")).then(
      () => toast("Registo copiado", "ok"), () => toast("Não foi possível copiar", "err"));
  };
  $("a-reboot").onclick = async (e) => {
    if (!armed(e.currentTarget, "Reiniciar?")) return;
    try { await api("/reboot", { method: "POST" }); toast("A reiniciar…", "ok"); } catch (err) { toast("Falhou", "err"); }
  };
  $("a-update").onclick = async (e) => {
    if (!armed(e.currentTarget, "Atualizar?")) return;
    try { await api("/auto-update", { method: "POST" }); toast("A atualizar — não desligues", "ok"); }
    catch (err) { toast("Falhou", "err"); }
  };
  $("a-defaults").onclick = async (e) => {
    if (!armed(e.currentTarget, "Apagar tudo?")) return;
    try { await api("/load-defaults", { method: "POST" }); toast("A repor…", "ok"); } catch (err) { toast("Falhou", "err"); }
  };
  load().then(connectEvents);
  // Diagnostics are a snapshot; refresh them while the tab is open.
  setInterval(() => {
    if ($("v-diag").classList.contains("on")) {
      api("/config").then((c) => { config = Object.assign(config, c); renderHeader(); renderDiag(); }).catch(() => {});
    }
  }, 10000);
});
