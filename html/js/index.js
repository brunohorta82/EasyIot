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
            loadDevice(fillSwitches, "switches", function () {
                loadDevice(fillSensors, "sensors", function () {
                });
            });
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

function removeFromSelect(select, value) {
    $("#" + select + " option[value='" + value + "']").remove();
}
function addToSelect(select, class_, value, label) {
    let sel = document.getElementById(select);
    if (sel) {
        removeFromSelect(select, value);
        let opt = document.createElement('option');
        opt.appendChild(document.createTextNode(""));
        opt.value = value;
        opt.label  = label
        sel.appendChild(opt);
        $("#" + select + " option[value='" + value + "']").addClass(class_);
    }
}
function buildSwitchTemplate() {
    if ($('#bs_NEW').length > 0) return
    let device = {
        "id": "NEW",
        "name": "Novo Interruptor",
        "family": "switch",
        "primaryGpio": 99,
        "secondaryGpio": 99,
        "upCourseTime": 25,
        "downCourseTime": 25,
        "autoStateDelay": 0,
        "autoStateValue": "",
        "typeControl": 2,
        "mode": 1,
        "knxLevelOne": 0,
        "knxLevelTwo": 0,
        "knxLevelThree": 0,
        "primaryGpioControl": 99,
        "secondaryGpioControl": 99,
    };
    buildSwitch(device);
}
function buildSensorTemplate() {
    if ($('#bss_NEW').length > 0) return
    let device = {
        "id": "NEW",
        "name": "Novo Sensor",
        "primaryGpio": 99,
        "type": 65,
        "delayRead": 2000
    };
    buildSensor(device);
}
function setOptionOnSelect(select, value) {
    let sel = document.getElementById(select);
    if (sel) {
        sel.selectedIndex = $("#" + select + " option[value='" + value + "']").index();
    }
}
function show(id) {
    $('#' + id).removeClass("hide");
}
function hide(id) {
    $('#' + id).addClass("hide");
}

function showMessage(pt) {
  alert(detectLang()[pt] || pt);
}

