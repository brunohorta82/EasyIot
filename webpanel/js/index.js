let baseUrl = "http://192.168.187.136"
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
    "trigger": "Trigger",
    "echo": "Echo",
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
    s.driver = parseInt(getValue("f-n-driver", 999));
    s.input1 = parseInt(getValue("f-n-pin-1", 999));
    s.input2 = parseInt(getValue("f-n-pin-2", 999));
    fetch(baseUrl + "/features", {
        method: "POST",
        headers: {'Content-Type': 'application/json', 'Accept': 'application/json'},
        body: JSON.stringify(s)
    }).then(response => response.json()).then(json => config = json).then(() => {
        showMessage("config_save_ok");
        refreshFeatures();
        clearCreate();
        closeModal("wizard");


    }).catch(() =>
        showMessage("config_save_error")
    );

}

function getI18n(key) {
    return WORDS_PT[key];
}

function toggleActive(menu) {
    findByClass("onofre-menu").getElementsByTagName("li").item(1).classList.remove("active");
    findByClass("onofre-menu").getElementsByTagName("li").item(0).classList.remove("active");
    if (menu === "devices") {
        findByClass("onofre-menu").getElementsByTagName("li").item(0).classList.add("active");
        findById("node-pn").classList.add("hide");
        findById("feature-pn").classList.remove("hide");
        findById("w-add").classList.remove("hide");
    } else {
        findByClass("onofre-menu").getElementsByTagName("li").item(1).classList.add("active");
        findById("node-pn").classList.remove("hide");
        findById("feature-pn").classList.add("hide");
        findById("w-add").classList.add("hide");
    }

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

function fillConfig(updateStats) {
    if (!config) return;
    document.title = 'OnOfre ' + config.nodeId;
    if (updateStats) {
        let percentage = Math.min(2 * (parseInt(config.signal) + 100), 100);
        findById("wifi-signal").textContent = percentage + "%";
        findById("version_lbl").textContent = config.firmware + " - " + config.mcu;
        findById("lbl-chip").textContent = "ID " + config.chipId;
        findById("lbl-mac").textContent = "MAC: " + config.mac;
        findById("ssid_lbl").textContent = config.wifiSSID;
        if (config.mqttConnected) {
            findById("mqtt_state").classList.add("online")
        } else {
            findById("mqtt_state").classList.remove("online")
        }
    }
    findById("dhcp").checked = config.dhcp;
    let nodeIn = findById("nodeId")
    nodeIn.value = config.nodeId;
    nodeIn.size = nodeIn.value.length + 4
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

function applyState(id, state) {
    if (state === undefined || state.length == 0) return;
    const j = JSON.parse(state);
    if (j) {
        const label1 = findById("f-" + id).getElementsByClassName("feature-value").item(0);
        const label2 = findById("f-" + id).getElementsByClassName("feature-value").item(1);
        const icon1 = findById("i1-" + id);
        if (label1) {
            if (j.error !== undefined)
                label1.textContent = "Error";
            if (j.state !== undefined) {
                label1.textContent = j.state;
                if (j.state === "open") {
                    icon1.src = icon1.src.replace("closed", j.state);
                } else {
                    icon1.src = icon1.src.replace("open", j.state);
                }
            }
            if (j.lux !== undefined) {
                label1.textContent = Math.round(j.lux * 100) / 100 + " lux";
            }
            if (j.temperature !== undefined)
                label1.textContent = Math.trunc(j.temperature) + "º";
            if (j.rain !== undefined) {
                label1.textContent = j.rain;
                icon1.src = "https://cloudio.bhonofre.pt/img/" + j.rain + ".svg";
            }
            if (j.distance !== undefined) {
                label1.textContent = j.distance + " cm";
            }
            if (j.motion !== undefined) {
                label1.textContent = j.motion;
            }
            if (j.stationaryTargetDistance !== undefined && j.movingTargetDistance !== undefined) {
                label1.textContent = j.stationaryTargetDistance+" cm";
                label2.textContent = j.movingTargetDistance+" cm";;
            }
            if (j.temperature !== undefined && j.humidity !== undefined) {
                label1.textContent = Math.trunc(j.temperature) + "º";
                label2.textContent = Math.trunc(j.humidity) + "%";
            }
            if (j.power !== undefined && j.voltage !== undefined) {
                label1.textContent = Math.trunc(j.voltage) + "V";
                label2.textContent = Math.trunc(j.power) + "W";
            }
        }
    }
}

async function refreshFeatures() {
    findById("actuators_config").innerHTML = '';
    findById("sensors_config").innerHTML = '';
    fillDevices();
}

function showWizard() {
    findById("wizard").style.display = "block";
}

function tarif(num) {
    if (1 === num) return "Vazio";
    if (2 === num) return "Ponta";
    if (3 === num) return "Cheias";
}

function fillDevices() {
    var inUsePins = [];
    let temp, item, a;
    const modal = findById("modal");
    const wizard = findById("wizard");
    for (const f of config.features) {
        if (f.driver.includes("HAN")) {
            temp = findById("HAN");
        } else {
            temp = findById(f.group);
        }
        if (f.outputs)
            for (const o of f.outputs) {
                inUsePins.push({name: f.name + " (out)", pin: o});
            }
        if (f.inputs)
            for (const i of f.inputs) {
                inUsePins.push({name: f.name + " (in)", pin: i});
            }

        item = temp.content.querySelector("div");
        a = document.importNode(item, true);
        a.id = "f-" + f.id;
        a.getElementsByClassName("feature-name").item(0).textContent = f.name ? f.name : "...";

        let clickArea = "feature-box";
        if ("ACTUATOR" === f.group) {
            findById("actuators_config").appendChild(a);
            let control = a;
            control.id = f.id;
            control.classList.remove("OFF");
            control.classList.remove("ON");
            a.getElementsByTagName("input").item(0).value = Math.abs(parseInt(f.state) - 100);
            a.getElementsByTagName("input").item(0).id = f.id;
            if ("SWITCH" === f.family) {
                control.onclick = () => toggleSwitch(f.id);
                a.getElementsByClassName("feature-icon").item(0).src = "https://cloudio.bhonofre.pt/img/PLUG.svg";
                control.classList.add(f.state > 0 ? "ON" : "OFF");
                a.getElementsByClassName("shutter-slider").item(0).classList.add("hide");
            }
            if ( "GARDEN" === f.family) {
                control.onclick = () => toggleSwitch(f.id);
                a.getElementsByClassName("feature-icon").item(0).src = "https://cloudio.bhonofre.pt/img/GARDEN.svg";
                control.classList.add(f.state > 0 ? "ON" : "OFF");
                a.getElementsByClassName("shutter-slider").item(0).classList.add("hide");
            }else if ("LIGHT" === f.family) {
                control.onclick = () => toggleSwitch(f.id);
                control.classList.add(f.state > 0 ? "ON" : "OFF");
                a.light = true;
                a.getElementsByClassName("feature-icon").item(0).src = "https://cloudio.bhonofre.pt/img/" + f.family + (f.state > 0 ? "_ON" : "") + ".svg";
                a.getElementsByClassName("shutter-slider").item(0).classList.add("hide");
            } else if ("SECURITY" === f.family) {
                control.classList.add("OFF");
                a.cover = true;
                a.getElementsByClassName("shutter-slider").item(0).onclick = () => toggleSwitch(f.id);
                a.getElementsByClassName("feature-icon").item(0).src = "https://cloudio.bhonofre.pt/img/GARAGE.svg";
            } else if ("CLIMATE" === f.family) {
                a.getElementsByClassName("shutter-slider").item(0).addEventListener("change", (event) => {
                    shutterPercentage(event.target);
                });
                a.getElementsByClassName("feature-icon").item(0).src = "https://cloudio.bhonofre.pt/img/COVER.svg";
                a.cover = true;
                control.classList.add("OFF");
            }
            source.addEventListener(f.id, (s) => {
                const box = findById(f.id);
                if (box.cover) {
                    box.getElementsByTagName("input").item(0).value = Math.abs(parseInt(s.data) - 100);
                } else {
                    box.classList.remove("ON");
                    box.classList.remove("OFF");
                    if (box.light) {
                        box.getElementsByClassName("feature-icon").item(0).src = "https://cloudio.bhonofre.pt/img/" + f.family + (s.data > 0 ? "_ON" : "") + ".svg";
                    }
                    box.classList.add(s.data > 0 ? "ON" : "OFF");
                }

            });
        } else if (!f.driver.includes("HAN")) {
            findById("sensors_config").appendChild(a);
            const label1 = a.getElementsByClassName("feature-value").item(0);
            const label2 = a.getElementsByClassName("feature-value").item(1);
            a.getElementsByClassName("sensor-icon").item(0).id = "i1-" + f.id;
            const icon1 = a.getElementsByClassName("sensor-icon").item(0);
            const icon2 = a.getElementsByClassName("sensor-icon").item(1);
            icon2.classList.add("hide");
            if (f.driver === 'SHT4X' || f.driver === 'DHT_11' || f.driver === 'DHT_21' || f.driver === 'DHT_22') {
                icon1.classList.remove("hide");
                icon2.classList.remove("hide");
                icon1.src = "https://cloudio.bhonofre.pt/img/TEMPERATURE.svg";
                icon2.src = "https://cloudio.bhonofre.pt/img/HUMIDITY.svg";
            } else if (f.driver === 'DS18B20') {
                icon1.src = "https://cloudio.bhonofre.pt/img/TEMPERATURE.svg";
            } else if (f.driver === 'LTR303') {
                icon1.src = "https://cloudio.bhonofre.pt/img/LUX.svg";
            } else if (f.driver === 'PIR' ) {
                icon1.src = "https://cloudio.bhonofre.pt/img/MOTION.svg";
            }else if(f.driver === 'LD2410'){
                icon2.classList.remove("hide");
                icon1.src = "https://cloudio.bhonofre.pt/img/MOTION.svg";
                icon2.src = "https://cloudio.bhonofre.pt/img/MOTION.svg";
            } else if (f.driver === 'RAIN') {
                icon1.src = "https://cloudio.bhonofre.pt/img/rain.svg";
            } else if (f.driver === 'PZEM_004T_V03') {
                icon2.classList.remove("hide");
                icon1.src = "https://cloudio.bhonofre.pt/img/voltage.svg";
                icon2.src = "https://cloudio.bhonofre.pt/img/ENERGY.svg";
            } else if (f.driver === 'DOOR') {
                icon1.src = "https://cloudio.bhonofre.pt/img/door_closed.svg";
            } else if (f.driver === 'WINDOW') {
                icon1.src = "https://cloudio.bhonofre.pt/img/window_closed.svg";
            } else if (f.driver === 'HCSR04' || f.driver === 'TMF880X') {
                icon1.src = "https://cloudio.bhonofre.pt/img/distance.svg";
            }
            if (f.state !== undefined) {
                applyState(f.id, f.state);
            }
            source.addEventListener(f.id, (s) => {
                applyState(f.id, s.data);
            })
        } else if (f.driver.includes("HAN")) {
            findById("han_config").appendChild(a);
            source.addEventListener(f.id, (s) => {
                const j = JSON.parse(s.data);
                if (j)
                    Object.keys(j).forEach(function (key) {
                            let v = findById(key);
                            if (v) {
                                v.textContent = j[key];
                                if ('tarif' === key) {
                                    v.textContent = tarif(j[key]);
                                }
                                if ('power' === key) {
                                    findById('power_1').textContent = j[key];
                                }

                                if (j['status'] != 'OK') {
                                    findById('status').textContent = j[key];
                                }
                                if (j['error'] === 0) {
                                    findById('error').textContent = "-";
                                }
                            }
                        }
                    );
            });
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
        if (event.target === wizard) {
            wizard.style.display = "none";
        }
    }
    if ("ESP8266-HAN" === config.mcu) {
        findById("wizard").innerHTML = '';
        findById("w-add").innerHTML = '';
    } else {
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

        if (config.inPins) {
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
    if (/^en/.test(navigator.language)) {
        lang = "EN";
    } else if (/^ro/.test(navigator.language)) {
        lang = "RO";
    }
    return window['WORDS_' + lang];
}

function applyNodeChanges() {
    config.nodeId = getValue("nodeId", config.nodeId).trim();
    config.mqttIpDns = getValue("mqttIpDns", config.mqttIpDns).trim();
    config.mqttUsername = getValue("mqttUsername", config.mqttUsername).trim();
    config.mqttPassword = getValue("mqttPassword", config.mqttPassword).trim();
    config.wifiSSID = getValue("wifiSSID", config.wifiSSID).trim();
    config.wifiSecret = getValue("wifiSecret", config.wifiSecret).trim();
    config.wifiIp = getValue("wifiIp", config.wifiIp).trim();
    config.wifiMask = getValue("wifiMask", config.wifiMask).trim();
    config.wifiGw = getValue("wifiGw", config.wifiGw).trim();
    config.dhcp = findById("dhcp", config.dhcp).checked;
    config.accessPointPassword = getValue("accessPointPassword", config.accessPointPassword).trim();
    config.apiPassword = getValue("apiPassword", config.apiPassword).trim();
    config.apiUser = getValue("apiUser", config.apiUser).trim();
}

function saveConfig(update) {
    if (currentPage === "node") {
        applyNodeChanges();
    }
    fetch(baseUrl + "/config", {
        method: "POST",
        headers: {'Content-Type': 'application/json', 'Accept': 'application/json'},
        body: JSON.stringify(config)
    }).then(response => response.json()).then(json => config = json).then(() => {
        showMessage("config_save_ok");
        if (update) {
            update();
        }
    }).catch(() =>
        showMessage("config_save_error")
    );
}

function closeModal(id) {
    const modal = findById(id);
    if (modal)
        modal.style.display = 'none';
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
        feature.autoOff = parseInt(getValue("f-autoOff", feature.autoOff).trim());
    }
    saveConfig(refreshFeatures);
    closeModal("modal");
}

function deleteFeature(e) {
    const index = config.features
        .indexOf(config.features
            .filter(f => f.id === e.featureId)[0]);
    config.features.splice(index, 1);
    if (!config.featuresToRemove) config.featuresToRemove = [];
    config.featuresToRemove.push(e.featureId)
    saveConfig(refreshFeatures);
    closeModal("modal");
}

function getValue(id, f) {
    let v = findById(id);
    return v ? v.value : f;
}
function setValue(id, value) {
    let v = findById(id);
    if(v)
        v.value = value;
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
        id: arg,
        state: 102
    };
    fetch(baseUrl + "/actuators/control", {
        method: "POST",
        headers: {'Content-Type': 'application/json', 'Accept': 'application/json'},
        body: JSON.stringify(action)
    }).catch(() =>
        showMessage("control_state_error")
    );
}

function createModal(a, modal, f) {
    if (a.getElementsByClassName("feature-edit").item(0) === null) return;
    a.getElementsByClassName("feature-edit").item(0).onclick = function (ev) {
        ev.stopPropagation();
        modal.style.display = "block";
        modal.getElementsByClassName("f-name").item(0).value = f.name;
        findById("f-knx").classList.remove("hide");
           findById("f-auto").classList.add("hide");
        modal.getElementsByClassName("f-ac").item(0).classList.remove("hide");
        findById("f-autoOff").value = f.autoOff;
        if (f.driver.includes("COVER")) {
            findById("f-calibration").classList.remove("hide")
            findById("f-in-mode-push-lbl").outerHTML = getI18n("dual_push");
            findById("f-in-mode-latch-lbl").outerHTML = getI18n("dual_latch");
            findById("f-in-mode-push-toggle-lbl").classList.remove("hide");
            findById("f-in-mode-push-toggle-lbl").outerHTML = getI18n("single_push");
        } else {
            if(f.typeControl == 1 && (f.driver.includes("LIGHT") || f.driver.includes("SWITCH") || f.driver.includes("GARDEN"))) {
                findById("f-auto").classList.remove("hide");
            }
            findById("f-push-t").classList.add("hide");
        }
        if (f.group === "SENSOR") {
            findById("f-knx").classList.add("hide");
            modal.getElementsByClassName("f-ac").item(0).classList.add("hide");
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

function clearCreate(a) {
    setValue("f-n-name",null);
    setValue("f-n-pin-1", 7);
    setValue("f-n-pin-2", 7);
    setValue("f-n-driver", 7);
    let p2 = findById("f-n-pin-2-g");
    p2.classList.remove("hide");
    p2.classList.add("hide");
}

function driverSelect(a) {
    let p2 = findById("f-n-pin-2-g");
    let p1l = findById("pin-up-l");
    let p2l = findById("pin-down-l");
    p1l.textContent = getI18n("pin_input")
    p2l.textContent = getI18n("pin_input")
    if (parseInt(a.value) === 4 || parseInt(a.value) === 5) {
        p2.classList.remove("hide");
        p1l.textContent = getI18n("pin_up")
        p2l.textContent = getI18n("pin_down")
    } else if (parseInt(a.value) === 72 ||parseInt(a.value) === 71 || parseInt(a.value) === 94) {
        p2.classList.remove("hide");
        p1l.textContent = 'RX'
        p2l.textContent = 'TX'
    } else if (parseInt(a.value) === 93) {
        p2.classList.remove("hide");
        p1l.textContent = getI18n("trigger")
        p2l.textContent = getI18n("echo")
    } else {
        p2.classList.add("hide");
    }

}

function showMessage(key) {
    const lanSet = detectLang();
    const v = lanSet[key];
    v ? alert(v) : alert(key);
}

async function backup() {
    const a = document.createElement("a");
    config.backup = true;
    a.href = URL.createObjectURL(
        new Blob([JSON.stringify(config, null, 2)], {
            type: "application/json"
        })
    );
    a.setAttribute("download", "CONFIG_" + config.nodeId + "_ONOFRE_" + config.firmware + ".json");
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
}

function reboot() {
    fetch(baseUrl + "/reboot", {
        method: "POST",
        headers: {
            'Content-Type': 'text/plain; charset=utf-8',
            'Accept': 'application/json'
        }
    }).then(response => response.status === 200 ?
        showMessage("device_reboot_ok")
        : showMessage("device_error"))
}

(function () {
    function onChange(event) {
        var reader = new FileReader();
        reader.onload = onReaderLoad;
        reader.readAsText(event.target.files[0]);
    }

    function onReaderLoad(event) {
        config = JSON.parse(event.target.result);
        fillConfig(false);
        refreshFeatures();
    }

    document.getElementById('restore').addEventListener('change', onChange);

}());

function loadDefaults() {
    fetch(baseUrl + "/load-defaults", {
        method: "POST",
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
    loadConfig().then(() => toggleActive("devices")).then(() => {
        fillConfig(true);
        fillDevices();
    });
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
