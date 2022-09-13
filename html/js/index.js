const endpoint = {
    baseUrl: ""
};
var source = null;
function removeFromSelect(select, value) {
    $("#" + select + " option[value='" + value + "']").remove();
}
function addToSelect(select, class_, value) {
    let sel = document.getElementById(select);
    if (sel) {
        removeFromSelect(select, value);
        let opt = document.createElement('option');
        opt.appendChild(document.createTextNode(""));
        opt.value = value;
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
        "pullup": false,
        "mqttRetain": false,
        "inverted": false,
        "mqttCommandTopic": "../set",
        "mqttStateTopic": "../state",
        "mqttPositionCommandTopic": "../setposition",
        "mqttPositionStateTopic": "../position",
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
        "knxLevelOne": 0,
        "knxLevelTwo": 0,
        "knxLevelThree": 0,
        "mqttRetain": true,
        "mqttStateTopic": "../state",
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

var config;
var WORDS_EN = {
    "pin-state-a":"Pin State A",
    "gate":"Gate",
    "pin-state-b":"Pin State B",
    "pin-out-3": "Output Pin C",
    "update-from-server": "NEW UPGRADE",
    "node": "NODE",
    "up": "Up",
    "down": "Down",
    "reading-interval": "Readings every",
    "sensors": "Sensors",
    "integrations": "INTEGRATIONS",
    "update": "UPDATE",
    "features": "FEATURES",
    "current-version": "Current version",
    "new-file": "Choose new version file",
    "install-new-version": "Auto Upgrade to version",
    "install-file-version": "Install file version",
    "version": "Version",
    "save": "Save",
    "clean-fields": "Clear all Fields",
    "username": "Username",
    "password": "Password",
    "yes": "Yes",
    "no": "No",
    "netmask": "Netmask",
    "system": "System",
    "name": "Name",
    "restart": "Restart",
    "seconds": "seconds",
    "in": "in",
    "reset-factory": "Load Factory Defaults",
    "switches": "Switches",
    "remove": "Remove",
    "new": "New",
    "family": "Family",
    "switch": "Switch",
    "light": "Light",
    "cover": "Cover",
    "garage": "Garage",
    "released": "Released",
    "disconnected": "disconnected",
    "dconnected": "connected",
    "integrate": "Integrate",
    "normal": "Generic",
    "choose": "select",
    "push": "Push",
    "dual-push": "Dual Push",
    "dual-normal": "Dual Generic",
    "mode": "Mode",
    "relay-mqtt": "Relay / MQTT",
    "control": "Output",
    "pin-in-a": "Input Pin a",
    "pin-in-b": "Input Pin b",
    "pin-out-1": "Output Pin a",
    "pin-out-2": "Output Pin b",
    "none": "None",
    "retain-message": "Retain Messages",
    "command": "Command",
    "state": "State",
    "on": "On",
    "auto-state": "Auto State",
    "off": "Off",
    "open": "Open",
    "close": "Closed",
    "mqtt": "MQTT",
    "stop": "Stop",
    "time": "Time",
    "17": "A0",
    "group": "Group",
    "pin-in-c": "Input Pin c",
    "line": "Line",
    "member": "Member",
    "address": "Address",
    "prefix": "Prefix",
    "apikey": "API Key"

};
function loadsLanguage(lang) {
    localStorage.setItem('lang', "EN");
    $('span[class^="lang"]').each(function () {
        var langVar = (this.className).replace('lang-', '');
        var text = window['WORDS_' + lang][langVar];
        $(this).text(text);
    });
    $('option[class^="lang"]').each(function () {
        var langVar = (this.className).replace('lang-', '');
        var text = window['WORDS_' + lang][langVar];
        if (!text) {
            text = langVar;
        }
        $(this).text(text);
    });
}

function showMessage(pt, en) {
    localStorage.getItem('lang').toString() === "PT" ? alert(pt) : alert(en);
}

function showText(pt, en) {
    return localStorage.getItem('lang').toString() === "PT" ? pt : en;
}

function loadConfig() {
    const targetUrl = endpoint.baseUrl + "/config";
    $.ajax({
        url: targetUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            config = response;
            fillConfig();
        },
        error: function () {
            showMessage("Erro a carregar configuração", "Configuration load failed.")
        }, complete: function () {

        },
        timeout: 2000
    });
}

function loadDevice(func, e, next) {
    const targetUrl = endpoint.baseUrl + "/" + e;
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
            showMessage("Erro a carregar configuração de funcionalidades.", "Features Configuration load failed.")
        },
        timeout: 2000
    });
}

