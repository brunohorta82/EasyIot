#pragma once
// Fita LED enderecável (WS2812/WS2813/WS2815...) com efeitos, controlada por
// MQTT e descoberta automática no Home Assistant. Só compila com -D LED_STRIP.
// Sem a flag, tudo aqui são funções vazias e o firmware fica igual ao original.
#include <Arduino.h>
#include <ArduinoJson.h>

/** Código de driver que o painel envia em "Nova função" para criar uma fita LED. */
constexpr int kLedStripDriverCode = 200;

class AsyncWebServerRequest;

/** Este firmware traz a fita LED? (Só os ESP32, ESP32-C3 e ESP32-C6; o ESP8266 não tem memória para ela.) */
#ifdef LED_STRIP
constexpr bool ledStripSupported() { return true; }
#else
constexpr bool ledStripSupported() { return false; }
#endif

#ifdef LED_STRIP
void ledStripSetup();
void ledStripLoop();
/** Chamar no callback MQTT. Devolve true se o tópico era da fita. */
bool ledStripHandleMqtt(const char *topic, const char *payload);
/** Subscreve os tópicos de comando. Chamar depois de ligar ao broker. */
void ledStripSubscribe();
/** Publica a descoberta do Home Assistant e o estado atual. */
void ledStripDiscovery();
/** Página de controlo (HTML em PROGMEM) servida em /led. */
const char *ledStripPageHtml();
size_t ledStripPageSize();
/** Estado atual para GET /led/state. */
void ledStripStateJson(JsonVariant &root);
/** Deixa um comando para o loop principal aplicar. False se ocupado ou grande demais. */
bool ledStripQueueCommand(JsonVariantConst json);
/** Cria a função Fita LED. 0 = ok; 1 nome, 2 pino inválido, 5 pino ocupado, 7 já existe. */
int ledStripCreate(const char *name, int pin, int count);
/** Remove a função (a fita é libertada pelo loop principal). */
void ledStripRemove();
/** O pino é da fita? Usado para que nenhuma outra função o possa escolher. */
bool ledStripUsesPin(unsigned int pin);
/** KNX: escrita recebida no bus. True se o endereço é da fita (o loop principal aplica). */
bool ledStripKnxWrite(uint8_t area, uint8_t line, uint8_t member, int value);
/** KNX: pedido de leitura. True e preenche o estado (0/100) se o endereço é exatamente o da fita. */
bool ledStripKnxRead(uint8_t area, uint8_t line, uint8_t member, int &state);
/** Acrescenta "ledStrip" ao JSON de /config, para o painel listar a função. */
void ledStripConfigJson(JsonVariant root);
/** Build the strip's cloud topics from the credentials the cloud returned. */
void ledStripCloudTopics(const char *username, const char *chipId);
/** The strip's cloud command topic; empty while there is no strip or no credentials. */
const char *ledStripCloudWriteTopic();
/** Describe the strip to the cloud as a LIGHT feature with the LED_STRIP driver,
 *  so it is adopted as a colour light instead of being invisible. */
void ledStripFeatureJson(JsonArray features, bool allFields);
#else
inline bool ledStripUsesPin(unsigned int) { return false; }
inline void ledStripConfigJson(JsonVariant) {}
inline void ledStripCloudTopics(const char *, const char *) {}
inline const char *ledStripCloudWriteTopic() { return ""; }
inline void ledStripFeatureJson(JsonArray, bool) {}
inline void ledStripSetup() {}
inline void ledStripLoop() {}
inline bool ledStripHandleMqtt(const char *, const char *) { return false; }
inline void ledStripSubscribe() {}
inline void ledStripDiscovery() {}
#endif
