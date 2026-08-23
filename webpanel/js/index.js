let baseUrl = (location.origin && location.origin !== "null") ? location.origin : "";
/* OnOfre device panel.
   The build strips this first line and injects `let baseUrl = ""` so the embedded
   copy talks to whatever address the device was reached at. */

var config = {};          // last config read from the device
var dirty = false;        // unsaved edits in the forms
var removed = [];         // feature ids queued for removal
var heapHistory = [];     // free heap samples, for the sparkline
var climateHistory = {};  // browser-only samples, keyed by feature id
var logPaused = false;
var logLines = [];
var source = null;
var featureEventHandlers = [];

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

/* Inline so the panel keeps working with no network: an icon set is the first
   thing a remote stylesheet would take away. */
const COG = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round">' +
  '<circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.6 1.6 0 0 0 .3 1.8l.1.1a2 2 0 1 1-2.8 2.8l-.1-.1a1.6 1.6 0 0 0-2.7 1.1V21a2 2 0 1 1-4 0v-.1A1.6 1.6 0 0 0 7.5 19l-.1.1a2 2 0 1 1-2.8-2.8l.1-.1A1.6 1.6 0 0 0 3 13.6H3a2 2 0 1 1 0-4h.1A1.6 1.6 0 0 0 4.7 7l-.1-.1a2 2 0 1 1 2.8-2.8l.1.1a1.6 1.6 0 0 0 1.8.3H10a1.6 1.6 0 0 0 1-1.5V3a2 2 0 1 1 4 0v.1a1.6 1.6 0 0 0 2.7 1.1l.1-.1a2 2 0 1 1 2.8 2.8l-.1.1a1.6 1.6 0 0 0-.3 1.8V10a1.6 1.6 0 0 0 1.5 1H21a2 2 0 1 1 0 4h-.1a1.6 1.6 0 0 0-1.5 1Z"/></svg>';
const ICONS = {
  light: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"><path d="M9 18h6M10 22h4M12 2a7 7 0 0 0-4 12.7V18h8v-3.3A7 7 0 0 0 12 2Z"/></svg>',
  socket: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"><rect x="3" y="3" width="18" height="18" rx="4"/><circle cx="9" cy="10" r="1.4"/><circle cx="15" cy="10" r="1.4"/><path d="M8 16h8"/></svg>',
  cover: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M3 8h18M3 12h18M3 16h18"/></svg>',
  garage: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"><path d="M3 21V9l9-5 9 5v12M7 21v-7h10v7M7 17h10"/></svg>',
  valve: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"><path d="M12 22a5 5 0 0 0 5-5c0-3.5-5-11-5-11S7 13.5 7 17a5 5 0 0 0 5 5Z"/></svg>',
};
function tileIcon(f) {
  const d = String(f.driver || "");
  if (d.indexOf("COVER") === 0) return ICONS.cover;
  if (d.indexOf("GARAGE") === 0) return ICONS.garage;
  if (d.indexOf("GARDEN") === 0) return ICONS.valve;
  if (d.indexOf("LIGHT") === 0) return ICONS.light;
  return ICONS.socket;
}

/* ---------------- irrigation ---------------- */
/* Programs run on the device, not in the cloud: a cycle that stops halfway
   because MQTT dropped would leave one zone soaked and the rest dry. The panel
   only edits and shows them. */
var irrigation = null;      // last state read from the device
var irrDirty = false;

const WEEKDAY_LABELS = ["D", "S", "T", "Q", "Q", "S", "S"];
const WEEKDAY_NAMES = ["domingo", "segunda", "terça", "quarta", "quinta", "sexta", "sábado"];

const isZone = (f) => f.driver === "GARDEN_VALVE";
const zones = () => (config.features || []).filter(isZone);
const zoneName = (id) => (zones().find((z) => z.id === id) || {}).name || id;

function hhmm(minuteOfDay) {
  const m = Math.max(0, Math.min(1439, parseInt(minuteOfDay, 10) || 0));
  return String(Math.floor(m / 60)).padStart(2, "0") + ":" + String(m % 60).padStart(2, "0");
}
function minutesFromHhmm(v) {
  const m = /^(\d{1,2}):(\d{2})$/.exec(String(v || "").trim());
  if (!m) return null;
  const h = parseInt(m[1], 10), min = parseInt(m[2], 10);
  if (h > 23 || min > 59) return null;
  return h * 60 + min;
}

function markIrrDirty() {
  irrDirty = true;
  $("irr-save").disabled = false;
}

/* The tab only exists on a board that has valves, so a light switch panel is
   not cluttered with irrigation it will never use. */
function renderIrrigationTab() {
  const has = zones().length > 0;
  $("tab-irrigation").classList.toggle("hide", !has);
  if (!has) return;
  irrigation = config.irrigation || { enabled: true, skipOnRain: true, programs: [] };
  irrDirty = false;
  $("irr-save").disabled = true;
  renderIrrStatus();
  renderIrrZones();
  renderIrrPrograms();
}

function renderIrrStatus() {
  const run = irrigation.running;
  const rain = (config.features || []).find((f) => f.driver === "RAIN");
  const raining = rain ? /rain/i.test(String(rain.state)) : false;
  // A schedule with no clock is the one failure people cannot guess at, so it
  // leads.
  const noClock = !config.clockSynced;
  $("irr-status").innerHTML =
    '<div class="card irr-state">' +
    (noClock
      ? '<div class="note err" style="margin:0 0 10px">Sem hora certa (NTP), os programas não correm. ' +
        "A rega manual continua disponível.</div>"
      : "") +
    (run
      ? '<div class="irr-running"><b>' + esc(zoneName(run.zone)) + "</b>" +
        '<span>a regar · faltam ' + duration(run.secondsLeft) + "</span></div>"
      : '<div class="irr-running idle"><b>Parado</b><span>' +
        (irrigation.enabled ? "à espera do próximo programa" : "programas desligados") + "</span></div>") +
    '<div class="irr-toggles">' +
    '<label class="f-check"><input type="checkbox" id="irr-enabled"' + (irrigation.enabled ? " checked" : "") +
      "> programas ativos</label>" +
    '<label class="f-check"><input type="checkbox" id="irr-rain"' + (irrigation.skipOnRain ? " checked" : "") +
      "> saltar o ciclo se estiver a chover" +
      (raining ? ' <span class="irr-chip">está a chover</span>' : "") + "</label>" +
    "</div>" +
    (run ? '<div class="btns" style="margin-top:11px"><button class="btn d" id="irr-stop">Parar rega</button></div>' : "") +
    "</div>";
}

function renderIrrZones() {
  const run = irrigation.running;
  $("irr-zones").innerHTML = zones().map((z) => {
    const on = String(z.state) === "100";
    const pin = (z.outputs || []).length ? "OUT " + z.outputs.join(",") : "";
    const btn = String(z.inputs || "") ? " · IN " + z.inputs.join(",") : "";
    return '<div class="tile-wrap"><div class="tile' + (on ? " on" : "") + '">' +
      '<span class="tile-top">' + ICONS.valve + "</span>" +
      "<b>" + esc(z.name) + "</b>" +
      '<span class="tile-sub">' + (on ? (run && run.zone === z.id ? "a regar · " + duration(run.secondsLeft) : "aberta") : "fechada") + "</span>" +
      '<div class="btns" style="margin-top:8px">' +
      '<button class="btn' + (on ? " d" : "") + '" data-zone="' + esc(z.id) + '">' +
      (on ? "Fechar" : "Regar agora") + "</button></div>" +
      '<span class="tile-pins">' + esc(pin + btn) + "</span></div></div>";
  }).join("");
}

function renderIrrPrograms() {
  const list = irrigation.programs || [];
  $("irr-programs").innerHTML = list.length ? list.map((prog, i) => {
    // A program with no zones or no days is silently dead; say so where the
    // total would be, which is where the eye already goes.
    const warn = !(prog.zones || []).length || !prog.weekdays;
    return '<div class="card irr-prog" data-pi="' + i + '">' +
    '<div class="irr-prog-head">' +
      '<label class="f-check"><input type="checkbox" data-ip="enabled" data-pi="' + i + '"' +
        (prog.enabled ? " checked" : "") + "> Programa " + (i + 1) + "</label>" +
      '<div class="btns irr-prog-actions">' +
      ((prog.zones || []).length
        ? '<button class="btn" data-iprun="' + prog.id + '">Regar agora</button>' : "") +
      '<span class="irr-actions-gap"></span>' +
      '<button class="btn d" data-ipdel="' + i + '">Remover</button></div>' +
    "</div>" +
    '<div class="row2">' +
      '<div class="field"><label>HORA DE INÍCIO</label>' +
      '<input type="time" data-ip="start" data-pi="' + i + '" value="' + hhmm(prog.startMinute) + '"></div>' +
      '<div class="field"><label>DIAS</label><div class="irr-days">' +
      WEEKDAY_LABELS.map((lbl, d) =>
        '<button type="button" class="irr-day' + ((prog.weekdays >> d) & 1 ? " on" : "") +
        '" data-ipday="' + i + ":" + d + '" title="' + WEEKDAY_NAMES[d] + '">' + lbl + "</button>").join("") +
      "</div></div>" +
    "</div>" +
    '<div class="sub">ZONAS E DURAÇÃO</div>' +
    zones().map((z) => {
      const entry = (prog.zones || []).find((x) => x.uniqueId === z.id);
      return '<div class="irr-zline">' +
        '<label class="f-check"><input type="checkbox" data-ipz="' + i + ":" + esc(z.id) + '"' +
          (entry ? " checked" : "") + "> " + esc(z.name) + "</label>" +
        '<input type="number" min="1" max="240" data-ipmin="' + i + ":" + esc(z.id) + '" value="' +
          (entry ? entry.minutes : 10) + '"' + (entry ? "" : " disabled") + '><span>min</span></div>';
    }).join("") +
    '<div class="note' + (warn ? " err" : "") + '">' +
    (!(prog.zones || []).length
      ? "Sem zonas escolhidas — este programa não rega nada."
      : !prog.weekdays
        ? "Sem dias escolhidos — este programa não corre."
        : "Total " + programTotal(prog) + " min, por esta ordem.") +
    (prog.enabled ? "" : " Programa desligado.") + "</div>" +
    "</div>";
  }).join("")
    : '<div class="note">Ainda não há programas. Cria um para a rega correr sozinha.</div>';
}

