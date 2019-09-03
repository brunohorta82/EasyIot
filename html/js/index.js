const endpoint = {
    baseUrl: "http://192.168.1.142"
};
var switches = [];
var sensors = [];
let sortByProperty = function (property) {
    return function (x, y) {
        return ((x[property] === y[property]) ? 0 : ((x[property] > y[property]) ? 1 : -1));
    };
};

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

function buildSensorTemplate() {
    if ($('#bss_NEW').length > 0) return
    let device = {
        "id": "NEW",
        "name": "Novo Sensor",
        "primaryGpio": 99,
        "type": 65,
        "mqttRetain": true,
        "haSupport": true,
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
    "node": "NODE",
    "reading-interval": "Readings every",
    "sensors": "Sensors",
    "integrations": "INTEGRATIONS",
    "update": "UPDATE",
    "features": "FEATURES",
    "memory-free": "Free RAM",
    "current-version": "Current version",
    "new-file": "Choose new version file",
    "install-new-version": "Install new version",
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
    "lock": "Lock",
    "released": "Released",
    "disconnected": "disconnected",
    "dconnected": "connected",
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
    "pin-out-1": "Output Pin 1",
    "pin-out-2": "Output Pin 2",
    "none": "None",
    "retain-message": "Retain Messages",
    "command": "Command",
    "state": "State",
    "on": "On",
    "off": "Off",
    "locked": "Lock",
    "unlock": "Unlocked",
    "open": "Open",
    "close": "Closed",
    "mqtt":"MQTT",
    "stop": "Stop",
    "time": "Time",
    "17": "A0"

};
var WORDS_PT = {
    "node": "NÓ",
    "integrations": "INTEGRAÇÕES",
    "sensors": "Sensores",
    "released": "Libertar",
    "reading-interval": "Leituras a cada",
    "update": "ATUALIZAR",
    "features": "FUNÇÕES",
    "current-version": "Versão atual",
    "new-file": "Escolher o ficheiro da nova versão",
    "install-new-version": "Instalar nova versão",
    "version": "Versão",
    "save": "Guardar",
    "memory-free": "RAM Livre",
    "choose": "escolher",
    "clean-fields": "Limpar todos os campos",
    "username": "Utilizador",
    "password": "Palavra Passe",
    "yes": "Sim",
    "no": "Não",
    "disconnected": "desligado",
    "dconnected": "ligado",
    "netmask": "Mascara de Rede",
    "system": "Sistema",
    "name": "Nome",
    "in": "em",
    "seconds": "segundos",
    "restart": "Reiniciar",
    "reset-factory": "Carregar Configuração de Fábrica",
    "switches": "Interruptores",
    "remove": "Remover",
    "new": "Criar Novo",
    "family": "Familia",
    "switch": "Interruptor",
    "light": "Luz",
    "cover": "Estore",
    "lock": "Fechadura",
    "normal": "Normal",
    "push": "Pressão",
    "dual-push": "Duplo Normal",
    "dual-normal": "Duplo Pressão",
    "mode": "Mode",
    "relay-mqtt": "Relé / MQTT",
    "mqtt":"MQTT",
    "control": "Saída",
    "pin-in-a": "Pino Entrada a",
    "pin-in-b": "Pino Entrada b",
    "pin-out-1": "Pino Saída 1",
    "pin-out-2": "Pino Saída 2",
    "none": "Não atribuido",
    "retain-message": "Reter Mensagens",
    "command": "Comando",
    "state": "Estado",
    "on": "Ligado",
    "off": "Desligado",
    "locked": "Trancado",
    "unlock": "Destrancado",
    "open": "Aberto",
    "close": "Fechado",
    "stop": "Parar",
    "time": "Tempo",
    "17": "A0"
};

function loadsLanguage(lang) {
    if(lang === null){
        window.navigator.language.startsWith("en") ?   lang = "EN" :  lang = "PT";
    }
    localStorage.setItem('lang', lang);
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
        contentType: "text/plain; charset=utf-8",
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
    $("#firmwareVersion").text(config.firmware);
    $(".bh-model").text(config.hardware);
    $(".bh-onofre-item").removeClass("hide");
    $("#version_lbl").text(config.firmware);
    $("#lbl-chip").text(config.chipId);
    $("#lbl-mac").text(config.mac);
    $('input[name="nodeId"]').val(config.nodeId);
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
            switches = [];
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
    switches = payload;
    if (!payload) return;
    $('#switch_config').empty();
    for (let obj of payload.sort(sortByProperty('name'))) {
        buildSwitch(obj);
    }
}

function fillSensors(payload) {
    sensors = payload;
    if (!payload) return;
    $('#sensors_config').empty();
    for (let obj of payload.sort(sortByProperty('name'))) {
        buildSensor(obj);
    }
}

function applySwitchFamily(id) {
    hide("mqttPositionCommandTopicRow_" + id);
    hide("mqttPositionStateTopicRow_" + id);
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
    removeFromSelect('autoStateValue_' + id, "OPEN");
    removeFromSelect('autoStateValue_' + id, "CLOSE");
    removeFromSelect('autoStateValue_' + id, "STOP");
    removeFromSelect('autoStateValue_' + id, "ON");
    removeFromSelect('autoStateValue_' + id, "OFF");
    removeFromSelect('autoStateValue_' + id, "UNLOCK");
    removeFromSelect('autoStateValue_' + id, "LOCK");
    if ($('#family_' + id).val() == "cover") {
        show("btn_close_" + id);
        show("btn_stop_" + id);
        show("btn_open_" + id);
        addToSelect('mode_' + id, "lang-push", 2);
        addToSelect('mode_' + id, "lang-dual-normal", 4);
        addToSelect('mode_' + id, "lang-dual-push", 5);
        show("secondaryGpioControlRow_" + id);
        show("secondaryGpioRow_" + id);
        show("mqttPositionCommandTopicRow_" + id);
        show("mqttPositionStateTopicRow_" + id);
        show("timeBetweenStatesRow_" + id);
        addToSelect('autoStateValue_' + id, "lang-open", "OPEN");
        addToSelect('autoStateValue_' + id, "lang-close", "CLOSE");
        addToSelect('autoStateValue_' + id, "lang-stop", "STOP");
    } else if ($('#family_' + id).val() == "lock") {
        show("btn_on_" + id);
        addToSelect('mode_' + id, "lang-push", 2);
        addToSelect('autoStateValue_' + id, "lang-released", "RELEASED");
    } else {
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
    var gpios = ["17", "0", "1", "2", "3", "4", "5", "12", "13", "14", "15"];
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

}

function applySensorRequiredGpio(id) {
    if ($('#s_type_' + id).val() != "70") {
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
    $('#sensors_config').append('<div id="bss_' + obj.id + '" style="padding: 0; margin: 10px;" class="col-lg-4 col-md-6 col-xs-12">' +
        '                <div style="margin-bottom: 0" class="info-box bg-aqua">' +
        '                    <div class="info-box-content"><span class="info-box-text">' + obj.name + '</span>' +
        '                        <div class="pull-right">' +
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
        '                                    <option value="56">REED SWITCH</option>' +
        '                                    <option value="0">DHT 11</option>' +
        '                                    <option value="2">DHT 21</option>' +
        '                                    <option value="3">DHT 22</option>' +
        '                                    <option value="70">PZEM 004T V2</option>' +
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
        '                                    class="lang-pin-in-a">Pinos Entrada A</span></span></td>' +
        '                                <td><select class="form-control select-device" id="s_primaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            <tr>' +
        '                            <tr id="s_secondaryGpioRow_' + obj.id + '"">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-in-b">Pinos Entrada B</span></span></td>' +
        '                                <td><select class="form-control select-device" id="s_secondaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            <tr>' +
        '                            <tr id="s_tertiaryGpioRow_' + obj.id + '"">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-in-c">Pinos Entrada C</span></span></td>' +
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
        '                                <td style="display: inline-flex">' + '' +
        '<span style="padding: 6px; color: #4ca2cd; font-weight: 600" >Home Assistant</span><input class="form-control" style="width: 20px; height: 20px; margin-right: 10px;" ' + checkedHaSupport + ' type="checkbox" id="s_haSupport' + obj.id + '" value="' + obj.haSupport + '">' +
        '<span style="padding: 6px; color: #4ca2cd; font-weight: 600" >Emoncms</span><input class="form-control" style="width: 20px; height: 20px;" ' + checkedEmoncmsSupport + ' type="checkbox" id="s_emoncmsSupport' + obj.id + '" value="' + obj.emoncmsSupport + '"></td>' +
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

function updateSensorReadings(json) {
    Object.keys(json).forEach(function (key) {
        if ($('#' + key).length > 0) {
            $("#" + key + "_value").text(json[key]);
        } else {
            $('#readings').append('  <div id="' + key + '" style="margin-right: 10px; margin-left: 10px">' +
                '                <div' +
                '                     style="width: 50px; height: 50px; padding-top:13px; margin-left: 10px; margin-right: 10px; border-radius: 50px; border: solid 2px white;">' +
                '                    <span id="' + key + '_value">' + json[key] + '</span>' +
                '                </div>' +
                '                <div' +
                '                    style="margin-top: -5px;border-radius: 10px; font-size: 10px;height: 15px;background-color: #86bd9a">' +
                '                    <span >' + key + '</span>' +
                '                </div>' +
                '            </div>')
        }
    });

}

function buildSwitch(obj) {
    let on = obj.stateControl === 'ON' ? " " + obj.stateControl + " " : "OFF";
    let open = obj.stateControl === 'OPEN' ? " " + obj.stateControl + " " : "";
    let close = obj.stateControl === 'CLOSE' ? " " + obj.stateControl + " " : "";
    let checkedMqttRetain = obj.mqttRetain ? "checked" : "";
    let checkedHaSupport = obj.haSupport ? "checked" : "";
    let checkedAlexaSupport = obj.alexaSupport ? "checked" : "";

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
        '                            <button onclick="stateSwitch(\'' + obj.id + '\',\'' + obj.stateControl + '\')" id="btn_on_' + obj.id + '" class="' + on + ' btn btn-primary btn-control">' +
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
        '                                    <option class="lang-lock" value="lock">Fechadura</option>' +
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
        '                                    class="lang-pin-in-a">Pinos Entrada A</span></span></td>' +
        '                                <td><select class="form-control select-device" id="primaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="secondaryGpioRow_' + obj.id + '"">' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-pin-in-b">Pinos Entrada B</span></span></td>' +
        '                                <td><select class="form-control select-device" id="secondaryGpio_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr id="primaryGpioControlRow_' + obj.id + '" style="  border-top: 1px solid #d9534f;">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-out-1">Pinos Saida 1</span></span></td>' +
        '                                <td><select class="form-control select-device" id="primaryGpioControl_' + obj.id + '">' +
        '                            </tr>' +
        '                            <tr id="secondaryGpioControlRow_' + obj.id + '" ">' +
        '                                <td><span class="label-device "><span' +
        '                                    class="lang-pin-out-2">Pinos Saida 2</span></span></td>' +
        '                                <td><select class="form-control select-device" id="secondaryGpioControl_' + obj.id + '">' +
        '                                </select></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device" style="color: #88bf9c; font-size: 13px;">MQTT</span></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device-indent"><span class="lang-command">Comando</span></span></td>' +
        '                                <td> <span style="word-break: break-word" >' + obj.mqttCommandTopic + '</span></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device-indent"><span class="lang-state">Estado</span></span></td>' +
        '                                <td><span  style="word-break: break-word">' + obj.mqttStateTopic + '</span></td>' +
        '                            </tr>' +
        '                            <tr id="mqttPositionCommandTopicRow_' + obj.id + '" ">' +
        '                                <td><span class="label-device-indent"><span class="lang-command">Comando</span></span></td>' +
        '                                <td><span  style="word-break: break-word" >' + ifdef(obj.mqttPositionCommandTopic, "../setposition") + '</span></td>' +
        '                            </tr>' +
        '                            <tr id="mqttPositionStateTopicRow_' + obj.id + '" ">' +
        '                                <td><span class="label-device-indent"><span class="lang-state">Estado</span></span></td>' +
        '                                <td><span  style="word-break: break-word" >' + ifdef(obj.mqttPositionStateTopic, "../position") + '</span></td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td style="vertical-align: middle"><span class="label-device-indent label-device"><span class="lang-retain-message">Reter Mensagens</span></span></td>' +
        '                                <td><input class="form-control" style="width: 20px; height: 20px;" ' + checkedMqttRetain + ' type="checkbox" id="mqttRetain_' + obj.id + '" value="' + obj.mqttRetain + '"></td>' +
        '                            </tr>' +
        '                            <tr id="timeBetweenStatesRow_' + obj.id + '" >' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-time">TEMPO</span></span></td>' +
        '                                <td class="col-xs-8"><input class="input-device form-control" value="' + obj.timeBetweenStates / 1000 + '"' +
        '                                                            type="text" id="timeBetweenStates_' + obj.id + '" placeholder="ex: 12"' +
        '                                                             maxlength="2" required/>' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td><span class="label-device"><span' +
        '                                    class="lang-auto-state">Estádo automático</span></span></td>' +
        '                                <td class="col-xs-8"><span style="float: left" class="lang-in">em</span><input style="width: 50px; float: left; margin-left: 5px;" class="input-device form-control" value="' + (obj.autoStateDelay / 1000) + '"' +
        '                                                            type="text" id="autoStateDelay_' + obj.id + '" placeholder="ex: 12"' +
        '                                                             maxlength="4" required/><span style="float: left; margin-left: 5px;" class="lang-seconds">segundos</span> ' +
        '                                <select class="form-control select-device" style="float: left; width: 150px; margin-left: 5px;" id="autoStateValue_' + obj.id + '">' +
        '                                    <option class="lang-choose" value="">Escolha</option>' +

        '                                </select>' +
        '                                </td>' +
        '                            </tr>' +
        '                            <tr>' +
        '                                <td style="vertical-align: middle"><span class="label-device"><span class="lang-integrate">Integrar</span></span></td>' +
        '                                <td style="display: inline-flex">' + '' +
        '<span style="padding: 6px; color: #4ca2cd; font-weight: 600" >Home Assistant</span><input class="form-control" style="width: 20px; height: 20px; margin-right: 10px;" ' + checkedHaSupport + ' type="checkbox" id="haSupport' + obj.id + '" value="' + obj.haSupport + '">' +
        '<span style="padding: 6px; color: #4ca2cd; font-weight: 600" >Alexa</span><input class="form-control" style="width: 20px; height: 20px;" ' + checkedAlexaSupport + ' type="checkbox" id="alexaSupport' + obj.id + '" value="' + obj.alexaSupport + '"></td>' +
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
    fillGpioSelect('secondaryGpioControl_' + obj.id);
    fillGpioSelect('primaryGpioControl_' + obj.id);
    fillGpioSelect('secondaryGpioControl_' + obj.id);
    setOptionOnSelect('family_' + obj.id, obj.family);
    applySwitchFamily(obj.id);
    setOptionOnSelect('typeControl_' + obj.id, obj.typeControl);
    setOptionOnSelect('mode_' + obj.id, obj.mode);
    applyTypeControl(obj.id);
    setOptionOnSelect('primaryGpioControl_' + obj.id, obj.primaryGpioControl);
    setOptionOnSelect('secondaryGpioControl_' + obj.id, obj.secondaryGpioControl);
    setOptionOnSelect('primaryGpio_' + obj.id, obj.primaryGpio);
    setOptionOnSelect('secondaryGpio_' + obj.id, obj.secondaryGpio);
    setOptionOnSelect('autoStateValue_' + obj.id, obj.autoStateValue);

    loadsLanguage(localStorage.getItem('lang'));
}

function stateSwitch(id, state) {
    let toggleState = state;
    if ((toggleState === "ON" || toggleState === "OFF") && ($("#btn_on_" + id).hasClass("ON") || $("#btn_on_" + id).hasClass("OFF"))) {
        toggleState = $("#btn_on_" + id).hasClass("ON") ? "OFF" : "ON";
    }
    if (toggleState === "RELEASED") {
        toggleState = "LOCK";
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

function saveSensor(id) {
    let device = {
        "id": id,
        "name": $('#s_name_' + id).val(),
        "primaryGpio": parseInt($('#s_primaryGpio_' + id).val()),
        "secondaryGpio": parseInt($('#s_secondaryGpio_' + id).val()),
        "tertiaryGpio": parseInt($('#s_tertiaryGpio_' + id).val()),
        "type": parseInt($('#s_type_' + id).val()),
        "mqttRetain": document.getElementById('s_mqttRetain_' + id).checked,
        "haSupport": document.getElementById('s_haSupport' + id).checked,
        "emoncmsSupport": document.getElementById('s_emoncmsSupport' + id).checked,
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
            let idx = sensors.findIndex(function (item, i) {
                return item.id === device.id
            });
            if (idx >= 0) {
                sensors.splice(idx, 1);
            }
            sensors.push(response);
            fillSensors(sensors);
            showMessage("Configuração Guardada", "Config Stored")

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
        "timeBetweenStates": parseInt($('#timeBetweenStates_' + id).val()) * 1000,
        "autoStateValue": $('#autoStateValue_' + id).val(),
        "autoStateDelay": parseInt($('#autoStateDelay_' + id).val()) * 1000,
        "typeControl": parseInt($('#typeControl_' + id).val()),
        "mode": parseInt($('#mode_' + id).val()),
        "pullup": true,
        "mqttRetain": document.getElementById('mqttRetain_' + id).checked,
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
            let idx = switches.findIndex(function (item, i) {
                return item.id === device.id
            });
            if (idx >= 0) {
                switches.splice(idx, 1);
            }
            switches.push(response);
            fillSwitches(switches);
            showMessage("Configuração Guardada", "Config Stored")

        },
        error: function () {
            showMessage("Não foi possivel guardar a configuração atual, por favor tenta novamente.", "Unable to save current configuration, please try again.")
        }, complete: function () {

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
        let source = new EventSource(endpoint.baseUrl + '/events');
        source.addEventListener('states', function (e) {
            let json = JSON.parse(e.data);
            let state = json.state == "OFF" ? "on" : json.state.toLowerCase();
            $("#btn_" + state + "_" + json.id).removeClass("ON");
            $("#btn_" + state + "_" + json.id).removeClass("OPEN");
            $("#btn_" + state + "_" + json.id).removeClass("CLOSE");
            $("#btn_" + state + "_" + json.id).addClass(json.state);
        }, false);
        source.addEventListener('sensors', function (e) {
            let json = JSON.parse(e.data);
            updateSensorReadings(json);
        }, false);

    }

});

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
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            switches.splice(switches.findIndex(function (item, i) {
                return item.id === id
            }), 1);
            fillSwitches(switches);
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
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            sensors.splice(sensors.findIndex(function (item, i) {
                return item.id === id
            }), 1);
            fillSensors(sensors);
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
            showMessage("Configuração de fábrica aplicada com sucesso. Por favor volte a ligar-se ao Access Point e aceda ao painel de controlo pelo endereço http://192.168.4.1 no seu browser.", "Configuração de fábrica aplicada com sucesso. Por favor volte a ligar-se ao Access Point e aceda ao painel de controlo pelo endereço http://192.168.4.1 no seu browser.")
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
            if (response.mqttConnected) {
                $('#mqtt-state').text(showText("ligado", "connected"));
            } else {
                $('#mqtt-state').text(showText("desligado", "disconnected"));
            }
            $('#lbl-heap').text((parseFloat(response.freeHeap / 1024).toFixed(2)).toString().concat(" KiB"));
            $('#wifi-signal').text(percentage + "%");
        }, error: function () {
            $('#wifi-signal').text("0%");
        },
        timeout: 1000
    });
}

function buildSwitchTemplate() {
    if ($('#bs_NEW').length > 0) return
    let device = {
        "id": "NEW",
        "name": "Novo Interruptor",
        "family": "switch",
        "primaryGpio": 99,
        "secondaryGpio": 99,
        "timeBetweenStates": 0,
        "autoStateDelay": 0,
        "autoStateValue": "",
        "typeControl": 2,
        "mode": 1,
        "pullup": false,
        "mqttRetain": false,
        "inverted": false,
        "haSupport": true,
        "alexaSupport": true,
        "mqttCommandTopic": "../set",
        "mqttStateTopic": "../state",
        "mqttPositionCommandTopic": "../setposition",
        "mqttPositionStateTopic": "../position",
        "primaryGpioControl": 99,
        "secondaryGpioControl": 99,
    };
    buildSwitch(device);
}