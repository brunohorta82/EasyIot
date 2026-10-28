#pragma once
// Motor de efeitos para fitas LED enderecáveis (estilo WLED).
// C++ puro: sem Arduino, sem bibliotecas. Por isso compila e testa-se no PC.
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <vector>

namespace led
{
    struct RGB
    {
        uint8_t r, g, b;
        RGB() : r(0), g(0), b(0) {}
        RGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
    };

    enum Effect : uint8_t
    {
        FX_STATIC = 0,
        FX_BLINK,
        FX_BREATHE,
        FX_RAINBOW,
        FX_COLOR_WIPE,
        FX_CHASE,
        FX_SCAN,
        FX_TWINKLE,
        FX_FIRE,
        FX_METEOR,
        FX_STROBE,
        FX_COLOR_CYCLE,
        FX_THEATER_RAINBOW,
        FX_RUNNING,
        FX_SPARKLE,
        FX_PLASMA,
        FX_CONFETTI,
        FX_LIGHTNING,
        FX_CANDLE,
        FX_POLICE,
        FX_HEARTBEAT,
        FX_JUGGLE,
        FX_GRADIENT,
        FX_NOISE,
        FX_COUNT
    };

    static const char *const kEffectNames[FX_COUNT] = {
        "Static", "Blink", "Breathe", "Rainbow", "Color Wipe",
        "Chase", "Scan", "Twinkle", "Fire", "Meteor",
        "Strobe", "Color Cycle", "Theater Rainbow", "Running Lights", "Sparkle", "Plasma", "Confetti",
        "Lightning", "Candle", "Police", "Heartbeat", "Juggle", "Gradient", "Noise"};

    inline RGB scale(RGB c, float f)
    {
        if (f < 0.f)
            f = 0.f;
        if (f > 1.f)
            f = 1.f;
        return RGB((uint8_t)(c.r * f), (uint8_t)(c.g * f), (uint8_t)(c.b * f));
    }