function fillConfig() {
    if (!config) return;
    $(".bh-model").text(config.hardware);
    $(".bh-onofre-item").removeClass("hide");
    $("#version_lbl").text(config.firmware);
    $("#lbl-chip").text(config.chipId);
    $("#lbl-mac").text(config.mac);
    $('input[name="nodeId"]').val(config.nodeId);
    $(document).prop('title', 'BH OnOfre ' + config.nodeId);
    $('input[name="mqttIpDns"]').val(config.mqttIpDns);
    $('#mqtt_lbl').text(config.mqttIpDns);
    $('input[name="mqttUsername"]').val(config.mqttUsername);
    $('input[name="mqttPassword"]').val(config.mqttPassword);
    $('input[name="wifiSSID"]').val(config.wifiSSID);
    $('input[name="wifiSecret"]').val(config.wifiSecret);
    $('input[name="knxArea"]').val(config.knxArea);
    $('input[name="knxMember"]').val(config.knxMember);
    $('input[name="knxLine"]').val(config.knxLine);
    let staticIp = document.getElementById("staticIp");
    if (staticIp) {
        staticIp.checked = !config.staticIp;
    }
    $('input[name="wifiIp"]').val(config.wifiIp);
    $('input[name="wifiMask"]').val(config.wifiMask);
    $('input[name="wifiGw"]').val(config.wifiGw);
    $('input[name="apSecret"]').val(config.apSecret);
    $('input[name="emoncmsServer"]').val(config.emoncmsServer);
    $('input[name="emoncmsPath"]').val(config.emoncmsPath);
    $('input[name="emoncmsApikey"]').val(config.emoncmsApikey);
    $('#ff').prop('disabled', false);

}

