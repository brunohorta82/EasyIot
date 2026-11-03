// Teste do motor de efeitos no PC:
//   g++ -std=c++11 -Wall -Wextra -O1 -fsanitize=address,undefined tools/test_led_effects.cpp -o t && ./t
#include "../include/LedEffects.h"
#include <stdio.h>
int main()
{
    led::Engine e;
    int bad = 0;
    e.resize(60);
    for (int fx = 0; fx < led::FX_COUNT; fx++)
    {
        e.set((led::Effect)fx, led::RGB(255, 80, 0), 200, 128);
        uint32_t t = 1000;
        unsigned frames = 0, everLit = 0;
        for (int i = 0; i < 1500; i++)
        {
            t += 20;
            if (e.update(t))
                frames++;
            for (auto &p : e.pixels())
                if (p.r || p.g || p.b)
                {
                    everLit++;
                    break;
                }
        }
        printf("%-16s frames=%-5u acesa em %u amostras\n", led::kEffectNames[fx], frames, everLit);
        if (frames == 0 || everLit == 0)
            bad++;
    }
    // tamanhos extremos e valores extremos de velocidade/intensidade
    const int sizes[] = {1, 2, 3, 7, 8, 300, 600};
    for (int n : sizes)
    {
        e.resize(n);
        for (int fx = 0; fx < led::FX_COUNT; fx++)
            for (int sp : {0, 255})
                for (int ix : {0, 255})
                {
                    e.set((led::Effect)fx, led::RGB(1, 2, 3), sp, ix);
                    uint32_t t = 5000;
                    for (int i = 0; i < 120; i++)
                    {
                        t += 20;
                        e.update(t);
                    }
                }
    }
    // --- smooth colour fade ---
    {
        led::Engine t;
        t.resize(4);
        t.setTransition(400);
        t.set(led::FX_STATIC, led::RGB(255, 0, 0), 128, 128);
        uint32_t tm = 1000;
        t.update(tm);
        if (t.pixels()[0].r != 255) { printf("FALHOU: cor inicial\n"); bad++; }
        t.set(led::FX_STATIC, led::RGB(0, 0, 255), 128, 128); // vermelho -> azul em 400 ms
        int steps = 0, midSeen = 0;
        for (int i = 0; i < 100; i++)
        {
            tm += 20;
            if (t.update(tm)) steps++;
            const led::RGB &p = t.pixels()[0];
            if (p.r > 20 && p.r < 235 && p.b > 20 && p.b < 235) midSeen++;
        }
        const led::RGB &fin = t.pixels()[0];
        printf("transição: %d quadros, %d intermédios, final %d/%d/%d\n", steps, midSeen, fin.r, fin.g, fin.b);
        if (fin.r != 0 || fin.b != 255 || midSeen < 3) { printf("FALHOU: transição\n"); bad++; }
        // no transition: jump straight there
        led::Engine u;
        u.resize(2);
        u.setTransition(0);
        u.set(led::FX_STATIC, led::RGB(255, 0, 0), 128, 128);
        u.update(1000);
        u.set(led::FX_STATIC, led::RGB(0, 255, 0), 128, 128);
        u.update(1020);
        if (u.pixels()[0].g != 255) { printf("FALHOU: sem transição\n"); bad++; }
    }
    // --- limite de corrente ---
    {
        // 60 LEDs at white, 60 mA each: 60*60 plus 60 idle = 3660 mA
        const uint32_t sum = 60u * 765u;
        const uint32_t est = led::estimateMA(sum, 60, 60);
        const uint16_t f = led::limitFactor256(est, 60, 2000);
        const uint32_t after = led::estimateMA((uint64_t)sum * f / 256, 60, 60);
        printf("limite: estimado %u mA, fator %u/256, depois %u mA (limite 2000)\n", est, f, after);
        if (est != 3660 || after > 2000 || after < 1900) { printf("FALHOU: limite\n"); bad++; }
        if (led::limitFactor256(est, 60, 0) != 256) { printf("FALHOU: sem limite\n"); bad++; }
        if (led::limitFactor256(1000, 60, 2000) != 256) { printf("FALHOU: abaixo do limite\n"); bad++; }
        if (led::limitFactor256(est, 60, 50) != 0) { printf("FALHOU: limite abaixo do repouso\n"); bad++; }
    }
    printf(bad ? "FALHOU (%d)\n" : "OK\n", bad);
    return bad;
}
