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

// ---- Configuração de compilação (platformio.ini / platformio_override.ini) ----
#ifndef LED_STRIP_PIN
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
#define LED_STRIP_PIN 4 // só um valor de reserva: o pino real escolhe-se em "Nova função" (no C3, 18/19 são o USB)
#else
#define LED_STRIP_PIN 18 // só um valor de reserva: o pino real escolhe-se em "Nova função"
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
    constexpr unsigned long kSaveDelayMs = 3000;  // poupa a flash: grava só depois de parar de mexer
    constexpr unsigned long kInitDelayMs = 8000;  // no arranque, a fita só acorda depois de tudo assentar
    constexpr unsigned long kCreateDelayMs = 500; // ao criar pelo painel, quase de imediato

    // Função "Fita LED" criada em Nova função. Criar e remover mexe só nestes campos (e no
    // ficheiro); o hardware da fita é sempre tratado pelo loop principal (reconcile em ledStripLoop).
    volatile bool created = false;
    char fname[24] = "Fita LED";
    int fpin = LED_STRIP_PIN;

    // KNX: um endereço de grupo (área/linha/membro) para ligar/desligar, com realimentação do estado,
    // como nas luzes. 0/0/0 = sem KNX. Valores como nos restantes atuadores: 0 = desligado, 100 = ligado.
    uint8_t knxAddr[3] = {0, 0, 0};
    volatile int knxPending = -1; // pedido recebido pelo KNX, aplicado no loop principal
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

    // Estado geral da fita: interruptor e brilho mestres, e número de LEDs.
    struct State
    {
        bool on = false;
        uint8_t brightness = 255;
        uint16_t count = LED_STRIP_COUNT;
        uint16_t transition = 400; // ms para ligar/desligar e mudar de cor suavemente (0 = sem transição)
        uint16_t limitMA = 0;      // limite de corrente da fonte, em mA (0 = sem limite)
        uint16_t mAperLED = 60;    // consumo de um LED a branco, a fundo
        uint8_t colorOrder = 0;    // índice em kOrders (0 = GRB, o normal das WS2812B)
        uint8_t gamma = 10;        // correção gama em décimas (10 = sem correção, 22 = 2,2, 28 = 2,8)
        uint8_t cal[3] = {100, 100, 100}; // balanço de cores: % de vermelho, verde e azul
    } st;

    // Ordens dos bytes de cor que as fitas usam. Quando "vermelho" sai verde ou azul, troca-se aqui.
    const neoPixelType kOrders[6] = {NEO_GRB, NEO_RGB, NEO_BRG, NEO_RBG, NEO_GBR, NEO_BGR};

    // Tabela da correção gama: o olho vê a luz de forma não linear e os LEDs são lineares; sem isto as
    // cores fracas parecem lavadas (um vermelho com um pouco de verde ou azul vira rosa).
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

    // Valores "a ver" durante as transições (não se gravam).
    float mLvl = 0.f;                 // 0..1: a fita toda a acender/apagar
    float mBri = 255.f;               // brilho geral suavizado
    float segLvl[8] = {1, 1, 1, 1, 1, 1, 1, 1}; // 0..1 de cada segmento a acender/apagar
    unsigned long lastRenderMs = 0;
    uint32_t estMA = 0;               // corrente estimada da última imagem enviada (já com o limite)
    bool limiting = false;            // o limite está a reduzir a luz

    // Segmentos (zonas da fita), como no WLED: cada um com o seu intervalo de LEDs, cor, efeito, etc.
    // Há sempre pelo menos um; por omissão o segmento 0 cobre a fita toda.
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

    // Redimensiona os motores ao tamanho dos segmentos (quando o desenho dos segmentos muda).
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

    // Atualiza só as propriedades (efeito, cor, velocidade, intensidade) nos motores.
    void syncEngines()
    {
        for (int i = 0; i < segCount; i++)
        {
            engines[i].setTransition(st.transition);
            engines[i].set((led::Effect)segs[i].effect, segs[i].color, segs[i].speed, segs[i].intensity);
        }
        frameDirty = true;
    }

    // Garante que os segmentos cabem na fita (depois de mudar o número de LEDs).
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
    bool saveNow = false; // definições (não deslizantes): gravar já, para não se perderem se o dispositivo reiniciar
    bool needsDiscovery = false, needsUndiscovery = false;
    bool testRequested = false;
    unsigned long lastChange = 0;
    char tCmd[64], tState[64], tSpeed[64], tSpeedSet[64], tInt[64], tIntSet[64];

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
                // segmentos: [início, fim, ligado, brilho, r, g, b, efeito, velocidade, intensidade]
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
                // ficheiro de uma versão sem segmentos: um segmento com tudo
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
            segs[0].stop = n - 1; // com um só segmento, acompanha a fita
        }
        else
            fitSegments();
        rebuildEngines();
    }

    // Define o desenho dos segmentos. Os novos herdam as propriedades do primeiro.
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

    // Aplica um comando (JSON ou "ON"/"OFF"). Serve o MQTT e a página web.
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
        // Cor, brilho, velocidade e intensidade mexem-se a deslizar e gravam-se com atraso (poupa a flash). O resto
        // são definições: gravam-se já, senão perdiam-se se o dispositivo reiniciasse logo a seguir.
        if (doc["leds"].is<int>() || doc["segments"].is<JsonArray>() ||
            doc["colorOrder"].is<int>() || doc["gamma"].is<int>() || doc["transition"].is<int>() ||
            doc["limit"].is<int>() || doc["maPerLed"].is<int>() || doc["knx"].is<JsonObject>())
            saveNow = true;
        // "seg": índice do segmento a que o comando se dirige; sem "seg" aplica-se a todos.
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
            st.limitMA = constrain((int)doc["limit"], 0, 20000); // mA; 0 = sem limite
        if (doc["maPerLed"].is<int>())
            st.mAperLED = constrain((int)doc["maPerLed"], 10, 100);
        if (doc["colorOrder"].is<int>())
        {
            st.colorOrder = constrain((int)doc["colorOrder"], 0, 5);
            if (strip)
                strip->updateType(kOrders[st.colorOrder] + NEO_KHZ800); // sem reiniciar
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

    // Comandos vindos da página web: o handler assíncrono só os deixa aqui; quem os
    // aplica é o loop principal, para nunca mexer na fita a partir de outra tarefa.
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
        mLvl = st.on ? 1.f : 0.f; // ao arrancar, a fita fica logo como estava
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
    // a massa e a alimentação estão bem, e qualquer problema depois disso é de configuração.
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
    // Só carrega o estado. O hardware fica para o loop (depois do arranque assentar), e só se
    // já existir uma função "Fita LED" criada em Nova função. Sem ela o módulo não toca em nada.
    topics();
    load();
    if (created)
        knxAssign(); // o registo do callback KNX já foi feito em config.load()
    initAfter = millis() + kInitDelayMs;
#ifdef DEBUG_ONOFRE
    Log.notice("[LED] %s" CR, created ? "função Fita LED configurada" : "sem função Fita LED");
#endif
}

