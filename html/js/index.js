let baseUrl = "http://192.168.122.134"
let config;
var newConfig = {};
let source = null;
var WORDS_PT = {}
var WORDS_RO = {}
var WORDS_EN = {
    "pin-state-a": "Pin State A",
    "gate": "Gate",
    "security": "Security",
    "pin-state-b": "Pin State B",
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
function stringToHTML (text) {
    let parser = new DOMParser();
    let doc = parser.parseFromString(text, 'text/html');
    return doc.body;
}
function toggleActive(menu) {
  //  $('.onofre-menu').find('li').removeClass('active');
    //$('.menu-item[data-menu="' + menu + '"]').closest('li').addClass('active');
 /*   document.getElementsByClassName("content").item(0)load(menu + ".html", function () {
        if (menu === "devices") {
           // fillDevices();
        } else {
            fillConfig();
        }
     //   loadsLanguage(localStorage.getItem('lang'));
    });*/
    fetch(menu + ".html").then(function (response) {
        if (response.ok) {
            return response.text();
        }
        throw response;
    }).then(function (text) {
        let dialog = document.getElementsByClassName("content").item(0);
        let html = stringToHTML(text);
        dialog.append( html);
        fillConfig();
    });

}
function findById(id){
 return    document.getElementById(id);
}
function fillConfig() {
    if (!config) return;
    document.title = 'BH OnOfre ' + config.nodeId;
    let percentage = Math.min(2 * (parseInt(config.signal) + 100), 100);
    findById("version_lbl").textContent =config.firmware;
    findById("lbl-chip").textContent ="ID: " + config.chipId;
    findById("lbl-mac").textContent ="MAC: " + config.mac;
    findById("ssid_lbl").textContent =config.wifiSSID;
    findById("mqtt_lbl").textContent =config.mqttIpDns;
    if (config.mqttConnected) {
       //TODO ICON
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

    /*


    $('#wifi-signal').text(percentage + "%");
    $('#mqtt_lbl').text(config.mqttIpDns);
    if (config.mqttConnected) {
        $('#mqtt-state').text(showText("ligado", "connected"));
    } else {
        $('#mqtt-state').text(showText("desligado", "disconnected"));
    }

*/
}

async function loadConfig() {
    const response = await fetch(baseUrl + "/config");
    config = await response.json();
    fillConfig();
}

/*
function loadsLanguage() {
    let lang = "PT";
    if (/^en/.test(navigator.language)) {
        lang = "EN";
    } else if (/^ro/.test(navigator.language)) {
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




function fillGpioSelect(id) {
    addToSelect(id, "lang-none", 99);
    for (let gpio of config.pins) {
        addToSelect(id, "lang-" + gpio, gpio);
    }
}

function saveConfig() {
    let mqttIpDns = document.getElementById("mqtt_ip");
    newConfig.nodeId = getValue("nodeId", config.nodeId).trim();
    newConfig.mqttIpDns = getValue("mqtt_ip", config.mqttIpDns).trim();
    newConfig.mqttUsername = getValue("mqtt_username", config.mqttUsername).trim();
    newConfig.mqttPassword = getValue("mqtt_password", config.mqttPassword).trim();
    newConfig.wifiSSID = getValue("ssid", config.wifiSSID).trim();
    newConfig.wifiSecret = getValue("wifi_secret", config.wifiSecret).trim();
    newConfig.wifiIp = getValue("wifiIp", config.wifiIp).trim();
    newConfig.wifiMask = getValue("wifiMask", config.wifiMask).trim();
    newConfig.wifiGw = getValue("wifiGw", config.wifiGw).trim();
    newConfig.dhcp = getValue("dhcp", config.dhcp) === "true";
    newConfig.accessPointPassword = getValue("accessPointPassword", config.accessPointPassword).trim();
    newConfig.apiPassword = getValue("apiPassword", config.apiPassword).trim();
    newConfig.apiUser = getValue("apiUser", config.apiUser).trim();

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

function getValue(id, f) {
    let v = document.getElementById(id);
    return v ? v.value : f;
}

function toggleSwitch(arg) {
    console.log(arg.id)
    console.log(arg.checked)
    arg.parentNode.parentNode.getElementsByTagName("svg").item(0).classList = {};
    arg.parentNode.parentNode.getElementsByTagName("svg").item(0).classList.add(arg.checked  ? "feature-icon-on" : "feature-icon-off");
}

function fillDevices() {
    let temp, item, a;
    temp = document.getElementsByTagName("template")[0];
    item = temp.content.querySelector("div");
    var modal = document.getElementById("myModal");
    for (const f of config.features) {
        a = document.importNode(item, true);
        a.getElementsByClassName("feature-name").item(0).textContent = f.name;
        a.getElementsByTagName("svg").item(0).classList.add(f.state === 1 ? "feature-icon-on" : "feature-icon-off");
        a.getElementsByTagName("input").item(0).checked = f.state === 1;
        a.getElementsByTagName("input").item(0).id = f.id;
        document.getElementById("devices_config").appendChild(a);
        a.getElementsByClassName("feature-name").item(0).onclick = function() {
            modal.style.display = "block";
            modal.getElementsByClassName("f-name").item(0).textContent = f.name;
            modal.getElementsByClassName("f-name").item(1).value = f.name;

        }
    }
    const span = document.getElementsByClassName("close")[0];
    span.onclick = function() {
        modal.style.display = "none";

    }
    window.onclick = function(event) {
        if (event.target === modal) {
            modal.style.display = "none";
        }
    }
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
}*/

document.addEventListener("DOMContentLoaded", () => {
    //loadsLanguage();
    loadConfig();
    toggleActive("node");
    if (!!window.EventSource) {
        source = new EventSource(baseUrl + '/events');
    }
});