    // h em [0,1), s e v em [0,1]
    inline RGB hsv(float h, float s, float v)
    {
        h -= floorf(h);
        float i = floorf(h * 6.f);
        float f = h * 6.f - i;
        float p = v * (1.f - s), q = v * (1.f - f * s), t = v * (1.f - (1.f - f) * s);
        float r, g, b;
        switch (((int)i) % 6)
        {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
        }
        return RGB((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
    }

    inline RGB blend(RGB a, RGB b, float t)
    {
        if (t < 0.f)
            t = 0.f;
        if (t > 1.f)
            t = 1.f;
        return RGB((uint8_t)(a.r + (b.r - a.r) * t), (uint8_t)(a.g + (b.g - a.g) * t), (uint8_t)(a.b + (b.b - a.b) * t));
    }

    inline RGB addSat(RGB a, RGB b)
    {
        const int r = a.r + b.r, g = a.g + b.g, bl = a.b + b.b;
        return RGB((uint8_t)(r > 255 ? 255 : r), (uint8_t)(g > 255 ? 255 : g), (uint8_t)(bl > 255 ? 255 : bl));
    }

    inline void rgb2hsv(RGB c, float &h, float &s, float &v)
    {
        const float r = c.r / 255.f, g = c.g / 255.f, b = c.b / 255.f;
        const float mx = fmaxf(r, fmaxf(g, b)), mn = fminf(r, fminf(g, b)), d = mx - mn;
        v = mx;
        s = mx <= 0.f ? 0.f : d / mx;
        if (d <= 0.f)
        {
            h = 0.f;
            return;
        }
        if (mx == r)
            h = fmodf((g - b) / d, 6.f);
        else if (mx == g)
            h = (b - r) / d + 2.f;
        else
            h = (r - g) / d + 4.f;
        h /= 6.f;
        if (h < 0.f)
            h += 1.f;
    }

    // Ruído de valor 1D, suave, entre 0 e 1.
    inline float hash01(uint32_t n)
    {
        n = (n << 13) ^ n;
        n = n * (n * n * 15731u + 789221u) + 1376312589u;
        return (n & 0x7fffffffu) / 2147483648.f;
    }
    inline float noise1(float x)
    {
        const float fl = floorf(x), f = x - fl;
        const uint32_t i = (uint32_t)(int32_t)fl;
        const float t = f * f * (3.f - 2.f * f);
        const float a = hash01(i), b = hash01(i + 1);
        return a + (b - a) * t;
    }

    // ---- Limite de corrente ----
    // Um LED WS281x gasta cerca de mAperLED com os três canais a fundo (branco) e um terço disso só com um canal.
    // Em repouso gasta ~1 mA. A estimativa soma os canais de todos os LEDs.
    inline uint32_t estimateMA(uint32_t channelSum, uint16_t leds, uint16_t mAperLED)
    {
        return (uint32_t)((uint64_t)channelSum * mAperLED / 765u) + leds;
    }
    // Fator (0..256) por que multiplicar a luz para a corrente não passar de limitMA. 256 = sem redução.
    inline uint16_t limitFactor256(uint32_t estMA, uint16_t leds, uint32_t limitMA)
    {
        if (limitMA == 0 || estMA <= limitMA)
            return 256;
        const uint32_t idle = leds;
        if (limitMA <= idle)
            return 0;
        const uint32_t active = estMA - idle; // > 0, porque estMA > limitMA > idle
        return (uint16_t)(((uint64_t)(limitMA - idle) * 256u) / active);
    }

    class Engine
    {
    public:
        void resize(uint16_t n)
        {
            if (n < 1)
                n = 1;
            _n = n;
            _px.assign(n, RGB());
            _heat.assign(n, 0);
            _dirty = true;
        }
        // Duração (ms) da transição suave entre cores. 0 = muda de cor de imediato.
        void setTransition(uint16_t ms) { _trMs = ms; }

        void set(Effect e, RGB c, uint8_t speed, uint8_t intensity)
        {
            if (e >= FX_COUNT)
                e = FX_STATIC;
            if (e != _fx)
            {
                _fx = e;
                _phase = 0.f;
                _acc = 0;
                for (auto &p : _px)
                    p = RGB();
                for (auto &h : _heat)
                    h = 0;
                _head = 0.f;
                _flash = 0;
                _lvl = _tgt = 1.f;
            }
            // _color é a cor que se vê; _tc é a pretendida. Aproximam-se em stepColor().
            _tc = c;
            if (!_hasColor || _trMs == 0)
            {
                _color = c;
                _cf[0] = c.r;
                _cf[1] = c.g;
                _cf[2] = c.b;
                _hasColor = true;
            }
            _speed = speed;
            _intensity = intensity;
            _dirty = true;
        }
        uint16_t size() const { return _n; }
        const std::vector<RGB> &pixels() const { return _px; }

        // Avança o tempo. Devolve true se o quadro mudou e há que enviar à fita.
        bool update(uint32_t now)
        {
            uint32_t dt = _last ? now - _last : 0;
            if (_last && dt < kMinFrameMs)
                return false;
            _last = now;
            if (dt > 250)
                dt = 250; // depois de uma pausa longa, não saltar
            float rate = 0.1f + (_speed / 255.f) * 3.9f; // ciclos por segundo
            _phase += rate * (dt / 1000.f);
            if (_phase > 1000.f)
                _phase -= 1000.f;
            const bool colorMoving = stepColor(dt);

            switch (_fx)
            {
            case FX_STATIC:
                if (!_dirty && !colorMoving)
                    return false;
                fill(_color);
                break;
            case FX_BLINK: blink(); break;
            case FX_BREATHE: breathe(); break;
            case FX_RAINBOW: rainbow(); break;
            case FX_COLOR_WIPE: colorWipe(); break;
            case FX_CHASE: chase(); break;
            case FX_SCAN: scan(); break;
            case FX_TWINKLE: stepped(dt, &Engine::twinkleStep); break;
            case FX_FIRE: stepped(dt, &Engine::fireStep); break;
            case FX_METEOR: stepped(dt, &Engine::meteorStep); break;
            case FX_STROBE: strobe(); break;
            case FX_COLOR_CYCLE: fill(hsv(frac(), 1.f, 1.f)); break;
            case FX_THEATER_RAINBOW: theaterRainbow(); break;
            case FX_RUNNING: runningLights(); break;
            case FX_SPARKLE: stepped(dt, &Engine::sparkleStep); break;
            case FX_PLASMA: plasma(); break;
            case FX_CONFETTI: stepped(dt, &Engine::confettiStep); break;
            case FX_LIGHTNING: stepped(dt, &Engine::lightningStep); break;
            case FX_CANDLE: stepped(dt, &Engine::candleStep); break;
            case FX_POLICE: police(); break;
            case FX_HEARTBEAT: heartbeat(); break;
            case FX_JUGGLE: stepped(dt, &Engine::juggleStep); break;
            case FX_GRADIENT: gradient(); break;
            case FX_NOISE: noise(); break;
            default: break;
            }
            _dirty = false;
            return true;
        }

    private:
        static constexpr uint32_t kMinFrameMs = 16;
        static constexpr uint32_t kTickMs = 20;
        uint16_t _n = 1;
        std::vector<RGB> _px;
        std::vector<uint8_t> _heat;
        Effect _fx = FX_STATIC;
        RGB _color{255, 160, 60}, _tc{255, 160, 60};
        float _cf[3] = {255.f, 160.f, 60.f};
        bool _hasColor = false;
        uint16_t _trMs = 0;

        // Aproxima _color de _tc (exponencial: ~95 % do caminho em _trMs). True enquanto se mexe.
        bool stepColor(uint32_t dt)
        {
            if (_color.r == _tc.r && _color.g == _tc.g && _color.b == _tc.b)
                return false;
            if (_trMs == 0)
            {
                _color = _tc;
                return true;
            }
            const float k = 1.f - expf(-3.f * dt / (float)_trMs);
            const float tgt[3] = {(float)_tc.r, (float)_tc.g, (float)_tc.b};
            bool near = true;
            for (int i = 0; i < 3; i++)
            {
                _cf[i] += (tgt[i] - _cf[i]) * k;
                if (fabsf(tgt[i] - _cf[i]) > 0.6f)
                    near = false;
            }
            if (near)
            {
                _cf[0] = tgt[0];
                _cf[1] = tgt[1];
                _cf[2] = tgt[2];
                _color = _tc;
            }
            else
                _color = RGB((uint8_t)(_cf[0] + 0.5f), (uint8_t)(_cf[1] + 0.5f), (uint8_t)(_cf[2] + 0.5f));
            return true;
        }
        uint8_t _speed = 128, _intensity = 128;
        float _phase = 0.f;
        uint32_t _last = 0, _acc = 0;
        bool _dirty = true;

        static uint32_t rnd() { return (uint32_t)rand(); }
        void fill(RGB c)
        {
            for (auto &p : _px)
                p = c;
        }
        float frac() const { return _phase - floorf(_phase); }

        void blink()
        {
            float duty = 0.1f + 0.8f * (_intensity / 255.f);
            fill(frac() < duty ? _color : RGB());
        }
        void breathe()
        {
            float v = (1.f - cosf(6.2831853f * frac())) * 0.5f;
            fill(scale(_color, v));
        }
        void rainbow()
        {
            float reps = 1.f + (_intensity / 255.f) * 4.f;
            for (uint16_t i = 0; i < _n; i++)
                _px[i] = hsv(frac() + reps * i / _n, 1.f, 1.f);
        }
        void colorWipe()
        {
            float t = fmodf(_phase * 0.5f, 1.f) * 2.f; // 0..2
            uint16_t k;
            if (t < 1.f)
            {
                k = (uint16_t)(t * _n);
                for (uint16_t i = 0; i < _n; i++)
                    _px[i] = i < k ? _color : RGB();
            }
            else
            {
                k = (uint16_t)((t - 1.f) * _n);
                for (uint16_t i = 0; i < _n; i++)
                    _px[i] = i < k ? RGB() : _color;
            }
        }
        void chase()
        {
            int gap = 2 + (_intensity / 64); // 2..5
            int off = (int)(_phase * 6.f);
            for (uint16_t i = 0; i < _n; i++)
                _px[i] = ((i + off) % gap == 0) ? _color : RGB();
        }
        void scan()
        {
            float t = fmodf(_phase * 0.5f, 1.f) * 2.f;
            float p = t < 1.f ? t : 2.f - t;
            float centre = p * (_n - 1);
            float size = 1.f + (_intensity / 255.f) * (_n / 6.f + 2.f);
            for (uint16_t i = 0; i < _n; i++)
                _px[i] = scale(_color, 1.f - fabsf(i - centre) / size);
        }

        typedef void (Engine::*StepFn)();
        void stepped(uint32_t dt, StepFn fn)
        {
            _acc += dt;
            while (_acc >= kTickMs)
            {
                (this->*fn)();
                _acc -= kTickMs;
            }
        }
        void twinkleStep()
        {
            for (uint16_t i = 0; i < _n; i++)
                _heat[i] = _heat[i] > 12 ? _heat[i] - 12 : 0;
            int tries = 1 + _speed / 64;
            for (int k = 0; k < tries; k++)
                if ((rnd() % 255) < (uint32_t)(10 + _intensity / 4))
                    _heat[rnd() % _n] = 255;
            for (uint16_t i = 0; i < _n; i++)
                _px[i] = scale(_color, _heat[i] / 255.f);
        }
        void fireStep()
        {
            int cooling = 55;
            int sparking = 50 + _intensity / 2;
            for (uint16_t i = 0; i < _n; i++)
            {
                int cool = rnd() % ((cooling * 10) / _n + 2);
                _heat[i] = _heat[i] > cool ? _heat[i] - cool : 0;
            }
            for (int k = _n - 1; k >= 2; k--)
                _heat[k] = (_heat[k - 1] + _heat[k - 2] + _heat[k - 2]) / 3;
            if ((int)(rnd() % 255) < sparking)
            {
                int y = rnd() % (_n < 7 ? _n : 7);
                int h = _heat[y] + 160 + (int)(rnd() % 95);
                _heat[y] = h > 255 ? 255 : h;
            }
            for (uint16_t j = 0; j < _n; j++)
            {
                uint8_t t192 = (uint8_t)((_heat[j] / 255.f) * 191);
                uint8_t ramp = (uint8_t)((t192 & 0x3F) << 2);
                if (t192 > 0x80)
                    _px[j] = RGB(255, 255, ramp);
                else if (t192 > 0x40)
                    _px[j] = RGB(255, ramp, 0);
                else
                    _px[j] = RGB(ramp, 0, 0);
            }
        }
        void meteorStep()
        {
            // cauda: apaga um pouco cada pixel; cabeça avança segundo a velocidade
            int fade = 20 + (255 - _intensity) / 4; // 20..83 em 256
            for (auto &p : _px)
            {
                p.r = (uint8_t)((p.r * (256 - fade)) >> 8);
                p.g = (uint8_t)((p.g * (256 - fade)) >> 8);
                p.b = (uint8_t)((p.b * (256 - fade)) >> 8);
            }
            _head += 0.1f + (_speed / 255.f) * 1.4f;
            if (_head >= _n + 8)
                _head = 0.f;
            if ((int)_head < _n)
                _px[(int)_head] = _color;
        }
        float _head = 0.f;
        int _flash = 0;
        uint16_t _la = 0, _lb = 0;
        float _lvl = 1.f, _tgt = 1.f;

        void strobe()
        {
            const float w = 0.05f + 0.3f * (_intensity / 255.f);
            fill(fmodf(_phase * 4.f, 1.f) < w ? _color : RGB());
        }
        void theaterRainbow()
        {
            const int gap = 3;
            const int off = (int)(_phase * 6.f);
            for (uint16_t i = 0; i < _n; i++)
                _px[i] = ((i + off) % gap == 0) ? hsv(frac() + (float)i / _n, 1.f, 1.f) : RGB();
        }
        void runningLights()
        {
            const float waves = 1.f + (_intensity / 64);
            for (uint16_t i = 0; i < _n; i++)
                _px[i] = scale(_color, (sinf(6.2831853f * (waves * i / _n - _phase)) + 1.f) * 0.5f);
        }
        void plasma()
        {
            const float t = _phase * 6.2831853f, spread = 0.15f + 0.35f * (_intensity / 255.f);
            for (uint16_t i = 0; i < _n; i++)
            {
                const float v = (sinf(i * 0.15f + t) + sinf(i * 0.07f - t * 1.3f) + sinf(i * 0.11f + t * 0.7f)) / 3.f;
                _px[i] = hsv(0.6f + v * spread, 1.f, 1.f);
            }
        }
        void police()
        {
            const float f = fmodf(_phase * 2.f, 1.f);
            const bool left = f < 0.5f;
            const float sub = fmodf(f * 2.f, 1.f);
            const bool on = fmodf(sub * 3.f, 1.f) < 0.6f;
            for (uint16_t i = 0; i < _n; i++)
            {
                const bool leftHalf = i < _n / 2 || _n == 1;
                _px[i] = (on && leftHalf == left) ? (left ? RGB(255, 0, 0) : RGB(0, 0, 255)) : RGB();
            }
        }
        void heartbeat()
        {
            const float f = fmodf(_phase * 0.5f, 1.f);
            const float a = (f - 0.12f) / 0.05f, b = (f - 0.32f) / 0.06f;
            fill(scale(_color, expf(-a * a) + 0.6f * expf(-b * b)));
        }
        void gradient()
        {
            float h, s, v;
            rgb2hsv(_color, h, s, v);
            const float spread = 0.1f + 0.5f * (_intensity / 255.f);
            for (uint16_t i = 0; i < _n; i++)
            {
                const float u = (float)i / _n + frac();
                _px[i] = hsv(h + spread * (0.5f - 0.5f * cosf(6.2831853f * u)), s, v);
            }
        }
        void noise()
        {
            const float zoom = 0.10f + 0.25f * (_intensity / 255.f);
            for (uint16_t i = 0; i < _n; i++)
            {
                float v = noise1(i * zoom + _phase * 3.f) * 1.5f - 0.25f;
                _px[i] = scale(_color, v);
            }
        }
        void sparkleStep()
        {
            const RGB bg = scale(_color, 0.15f);
            for (uint16_t i = 0; i < _n; i++)
                _heat[i] = _heat[i] > 60 ? _heat[i] - 60 : 0;
            const int tries = 1 + _speed / 90;
            for (int k = 0; k < tries; k++)
                if ((rnd() % 255) < (uint32_t)(15 + _intensity / 2))
                    _heat[rnd() % _n] = 255;
            for (uint16_t i = 0; i < _n; i++)
                _px[i] = blend(bg, RGB(255, 255, 255), _heat[i] / 255.f);
        }
        void confettiStep()
        {
            for (auto &p : _px)
            {
                p.r = (uint8_t)((p.r * 235) >> 8);
                p.g = (uint8_t)((p.g * 235) >> 8);
                p.b = (uint8_t)((p.b * 235) >> 8);
            }
            const int tries = 1 + _speed / 90;
            for (int k = 0; k < tries; k++)
                if ((rnd() % 255) < (uint32_t)(20 + _intensity / 2))
                    _px[rnd() % _n] = hsv((rnd() % 256) / 256.f, 1.f, 1.f);
        }
        void lightningStep()
        {
            if (_flash > 0)
            {
                const float b = 0.6f + 0.4f * ((rnd() % 100) / 100.f);
                for (uint16_t i = _la; i <= _lb && i < _n; i++)
                    _px[i] = scale(_color, b);
                _flash--;
                return;
            }
            for (auto &p : _px)
            {
                p.r = (uint8_t)((p.r * 90) >> 8);
                p.g = (uint8_t)((p.g * 90) >> 8);
                p.b = (uint8_t)((p.b * 90) >> 8);
            }
            if ((rnd() % 1000) < (uint32_t)(3 + _intensity / 12))
            {
                _flash = 1 + rnd() % 4;
                const uint16_t len = 1 + _n / 4 + rnd() % (1 + _n / 4);
                _la = rnd() % _n;
                _lb = _la + len;
            }
        }
        void candleStep()
        {
            if (rnd() % 8 == 0)
                _tgt = 0.45f + 0.55f * ((rnd() % 100) / 100.f);
            _lvl += (_tgt - _lvl) * 0.25f;
            for (uint16_t i = 0; i < _n; i++)
                _px[i] = scale(_color, _lvl * (0.88f + 0.12f * ((rnd() % 100) / 100.f)));
        }
        void juggleStep()
        {
            for (auto &p : _px)
            {
                p.r = (uint8_t)((p.r * 200) >> 8);
                p.g = (uint8_t)((p.g * 200) >> 8);
                p.b = (uint8_t)((p.b * 200) >> 8);
            }
            const int dots = 3 + _intensity / 64;
            for (int k = 0; k < dots; k++)
            {
                const float pos = (sinf(6.2831853f * _phase * (0.35f + 0.21f * k) + k * 1.3f) + 1.f) * 0.5f * (_n - 1);
                const int idx = (int)pos;
                _px[idx] = addSat(_px[idx], hsv((float)k / dots, 1.f, 1.f));
            }
        }
    };
} // namespace led
