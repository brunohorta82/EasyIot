<img src="https://github.com/brunohorta82/BH-Easy-Iot/blob/master/screenshots/logo.png " width="220">

* [Intro](#id1)
* [BH ONOFRE BOARD](https://github.com/brunohorta82/BH_OnOfre)
* [BH PZEM BOARD](https://github.com/brunohorta82/BH_PZEM_ESP8266)
* [Video Tutoriais - YouTube](https://www.youtube.com/watch?v=OZenBfHWtak&list=PLxDLawCWayzDqAgOpIDJ-DHFAXYd_S-pr)
* [Donativos](#id6)
* [Site Oficial](http://onofre.store/)
* [OnOfre Doctor](https://doctor.onofre.store)

## AP Password
bhonofre

## WEBPANEL User/Password
admin / xpto

## Intro <a name="id1"></a>
O projeto EasyIot, é o firmware oficial para todas as boards OnOfre, no entanto tambem é compativel com todas as boards baseadas em ESP8266/ESP32 tem  o objetivo de tornar a automação domiciliar muito mais simples, de forma aberta e sem restrições. Todo o código fonte é aberto.


## Painel de Controlo <a name="id3"></a>

![dash](https://github.com/brunohorta82/BH-Easy-Iot/blob/master/screenshots/system.png)
![features](https://github.com/brunohorta82/BH-Easy-Iot/blob/master/screenshots/features.png)
![han](https://github.com/brunohorta82/BH-Easy-Iot/blob/master/screenshots/han.png)


## Hardware <a name="id2"></a>
* OnOfre V6
![dev_board_6](https://github.com/brunohorta82/BH-Easy-Iot/blob/master/screenshots/v6.png)
* OnOfre Programador
![dev_board_6](https://github.com/brunohorta82/BH-Easy-Iot/blob/master/screenshots/prog-usbc.png)

## Software necessário <a name="id3"></a>

- Visual Studio Code e PlatformIO (caso seja para alterar o codigo fonte)
- Upload do Firmware para o ESP12(8266) / ESP32 o (dentro de cada projeto o firmware está dentro da pasta binarios)
- Aceder ao AP `SSID:ONFFRE_xxxx_xxx` password `bhonofre` e configurar a Rede Wi-Fi
- Navegar até  `http://<bh-onofre name>.local` ou `http://<bh-onofre IP address>` ou `http://192.168.4.1` ligado directamente ao módulo
- Configurar o `Wi-Fi broker`, `MQTT broker` entre outras coisas como integração automática com `Home Assistant`
- E tá feito, agora é só curtir :) 

## Verificações do projeto

Antes de criar um commit ou pull request, execute as verificações rápidas:

- `python3 tools/check_project.py --quick`

Para incluir uma compilação da configuração atual:

- `python3 tools/check_project.py --build ESP8266_DEBUG`

Consulte [docs/RELEASE_WORKFLOW.md](docs/RELEASE_WORKFLOW.md#project-checks)
para a matriz completa e os detalhes de cada verificação.

## Binários locais de firmware

Depois de compilar com PlatformIO, `tools/export_firmware.py` pode guardar o
binário como candidato local, verificá-lo com SHA-256 e, após teste na board,
preservá-lo como uma versão conhecida e funcional. A pasta gerada
`firmware_bins/` é local e ignorada pelo Git.

Os ambientes com `DEBUG` ou `RELEASE` no nome guardam automaticamente o seu
candidato no fim de uma compilação bem-sucedida.

Consulte [docs/RELEASE_WORKFLOW.md](docs/RELEASE_WORKFLOW.md#local-firmware-binaries)
para os comandos de publicação, verificação e promoção.


## Donativos <a name="id6"></a>

Se gostaste do projeto podes fazer o teu donativo :).

[![Donativo](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.me/bhonofre)

## API
h1. EasyIot Api This api allows you to control all your devices that have the EasyIot Firmware installed.

*Version:* 9.0.0
