let baseUrl = "http://192.168.187.149"
var config;
var lastVersion = 0.0;
let source = null;
var currentPage = "node"
var WORDS_PT = {
    "dual_push": "Pulsador Duplo",
    "dual_latch": "Normal Duplo",
    "single_latch": "Normal",
    "pin_input": "Pino Entrada",
    "pin_up": "Pino Abrir",
    "pin_down": "Pino Fechar",
    "single_push": "Pulsador",
    "update_to": "Atualizar automáticamente para a versão",
    "config_save_error": "Não foi possivel guardar a configuração atual, por favor tenta novamente.",
    "config_save_ok": "Configuração Guardada",
    "device_reboot_ok": "O dispositivo está a reiniciar, ficará disponivel dentro de 10 segundos.",
    "device_error": "Não foi possivel finalizar a acção, verifique se está correctamente ligado à rede. Se o problema persistir tente desligar da energia e voltar a ligar.",
    "device_update_ok": "O dispositivo está a atualizar, ficará disponivel dentro de 20 segundos.",
    "defaults_ok": "Configuração de fábrica aplicada com sucesso. Por favor volte a ligar-se ao Access Point e aceda ao painel de controlo pelo endereço http://192.168.4.1 no seu browser.",
}

function create() {
    let s = {};
    s.name = getValue("f-n-name", "sem nome");
    s.driver = parseInt( getValue("f-n-driver", 999));
    s.input1 =parseInt( getValue("f-n-pin-1", 999));
    s.input2 = parseInt(getValue("f-n-pin-2", 999));
    fetch(baseUrl + "/switches/virtual", {
        method: "POST",
        headers: {'Content-Type': 'application/json', 'Accept': 'application/json'},
        body: JSON.stringify(s)
    }).then(response => response.json()).then(json => config = json).then(() => {
        showMessage("config_save_ok");
        toggleActive("devices");
    }).catch(() =>
        showMessage("config_save_error")
    );

}

function getI18n(key) {
    return WORDS_PT[key];
}

function stringToHTML(text) {
    let parser = new DOMParser();
    let doc = parser.parseFromString(text, 'text/html');
    return doc.body;
}

function toggleActive(menu) {
    if (currentPage === "node" && menu === "devices") {
        applyNodeChanges();
    }
    currentPage = menu;
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
    findById("version_lbl").textContent = config.firmware + " - " + config.mcu;
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

    findById("wifiSSID").value = config.wifiSSID;
    findById("wifiIp").value = config.wifiIp;
    findById("wifiMask").value = config.wifiMask;
    findById("wifiGw").value = config.wifiGw;
    findById("apiUser").value = config.apiUser;
    findById("wifiIp").value = config.wifiIp;
    findById("wifiSecret").value = "******";
    findById("accessPointPassword").value = "******";
    findById("apiPassword").value = "******";
    findById("mqttPassword").value = "******";
    if (lastVersion > parseFloat(config.firmware)) {
        findById("btn-auto-update").classList.remove("hide");
        findById("btn-auto-update").textContent = getI18n("update_to") + " " + lastVersion;
    }
}

async function loadConfig() {
    const response = await fetch(baseUrl + "/config");
    config = await response.json();
    const vR = await fetch("https://update.bhonofre.pt/firmware/latest-version/" + config.mcu, {
        mode: "cors"
    });
    lastVersion = parseFloat(await vR.text());


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
    config.dhcp = findById("dhcp", config.dhcp).checked;
    config.accessPointPassword = getValue("accessPointPassword", config.accessPointPassword).trim();
    config.apiPassword = getValue("apiPassword", config.apiPassword).trim();
    config.apiUser = getValue("apiUser", config.apiUser).trim();
}