/* Editing a field must not rebuild the list it lives in: innerHTML replaces the
   input being typed into, so the focus is lost and a half-entered time snaps
   back. Only the derived bits are refreshed in place. */
function refreshProgramNote(i) {
  const card = document.querySelector('.irr-prog[data-pi="' + i + '"]');
  if (!card) return;
  const prog = irrigation.programs[i];
  const note = card.querySelector(".note");
  if (!note) return;
  const dead = !(prog.zones || []).length || !prog.weekdays;
  note.className = "note" + (dead ? " err" : "");
  note.textContent =
    (!(prog.zones || []).length
      ? "Sem zonas escolhidas — este programa não rega nada."
      : !prog.weekdays
        ? "Sem dias escolhidos — este programa não corre."
        : "Total " + programTotal(prog) + " min, por esta ordem.") +
    (prog.enabled ? "" : " Programa desligado.");
}

function programTotal(prog) {
  return (prog.zones || []).reduce((a, z) => a + (parseInt(z.minutes, 10) || 0), 0);
}

async function saveIrrigation() {
  const body = {
    enabled: $("irr-enabled").checked,
    skipOnRain: $("irr-rain").checked,
    programs: irrigation.programs || [],
  };
  const msg = $("irr-msg");
  msg.className = "note";
  msg.textContent = "A guardar…";
  try {
    irrigation = await api("/irrigation", { method: "POST", body: JSON.stringify(body) });
    config.irrigation = irrigation;
    irrDirty = false;
    $("irr-save").disabled = true;
    msg.className = "note ok";
    msg.textContent = "Programas guardados no equipamento.";
    renderIrrStatus();
    renderIrrPrograms();
  } catch (e) {
    if (e.auth) return;
    msg.className = "note err";
    msg.textContent = "O equipamento recusou os programas.";
  }
}

async function runProgramNow(programId) {
  // The equipment runs its own copy of the program, by id. Asking it to run one
  // it has never been told about cannot work — and reloading afterwards used to
  // throw away the unsaved edits, which looked exactly like the program being
  // deleted by the button that was supposed to start it.
  if (irrDirty) { toast("Guarda os programas primeiro", "err"); return; }
  try {
    irrigation = await api("/irrigation-run", { method: "POST", body: JSON.stringify({ programId: programId }) });
    config.irrigation = irrigation;
    renderIrrStatus();
    renderIrrZones();
  } catch (e) { toast("Não foi possível arrancar o programa", "err"); }
}

async function stopIrrigation() {
  try {
    irrigation = await api("/irrigation-stop", { method: "POST" });
    config.irrigation = irrigation;
    // Only the state and the zones change; the programs on screen are untouched,
    // including anything being edited.
    renderIrrStatus();
    renderIrrZones();
  } catch (e) { toast("Não foi possível parar", "err"); }
}

/* The device answers first and performs the update in its main loop. The safe
   updater temporarily stops the web server, so a missed poll means only "still
   unavailable" — never success. Success is confirmed by a new firmware version
   (or an explicit done state); failures are shown after the old server returns. */
function followUpdate() {
  const bar = $("ota-bar");
  const fill = bar.querySelector("i");
  const msg = $("ota-msg");
  const startingVersion = String(config.firmware || "");
  const deadline = Date.now() + 180000;
  bar.classList.remove("hide");
  bar.classList.add("indeterminate");
  bar.removeAttribute("aria-valuenow");
  bar.setAttribute("aria-valuetext", "A descarregar e instalar");
  fill.style.width = "0";
  msg.className = "note";
  msg.textContent = "A descarregar e instalar — o painel ficará temporariamente indisponível. Não desligues o equipamento.";
  $("a-update").disabled = true;

  let finished = false;
  const setProgress = (percent) => {
    const value = Math.max(0, Math.min(100, Number(percent) || 0));
    bar.classList.remove("indeterminate");
    fill.style.width = value + "%";
    bar.setAttribute("aria-valuenow", String(value));
    bar.setAttribute("aria-valuetext", value + "%");
  };
  const finish = (ok, text, percent) => {
    finished = true;
    bar.classList.remove("indeterminate");
    if (percent != null) setProgress(percent);
    else bar.setAttribute("aria-valuetext", text);
    msg.className = ok ? "note ok" : "note err";
    msg.textContent = text;
    $("a-update").disabled = false;
  };

  const poll = async () => {
    if (finished) return;
    let snapshot;
    try {
      // Safari may otherwise reuse the pre-reboot snapshot and leave the bar
      // waiting at the end even though the new firmware is already online.
      snapshot = await api("/config?otaPoll=" + Date.now(), { cache: "no-store" });
    } catch (e) {
      if (Date.now() >= deadline) {
        finish(false, "Não foi possível confirmar o resultado. Volta a abrir a página e verifica a versão instalada.");
        return;
      }
      setTimeout(poll, 1500);
      return;
    }

    const ota = snapshot.ota || null;
    if (startingVersion && snapshot.firmware && snapshot.firmware !== startingVersion) {
      finish(true, "Atualização confirmada · versão " + snapshot.firmware, 100);
      return;
    }
    if (!ota) {
      if (Date.now() >= deadline)
        finish(false, "O equipamento respondeu, mas não confirmou a atualização.");
      else
        setTimeout(poll, 1500);
      return;
    }
    if (ota.state === "running" && ota.percent > 0) setProgress(ota.percent);
    if (ota.state === "failed") {
      if (ota.percent > 0) setProgress(ota.percent);
      finish(false, "A atualização falhou: " + (ota.error || "sem detalhe do equipamento"));
    } else if (ota.state === "done") {
      finish(true, "Gravado. A reiniciar…", 100);
    } else if (Date.now() >= deadline) {
      finish(false, "A atualização não terminou dentro do tempo esperado. Verifica a versão antes de repetir.");
    } else {
      setTimeout(poll, 1500);
    }
  };
  setTimeout(poll, 1500);
}

/* The device keeps its own short log; this is the only way to read it without a
   cable, which is the whole point. Plain text on purpose: it goes straight into a
   message. */
async function loadDeviceLog() {
  const box = $("d-devlog");
  box.textContent = "";
  try {
    const res = await fetch(baseUrl + "/logs", { headers: { Accept: "text/plain" } });
    if (!res.ok) throw new Error("HTTP " + res.status);
    const text = (await res.text()).trim();
    if (!text) {
      box.innerHTML = '<div class="dim">o equipamento ainda não registou nada</div>';
      return;
    }
    box.innerHTML = text.split("\n").map((l) => "<div>" + esc(l) + "</div>").join("");
    box.scrollTop = box.scrollHeight;
  } catch (e) {
    box.innerHTML = '<div class="e">não foi possível ler o registo do equipamento</div>';
  }
}

/* ---------------- theme ---------------- */
/* The choice lives in localStorage, not in the device config: it belongs to
   whoever is looking, and writing it to the device would burn flash on a
   preference the next browser would not share. The <head> applies it before
   the first paint; this only handles the toggle. */
const THEME_KEY = "onofre-theme";
const SUN = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round">' +
  '<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M2 12h2M20 12h2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M19.1 4.9l-1.4 1.4M6.3 17.7l-1.4 1.4"/></svg>';
const MOON = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">' +
  '<path d="M21 12.8A9 9 0 1 1 11.2 3a7 7 0 0 0 9.8 9.8Z"/></svg>';

function currentTheme() {
  return document.documentElement.getAttribute("data-theme") === "light" ? "light" : "dark";
}
function applyTheme(name) {
  document.documentElement.setAttribute("data-theme", name);
  try { localStorage.setItem(THEME_KEY, name); } catch (e) { /* private mode */ }
  const btn = $("theme-btn");
  if (btn) {
    // Show what the click will do, not what is on screen.
    btn.innerHTML = name === "light" ? MOON : SUN;
    btn.title = name === "light" ? "Tema escuro" : "Tema claro";
  }
  drawSpark("d-heap-spark", heapHistory, accent());
}
/* Charts are drawn, not styled, so they have to read the token themselves. */
function accent() {
  return getComputedStyle(document.documentElement).getPropertyValue("--grn").trim() || "#97d700";
}

/* ---------------- helpers ---------------- */
function toast(msg, kind) {
  const t = $("toast");
  const isError = kind === "err";
  t.setAttribute("role", isError ? "alert" : "status");
  t.setAttribute("aria-live", isError ? "assertive" : "polite");
  $("toast-msg").textContent = msg;
  t.className = "on " + (kind || "");
  clearTimeout(toast._t);
  // Errors need enough time to be read on a phone; every toast can also be
  // dismissed immediately without stealing keyboard focus from the form.
  toast._t = setTimeout(dismissToast, isError ? 9000 : 4200);
}
function dismissToast() {
  clearTimeout(toast._t);
  $("toast").className = "";
}
/* The save button lives in the header and starts disabled, so both of these
   have to drive it — otherwise there is nothing to press. */
