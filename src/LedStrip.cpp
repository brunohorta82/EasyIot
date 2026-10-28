#ifdef LED_STRIP
#include "LedStrip.h"
#include "LedEffects.h"
#include "ConfigOnofre.h"
#include "Mqtt.h"
#include "Persistence.h"
#include "LittleFS.h"
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <esp-knx-ip.h>
#include <esp_system.h>
#include "CoreWiFi.h"
#include "DeviceLog.h"
#include "WebServer.h"
#include "CloudIO.h"

// ---- Build-time configuration (platformio.ini / platformio_override.ini) ----
#ifndef LED_STRIP_PIN
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
#define LED_STRIP_PIN 4 // a fallback only: the real pin is chosen in "New feature" (on the C3, 18/19 are USB)
#else
#define LED_STRIP_PIN 18 // a fallback only: the real pin is chosen in "New feature"
#endif
#endif
#ifndef LED_STRIP_COUNT
#define LED_STRIP_COUNT 60
#endif
#ifndef LED_STRIP_MAX
#define LED_STRIP_MAX 600
#endif

extern ConfigOnofre config;

namespace
{
    const char *kFile = "/led.json";
    const char *kTmp = "/led.tmp";
    constexpr unsigned long kSaveDelayMs = 3000;  // spares the flash: write only once the fiddling stops
    constexpr unsigned long kInitDelayMs = 8000;  // at boot the strip wakes only after everything settles
    constexpr unsigned long kCreateDelayMs = 500; // when created from the panel, almost at once

    // The strip feature created from New feature. Creating and removing touches only
    // these fields and the file; the hardware is always the main loop's business
    // (the reconcile step in ledStripLoop).
    volatile bool created = false;
    char fname[24] = "Fita LED";
    int fpin = LED_STRIP_PIN;

    // KNX: one group address (area/line/member) for on and off, reporting state back,
    // as the lights do. 0/0/0 = no KNX. Values as elsewhere: 0 = off, 100 = on.
    uint8_t knxAddr[3] = {0, 0, 0};
    volatile int knxPending = -1; // request from the KNX bus, applied in the main loop
    int lastKnxSent = 0;
    bool knxOn() { return knxAddr[0] > 0; }
    void knxAssign()
    {
        if (!knxOn())
            return;
        knx.callback_assign(config.knxIdRegister, knx.GA_to_address(knxAddr[0], knxAddr[1], knxAddr[2]));
        knx.callback_assign(config.knxIdRegister, knx.GA_to_address(knxAddr[0], knxAddr[1], 0));
        knx.callback_assign(config.knxIdRegister, knx.GA_to_address(knxAddr[0], 0, 0));
    }

    // Whole-strip state: master switch and brightness, and the LED count.
    struct State
    {
        bool on = false;
        uint8_t brightness = 255;
        uint16_t count = LED_STRIP_COUNT;
        uint16_t transition = 400; // ms to fade on, off and between colours (0 = no fade)
        uint16_t limitMA = 0;      // supply current limit in mA (0 = no limit)
        uint16_t mAperLED = 60;    // consumo de um LED a branco, a fundo
        uint8_t colorOrder = 0;    // index into kOrders (0 = GRB, usual for WS2812B)
        uint8_t gamma = 10;        // gamma in tenths (10 = none, 22 = 2.2, 28 = 2.8)
        uint8_t cal[3] = {100, 100, 100}; // white balance: per cent of red, green and blue
    } st;

    // Colour byte orders strips use. When "red" comes out green or blue, change it here.
    const neoPixelType kOrders[6] = {NEO_GRB, NEO_RGB, NEO_BRG, NEO_RBG, NEO_GBR, NEO_BGR};

    // Gamma table: the eye sees light non-linearly and the LEDs are linear, so without
    // this faint colours look washed out and a red with a little green or blue turns pink.
    uint8_t gammaLut[256];
    uint8_t gammaBuilt = 0;
    void buildGamma()
    {
        if (gammaBuilt == st.gamma)
            return;
        for (int i = 0; i < 256; i++)
            gammaLut[i] = st.gamma <= 10 ? (uint8_t)i : (uint8_t)(powf(i / 255.f, st.gamma / 10.f) * 255.f + 0.5f);
        gammaBuilt = st.gamma;
    }

    // The values on show during a fade; never persisted.
    float mLvl = 0.f;                 // 0..1: a fita toda a acender/apagar
    float mBri = 255.f;               // brilho geral suavizado
    float segLvl[8] = {1, 1, 1, 1, 1, 1, 1, 1}; // 0..1 as each segment fades in or out
    unsigned long lastRenderMs = 0;
    uint32_t estMA = 0;               // estimated current of the last frame sent, limit included
    bool limiting = false;            // the limit is holding the light back

    // Segments (zones of the strip), as in WLED: each with its own LED range, colour,
    // effect and so on. There is always at least one; segment 0 covers the whole strip.
    constexpr int kMaxSeg = 8;
    struct Seg
    {
        uint16_t start = 0, stop = 0; // LEDs start..stop, inclusive
        bool on = true;
        uint8_t bri = 255;
        led::RGB color{255, 160, 60};
        uint8_t effect = led::FX_STATIC;
        uint8_t speed = 128;
        uint8_t intensity = 128;
    };
    Seg segs[kMaxSeg];
    int segCount = 1;
    led::Engine engines[kMaxSeg]; // um motor de efeitos por segmento
    std::vector<led::RGB> frame;  // imagem final da fita
    bool frameDirty = true;

