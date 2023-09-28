let baseUrl = "http://192.168.187.134"
let config;
var newConfig = {};
let source = null;
var WORDS_PT = {
    "config_save_error": "Não foi possivel guardar a configuração atual, por favor tenta novamente.",
    "config_save_ok": "Configuração Guardada",
    "device_reboot_ok": "O dispositivo está a reiniciar, ficará disponivel dentro de 10 segundos.",
    "device_reboot_error": "Não foi possivel reiniciar o dispositivo, verifica se está correctamente ligado à rede. Se o problema persistir tenta desligar da energia e voltar a ligar.",
    "defaults_ok": "Configuração de fábrica aplicada com sucesso. Por favor volte a ligar-se ao Access Point e aceda ao painel de controlo pelo endereço http://192.168.4.1 no seu browser.",
    "defaults_error": "Não foi possivel carregar a configuração de fábrica no dispositivo, verifica se está correctamente ligado à rede. Se o problema persistir tenta desligar da energia e voltar a ligar.",
}


function stringToHTML(text) {
    let parser = new DOMParser();
    let doc = parser.parseFromString(text, 'text/html');
    return doc.body;
}

function toggleActive(menu) {
    findByClass("onofre-menu").getElementsByTagName("li").item(0).classList.remove("active")
    findByClass("onofre-menu").getElementsByTagName("li").item(1).classList.remove("active")
    fetch(menu + ".html").then(function (response) {
        if (response.ok) {
            return response.text();
        }
        throw response;
    }).then(function (text) {
        let section = document.getElementsByClassName("content").item(0);
        for (const sectionElement of section.childNodes) {
            section.removeChild(sectionElement);
        }
        let html = stringToHTML(text);
        section.append(html);
        if (menu === "devices") {
            findByClass("onofre-menu").getElementsByTagName("li").item(1).classList.add("active")
            fillDevices();
        } else {
            findByClass("onofre-menu").getElementsByTagName("li").item(0).classList.add("active")
            fillConfig();
        }
    }).then(() => {
        const langSet = detectLang();
        document.querySelectorAll("span[class^='lang-']").forEach((l) => {
            const t = langSet[l.className.replace("lang-", "")];
            if (t)
                l.textContent = t;
        });
    });

}
function findByClass(c) {
    return document.getElementsByClassName(c).item(0);
}

function findById(id) {
    const a = document.getElementById(id);
    if (!a)
        console.log("NOT_FOUND " + id)
    return a;
}

function fillConfig() {
    if (!config) return;
    document.title = 'BH OnOfre ' + config.nodeId;
    let percentage = Math.min(2 * (parseInt(config.signal) + 100), 100);
    findById("wifi-signal").textContent = percentage + "%";
    findById("version_lbl").textContent = config.firmware;
    findById("lbl-chip").textContent = "ID: " + config.chipId;
    findById("lbl-mac").textContent = "MAC: " + config.mac;
    findById("ssid_lbl").textContent = config.wifiSSID;
    findById("mqtt_lbl").textContent = config.mqttIpDns;
    if (config.mqttConnected) {
        findById("mqtt_state").classList.add("online")
    } else {
        findById("mqtt_state").classList.remove("online")
    }
    findById("dhcp").checked = config.dhcp;
    findById("ff").removeAttribute('disabled');
    findById("nodeId").value = config.nodeId;
    findById("mqttIpDns").value = config.mqttIpDns;
    findById("mqttUsername").value = config.mqttUsername;
    findById("mqttPassword").value = config.mqttPassword;
    findById("wifiSSID").value = config.wifiSSID;
    findById("wifiSecret").value = config.wifiSecret;
    findById("wifiIp").value = config.wifiIp;
    findById("wifiMask").value = config.wifiMask;
    findById("wifiGw").value = config.wifiGw;
    findById("accessPointPassword").value = config.accessPointPassword;
    findById("apiPassword").value = config.apiPassword;
    findById("apiUser").value = config.apiUser;
    findById("wifiIp").value = config.wifiIp;


}

async function loadConfig() {
    const response = await fetch(baseUrl + "/config");
    config = await response.json();
}

function detectLang() {
    let lang = "PT";
   /* if (/^en/.test(navigator.language)) {
        lang = "EN";
    } else if (/^ro/.test(navigator.language)) {
        lang = "RO";
    }*/
    return window['WORDS_' + lang];
}