function toggleActive(menu) {
    $('.sidebar-menu').find('li').removeClass('active');
    $('.menu-item[data-menu="' + menu + '"]').closest('li').addClass('active');
    $(".content").load(menu + ".html", function () {
        if (menu === "devices") {
            loadDevice(fillSwitches, "switches", function () {
                loadDevice(fillSensors, "sensors", function () {
                });

            });
        } else {
            systemStatus();
            fillConfig();
        }
        loadsLanguage(localStorage.getItem('lang'));
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
    if ($('#family_' + id).val() == "cover") {
        show("btn_close_" + id);
        show("btn_stop_" + id);
        show("btn_open_" + id);
        addToSelect('mode_' + id, "lang-push", 2);
        addToSelect('mode_' + id, "lang-dual-normal", 4);
        addToSelect('mode_' + id, "lang-dual-push", 5);
        addToSelect('mode_' + id, "lang-gate", 6);
        show("secondaryGpioControlRow_" + id);
        show("secondaryGpioRow_" + id);
        show("timeBetweenStatesRow_" + id);
        addToSelect('autoStateValue_' + id, "lang-open", "OPEN");
        addToSelect('autoStateValue_' + id, "lang-close", "CLOSE");
        addToSelect('autoStateValue_' + id, "lang-stop", "STOP");
    } else if ($('#family_' + id).val() == "garage") {
        show("btn_on_" + id);
        addToSelect('mode_' + id, "lang-push", 2);
        addToSelect('mode_' + id, "lang-gate", 6);
        addToSelect('autoStateValue_' + id, "lang-released", "RELEASED");
    }  else {
        show("btn_on_" + id);
        addToSelect('mode_' + id, "lang-normal", 1);
        addToSelect('mode_' + id, "lang-push", 2);
        addToSelect('autoStateValue_' + id, "lang-on", "ON");
        addToSelect('autoStateValue_' + id, "lang-off", "OFF");
    }
    applySwitchMode(id);
    applyTypeControl(id);
    loadsLanguage(localStorage.getItem('lang'));

}

function fillGpioSelect(id) {
    var gpios = ["17", "0", "1", "2", "3", "4", "5", "12", "13", "14", "15", "16"];
    addToSelect(id, "lang-none", 99);
    for (let gpio of gpios) {
        addToSelect(id, "lang-" + gpio, gpio);
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
    loadsLanguage(localStorage.getItem('lang'));
}

function ifdef(value, defaultValue) {
    if (value) return value;
    return defaultValue;
}

function applyTypeControl(id) {
    hide("secondaryGpioControlRow_" + id);
    hide("primaryGpioControlRow_" + id);
    if ($('#typeControl_' + id).val() == 1) {
        show("primaryGpioControlRow_" + id);
        if ($('#family_' + id).val() == "cover") {
            if ($('#mode_' + id).val() != 2) {
                show("secondaryGpioControlRow_" + id);
            }
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
    let checkedMqttRetain = obj.mqttRetain ? "checked" : "";
    let checkedHaSupport = obj.haSupport ? "checked" : "";
    let checkedEmoncmsSupport = obj.emoncmsSupport ? "checked" : "";
    let checkedMqttSupport = obj.mqttSupport ? "checked" : "";
    let checkedCloudIOSupport = obj.cloudIOSupport ? "checked" : "";
    $('#sensors_config').append('<div id="bss_' + obj.id + '" style="padding: 0; margin: 10px;" class="col-lg-4 col-md-6 col-xs-12">' +
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
        '                                <td><span class="label-device "><span' +
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
        '                                <td class="col-xs-8"><input class="input-device form-control" value="' + obj.name + '"' +
        '                                                            type="text" id="s_name_' + obj.id + '" placeholder="ex: sala"' +
        '                                                            maxlength="30" required/>' +
        '                                </td>' +
        '                            </tr>' +

        '                            <tr style="  border-top: 1px solid #88bf9c;">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-in-a">Pino Entrada A</span></span></td>' +
        '                                <td><select class="form-control select-device" id="s_primaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            <tr>' +
        '                            <tr id="s_secondaryGpioRow_' + obj.id + '"">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-in-b">Pino Entrada B</span></span></td>' +
        '                                <td><select class="form-control select-device" id="s_secondaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            <tr>' +
        '                            <tr id="s_tertiaryGpioRow_' + obj.id + '"">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-in-c">Pino Entrada C</span></span></td>' +
        '                                <td><select class="form-control select-device" id="s_tertiaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            <tr>' +
        '                                <td><span class="label-device" style="color: #88bf9c; font-size: 13px;">MQTT</span></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device-indent"><span class="lang-state">Estado</span></span></td>' +
        '                                <td><span  style="word-break: break-word">' + obj.mqttStateTopic + '</span></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td style="vertical-align: middle"><span class="label-device-indent label-device"><span class="lang-retain-message">Reter Mensagens</span></span></td>' +
        '                                <td><input class="form-control" style="width: 20px; height: 20px;" ' + checkedMqttRetain + ' type="checkbox" id="s_mqttRetain_' + obj.id + '" value="' + obj.mqttRetain + '"></td>' +
        '                            </tr>' +

        '                            <tr >' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-reading-interval">Leituras a cada </span></span></td>' +
        '                                <td class="col-xs-8"><input style="float: left; width: 70%;" class="input-device form-control" value="' + obj.delayRead / 1000 + '"' +
        '                                                            type="text" id="s_delayRead_' + obj.id + '" placeholder="ex: 12"' +
        '                                                             maxlength="2" required/><span style=" margin-left' +
        ':10px; float: left;" ' +
        '                                    class="lang-seconds">Segundos</span> ' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td style="vertical-align: middle"><span class="label-device"><span class="lang-integrate">Integrar</span></span></td>' +
        '                                <td style="display: inline-flex">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >Home Assistant</span><input class="form-control" style="width: 20px; height: 20px; margin-right: 10px;" ' + checkedHaSupport + ' type="checkbox" id="s_haSupport_' + obj.id + '" value="' + obj.haSupport + '">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >Emoncms</span><input class="form-control" style="width: 20px; height: 20px;" ' + checkedEmoncmsSupport + ' type="checkbox" id="s_emoncmsSupport_' + obj.id + '" value="' + obj.emoncmsSupport + '">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >MQTT</span><input class="form-control" style="width: 20px; height: 20px;" ' + checkedMqttSupport + ' type="checkbox" id="s_mqttSupport_' + obj.id + '" value="' + obj.mqttSupport + '">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >CloudIO</span><input class="form-control" style="width: 20px; height: 20px;" ' + checkedCloudIOSupport + ' type="checkbox" id="s_cloudIOSupport_' + obj.id + '" value="' + obj.cloudIOSupport + '">' +
        '                               </td>' +
        '                            </tr>' +
        '                            </tbody>' +
        '                        </table>' +
        '                        <div class="box-footer save">' +
        '                            <button onclick="removeSensor(\'' + obj.id + '\')" style="font-size: 12px" class="btn btn-danger"><span' +
        '                                class="lang-remove">Remover</span></button>' +
        '                            <button  onclick="saveSensor(\'' + obj.id + '\')" style="font-size: 12px" class="btn btn-primary"><span' +
        '                                class="lang-save">Guardar</span></button>' +
        '                        </div>' +
        '                    </div>' +
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
    loadsLanguage(localStorage.getItem('lang'));
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
    let checkedMqttSupport = obj.mqttSupport ? "checked" : "";
    let checkedHaSupport = obj.haSupport ? "checked" : "";
    let checkedCloudIOSupport = obj.cloudIOSupport ? "checked" : "";
    let checkedKnxSupport = obj.knxSupport ? "checked" : "";
    source.addEventListener(obj.id, function (e) {
        let state = (e.data === "OFF" || e.data === "ON") ? "on" : e.data.toLowerCase();
        $("#btn_" + state + "_" + obj.id).removeClass("ON");
        $("#btn_" + state + "_" + obj.id).removeClass("OPEN");
        $("#btn_" + state + "_" + obj.id).removeClass("CLOSE");
        $("#btn_" + state + "_" + obj.id).addClass(e.data);
    }, false);
    $('#switch_config').append('<div id="bs_' + obj.id + '" style="padding: 0; margin: 10px;" class="col-lg-4 col-md-6 col-xs-12">' +
        '                <div style="margin-bottom: 0" class="info-box bg-aqua">' +
        '                    <div class="info-box-content"><span class="info-box-text">' + obj.name + '</span>' +
        '                        <div class="pull-right">' +
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
        '                        </div>' +
        '                    </div>' +
        '                </div>' +
        '                <div style="font-size: 10px;  border: 0 solid #08c; border-radius: 0" class="box">' +
        '                    <div class="box-body no-padding">' +
        '                        <table class="table table-condensed">' +
        '                            <tbody>' +
        '                            <tr>' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-name">NOME</span></span></td>' +
        '                                <td class="col-xs-8"><input class="input-device form-control" value="' + obj.name + '"' +
        '                                                            type="text" id="name_' + obj.id + '" placeholder="ex: luz sala"' +
        '                                                            maxlength="30" required/>' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-family">FAMILIA</span></span></td>' +
        '                                <td><select onchange="applySwitchFamily(\'' + obj.id + '\');" class="form-control select-device" id="family_' + obj.id + '">' +
        '                                    <option class="lang-switch" value="switch">Interruptor</option>' +
        '                                    <option class="lang-light" value="light">Luz</option>' +
        '                                    <option class="lang-cover" value="cover">Estore</option>' +
        '                                    <option class="lang-garage" value="garage">Garagem</option>' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-mode">MODO</span></span></td>' +
        '                                <td><select onchange="applySwitchMode(\'' + obj.id + '\');" class="form-control select-device" id="mode_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-control">Controla</span></span></td>' +
        '                                <td><select onchange="applyTypeControl(\'' + obj.id + '\');" class="form-control select-device" id="typeControl_' + obj.id + '">' +
        '                                    <option class="lang-relay-mqtt" value="1">Relé / MQTT</option>' +
        '                                    <option class="lang-mqtt" value="2">MQTT</option>' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr style="  border-top: 1px solid #88bf9c;">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-in-a">Pino Entrada A</span></span></td>' +
        '                                <td><select class="form-control select-device" id="primaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="secondaryGpioRow_' + obj.id + '"">' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-pin-in-b">Pino Entrada B</span></span></td>' +
        '                                <td><select class="form-control select-device" id="secondaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="primaryStateGpioRow_' + obj.id + '"">' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-pin-state-a">Pino Estado A</span></span></td>' +
        '                                <td><select class="form-control select-device" id="primaryStateGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="secondaryStateGpioRow_' + obj.id + '"">' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-pin-state-b">Pino Estado B</span></span></td>' +
        '                                <td><select class="form-control select-device" id="secondaryStateGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="primaryGpioControlRow_' + obj.id + '" style="  border-top: 1px solid #d9534f;">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-out-1">Pino Saida 1</span></span></td>' +
        '                                <td><select class="form-control select-device" id="primaryGpioControl_' + obj.id + '">' +
        '                            </tr>' +
        '                            <tr id="secondaryGpioControlRow_' + obj.id + '" ">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-out-2">Pino Saida 2</span></span></td>' +
        '                                <td><select class="form-control select-device" id="secondaryGpioControl_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="thirdGpioControlRow_' + obj.id + '" ">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-out-3">Pino Saida 3</span></span></td>' +
        '                                <td><select class="form-control select-device" id="thirdGpioControl_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device" style="color: #88bf9c; font-size: 13px;">MQTT</span></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device-indent"><span class="lang-command">Comando</span></span></td>' +
        '                                <td> <span style="word-break: break-word" >%u/%c' + obj.family +'/' + obj.id + '/set </span></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device-indent"><span class="lang-state">Estado</span></span></td>' +
        '                                <td><span  style="word-break: break-word">%u/%c' + obj.family +'/' + obj.id + '/status</span></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device" style="color: dodgerblue; font-size: 13px;">KNX</span></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device-indent"><span' +
        '                                    class="lang-group">Grupo</span></span></td>' +
        '                                <td class="col-xs-8"><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + obj.knxLevelOne + '"' +
        '                                                            type="text" id="knxLevelOne_' + obj.id + '" placeholder="ex: 2"' +
        '                                                             maxlength="2" required/><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + obj.knxLevelTwo + '"' +
        '                                                            type="text" id="knxLevelTwo_' + obj.id + '" placeholder="ex: 1"' +
        '                                                             maxlength="2" required/><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + obj.knxLevelThree + '"' +
        '                                                            type="text" id="knxLevelThree_' + obj.id + '" placeholder="ex: 1"' +
        '                                                             maxlength="2" required/>' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr id="timeBetweenStatesRow_' + obj.id + '" >' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-time">TEMPO Subida/Descida</span></span></td>' +
        '                                <td class="col-xs-4"><span style="float: left" class="lang-up">Subida</span><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + obj.upCourseTime / 1000 + '"' +
        '                                                            type="text" id="upCourseTime_' + obj.id + '" placeholder="ex: 12"' +
        '                                                             maxlength="5" required/><span style="float: left; margin-left: 10px" class="lang-down">Descida</span><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + obj.downCourseTime / 1000 + '"' +
        '                                                            type="text" id="downCourseTime_' + obj.id + '" placeholder="ex: 12"' +
        '                                                             maxlength="5" required/><span style="float: left;margin-right: 5px" class="lang-seconds">segundos</span>' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-auto-state">Estádo automático</span></span></td>' +
        '                                <td class="col-xs-8"><span style="float: left" class="lang-in">em</span><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + (obj.autoStateDelay / 1000) + '"' +
        '                                                            type="text" id="autoStateDelay_' + obj.id + '" placeholder="ex: 12"' +
        '                                                             maxlength="8" required/><span style="float: left; margin-left: 5px;" class="lang-seconds">segundos</span> ' +
        '                                <select class="form-control select-device" style="float: left; width: 100px; margin-left: 5px;" id="autoStateValue_' + obj.id + '">' +
        '                                    <option class="lang-choose" value="">Escolha</option>' +
        '                                </select>' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td style="vertical-align: middle"><span class="label-device"><span class="lang-integrate">Integrar</span></span></td>' +
        '                                <td style="display: inline-flex">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >KNX</span><input class="form-control" style="width: 20px; height: 20px; margin-right: 10px;" ' + checkedKnxSupport + ' type="checkbox" id="knxSupport_' + obj.id + '" value="' + obj.knxSupport + '">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >Home Assistant</span><input class="form-control" style="width: 20px; height: 20px; margin-right: 10px;" ' + checkedHaSupport + ' type="checkbox" id="haSupport_' + obj.id + '" value="' + obj.haSupport + '">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >MQTT</span><input class="form-control" style="width: 20px; height: 20px; margin-right: 10px;" ' + checkedMqttSupport + ' type="checkbox" id="mqttSupport_' + obj.id + '" value="' + obj.mqttSupport + '">' +
        '                                       <span style="padding: 6px; color: #4ca2cd; font-weight: 600" >CloudIO</span><input class="form-control" style="width: 20px; height: 20px;" ' + checkedCloudIOSupport + ' type="checkbox" id="cloudIOSupport_' + obj.id + '" value="' + obj.cloudIOSupport + '">' +
        '                               </td>' +
        '                            </tr>' +
        '                            </tbody>' +
        '                        </table>' +
        '                        <div class="box-footer save">' +
        '                            <button onclick="removeSwitch(\'' + obj.id + '\')" style="font-size: 12px" class="btn btn-danger"><span' +
        '                                class="lang-remove">Remover</span></button>' +
        '                            <button  onclick="saveSwitch(\'' + obj.id + '\')" style="font-size: 12px" class="btn btn-primary"><span' +
        '                                class="lang-save">Guardar</span></button>' +
        '                        </div>' +
        '                    </div>' +
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

    loadsLanguage(localStorage.getItem('lang'));
}

function stateSwitch(id, state) {
    let toggleState = state;
    if ((toggleState === "ON" || toggleState === "OFF") && ($("#btn_on_" + id).hasClass("ON") || $("#btn_on_" + id).hasClass("OFF"))) {
        toggleState = $("#btn_on_" + id).hasClass("ON") ? "OFF" : "ON";
    }
    const targetUrl = endpoint.baseUrl + "/state-switch?state=" + toggleState + "&id=" + id;
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
    const targetUrl = endpoint.baseUrl + "/rotate-state-switch?id=" + id;
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
    const targetUrl = endpoint.baseUrl + "/save-sensor?id=" + device.id;
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
        "knxSupport": document.getElementById('knxSupport_' + id).checked,
        "mqttSupport": document.getElementById('mqttSupport_' + id).checked,
        "inverted": false,
        "primaryGpioControl": parseInt($('#primaryGpioControl_' + id).val()),
        "secondaryGpioControl": parseInt($('#secondaryGpioControl_' + id).val()),
    };
    const targetUrl = endpoint.baseUrl + "/save-switch?id=" + device.id;
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


function storeConfig() {
    const targetUrl = endpoint.baseUrl + "/save-config";
    $.ajax({
        type: "POST",
        url: targetUrl,
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(config),
        success: function (response) {
            config = response;
            fillConfig();
            showMessage("Configuração Guardada", "Config Stored")
        },
        error: function () {
            showMessage("Não foi possivel guardar a configuração atual, por favor tenta novamente.", "Unable to save current configuration, please try again.")
        },
        timeout: 2000
    });
}

function saveNode() {
    config.nodeId = $('#nodeId').val().trim();
    storeConfig();
}

function saveWifi() {
    config.wifiSSID = $('#ssid').val().trim();
    config.wifiSecret = $('#wifi_secret').val().trim();
    config.wifiIp = $('#wifiIp').val().trim();
    config.wifiMask = $('#wifiMask').val().trim();
    config.wifiGw = $('#wifiGw').val().trim();
    config.staticIp = !document.getElementById("staticIp").checked;
    config.apSecret = $('#apSecret').val().trim();
    storeConfig();
}

function saveMqtt() {
    config.mqttIpDns = $('#mqtt_ip').val().trim();
    config.mqttUsername = $('#mqtt_username').val().trim();
    config.mqttPassword = $('#mqtt_password').val().trim();
    storeConfig();
}

function saveKnx() {
    config.knxArea = parseInt($('#knxArea').val().trim());
    config.knxLine = parseInt($('#knxLine').val().trim());
    config.knxMember = parseInt($('#knxMember').val().trim());
    storeConfig();
}

function saveEmoncms() {
    config.emoncmsServer = $('#emoncmsServer').val().trim();
    config.emoncmsPath = $('#emoncmsPath').val().trim();
    config.emoncmsApikey = $('#emoncmsApikey').val().trim();
    storeConfig();
}

function removeSwitch(id) {
    const targetUrl = endpoint.baseUrl + "/remove-switch?id=" + id;
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
    const targetUrl = endpoint.baseUrl + "/remove-sensor?id=" + id;
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


function reboot() {
    $.ajax({
        url: endpoint.baseUrl + "/reboot",
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            showMessage("O dispositivo está a reiniciar, ficará disponivel dentro de 10 segundos.", "The device is restartin, will be available in 10 seconds.")
        }, error: function () {
            showMessage("Não foi possivel reiniciar o dispositivo, verifica se está correctamente ligado à rede. Se o problema persistir tenta desligar da energia e voltar a ligar.", "Unable to restart the device, check if it is connected to the correct network. If the problem persists try turning the power off.")
        },
        timeout: 2000
    });
}

function loadDefaults() {
    $.ajax({
        url: endpoint.baseUrl + "/load-defaults",
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            showMessage("Configuração de fábrica aplicada com sucesso. Por favor volte a ligar-se ao Access Point e aceda ao painel de controlo pelo endereço http://192.168.4.1 no seu browser.", "Factory setting applied successfully. Please reconnect to Access Point and access the control panel at http://192.168.4.1 in your browser.")
        }, error: function () {
            showMessage("Não foi possivel carregar a configuração de fábrica no dispositivo, verifica se está correctamente ligado à rede. Se o problema persistir tenta desligar da energia e voltar a ligar.", "Unable to load factory configuration on the device, check if it is connected to the correct network. If the problem persists try turning the power off.")
        },
        timeout: 1000
    });
}

function systemStatus() {
    $.ajax({
        url: endpoint.baseUrl + "/system-status",
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            $('#ssid_lbl').text(response.wifiSSID);
            if (document.getElementById("wifiIp") && ($('input[name="wifiIp"]').val().trim().length === 0 || $('input[name="wifiIp"]').val() === "(IP unset)")) {
                $('input[name="wifiIp"]').val(response.wifiIp);
                $('input[name="wifiMask"]').val(response.wifiMask);
                $('input[name="wifiGw"]').val(response.wifiGw);
            }
            let percentage = Math.min(2 * (parseInt(response.signal) + 100), 100);
            if (config) {
                $('#mqtt_lbl').text(config.mqttIpDns);
            }
            if (response.mqtt) {
                $('#mqtt-state').text(showText("ligado", "connected"));
            } else {
                $('#mqtt-state').text(showText("desligado", "disconnected"));
            }
            $('#wifi-signal').text(percentage + "%");
        }, error: function () {
            $('#wifi-signal').text("0%");
        },
        timeout: 2000
    });
}

$(document).ready(function () {
    let lang = localStorage.getItem('lang');
    if (lang) {
        loadsLanguage(lang);
    } else {
        window.navigator.language.startsWith("en") ? loadsLanguage("EN") : loadsLanguage("PT");
    }
    loadConfig();
    $('#node_id').on('keypress', function (e) {
        if (e.which === 32)
            return false;
    });
    $('.menu-item').click(function (e) {
        let menu = $(e.currentTarget).data('menu');
        toggleActive(menu);

    });
    systemStatus();
    toggleActive("node");
    setInterval(systemStatus, 15000);
    if (!!window.EventSource) {
        source = new EventSource(endpoint.baseUrl + '/events');
    }
});