    // Resize the engines to the segments, after the segment layout changes.
    void rebuildEngines()
    {
        for (int i = 0; i < segCount; i++)
        {
            const Seg &g = segs[i];
            engines[i].resize(g.stop - g.start + 1);
            engines[i].setTransition(st.transition);
            engines[i].set((led::Effect)g.effect, g.color, g.speed, g.intensity);
        }
        frame.assign(st.count, led::RGB());
        frameDirty = true;
    }

    // Push only the properties (effect, colour, speed, intensity) into the engines.
    void syncEngines()
    {
        for (int i = 0; i < segCount; i++)
        {
            engines[i].setTransition(st.transition);
            engines[i].set((led::Effect)segs[i].effect, segs[i].color, segs[i].speed, segs[i].intensity);
        }
        frameDirty = true;
    }

    // Keep the segments inside the strip after the LED count changes.
    void fitSegments()
    {
        const int n = st.count;
        int keep = 0;
        for (int i = 0; i < segCount; i++)
        {
            if (segs[i].start >= n)
                continue;
            if (segs[i].stop >= n)
                segs[i].stop = n - 1;
            if (keep != i)
                segs[keep] = segs[i];
            keep++;
        }
        if (keep == 0)
        {
            segs[0] = Seg();
            segs[0].stop = n - 1;
            keep = 1;
        }
        segCount = keep;
    }

    Adafruit_NeoPixel *strip = nullptr;
    unsigned long initAfter = 0;
    bool needsSave = false, wasOn = false, needsPublish = false;
    bool saveNow = false; // settings, not slider moves: write at once so a reboot cannot lose them
    bool needsDiscovery = false, needsUndiscovery = false;
    bool testRequested = false;
    unsigned long lastChange = 0;
    char tCmd[64], tState[64], tSpeed[64], tSpeedSet[64], tInt[64], tIntSet[64];
    // Cloud topics follow the actuator shape: <user>/<chip>/light/<id>/set and /status.
    // Empty until the cloud hands over credentials, which is also the "do not publish" flag.
    char tCloudCmd[160] = {0}, tCloudState[160] = {0};

    void topics()
    {
        sprintf(tCmd, "onofre/%s/led/set", config.chipId);
        sprintf(tState, "onofre/%s/led/state", config.chipId);
        sprintf(tSpeed, "onofre/%s/led/speed", config.chipId);
        sprintf(tSpeedSet, "onofre/%s/led/speed/set", config.chipId);
        sprintf(tInt, "onofre/%s/led/intensity", config.chipId);
        sprintf(tIntSet, "onofre/%s/led/intensity/set", config.chipId);
    }

    void touch()
    {
        needsSave = true;
        lastChange = millis();
        syncEngines();
    }

    void load()
    {
        File f = LittleFS.open(kFile, "r");
        if (!f)
            return;
        JsonDocument doc;
        if (deserializeJson(doc, f) == DeserializationError::Ok)
        {
            st.on = doc["on"] | false;
            st.brightness = doc["bri"] | 255;
            uint16_t n = doc["n"] | LED_STRIP_COUNT;
            st.count = (n >= 1 && n <= LED_STRIP_MAX) ? n : LED_STRIP_COUNT;
            st.transition = constrain((int)(doc["tr"] | 400), 0, 3000);
            st.limitMA = constrain((int)(doc["lm"] | 0), 0, 20000);
            st.mAperLED = constrain((int)(doc["lp"] | 60), 10, 100);
            st.colorOrder = constrain((int)(doc["co"] | 0), 0, 5);
            st.gamma = constrain((int)(doc["gm"] | 10), 10, 40);
            st.cal[0] = constrain((int)(doc["cr"] | 100), 10, 100);
            st.cal[1] = constrain((int)(doc["cg"] | 100), 10, 100);
            st.cal[2] = constrain((int)(doc["cb"] | 100), 10, 100);
            if (doc["sg"].is<JsonArray>() && doc["sg"].size() > 0)
            {
                // segments: [start, stop, on, brightness, r, g, b, effect, speed, intensity]
                segCount = 0;
                for (JsonVariant v : doc["sg"].as<JsonArray>())
                {
                    if (segCount >= kMaxSeg)
                        break;
                    JsonArray a = v.as<JsonArray>();
                    Seg &g = segs[segCount++];
                    g.start = a[0] | 0;
                    g.stop = a[1] | 0;
                    g.on = a[2] | true;
                    g.bri = a[3] | 255;
                    g.color = led::RGB(a[4] | 255, a[5] | 160, a[6] | 60);
                    uint8_t fx = a[7] | 0;
                    g.effect = fx < led::FX_COUNT ? fx : 0;
                    g.speed = a[8] | 128;
                    g.intensity = a[9] | 128;
                }
            }
            else
            {
                // a file from a version without segments: one segment covering everything
                segCount = 1;
                segs[0] = Seg();
                segs[0].stop = st.count - 1;
                segs[0].color = led::RGB(doc["r"] | 255, doc["g"] | 160, doc["b"] | 60);
                uint8_t fx = doc["fx"] | 0;
                segs[0].effect = fx < led::FX_COUNT ? fx : 0;
                segs[0].speed = doc["sx"] | 128;
                segs[0].intensity = doc["ix"] | 128;
            }
            fitSegments();
            fpin = doc["pin"] | LED_STRIP_PIN;
            strlcpy(fname, doc["name"] | "Fita LED", sizeof(fname));
            created = doc["created"] | false;
            knxAddr[0] = doc["ka"] | 0;
            knxAddr[1] = doc["kl"] | 0;
            knxAddr[2] = doc["km"] | 0;
        }
        f.close();
    }

