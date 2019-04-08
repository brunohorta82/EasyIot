const endpoint = {
    baseUrl: ""
};

var switchs;
function toggleSwitch(id) {
    const someUrl = endpoint.baseUrl + "/toggle-switch?id=" + id;
    $.ajax({
        type: "POST",
        url: someUrl,
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

function removeDevice(e, id, func) {
    const someUrl = endpoint.baseUrl + "/" + e + "?id=" + id;
    $.ajax({
        url: someUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            func(response);
        },
        error: function () {
            alert("Erro a ao remover dispositivo");
        },
        timeout: 2000
    });
}

function storeDevice(id, _device, endpointstore, endointget, func) {
    const someUrl = endpoint.baseUrl + "/" + endpointstore + "?id=" + id;
    $.ajax({
        type: "POST",
        url: someUrl,
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(_device),
        success: function (response) {
            loadDevice(func, endointget);
            alert("Configuração Guardada");
        },
        error: function () {
            alert("Erro não foi possivel guardar a configuração");
        }, complete: function () {

        },
        timeout: 2000
    });
}

function storeConfig(path, newConfig) {
    const someUrl = endpoint.baseUrl + "/" + path;
    $.ajax({
        type: "POST",
        url: someUrl,
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(newConfig),
        success: function (response) {
            alert("Configuração Guardada");
        },
        error: function () {

        }, complete: function () {

        },
        timeout: 2000
    });
}

function toggleAP() {
    if ($('#wifi_status').text() !== 'ligado') {
        alert("Só é possivel desligar o AP depois de estar ligado com sucesso a uma Rede Wi-Fi")
        return;
    }
    const someUrl = endpoint.baseUrl + "/dissableAP";
    $.ajax({
        url: someUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
        },
        timeout: 2000
    });
}

function findNetworks() {
    $('#networks').empty();
    $('#status-scan').text('a pesquisar, aguarde...');
    const someUrl = endpoint.baseUrl + "/scan";
    $.ajax({
        url: someUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {

        },
        error: function () {
            alert("Erro não foi possivel fazer o pedido");
        }, complete: function () {

        },
        timeout: 2000
    });
}

function loadConfig(next) {
    const someUrl = endpoint.baseUrl + "/config";
    $.ajax({
        url: someUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            fillConfig(response);
            if (next) {
                next();
            }

        },
        error: function () {
            alert("Erro a carregar configuração");
        }, complete: function () {

        },
        timeout: 2000
    });
}

function loadDevice(func, e, next) {
    const someUrl = endpoint.baseUrl + "/" + e;
    $.ajax({
        url: someUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            func(response);
            if (next) {
                next();
            }

        },
        error: function () {
            alert("Erro a carregar configuração dos dispositivos");
        },
        timeout: 2000
    });
}

function fillConfig(response) {
    $("#firmwareVersion").text(response.configVersion);
    $(".bh-model").text(response.hardware);
        $(".bh-onofre-item").removeClass("hide");
    $("#version_lbl").text(response.configVersion);
    $('input[name="nodeId"]').val(response.nodeId);
    $('input[name="mqttIpDns"]').val(response.mqttIpDns);
    $('input[name="mqttUsername"]').val(response.mqttUsername);
    $('select[name="homeAssistantAutoDiscovery"] option[value="' + response.homeAssistantAutoDiscovery + '"]').attr("selected", "selected");
    $('input[name="homeAssistantAutoDiscoveryPrefix"]').val(response.homeAssistantAutoDiscoveryPrefix);
    $('input[name="mqttPassword"]').val(response.mqttPassword);
    $('input[name="wifiSSID"]').val(response.wifiSSID);
    $('input[name="wifiSecret"]').val(response.wifiSecret);
    $('select[name="staticIp"] option[value="' + response.staticIp + '"]').attr("selected", "selected");
    $('input[name="wifiIp"]').val(response.wifiIp);
    $('input[name="wifiMask"]').val(response.wifiMask);
    $('input[name="wifiGw"]').val(response.wifiGw);
    $('input[name="apSecret"]').val(response.apSecret);
    $('select[name="notificationInterval"] option[value="' + response.notificationInterval + '"]').attr("selected", "selected");
    $('select[name="directionCurrentDetection"] option[value="' + response.directionCurrentDetection + '"]').attr("selected", "selected");

    $('#ff').prop('disabled', false);
}

function toggleActive(menu) {
    $('.sidebar-menu').find('li').removeClass('active');
    $('.menu-item[data-menu="' + menu + '"]').closest('li').addClass('active');
    $(".content").load(menu + ".html", function () {
        if (menu === "dashboard") {
            loadConfig(function () {
                    loadDevice(refreshDashboard, "switchs");

            })
        } else if (menu === "devices") {
            loadDevice(fillSwitches, "switchs", function () {
                loadDevice(fillRelays, "relays", function () {
                    loadDevice(fillSensors, "sensors");
                });
            });

        } else if (menu === "wifi") {
            loadConfig(function () {
                wifiStatus();
            });

        } else {
            loadConfig();
        }

    });
}

