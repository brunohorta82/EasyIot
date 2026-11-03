#pragma once
// Addressable LED strip (WS2812/WS2813/WS2815...) with effects, driven over
// MQTT and discovered by Home Assistant. Only built with -D LED_STRIP; without
// the flag everything here is empty and the firmware is unchanged.
#include <Arduino.h>
#include <ArduinoJson.h>

/** Driver code the panel sends from "New feature" to create a strip. */
constexpr int kLedStripDriverCode = 200;

class AsyncWebServerRequest;

/** Does this firmware carry the strip? ESP32, ESP32-C3 and ESP32-C6 only; the ESP8266 has no memory for it. */
#ifdef LED_STRIP
constexpr bool ledStripSupported() { return true; }
#else
constexpr bool ledStripSupported() { return false; }
#endif

#ifdef LED_STRIP
void ledStripSetup();
void ledStripLoop();
/** Call from the MQTT callback. True when the topic belonged to the strip. */
bool ledStripHandleMqtt(const char *topic, const char *payload);
/** Subscribe the command topics. Call after connecting to the broker. */
void ledStripSubscribe();
/** Publica a descoberta do Home Assistant e o estado atual. */
void ledStripDiscovery();
/** Control page (HTML in PROGMEM) served at /led. */
const char *ledStripPageHtml();
size_t ledStripPageSize();
/** Current state, for GET /led/state. */
void ledStripStateJson(JsonVariant &root);
/** Leave a command for the main loop to apply. False when busy or too large. */
bool ledStripQueueCommand(JsonVariantConst json);
/** Create the strip feature. 0 = ok; 1 name, 2 bad pin, 5 pin taken, 7 already exists. */
int ledStripCreate(const char *name, int pin, int count);
/** Remove the feature; the main loop releases the strip itself. */
void ledStripRemove();
/** Is this pin the strip's? Keeps any other feature from claiming it. */
bool ledStripUsesPin(unsigned int pin);
/** KNX write from the bus. True when the address is the strip's; the main loop applies it. */
bool ledStripKnxWrite(uint8_t area, uint8_t line, uint8_t member, int value);
/** KNX read request. True, and fills state with 0/100, when the address is exactly the strip's. */
bool ledStripKnxRead(uint8_t area, uint8_t line, uint8_t member, int &state);
/** Add "ledStrip" to the /config JSON so the panel lists the feature. */
void ledStripConfigJson(JsonVariant root);
/** Build the strip's cloud topics from the credentials the cloud returned. */
void ledStripCloudTopics(const char *username, const char *chipId);
/** The strip's cloud command topic; empty while there is no strip or no credentials. */
const char *ledStripCloudWriteTopic();
/** Describe the strip to the cloud as a LIGHT feature with the LED_STRIP driver,
 *  so it is adopted as a colour light instead of being invisible. */
void ledStripFeatureJson(JsonArray features);
#else
inline bool ledStripUsesPin(unsigned int) { return false; }
inline void ledStripConfigJson(JsonVariant) {}
inline void ledStripCloudTopics(const char *, const char *) {}
inline const char *ledStripCloudWriteTopic() { return ""; }
inline void ledStripFeatureJson(JsonArray) {}
inline void ledStripSetup() {}
inline void ledStripLoop() {}
inline bool ledStripHandleMqtt(const char *, const char *) { return false; }
inline void ledStripSubscribe() {}
inline void ledStripDiscovery() {}
#endif