    void save()
    {
        JsonDocument doc;
        doc["created"] = (bool)created;
        doc["name"] = fname;
        doc["pin"] = fpin;
        doc["on"] = st.on;
        doc["bri"] = st.brightness;
        doc["n"] = st.count;
        doc["tr"] = st.transition;
        doc["lm"] = st.limitMA;
        doc["lp"] = st.mAperLED;
        doc["co"] = st.colorOrder;
        doc["gm"] = st.gamma;
        doc["cr"] = st.cal[0];
        doc["cg"] = st.cal[1];
        doc["cb"] = st.cal[2];
        JsonArray sg = doc["sg"].to<JsonArray>();
        for (int i = 0; i < segCount; i++)
        {
            const Seg &g = segs[i];
            JsonArray a = sg.add<JsonArray>();
            a.add(g.start);
            a.add(g.stop);
            a.add(g.on);
            a.add(g.bri);
            a.add(g.color.r);
            a.add(g.color.g);
            a.add(g.color.b);
            a.add(g.effect);
            a.add(g.speed);
            a.add(g.intensity);
        }
        doc["ka"] = knxAddr[0];
        doc["kl"] = knxAddr[1];
        doc["km"] = knxAddr[2];
        persistJsonAtomically(kFile, kTmp, doc);
    }

    void publishState()
    {
        JsonDocument doc;
        doc["state"] = st.on ? "ON" : "OFF";
        doc["brightness"] = st.brightness;
        doc["color_mode"] = "rgb";
        JsonObject c = doc["color"].to<JsonObject>();
        c["r"] = segs[0].color.r;
        c["g"] = segs[0].color.g;
        c["b"] = segs[0].color.b;
        doc["effect"] = led::kEffectNames[segs[0].effect];
        String out;
        serializeJson(doc, out);
        publishOnMqtt(tState, out.c_str(), true);
        if (tCloudState[0] != '\0')
            notifyStateToCloudIO(tCloudState, out.c_str());
        publishOnMqtt(tSpeed, String(segs[0].speed).c_str(), true);
        publishOnMqtt(tInt, String(segs[0].intensity).c_str(), true);
    }

    int findEffect(const char *name)
    {
        for (int i = 0; i < led::FX_COUNT; i++)
            if (strcasecmp(name, led::kEffectNames[i]) == 0)
                return i;
        return -1;
    }

    void applyCount(uint16_t n)
    {
        if (n < 1 || n > LED_STRIP_MAX || n == st.count)
            return;
        st.count = n;
        if (strip)
        {
            strip->clear();
            strip->show();
            strip->updateLength(n);
        }
        if (segCount == 1)
        {
            segs[0].start = 0;
            segs[0].stop = n - 1; // with a single segment, it follows the strip
        }
        else
            fitSegments();
        rebuildEngines();
    }

    // Set the segment layout. New ones inherit the first one's properties.
    void applySegments(JsonArray arr)
    {
        Seg tmp[kMaxSeg];
        int n = 0;
        for (JsonVariant v : arr)
        {
            if (n >= kMaxSeg)
                break;
            int a = constrain((int)(v["start"] | 0), 0, st.count - 1);
            int b = constrain((int)(v["stop"] | 0), 0, st.count - 1);
            if (b < a)
            {
                const int x = a;
                a = b;
                b = x;
            }
            Seg g = (n < segCount) ? segs[n] : segs[0];
            g.start = a;
            g.stop = b;
            tmp[n++] = g;
        }
        if (n == 0)
            return;
        for (int i = 0; i < n; i++)
            segs[i] = tmp[i];
        segCount = n;
        for (int i = 0; i < kMaxSeg; i++)
            segLvl[i] = (i < segCount && segs[i].on) ? 1.f : 0.f;
        rebuildEngines();
    }