function fillSwitches(payload) {
    if (!payload) return;
    switchs = payload;
    $('#switch_config').empty();
    for (let obj of payload) {
        buildSwitch(obj);
    }
    console.log(payload.length);
}

function fillRelays(payload) {
    if (!payload) return;
    $('#relay_config').empty();
    for (let obj of payload) {
        buildRelay(obj);
    }
}

function buildSwitch(obj) {
    $('#switch_config').append("<div id=\"bs_" + obj.id + "\" class=\"col-lg-4 col-md-6 col-xs-12\">" +
        "        <div style=\"margin-bottom: 0\" class=\"info-box bg-aqua\"><span class=\"info-box-icon\">" +
        "        <i id=\"icon_" + obj.id + "\" class=\"fa " + obj.icon + " false\"></i></span>" +
        "            <div class=\"info-box-content\"><span class=\"info-box-text\">" + obj.name + "</span>" +
        "<div  id=\"on_off_control_" + obj.id + "\" class=\"" + ((obj.mode === 4 || obj.mode === 5) ? 'hide' : '') + "\">" +
        "                <button onclick='toggleSwitch(\"" + obj.id + "\");' id=\"btn_" + obj.id + "\" style=\"float: right\" class=\"btn btn-primary\" >" + (obj.stateControl ? 'ON' : 'OFF') + "</button>" +
        "</div><div  id=\"open_close_control_" + obj.id + "\" class=\"" + ((obj.mode === 4 || obj.mode === 5) ? '' : 'hide') + "\">" +
        "<button onclick='stateSwitch(\"" + obj.id + "\",\"OPEN\");' id=\"btn_up_" + obj.id + "\" style=\"float: right\" class=\"btn btn-primary\" >ABRIR</button><button onclick='stateSwitch(\"" + obj.id + "\",\"STOP\");' id=\"btn_stop_" + obj.id + "\" style=\"float: right\" class=\"btn btn-primary\" >PARAR</button><button onclick='stateSwitch(\"" + obj.id + "\",\"CLOSE\");' id=\"btn_down_" + obj.id + "\" style=\"float: right\" class=\"btn btn-primary\" >FECHAR</button>"+
        "            </div></div>" +
        "        </div>" +
        "        <div style=\"font-size: 10px;  border: 0px solid #08c; border-radius: 0\" class=\"box\">" +
        "            <div class=\"box-body no-padding\">" +
        "                <table class=\"table table-condensed\">" +
        "                    <tbody>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">NOME</span></td>" +
        "                        <td><input  style=\"font-size: 10px; height: 20px;\"  class=\"form-control\" value=\"" + obj.name + "\" type=\"text\"  id=\"name_" + obj.id + "\" placeholder=\"ex: luz sala\"  required=\"true\"/></td>" +
        "                    </tr>" +
        "                   <tr  id=\"tipo_" + obj.id + "\" class=\"" + (obj.mode === 4 ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">TIPO</span></td>" +
        "                        <td><select onchange=\"switchTypeOptions('" + obj.id + "')\" class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"type_" + obj.id + "\">" +
        "                            <option " + (obj.type === 'switch' ? 'selected' : '') + " value=\"switch\">Genérico</option>" +
        "                            <option " + (obj.type === 'light' ? 'selected' : '') + " value=\"light\">Luz</option>" +
        "                            <option " + (obj.type === 'cover' ? 'selected' : '') + " value=\"cover\">Estore</option>" +
        "                            <option " + (obj.type === 'lock' ? 'selected' : '') + " value=\"lock\">Fechadura</option>" +
        "                            <option " + (obj.type === 'sensor' ? 'selected' : '') + " value=\"sensor\">Sensor</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">MODO</span></td>" +
        "                   <td   id=\"mode_" + obj.id + "\" class=\"generic_type_" + obj.id + " " + (obj.type !== 'cover' ? '' : 'hide') + "\">" +
        "                            <select onchange=\"switchModeRules('" + obj.id + "')\" class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"modeg_" + obj.id + "\">" +
        "                                <option " + (obj.mode === 1 ? 'selected' : '') + " value=\"1\">ON | OFF NORMAL</option>" +
        "                                <option " + (obj.mode === 2 ? 'selected' : '') + " value=\"2\">ON | OFF PUSH/TOUCH</option>" +
        "                                <option " + (obj.mode === 6 ? 'selected' : '') + " value=\"6\">ON | AUTO OFF PUSH/TOUCH</option>" +
        "                            </select>" +
        "                        </td>" +
        "                   <td   id=\"mode_" + obj.id + "\" class=\"cover_type_" + obj.id + " " + (obj.type === 'cover' ? '' : 'hide') + "\">" +
        "                            <select onchange=\"switchModeRules('" + obj.id + "')\" class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"modec_" + obj.id + "\">" +
        "                                <option " + (obj.mode === 4 ? 'selected' : '') + " value=\"4\">OPEN | STOP | CLOSE NORMAL</option>" +
        "                                <option " + (obj.mode === 5 ? 'selected' : '') + " value=\"5\">OPEN | STOP | CLOSE PUSH/TOUCH</option>" +
        "                            </select>" +
        "                        </td>" +
        "                    </tr>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">PULLUP</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"pullup_" + obj.id + "\">" +
        "                            <option " + (obj.pullup ? 'selected' : '') + " value=\"true\">SIM</option>" +
        "                            <option " + (!obj.pullup ? 'selected' : '') + " value=\"false\">NÃO</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                   <tr  id=\"single_gpio_" + obj.id + "\" class=\"" + (obj.mode === 4 ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">GPIO</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"gpio_" + obj.id + "\">" +
        "                            <option " + (obj.gpio === 99 ? 'selected' : '') + " value=\"99\">NENHUM</option>" +
        "                            <option " + (obj.gpio === 0 ? 'selected' : '') + " value=\"0\">0</option>" +
        "                            <option " + (obj.gpio === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpio === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpio === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpio === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                   <tr  id=\"open_gpio_" + obj.id + "\" class=\"" + (obj.mode !== 4 ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">GPIO OPEN</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"gpio_open_" + obj.id + "\">" +
        "                            <option " + (obj.gpioOpen === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpioOpen === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpioOpen === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpioOpen === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                   <tr  id=\"close_gpio_" + obj.id + "\" class=\"" + (obj.mode !== 4 ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">GPIO CLOSE</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"gpio_close_" + obj.id + "\">" +
        "                            <option " + (obj.gpioClose === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpioClose === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpioClose === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpioClose === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +

        "                   <tr  id=\"relay_open_gpio_" + obj.id + "\" class=\"" + ((obj.mode === 4 || obj.mode === 5) ? '' : 'hide') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">RELÉ OPEN</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"relay_open_" + obj.id + "\">" +
        "                            <option " + (obj.gpioControlOpen === 4 ? 'selected' : '') + " value=\"4\">4</option>" +
        "                            <option " + (obj.gpioControlOpen === 5 ? 'selected' : '') + " value=\"5\">5</option>" +
        "                            <option " + (obj.gpioControlOpen === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpioControlOpen === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpioControlOpen === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpioControlOpen === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                   <tr  id=\"relay_close_gpio_" + obj.id + "\" class=\"" + ((obj.mode === 4 || obj.mode === 5) ? '' : 'hide') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">RELÉ CLOSE</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"relay_close_" + obj.id + "\">" +
        "                            <option " + (obj.gpioControlClose === 4 ? 'selected' : '') + " value=\"4\">4</option>" +
        "                            <option " + (obj.gpioControlClose === 5 ? 'selected' : '') + " value=\"5\">5</option>" +
        "                            <option " + (obj.gpioControlClose === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpioControlClose === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpioControlClose === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpioControlClose === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                   <tr  id=\"type_mode_" + obj.id + "\" class=\"" + ((obj.mode === 4 || obj.mode === 5) ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device \">COMUTA</span></td>" +
        "                        <td><div class=\"row\">" +
        "                <div class=\"col-xs-5\">" +
        "                        <select onchange=\"switchTypeRules('" + obj.id + "')\" class=\"form-control\" style=\"font-size: 10px;  padding: 0 12px; height: 20px;\"" +
        "                                 id=\"typeControl_" + obj.id + "\">" +
        "                            <option " + (obj.typeControl === "relay" ? 'selected' : '') + " value=\"relay\">RELÉ</option>" +
        "                            <option " + (obj.typeControl === "mqtt" ? 'selected' : '') + " value=\"mqtt\">MQTT</option>" +
        "                        </select>" +
        "                        </select>" +
        "                </div>" +
        "                <div  id=\"type-control-lbl" + obj.id + "\" class=\"col-xs-2 " + (obj.typeControl === 'mqtt' ? 'hide' : '') + "\">ID/GPIO</div>" +
        "                <div  id=\"type-control-box" + obj.id + "\" class=\"col-xs-5 " + (obj.typeControl === 'mqtt' ? 'hide' : '') + "\">" +
        "                           <select class=\"form-control\" style=\" font-size: 10px;padding: 0px 12px; height: 20px;\"" +
        "                                 id=\"gpioControl_" + obj.id + "\">" +
        "                            <option " + (obj.gpioControl === 0 ? 'selected' : '') + " value=\"0\">0</option>" +
        "                            <option " + (obj.gpioControl === 2 ? 'selected' : '') + " value=\"2\">2</option>" +
        "                            <option " + (obj.gpioControl === 4 ? 'selected' : '') + " value=\"4\">4</option>" +
        "                            <option " + (obj.gpioControl === 5 ? 'selected' : '') + " value=\"5\">5</option>" +
        "                            <option " + (obj.gpioControl === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpioControl === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpioControl === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpioControl === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select>" +
        "                </div>" +
        "              </div>" +


        "</td>" +
        "                    </tr>" +
        "                   <tr class=\"hide " + (obj.typeControl === 'mqtt' ? 'hide' : '') + "\">" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">MESTRE</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"master_" + obj.id + "\">" +
        "                            <option " + (!obj.master ? 'selected' : '') + " value=\"true\">SIM</option>" +
        "                            <option " + (obj.master ? 'selected' : '') + " value=\"false\">NÃO</option>" +
        "                        </select></td>" +
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
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">AUTO DISCOVERY</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"discovery_" + obj.id + "\">" +
        "                            <option " + (!obj.discoveryDisabled ? 'selected' : '') + " value=\"false\">SIM</option>" +
        "                            <option " + (obj.discoveryDisabled ? 'selected' : '') + " value=\"true\">NÃO</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                    </tbody>" +
        "                </table>" +
        "                <div class=\"box-footer save\">" +
        "                    <button onclick=\"removeDevice('remove-switch','" + obj.id + "',fillSwitches)\" style=\"font-size: 12px\" class=\"btn btn-danger\">Remover</button>" +
        "                    <button onclick=\"saveSwitch('" + obj.id + "')\" style=\"font-size: 12px\" class=\"btn btn-primary\">Guardar</button>" +

        "                </div>" +
        "            </div>" +
        "        </div>" +
        "" +
        "    </div>"
    );




}

function stateSwitch(id, state) {
    const someUrl = endpoint.baseUrl + "/state-switch?state=" + state + "&id=" + id;
    $.ajax({
        type: "POST",
        url: someUrl,
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
    $('#icon_' + e).removeClass("fa-window-maximize").removeClass("fa-lightbulb-o").removeClass("fa-plug").removeClass("fa-lock").removeClass("fa-circle-o");

    switchModeRules(e);
    if (type === "cover") {
        $('#icon_' + e).addClass("fa-window-maximize");
    } else if (type === "light") {
        $('#icon_' + e).addClass("fa-lightbulb-o");
    } else if (type === "switch") {
        $('#icon_' + e).addClass("fa-plug");
    } else if (type === "lock") {
        $('#icon_' + e).addClass("fa-lock");
    } else if (type === "sensor") {
        console.log(type);
        $('#icon_' + e).addClass("fa-circle-o");
    }
}

function switchModeRules(e) {
    var mode = 0;
    let type = $('#type_' + e).val();

    if (type !== 'cover') {
        mode = $('#modeg_' + e).val();
    } else {
        mode = $('#modec_' + e).val();
    }
    console.log(mode);
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

function buildRelay(obj) {

    $('#relay_config').append("<div id=\"rl_" + obj.id + "\" class=\"col-lg-4 col-md-6 col-xs-12\">" +
        "        <div style=\"margin-bottom: 0px\" class=\"info-box bg-aqua\"><span class=\"info-box-icon\">" +
        "        <i id=\"icon_" + obj.id + "\" class=\"fa " + obj.icon + " false off\"></i></span>" +
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
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">GPIO</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"gpio_" + obj.id + "\">" +
        "                            <option " + (obj.gpio === 0 ? 'selected' : '') + " value=\"0\">0</option>" +
        "                            <option " + (obj.gpio === 2 ? 'selected' : '') + " value=\"2\">2</option>" +
        "                            <option " + (obj.gpio === 4 ? 'selected' : '') + " value=\"4\">4</option>" +
        "                            <option " + (obj.gpio === 5 ? 'selected' : '') + " value=\"5\">5</option>" +
        "                            <option " + (obj.gpio === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpio === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpio === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpio === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">INVERTIDO</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"inverted_" + obj.id + "\">" +
        "                            <option " + (obj.inverted ? 'selected' : '') + " value=\"true\">SIM</option>" +
        "                            <option " + (!obj.inverted ? 'selected' : '') + " value=\"false\">NÃO</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                    </tbody>" +
        "                </table>" +
        "                <div class=\"box-footer save\">" +
        "                    <button onclick=\"removeDevice('remove-relay','" + obj.id + "',fillRelays)\" style=\"font-size: 12px\" class=\"btn btn-danger\">Remover</button>" +
        "                    <button onclick=\"saveRelay('" + obj.id + "')\" style=\"font-size: 12px\" class=\"btn btn-primary\">Guardar</button>" +
        "                </div></div></div></div>");

}

function buildSensor(obj) {
    $('#sensor_config').append("<div id=\"sn_" + obj.id + "\" class=\"col-lg-4 col-md-6 col-xs-12\">" +
        "        <div style=\"margin-bottom: 0px\" class=\"info-box bg-aqua\"><span class=\"info-box-icon\">" +
        "        <i id=\"icon_" + obj.id + "\" class=\"fa " + obj.icon + " false off\"></i></span>" +
        "            <div class=\"info-box-content\"><span class=\"info-box-text\">" + obj.name + "</span>" +
        "            </div>" +
        "        </div>" +
        "        <div style=\"font-size: 10px; border-radius: 0\" class=\"box\">" +
        "            <div class=\"box-body no-padding\">" +
        "                <table class=\"table table-condensed\">" +
        "                    <tbody>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">ATIVO</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"disabled_" + obj.id + "\">" +
        "                            <option " + (obj.disabled ? 'selected' : '') + " value=\"true\">NÃO</option>" +
        "                            <option " + (!obj.disabled ? 'selected' : '') + " value=\"false\">SIM</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">NOME</span></td>" +
        "                        <td><input  style=\"font-size: 10px; height: 20px;\"  class=\"form-control\" value=\"" + obj.name + "\" type=\"text\"  id=\"name_" + obj.id + "\" placeholder=\"ex: luz sala\"  required=\"true\"/></td>" +
        "                    </tr>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">TIPO</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"type_" + obj.id + "\">" +
        "                            <option  " + (obj.type === 0 ? 'selected' : '') + " value=\"0\">DHT 11</option>" +
        "                            <option " + (obj.type === 1 ? 'selected' : '') + " value=\"1\">DHT 21</option>" +
        "                            <option " + (obj.type === 2 ? 'selected' : '') + " value=\"2\">DHT 22</option>" +
        "                            <option " + (obj.type === 90 ? 'selected' : '') + " value=\"90\">DS18B20</option>" +
        "                            <option " + (obj.type === 65 ? 'selected' : '') + " value=\"65\">PIR</option>" +
        "                            <option " + (obj.type === 21 ? 'selected' : '') + " value=\"21\">LDR</option>" +
        "                        </select></td>" +
        "                    </tr>" +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">GPIO</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                    id=\"gpio_" + obj.id + "\">" +
        "                            <option " + (obj.gpio === 4 ? 'selected' : '') + " value=\"4\">4</option>" +
        "                            <option " + (obj.gpio === 4 ? 'selected' : '') + " value=\"4\">4</option>" +
        "                            <option " + (obj.gpio === 5 ? 'selected' : '') + " value=\"5\">5</option>" +
        "                            <option " + (obj.gpio === 12 ? 'selected' : '') + " value=\"12\">12</option>" +
        "                            <option " + (obj.gpio === 13 ? 'selected' : '') + " value=\"13\">13</option>" +
        "                            <option " + (obj.gpio === 14 ? 'selected' : '') + " value=\"14\">14</option>" +
        "                            <option " + (obj.gpio === 16 ? 'selected' : '') + " value=\"16\">16</option>" +
        "                            <option " + (obj.type === 21 ? 'selected' : '') + " value=\"A0\">A0</option>" +
        "                        </select></td>" +
        "                    </tr>" + getSensorFunctions(obj) +
        "                    <tr>" +
        "                        <td><span style=\"font-size: 10px;\" class=\"label-device\">AUTO DISCOVERY</span></td>" +
        "                        <td><select class=\"form-control\" style=\"font-size: 10px; padding: 0px 12px; height: 20px;\"" +
        "                                     id=\"discovery_" + obj.id + "\">" +
        "                            <option " + (!obj.discoveryDisabled ? 'selected' : '') + " value=\"false\">SIM</option>" +
        "                            <option " + (obj.discoveryDisabled ? 'selected' : '') + " value=\"true\">NÃO</option>" +
        "                        </select></td>" +
        "                    </tr>" +
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
    for (let obj of payload) {
        buildSensor(obj);
    }
}


function getSensorFunctions(obj) {
    var a = "";
    for (let fun of obj.functions) {
        a += "<tr>" +
            "<td><span style=\"font-size: 10px;\" class=\"label-device\">FUNÇÃO</span></td>" +
            "<td><input  style=\"font-size: 10px; height: 20px;\"  class=\"form-control\" value=\"" + fun.name + "\" type=\"text\"  id=\"name_" + obj.id + "_" + fun.uniqueName + "\" placeholder=\"ex: sala\"  required=\"true\"/></td> <tr></tr>" +
            "<td><span style=\"font-size: 10px;\" class=\"label-device\">MQTT ESTADO</span></td>" +
            "<td><span style=\"font-weight: bold; font-size:11px; color: #00a65a\">" + fun.mqttStateTopic + "</span></td>" +
            "</tr>";
    }
    return a;

}

function buildSwitchTemplate() {
    if ($('#bs_0').length > 0) {
        return
    }
    let device = {
        "id": 0,
        "name": "Novo Interruptor",
        "gpio": 0,
        "pullup": true,
        "mode": 1,
        "typeControl": "mqtt",
        "gpioControl": 0,
        "master": true,
        "mqttCommandTopic": "-",
        "mqttStateTopic": "-",
    };
    buildSwitch(device);
}

function buildSensorDHTemplate() {
    if ($('#sn_0').length > 0) {
        return
    }
    let sensor = {
        "id": "0",
        "gpio": 14,
        "name": "Temperatura",
        "disabled": true,
        "type": 2,
        "class": "sensor",
        "discoveryDisabled": false,
        "functions": [{
            "name": "Temperatura",
            "uniqueName": "temperature",
            "icon": "fa-thermometer-half",
            "unit": "ºC",
            "type": 1,
            "mqttStateTopic": "-",
            "mqttRetain": false
        }, {
            "name": "Humidade",
            "uniqueName": "humidity",
            "icon": "fa-percent",
            "unit": "%",
            "type": 2,
            "mqttStateTopic": "-",
            "mqttRetain": false
        }]
    };
    buildSensor(sensor);
}

function buildSensorDallasTemplate() {
    if ($('#sn_0').length > 0) {
        return
    }
    let sensor = {
        "id": "0",
        "gpio": 0,
        "name": "Temperatura",
        "disabled": false,
        "type": 90,
        "class": "sensor",
        "functions": [{
            "name": "Temperatura",
            "uniqueName": "temperature",
            "unit": "ºC",
            "type": 1,
            "mqttStateTopic": "-",
            "mqttRetain": false
        }]
    };
    buildSensor(sensor);
}

function buildSensorPirTemplate() {
    if ($('#sn_0').length > 0) {
        return
    }
    let sensor = {
        "id": "0",
        "gpio": 0,
        "name": "Movimento",
        "disabled": false,
        "type": 65,
        "class": "sensor",
        "functions": [{
            "name": "Movimento",
            "uniqueName": "motion",
            "unit": "",
            "type": 4,
            "mqttStateTopic": "-",
            "mqttRetain": false
        }]
    };
    buildSensor(sensor);
}
function buildSensorLdrTemplate() {
    if ($('#sn_0').length > 0) {
        return
    }
    let sensor = {
        "id": "0",
        "gpio": 0,
        "name": "Sensor de Luz",
        "disabled": false,
        "type": 21,
        "class": "sensor",
        "functions": [{
            "name": "Sensor de Luz",
            "uniqueName": "light_sensor",
            "unit": "",
            "type": 7,
            "mqttStateTopic": "-",
            "mqttRetain": false
        }]
    };
    buildSensor(sensor);
}
function buildRelayTemplate() {
    if ($('#rl_0').length > 0) {
        return
    }
    let device = {
        "id": 0,
        "name": "Novo Rlé",
        "gpio": 0,
        "inverted": false,
        "mode": 1,
        "maxAmp": 2, "state": false, "class": "relay"
    };
    buildRelay(device);
}

function saveSwitch(id) {
    var mode = 1;
    let type = $('#type_' + id).val();

    if (type !== 'cover') {
        mode = $('#modeg_' + id).val();
    } else {
        mode = $('#modec_' + id).val();
    }
    let device = {
        "name": $('#name_' + id).val(),
        "gpio": $('#gpio_' + id).val(),
        "gpioOpen": $('#gpio_open_' + id).val(),
        "gpioClose": $('#gpio_close_' + id).val(),
        "pullup": $('#pullup_' + id).val(),
        "discoveryDisabled": $('#discovery_' + id).val(),
        "type": type,
        "mode": mode,
        "typeControl": $('#typeControl_' + id).val(),
        "gpioControl": $('#gpioControl_' + id).val(),
        "gpioControlOpen": $('#relay_open_' + id).val(),
        "gpioControlClose": $('#relay_close_' + id).val(),
        "master": true
        // "master": $('#master_' + id).val()
    };

    storeDevice(id, device, "save-switch", "switchs", fillSwitches);
}

function saveRelay(id) {
    var device = {
        "name": $('#name_' + id).val(),
        "gpio": $('#gpio_' + id).val(),
        "inverted": $('#inverted_' + id).val()
    };
    storeDevice(id, device, "save-relay", "relays", fillRelays);
}

function saveSensor(id) {
    let temp = $("#name_" + id + "_temperature").val();
    let hum = $("#name_" + id + "_humidity").val();
    let motion = $("#name_" + id + "_motion").val();
    let ldr = $("#name_" + id + "_light_sensor").val();
    let functions = [];
    let type = $('#type_' + id).val();

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
        functions = [{
            "name": motion, "uniqueName": "motion",
            "unit": "",
            "type": 4
        }];
    }else if ('21' === type) {
        functions = [{
            "name": ldr, "uniqueName": "light_sensor",
            "unit": "",
            "type": 7
        }];
    }

    let device = {
        "name": $('#name_' + id).val(),
        "gpio": $('#gpio_' + id).val(),
        "disabled": $('#disabled_' + id).val(),
        "discoveryDisabled": $('#discovery_' + id).val(),
        "type": $('#type_' + id).val(),
        "functions": functions
    };
    storeDevice(id, device, "save-sensor", "sensors", fillSensors);
}

function saveNode() {
    let _config = {
        "nodeId": $('#nodeId').val(),
        "notificationInterval": $('#notificationInterval').val(),
        "directionCurrentDetection": $('#directionCurrentDetection').val()
    };
    storeConfig("save-node", _config);
}

function saveWifi() {
    let _config = {
        "wifiSSID": $('#ssid').val(),
        "wifiSecret": $('#wifi_secret').val(),
        "wifiIp": $('#wifiIp').val(),
        "wifiMask": $('#wifiMask').val(),
        "wifiGw": $('#wifiGw').val(),
        "staticIp": $('#staticIp').val(),
        "apSecret": $('#apSecret').val()

    };
    const someUrl = endpoint.baseUrl + "/save-wifi";
    $.ajax({
        type: "POST",
        url: someUrl,
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(_config),
        success: function (response) {
            alert("Configuração Guardada");
        },
        error: function () {

        }, complete: function () {

        },
        timeout: 2000
    });

}

function saveMqtt() {
    let _config = {
        "mqttIpDns": $('#mqtt_ip').val(),
        "mqttUsername": $('#mqtt_username').val(),
        "mqttPassword": $('#mqtt_password').val()

    };
    storeConfig("save-mqtt", _config);
}



function refreshDashboard(payload) {
    if (!payload) return;
    let devices = $('#devices');
    devices.empty();
    for (let obj of payload) {
        if (obj.type === "cover") {
            devices.append('<div class="col-lg-4 col-md-6 col-xs-12"><div style="min-height: 100px;" class="info-box bg-aqua"><div class="info-box-content"><span class="info-box-text">' + obj["name"] + '</span>' +
                '<button onclick="stateSwitch(\'' + obj["id"] + '\',\'OPEN\');" id="btn_up_' + obj["id"] + '" style="float: right" class="btn btn-primary" >ABRIR</button>' +
                '<button onclick="stateSwitch(\'' +obj["id"] + '\',\'STOP\');" id="btn_stop_' + obj["id"] + '" style="float: right" class="btn btn-primary" >PARAR</button>' +
                '<button onclick="stateSwitch(\'' +obj["id"] + '\',\'CLOSE\');" id="btn_down_' + obj["id"] + '" style="float: right" class="btn btn-primary" >FECHAR</button>' +
                '</div></div></div>');
        } else if (obj.type === "light" || obj.type === "switch") {
            devices.append('<div class="col-lg-4  col-xs-12"><div class="info-box bg-aqua"><div class="info-box-content"><span class="info-box-text">' + obj["name"] + '</span>' +
                '<button onclick="toggleSwitch(\''+obj["id"]+'\')" id="btn_' + obj["id"] + '" style="float: right" class="btn btn-primary" >' + (obj.stateControl ? "ON" : "OFF") + '</button></div></div></div>');
        }
    }
}

function updateSwitch(obj) {
    if (!obj) return;
    let btn = $('#btn_' + obj["id"]);
    btn.text(obj["stateControl"] ? 'ON' : 'OFF');
}

function wifiStatus() {
    let someUrl = endpoint.baseUrl + "/wifi-status";
    let icon = $('#wifi-icon');
    $.ajax({
        url: someUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            $('#ssid_lbl').text(response.wifiSSID);
            icon.attr('title', response.wifiSSID);
            if (response.apOn === 1) {
                $('#ap_control_btn').removeClass("disabled");
            } else {
                $('#ap_control_btn').addClass("disabled");
            }
            if (response.status) {

                if ($('#staticIp').val() === "false") {
                    $('input[name="wifiIp"]').val(response.wifiIp);
                    $('input[name="wifiMask"]').val(response.wifiMask);
                    $('input[name="wifiGw"]').val(response.wifiGw);
                }
                if (window.location.hostname === "192.168.4.1") {
                    $('#wifi_log_box').removeClass("hidden");
                    $('#wifi_log_lbl').text("O BH OnOfre já se encontra ligado à sua rede, deve desligar-se do Wi-Fi de configuração e voltar a ligar-se ao seu Wi-Fi pessoal. Pode aceder a este painel via http://" + response.wifiIp + ".");
                } else {
                    $('#wifi_log_box').addClass("hidden");
                    $('#wifi_log_lbl').text("");
                }

                $('#wifi_status').text('ligado');
                $('#wifi_status_icon').removeClass('text-danger').addClass('text-ok');
                var percentage = Math.min(2 * (parseInt(response.signal) + 100), 100);
                $('#wifi-signal').text(percentage + "%");
                if (percentage > 0 && percentage < 30) {
                    icon
                        .removeClass('signal-zero')
                        .removeClass('signal-med')
                        .removeClass('signal-good')
                        .removeClass('signal-bad')
                        .addClass('signal-bad');
                } else if (percentage >= 30 && percentage < 61) {
                    icon
                        .removeClass('signal-zero')
                        .removeClass('signal-med')
                        .removeClass('signal-good')
                        .removeClass('signal-bad')
                        .addClass('signal-med');
                } else if (percentage >= 61) {
                    icon
                        .removeClass('signal-zero')
                        .removeClass('signal-med')
                        .removeClass('signal-good')
                        .removeClass('signal-bad')
                        .addClass('signal-good');
                }
            } else {
                $('#wifi_status_icon').removeClass('text-ok').addClass('text-danger')
                $('#wifi_status').text('desligado');
            }
        }, error: function () {
            $('#wifi_status_icon').removeClass('text-ok').addClass('text-danger')
            $('#wifi_status').text('desligado');
            icon
                .removeClass('signal-med')
                .removeClass('signal-good')
                .removeClass('signal-bad')
                .addClass('signal-zero');
            $('#wifi-signal').text("0%");
        },
        timeout: 1000
    });

}


function selectNetwork(node) {
    $('input[name="wifiSSID"]').val(node.split(": ")[5].trim());
    $('input[name="wifiSecret"]').val("").focus();

}

function appendWifiLog(log) {
    console.log(log);
    $('#wifi-log').append('<p>' + log + '</p>');

}

function appendNetwork(network) {
    if (network === "No networks found") {
        $('#status-scan').text('Não foram encontradas redes wi-fi');
    } else {
        $('#status-scan').text('resultado da pesquisa: ');
        $('#networks').append('<a href="#" class="wifi-node"  data-menu="' + network + '">' + network + '</a><p></p>');
        $('.wifi-node').click(function (e) {
            var node = $(e.currentTarget).data('menu');
            selectNetwork(node);

        });
    }
}


function loadDefaults() {
    let someUrl = endpoint.baseUrl + "/loaddefaults";
    $.ajax({
        url: someUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
        },
        complete: function () {
            alert("Configuração de fábrica aplicada com sucesso. Por favor volte a ligar-se ao Access Point de configuração e aceda ao painel de controlo pelo endereço http://192.168.4.1 no seu browser.");

        },
        timeout: 1000
    });
}


function reboot() {
    let someUrl = endpoint.baseUrl + "/reboot";
    $.ajax({
        url: someUrl,
        contentType: "text/plain; charset=utf-8",
        dataType: "json",
        success: function (response) {
            alert("O OnoFre está a reiniciar, ficará disponivel dentro de 10 segundos.");
        },
        timeout: 2000
    });
}

$(document).ready(function () {
    loadConfig();
    if (!!window.EventSource) {
        const source = new EventSource(endpoint.baseUrl + '/events');
        source.addEventListener('switch', function (e) {
            updateSwitch(JSON.parse(e.data));
        }, false);

        source.addEventListener('wifi-networks', function (e) {
            appendNetwork(e.data);
        }, false);
        source.addEventListener('wifi-log', function (e) {
            appendWifiLog(e.data);
        }, false);

    }

    $('#node_id').on('keypress', function (e) {
        if (e.which === 32)
            return false;
    });


    $('.menu-item').click(function (e) {
        let menu = $(e.currentTarget).data('menu');
        toggleActive(menu);

    });
    wifiStatus();
    toggleActive("dashboard");
    setInterval(wifiStatus, 3000);
});