void ledStripLoop()
{
    unsigned long now = millis();

    // Reconciliar o hardware com a configuração (criar/remover a função faz-se noutra tarefa).
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
        lastKnxSent = st.on ? 100 : 0; // veio do KNX: não devolver eco
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

    // Fita apagada e totalmente desvanecida: apaga uma vez e deixa-a em paz.
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
        dtMs = 100; // depois de uma pausa, não saltar
    lastRenderMs = now;

    // Transições: ligar/desligar (linear) e brilho geral (suave), para nada saltar.
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

    // Cada segmento anima-se no seu motor; depois juntam-se todos numa só imagem.
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
        const int sb = (int)(segs[i].bri * segLvl[i]); // brilho do segmento, já com o desvanecimento
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

    // Brilho geral (com o desvanecimento de ligar/desligar), correção gama e balanço de cores. O que sai
    // daqui é o que os LEDs vão mesmo mostrar, por isso é com isto que se estima a corrente.
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
    const uint16_t fq = led::limitFactor256(estMA, st.count, st.limitMA); // 256 = sem redução
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
    if (strcmp(topic, tCmd) != 0)
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

// ---- Criar / remover a função (chamado pelo painel, noutra tarefa) ----

int ledStripCreate(const char *name, int pin, int count)
{
    if (!name || !*name || strlen(name) >= sizeof(fname))
        return 1; // nome inválido
    if (created)
        return 7; // só pode existir uma fita
    if (pin < 0 || !config.validOutputPin((unsigned int)pin))
        return 2; // pino que não serve nesta placa
    if (config.pinInUse((unsigned int)pin))
        return 5; // já usado por outra função
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
    save(); // fica guardada já; o hardware arranca no loop principal
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

// ---- KNX (chamado por actuatoresCallback) ----
static bool knxMatches(uint8_t a, uint8_t l, uint8_t m)
{
    // mesma regra dos restantes atuadores: endereço exato, ou o do grupo (área/linha/0 e área/0/0)
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

// ---- Página de controlo em /led ----
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