    // Apply a command, JSON or plain "ON"/"OFF". Serves both MQTT and the web page.
    void applyCommand(const char *payload)
    {
        if (strcasecmp(payload, "ON") == 0 || strcasecmp(payload, "OFF") == 0)
        {
            st.on = strcasecmp(payload, "ON") == 0;
            touch();
            return;
        }
        JsonDocument doc;
        if (deserializeJson(doc, payload) != DeserializationError::Ok)
            return;
        // Colour, brightness, speed and intensity are dragged on sliders and written
        // late, to spare the flash. The rest are settings: written at once, or a reboot
        // right afterwards would lose them.
        if (doc["leds"].is<int>() || doc["segments"].is<JsonArray>() ||
            doc["colorOrder"].is<int>() || doc["gamma"].is<int>() || doc["transition"].is<int>() ||
            doc["limit"].is<int>() || doc["maPerLed"].is<int>() || doc["knx"].is<JsonObject>())
            saveNow = true;
        // "seg": which segment the command is aimed at; without it, all of them.
        int t = doc["seg"] | -1;
        if (t < 0 || t >= segCount)
            t = -1;
        const int first = (t < 0) ? 0 : t, last = (t < 0) ? segCount - 1 : t;
        if (doc["state"].is<const char *>())
        {
            const bool v = strcasecmp(doc["state"] | "ON", "ON") == 0;
            if (t < 0)
                st.on = v;
            else
            {
                segs[t].on = v;
                if (v)
                    st.on = true; // ligar um segmento liga a fita
            }
        }
        if (doc["brightness"].is<int>())
        {
            const uint8_t v = constrain((int)doc["brightness"], 0, 255);
            if (t < 0)
                st.brightness = v;
            else
                segs[t].bri = v;
        }
        for (int i = first; i <= last; i++)
        {
            if (doc["color"].is<JsonObject>())
                segs[i].color = led::RGB(doc["color"]["r"] | 0, doc["color"]["g"] | 0, doc["color"]["b"] | 0);
            if (doc["effect"].is<const char *>())
            {
                const int fx = findEffect(doc["effect"]);
                if (fx >= 0)
                    segs[i].effect = fx;
            }
            if (doc["speed"].is<int>())
                segs[i].speed = constrain((int)doc["speed"], 0, 255);
            if (doc["intensity"].is<int>())
                segs[i].intensity = constrain((int)doc["intensity"], 0, 255);
        }
        if (doc["leds"].is<int>())
            applyCount((uint16_t)constrain((int)doc["leds"], 1, LED_STRIP_MAX));
        if (doc["segments"].is<JsonArray>())
            applySegments(doc["segments"].as<JsonArray>());
        if (doc["transition"].is<int>())
            st.transition = constrain((int)doc["transition"], 0, 3000);
        if (doc["limit"].is<int>())
            st.limitMA = constrain((int)doc["limit"], 0, 20000); // mA; 0 = no limit
        if (doc["maPerLed"].is<int>())
            st.mAperLED = constrain((int)doc["maPerLed"], 10, 100);
        if (doc["colorOrder"].is<int>())
        {
            st.colorOrder = constrain((int)doc["colorOrder"], 0, 5);
            if (strip)
                strip->updateType(kOrders[st.colorOrder] + NEO_KHZ800); // no restart needed
            frameDirty = true;
        }
        if (doc["gamma"].is<int>())
            st.gamma = constrain((int)doc["gamma"], 10, 40);
        if (doc["cal"].is<JsonArray>() && doc["cal"].size() == 3)
            for (int i = 0; i < 3; i++)
                st.cal[i] = constrain((int)doc["cal"][i], 10, 100);
        if (doc["test"] | false)
            testRequested = true;
        if (doc["knx"].is<JsonObject>())
        {
            knxAddr[0] = constrain((int)(doc["knx"]["area"] | 0), 0, 31);
            knxAddr[1] = constrain((int)(doc["knx"]["line"] | 0), 0, 7);
            knxAddr[2] = constrain((int)(doc["knx"]["member"] | 0), 0, 255);
            knxAssign();
            lastKnxSent = st.on ? 100 : 0;
        }
        touch();
    }

    // Commands from the web page: the async handler only leaves them here and the main
    // loop applies them, so the strip is never touched from another task.
    char pendingBuf[640];
    volatile bool pendingFlag = false;

    void haDevice(JsonObject &o)
    {
        JsonObject d = o["dev"].to<JsonObject>();
        d["ids"] = "OnOfre-" + String(config.chipId);
        d["name"] = config.nodeId;
        d["mf"] = "OnOfre Portugal";
        d["sw"] = String(VERSION);
    }

    void publishConfig(const char *domain, const String &uid, JsonDocument &doc)
    {
        String out;
        serializeJson(doc, out);
        publishOnMqtt((String("homeassistant/") + domain + "/" + uid + "/config").c_str(), out.c_str(), false);
    }

    void selfTest(); // definida mais abaixo

    void initStrip()
    {
#ifdef DEBUG_ONOFRE
        Log.notice("[LED] 1/3 criar objeto (pino %d, %d LEDs)" CR, fpin, st.count);
#endif
        strip = new Adafruit_NeoPixel(st.count, fpin, kOrders[st.colorOrder] + NEO_KHZ800);
#ifdef DEBUG_ONOFRE
        Log.notice("[LED] 2/3 begin()" CR);
#endif
        strip->begin();
#ifdef DEBUG_ONOFRE
        Log.notice("[LED] 3/3 primeiro show()" CR);
#endif
        strip->clear();
        strip->show();
        rebuildEngines();
        mLvl = st.on ? 1.f : 0.f; // at boot the strip comes straight back as it was
        mBri = st.brightness;
        for (int i = 0; i < kMaxSeg; i++)
            segLvl[i] = (i < segCount && segs[i].on) ? 1.f : 0.f;
        selfTest();
        needsDiscovery = true;
        needsPublish = true;
        lastKnxSent = st.on ? 100 : 0;
#ifdef DEBUG_ONOFRE
        Log.notice("[LED] fita pronta" CR);
#endif
    }

    // Teste de hardware: a fita toda a vermelho, verde e azul, a brilho baixo. Se isto acende, o sinal,
    // ground and power are sound, and anything wrong after that is configuration.
    void selfTest()
    {
        if (!strip)
            return;
#ifdef DEBUG_ONOFRE
        Log.notice("[LED] teste: vermelho, verde, azul (%d LEDs, pino %d)" CR, st.count, fpin);
#endif
        const uint32_t cores[3] = {strip->Color(40, 0, 0), strip->Color(0, 40, 0), strip->Color(0, 0, 40)};
        for (int i = 0; i < 3; i++)
        {
            strip->fill(cores[i], 0, st.count);
            strip->show();
            delay(350);
        }
        strip->clear();
        strip->show();
        syncEngines(); // redesenha o estado atual
#ifdef DEBUG_ONOFRE
        Log.notice("[LED] teste concluído" CR);
#endif
    }