function saveConfig() {
    let mqttIpDns = document.getElementById("mqtt_ip");
    newConfig.nodeId = getValue("nodeId", config.nodeId).trim();
    newConfig.mqttIpDns = getValue("mqttIpDns", config.mqttIpDns).trim();
    newConfig.mqttUsername = getValue("mqttUsername", config.mqttUsername).trim();
    newConfig.mqttPassword = getValue("mqttPassword", config.mqttPassword).trim();
    newConfig.wifiSSID = getValue("ssid", config.wifiSSID).trim();
    newConfig.wifiSecret = getValue("wifiSecret", config.wifiSecret).trim();
    newConfig.wifiIp = getValue("wifiIp", config.wifiIp).trim();
    newConfig.wifiMask = getValue("wifiMask", config.wifiMask).trim();
    newConfig.wifiGw = getValue("wifiGw", config.wifiGw).trim();
    newConfig.dhcp = getValue("dhcp", config.dhcp) === "true";
    newConfig.accessPointPassword = getValue("accessPointPassword", config.accessPointPassword).trim();
    newConfig.apiPassword = getValue("apiPassword", config.apiPassword).trim();
    newConfig.apiUser = getValue("apiUser", config.apiUser).trim();
    fetch(baseUrl + "/save-config", {
        method: "POST",
        headers: { 'Content-Type': 'application/json', 'Accept': 'application/json' },
        body: JSON.stringify(newConfig)
    }).then(response => response.json()).then(json => config = json).then(() => {
        showMessage("config_save_ok");
        fillConfig();
    }).catch(() =>
        showMessage("config_save_error")
    );

}

function getValue(id, f) {
    let v = document.getElementById(id);
    return v ? v.value : f;
}

function toggleSwitch(arg) {
    const action = {
        id: parseInt(arg.id),
        state: arg.checked ? 1 : 0
    };
    fetch(baseUrl + "/control-feature", {
        method: "POST",
        headers: { 'Content-Type': 'application/json', 'Accept': 'application/json' },
        body: JSON.stringify(action)
    }).catch(() =>
        showMessage("control_state_error")
    );
}

function fillDevices() {
    let temp, item, a;
    temp = document.getElementsByTagName("template")[0];
    item = temp.content.querySelector("div");
    const modal = document.getElementById("modal");
    for (const f of config.features) {
        a = document.importNode(item, true);
        a.id = "f-"+f.id;
        a.getElementsByClassName("feature-name").item(0).textContent = f.name;
        a.getElementsByTagName("svg").item(0).classList.add(f.state > 0 ? "feature-icon-on" : "feature-icon-off");
        a.getElementsByTagName("input").item(0).checked = f.state > 0;
        a.getElementsByTagName("input").item(0).id = f.id;
        document.getElementById("devices_config").appendChild(a);
        a.getElementsByClassName("feature-name").item(0).onclick = function () {
            modal.style.display = "block";
            modal.getElementsByClassName("f-name").item(0).textContent = f.name;
            modal.getElementsByClassName("f-name").item(1).value = f.name;

        }
        source.addEventListener(f.id, (s) => {
            const box = document.getElementById("f-"+f.id);
            box.getElementsByTagName("svg").item(0).classList.remove("feature-icon-on");
            box.getElementsByTagName("svg").item(0).classList.remove("feature-icon-off");
            box.getElementsByTagName("svg").item(0).classList.add(s.data > 0 ? "feature-icon-on" : "feature-icon-off");
            box.getElementsByTagName("input").item(0).checked = s.data > 0;
        })
    }
    const span = document.getElementsByClassName("close")[0];
    span.onclick = function () {
        modal.style.display = "none";

    }
    window.onclick = function (event) {
        if (event.target === modal) {
            modal.style.display = "none";
        }
    }
}

function showMessage(key) {
    const lanSet = detectLang();
    const v = lanSet[key];
    v ? alert(v) : alert(key);
}

function reboot() {
    fetch(baseUrl + "/reboot", {
        headers: {
            'Content-Type': 'text/plain; charset=utf-8',
            'Accept': 'application/json'
        }
    }).then(response => response.status === 200 ?
        showMessage("device_reboot_ok")
        : showMessage("device_reboot_error"))
}

function loadDefaults() {
    fetch(baseUrl + "/load-defaults", {
        headers: {
            'Content-Type': 'text/plain; charset=utf-8',
            'Accept': 'application/json'
        }
    }).then(response => response.status === 200 ?
        showMessage("defaults_ok")
        : showMessage("defaults_error"))
}

document.addEventListener("DOMContentLoaded", () => {

    document.getElementById('features-btn').onclick = function (e) {
        toggleActive("devices");
    };
    document.getElementById('node-btn').onclick = function (e) {
        toggleActive("node");
    }
    loadConfig().then(() => toggleActive("node"));
    if (!!window.EventSource) {
        source = new EventSource(baseUrl + '/events');
    }
    source.addEventListener("mqtt_health", (s) => {
        if (s.data === "online") {
            findById("mqtt_state").classList.add("online")
        } else {
            findById("mqtt_state").classList.remove("online")
        }
    })
});