function markDirty() {
  dirty = true;
  $("dirty").classList.remove("hide");
  $("save-btn").disabled = false;
}
function clearDirty() {
  dirty = false;
  $("dirty").classList.add("hide");
  $("save-btn").disabled = true;
}

/* A diagnostics refresh returns the complete config, including features. Those
   feature objects may contain unsaved name, pin, or timing edits, so replacing
   them here would make the later Save silently send the old device values. */
function applyDiagnosticsSnapshot(snapshot) {
  Object.keys(snapshot || {}).forEach((key) => {
    if (key !== "features") config[key] = snapshot[key];
  });
}

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
  let res;
  const busyDeadline = Date.now() + 2000;
  for (;;) {
    res = await fetch(baseUrl + path, Object.assign(
      { headers: { "Content-Type": "application/json", "Accept": "application/json" } }, opts || {}));
    if (res.status !== 409 || Date.now() >= busyDeadline) break;
    // A failed acquisition reserves the next lease for foreground work. Retry
    // beyond the 100 ms handoff window so a long-running current owner can
    // finish without making a transient feature-loop collision user-visible.
    await new Promise(resolve => setTimeout(resolve, 120));
  }
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
  renderIrrigationTab();
  wireFeatureEvents();
}

function renderHeader() {
  $("h-name").textContent = config.nodeId || "—";
  $("h-chip").textContent = config.chipId || "";
  $("h-mcu").textContent = config.mcu || "";
  $("h-fw").textContent = config.firmware || "—";
  const w = $("h-wifi");
  w.className = "pill " + rssiClass(config.signal);
  w.innerHTML = "WiFi <b>" + esc(rssiText(config.signal)) + "</b>";
  setMqttPill(config.mqttConnected);
  $("h-up").innerHTML = "ligado há <b>" + duration(config.uptime) + "</b>";
}
function setFirmwareHeaderState(state, latest) {
  const badge = $("h-fw-link");
  const latestLabel = $("h-fw-latest");
  badge.classList.remove("available", "error");
  badge.disabled = true;
  latestLabel.textContent = "";
  if (state === "available") {
    badge.classList.add("available");
    badge.disabled = false;
    latestLabel.textContent = latest;
    badge.title = "Atualização " + latest + " disponível — abrir Firmware";
    badge.setAttribute("aria-label", "Atualização de firmware " + latest + " disponível. Abrir Firmware");
  } else if (state === "error") {
    badge.classList.add("error");
    badge.title = "Não foi possível verificar atualizações";
    badge.setAttribute("aria-label", "Não foi possível verificar atualizações");
  } else {
    badge.title = "Firmware instalado";
    badge.setAttribute("aria-label", "Firmware instalado " + (config.firmware || "desconhecido"));
  }
}
function setStatusBadge(el, state, text) {
  if (!el) return;
  el.className = "status-badge " + state;
  el.textContent = text;
}
function setMqttPill(on) {
  const configured = Boolean((config.mqttIpDns || "").trim());
  const state = !configured ? "off" : on ? "ok" : "bad";
  const text = !configured ? "desativado" : on ? "ligado" : "sem ligação";
  const m = $("h-mqtt");
  m.className = "pill " + state;
  m.innerHTML = "MQTT <b>" + text + "</b>";
  setStatusBadge($("d-mqtt"), state, text);
}

const isActuator = (f) => f.group === "ACTUATOR";
const isCover = (f) => (f.driver || "").indexOf("COVER") === 0;
/* ActuatorControlType::VIRTUAL. A virtual switch drives no output of its own: it
   is a wall button that commands other relays over KNX. Showing it among the
   accessories offers a switch whose state reflects nothing in the house, so the
   dashboard leaves it out. It stays visible where it belongs — in FUNÇÕES, where
   it is configured, and in the pinout, where its input is spoken for. */
const isVirtual = (f) => f.typeControl === 2;
const isDashboardAccessory = (f) => isActuator(f) && !isVirtual(f);
const tileAction = (on) => on ? "desligar" : "ligar";

function setToggleTileState(tile, on) {
  tile.classList.toggle("on", on);
  tile.setAttribute("aria-pressed", String(on));
  const action = tileAction(on);
  const subtitle = tile.querySelector(".tile-sub");
  if (subtitle) subtitle.textContent = action;
  const name = tile.querySelector("b");
  tile.setAttribute("aria-label", action + (name ? " " + name.textContent : " acessório"));
}

/* Jump from a tile straight to that feature's settings, highlighted so it is
   obvious which of a long list was meant. */
function openFeatureConfig(id) {
  document.querySelector('[data-view="features"]').click();
  const idx = (config.features || []).findIndex((f) => f.id === id);
  if (idx < 0) return;
  const card = document.querySelector('[data-fi="' + idx + '"]');
  if (!card) return;
  card.scrollIntoView({ behavior: "smooth", block: "center" });
  card.classList.add("flash");
  setTimeout(() => card.classList.remove("flash"), 1600);
  const name = card.querySelector('[data-f="name"]');
  if (name) name.focus();
}