    void deinitStrip()
    {
        strip->clear();
        strip->show();
        delete strip;
        strip = nullptr;
        wasOn = false;
        needsSave = false;
        needsUndiscovery = true;
#ifdef DEBUG_ONOFRE
        Log.notice("[LED] fita removida" CR);
#endif
    }

    void undiscover()
    {
        String chip = String(config.chipId);
        publishOnMqtt((String("homeassistant/light/") + chip + "_led/config").c_str(), "", false);
        publishOnMqtt((String("homeassistant/number/") + chip + "_led_speed/config").c_str(), "", false);
        publishOnMqtt((String("homeassistant/number/") + chip + "_led_intensity/config").c_str(), "", false);
        publishOnMqtt(tState, "", true);
        publishOnMqtt(tSpeed, "", true);
        publishOnMqtt(tInt, "", true);
    }
} // namespace

void ledStripSetup()
{
    // Load the state only. The hardware waits for the loop, once boot has settled, and
    // only when a strip feature already exists. Without one this module touches nothing.
    topics();
    load();
    if (created)
        knxAssign(); // the KNX callback was registered back in config.load()
    initAfter = millis() + kInitDelayMs;
#ifdef DEBUG_ONOFRE
    Log.notice("[LED] %s" CR, created ? "função Fita LED configurada" : "sem função Fita LED");
#endif
}

void ledStripLoop()
{
    unsigned long now = millis();

    // Reconcile the hardware with the configuration; create and remove run in another task.
    if (created && !strip)
    {
        if ((long)(now - initAfter) >= 0)
            initStrip();
        return;
    }
    if (!created && strip)
    {
        deinitStrip();
        return;
    }
    if (!strip)
    {
        if (needsUndiscovery && mqttConnected() && config.tryBeginFeatureAccess())
        {
            undiscover();
            config.endFeatureAccess();
            needsUndiscovery = false;
        }
        return;
    }

    if (pendingFlag)
    {
        char local[sizeof(pendingBuf)];
        memcpy(local, pendingBuf, sizeof(local));
        pendingFlag = false;
#ifdef DEBUG_ONOFRE
        Log.notice("[LED] comando (web): %s" CR, local);
#endif
        applyCommand(local);
        needsPublish = true;
    }
    if (testRequested)
    {
        testRequested = false;
        selfTest();
    }
    if (knxPending >= 0)
    {
        const int v = knxPending;
        knxPending = -1;
#ifdef DEBUG_ONOFRE
        Log.notice("[LED] comando (KNX): %d" CR, v);
#endif
        st.on = (v == 102) ? !st.on : (v != 0);
        touch();
        needsPublish = true;
        lastKnxSent = st.on ? 100 : 0; // it came from KNX: do not echo it back
    }
    if (knxOn() && (st.on ? 100 : 0) != lastKnxSent)
    {
        // o estado mudou por outra via (painel, MQTT, Home Assistant): avisar o bus KNX
        lastKnxSent = st.on ? 100 : 0;
        knx.write_1byte_int(knx.GA_to_address(knxAddr[0], knxAddr[1], knxAddr[2]), lastKnxSent);
    }
    if ((needsPublish || needsDiscovery) && mqttConnected() && config.tryBeginFeatureAccess())
    {
        if (needsDiscovery)
        {
            ledStripSubscribe();
            ledStripDiscovery();
            needsDiscovery = false;
        }
        publishState();
        config.endFeatureAccess();
        needsPublish = false;
    }

    if (needsSave && (saveNow || now - lastChange > kSaveDelayMs))
    {
        needsSave = false;
        saveNow = false;
        if (created)
            save();
    }

    // Strip off and fully faded: clear it once and leave it alone.
    if (!st.on && mLvl <= 0.f)
    {
        if (wasOn)
        {
            strip->clear();
            strip->show();
            wasOn = false;
            estMA = 0;
            limiting = false;
        }
        return;
    }
    if (!wasOn)
    {
#ifdef DEBUG_ONOFRE
        Log.notice("[LED] ligada: %d LEDs, %d segmento(s), brilho %d, transição %d ms, limite %d mA" CR, st.count,
                   segCount, st.brightness, st.transition, st.limitMA);
#endif
        wasOn = true;
        frameDirty = true;
        lastRenderMs = now;
    }
    if (now - lastRenderMs < 16)
        return;
    unsigned long dtMs = now - lastRenderMs;
    if (dtMs > 100)
        dtMs = 100; // after a pause, do not jump
    lastRenderMs = now;

    // Fades: on/off linearly and master brightness smoothly, so nothing jumps.
    const float stepLvl = st.transition == 0 ? 1.f : dtMs / (float)st.transition;
    bool moving = false;
    const float mTarget = st.on ? 1.f : 0.f;
    if (mLvl != mTarget)
    {
        mLvl += (mTarget > mLvl) ? stepLvl : -stepLvl;
        mLvl = constrain(mLvl, 0.f, 1.f);
        moving = true;
    }
    if (fabsf(mBri - st.brightness) > 0.5f && st.transition > 0)
    {
        mBri += (st.brightness - mBri) * (1.f - expf(-3.f * dtMs / (float)st.transition));
        moving = true;
    }
    else
        mBri = st.brightness;
    for (int i = 0; i < segCount; i++)
    {
        const float sTarget = segs[i].on ? 1.f : 0.f;
        if (segLvl[i] != sTarget)
        {
            segLvl[i] += (sTarget > segLvl[i]) ? stepLvl : -stepLvl;
            segLvl[i] = constrain(segLvl[i], 0.f, 1.f);
            moving = true;
        }
    }

    // Each segment animates in its own engine; they are then composed into one frame.
    bool changed = frameDirty || moving;
    for (int i = 0; i < segCount; i++)
        if ((segs[i].on || segLvl[i] > 0.f) && engines[i].update(now))
            changed = true;
    if (!changed)
        return;
    frameDirty = false;
    if (frame.size() != st.count)
        frame.assign(st.count, led::RGB());
    for (auto &p : frame)
        p = led::RGB();
    for (int i = 0; i < segCount; i++)
    {
        if (segLvl[i] <= 0.f)
            continue;
        const int sb = (int)(segs[i].bri * segLvl[i]); // segment brightness, fade included
        const std::vector<led::RGB> &px = engines[i].pixels();
        for (size_t k = 0; k < px.size(); k++)
        {
            const size_t idx = segs[i].start + k;
            if (idx >= frame.size())
                break;
            frame[idx] = led::RGB((uint8_t)((px[k].r * sb) / 255), (uint8_t)((px[k].g * sb) / 255),
                                  (uint8_t)((px[k].b * sb) / 255));
        }
    }

    // Master brightness including the on/off fade, then gamma and white balance. What
    // leaves here is what the LEDs actually show, so this is what the current estimate uses.
    const int B = (int)(mBri * mLvl + 0.5f);
    buildGamma();
    uint32_t sum = 0;
    for (uint16_t i = 0; i < st.count; i++)
    {
        led::RGB &p = frame[i];
        p.r = (uint8_t)(gammaLut[(p.r * B) / 255] * st.cal[0] / 100);
        p.g = (uint8_t)(gammaLut[(p.g * B) / 255] * st.cal[1] / 100);
        p.b = (uint8_t)(gammaLut[(p.b * B) / 255] * st.cal[2] / 100);
        sum += p.r + p.g + p.b;
    }
    estMA = led::estimateMA(sum, st.count, st.mAperLED);
    const uint16_t fq = led::limitFactor256(estMA, st.count, st.limitMA); // 256 = no reduction
    limiting = fq < 256;
    if (limiting)
        estMA = led::estimateMA((uint64_t)sum * fq / 256, st.count, st.mAperLED);
    for (uint16_t i = 0; i < st.count; i++)
    {
        const led::RGB &p = frame[i];
        strip->setPixelColor(i, (uint8_t)(((uint32_t)p.r * fq) >> 8), (uint8_t)(((uint32_t)p.g * fq) >> 8),
                             (uint8_t)(((uint32_t)p.b * fq) >> 8));
    }
    strip->show();
}

