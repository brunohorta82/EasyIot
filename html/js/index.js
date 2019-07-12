const endpoint = {
    baseUrl: "http://192.168.187.168"
};
let sortByProperty = function (property) {
    return function (x, y) {
        return ((x[property] === y[property]) ? 0 : ((x[property] > y[property]) ? 1 : -1));
    };
};
var config;
var WORDS_EN = {
    "node": "NODE",
    "update": "UPDATE",
    "features": "FEATURES",
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
    "reset-factory": "Load Factory Defaults",
    "switches": "Switches",
    "remove": "Remove",
    "new": "New",
    "family": "Family",
    "switch":"Switch",
    "light":"Light",
    "cover":"Cover",
    "lock":"Lock"

};
var WORDS_PT = {
    "node": "NÓ",
    "update": "ATUALIZAR",
    "features": "FUNÇÕES",
    "current-version": "Versão atual",
    "new-file": "Escolher o ficheiro da nova versão",
    "install-new-version": "Instalar nova versão",
    "version": "Versão",
    "save": "Guardar",
    "clean-fields": "Limpar todos os campos",
    "username": "Utilizador",
    "password": "Palavra Passe",
    "yes": "Sim",
    "no": "Não",
    "netmask": "Mascara de Rede",
    "system": "Sistema",
    "name": "Nome",
    "restart": "Reiniciar",
    "reset-factory": "Carregar Configuração de Fábrica",
    "switches": "Interruptores",
    "remove": "Remover",
    "new": "Criar Novo",
    "family": "Familia",
    "switch":"Interruptor",
    "light":"Luz",
    "cover":"Estore",
    "lock":"Fechadura"

};

function loadsLanguage(lang) {
    localStorage.setItem('lang', lang);
    $('span[class^="lang"]').each(function () {
        var langVar = (this.className).replace('lang-', '');
        var text = window["WORDS_" + lang][langVar];
        $(this).text(text);
    });
    $('option[class^="lang"]').each(function () {
        var langVar = (this.className).replace('lang-', '');
        var text = window["WORDS_" + lang][langVar];
        $(this).text(text);
    });
}

function showMessage(pt, en) {
    localStorage.getItem('lang').toString() === "PT" ? alert(pt) : alert(en);
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
    if (!this.config) return;
    $("#firmwareVersion").text(this.config.firmware);
    $(".bh-model").text(this.config.hardware);
    $(".bh-onofre-item").removeClass("hide");
    $("#version_lbl").text(this.config.firmware);
    $('input[name="nodeId"]').val(this.config.nodeId);
    $('input[name="mqttIpDns"]').val(this.config.mqttIpDns);
    $('input[name="mqttUsername"]').val(this.config.mqttUsername);
    $('select[name="homeAssistantAutoDiscovery"] option[value="' + this.config.homeAssistantAutoDiscovery + '"]').attr("selected", "selected");
    $('input[name="homeAssistantAutoDiscoveryPrefix"]').val(this.config.homeAssistantAutoDiscoveryPrefix);
    $('input[name="mqttPassword"]').val(this.config.mqttPassword);
    $('input[name="wifiSSID"]').val(this.config.wifiSSID);
    $('input[name="wifiSecret"]').val(this.config.wifiSecret);
    let staticIp = document.getElementById("staticIp");
    if (staticIp) {
        staticIp.checked = !config.staticIp;
    }
    $('input[name="wifiIp"]').val(this.config.wifiIp);
    $('input[name="wifiMask"]').val(this.config.wifiMask);
    $('input[name="wifiGw"]').val(this.config.wifiGw);
    $('input[name="apSecret"]').val(this.config.apSecret);
    $('select[name="notificationInterval"] option[value="' + this.config.notificationInterval + '"]').attr("selected", "selected");
    $('select[name="directionCurrentDetection"] option[value="' + this.config.directionCurrentDetection + '"]').attr("selected", "selected");
    $('#ff').prop('disabled', false);
}

