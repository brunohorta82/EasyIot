let baseUrl = "http://192.168.187.135"
var config;
let source = null;
var currentPage = "node"
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
    currentPage = menu;
    if (menu === "devices") {
        applyNodeChanges();
    }
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

function applyNodeChanges() {
    config.nodeId = getValue("nodeId", config.nodeId).trim();
    config.mqttIpDns = getValue("mqttIpDns", config.mqttIpDns).trim();
    config.mqttUsername = getValue("mqttUsername", config.mqttUsername).trim();
    config.mqttPassword = getValue("mqttPassword", config.mqttPassword).trim();
    config.wifiSSID = getValue("ssid", config.wifiSSID).trim();
    config.wifiSecret = getValue("wifiSecret", config.wifiSecret).trim();
    config.wifiIp = getValue("wifiIp", config.wifiIp).trim();
    config.wifiMask = getValue("wifiMask", config.wifiMask).trim();
    config.wifiGw = getValue("wifiGw", config.wifiGw).trim();
    config.dhcp = getValue("dhcp", config.dhcp) === "true";
    config.accessPointPassword = getValue("accessPointPassword", config.accessPointPassword).trim();
    config.apiPassword = getValue("apiPassword", config.apiPassword).trim();
    config.apiUser = getValue("apiUser", config.apiUser).trim();
}

function saveConfig() {
    if (currentPage === "node") {
    applyNodeChanges();
    }
    fetch(baseUrl + "/save-config", {
        method: "POST",
        headers: { 'Content-Type': 'application/json', 'Accept': 'application/json' },
        body: JSON.stringify(config)
    }).then(response => response.json()).then(json => config = json).then(() => {
        showMessage("config_save_ok");
    }).catch(() =>
        showMessage("config_save_error")
    );

}

function applyFeatureChanges(e) {
    const index = config.features
        .indexOf(config.features
            .filter(f => f.id === e.featureId)[0]);
    if (index < 0) return;
    let feature = config.features[index];
    feature.name = getValue("f-name", feature.name).trim();
    feature.driver = parseInt(document.querySelector('input[name="f-driver"]:checked').value);
    saveConfig();
    toggleActive("devices");
}

function deleteFeature(e) {
    const index = config.features
        .indexOf(config.features
            .filter(f => f.id === e.featureId)[0]);
    config.features.splice(index, 1);
    if (!config.featuresToRemove) config.featuresToRemove = [];
    config.featuresToRemove.push(e.featureId)
    saveConfig();
    toggleActive("devices");
}

function getValue(id, f) {
    let v = document.getElementById(id);
    return v ? v.value : f;
}

