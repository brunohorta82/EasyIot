let baseUrl= "http://192.168.187.135"
let config;
var newConfig={};
let source = null;
var WORDS_PT={}
var WORDS_RO={}
var WORDS_EN = {
    "pin-state-a":"Pin State A",
    "gate":"Gate",
    "security":"Security",
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
    "save": "Save All",
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
    "relay": "Relay",
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
    "group": "Group",
    "pin-in-c": "Input Pin c",
    "line": "Line",
    "member": "Member",
    "address": "Address",
    "prefix": "Prefix",
    "apikey": "API Key"
};
function loadsLanguage() {
    let lang = "PT";
    console.log(navigator.language);
    if (/^en/.test(navigator.language)) {
        lang = "EN";
    }else  if (/^ro/.test(navigator.language)) {
        lang = "RO";
    }
    $('span[class^="lang"]').each(function () {
        let langVar = (this.className).replace('lang-', '');
        let text = window['WORDS_' + lang][langVar];
        $(this).text(text);
    });
    $('option[class^="lang"]').each(function () {
        let langVar = (this.className).replace('lang-', '');
        let text = window['WORDS_' + lang][langVar];
        if (!text) {
            text = langVar;
        }
        $(this).text(text);
    });
}
function showMessage(pt, en) {
   // localStorage.getItem('lang').toString() === "PT" ? alert(pt) : alert(en);
}
function showText(pt, en) {
  //  return localStorage.getItem('lang').toString() === "PT" ? pt : en;
}
function loadConfig() {
    var targetUrl = baseUrl + "/config";
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
            showMessage("Erro a carregar configuração de funcionalidades.", "Features Configuration load failed.")
        },
        timeout: 2000
    });
}

function toggleActive(menu) {
    $('.onofre-menu').find('li').removeClass('active');
    $('.menu-item[data-menu="' + menu + '"]').closest('li').addClass('active');
    $(".content").load(menu + ".html", function () {
        if (menu === "devices") {
            fillDevices();
        } else {
            fillConfig();
        }
        loadsLanguage(localStorage.getItem('lang'));
    });
}


function fillGpioSelect(id) {
    addToSelect(id, "lang-none", 99);
    for (let gpio of config.pins) {
        addToSelect(id, "lang-" + gpio, gpio);
    }
}
function saveConfig() {
    let mqttIpDns=  document.getElementById("mqtt_ip");
    newConfig.nodeId =getValue("nodeId", config.nodeId).trim();
    newConfig.mqttIpDns = getValue("mqtt_ip", config.mqttIpDns).trim();
    newConfig.mqttUsername =getValue("mqtt_username", config.mqttUsername).trim();
    newConfig.mqttPassword = getValue("mqtt_password", config.mqttPassword).trim();
    newConfig.wifiSSID =getValue("ssid", config.wifiSSID).trim();
    newConfig.wifiSecret = getValue("wifi_secret", config.wifiSecret).trim();
    newConfig.wifiIp =getValue("wifiIp", config.wifiIp).trim();
    newConfig.wifiMask =getValue("wifiMask", config.wifiMask).trim();
    newConfig.wifiGw =getValue("wifiGw", config.wifiGw).trim();
    newConfig.dhcp =getValue("dhcp", config.dhcp) === "true";
    newConfig.accessPointPassword =getValue("accessPointPassword", config.accessPointPassword).trim();
    newConfig.apiPassword =getValue("apiPassword", config.apiPassword).trim();
    newConfig.apiUser =getValue("apiUser", config.apiUser).trim();

    const targetUrl = baseUrl + "/save-config";
    $.ajax({
        type: "POST",
        url: targetUrl,
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(newConfig),
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
function getValue(id, f){
    let v =  document.getElementById(id);
    return v ? v.value : f;
}
function fillDevices(){

}
function fillConfig() {
    if (!config) return;
    $(".bh-model").text(config.hardware);
    $(".bh-onofre-item").removeClass("hide");
    $("#version_lbl").text(config.firmware);
    $("#lbl-chip").text("ID: "+config.chipId);
    $("#lbl-mac").text("MAC: "+config.mac);
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
    $('input[name="accessPointPassword"]').val(config.accessPointPassword);
    $('input[name="apiPassword"]').val(config.apiPassword);
    $('input[name="apiUser"]').val(config.apiUser);
    $('input[name="emoncmsServer"]').val(config.emoncmsServer);
    $('input[name="emoncmsPath"]').val(config.emoncmsPath);
    $('input[name="emoncmsApikey"]').val(config.emoncmsApikey);
    $('#ssid_lbl').text(config.wifiSSID);
    let percentage = Math.min(2 * (parseInt(config.signal) + 100), 100);
    $('#wifi-signal').text(percentage + "%");
    $('#mqtt_lbl').text(config.mqttIpDns);
    if (config.mqttConnected) {
        $('#mqtt-state').text(showText("ligado", "connected"));
    } else {
        $('#mqtt-state').text(showText("desligado", "disconnected"));
    }
    $('#ff').prop('disabled', false);

}
function reboot() {
    $.ajax({
        url: baseUrl + "/reboot",
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
        url: baseUrl + "/load-defaults",
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

$(document).ready(function () {
    loadsLanguage();
    loadConfig();
    $('#node_id').on('keypress', function (e) {
        if (e.which === 32)
            return false;
    });
    $('.menu-item').click(function (e) {
        let menu = $(e.currentTarget).data('menu');
        toggleActive(menu);

    });
    toggleActive("node");
    if (!!window.EventSource) {
        source = new EventSource(baseUrl + '/events');
    }
});
