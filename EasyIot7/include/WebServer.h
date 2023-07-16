#ifndef WEBSERVER_O_H
#define WEBSERVER_O_H
#include <Arduino.h>
const char HTTP_HEADER[] PROGMEM = "<!DOCTYPE html><html lang=\"pt\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>BH OnOfre</title>";
const char HTTP_STYLE[] PROGMEM = "<style>.st{font-size:12px;text-align:center;}.it{padding-top:2px;}.sc{font-size:10px;}input{padding:.175rem .75rem;font-size:14px;color:#fff;width:200px;margin-top:14px;background-color:transparent;background-clip:padding-box;border:1px solid rgba(255,255,255,.1)}input{width:235px}body{display:flex;color:#d8d8d8;justify-content:center;align-items:center;background-color:#1b1b1b;flex-flow:column;font-family:verdana,serif}h3{text-transform:uppercase;font-size:14px;padding-bottom:5px;display:flex;justify-content:space-between;flex-flow:row;align-items:end;border-bottom:1px solid rgba(255,255,255,.1)}.i{display:flex;flex-flow:column;background-color:#1b1b1b;font-family:verdana,serif}.t{background-color:#1b1b1b;padding:10px;width:100%;padding-left:43px;display:flex;align-items:center;color:#86bd9a}img{padding-right:10px}form{text-align:center;display:flex;flex-flow:column}a{color:#d8d8d8;font-size:12px;text-decoration:none}button{background-color:#88bf9c;border:none;height:30px;margin-left:71px;width:100px;cursor:pointer;border-radius:3px;font-size:12px;color:#f5f5f5;margin-top:20px;opacity:.9}</style>";
const char HTTP_SCRIPT[] PROGMEM = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
const char HTTP_HEADER_END[] PROGMEM = "</head><body><div class=\"t\"><span>OnOfre</span></div><h3 class='box-title'>Configuração da ligação Wi-Fi</h3><div class=\"i\">";
const char HTTP_ITEM[] PROGMEM = "<div class=\"it\"><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='{i}'>{r}%</span></div>";
const char HTTP_FORM_START[] PROGMEM = "<form method='get' action='/'> <div> <input maxlength='32' id='s' name='s'  placeholder='nome da tua rede Wi-Fi'/> <a href='/?sc'>pesquisar</a> </div><input id='p' name='p' maxlength='24' type='password' placeholder='palavra passe'/> <input maxlength='32' id='i' name='i' value='{n}' placeholder='nome do dispositivo'/><button type='submit'>Guardar</button></form>";
const char HTTP_SAVED[] PROGMEM = "<div class=\"st\">Configuração guardada<br />Verifica na tua Aplicação Móvel ou no teu Browser se já consegues aceder ao teu OnOfre <br/>Se não funcionar tenta novamente o processo de configuração.<br/><br/>Utiliza o endereço {o} para aceder no Browser</a></div>";
const char HTTP_END[] PROGMEM = "</div></body></html>";
void setupWebserverAsync();
void webserverServicesLoop();
void addSwitchToAlexa(const char *name);
void removeSwitchFromAlexa(const char *name);
void sendToServerEvents(const String &topic, const char *payload);
#endif