bool ledStripHandleMqtt(const char *topic, const char *payload)
{
    if (!created || !strip)
        return false;
    if (strcmp(topic, tSpeedSet) == 0)
    {
        for (int i = 0; i < segCount; i++)
            segs[i].speed = constrain(atoi(payload), 0, 255);
        touch();
        publishState();
        return true;
    }
    if (strcmp(topic, tIntSet) == 0)
    {
        for (int i = 0; i < segCount; i++)
            segs[i].intensity = constrain(atoi(payload), 0, 255);
        touch();
        publishState();
        return true;
    }
    const bool fromCloud = tCloudCmd[0] != '\0' && strcmp(topic, tCloudCmd) == 0;
    if (!fromCloud && strcmp(topic, tCmd) != 0)
        return false;
#ifdef DEBUG_ONOFRE
    Log.notice("[LED] comando (MQTT): %s" CR, payload);
#endif
    applyCommand(payload);
    publishState();
    return true;
}

void ledStripSubscribe()
{
    if (!created)
        return;
    subscribeOnMqtt(tCmd);
    subscribeOnMqtt(tSpeedSet);
    subscribeOnMqtt(tIntSet);
}

void ledStripDiscovery()
{
    if (!created || !mqttConnected())
        return;
    String chip = String(config.chipId);
    {
        JsonDocument doc;
        JsonObject o = doc.to<JsonObject>();
        o["name"] = fname;
        o["uniq_id"] = chip + "_led";
        o["has_entity_name"] = true;
        o["schema"] = "json";
        o["cmd_t"] = tCmd;
        o["stat_t"] = tState;
        o["avty_t"] = config.healthTopic;
        o["brightness"] = true;
        o["effect"] = true;
        o["ic"] = "mdi:led-strip-variant";
        JsonArray modes = o["supported_color_modes"].to<JsonArray>();
        modes.add("rgb");
        JsonArray fx = o["effect_list"].to<JsonArray>();
        for (int i = 0; i < led::FX_COUNT; i++)
            fx.add(led::kEffectNames[i]);
        haDevice(o);
        publishConfig("light", chip + "_led", doc);
    }
    const char *names[2] = {"velocidade", "intensidade"};
    const char *ids[2] = {"_led_speed", "_led_intensity"};
    const char *cmd[2] = {tSpeedSet, tIntSet};
    const char *stat[2] = {tSpeed, tInt};
    for (int i = 0; i < 2; i++)
    {
        JsonDocument doc;
        JsonObject o = doc.to<JsonObject>();
        o["name"] = String(fname) + " " + names[i];
        o["uniq_id"] = chip + ids[i];
        o["has_entity_name"] = true;
        o["cmd_t"] = cmd[i];
        o["stat_t"] = stat[i];
        o["avty_t"] = config.healthTopic;
        o["min"] = 0;
        o["max"] = 255;
        o["step"] = 1;
        o["mode"] = "slider";
        o["ent_cat"] = "config";
        haDevice(o);
        publishConfig("number", chip + ids[i], doc);
    }
    publishState();
}