function loadDevice(func, e, next) {
    const targetUrl = baseUrl + "/" + e;
    $.ajax({
        url: targetUrl,
        contentType: "application/json",
        dataType: "json",
        success: function (response) {
            func(response);
            if (next) {
                next();
            }
        },
        error: function () {
            showMessage("Erro a carregar configuração de funcionalidades.")
        },
        timeout: 2000
    });
}
function fillSwitches(payload) {
    if (!payload) return;
    $('#switch_config').empty();
    for (let obj of payload) {
        buildSwitch(obj);
    }
}
function fillSensors(payload) {
    if (!payload) return;
    $('#sensors_config').empty();
    for (let obj of payload) {
        buildSensor(obj);
        source.addEventListener(obj.id, function (e) {
            updateSensorReadings(obj.id, obj.name, e.data);
        }, false);
    }
}
function applySwitchFamily(id) {
    hide("timeBetweenStatesRow_" + id);
    hide("secondaryGpioControlRow_" + id);
    hide("secondaryGpioRow_" + id);
    hide("primaryStateGpioRow_" + id);
    hide("secondaryStateGpioRow_" + id);
    hide("thirdGpioControlRow_" + id);
    hide("btn_on_" + id);
    hide("btn_close_" + id);
    hide("btn_stop_" + id);
    hide("btn_open_" + id);
    removeFromSelect('mode_' + id, 1);
    removeFromSelect('mode_' + id, 2);
    removeFromSelect('mode_' + id, 4);
    removeFromSelect('mode_' + id, 5);
    removeFromSelect('mode_' + id, 6);
    removeFromSelect('autoStateValue_' + id, "OPEN");
    removeFromSelect('autoStateValue_' + id, "CLOSE");
    removeFromSelect('autoStateValue_' + id, "STOP");
    removeFromSelect('autoStateValue_' + id, "ON");
    removeFromSelect('autoStateValue_' + id, "OFF");
    if ($('#family_' + id).val() === "cover") {
        show("btn_close_" + id);
        show("btn_stop_" + id);
        show("btn_open_" + id);
        addToSelect('mode_' + id, "lang-push", 2,"Pulsador");
        addToSelect('mode_' + id, "lang-dual-normal", 4,"Normal");
        addToSelect('mode_' + id, "lang-dual-push", 5,"Pulsador Dulplo");
        show("secondaryGpioControlRow_" + id);
        show("secondaryGpioRow_" + id);
        show("timeBetweenStatesRow_" + id);
        addToSelect('autoStateValue_' + id, "lang-open", "OPEN","Abrir");
        addToSelect('autoStateValue_' + id, "lang-close", "CLOSE","Fechar");
        addToSelect('autoStateValue_' + id, "lang-stop", "STOP","Parar");
    } else if ($('#family_' + id).val() === "garage") {
        show("btn_on_" + id);
        addToSelect('mode_' + id, "lang-push", 2,"Pulsador");
        addToSelect('autoStateValue_' + id, "lang-released", "RELEASED","Libertar");
    }  else {
        show("btn_on_" + id);
        addToSelect('mode_' + id, "lang-normal", 1,"Normal");
        addToSelect('mode_' + id, "lang-push", 2,"Pulsador");
        addToSelect('autoStateValue_' + id, "lang-on", "ON","Ligar");
        addToSelect('autoStateValue_' + id, "lang-off", "OFF","Desligar");
    }
    applySwitchMode(id);
    applyTypeControl(id);
}
function fillGpioSelect(id) {
    addToSelect(id, "lang-none", 99,"Nenhum");
    for (let gpio of config.outInPins) {
        addToSelect(id, "lang-" + gpio, gpio,gpio);
    }
}
function applySwitchMode(id) {
    if ($('#family_' + id).val() == "cover") {
        if (($('#mode_' + id).val() == 2)) {
            setOptionOnSelect('secondaryGpio_' + id, 99);
            hide("secondaryGpioRow_" + id)
        } else {
            show("secondaryGpioRow_" + id)
        }
        show("secondaryGpioControlRow_" + id)
    }else   if ($('#family_' + id).val() == "gate") {
        show("secondaryGpioControlRow_" + id)
    }
}
function applyTypeControl(id) {
    hide("secondaryGpioControlRow_" + id);
    hide("primaryGpioControlRow_" + id);
    if ($('#typeControl_' + id).val() == 1) {
        show("primaryGpioControlRow_" + id);
        if ($('#family_' + id).val() == "cover") {
                show("secondaryGpioControlRow_" + id);
        }
    } else {
        setOptionOnSelect('primaryGpioControl_' + id, 99);
        setOptionOnSelect('secondaryGpioControl_' + id, 99);
        hide("secondaryGpioControlRow_" + id);
        hide("primaryGpioControlRow_" + id);
    }
    if ($('#mode_' + id).val() == 6) {
        show("secondaryGpioControlRow_" + id);
    }

}
function applySensorRequiredGpio(id) {
    if ($('#s_type_' + id).val() != "70" && $('#s_type_' + id).val() != "71") {
        hide("s_secondaryGpioRow_" + id);
        hide("s_tertiaryGpioRow_" + id);
        setOptionOnSelect('s_secondaryGpio_' + id, 99);
        setOptionOnSelect('s_tertiaryGpio_' + id, 99);
    } else {
        show("s_secondaryGpioRow_" + id);
        show("s_tertiaryGpioRow_" + id);
    }

}
function buildSensor(obj) {
    let checkedHaSupport = obj.haSupport ? "checked" : "";
    let checkedMqttSupport = obj.mqttSupport ? "checked" : "";
    let checkedCloudIOSupport = obj.cloudIOSupport ? "checked" : "";
    $('#sensors_config').append('<div id="bss_' + obj.id + '" style="padding: 0; margin: 10px;" class="box box-primary">' +
        '                <div style="margin-bottom: 0" class="info-box bg-aqua">' +
        '                    <div class="info-box-content"><span class="info-box-text">' + obj.name + '</span>' +
        '                        <div class="pull-right"><span id="value_' + obj.id + '"></span>' +
        '                        </div>' +
        '                    </div>' +
        '                </div>' +
        '                <div style="font-size: 10px;  border: 0 solid #08c; border-radius: 0" class="box">' +
        '                    <div class="box-body no-padding">' +
        '                        <table class="table table-condensed">' +
        '                            <tbody>' +
        '                            <tr style="  border-top: 1px solid #88bf9c;">' +
        '                                <td><span><span' +
        '                                    class="lang-sensor">Sensor</span></span></td>' +
        '                                <td><select onchange="applySensorRequiredGpio(\'' + obj.id + '\');" class="form-control select-device" id="s_type_' + obj.id + '">' +
        '                                    <option value="65">PIR</option>' +
        '                                    <option value="66">RCWL-0516</option>' +
        '                                    <option value="21">LDR</option>' +
        '                                    <option value="90">DS18B20</option>' +
        '                                    <option value="56">REED SWITCH NC</option>' +
        '                                    <option value="57">REED SWITCH NO</option>' +
        '                                    <option value="0">DHT 11</option>' +
        '                                    <option value="1">DHT 21</option>' +
        '                                    <option value="2">DHT 22</option>' +
        '                                    <option value="70">PZEM 004T V2</option>' +
        '                                    <option value="71">PZEM 004T V3</option>' +
        '                                </select></td>' +
        '                            <tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-name">NOME</span></span></td>' +
        '                                <td><input class="input-device form-control" value="' + obj.name + '"' +
        '                                                            type="text" id="s_name_' + obj.id + '" placeholder="ex: sala"' +
        '                                                            maxlength="30" required/>' +
        '                                </td>' +
        '                            </tr>' +

        '                            <tr style="  border-top: 1px solid #88bf9c;">' +
        '                                <td><span><span' +
        '                                    class="lang-pin-in-a">Pino Entrada A</span></span></td>' +
        '                                <td><select class="form-control select-device" id="s_primaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            <tr>' +
        '                            <tr id="s_secondaryGpioRow_' + obj.id + '"">' +
        '                                <td><span><span' +
        '                                    class="lang-pin-in-b">Pino Entrada B</span></span></td>' +
        '                                <td><select class="form-control select-device" id="s_secondaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            <tr>' +
        '                            <tr id="s_tertiaryGpioRow_' + obj.id + '"">' +
        '                                <td><span><span' +
        '                                    class="lang-pin-in-c">Pino Entrada C</span></span></td>' +
        '                                <td><select class="form-control select-device" id="s_tertiaryGpio_' + obj.id + '">' +
        '                                </select></td>' +

        '                            <tr >' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-reading-interval">Leituras a cada </span></span></td>' +
        '                                <td ><input style="float: left; width: 70%;" class="input-device form-control" value="' + obj.delayRead / 1000 + '"' +
        '                                                            type="text" id="s_delayRead_' + obj.id + '" placeholder="ex: 12" maxlength="2" required/><span style=" margin-left:10px; float: left;" class="lang-seconds">Segundos</span> ' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td style="vertical-align: middle"><span class="label-device"><span class="lang-integrate">Integrar</span></span></td>' +
        '                                <td style="display: inline-flex">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >Home Assistant</span><input class="form-control" style="width: 20px; height: 20px; margin-right: 10px;" ' + checkedHaSupport + ' type="checkbox" id="s_haSupport_' + obj.id + '" value="' + obj.haSupport + '">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >CloudIO</span><input class="form-control" style="width: 20px; height: 20px;" ' + checkedCloudIOSupport + ' type="checkbox" id="s_cloudIOSupport_' + obj.id + '" value="' + obj.cloudIOSupport + '">' +
        '                               </td>' +
        '                            </tr>' +
        '                            </tbody>' +
        '                        </table>' +
        '                    </div>' +
        '                        <div class="box-footer save">' +
        '                            <button onclick="removeSensor(\'' + obj.id + '\')" style="font-size: 12px" class="btn btn-danger"><span' +
        '                                class="lang-remove">Remover</span></button>' +
        '                            <button  onclick="saveSensor(\'' + obj.id + '\')" style="font-size: 12px" class="btn btn-primary"><span' +
        '                                class="lang-save">Guardar</span></button>' +
        '                        </div>' +
        '                </div>' +
        '            </div>');
    fillGpioSelect('s_primaryGpio_' + obj.id + '');
    fillGpioSelect('s_secondaryGpio_' + obj.id + '');
    fillGpioSelect('s_tertiaryGpio_' + obj.id + '');
    setOptionOnSelect('s_primaryGpio_' + obj.id, obj.primaryGpio);
    setOptionOnSelect('s_secondaryGpio_' + obj.id, obj.secondaryGpio);
    setOptionOnSelect('s_tertiaryGpio_' + obj.id, obj.tertiaryGpio);
    setOptionOnSelect('s_type_' + obj.id, obj.type);
    applySensorRequiredGpio(obj.id);
}
function updateSensorReadings(id, name, value) {
    if ($('#value_' + id).length > 0) {
        $("#value_" + id).text(value);
    }
}
function buildSwitch(obj) {
    let on = obj.stateControl === 'ON' ? " " + obj.stateControl + " " : "OFF";
    let open = obj.stateControl === 'OPEN' ? " " + obj.stateControl + " " : "";
    let close = obj.stateControl === 'CLOSE' ? " " + obj.stateControl + " " : "";
    let checkedHaSupport = obj.haSupport ? "checked" : "";
    let checkedCloudIOSupport = obj.cloudIOSupport ? "checked" : "";
    source.addEventListener(obj.id, function (e) {
        let state = (e.data === "OFF" || e.data === "ON") ? "on" : e.data.toLowerCase();
        $("#btn_" + state + "_" + obj.id).removeClass("ON");
        $("#btn_" + state + "_" + obj.id).removeClass("OFF");
        $("#btn_" + state + "_" + obj.id).removeClass("OPEN");
        $("#btn_" + state + "_" + obj.id).removeClass("CLOSE");
        $("#btn_" + state + "_" + obj.id).addClass(e.data);
    }, false);
    $('#switch_config').append('<div id="bs_' + obj.id + '" style="padding: 0; margin: 10px;" class="feature-box">' +
        '                <div>' +
        '                    <div class="old-box-header">' +
        '                            <button onclick="stateSwitch(\'' + obj.id + '\',\'OPEN\')" id="btn_open_' + obj.id + '" class="' + open + ' btn btn-primary btn-control">' +
        '                                <svg width="24" height="24" viewBox="0 0 24 24">' +
        '                                    <path d="M12 8l-6 6 1.41 1.41L12 10.83l4.59 4.58L18 14z"/>' +
        '                                    <path d="M0 0h24v24H0z" fill="none"/>' +
        '                                </svg>' +
        '                            </button>' +
        '                            <button onclick="stateSwitch(\'' + obj.id + '\',\'STOP\')" id="btn_stop_' + obj.id + '" class="' + ' btn btn-primary btn-control">' +
        '                                <svg width="24" height="24" viewBox="0 0 24 24">' +
        '                                    <path d="M0 0h24v24H0z" fill="none"/>' +
        '                                    <path d="M6 6h12v12H6z"/>' +
        '                                </svg>' +
        '                            </button>' +
        '                            <button onclick="stateSwitch(\'' + obj.id + '\',\'CLOSE\')" id="btn_close_' + obj.id + '" class="' + close + ' btn btn-primary btn-control">' +
        '                                <svg width="24" height="24" viewBox="0 0 24 24">' +
        '                                    <path d="M16.59 8.59L12 13.17 7.41 8.59 6 10l6 6 6-6z"/>' +
        '                                    <path d="M0 0h24v24H0z" fill="none"/>' +
        '                                </svg>' +
        '                            </button>' +
        '                            <button onclick="rotateState(\'' + obj.id + '\')" id="btn_on_' + obj.id + '" class="' + on + ' btn btn-primary btn-control">' +
        '                                <svg style="width:24px;height:24px" viewBox="0 0 24 24">' +
        '                                    <path fill="#000000" d="M11,3H13V21H11V3Z"/>' +
        '                                </svg>' +
        '                            </button>' +
        '                    </div>' +
        '                </div>' +
        '                <div style="font-size: 10px;  border: 0 solid #08c; border-radius: 0" class="box">' +
        '                    <div class="box-body">' +
        '                        <table>' +
        '                            <tbody>' +
        '                            <tr>' +
        '                                <td>Nome</td>' +
        '                                <td ><input class="input-device form-control" value="' + obj.name + '"' +
        '                                                            type="text" id="name_' + obj.id + '" placeholder="ex: luz sala"' +
        '                                                            maxlength="30" required/>' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td>Família</td>' +
        '                                <td><select onchange="applySwitchFamily(\'' + obj.id + '\');" class="form-control select-device" id="family_' + obj.id + '">' +
        '                                    <option value="switch">Interruptor</option>' +
        '                                    <option value="light">Iluminação</option>' +
        '                                    <option value="cover">Estore</option>' +
        '                                    <option value="garage">Garagem</option>' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td>Modo</td>' +
        '                                <td><select onchange="applySwitchMode(\'' + obj.id + '\');" class="form-control select-device" id="mode_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td>Atuador</td>' +
        '                                <td><select onchange="applyTypeControl(\'' + obj.id + '\');" class="form-control select-device" id="typeControl_' + obj.id + '">' +
        '                                    <option class="lang-relay" value="1">Relé</option>' +
        '                                    <option value="2">Virtual</option>' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr style="  border-top: 1px solid #88bf9c;">' +
        '                                <td>Pino Entrada A</td>' +
        '                                <td><select class="form-control select-device" id="primaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="secondaryGpioRow_' + obj.id + '"">' +
        '                                <td>Pino Entrada B</td>' +
        '                                <td><select class="form-control select-device" id="secondaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="primaryStateGpioRow_' + obj.id + '"">' +
        '                                <td>Pino Estado A</td>' +
        '                                <td><select class="form-control select-device" id="primaryStateGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="secondaryStateGpioRow_' + obj.id + '"">' +
        '                                <td>Pino Estado B</td>' +
        '                                <td><select class="form-control select-device" id="secondaryStateGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="primaryGpioControlRow_' + obj.id + '" style="  border-top: 1px solid #d9534f;">' +
        '                                <td>Pino Saida 1</td>' +
        '                                <td><select class="form-control select-device" id="primaryGpioControl_' + obj.id + '">' +
        '                            </tr>' +
        '                            <tr id="secondaryGpioControlRow_' + obj.id + '" ">' +
        '                                <td>Pino Saida 2</td>' +
        '                                <td><select class="form-control select-device" id="secondaryGpioControl_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="thirdGpioControlRow_' + obj.id + '" ">' +
        '                                <td>Pino Saida 3</td>' +
        '                                <td><select class="form-control select-device" id="thirdGpioControl_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td>KNX</td>' +
        '                                <td ><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + obj.knxLevelOne + '"' +
        '                                                            type="text" id="knxLevelOne_' + obj.id + '" placeholder="ex: 2"' +
        '                                                             maxlength="2" required/><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + obj.knxLevelTwo + '"' +
        '                                                            type="text" id="knxLevelTwo_' + obj.id + '" placeholder="ex: 1"' +
        '                                                             maxlength="2" required/><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + obj.knxLevelThree + '"' +
        '                                                            type="text" id="knxLevelThree_' + obj.id + '" placeholder="ex: 1"' +
        '                                                             maxlength="2" required/>' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr id="timeBetweenStatesRow_' + obj.id + '" >' +
        '                                <td>Calibração</td>' +
        '                                <td><span style="float: left" class="lang-up">Subida</span><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + obj.upCourseTime / 1000 + '"' +
        '                                                            type="text" id="upCourseTime_' + obj.id + '" placeholder="ex: 12"' +
        '                                                             maxlength="5" required/><span style="float: left; margin-left: 10px" class="lang-down">Descida</span><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + obj.downCourseTime / 1000 + '"' +
        '                                                            type="text" id="downCourseTime_' + obj.id + '" placeholder="ex: 12"' +
        '                                                             maxlength="5" required/><span style="float: left;margin-right: 5px" class="lang-seconds">segundos</span>' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td>Estádo automático</td>' +
        '                                <td ><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + (obj.autoStateDelay / 1000) + '"' +
        '                                                            type="text" id="autoStateDelay_' + obj.id + '" placeholder="ex: 12"' +
        '                                                             maxlength="8" required/>' +
        '                                <select class="form-control select-device" style="float: left; width: 100px; margin-left: 5px;" id="autoStateValue_' + obj.id + '">' +
        '                                    <option class="lang-choose" value="">Escolha</option>' +
        '                                </select>' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                            <td></td><td><span style="margin-left: 5px">segundos</span> ' +
        '                            </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td style="vertical-align: middle"><span class="label-device"><span class="lang-integrate">Integrar</span></span></td>' +
        '                                <td style="display: inline-flex">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >Home Assistant</span><input class="form-control" style="width: 20px; height: 20px; margin-right: 10px;" ' + checkedHaSupport + ' type="checkbox" id="haSupport_' + obj.id + '" value="' + obj.haSupport + '">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >CloudIO</span><input class="form-control" style="width: 20px; height: 20px;" ' + checkedCloudIOSupport + ' type="checkbox" id="cloudIOSupport_' + obj.id + '" value="' + obj.cloudIOSupport + '">' +
        '                               </td>' +
        '                            </tr>' +
        '                            </tbody>' +
        '                        </table>' +
        '                    </div>' +
        '                        <div class="box-footer save">' +
        '                            <button onclick="removeSwitch(\'' + obj.id + '\')" style="font-size: 12px" class="btn btn-danger"><span' +
        '                                class="lang-remove">Remover</span></button>' +
        '                            <button  onclick="saveSwitch(\'' + obj.id + '\')" style="font-size: 12px" class="btn btn-primary"><span' +
        '                                class="lang-save">Guardar</span></button>' +
        '                        </div>' +
        '                </div>' +
        '            </div>');

    if (obj.family === "cover") {
        removeFromSelect('mode_' + obj.id, 1);
    } else {
        removeFromSelect('mode_' + obj.id, 4);
        removeFromSelect('mode_' + obj.id, 5);
    }
    fillGpioSelect('primaryGpio_' + obj.id);
    fillGpioSelect('secondaryGpio_' + obj.id);
    fillGpioSelect('primaryStateGpio_' + obj.id);
    fillGpioSelect('secondaryStateGpio_' + obj.id);
    fillGpioSelect('secondaryGpioControl_' + obj.id);
    fillGpioSelect('primaryGpioControl_' + obj.id);
    fillGpioSelect('secondaryGpioControl_' + obj.id);
    fillGpioSelect('thirdGpioControl_' + obj.id);
    setOptionOnSelect('family_' + obj.id, obj.family);
    applySwitchFamily(obj.id);
    setOptionOnSelect('typeControl_' + obj.id, obj.typeControl);
    setOptionOnSelect('mode_' + obj.id, obj.mode);
    applySwitchMode(obj.id);
    applyTypeControl(obj.id);
    setOptionOnSelect('primaryGpioControl_' + obj.id, obj.primaryGpioControl);
    setOptionOnSelect('secondaryGpioControl_' + obj.id, obj.secondaryGpioControl);
    setOptionOnSelect('thirdGpioControl_' + obj.id, obj.thirdGpioControl);
    setOptionOnSelect('primaryGpio_' + obj.id, obj.primaryGpio);
    setOptionOnSelect('secondaryGpio_' + obj.id, obj.secondaryGpio);
    setOptionOnSelect('primaryStateGpio_' + obj.id, obj.primaryStateGpio);
    setOptionOnSelect('secondaryStateGpio_' + obj.id, obj.secondaryStateGpio);
    setOptionOnSelect('autoStateValue_' + obj.id, obj.autoStateValue);
}
function stateSwitch(id, state) {
    let toggleState = state;
    if ((toggleState === "ON" || toggleState === "OFF") && ($("#btn_on_" + id).hasClass("ON") || $("#btn_on_" + id).hasClass("OFF"))) {
        toggleState = $("#btn_on_" + id).hasClass("ON") ? "OFF" : "ON";
    }
    const targetUrl = baseUrl + "/state-switch?state=" + toggleState + "&id=" + id;
    $.ajax({
        type: "GET",
        url: targetUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {

        },
        error: function () {

        }, complete: function () {

        },
        timeout: 2000
    });
}
function rotateState(id) {
    const targetUrl = baseUrl + "/rotate-state-switch?id=" + id;
    $.ajax({
        type: "GET",
        url: targetUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {

        },
        error: function () {

        }, complete: function () {

        },
        timeout: 2000
    });
}
function saveSensor(id) {
    let device = {
        "id": id,
        "name": $('#s_name_' + id).val(),
        "primaryGpio": parseInt($('#s_primaryGpio_' + id).val()),
        "secondaryGpio": parseInt($('#s_secondaryGpio_' + id).val()),
        "tertiaryGpio": parseInt($('#s_tertiaryGpio_' + id).val()),
        "type": parseInt($('#s_type_' + id).val()),
        "knxLevelOne": parseInt($('#s_knxLevelOne_' + id).val()),
        "knxLevelTwo": parseInt($('#s_knxLevelTwo_' + id).val()),
        "knxLevelThree": parseInt($('#s_knxLevelThree_' + id).val()),
        "mqttRetain": document.getElementById('s_mqttRetain_' + id).checked,
        "haSupport": document.getElementById('s_haSupport_' + id).checked,
        "emoncmsSupport": document.getElementById('s_emoncmsSupport_' + id).checked,
        "mqttSupport": document.getElementById('s_mqttSupport_' + id).checked,
        "cloudIOSupport": document.getElementById('s_cloudIOSupport_' + id).checked,
        "delayRead": parseInt($('#s_delayRead_' + id).val()) * 1000,

    };
    const targetUrl = baseUrl + "/save-sensor?id=" + device.id;
    $.ajax({
        type: "POST",
        url: targetUrl,
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(device),
        success: function (response) {
            fillSensors(response);
            showMessage("Configuração Guardada", "Config Stored");

        },
        error: function () {
            showMessage("Não foi possivel guardar a configuração atual, por favor tenta novamente.", "Unable to save current configuration, please try again.")
        }, complete: function () {

        },
        timeout: 2000
    });
}
function saveSwitch(id) {
    let device = {
        "id": id,
        "name": $('#name_' + id).val(),
        "family": $('#family_' + id).val(),
        "primaryGpio": parseInt($('#primaryGpio_' + id).val()),
        "secondaryGpio": parseInt($('#secondaryGpio_' + id).val()),
        "primaryStateGpio": parseInt($('#primaryStateGpio_' + id).val()),
        "secondaryStateGpio": parseInt($('#secondaryStateGpio_' + id).val()),
        "thirdGpioControl": parseInt($('#thirdGpioControl_' + id).val()),
        "upCourseTime": parseInt($('#upCourseTime_' + id).val()) * 1000,
        "downCourseTime": parseInt($('#downCourseTime_' + id).val()) * 1000,
        "autoStateValue": $('#autoStateValue_' + id).val(),
        "autoStateDelay": parseInt($('#autoStateDelay_' + id).val()) * 1000,
        "typeControl": parseInt($('#typeControl_' + id).val()),
        "mode": parseInt($('#mode_' + id).val()),
        "knxLevelOne": parseInt($('#knxLevelOne_' + id).val()),
        "knxLevelTwo": parseInt($('#knxLevelTwo_' + id).val()),
        "knxLevelThree": parseInt($('#knxLevelThree_' + id).val()),
        "pullup": true,
        "mqttRetain": false,
        "haSupport": document.getElementById('haSupport_' + id).checked,
        "cloudIOSupport": document.getElementById('cloudIOSupport_' + id).checked,
        "mqttSupport": document.getElementById('mqttSupport_' + id).checked,
        "inverted": false,
        "primaryGpioControl": parseInt($('#primaryGpioControl_' + id).val()),
        "secondaryGpioControl": parseInt($('#secondaryGpioControl_' + id).val()),
    };
    const targetUrl = baseUrl + "/save-switch?id=" + device.id;
    $.ajax({
        type: "POST",
        url: targetUrl,
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(device),
        success: function (response) {
            loadDevice(fillSwitches, "switches");
            showMessage("Configuração Guardada", "Config Stored")

        },
        error: function () {
            showMessage("Não foi possivel guardar a configuração atual, por favor tenta novamente.", "Unable to save current configuration, please try again.")
        }, complete: function () {

        },
        timeout: 2000
    });
}


function removeSwitch(id) {
    const targetUrl = baseUrl + "/remove-switch?id=" + id;
    $.ajax({
        url: targetUrl,
        type: 'DELETE',
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            fillSwitches(response);
        },
        error: function () {
            showMessage("Não foi possivel remvover a funcionalidade, por favor tenta novamente", "Unable to remove this feature, please try again.")
        },
        timeout: 2000
    });
}
function removeSensor(id) {
    const targetUrl = baseUrl + "/remove-sensor?id=" + id;
    $.ajax({
        url: targetUrl,
        type: 'DELETE',
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            fillSensors(response);
        },
        error: function () {
            showMessage("Não foi possivel remvover a funcionalidade, por favor tenta novamente", "Unable to remove this feature, please try again.")
        },
        timeout: 2000
    });
}