function toggleSwitch(arg) {
    const action = {
        id: arg.id,
        state: arg.checked ? 100 : 0
    };
    fetch(baseUrl + "/control-feature", {
        method: "POST",
        headers: { 'Content-Type': 'application/json', 'Accept': 'application/json' },
        body: JSON.stringify(action)
    }).catch(() =>
        showMessage("control_state_error")
    );
}
function appendSvgPath(node,d,strokeColor){
    let a = document.createElementNS("http://www.w3.org/2000/svg", 'path');
    a.setAttribute("d",d);
    a.setAttribute("stroke",strokeColor);
    a.setAttribute("stroke-linejoin","round");
    a.setAttribute("stroke-linecap","round");
    node.appendChild(a)
}
function fillDevices() {
    let temp, item, a;
    temp = document.getElementsByTagName("template")[0];
    item = temp.content.querySelector("div");
    const modal = document.getElementById("modal");
    for (const f of config.features) {
        a = document.importNode(item, true);
        a.id = "f-" + f.id;
        a.getElementsByClassName("feature-name").item(0).textContent = f.name;
        let icon = a.getElementsByTagName("svg").item(0);
        icon.classList.add(f.state > 0 ? "feature-icon-on" : "feature-icon-off");
        a.getElementsByTagName("svg").item(0).id = 'i-'+f.id;
        a.getElementsByTagName("input").item(0).checked = f.state > 0;
        a.getElementsByTagName("input").item(0).id = f.id;
        document.getElementById("devices_config").appendChild(a);
        icon = document.getElementById('i-' + f.id);
        if("SWITCH" === f.family){
            appendSvgPath(icon,"M20 12C22.7614 12 25 14.2386 25 17L25 24C25 26.7614 22.7614 29 20 29C17.2386 29 15 26.7614 15 24L15 17C15 14.2386 17.2386 12 20 12Z");
            let b = document.createElementNS("http://www.w3.org/2000/svg", 'circle');
            b.setAttribute("cx","20");
            b.setAttribute("cy","24");
            b.setAttribute("r","3");
            b.setAttribute("fill","#1D1D1D");
            icon.appendChild(b)
        }else if("LIGHT" === f.family){
            appendSvgPath(icon,"M 6.5292969 2.515625 A 1 1 0 0 0 5.8085938 2.8085938 A 1 1 0 0 0 5.8085938 4.2226562 A 1 1 0 0 0 7.2226562 4.2226562 A 1 1 0 0 0 7.2226562 2.8085938 A 1 1 0 0 0 6.5292969 2.515625 z M 23.5 2.515625 A 1 1 0 0 0 22.777344 2.8085938 A 1 1 0 0 0 22.777344 4.2226562 A 1 1 0 0 0 24.191406 4.2226562 A 1 1 0 0 0 24.191406 2.8085938 A 1 1 0 0 0 23.5 2.515625 z M 15 3 C 10.029 3 6 7.029 6 12 C 6 17 10 19 12 23 L 18 23 C 20 19 24 17 24 12 C 24 7.029 19.971 3 15 3 z M 15 6 L 15 11 L 19 11 L 15 18 L 15 13 L 11 13 L 15 6 z M 3 11 A 1 1 0 0 0 2 12 A 1 1 0 0 0 3 13 A 1 1 0 0 0 4 12 A 1 1 0 0 0 3 11 z M 27 11 A 1 1 0 0 0 26 12 A 1 1 0 0 0 27 13 A 1 1 0 0 0 28 12 A 1 1 0 0 0 27 11 z M 6.5292969 19.484375 A 1 1 0 0 0 5.8066406 19.777344 A 1 1 0 0 0 5.8066406 21.191406 A 1 1 0 0 0 7.2226562 21.191406 A 1 1 0 0 0 7.2226562 19.777344 A 1 1 0 0 0 6.5292969 19.484375 z M 23.498047 19.486328 A 1 1 0 0 0 22.777344 19.777344 A 1 1 0 0 0 22.777344 21.193359 A 1 1 0 0 0 24.191406 21.193359 A 1 1 0 0 0 24.191406 19.777344 A 1 1 0 0 0 23.498047 19.486328 z M 12 25 L 12 26 C 12 27.105 12.895 28 14 28 A 1 1 0 0 0 15 29 A 1 1 0 0 0 16 28 C 17.105 28 18 27.105 18 26 L 18 25 L 12 25 z");
        }
        else if("SECURITY" === f.family){
            appendSvgPath(icon,"M16.3357 25.9353H22.1166M16.3357 25.9353C16.3357 26.7471 15.6886 27.4052 14.8905 27.4052C14.0923 27.4052 13.4452 26.7471 13.4452 25.9353M16.3357 25.9353C16.3357 25.1234 15.6886 24.4654 14.8905 24.4654C14.0923 24.4654 13.4452 25.1234 13.4452 25.9353M22.1166 25.9353C22.1166 26.7471 22.7636 27.4052 23.5618 27.4052C24.36 27.4052 25.007 26.7471 25.007 25.9353M22.1166 25.9353C22.1166 25.1234 22.7636 24.4654 23.5618 24.4654C24.36 24.4654 25.007 25.1234 25.007 25.9353M13.4452 25.9353H13.1562C12.7515 25.9353 12.5491 25.9353 12.3946 25.8552C12.2586 25.7847 12.148 25.6723 12.0788 25.534C12 25.3768 12 25.171 12 24.7593V23.8774C12 23.0542 12 22.6425 12.1575 22.3281C12.2961 22.0515 12.5172 21.8266 12.7891 21.6857C13.0983 21.5255 13.503 21.5255 14.3124 21.5255H22.9837C23.5208 21.5255 23.7893 21.5255 24.0139 21.5617C25.2508 21.7609 26.2208 22.7475 26.4167 24.0055C26.4523 24.234 26.4523 24.5071 26.4523 25.0533C26.4523 25.1899 26.4523 25.2582 26.4434 25.3153C26.3944 25.6298 26.1519 25.8764 25.8427 25.9262C25.7865 25.9353 25.7194 25.9353 25.5851 25.9353H25.007M17.7809 17.1158V21.5255M13.4452 21.5255L13.6848 20.0636C13.8564 19.0163 13.9422 18.4927 14.1991 18.0997C14.4256 17.7533 14.7438 17.4792 15.117 17.3089C15.5403 17.1158 16.0622 17.1158 17.1061 17.1158H19.5376C20.2163 17.1158 20.5557 17.1158 20.8637 17.2109C21.1364 17.2951 21.3901 17.4332 21.6102 17.6173C21.859 17.8252 22.0472 18.1123 22.4237 18.6867L24.2844 21.5255","#1D1D1D");
            appendSvgPath(icon,"M30.065 21.2316V16.48C30.065 14.558 28.5069 13 26.585 13H22.8389");
        }
        else if("CLIMATE" === f.family){
            appendSvgPath(icon,"M27 18L13 18","#fff");
            appendSvgPath(icon,"M27 15L13 15","#fff");
            appendSvgPath(icon,"M20 21L20 24","#fff");
            appendSvgPath(icon,"M13.5 21.5H26.5V24C26.5 24.8284 25.8284 25.5 25 25.5H15C14.1716 25.5 13.5 24.8284 13.5 24V21.5Z","#fff");
        }
        a.getElementsByClassName("feature-name").item(0).onclick = function () {
            modal.style.display = "block";
            modal.getElementsByClassName("f-name").item(0).textContent = f.name;
            modal.getElementsByClassName("f-name").item(1).value = f.name;
            document.getElementById("f-light-generic").checked = f.driver.includes("GENERIC");
            document.getElementById("f-light-push").checked = f.driver.includes("PUSH");
            document.getElementById("btn-delete").featureId = f.id;
            document.getElementById("btn-update").featureId = f.id;

        }
        source.addEventListener(f.id, (s) => {
            const box = document.getElementById("f-" + f.id);
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