// ---- Create and remove the feature; called by the panel, from another task ----

int ledStripCreate(const char *name, int pin, int count)
{
    if (!name || !*name || strlen(name) >= sizeof(fname))
        return 1; // invalid name
    if (created)
        return 7; // only one strip may exist
    if (pin < 0 || !config.validOutputPin((unsigned int)pin))
        return 2; // a pin this board cannot use
    if (config.pinInUse((unsigned int)pin))
        return 5; // already taken by another feature
    strlcpy(fname, name, sizeof(fname));
    fpin = pin;
    st.count = (count >= 1 && count <= LED_STRIP_MAX) ? count : LED_STRIP_COUNT;
    st.on = false;
    st.brightness = 255;
    segCount = 1;
    segs[0] = Seg();
    segs[0].stop = st.count - 1;
    initAfter = millis() + kCreateDelayMs;
    __sync_synchronize();
    created = true;
    save(); // stored right away; the hardware starts in the main loop
    return 0;
}

void ledStripRemove()
{
    created = false; // o loop principal desliga e liberta a fita
    pendingFlag = false;
    needsSave = false;
    LittleFS.remove(kFile);
}

bool ledStripUsesPin(unsigned int pin) { return created && (int)pin == fpin; }

void ledStripConfigJson(JsonVariant root)
{
    if (!created)
        return;
    JsonObject o = root["ledStrip"].to<JsonObject>();
    o["id"] = "led";
    o["name"] = fname;
    o["pin"] = fpin;
    o["leds"] = st.count;
    o["area"] = knxAddr[0];
    o["line"] = knxAddr[1];
    o["member"] = knxAddr[2];
}

void ledStripCloudTopics(const char *username, const char *chipId)
{
    if (!created || username == nullptr || username[0] == '\0')
    {
        tCloudCmd[0] = '\0';
        tCloudState[0] = '\0';
        return;
    }
    snprintf(tCloudCmd, sizeof(tCloudCmd), "%s/%s/light/led/set", username, chipId);
    snprintf(tCloudState, sizeof(tCloudState), "%s/%s/light/led/status", username, chipId);
}

const char *ledStripCloudWriteTopic()
{
    return tCloudCmd;
}

void ledStripFeatureJson(JsonArray features, bool allFields)
{
    if (!created)
        return;
    JsonObject a = features.add<JsonObject>();
    if (allFields)
        a["group"] = "ACTUATOR";
    a["id"] = "led";
    a["name"] = fname;
    a["family"] = "LIGHT";
    a["driver"] = "LED_STRIP";
    a["state"] = st.on ? "ON" : "OFF";
    a["area"] = knxAddr[0];
    a["line"] = knxAddr[1];
    a["member"] = knxAddr[2];
}

// ---- KNX (chamado por actuatoresCallback) ----
static bool knxMatches(uint8_t a, uint8_t l, uint8_t m)
{
    // same rule as the other actuators: the exact address, or the group's (area/line/0 and area/0/0)
    return knxOn() && knxAddr[0] == a && (knxAddr[1] == l || (l == 0 && m == 0)) && (knxAddr[2] == m || m == 0);
}

bool ledStripKnxWrite(uint8_t a, uint8_t l, uint8_t m, int value)
{
    if (!created || !knxMatches(a, l, m))
        return false;
    knxPending = value;
    return true;
}

bool ledStripKnxRead(uint8_t a, uint8_t l, uint8_t m, int &state)
{
    if (!created || !knxOn() || knxAddr[0] != a || knxAddr[1] != l || knxAddr[2] != m)
        return false;
    state = st.on ? 100 : 0;
    return true;
}