function renderOverview() {
  const feats = config.features || [];
  const acts = feats.filter(isDashboardAccessory);
  const sens = feats.filter((f) => !isActuator(f));

  // Tiles rather than full-width rows: an accessory is a small thing and reads
  // better as a grid, the way a phone's home app lays them out. Each tile is
  // wrapped because the cog cannot live inside the button that toggles it.
  $("ov-actuators").innerHTML = acts.length ? '<div class="tiles">' + acts.map((f) => {
    const state = parseInt(f.state, 10) || 0;
    const pins = (f.outputs || []).length ? "OUT " + f.outputs.join(",") : "";
    const ins = (f.inputs || []).length ? "IN " + f.inputs.join(",") : "";
    const meta = '<span class="tile-pins">' + esc([pins, ins].filter(Boolean).join(" · ")) + "</span>";
    const cog = '<button type="button" class="cog" data-config="' + esc(f.id) +
      '" title="Configurar" aria-label="Configurar ' + esc(f.name) + '">' + COG + "</button>";
    // Firmware state for a cover is how CLOSED it is (OFF_OPEN=0, ON_CLOSE=100);
    // the panel speaks in "% open", so convert here and on the way back.
    const openPct = isCover(f) ? 100 - state : state;
    const body = isCover(f)
      ? '<div class="tile cover' + (openPct > 0 ? " on" : "") + '">' +
        '<span class="tile-top">' + tileIcon(f) + "</span>" +
        "<b>" + esc(f.name) + "</b>" +
        '<span class="tile-sub">' + openPct + "% aberto</span>" +
        '<input type="range" min="0" max="100" value="' + openPct + '" data-cover="' + esc(f.id) + '">' +
        meta + "</div>"
      // The tile itself is the switch, so the whole surface is the target.
      : '<button type="button" class="tile tap' + (state > 0 ? " on" : "") + '" data-toggle="' +
        esc(f.id) + '" aria-pressed="' + (state > 0 ? "true" : "false") +
        '" aria-label="' + tileAction(state > 0) + " " + esc(f.name) + '">' +
        '<span class="tile-top">' + tileIcon(f) + "</span>" +
        "<b>" + esc(f.name) + "</b>" +
        '<span class="tile-sub">' + tileAction(state > 0) + "</span>" +
        meta + "</button>";
    return '<div class="tile-wrap">' + body + cog + "</div>";
  }).join("") + "</div>" : '<div class="note">Sem acessórios configurados.</div>';

  $("ov-sensors-title").classList.toggle("hide", !sens.length);
  $("ov-sensors").innerHTML = sens.map((f) => isEnergy(f) ? energyCard(f) :
    isClimate(f) ? climateCard(f, true) :
      // A reading with a unit gets the same room as a climate card; a door or a
      // motion sensor is one word and stays narrow.
      '<div class="card' + (MEASURE_DRIVERS.indexOf(f.driver) >= 0 ? " wide" : "") +
      '"><h4>' + esc(f.name) + '</h4><div class="fval" id="sv-' + esc(f.id) + '">' +
      esc(sensorText(f.state, f.driver)) + "</div></div>").join("");
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
/* A meter reads as three things at once — what it draws, what it returns, and
   which period it is billing — so the card leads with those instead of burying
   them in a list. Import and export sit either side of the meter's own health,
   the tariff periods carry a traffic light against the contracted power, and the
   raw instantaneous values follow for whoever wants them. */
const TARIFF_PERIODS = [
  { k: "rate1", n: "Vazio", t: 1 },
  { k: "rate2", n: "Cheias", t: 2 },
  { k: "rate3", n: "Ponta", t: 3 },
];
const ADVANCED_FIELDS = [
  { k: "power", n: "potência", u: " W", d: 0 },
  { k: "voltage", n: "tensão", u: " V", d: 1 },
  { k: "current", n: "corrente", u: " A", d: 2 },
  { k: "frequency", n: "frequência", u: " Hz", d: 1 },
  { k: "powerFactor", n: "fator de potência", u: "", d: 3 },
  { k: "energy", n: "energia", u: " kWh", d: 2 },
];
const TOTALS_FIELDS = [
  { k: "powerImport", n: "energia importada", u: " kWh", d: 2 },
  { k: "powerExport", n: "energia exportada", u: " kWh", d: 2 },
];

function metric(value, unit, label, cls) {
  return '<div class="metric ' + (cls || "") + '">' +
    '<b>' + esc(value) + '<small>' + unit + "</small></b>" +
    '<span>' + esc(label) + "</span></div>";
}
function fieldRows(o, fields) {
  return fields.filter((x) => o[x.k] != null).map((x) =>
    '<div class="kv"><span>' + x.n + "</span><b>" +
    Number(o[x.k]).toFixed(x.d) + x.u + "</b></div>").join("");
}

function energyCard(f) {
  const o = parseState(f.state) || {};
  const imp = Number(o.power || 0);
  const exp = Number(o.export || 0);
  const exporting = exp > 0 && imp <= 0;
  const bad = o.status && o.status !== "HAN OK" ? o.status : (o.error ? "erro de leitura" : "");

  // Headline: drawn, health, returned. The live value the SSE handler tracks is
  // whichever direction is active.
  const head = '<div class="metrics">' +
    metric(Math.round(imp), "W", "importado", imp > 0 ? "hot" : "") +
    '<div class="metric health">' +
      '<b class="' + (bad ? "red" : "grn") + '">' + (bad ? "!" : "OK") + "</b>" +
      '<span>' + esc(bad || "medidor") + "</span></div>" +
    metric(Math.round(exp), "W", "exportado", exporting ? "grn" : "") +
    "</div>";

  const periods = TARIFF_PERIODS.filter((p) => o[p.k] != null);
  // Contracted power beside the real reading, as the meter's own panel shows it:
  // the pair only means something read together.
  const contracted = o.demandControlT1 != null || o.demandControlT2 != null || o.demandControlT3 != null;
  const periodBlock = periods.length
    ? '<div class="sub">Contrato e leituras</div>' +
      '<div class="period head"><i></i><span></span>' +
      (contracted ? "<em>contratado</em>" : "") + "<b>consumo real</b></div>" +
      periods.map((p) => {
        const limit = o["demandControlT" + p.t];
        return '<div class="period' + (o.tarif === p.t ? " now" : "") + '">' +
          '<i></i><span>' + p.n + "</span>" +
          (contracted ? "<em>" + (limit != null ? Number(limit).toFixed(1) + " kVA" : "—") + "</em>" : "") +
          '<b>' + Number(o[p.k]).toFixed(3) + "<small> kWh</small></b></div>";
      }).join("")
    : "";

  const totals = fieldRows(o, TOTALS_FIELDS);
  const advanced = fieldRows(o, ADVANCED_FIELDS);

  return '<div class="card energy"><h4>' + esc(f.name) +
    (o.tarif != null ? '<span class="chip">' + esc(TARIFF[o.tarif] || o.tarif) + "</span>" : "") +
    "</h4>" +
    // Kept for the live handler, which addresses the card by this id.
    '<span class="hide" id="sv-' + esc(f.id) + '"></span>' +
    head +
    '<div class="note" style="margin:0 0 10px">' +
      (exporting ? "a exportar para a rede" : imp > 0 ? "a consumir da rede" : "sem trânsito") +
    "</div>" +
    periodBlock +
    (totals ? '<div class="sub">Acumulado</div>' + totals : "") +
    (advanced ? '<div class="sub">Instantâneo</div>' + advanced : "") +
    (o.dateTime ? '<div class="note">leitura de ' + esc(o.dateTime) + "</div>" : "") +
    (f.driver === "PZEM_004T_V03"
      ? '<div class="btns" style="margin-top:10px">' +
        '<button class="btn" data-reset="' + esc(f.id) + '">Repor energia</button></div>'
      : "") +
    "</div>";
}

/* Sensor state arrives as JSON whose shape depends on the driver, and each
   deserves its own words: a door is open or closed, not "active". The payload
   values are the firmware's own (constants.h: "closed"/"open", "detected"/
   "clear", "rain"/"clear"). */
const BINARY_WORDS = {
  DOOR: { closed: "fechada", open: "aberta" },
  WINDOW: { closed: "fechada", open: "aberta" },
};
function sensorText(state, driver) {
  const o = parseState(state);
  if (o == null) {
    // Not an object: a bare value, or nothing yet.
    if (state == null || state === "") return "—";
    return typeof state === "string" ? state : String(state);
  }
  const bits = [];
  if (o.temperature != null) bits.push(Number(o.temperature).toFixed(1) + "°C");
  if (o.humidity != null) bits.push(Math.round(o.humidity) + "%");
  if (o.power != null) bits.push(Math.round(o.power) + "W");
  if (o.distance != null) bits.push(Math.round(o.distance) + " cm");
  if (o.illuminance != null) bits.push(Math.round(o.illuminance) + " lx");
  if (o.motion != null) bits.push(o.motion === "detected" ? "movimento" : "sem movimento");
  if (o.rain != null) bits.push(o.rain === "rain" ? "a chover" : "sem chuva");
  if (o.state != null && !bits.length) {
    const words = BINARY_WORDS[driver];
    bits.push(words && words[o.state] ? words[o.state] : String(o.state));
  }
  if (o.error && !bits.length) bits.push("erro de leitura");
  return bits.length ? bits.join(" · ") : "—";
}

/* Numeric readings deserve the width; binary states do not. */
const MEASURE_DRIVERS = ["LTR303", "HCSR04", "LD2410", "TMF882X"];
const CLIMATE_DRIVERS = ["DS18B20", "SHT4X", "DHT_11", "DHT_21", "DHT_22"];
const CLIMATE_MAX_SAMPLES = 360;
const isClimate = (f) => CLIMATE_DRIVERS.indexOf(f.driver) >= 0;

function climateReading(state) {
  const o = parseState(state);
  if (!o || typeof o !== "object") return null;
  const temperature = Number(o.temperature);
  const humidity = Number(o.humidity);
  const hasTemperature = o.temperature != null && isFinite(temperature);
  const hasHumidity = o.humidity != null && isFinite(humidity);
  if (!hasTemperature && !hasHumidity) return null;
  return {
    temperature: hasTemperature ? temperature : null,
    humidity: hasHumidity ? humidity : null,
  };
}

function recordClimate(f) {
  const reading = climateReading(f.state);
  if (!reading) return null;
  const now = Date.now();
  const history = climateHistory[f.id] || { samples: [], truncated: false };
  const last = history.samples[history.samples.length - 1];
  // renderOverview() can run again after a save; do not duplicate the reading
  // that the live event just recorded.
  if (!last || now - last.at > 1000) {
    history.samples.push({ at: now, temperature: reading.temperature, humidity: reading.humidity });
    if (history.samples.length > CLIMATE_MAX_SAMPLES) {
      history.samples.shift();
      history.truncated = true;
    }
  }
  climateHistory[f.id] = history;
  return history;
}

function climateSeries(samples, key, label, unit, digits, cls) {
  const validSamples = samples.filter((sample) => sample[key] != null && isFinite(sample[key]));
  const values = validSamples.map((sample) => sample[key]);
  if (values.length < 2) return "";
  const rawMin = Math.min.apply(null, values), rawMax = Math.max.apply(null, values);
  const padding = Math.max((rawMax - rawMin) * .12, key === "temperature" ? .2 : 1);
  const min = rawMin - padding, max = rawMax + padding, span = max - min || 1;
  const plotted = values.map((value, i) => ({
    x: i / (values.length - 1) * 300,
    y: 50 - (value - min) / span * 44,
  }));
  const points = plotted.map((point) => point.x.toFixed(1) + "," + point.y.toFixed(1)).join(" ");
  const area = "0,54 " + points + " 300,54";
  const latest = plotted[plotted.length - 1];
  const range = rawMin === rawMax
    ? "estável · " + rawMax.toFixed(digits) + unit
    : "mín " + rawMin.toFixed(digits) + unit + " · máx " + rawMax.toFixed(digits) + unit;
  const clock = (sample) => new Date(sample.at).toLocaleTimeString([], {
    hour: "2-digit", minute: "2-digit", second: "2-digit",
  });
  return '<div class="climate-series"><div class="climate-series-head"><span class="' + cls + '">' +
    label + '</span><span>' + range +
    '</span></div><svg class="climate-chart" viewBox="0 0 300 56" preserveAspectRatio="none" role="img" aria-label="' +
    esc(label + " desde que a página foi aberta") + '"><line class="guide" x1="0" y1="14" x2="300" y2="14"></line>' +
    '<line class="guide" x1="0" y1="38" x2="300" y2="38"></line>' +
    '<polygon class="climate-area ' + cls + '" points="' + area + '"></polygon>' +
    '<polyline class="climate-line ' + cls + '" points="' + points + '"></polyline>' +
    '<line class="climate-latest ' + cls + '" x1="298" y1="' + (latest.y - 4).toFixed(1) +
    '" x2="298" y2="' + (latest.y + 4).toFixed(1) + '"></line></svg>' +
    '<div class="climate-time"><span>' + clock(validSamples[0]) + '</span><span>' +
    clock(validSamples[validSamples.length - 1]) + '</span></div></div>';
}

function climateCard(f, shouldRecord) {
  const reading = climateReading(f.state);
  const history = shouldRecord ? recordClimate(f) : climateHistory[f.id];
  const samples = history ? history.samples : [];
  const temperature = reading && reading.temperature != null
    ? '<span class="climate-value temperature">' + reading.temperature.toFixed(1) + '<small>°C</small></span>' : "";
  const humidity = reading && reading.humidity != null
    ? '<span class="climate-value humidity">' + Math.round(reading.humidity) + '<small>%</small></span>' : "";
  const temperatureGraph = climateSeries(samples, "temperature", "temperatura", "°C", 1, "temperature");
  const humidityGraph = climateSeries(samples, "humidity", "humidade", "%", 0, "humidity");
  const collecting = !temperatureGraph && !humidityGraph
    ? '<div class="climate-collecting">A recolher leituras…</div>' : "";
  const scope = history && history.truncated ? "leituras recentes" : "desde que abriu esta página";
  return '<div class="card climate-card" id="climate-' + esc(f.id) + '"><h4>' + esc(f.name) + '</h4>' +
    '<div class="climate-values" id="sv-' + esc(f.id) + '">' + (temperature || humidity || "—") + '</div>' +
    collecting + temperatureGraph + humidityGraph +
    '<div class="climate-caption">' + scope + (samples.length ? " · " + samples.length +
      (samples.length === 1 ? " leitura" : " leituras") : "") + "</div></div>";
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

/* Which feature drives a given pin, ignoring the one being edited. Every pin
   stays on the menu — an occupied one is named, not hidden, so it is obvious
   what has to be freed first. */
function pinOwners(selfId) {
  const owner = {};
  for (const o of config.features || []) {
    if (o.id === selfId) continue;
    for (const p of o.outputs || []) owner[p] = o.name;
    for (const p of o.inputs || []) owner[p] = o.name;
  }
  return owner;
}

function pinSelect(f, fi, kind, slot, current, owner) {
  const opts = (config.usablePins || []).map((p) =>
    '<option value="' + p + '"' + (p === current ? " selected" : "") + ">GPIO" + p +
    (owner[p] ? " · ocupado (" + esc(owner[p]) + ")" : "") + "</option>").join("");
  return '<select data-pin="' + kind + '" data-i="' + fi + '" data-k="' + slot + '">' + opts + "</select>";
}

/* Editable wiring. The device refuses a pin it cannot give away, so a clash is
   flagged here rather than letting the save look like it worked. */
function pinEditor(f, i) {
  const outs = f.outputs || [];
  const ins = f.inputs || [];
  if (!outs.length && !ins.length) return "";
  const owner = pinOwners(f.id);
  const block = (label, kind, pins) => !pins.length ? "" :
    '<div class="field"><label>' + label + "</label>" +
    '<div class="row2" style="grid-template-columns:repeat(' + Math.min(pins.length, 3) + ',1fr)">' +
    pins.map((p, k) => pinSelect(f, i, kind, k, p, owner)).join("") + "</div></div>";
  const clashes = outs.concat(ins).filter((p) => owner[p]);
  const warn = clashes.length
    ? '<div class="note err">GPIO' + clashes.join(", GPIO") + " já " +
      (clashes.length > 1 ? "estão a ser usados" : "está a ser usado") +
      " por <b>" + esc(owner[clashes[0]]) + "</b>. O dispositivo recusa a alteração até libertares " +
      (clashes.length > 1 ? "esses pinos" : "esse pino") + ".</div>"
    : "";
  return block("SAÍDAS (relé)", "out", outs) + block("ENTRADAS (botão/sensor)", "in", ins) + warn;
}

/* How the physical input behaves. An omitted value preserves the current
   driver; an explicit value is validated against the actuator family. Keep a
   one-button cover on mode 2 because changing its topology also needs another
   input pin. */
const INPUT_MODES = [
  { v: 0, n: "botão de pressão (momentâneo)" },
  { v: 1, n: "interruptor (mantém posição)" },
];
const COVER_SINGLE_MODE = { v: 2, n: "botão único (rodar / parar)" };
function inputModes(f) {
  const driver = String(f.driver || "");
  if (driver === "COVER_SINGLE_PUSH") return [COVER_SINGLE_MODE];
  if (driver.indexOf("COVER_DUAL") === 0) return INPUT_MODES;
  if (driver === "GARAGE_PUSH" || driver === "GARDEN_VALVE") return [];
  return INPUT_MODES;
}
function inputModeField(f, i) {
  if (!isActuator(f) || f.inputMode == null) return "";
  const modes = inputModes(f);
  if (!modes.length) return "";
  const opts = modes.map((m) =>
    '<option value="' + m.v + '"' + (m.v === f.inputMode ? " selected" : "") + ">" +
    m.n + "</option>").join("");
  return '<div class="field"><label>MODO DE ENTRADA</label>' +
    '<select data-f="inputMode" data-i="' + i + '" data-num="1">' + opts + "</select></div>";
}

function featureKind(f) {
  if (isCover(f)) return { label: "ESTORE", cls: "kind-cover" };
  if (isActuator(f)) return { label: "ATUADOR", cls: "kind-actuator" };
  return { label: "SENSOR", cls: "kind-sensor" };
}

function renderFeatures() {
  const feats = config.features || [];
  $("feat-count").textContent = feats.length + (feats.length === 1 ? " função" : " funções");
  $("feat-list").innerHTML = feats.length ? feats.map((f, i) => {
    const cover = isCover(f);
    const kind = featureKind(f);
    return '<div class="card feature-card" data-fi="' + i + '">' +
      '<div class="feature-card-head"><span class="feature-index">FUNÇÃO ' + (i + 1) + '</span>' +
      '<div class="feature-badges"><span class="feature-badge ' + kind.cls + '">' + kind.label + '</span>' +
      '<span class="feature-badge kind-driver">' + esc(driverLabel(f.driver)) + '</span></div></div>' +
      '<div class="field"><label>NOME</label><input data-f="name" data-i="' + i + '" maxlength="23" value="' + esc(f.name) + '"></div>' +
      inputModeField(f, i) +
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
      '<div class="feature-card-actions"><button class="btn d" data-del="' + esc(f.id) + '">Remover função</button></div></div>';
  }).join("") : '<div class="note">Ainda não há funções configuradas.</div>';
}

function renderDiag() {
  const heap = config.freeHeap;
  if (heap != null) {
    heapHistory.push(heap);
    if (heapHistory.length > 40) heapHistory.shift();
    $("d-heap").innerHTML = (heap / 1024).toFixed(1) + "<small>KB</small>";
    drawSpark("d-heap-spark", heapHistory, accent());
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
  const netState = config.signal == null ? "off" : rssiClass(config.signal);
  const netLabel = netState === "ok" ? "bom" : netState === "warn" ? "fraco" :
    netState === "bad" ? "muito fraco" : "desconhecido";
  $("d-net-card").className = "card status-card " + netState;
  setStatusBadge($("d-net-status"), netState, netLabel);
  $("d-ip").textContent = config.wifiIp || "—";
  $("d-net").textContent = (config.wifiMask || "—") + " / " + (config.wifiGw || "—");
  setMqttPill(config.mqttConnected);
  $("d-broker").textContent = (config.mqttIpDns || "—") + ":" + (config.mqttPort || "");
  const cloud = config.cloudConfigured;
  // Three distinct states, because "não configurada" on a working device sent
  // people looking for a problem that was not there.
  const cloudState = !cloud ? "off" : config.cloudConnected ? "ok" : "bad";
  const cloudText = !cloud ? "sem credenciais (não adotado)" :
    config.cloudConnected ? "ligada" : "adotado, sem ligação";
  setStatusBadge($("d-cloud"), cloudState, cloudText);
  // Without a synced clock the device refuses to run a schedule rather than
  // guess the hour, so the panel has to be able to say that out loud.
  setStatusBadge($("d-clock"),
    config.clockSynced ? "ok" : "warn",
    config.clockSynced ? (config.clockNow || "certo").replace("T", " ") : "sem hora (NTP)");
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
  // Name the variant on the upload warning so the right .bin is picked.
  $("u-mcu").textContent = config.mcu || "desconhecida";
  updateNameDirty();
}

function updateNameDirty() {
  const changed = $("s-nodeId").value.trim() !== (config.nodeId || "");
  $("s-name-dirty").classList.toggle("hide", !changed);
}

function showSystemPane(name) {
  document.querySelectorAll(".system-tab").forEach((tab) => {
    const on = tab.dataset.systemView === name;
    tab.classList.toggle("on", on);
    tab.setAttribute("aria-selected", String(on));
  });
  document.querySelectorAll(".system-pane").forEach((pane) =>
    pane.classList.toggle("on", pane.id === "system-" + name));
}

function openFirmwareSettings() {
  const systemTab = document.querySelector('[data-view="system"]');
  const firmwareTab = document.querySelector('[data-system-view="firmware"]');
  if (systemTab) systemTab.click();
  if (firmwareTab) firmwareTab.click();
  window.scrollTo({ top: 0, behavior: "auto" });
}

function fillNewFeatureForm() {
  const sel = $("nf-driver");
  sel.innerHTML = DRIVERS.map((g) => '<optgroup label="' + esc(g.g) + '">' +
    g.items.map((d) => '<option value="' + d.v + '">' + esc(d.n) + "</option>").join("") + "</optgroup>").join("");
  // Every usable pin is listed; an occupied one names its owner instead of
  // disappearing, so it is clear what is in the way.
  const owner = pinOwners(null);
  const pins = config.usablePins || [];
  const opts = pins.map((p) =>
    '<option value="' + p + '">GPIO' + p +
    (owner[p] ? " · ocupado (" + esc(owner[p]) + ")" : "") + "</option>").join("");
  const first = pins.find((p) => !owner[p]);
  $("nf-p1").innerHTML = opts;
  $("nf-p2").innerHTML = opts;
  if (first != null) $("nf-p1").value = String(first);
  onDriverChange();
  onNewPinChange();
}
function onDriverChange() {
  const d = driverInfo(parseInt($("nf-driver").value, 10));
  const singlePin = !d || d.pins < 2;
  $("nf-p2-box").classList.toggle("hide", singlePin);
  $("nf-fields").classList.toggle("two-pins", !singlePin);
  onNewPinChange();
}

/* Warn before the device refuses the pin, rather than after. */
function onNewPinChange() {
  const d = driverInfo(parseInt($("nf-driver").value, 10));
  const owner = pinOwners(null);
  const chosen = [parseInt($("nf-p1").value, 10)];
  if (d && d.pins > 1) chosen.push(parseInt($("nf-p2").value, 10));
  const busy = chosen.filter((p) => !isNaN(p) && owner[p]);
  const msg = $("nf-msg");
  if (busy.length) {
    msg.className = "note err";
    msg.innerHTML = "GPIO" + busy.join(", GPIO") + " já " +
      (busy.length > 1 ? "estão ocupados" : "está ocupado") +
      " por <b>" + esc(owner[busy[0]]) + "</b>. Escolhe outro ou liberta-o primeiro.";
  } else if (msg.className.indexOf("err") >= 0) {
    msg.className = "note";
    msg.textContent = "";
  }
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
    wireFeatureEvents();
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
  if (code === 4) return "Este tipo precisa de dois pinos válidos.";
  if (code === 5) return "Esse pino já está a ser usado por outra função.";
  if (code === 6) return "Não foi possível guardar na memória do dispositivo; a função não será mantida.";
  return "O dispositivo recusou a função.";
}

/* ConfigOnofre::update() returns stable result codes for rejected saves. */
function configError(e) {
  const code = e && e.body && e.body.result;
  if (code === 1) return "A configuração enviada é inválida.";
  if (code === 2) return "Um dos pinos não é válido nesta placa.";
  if (code === 3) return "A quantidade de pinos não corresponde ao tipo de função.";
  if (code === 4) return "Um dos pinos já está a ser usado por outra função.";
  if (code === 5) return "O dispositivo está ocupado — tenta guardar novamente.";
  if (code === 6) return "Não foi possível guardar na memória do dispositivo; a configuração anterior será reposta.";
  return "Não foi possível guardar.";
}

async function save() {
  const body = JSON.parse(JSON.stringify(config));
  body.nodeId = $("s-nodeId").value.trim();
  body.wifiSSID = $("s-ssid").value.trim();
  if ($("s-wpw").value) body.wifiSecret = $("s-wpw").value;
  body.dhcp = $("s-dhcp").checked;
  if (body.dhcp) {
    // GET /config reports the live DHCP lease for diagnostics. It is not a
    // static-address edit and must not make an unrelated save restart Wi-Fi.
    delete body.wifiIp;
    delete body.wifiMask;
    delete body.wifiGw;
  } else {
    body.wifiIp = $("s-ip").value.trim();
    body.wifiMask = $("s-mask").value.trim();
    body.wifiGw = $("s-gw").value.trim();
  }
  body.mqttIpDns = $("s-mqttHost").value.trim();
  body.mqttPort = parseInt($("s-mqttPort").value, 10) || 1883;
  body.mqttUsername = $("s-mqttUser").value.trim();
  if ($("s-mqttPw").value) body.mqttPassword = $("s-mqttPw").value;
  body.apiUser = $("s-apiUser").value.trim();
  if ($("s-apiPw").value) body.apiPassword = $("s-apiPw").value;
  if (removed.length) body.featuresToRemove = removed;

  const btn = $("save-btn");
  btn.disabled = true;
  btn.textContent = "A guardar…";
  try {
    const saved = await api("/config", { method: "POST", body: JSON.stringify(body) });
    const restartRequired = saved.restartRequired === true;
    config = saved;
    removed = [];
    clearDirty();
    toast(restartRequired ? "Guardado — o dispositivo vai reiniciar." : "Guardado", "ok");
    renderHeader(); renderOverview(); renderPinout(); renderFeatures(); renderDiag(); fillSystem(); fillNewFeatureForm();
    wireFeatureEvents();
  } catch (e) {
    toast(configError(e), "err");
  } finally {
    // Always restore the button: leaving it disabled strands the page with no
    // way to retry, which is how a failed save looked like a silent one.
    btn.textContent = "Guardar";
    btn.disabled = !dirty;
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
    toast("Predefinição aceite · a reiniciar", "ok");
  } catch (e) { toast("Não foi possível aplicar", "err"); }
}

/* Update availability. The device only knows its own version, so the panel asks
   the release server and offers the update when there is one — otherwise nobody
   discovers a fix exists. Compared component by component as numbers: string
   comparison would rank 9.99 above 9.100. */
const UPDATE_HOST = "https://update.bhonofre.pt";

function versionParts(v) {
  return String(v == null ? "" : v).trim().split("-")[0].split(".")
    .map((p) => parseInt(p.replace(/[^0-9]/g, ""), 10) || 0);
}
function isNewer(candidate, current) {
  const a = versionParts(candidate), b = versionParts(current);
  for (let i = 0; i < Math.max(a.length, b.length); i++) {
    const x = a[i] || 0, y = b[i] || 0;
    if (x !== y) return x > y;
  }
  return false;
}

async function checkForUpdate() {
  const btn = $("a-update");
  const note = $("u-avail");
  if (!config.mcu) return;
  let latest = "";
  try {
    // Cross-origin by design; an https fetch from this http page is allowed.
    const res = await fetch(UPDATE_HOST + "/firmware/latest-version/" + encodeURIComponent(config.mcu),
      { mode: "cors" });
    if (!res.ok) throw new Error("HTTP " + res.status);
    latest = (await res.text()).trim();
  } catch (e) {
    // Offline, or no route to the release server — say so instead of implying
    // the firmware is current.
    note.className = "note";
    note.textContent = "Não foi possível verificar se há atualização.";
    setFirmwareHeaderState("error");
    return;
  }
  if (isNewer(latest, config.firmware)) {
    note.className = "note ok";
    note.textContent = "Está disponível a versão " + latest + ".";
    btn.textContent = "Atualizar para a " + latest;
    btn.classList.add("p");
    setFirmwareHeaderState("available", latest);
  } else {
    note.className = "note";
    note.textContent = latest
      ? "Já tens a versão mais recente (" + latest + ")."
      : "";
    btn.textContent = "Procurar atualização";
    btn.classList.remove("p");
    setFirmwareHeaderState("current");
  }
}

/* Every published build for THIS board, newest first, each a direct download.
   Filtering by the device's own MCU is the point: handing someone the wrong
   variant's image is the one mistake here that costs a USB recovery. */
async function listFirmwareVersions(btn) {
  const box = $("u-versions");
  if (!config.mcu) return;
  if (box.dataset.open === "1") {          // second press closes it again
    box.dataset.open = "";
    box.innerHTML = "";
    btn.textContent = "Ver versões disponíveis";
    return;
  }
  btn.disabled = true;
  box.innerHTML = '<div class="note">A consultar…</div>';
  let list;
  try {
    const res = await fetch(UPDATE_HOST + "/firmware/all-versions/" + encodeURIComponent(config.mcu),
      { mode: "cors" });
    list = await res.json();
  } catch (e) {
    box.innerHTML = '<div class="note err">Não foi possível obter a lista. ' +
      "Verifica se o dispositivo tem acesso à internet.</div>";
    btn.disabled = false;
    return;
  }
  btn.disabled = false;
  if (!Array.isArray(list) || !list.length) {
    box.innerHTML = '<div class="note">Não há binários publicados para ' + esc(config.mcu) + ".</div>";
    return;
  }
  box.dataset.open = "1";
  btn.textContent = "Esconder versões";
  box.innerHTML = '<div class="sub">' + esc(config.mcu) + " · " + list.length +
    " versões, mais recente primeiro</div>" +
    list.map((v) => {
      const installed = v.version === config.firmware;
      return '<div class="fwrow' + (installed ? " now" : "") + '">' +
        "<b>" + esc(v.version) + "</b>" +
        '<span>' + (installed ? "instalada" : "") + "</span>" +
        // A plain link: the server answers application/octet-stream, so the
        // browser saves it instead of trying to render it.
        '<a class="btn" href="' + esc(v.url) + '" download>Descarregar</a></div>';
    }).join("") +
    '<div class="note">Descarrega o ficheiro e escolhe-o abaixo em "Escolher ficheiro".</div>';
}

/* Manual firmware upload — the recovery path when the cloud is unreachable.
   POST /update takes a multipart body and reboots the device when it ends. */
function uploadFirmware(btn) {
  const file = ($("u-file").files || [])[0];
  const msg = $("u-msg");
  if (!file) {
    msg.className = "note err";
    msg.textContent = "Escolhe primeiro o ficheiro .bin.";
    return;
  }
  // Both ESP8266 and ESP32 application images start with 0xE9. Refusing anything
  // else here costs nothing and saves a USB recovery after an accidental pick.
  const head = new FileReader();
  head.onload = () => {
    if (new Uint8Array(head.result)[0] !== 0xe9) {
      msg.className = "note err";
      msg.textContent = "Isto não parece uma imagem de firmware. Envio cancelado.";
      return;
    }
    if (!armed(btn, "Confirmar envio?")) return;
    send();
  };
  head.readAsArrayBuffer(file.slice(0, 1));

  function send() {
    const bar = $("u-bar");
    const fill = bar.querySelector("i");
    bar.classList.remove("hide");
    $("u-send").disabled = true;
    msg.className = "note";
    msg.textContent = "A enviar… não desligues o dispositivo.";

    const form = new FormData();
    form.append("update", file, file.name);
    const xhr = new XMLHttpRequest();
    xhr.open("POST", baseUrl + "/update");
    // fetch() reports no upload progress, and this can be a slow minute on WiFi.
    xhr.upload.onprogress = (e) => {
      if (e.lengthComputable) fill.style.width = Math.round(e.loaded / e.total * 100) + "%";
    };
    const done = (ok) => {
      $("u-send").disabled = false;
      msg.className = ok ? "note ok" : "note err";
      msg.textContent = ok
        ? "Enviado. O dispositivo está a reiniciar — volta a abrir esta página daqui a pouco."
        : "O envio falhou. O firmware anterior continua ativo; tente novamente.";
    };
    // The backend now returns a truthful status before scheduling a restart.
    // Sending every byte is not proof that flash finalization succeeded.
    xhr.onload = () => done(xhr.status === 200);
    xhr.onerror = () => done(false);
    xhr.send(form);
  }
}

const RESTORE_SCALAR_FIELDS = [
  "nodeId", "mqttIpDns", "mqttPort", "mqttUsername", "wifiSSID", "dhcp",
  "wifiIp", "wifiMask", "wifiGw", "apiUser"
];
const RESTORE_FEATURE_FIELDS = [
  "group", "id", "name", "inputMode", "upCourseTime", "downCourseTime",
  "autoOff", "area", "line", "member", "inputs", "outputs"
];

function copyFields(source, fields) {
  const copy = {};
  fields.forEach((field) => {
    if (Object.prototype.hasOwnProperty.call(source, field)) copy[field] = source[field];
  });
  return copy;
}

function featureIdentityList(features) {
  if (!Array.isArray(features)) throw new Error("A cópia não contém uma lista de funções válida.");
  const identities = features.map((feature) => {
    if (!feature || typeof feature !== "object" ||
        (feature.group !== "ACTUATOR" && feature.group !== "SENSOR") ||
        typeof feature.id !== "string" || !feature.id) {
      throw new Error("A cópia contém uma função inválida.");
    }
    return feature.group + ":" + feature.id;
  }).sort();
  if (new Set(identities).size !== identities.length)
    throw new Error("A cópia contém identificadores de função repetidos.");
  return identities;
}

/* The exported object is intentionally non-secret. A restore preserves the
   credentials already stored on the device and uses POST /config, whose
   firmware-side preflight validates every submitted pin before changing it. */
function exportConfig() {
  const snapshot = Object.assign(copyFields(config, RESTORE_SCALAR_FIELDS), {
    snapshotType: "easyiot-config",
    snapshotVersion: 1
  });
  snapshot.chipId = config.chipId;
  snapshot.mcu = config.mcu;
  snapshot.firmware = config.firmware;
  snapshot.features = (config.features || []).map((feature) =>
    copyFields(feature, RESTORE_FEATURE_FIELDS));
  const blob = new Blob([JSON.stringify(snapshot, null, 2)], { type: "application/json" });
  const a = document.createElement("a");
  a.href = URL.createObjectURL(blob);
  a.download = "CONFIG_" + (config.nodeId || "onofre") + "_ONOFRE_" + (config.firmware || "") + ".json";
  a.click();
  URL.revokeObjectURL(a.href);
}

async function restoreConfig(file, btn) {
  const msg = $("r-msg");
  btn.disabled = true;
  msg.className = "note";
  msg.textContent = "A validar a cópia…";
  try {
    if (!file || file.size > 512 * 1024)
      throw new Error("O ficheiro não é uma cópia JSON válida ou é demasiado grande.");
    let snapshot;
    try { snapshot = JSON.parse(await file.text()); }
    catch (e) { throw new Error("Não foi possível ler este ficheiro JSON."); }
    if (!snapshot || typeof snapshot !== "object" || Array.isArray(snapshot))
      throw new Error("O ficheiro não contém uma configuração válida.");
    if (snapshot.snapshotType &&
        snapshot.snapshotType !== "easyiot-config" &&
        snapshot.snapshotType !== "easyiot-diagnostic")
      throw new Error("Este ficheiro não é uma cópia de configuração EasyIot.");
    if (String(snapshot.chipId || "") !== String(config.chipId || ""))
      throw new Error("Esta cópia pertence a outro equipamento.");
    if (String(snapshot.mcu || "") !== String(config.mcu || ""))
      throw new Error("Esta cópia foi criada para outra variante de hardware.");

    const savedIds = featureIdentityList(snapshot.features);
    const currentIds = featureIdentityList(config.features || []);
    if (savedIds.length !== currentIds.length ||
        savedIds.some((identity, index) => identity !== currentIds[index]))
      throw new Error("A lista de funções já não é igual à da cópia. O restauro foi cancelado.");

    const body = copyFields(snapshot, RESTORE_SCALAR_FIELDS);
    body.features = snapshot.features.map((feature) =>
      copyFields(feature, RESTORE_FEATURE_FIELDS));
    // A DHCP snapshot contains the live lease returned by GET /config, not a
    // static address request. Match the normal Save path and omit those values.
    if (body.dhcp === true) {
      delete body.wifiIp;
      delete body.wifiMask;
      delete body.wifiGw;
    }

    const saved = await api("/config", { method: "POST", body: JSON.stringify(body) });
    const restartRequired = saved.restartRequired === true;
    config = saved;
    removed = [];
    clearDirty();
    renderHeader(); renderOverview(); renderPinout(); renderFeatures(); renderDiag(); fillSystem(); fillNewFeatureForm();
    wireFeatureEvents();
    msg.className = "note ok";
    msg.textContent = restartRequired
      ? "Configuração reposta. O equipamento vai reiniciar; volta a abrir a página dentro de alguns segundos."
      : "Configuração reposta com sucesso.";
    toast("Configuração reposta", "ok");
  } catch (e) {
    msg.className = "note err";
    msg.textContent = e && e.body ? configError(e) : (e.message || "Não foi possível restaurar a configuração.");
  } finally {
    btn.disabled = false;
  }
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
    config.mqttConnected = on;
    setMqttPill(on);
    addLog("i", "[mqtt] " + (on ? "ligado" : "desligado"));
  });
  // Every feature publishes its state under its own id.
  source.onmessage = () => {};
  source.addEventListener("error", () => addLog("i", "[aviso] ligação de eventos interrompida"));
  // Feature events are named after the uniqueId, so they are wired per feature.
  wireFeatureEvents();
}
function wireFeatureEvents() {
  if (!source) return;
  // Config responses replace the feature objects. Remove the old closures so
  // events update the current objects and newly added/restored ids are wired.
  for (const entry of featureEventHandlers) {
    source.removeEventListener(entry.id, entry.handler);
  }
  featureEventHandlers = [];
  for (const f of config.features || []) {
    (function (feat) {
      const handler = (e) => {
        feat.state = e.data;
        if (isActuator(feat)) {
          const sw = document.querySelector('[data-toggle="' + feat.id + '"]');
          if (sw) {
            const on = (parseInt(e.data, 10) || 0) > 0;
            setToggleTileState(sw, on);
          }
          const rng = document.querySelector('[data-cover="' + feat.id + '"]');
          if (rng) rng.value = 100 - (parseInt(e.data, 10) || 0);
          // A valve tile carries more than a switch position (open/closed plus
          // the countdown), so redraw the strip instead of poking one node.
          if (isZone(feat) && irrigation) {
            // The device cancels the cycle when a valve is taken over by hand,
            // so the state card must not keep counting down a dead program.
            const run = irrigation.running;
            const open = (parseInt(e.data, 10) || 0) > 0;
            if (run && ((open && run.zone !== feat.id) || (!open && run.zone === feat.id))) {
              irrigation.running = null;
              renderIrrStatus();
            }
            renderIrrZones();
          }
        } else if (isEnergy(feat)) {
          // The whole card is derived from the payload, so redraw it in place.
          const el = $("sv-" + feat.id);
          const card = el && el.closest(".card");
          if (card) card.outerHTML = energyCard(feat);
        } else if (isClimate(feat)) {
          recordClimate(feat);
          const card = $("climate-" + feat.id);
          if (card) card.outerHTML = climateCard(feat, false);
        } else {
          const el = $("sv-" + feat.id);
          if (el) el.textContent = sensorText(e.data, feat.driver);
        }
        addLog("i", "[" + feat.name + "] " + String(e.data).slice(0, 90));
      };
      source.addEventListener(feat.id, handler);
      featureEventHandlers.push({ id: feat.id, handler: handler });
    })(f);
  }
}

/* ---------------- wiring ---------------- */
document.addEventListener("click", (ev) => {
  const systemTab = ev.target.closest("[data-system-view]");
  if (systemTab) {
    showSystemPane(systemTab.dataset.systemView);
    return;
  }
  const tab = ev.target.closest("[data-view]");
  if (tab) {
    document.querySelectorAll(".tab").forEach((t) => t.classList.toggle("on", t === tab));
    document.querySelectorAll(".view").forEach((v) => v.classList.toggle("on", v.id === "v-" + tab.dataset.view));
    // Read once on arrival, so nobody has to know there is a button for it.
    if (tab.dataset.view === "diag") loadDeviceLog();
    return;
  }
  // The cog is inside the tile, so it must claim the click before the toggle.
  const cfg = ev.target.closest("[data-config]");
  if (cfg) {
    openFeatureConfig(cfg.dataset.config);
    return;
  }
  const sw = ev.target.closest("[data-toggle]");
  if (sw) {
    const on = sw.classList.toggle("on");
    setToggleTileState(sw, on);
    control(sw.dataset.toggle, 102);
    return;
  }
  const tpl = ev.target.closest("[data-tpl]");
  if (tpl) {
    if (armed(tpl, "Substituir tudo?")) applyTemplate(parseInt(tpl.dataset.tpl, 10));
    return;
  }
  // Irrigation: zone command, day toggle, add/remove program, stop, save.
  const zbtn = ev.target.closest("[data-zone]");
  if (zbtn) {
    const z = zones().find((x) => x.id === zbtn.dataset.zone);
    if (z) control(z.id, String(z.state) === "100" ? 0 : 100);
    return;
  }
  if (ev.target.closest("#irr-stop")) { stopIrrigation(); return; }
  if (ev.target.closest("#irr-save")) { saveIrrigation(); return; }
  const day = ev.target.closest("[data-ipday]");
  if (day) {
    const parts = day.dataset.ipday.split(":");
    const prog = irrigation.programs[Number(parts[0])];
    prog.weekdays ^= (1 << Number(parts[1]));
    day.classList.toggle("on", ((prog.weekdays >> Number(parts[1])) & 1) === 1);
    markIrrDirty();
    refreshProgramNote(Number(parts[0]));
    return;
  }
  const prun = ev.target.closest("[data-iprun]");
  if (prun) {
    // Forcing a cycle waters the whole garden, so it asks first.
    if (armed(prun, "Regar tudo?")) runProgramNow(parseInt(prun.dataset.iprun, 10));
    return;
  }
  const pdel = ev.target.closest("[data-ipdel]");
  if (pdel) {
    irrigation.programs.splice(Number(pdel.dataset.ipdel), 1);
    markIrrDirty();
    renderIrrPrograms();
    return;
  }
  if (ev.target.closest("#irr-add")) {
    irrigation.programs = irrigation.programs || [];
    irrigation.programs.push({
      id: Math.max(0, ...irrigation.programs.map((x) => x.id || 0)) + 1,
      enabled: true, startMinute: 420, weekdays: 0b0111110, zones: [],
    });
    markIrrDirty();
    renderIrrPrograms();
    return;
  }
  const rst = ev.target.closest("[data-reset]");
  if (rst) {
    if (!armed(rst, "Repor a zero?")) return;
    api("/sensors/reset-energy", { method: "POST", body: JSON.stringify({ id: rst.dataset.reset }) })
      .then(() => toast("Pedido de reposição em fila — confirme o valor na próxima leitura", "ok"))
      .catch(() => toast("Não foi possível repor o contador", "err"));
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
  if (cover) {
    // Slider is % open; the device wants % closed.
    control(cover.dataset.cover, 100 - (parseInt(cover.value, 10) || 0));
    return;
  }
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
    // A <select> reports type "select-one", so numeric ones say so explicitly;
    // the firmware validates inputMode as an unsigned integer when it is sent.
    const numeric = f.type === "number" || f.dataset.num === "1";
    const val = numeric ? (parseInt(f.value, 10) || 0) : f.value;
    if (config.features[i]) { config.features[i][key] = val; markDirty(); }
    return;
  }
  // Irrigation edits: start time, zone selection, per-zone duration.
  const ip = ev.target.closest("[data-ip]");
  if (ip) {
    const prog = irrigation.programs[Number(ip.dataset.pi)];
    if (ip.dataset.ip === "enabled") prog.enabled = ip.checked;
    if (ip.dataset.ip === "start") {
      const m = minutesFromHhmm(ip.value);
      if (m == null) { toast("Hora inválida", "err"); return; }
      prog.startMinute = m;
    }
    markIrrDirty();
    refreshProgramNote(Number(ip.dataset.pi));
    return;
  }
  const ipz = ev.target.closest("[data-ipz]");
  if (ipz) {
    const parts = ipz.dataset.ipz.split(":");
    const prog = irrigation.programs[Number(parts[0])], zid = parts[1];
    prog.zones = prog.zones || [];
    if (ipz.checked) {
      if (!prog.zones.some((z) => z.uniqueId === zid)) prog.zones.push({ uniqueId: zid, minutes: 10 });
      // The zones run in the order they are stored, so keep that equal to the
      // order they are listed in: the screen says "por esta ordem" and has to
      // mean it, whatever order the boxes were ticked in.
      const order = zones().map((z) => z.id);
      prog.zones.sort((a, b) => order.indexOf(a.uniqueId) - order.indexOf(b.uniqueId));
    } else {
      prog.zones = prog.zones.filter((z) => z.uniqueId !== zid);
    }
    // The minutes box belongs to the zone next to it, so it follows the tick.
    const mins = document.querySelector('[data-ipmin="' + parts[0] + ":" + zid + '"]');
    if (mins) mins.disabled = !ipz.checked;
    markIrrDirty();
    refreshProgramNote(Number(parts[0]));
    return;
  }
  const ipmin = ev.target.closest("[data-ipmin]");
  if (ipmin) {
    const parts = ipmin.dataset.ipmin.split(":");
    const entry = (irrigation.programs[Number(parts[0])].zones || []).find((z) => z.uniqueId === parts[1]);
    if (entry) entry.minutes = Math.max(1, Math.min(240, parseInt(ipmin.value, 10) || 1));
    markIrrDirty();
    refreshProgramNote(Number(parts[0]));
    return;
  }
  if (ev.target.id === "irr-enabled" || ev.target.id === "irr-rain") { markIrrDirty(); return; }
  if (ev.target.id === "nf-driver") { onDriverChange(); return; }
  if (ev.target.id === "nf-p1" || ev.target.id === "nf-p2") { onNewPinChange(); return; }
  if (ev.target.id === "s-dhcp") { $("s-static").classList.toggle("hide", ev.target.checked); markDirty(); return; }
  if (ev.target.closest("#v-system")) markDirty();
});

document.addEventListener("input", (ev) => {
  if (ev.target.id === "s-nodeId") updateNameDirty();
});

window.addEventListener("beforeunload", (e) => {
  if (dirty) { e.preventDefault(); e.returnValue = ""; }
});

document.addEventListener("DOMContentLoaded", () => {
  applyTheme(currentTheme());        // paints the button for the theme already applied
  $("toast-close").onclick = dismissToast;
  $("theme-btn").onclick = () => applyTheme(currentTheme() === "light" ? "dark" : "light");
  $("h-fw-link").onclick = openFirmwareSettings;
  $("save-btn").onclick = save;
  $("nf-add").onclick = addFeature;
  $("a-export").onclick = exportConfig;
  $("r-file").onchange = () => {
    const file = ($("r-file").files || [])[0];
    $("r-msg").className = "note";
    $("r-msg").textContent = file
      ? "Pronto para validar e aplicar " + file.name + "."
      : "Escolhe uma cópia JSON exportada por este equipamento.";
  };
  $("d-log-pause").onclick = (e) => {
    logPaused = !logPaused;
    e.target.textContent = logPaused ? "Retomar" : "Pausar";
  };
  $("d-log-clear").onclick = () => { logLines = []; $("d-log").innerHTML = ""; };
  $("d-log-copy").onclick = () => {
    navigator.clipboard.writeText(logLines.join("\n")).then(
      () => toast("Registo copiado", "ok"), () => toast("Não foi possível copiar", "err"));
  };
  $("u-send").onclick = (e) => uploadFirmware(e.currentTarget);
  $("u-list").onclick = (e) => listFirmwareVersions(e.currentTarget);
  $("r-send").onclick = (e) => {
    const f = ($("r-file").files || [])[0];
    if (!f) {
      $("r-msg").className = "note err";
      $("r-msg").textContent = "Escolhe primeiro o ficheiro .json.";
      return;
    }
    restoreConfig(f, e.currentTarget);
  };
  $("a-reboot").onclick = async (e) => {
    if (!armed(e.currentTarget, "Reiniciar?")) return;
    try { await api("/reboot", { method: "POST" }); toast("A reiniciar…", "ok"); } catch (err) { toast("Falhou", "err"); }
  };
  $("a-update").onclick = async (e) => {
    if (!armed(e.currentTarget, "Atualizar?")) return;
    try {
      await api("/auto-update", { method: "POST" });
      followUpdate();
    } catch (err) { toast("Falhou o pedido de atualização", "err"); }
  };
  $("a-defaults").onclick = async (e) => {
    if (!armed(e.currentTarget, "Apagar tudo?")) return;
    try { await api("/load-defaults", { method: "POST" }); toast("A repor…", "ok"); } catch (err) { toast("Falhou", "err"); }
  };
  // Announce an available update on arrival; the old panel did and people
  // relied on it to know a fix existed.
  load().then(connectEvents).then(checkForUpdate);
  $("d-devlog-refresh").onclick = loadDeviceLog;
  $("d-devlog-copy").onclick = async (e) => {
    try {
      await navigator.clipboard.writeText($("d-devlog").innerText);
      toast("Registo copiado", "ok");
    } catch (err) {
      toast("O browser não deixou copiar — seleciona e copia à mão", "err");
    }
  };

  // Diagnostics are a snapshot; refresh them while the tab is open.
  setInterval(() => {
    if ($("v-diag").classList.contains("on")) {
      api("/config").then((c) => { applyDiagnosticsSnapshot(c); renderHeader(); renderDiag(); }).catch(() => {});
      // Left to the buttons otherwise: re-reading it under someone's cursor every
      // ten seconds would move the text they are trying to select.
    }
  }, 10000);
  // The remaining time is counted down here so the tile does not need a push
  // every second; the device is re-asked now and then in case a program
  // advanced to the next zone on its own.
  setInterval(() => {
    if (!irrigation || !$("v-irrigation").classList.contains("on")) return;
    const run = irrigation.running;
    if (run && run.secondsLeft > 0) {
      run.secondsLeft -= 1;
      renderIrrStatus();
      renderIrrZones();
    }
  }, 1000);
  setInterval(() => {
    if (!irrigation || !$("v-irrigation").classList.contains("on") || irrDirty) return;
    api("/config").then((c) => {
      irrigation = c.irrigation || irrigation;
      config.irrigation = irrigation;
      // Valve states travel by event; only the cycle itself is re-read.
      renderIrrStatus();
      renderIrrZones();
    }).catch(() => {});
  }, 15000);
});
