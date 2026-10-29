# 🚀 Simulação do ESP32/ESP8266 & Alternativas Open-Source

Este guia explica como simular e testar o **EasyIot** e a **Fonte Musical AquaDance** sem necessitar de hardware físico ESP32 ou ESP8266 ligado ao computador.

---

## 1. 🌟 Simulador Web & Hardware Integrado (Recomendado - Instantâneo)

Criámos um simulador interativo em Python que emula o hardware ESP, o servidor REST/SSE e a interface web completa em tempo real.

### Como Executar:
```powershell
.\.venv\Scripts\python tools/esp_simulator.py
```

### O que permite fazer:
1. Abrir **`http://localhost:8080`** no navegador.
2. Navegar para a aba **REGA** -> **FONTE AQUADANCE**.
3. **Simulador 2D do Lago:** Arrastar os bicos de água e os focos de luz RGBW pelo lago ou escolher disposições automáticas (*Círculo*, *Linha*, *Cruz*, *Arco*).
4. **Partição Musical:** Desenhar notas de água, ajustar potência de luz dimmer (`25%` a `100%`) e pintar cores RGBW com a paleta interativa.
5. Clicar em **"▶ Reproduzir Dança"** para ver a coreografia em tempo real (jatos de água animados e iluminação colorida sincronizada).
6. Exportar o cartão 2D para o Home Assistant em formato Lovelace YAML.

---

## 2. 🔌 Alternativas Open-Source & Gratuitas ao Wokwi

Para simulação ao nível de código binário compilado (CPU Xtensa, memória flash, barramentos GPIO e WiFi), existem alternativas 100% livres e de código aberto:

### A) 🛠️ Espressif QEMU (Oficial da Espressif)
* **Licença:** 100% Open-Source (GPLv2) & Gratuito.
* **Repositório:** [github.com/espressif/qemu](https://github.com/espressif/qemu)
* **O que faz:**
  - Emulador oficial do core Xtensa LX6 (ESP32) e RISC-V (ESP32-C3).
  - Executa diretamente o ficheiro `.bin` / `.elf` compilado pelo PlatformIO.
  - Suporta emulação de memória flash LittleFS, portas série UART, timers e interface de rede virtual TAP/slirp para comunicação WiFi/IP.

### B) 🌐 Renode (da Antmicro)
* **Licença:** 100% Open-Source (Licença BSD) & Gratuito.
* **Website / Repo:** [renode.io](https://renode.io) / [github.com/renode/renode](https://github.com/renode/renode)
* **O que faz:**
  - Framework de simulação de sistemas embebidos de classe industrial.
  - Suporta ESP32 com periféricos completos, sensores I2C/SPI e redes mesh multi-dispositivo.
  - Permite testes automatizados em CI/CD sem qualquer hardware.

### C) ⚡ SimulIDE
* **Licença:** 100% Open-Source (GPL).
* **Website:** [simulide.com](https://simulide.com)
* **O que faz:**
  - Simulador de circuitos eletrónicos em tempo real com suporte a microcontroladores e componentes lógicos (relés, LEDs, válvulas).