// ---- Control page at /led ----
static const char kPage[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="pt"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Fita LED</title>
<style>
:root{--bg:#f4f5f7;--card:#fff;--tx:#1b1d21;--mut:#6b7280;--ac:#97d700;--bd:#e2e5ea}
@media(prefers-color-scheme:dark){:root{--bg:#14161a;--card:#1d2026;--tx:#eceef1;--mut:#9aa1ad;--bd:#2c313a}}
*{box-sizing:border-box}body{margin:0;padding:16px;background:var(--bg);color:var(--tx);font:16px system-ui,sans-serif}
.w{max-width:460px;margin:0 auto}.c{background:var(--card);border:1px solid var(--bd);border-radius:14px;padding:16px;margin-bottom:12px}
h1{font-size:20px;margin:0 0 12px;display:flex;justify-content:space-between;align-items:center;gap:8px}
label{display:block;font-size:13px;color:var(--mut);margin:14px 0 6px}label:first-child{margin-top:0}
input[type=range]{width:100%;accent-color:var(--ac)}select,input[type=number]{width:100%;padding:10px;border-radius:10px;border:1px solid var(--bd);background:var(--bg);color:var(--tx);font-size:16px}
input[type=color]{width:100%;height:52px;border:1px solid var(--bd);border-radius:10px;background:var(--bg);padding:4px}
button{padding:10px 18px;border-radius:999px;border:0;font-size:15px;font-weight:600;cursor:pointer;background:var(--bd);color:var(--tx)}
button.on{background:var(--ac);color:#10200a}.v{float:right;color:var(--mut)}
#st,#info{font-size:12px;color:var(--mut);text-align:center}a{color:var(--mut)}
</style></head><body><div class="w">
<div class="c" id="off" style="display:none">Ainda não há nenhuma fita LED. No painel principal, em <b>Funções</b> &rarr; <b>Nova função</b>, escolhe <b>Fita LED</b>, o GPIO e o número de LEDs.</div>
<div class="c" id="ctl"><h1><span id="nm">Fita LED</span> <button id="pw">Desligada</button></h1>
<label>Cor</label><input type="color" id="co" value="#ffa03c">
<label>Brilho <span class="v" id="bv"></span></label><input type="range" id="br" min="1" max="255">
<label>Efeito</label><select id="fx"></select>
<label>Velocidade <span class="v" id="sv"></span></label><input type="range" id="sp" min="0" max="255">
<label>Intensidade <span class="v" id="iv"></span></label><input type="range" id="it" min="0" max="255">
<label>Número de LEDs</label><input type="number" id="nl" min="1" max="600">
</div>
<div id="info"></div><div id="st"></div></div>
<script>
const $=i=>document.getElementById(i);let on=false,busy=0,t;
const hex=c=>'#'+[c.r,c.g,c.b].map(x=>x.toString(16).padStart(2,'0')).join('');
const rgb=h=>({r:parseInt(h.substr(1,2),16),g:parseInt(h.substr(3,2),16),b:parseInt(h.substr(5,2),16)});
function show(s){on=s.on;$('pw').textContent=on?'Ligada':'Desligada';$('pw').className=on?'on':'';
 $('off').style.display=s.enabled?'none':'block';$('ctl').style.display=s.enabled?'block':'none';
 $('nm').textContent=s.name||'Fita LED';$('info').textContent=s.enabled?'GPIO'+s.pin+' \u00b7 '+s.leds+' LEDs':'';
 if(busy)return;$('co').value=hex(s.color);$('br').value=s.brightness;$('sp').value=s.speed;$('it').value=s.intensity;
 if(!$('fx').options.length)s.effects.forEach(e=>$('fx').add(new Option(e,e)));$('fx').value=s.effect;
 if(document.activeElement!==$('nl'))$('nl').value=s.leds;
 $('bv').textContent=s.brightness;$('sv').textContent=s.speed;$('iv').textContent=s.intensity}
async function get(){try{show(await(await fetch('/ledstate')).json());$('st').textContent=''}catch(e){$('st').textContent='sem ligação'}}
function send(o,now){clearTimeout(t);busy=Date.now();const go=async()=>{try{show(await(await fetch('/led/set',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(o)})).json())}catch(e){$('st').textContent='erro ao enviar'}setTimeout(()=>busy=0,600)};now?go():t=setTimeout(go,90)}
$('pw').onclick=()=>send({state:on?'OFF':'ON'},1);
$('co').oninput=e=>send({state:'ON',color:rgb(e.target.value)});
$('br').oninput=e=>{$('bv').textContent=e.target.value;send({brightness:+e.target.value})};
$('sp').oninput=e=>{$('sv').textContent=e.target.value;send({speed:+e.target.value})};
$('it').oninput=e=>{$('iv').textContent=e.target.value;send({intensity:+e.target.value})};
$('fx').onchange=e=>send({state:'ON',effect:e.target.value},1);
$('nl').onchange=e=>send({leds:+e.target.value},1);
get();setInterval(()=>{if(!busy)get()},3000);
</script></body></html>)rawliteral";

const char *ledStripPageHtml() { return kPage; }
size_t ledStripPageSize() { return sizeof(kPage) - 1; }

bool ledStripQueueCommand(JsonVariantConst json)
{
    if (!created || !strip || pendingFlag || measureJson(json) >= sizeof(pendingBuf))
        return false;
    serializeJson(json, pendingBuf, sizeof(pendingBuf));
    __sync_synchronize();
    pendingFlag = true;
    return true;
}

void ledStripStateJson(JsonVariant &root)
{
    root["enabled"] = (bool)created;
    root["name"] = fname;
    root["pin"] = fpin;
    root["on"] = st.on;
    root["brightness"] = st.brightness;
    JsonObject c = root["color"].to<JsonObject>();
    c["r"] = segs[0].color.r;
    c["g"] = segs[0].color.g;
    c["b"] = segs[0].color.b;
    root["effect"] = led::kEffectNames[segs[0].effect];
    root["speed"] = segs[0].speed;
    root["intensity"] = segs[0].intensity;
    JsonArray sg = root["segs"].to<JsonArray>();
    for (int i = 0; i < segCount; i++)
    {
        const Seg &g = segs[i];
        JsonObject o = sg.add<JsonObject>();
        o["start"] = g.start;
        o["stop"] = g.stop;
        o["on"] = g.on;
        o["brightness"] = g.bri;
        JsonObject gc = o["color"].to<JsonObject>();
        gc["r"] = g.color.r;
        gc["g"] = g.color.g;
        gc["b"] = g.color.b;
        o["effect"] = led::kEffectNames[g.effect];
        o["speed"] = g.speed;
        o["intensity"] = g.intensity;
    }
    root["leds"] = st.count;
    root["knx"] = knxAddr[0] > 0;
    root["transition"] = st.transition;
    root["limit"] = st.limitMA;
    root["maPerLed"] = st.mAperLED;
    root["ma"] = estMA;
    root["limited"] = limiting;
    root["colorOrder"] = st.colorOrder;
    root["gamma"] = st.gamma;
    JsonArray cal = root["cal"].to<JsonArray>();
    for (int i = 0; i < 3; i++)
        cal.add(st.cal[i]);
    JsonArray fx = root["effects"].to<JsonArray>();
    for (int i = 0; i < led::FX_COUNT; i++)
        fx.add(led::kEffectNames[i]);
}
#endif // LED_STRIP
