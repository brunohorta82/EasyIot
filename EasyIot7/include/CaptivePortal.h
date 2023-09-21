#pragma once
#include <Arduino.h>

const char HTTP_HEADER[] PROGMEM = "<!DOCTYPE html><html lang=\"pt\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>BH OnOfre</title>";
const char HTTP_STYLE[] PROGMEM = "<style>.hide{display:none;}.st{font-size:12px;text-align:center;}.it{padding-top:2px;}.sc{font-size:10px;}input{padding:.175rem .75rem;font-size:14px;color:#fff;width:200px;margin-top:14px;background-color:transparent;background-clip:padding-box;border:1px solid rgba(255,255,255,.1)}input{width:235px}body{display:flex;color:#d8d8d8;justify-content:center;align-items:center;background-color:#1b1b1b;flex-flow:column;font-family:verdana,serif}h3{text-transform:uppercase;font-size:14px;padding-bottom:5px;display:flex;justify-content:space-between;flex-flow:row;align-items:end;border-bottom:1px solid rgba(255,255,255,.1)}.i{display:flex;flex-flow:column;background-color:#1b1b1b;font-family:verdana,serif}.t{background-color:#1b1b1b;padding:10px;width:100%;padding-left:43px;display:flex;align-items:center;color:#86bd9a}img{padding-right:10px}form{display:flex;flex-flow:column}a{color:#d8d8d8;font-size:12px;text-decoration:none}button{background-color:#88bf9c;border:none;height:30px;margin-left:115px;width:100px;cursor:pointer;border-radius:3px;font-size:12px;color:#f5f5f5;margin-top:20px;opacity:.9}</style>";
const char HTTP_SCRIPT[] PROGMEM = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
#ifdef CONFIG_LANG_PT
const char HTTP_HEADER_END[] PROGMEM = "</head><body><div class=\"t\"><span>OnOfre</span></div><h3 class='box-title'>Configuração</h3><div class=\"i\">";
#elif CONFIG_LANG_EN
const char HTTP_HEADER_END[] PROGMEM = "</head><body><div class=\"t\"><span>OnOfre</span></div><h3 class='box-title'>Configuration</h3><div class=\"i\">";
#elif CONFIG_LANG_RO
const char HTTP_HEADER_END[] PROGMEM = "</head><body><div class=\"t\"><span>OnOfre</span></div><h3 class='box-title'>>Configurare</h3><div class=\"i\">";
#endif
const char HTTP_ITEM[] PROGMEM = "<div class=\"it\"><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='{i}'>{r}%</span></div>";
#ifdef CONFIG_LANG_PT
const char HTTP_FORM_START[] PROGMEM = "<form method='get' action='/'> <div> <input maxlength='32' id='s' name='s'  placeholder='WiFi'/> <a href='/?sc'>pesquisar</a> </div><input id='p' name='p' maxlength='24' type='password' placeholder='palavra passe'/><label for='i' style='margin: 10px 0px 0px 0px;'>Nome</label><input maxlength='32' id='i' name='i' value='{n}'/><label for='t' class='hide' style='margin: 10px 0px 10px 0px;'>Template</label><select  class='hide' style='min-height: 24px;' name='t' id='t'><option value='0' >Nenhum</option><option value='1'>Iluminação</option><option value='2'>Estores</option><option value='3'>Portão</option><option value='4'>Pzem</option></select><button type='submit'>Guardar</button></form>";
#elif CONFIG_LANG_EN
const char HTTP_FORM_START[] PROGMEM = "<form method='get' action='/'> <div> <input maxlength='32' id='s' name='s'  placeholder='WiFi'/> <a href='/?sc'>search</a> </div><input id='p' name='p' maxlength='24' type='password' placeholder='password'/><label for='i' style='margin: 10px 0px 0px 0px;'>Name</label><input maxlength='32' id='i' name='i' value='{n}'/><label for='t' style='margin: 10px 0px 10px 0px;'>Template</label><select style='min-height: 24px;' name='t' id='t'><option value='0' >None</option><option value='1'>Lights</option><option value='2'>Covers</option><option value='3'>Gate</option><option value='4'>Pzem</option></select><button type='submit'>Save</button></form>";
#elif CONFIG_LANG_RO
const char HTTP_FORM_START[] PROGMEM = "<form method='get' action='/'> <div> <input maxlength='32' id='s' name='s'  placeholder='Numele retelei dvs. Wi-Fi'/> <a href='/?sc'>Cautare</a> </div><input id='p' name='p' maxlength='24' type='password' placeholder='Parola'/><label for='i' style='margin: 10px 0px 0px 0px;'>Numele dispozitivului</label><input maxlength='32' id='i' name='i' value='{n}'/><label for='t' style='margin: 10px 0px 10px 0px;'>Template</label><select style='min-height: 24px;' name='t' id='t'><option value='0' >None</option><option value='1'>Lights</option><option value='2'>Covers</option><option value='3'>Gate</option><option value='4'>Pzem</option></select><button type='submit'>Salvati</button></form>";

#endif
#ifdef CONFIG_LANG_PT
const char HTTP_SAVED[] PROGMEM = "<div class=\"st\">Configuração guardada<br />Verifica na tua Aplicação Móvel ou no teu Browser se já consegues aceder ao teu OnOfre <br/>Se não funcionar tenta novamente o processo de configuração.<br/><br/>Utiliza o endereço {o} para aceder no Browser</a></div>";
#elif CONFIG_LANG_EN
const char HTTP_SAVED[] PROGMEM = "<div class=\"st\">Configuration Saved<br />Check in your Mobile Application or in your Browser if you can already access your OnOfre<br/>If that doesn't work, try the setup process again..<br/><br/>Use the address {o} to access in the browser</a></div>";
#elif CONFIG_LANG_RO
const char HTTP_SAVED[] PROGMEM = "<div class=\"st\">Configurarea a fost salvata<br />Verificati aplicatia Mobila sau in browserul dvs. daca va puteti deja conecta la OnOfre<br/>Daca nu functioneaza, încercati din nou procesul de configurare.<br/><br/>Utilizati adresa {o} pentru a accesa in browser</a></div>";
#endif
const char HTTP_END[] PROGMEM = "</div></body></html>";