function saveConfig() {
    if (currentPage === "node") {
        applyNodeChanges();
    }
    fetch(baseUrl + "/config", {
        method: "POST",
        headers: {'Content-Type': 'application/json', 'Accept': 'application/json'},
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
    if ("ACTUATOR" === feature.group) {
        feature.inputMode = parseInt(document.querySelector('input[name="f-in-mode"]:checked').value);
        feature.upCourseTime = parseInt(getValue("f-up", feature.upCourseTime).trim());
        feature.downCourseTime = parseInt(getValue("f-down", feature.downCourseTime).trim());
        feature.area = parseInt(getValue("f-area", feature.area).trim());
        feature.line = parseInt(getValue("f-line", feature.line).trim());
        feature.member = parseInt(getValue("f-member", feature.member).trim());
    }
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
    let v = findById(id);
    return v ? v.value : f;
}

function shutterPercentage(arg) {
    const action = {
        id: arg.id,
        state: Math.abs(parseInt(arg.value) - 100)
    };
    fetch(baseUrl + "/actuators/control", {
        method: "POST",
        headers: {'Content-Type': 'application/json', 'Accept': 'application/json'},
        body: JSON.stringify(action)
    }).catch(() =>
        showMessage("control_state_error")
    );
}

function toggleSwitch(arg) {
    const action = {
        id: arg.id,
        state: arg.checked ? 100 : 0
    };
    fetch(baseUrl + "/actuators/control", {
        method: "POST",
        headers: {'Content-Type': 'application/json', 'Accept': 'application/json'},
        body: JSON.stringify(action)
    }).catch(() =>
        showMessage("control_state_error")
    );
}

function appendSvgPath(node, d, strokeColor) {
    let a = document.createElementNS("http://www.w3.org/2000/svg", 'path');
    a.setAttribute("d", d);
    a.setAttribute("stroke", strokeColor);
    a.setAttribute("stroke-linejoin", "round");
    a.setAttribute("stroke-linecap", "round");
    node.appendChild(a)
}

function createModal(a, modal, f) {
    a.getElementsByClassName("feature-name").item(0).onclick = function () {
        modal.style.display = "block";
        modal.getElementsByClassName("f-name").item(0).textContent = f.name;
        modal.getElementsByClassName("f-name").item(1).value = f.name;
        findById("f-knx").classList.remove("hide");
        if (f.driver.includes("COVER")) {
            findById("f-calibration").classList.remove("hide")
            findById("f-in-mode-push-lbl").outerHTML = getI18n("dual_push");
            findById("f-in-mode-latch-lbl").outerHTML = getI18n("dual_latch");
            findById("f-in-mode-push-toggle-lbl").classList.remove("hide");
            findById("f-in-mode-push-toggle-lbl").outerHTML = getI18n("single_push");
        } else {
            findById("f-push-t").classList.add("hide");
        }
        if (f.group === "SENSOR")
            findById("f-knx").classList.add("hide");
            for (let i = 0; i < modal.getElementsByClassName("f-ac").length; i++) {
                modal.getElementsByClassName("f-ac").item(i).classList.add("hide");
            }
        findById("f-up").value = f.upCourseTime;
        findById("f-down").value = f.downCourseTime;
        findById("f-area").value = f.area;
        findById("f-line").value = f.line;
        findById("f-member").value = f.member;
        findById("f-in-mode-push").checked = f.inputMode === 0;
        findById("f-in-mode-latch").checked = f.inputMode === 1;
        findById("f-in-mode-push-toggle").checked = f.inputMode === 2;
        findById("btn-delete").featureId = f.id;
        findById("btn-update").featureId = f.id;
    }
}
function driverSelect(a){
    let p2 = findById("f-n-pin-2-g");
    let pu = findById("pin-up-l");
    let pd = findById("pin-down-l");
    pu.textContent = getI18n("pin_input")
    pd.textContent = getI18n("pin_input")
    if(parseInt( a.value) === 4 || parseInt(a.value) === 5){
        p2.classList.remove("hide");
        pu.textContent = getI18n("pin_up")
        pd.textContent = getI18n("pin_down")
    }else{
        p2.classList.add("hide");
    }

}
function fillDevices() {
    let p1 = findById("f-n-pin-1");
    let p2 = findById("f-n-pin-2");
    for (const p of config.outInPins) {
        let option = document.createElement("option");
        option.text = p;
        option.value = p;
        p1.add(option);
    }
    for (const p of config.outInPins) {
        let option = document.createElement("option");
        option.text = p;
        option.value = p;
        p2.add(option);
    }
    if(config.inPins) {
        for (const p of config.inPins) {
            let option = document.createElement("option");
            option.text = p;
            option.value = p;
            p1.add(option);
        }
        for (const p of config.inPins) {
            let option = document.createElement("option");
            option.text = p;
            option.value = p;
            p2.add(option);
        }
    }
    let temp, item, a;
    const modal = findById("modal");
    for (const f of config.features) {
        temp = findById(f.group);
        item = temp.content.querySelector("div");
        a = document.importNode(item, true);
        a.id = "f-" + f.id;
        a.getElementsByClassName("feature-name").item(0).textContent = f.name ? f.name : "...";
        let icon = a.getElementsByTagName("svg").item(0);
        icon.classList.add(f.state > 0 ? "feature-icon-on" : "feature-icon-off");
        a.getElementsByTagName("svg").item(0).id = 'i-' + f.id;
        findById("devices_config").appendChild(a);
        icon = findById('i-' + f.id);

        if ("ACTUATOR" === f.group) {
            a.getElementsByTagName("input").item(0).checked = f.state > 0;
            a.getElementsByTagName("input").item(0).id = f.id;
            a.getElementsByTagName("input").item(1).value = Math.abs(parseInt(f.state) - 100);
            a.getElementsByTagName("input").item(1).id = f.id;
            if ("SWITCH" === f.family) {
                a.getElementsByClassName("shutter-slider").item(0).classList.add("hide");
                appendSvgPath(icon, "M20 12C22.7614 12 25 14.2386 25 17L25 24C25 26.7614 22.7614 29 20 29C17.2386 29 15 26.7614 15 24L15 17C15 14.2386 17.2386 12 20 12Z");
                let b = document.createElementNS("http://www.w3.org/2000/svg", 'circle');
                b.setAttribute("cx", "20");
                b.setAttribute("cy", "24");
                b.setAttribute("r", "3");
                b.setAttribute("fill", "#1D1D1D");
                icon.appendChild(b)
            } else if ("LIGHT" === f.family) {
                a.getElementsByClassName("shutter-slider").item(0).classList.add("hide");
                appendSvgPath(icon, "M 6.5292969 2.515625 A 1 1 0 0 0 5.8085938 2.8085938 A 1 1 0 0 0 5.8085938 4.2226562 A 1 1 0 0 0 7.2226562 4.2226562 A 1 1 0 0 0 7.2226562 2.8085938 A 1 1 0 0 0 6.5292969 2.515625 z M 23.5 2.515625 A 1 1 0 0 0 22.777344 2.8085938 A 1 1 0 0 0 22.777344 4.2226562 A 1 1 0 0 0 24.191406 4.2226562 A 1 1 0 0 0 24.191406 2.8085938 A 1 1 0 0 0 23.5 2.515625 z M 15 3 C 10.029 3 6 7.029 6 12 C 6 17 10 19 12 23 L 18 23 C 20 19 24 17 24 12 C 24 7.029 19.971 3 15 3 z M 15 6 L 15 11 L 19 11 L 15 18 L 15 13 L 11 13 L 15 6 z M 3 11 A 1 1 0 0 0 2 12 A 1 1 0 0 0 3 13 A 1 1 0 0 0 4 12 A 1 1 0 0 0 3 11 z M 27 11 A 1 1 0 0 0 26 12 A 1 1 0 0 0 27 13 A 1 1 0 0 0 28 12 A 1 1 0 0 0 27 11 z M 6.5292969 19.484375 A 1 1 0 0 0 5.8066406 19.777344 A 1 1 0 0 0 5.8066406 21.191406 A 1 1 0 0 0 7.2226562 21.191406 A 1 1 0 0 0 7.2226562 19.777344 A 1 1 0 0 0 6.5292969 19.484375 z M 23.498047 19.486328 A 1 1 0 0 0 22.777344 19.777344 A 1 1 0 0 0 22.777344 21.193359 A 1 1 0 0 0 24.191406 21.193359 A 1 1 0 0 0 24.191406 19.777344 A 1 1 0 0 0 23.498047 19.486328 z M 12 25 L 12 26 C 12 27.105 12.895 28 14 28 A 1 1 0 0 0 15 29 A 1 1 0 0 0 16 28 C 17.105 28 18 27.105 18 26 L 18 25 L 12 25 z");
            } else if ("SECURITY" === f.family) {
                a.getElementsByClassName("shutter-slider").item(0).classList.add("hide");
                appendSvgPath(icon, "M16.3357 25.9353H22.1166M16.3357 25.9353C16.3357 26.7471 15.6886 27.4052 14.8905 27.4052C14.0923 27.4052 13.4452 26.7471 13.4452 25.9353M16.3357 25.9353C16.3357 25.1234 15.6886 24.4654 14.8905 24.4654C14.0923 24.4654 13.4452 25.1234 13.4452 25.9353M22.1166 25.9353C22.1166 26.7471 22.7636 27.4052 23.5618 27.4052C24.36 27.4052 25.007 26.7471 25.007 25.9353M22.1166 25.9353C22.1166 25.1234 22.7636 24.4654 23.5618 24.4654C24.36 24.4654 25.007 25.1234 25.007 25.9353M13.4452 25.9353H13.1562C12.7515 25.9353 12.5491 25.9353 12.3946 25.8552C12.2586 25.7847 12.148 25.6723 12.0788 25.534C12 25.3768 12 25.171 12 24.7593V23.8774C12 23.0542 12 22.6425 12.1575 22.3281C12.2961 22.0515 12.5172 21.8266 12.7891 21.6857C13.0983 21.5255 13.503 21.5255 14.3124 21.5255H22.9837C23.5208 21.5255 23.7893 21.5255 24.0139 21.5617C25.2508 21.7609 26.2208 22.7475 26.4167 24.0055C26.4523 24.234 26.4523 24.5071 26.4523 25.0533C26.4523 25.1899 26.4523 25.2582 26.4434 25.3153C26.3944 25.6298 26.1519 25.8764 25.8427 25.9262C25.7865 25.9353 25.7194 25.9353 25.5851 25.9353H25.007M17.7809 17.1158V21.5255M13.4452 21.5255L13.6848 20.0636C13.8564 19.0163 13.9422 18.4927 14.1991 18.0997C14.4256 17.7533 14.7438 17.4792 15.117 17.3089C15.5403 17.1158 16.0622 17.1158 17.1061 17.1158H19.5376C20.2163 17.1158 20.5557 17.1158 20.8637 17.2109C21.1364 17.2951 21.3901 17.4332 21.6102 17.6173C21.859 17.8252 22.0472 18.1123 22.4237 18.6867L24.2844 21.5255", "#1D1D1D");
                appendSvgPath(icon, "M30.065 21.2316V16.48C30.065 14.558 28.5069 13 26.585 13H22.8389");
            } else if ("CLIMATE" === f.family) {
                a.getElementsByClassName("switch").item(0).classList.add("hide");
                appendSvgPath(icon, "M27 18L13 18", "#fff");
                appendSvgPath(icon, "M27 15L13 15", "#fff");
                appendSvgPath(icon, "M20 21L20 24", "#fff");
                appendSvgPath(icon, "M13.5 21.5H26.5V24C26.5 24.8284 25.8284 25.5 25 25.5H15C14.1716 25.5 13.5 24.8284 13.5 24V21.5Z", "#fff");
            }
            source.addEventListener(f.id, (s) => {
                const box = findById("f-" + f.id);
                box.getElementsByTagName("svg").item(0).classList.remove("feature-icon-on");
                box.getElementsByTagName("svg").item(0).classList.remove("feature-icon-off");
                box.getElementsByTagName("svg").item(0).classList.add(s.data > 0 ? "feature-icon-on" : "feature-icon-off");
                box.getElementsByTagName("input").item(0).checked = s.data > 0;
                box.getElementsByTagName("input").item(1).value = Math.abs(parseInt(s.data) - 100);
            })
        }
        createModal(a, modal, f);
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
        : showMessage("device_error"))
}


function loadDefaults() {
    fetch(baseUrl + "/load-defaults", {
        headers: {
            'Content-Type': 'text/plain; charset=utf-8',
            'Accept': 'application/json'
        }
    }).then(response => response.status === 200 ?
        showMessage("defaults_ok")
        : showMessage("device_error"))
}



document.addEventListener("DOMContentLoaded", () => {

    findById('features-btn').onclick = function (e) {
        toggleActive("devices");
    };
    findById('node-btn').onclick = function (e) {
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