function toggleActive(menu) {
    $('.sidebar-menu').find('li').removeClass('active');
    $('.menu-item[data-menu="' + menu + '"]').closest('li').addClass('active');
    $(".content").load(menu + ".html", function () {
        if (menu === "devices") {
            loadDevice(fillSwitches, "switches", function () {
                // loadDevice(fillSensors, "sensors", function () {
                // });

            });
        } else {
            wifiStatus();
            fillConfig();
        }
        loadsLanguage(localStorage.getItem('lang'));
    });
}

function fillSwitches(payload) {
    if (!payload) return;
    $('#switch_config').empty();
    for (let obj of payload.sort(sortByProperty('name'))) {
        buildSwitch(obj);
    }

}

function btnSwitch(e) {
    stateSwitch($(e).attr("data-id"), $(e).text() === "ON" ? "OFF" : "ON");
}

function buildSwitch(obj) {
    $('#switch_config').append("<div id=\"bs_" + obj.id + "\" class=\"col-lg-4 col-md-6 col-xs-12\">" +
        "        <div style=\"margin-bottom: 0\" class=\"info-box bg-aqua\">" +

        "            <div class=\"info-box-content\"><span class=\"info-box-text\">" + obj.name + "</span>" +
        "<div  id=\"on_off_control_" + obj.id + "\" class=\"" + ((obj.mode === 4 || obj.mode === 5) ? 'hide' : '') + "\">" +
        "                <button onclick='btnSwitch(this)' id=\"btn_" + obj.id + "\" data-id=\"" + obj.id + "\" style=\"float: right\" class=\"btn btn-primary\" >" + (obj.stateControl ? 'ON' : 'OFF') + "</button>" +
        "</div><div  id=\"open_close_control_" + obj.id + "\" class=\"" + ((obj.mode === 4 || obj.mode === 5) ? '' : 'hide') + "\">" +
        "<button onclick='stateSwitch(\"" + obj.id + "\",\"OPEN\");' id=\"btn_up_" + obj.id + "\" style=\"float: right\" class=\"btn btn-primary\" >ABRIR</button><button onclick='stateSwitch(\"" + obj.id + "\",\"STOP\");' id=\"btn_stop_" + obj.id + "\" style=\"float: right\" class=\"btn btn-primary\" >PARAR</button><button onclick='stateSwitch(\"" + obj.id + "\",\"CLOSE\");' id=\"btn_down_" + obj.id + "\" style=\"float: right\" class=\"btn btn-primary\" >FECHAR</button>" +
        "            </div></div>" +
        "        </div>" +
        "        <div style=\"font-size: 10px;  border: 0px solid #08c; border-radius: 0\" class=\"box\">" +
        "            <div class=\"box-body no-padding\">" +
        "                <table class=\"table table-condensed\">" +
        "                    <tbody>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device lang-name\">NOME</span></td>" +
        "                        <td><input  style=\"font-size: 10px; height: 20px;\"  class=\"form-control\" value=\"" + obj.name + "\" type=\"text\"  id=\"name_" + obj.id + "\" placeholder=\"ex: luz sala\"  required=\"true\"/></td>" +
        "                    </tr>" +
        "                   <tr  id=\"tipo_" + obj.id + "\" class=\"" + (obj.mode === 4 ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device lang-type\">TIPO</span></td>" +
        "                        <td><select onchange=\"switchTypeOptions('" + obj.id + "')\" class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"type_" + obj.id + "\">" +
        "                            <option " + (obj.family === 'switch' ? 'selected' : '') + " value=\"switch\">Interruptor</option>" +
        "                            <option " + (obj.family === 'light' ? 'selected' : '') + " value=\"light\">Luz</option>" +
        "                            <option " + (obj.family === 'cover' ? 'selected' : '') + " value=\"cover\">Estore</option>" +
        "                            <option " + (obj.family === 'lock' ? 'selected' : '') + " value=\"lock\">Fechadura</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device lang-mode\">MODO</span></td>" +
        "                   <td   id=\"mode_" + obj.id + "\" class=\"generic_type_" + obj.id + " " + (obj.family !== 'cover' ? '' : 'hide') + "\">" +
        "                            <select onchange=\"switchModeRules('" + obj.id + "')\" class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"modeg_" + obj.id + "\">" +
        "                                <option " + (obj.mode === 1 ? 'selected' : '') + " value=\"1\">ON | OFF NORMAL</option>" +
        "                                <option " + (obj.mode === 2 ? 'selected' : '') + " value=\"2\">ON | OFF PUSH/TOUCH</option>" +
        "                                <option " + (obj.mode === 6 ? 'selected' : '') + " value=\"6\">ON | AUTO OFF PUSH/TOUCH</option>" +
        "                            </select>" +
        "                        </td>" +
        "                   <td   id=\"mode_" + obj.id + "\" class=\"cover_type_" + obj.id + " " + (obj.family === 'cover' ? '' : 'hide') + "\">" +
        "                            <select onchange=\"switchModeRules('" + obj.id + "')\" class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"modec_" + obj.id + "\">" +
        "                                <option " + (obj.mode === 4 ? 'selected' : '') + " value=\"4\">OPEN | STOP | CLOSE NORMAL</option>" +
        "                                <option " + (obj.mode === 5 ? 'selected' : '') + " value=\"5\">OPEN | STOP | CLOSE PUSH/TOUCH</option>" +
        "                            </select>" +
        "                        </td>" +
        "                    </tr>" +
        "                   <tr  id=\"single_gpio_" + obj.id + "\" class=\"" + (obj.mode === 4 ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">GPIO</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"gpio_" + obj.id + "\">" +
        "                            <option " + (obj.primaryGpio === 99 ? 'selected' : '') + " value=\"99\">NENHUM</option>" +
        "                            <option " + (obj.primaryGpio === 0 ? 'selected' : '') + " value=\"0\">0</option>" +
        "                            <option " + (obj.primaryGpio === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.primaryGpio === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.primaryGpio === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.primaryGpio === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                   <tr  id=\"open_gpio_" + obj.id + "\" class=\"" + (obj.mode !== 4 ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">GPIO OPEN</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"gpio_open_" + obj.id + "\">" +
        "                            <option " + (obj.primaryGpio === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.primaryGpio === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.primaryGpio === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.primaryGpio === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                   <tr  id=\"close_gpio_" + obj.id + "\" class=\"" + (obj.mode !== 4 ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">GPIO CLOSE</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"gpio_close_" + obj.id + "\">" +
        "                            <option " + (obj.secondaryGpio === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.secondaryGpio === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.secondaryGpio === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.secondaryGpio === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +

        "                   <tr  id=\"relay_open_gpio_" + obj.id + "\" class=\"" + ((obj.mode === 4 || obj.mode === 5) ? '' : 'hide') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">RELÉ OPEN CLOSE</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"relay_open_" + obj.id + "\">" +
        "                            <option " + (obj.gpioOpenCloseControl === 4 ? 'selected' : '') + " value=\"4\">4</option>" +
        "                            <option " + (obj.gpioOpenCloseControl === 5 ? 'selected' : '') + " value=\"5\">5</option>" +
        "                            <option " + (obj.gpioOpenCloseControl === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpioOpenCloseControl === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpioOpenCloseControl === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpioOpenCloseControl === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                   <tr  id=\"relay_close_gpio_" + obj.id + "\" class=\"" + ((obj.mode === 4 || obj.mode === 5) ? '' : 'hide') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">RELÉ STOP</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"relay_close_" + obj.id + "\">" +
        "                            <option " + (obj.gpioStopControl === 4 ? 'selected' : '') + " value=\"4\">4</option>" +
        "                            <option " + (obj.gpioStopControl === 5 ? 'selected' : '') + " value=\"5\">5</option>" +
        "                            <option " + (obj.gpioStopControl === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpioStopControl === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpioStopControl === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpioStopControl === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                   <tr  id=\"type_mode_" + obj.id + "\" class=\"" + ((obj.mode === 4 || obj.mode === 5) ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device \">COMUTA</span></td>" +
        "                        <td><div class=\"row\">" +
        "                <div class=\"col-xs-5\">" +
        "                        <select onchange=\"switchTypeRules('" + obj.id + "')\" class=\"form-control\" style=\"font-size: 10px;  padding: 0 12px; height: 20px;\"" +
        "                                 id=\"typeControl_" + obj.id + "\">" +
        "                            <option " + (obj.typeControl === 1 ? 'selected' : '') + " value=\"1\">RELÉ</option>" +
        "                            <option " + (obj.typeControl === 2 ? 'selected' : '') + " value=\"2\">MQTT</option>" +
        "                        </select>" +
        "                        </select>" +
        "                </div>" +
        "                <div  id=\"type-control-lbl" + obj.id + "\" class=\"col-xs-2 " + (obj.typeControl === 'mqtt' ? 'hide' : '') + "\">ID/GPIO</div>" +
        "                <div  id=\"type-control-box" + obj.id + "\" class=\"col-xs-5 " + (obj.typeControl === 'mqtt' ? 'hide' : '') + "\">" +
        "                           <select class=\"form-control\" style=\" font-size: 10px;padding: 0px 12px; height: 20px;\"" +
        "                                 id=\"gpioControl_" + obj.id + "\">" +
        "                            <option " + (obj.gpioSingleControl == 0 ? 'selected' : '') + " value=\"0\">0</option>" +
        "                            <option " + (obj.gpioSingleControl == 2 ? 'selected' : '') + " value=\"2\">2</option>" +
        "                            <option " + (obj.gpioSingleControl == 4 ? 'selected' : '') + " value=\"4\">4</option>" +
        "                            <option " + (obj.gpioSingleControl == 5 ? 'selected' : '') + " value=\"5\">5</option>" +
        "                            <option " + (obj.gpioSingleControl == 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpioSingleControl == 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpioSingleControl == 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpioSingleControl == 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select>" +
        "                </div>" +
        "              </div>" +


        "</td>" +
        "                    </tr>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">MQTT ESTADO</span></td>" +
        "                        <td><span style=\"font-weight: bold; font-size:11px; color: #00a65a\">" + obj.mqttStateTopic + "</span>" +
        "                        </td>" +
        "" +
        "                    </tr>" +
        "                    <tr id=\"mqtt_control" + obj.id + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">MQTT CONTROLO</span></td>" +
        "                        <td><span style=\"font-weight: bold; font-size:11px; color:#f39c12\">" + obj.mqttCommandTopic + "</span>" +
        "                        </td>" +
        "" +
        "                    </tr>" +
        "                    </tbody>" +
        "                </table>" +
        "                <div class=\"box-footer save\">" +
        "                    <button onclick=\"removeDevice('remove-switch','" + obj.id + "',fillSwitches)\" style=\"font-size: 12px\" class=\"btn btn-danger\"><span class=\"lang-remove\">Remover</span></button>" +
        "                    <button onclick=\"saveSwitch('" + obj.id + "')\" style=\"font-size: 12px\" class=\"btn btn-primary\"><span class=\"lang-save\">Guardar</span></button>" +

        "                </div>" +
        "            </div>" +
        "        </div>" +
        "" +
        "    </div>"
    );


}

function stateSwitch(id, state) {
    const targetUrl = endpoint.baseUrl + "/state-switch?state=" + state + "&id=" + id;
    $.ajax({
        type: "POST",
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

function switchTypeRules(e) {
    $('#master_' + e).parent().parent().toggleClass("hide");
    $('#type-control-lbl' + e).toggleClass("hide");
    $('#type-control-box' + e).toggleClass("hide");
}

function switchTypeOptions(e) {
    let type = $('#type_' + e).val();
    if (type !== 'cover') {
        $('.generic_type_' + e).removeClass("hide");
        $('.cover_type_' + e).addClass("hide");
    } else {
        $('.generic_type_' + e).addClass("hide");
        $('.cover_type_' + e).removeClass("hide");
    }

    switchModeRules(e);
}

function switchModeRules(e) {
    var mode = 0;
    let type = $('#type_' + e).val();

    if (type !== 'cover') {
        mode = $('#modeg_' + e).val();
    } else {
        mode = $('#modec_' + e).val();
    }
    if (mode === '4' || mode === '5') {
        $('#relay_open_gpio_' + e).removeClass("hide");
        $('#relay_close_gpio_' + e).removeClass("hide");
        $('#type_mode_' + e).addClass("hide");
        $('#on_off_control_' + e).addClass("hide");
        $('#open_close_control_' + e).removeClass("hide");
        if (mode === '4') {
            $('#single_gpio_' + e).addClass("hide");
            $('#open_gpio_' + e).removeClass("hide");
            $('#close_gpio_' + e).removeClass("hide");
        } else {
            $('#open_gpio_' + e).addClass("hide");
            $('#close_gpio_' + e).addClass("hide");
        }
    } else {
        $('#on_off_control_' + e).removeClass("hide");
        $('#open_close_control_' + e).addClass("hide");
        $('#single_gpio_' + e).removeClass("hide");
        $('#relay_open_gpio_' + e).addClass("hide");
        $('#relay_close_gpio_' + e).addClass("hide");
        $('#type_mode_' + e).removeClass("hide");
        $('#open_gpio_' + e).addClass("hide");
        $('#close_gpio_' + e).addClass("hide");
    }


}


function buildSensor(obj) {
    $('#sensor_config').append("<div id=\"sn_" + obj.id + "\" class=\"col-lg-4 col-md-6 col-xs-12\">" +
        "        <div style=\"margin-bottom: 0px\" class=\"info-box bg-aqua\">" +

        "            <div class=\"info-box-content\"><span class=\"info-box-text\">" + obj.name + "</span>" +
        "            </div>" +
        "        </div>" +
        "        <div style=\"font-size: 10px; border-radius: 0\" class=\"box\">" +
        "            <div class=\"box-body no-padding\">" +
        "                <table class=\"table table-condensed\">" +
        "                    <tbody>" +

        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">NOME</span></td>" +
        "                        <td><input  style=\"font-size: 10px; height: 20px;\"  class=\"form-control\" value=\"" + obj.name + "\" type=\"text\"  id=\"name_" + obj.id + "\" placeholder=\"ex: luz sala\"  required=\"true\"/></td>" +
        "                    </tr>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">TIPO</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"type_" + obj.id + "\">" +
        "                            <option  " + (obj.type == 0 ? 'selected' : '') + " value=\"0\">DHT 11</option>" +
        "                            <option " + (obj.type == 1 ? 'selected' : '') + " value=\"1\">DHT 21</option>" +
        "                            <option " + (obj.type == 2 ? 'selected' : '') + " value=\"2\">DHT 22</option>" +
        "                            <option " + (obj.type == 90 ? 'selected' : '') + " value=\"90\">DS18B20</option>" +
        "                            <option " + (obj.type == 65 ? 'selected' : '') + " value=\"65\">PIR</option>" +
        "                            <option " + (obj.type == 21 ? 'selected' : '') + " value=\"21\">LDR</option>" +
        "                            <option " + (obj.type == 56 ? 'selected' : '') + " value=\"56\">REED SWITCH</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">GPIO</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"gpio_" + obj.id + "\">" +
        "                            <option " + (obj.gpio == 0 ? 'selected' : '') + " value=\"0\">0</option>" +
        "                            <option " + (obj.gpio == 4 ? 'selected' : '') + " value=\"4\">4</option>" +
        "                            <option " + (obj.gpio == 5 ? 'selected' : '') + " value=\"5\">5</option>" +
        "                            <option " + (obj.gpio == 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpio == 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpio == 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpio == 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                            <option " + (obj.type == 21 ? 'selected' : '') + " value=\"A0\">A0</option>" +
        "                        </select></td>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">MQTT ESTADO</span></td>" +
        "                        <td><span style=\"font-weight: bold; font-size:11px; color: #00a65a\">" + obj.mqttStateTopic + "</span>" +
        "                        </td>" +
        "" +
        "                    </tr>" +
        "                    </tr>" + getSensorFunctions(obj) +

        "                    </tbody>" +
        "                </table>" +
        "                <div class=\"box-footer save\">" +
        "                    <button onclick=\"removeDevice('remove-sensor','" + obj.id + "',fillSensors)\" style=\"font-size: 12px\" class=\"btn btn-danger\">Remover</button>" +
        "                    <button onclick=\"saveSensor('" + obj.id + "')\" style=\"font-size: 12px\" class=\"btn btn-primary\">Guardar</button>" +
        "                </div></div></div></div>");
}


function fillSensors(payload) {
    if (!payload) return;
    $('#sensor_config').empty();

    for (let obj of payload.sort(sortByProperty('name'))) {
        buildSensor(obj);
    }
}


function getSensorFunctions(obj) {
    console.log(obj);
    var a = "";
    for (let fun of obj.functions) {
        a += "<tr>" +
            "<td><span style=\"font-size: 10px;\" class=\"label-device\">FUNÇÃO</span></td>" +
            "<td><input  style=\"font-size: 10px; height: 20px;\"  class=\"form-control\" value=\"" + fun.name + "\" type=\"text\"  id=\"name_" + obj.id + "_" + fun.uniqueName + "\" placeholder=\"ex: sala\"  required=\"true\"/></td> <tr></tr>" +
            "</tr>";
        if (obj.type == 56) {
            a += "<tr>" +
                "<td><span style=\"font-size: 10px;\" class=\"label-device\">Payload Fechado</span></td>" +
                "<td><input  style=\"font-size: 10px; height: 20px;\"  class=\"form-control\" value=\"" + fun.payloadOn + "\" type=\"text\"  id=\"payloadOn_" + obj.id + "_" + fun.uniqueName + "\" placeholder=\"ex: ON\"  required=\"true\"/></td> <tr></tr>" +
                "<td><span style=\"font-size: 10px;\" class=\"label-device\">Payload Aberto</span></td>" +
                "<td><input  style=\"font-size: 10px; height: 20px;\"  class=\"form-control\" value=\"" + fun.payloadOff + "\" type=\"text\"  id=\"payloadOff_" + obj.id + "_" + fun.uniqueName + "\" placeholder=\"ex: OFF\"  required=\"true\"/></td> <tr></tr>" +

                "</tr>";
        }
    }
    return a;

}


function buildSensorDHTemplate() {
    if ($('#sn_0').length > 0) {
        return
    }
    let sensor = {
        "id": "NEW",
        "gpio": 14,
        "name": "Temperatura",
        "disabled": true,
        "type": 2,
        "class": "sensor",
        "discoveryDisabled": false,
        "mqttStateTopic": "-",
        "functions": [{
            "name": "Temperatura",
            "uniqueName": "temperature",
            "icon": "fa-thermometer-half",
            "unit": "ºC",
            "type": 1,
        }, {
            "name": "Humidade",
            "uniqueName": "humidity",
            "icon": "fa-percent",
            "unit": "%",
            "type": 2
        }]
    };
    buildSensor(sensor);
}

function buildSensorDallasTemplate() {
    if ($('#sn_0').length > 0) {
        return
    }
    let sensor = {
        "id": "NEW",
        "gpio": 0,
        "name": "Temperatura",
        "disabled": false,
        "type": 90,
        "class": "sensor",
        "mqttStateTopic": "-",
        "functions": [{
            "name": "Temperatura",
            "uniqueName": "temperature",
            "unit": "ºC",
            "type": 1
        }]
    };
    buildSensor(sensor);
}

function buildSensorMagTemplate() {
    if ($('#sn_0').length > 0) {
        return
    }
    let sensor = {
        "id": "NEW",
        "gpio": 14,
        "name": "Magnético",
        "disabled": false,
        "type": 56,
        "class": "binary_sensor",
        "mqttStateTopic": "-",
        "functions": [{
            "payloadOn": "ON",
            "payloadOff": "OFF",
            "name": "Janela",
            "uniqueName": "opening",
            "type": 5,
        }]
    };
    buildSensor(sensor);
}

function buildSensorPirTemplate() {
    if ($('#sn_0').length > 0) {
        return
    }
    let sensor = {
        "id": "NEW",
        "gpio": 0,
        "name": "Movimento",
        "disabled": false,
        "type": 65,
        "class": "binary_sensor",
        "mqttStateTopic": "-",
        "functions": [{
            "name": "Movimento",
            "uniqueName": "motion",
            "type": 4
        }]
    };
    buildSensor(sensor);
}

function buildSensorLdrTemplate() {
    if ($('#sn_0').length > 0) {
        return
    }
    let sensor = {
        "id": "NEW",
        "gpio": 0,
        "name": "Sensor de Luz",
        "disabled": false,
        "type": 21,
        "class": "sensor",
        "mqttStateTopic": "-",
        "functions": [{
            "name": "Sensor de Luz",
            "uniqueName": "illuminance",
            "unit": "lx",
            "type": 7
        }]
    };
    buildSensor(sensor);
}


function saveSwitch(id) {
    let device = {
        "id": id,
        "name": $('#name_' + id).val(),
        "family": parseInt($('#family' + id).val()),
        "primaryGpio": parseInt($('#primaryGpio_' + id).val()),
        "secondaryGpio": parseInt($('#secondaryGpio_' + id).val()),
        "timeBetweenStates": parseInt($('#timeBetweenStates_' + id).val()),
        "autoState": false,
        "autoStateDelay": parseInt($('#autoStateDelay_' + id).val()),
        "typeControl": $('#typeControl_' + id).val(),
        "mode": parseInt($('#mode' + id).val()),
        "pullup": document.getElementById('#pullup_' + id).checked(),
        "mqttReatain": document.getElementById('#mqttReatain_' + id).checked(),
        "inverted": document.getElementById('#inverted_' + id).checked(),
        "gpioSingleControl": parseInt($('#gpioSingleControl_' + id).val()),
        "gpioOpenControl": parseInt($('#gpioOpenControl_' + id).val()),
        "gpioCloseControl": parseInt($('#gpioCloseControl_' + id).val()),
        "gpioOpenCloseControl": parseInt($('#gpioOpenCloseControl_' + id).val()),
        "gpioStopControl": parseInt($('#gpioStopControl_' + id).val()),
    };

    storeDevice(device, "save-switch", "switches", fillSwitches);
}


function saveSensor(id) {
    let payloadOn = $("#payloadOn_" + id + "_opening").val();
    let payloadOff = $("#payloadOff_" + id + "_opening").val();
    let temp = $("#name_" + id + "_temperature").val();
    let hum = $("#name_" + id + "_humidity").val();
    let motion = $("#name_" + id + "_motion").val();
    let ldr = $("#name_" + id + "_illuminance").val();
    let reed = $("#name_" + id + "_opening").val();
    let functions = [];
    let type = $('#type_' + id).val();
    let classs = "sensor";
    if (type === '0' || type === '1' || type === '2') {
        functions = [{
            "name": temp, "uniqueName": "temperature",
            "unit": "ºC",
            "type": 1
        }, {
            "name": hum, "uniqueName": "humidity",
            "unit": "%",
            "type": 2
        }];
    } else if (type === '90') {
        functions = [{
            "name": temp, "uniqueName": "temperature",
            "unit": "ºC",
            "type": 1
        }];
    } else if ('65' === type) {
        classs = "binary_sensor";
        functions = [{
            "name": motion, "uniqueName": "motion",
            "type": 4
        }];
    } else if ('21' === type) {
        functions = [{
            "name": ldr,
            "uniqueName": "illuminance",
            "unit": "lx",
            "type": 7
        }];
    } else if ('56' === type) {
        classs = "binary_sensor";
        functions = [{
            "payloadOff": payloadOff,
            "payloadOn": payloadOn,
            "name": reed,
            "uniqueName": "opening",
            "type": 5
        }];
    }

    let device = {
        "id": id,
        "class": classs,
        "name": $('#name_' + id).val(),
        "gpio": $('#gpio_' + id).val(),
        "disabled": false,
        "discoveryDisabled": false,
        "type": $('#type_' + id).val(),
        "functions": functions
    };
    storeDevice(id, device, "save-sensor", "sensors", fillSensors);
}


function updateSwitch(obj) {
    if (!obj) return;
    let btn = $('#btn_' + obj["id"]);
    btn.text(obj["stateControl"] ? 'ON' : 'OFF');
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
    wifiStatus();
    toggleActive("node");
    setInterval(wifiStatus, 15000);

});

function storeConfig() {
    console.log(config);
    const targetUrl = endpoint.baseUrl + "/save-config";
    $.ajax({
        type: "POST",
        url: targetUrl,
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(config),
        success: function (response) {
            this.config = response;
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
    config.nodeId = $('#nodeId').val();
    storeConfig();
}

function saveWifi() {
    config.wifiSSID = $('#ssid').val();
    config.wifiSecret = $('#wifi_secret').val();
    config.wifiIp = $('#wifiIp').val();
    config.wifiMask = $('#wifiMask').val();
    config.wifiGw = $('#wifiGw').val();
    config.staticIp = !document.getElementById("staticIp").checked;
    config.apSecret = $('#apSecret').val();
    storeConfig();
}

function saveMqtt() {
    config.mqttIpDns = $('#mqtt_ip').val();
    config.mqttUsername = $('#mqtt_username').val();
    config.mqttPassword = $('#mqtt_password').val();
    storeConfig();
}

function removeDevice(e, id, func) {
    const targetUrl = endpoint.baseUrl + "/" + e + "?id=" + id;
    $.ajax({
        url: targetUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            func(response);
        },
        error: function () {
            showMessage("Não foi possivel remvover a funcionalidade, por favor tenta novamente", "Unable to remove this feature, please try again.")
        },
        timeout: 2000
    });
}

function storeDevice(device, endpointstore, endointget, func) {
    const targetUrl = endpoint.baseUrl + "/" + endpointstore + "?id=" + device.id;
    $.ajax({
        type: "POST",
        url: targetUrl,
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(device),
        success: function (response) {
            loadDevice(func, endointget);
            showMessage("Configuração Guardada", "Config Stored")

        },
        error: function () {
            showMessage("Não foi possivel guardar a configuração atual, por favor tenta novamente.", "Unable to save current configuration, please try again.")
        }, complete: function () {

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

function wifiStatus() {
    $.ajax({
        url: endpoint.baseUrl + "/wifi-status",
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            $('#ssid_lbl').text(response.wifiSSID);
            if (document.getElementById("wifiIp") && $('input[name="wifiIp"]').val().trim().length === 0) {
                $('input[name="wifiIp"]').val(response.wifiIp);
                $('input[name="wifiMask"]').val(response.wifiMask);
                $('input[name="wifiGw"]').val(response.wifiGw);
            }
            var percentage = Math.min(2 * (parseInt(response.signal) + 100), 100);
            $('#wifi-signal').text(percentage + "%");
        }, error: function () {
            $('#wifi-signal').text("0%");
        },
        timeout: 1000
    });
}

function buildSwitchTemplate() {
    if ($('#bs_0').length > 0) return
    let device = {
        "id": "NEW",
        "name": "Novo Interruptor",
        "family": "switch",
        "primaryGpio": 99,
        "secondaryGpio": 99,
        "timeBetweenStates": 60000,
        "autoState": false,
        "autoStateDelay": 0,
        "typeControl": 2,
        "mode": 1,
        "pullup": false,
        "mqttReatain": true,
        "inverted": false,
        "mqttCommandTopic": "-",
        "mqttStateTopic": "-",
        "mqttPositionCommandTopic": "-",
        "mqttPositionStateTopic": "-",
        "gpioSingleControl": 99,
        "gpioOpenControl": 99,
        "gpioCloseControl": 99,
        "gpioOpenCloseControl": 99,
        "gpioStopControl": 99
    };
    buildSwitch(device);
}