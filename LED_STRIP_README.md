# Fita LED (estilo WLED) para o EasyIot

Módulo opcional: só entra no firmware com `-D LED_STRIP`. Sem essa flag o código fica igual ao original.

## Como se usa
O módulo vem incluído nos firmwares ESP32, ESP32-C3 e ESP32-C6 e só liga quando crias a função. Os ESP8266 não o trazem (não têm memória para ele) e o painel nem o oferece.

1. Grava um ambiente ESP32 (placas de 4 MB, como o ESP32-PICO: `ESP32_4M_DEBUG` ou `ESP32_4M_RELEASE`;
   placas de 8 MB: `ESP32_DEBUG` ou `ESP32_RELEASE`; ESP32-C3: `ESP32C3_LED_DEBUG` ou `ESP32C3_LED_RELEASE`; ESP32-C6: `ESP32C6_IRRIGATION` (ou `_DEBUG`); o `ESP32C3_HAN` é para o contador de energia e
   configura o Wi-Fi por Bluetooth, sem access point).
2. No painel, em **Funções → Nova função**, escolhe o tipo **Fita LED**, dá-lhe um nome, escolhe o **GPIO**
   de dados e o **número de LEDs**, e carrega em **Adicionar função**.
3. A fita aparece na **Visão geral**, em Acessórios, como um mosaico: clicar liga e desliga. A **rodinha** do mosaico
   abre uma janela com cor, brilho, efeito, velocidade, intensidade e número de LEDs.
4. Na lista de funções ficam o endereço KNX e o botão **Remover função**. Para mudar o pino, remove a função e cria-a de novo.

Na lista de funções há o campo **Endereço KNX** (área / linha / membro), como nas luzes: liga/desliga a fita e devolve o estado ao bus.

**Segmentos:** na janela da rodinha podes dividir a fita em até 8 zonas (**+ Dividir**), com o LED inicial e final de
cada uma. Escolhe o segmento no seletor e define a cor, o brilho, o efeito, a velocidade e a intensidade só dessa zona, ou
"Todos os segmentos" para mexer em tudo. O botão do mosaico liga e desliga a fita toda; cada segmento tem também o seu
interruptor. Pelo MQTT, usa `"seg": n` num comando para o dirigir a um segmento, e
`"segments": [{"start":0,"stop":29},{"start":30,"stop":59}]` para definir as zonas. O Home Assistant e o KNX
controlam a fita toda.

Só pode existir uma fita. O GPIO escolhido fica reservado: nenhuma outra função o pode usar.
Evita 6 a 11, 16 e 17 (flash interna nos módulos PICO), 0, 2, 12, 15 (arranque) e 21/22 (I2C do firmware).

## Atualizar e nuvem
- **Atualizar por web (a que funciona com este firmware):** no painel, em Sistema → **Atualizar por ficheiro**, envia o
  `firmware.bin` que o PlatformIO gera em `.pio/build/<ambiente>/firmware.bin`. A fita e as definições mantêm-se.
  Não uses o botão **Procurar atualização**: descarrega o firmware oficial, sem a fita LED.
- Só se atualiza por web **dentro do mesmo ambiente** (a tabela de partições tem de ser a mesma). Para mudar de ambiente
  grava por USB com `-t erase`. Se o ficheiro for maior do que a ranhura (1,75 MB nos ambientes `4M` e no C3), a
  atualização falha: confirma a percentagem de `Flash:` no fim da compilação.
- **Nuvem (CloudIO):** o dispositivo liga-se ao servidor, mas responde "not adopted" até o equipamento ser associado a uma
  conta no painel CloudIO (bhonofre.pt). Isso faz-se no serviço da OnOfre, e não no firmware.
  Depois de adotado, a fita vai para a nuvem como uma função da família **LIGHT** com o driver **LED_STRIP**, que o
  CloudIO trata como luz de cor: comandos em `<utilizador>/<chipId>/light/led/set` e estado retido em `.../light/led/status`,
  com o mesmo JSON do MQTT local.

## Transições e limite de corrente
- **Transição (ms):** ligar, desligar, mudar de cor, de brilho e ligar/desligar segmentos faz-se com um desvanecimento suave
  (400 ms por omissão; 0 = sem transição).
- **Limite da fonte (A):** diz quanto a tua fonte aguenta (por exemplo 2 A) e o firmware reduz o brilho sempre que a luz
  pedida passaria disso. 0 = sem limite. O consumo é estimado a partir das cores acesas e de **Consumo de 1 LED a branco**
  (60 mA numa WS2812B; ajusta se a tua fita gastar outro valor). A janela mostra o consumo estimado e avisa quando está a
  reduzir. É uma estimativa: deixa uma margem para a fonte, e não substitui um fusível.
- Por MQTT: `"transition": 400`, `"limit": 2000` (mA) e `"maPerLed": 60`.

## Ajuste de cores
Na janela da rodinha, em **Ajuste de cores**:
- **Ordem das cores:** se o vermelho sair verde ou azul, escolhe outra (GRB é o normal nas WS2812B). Aplica-se logo, sem reiniciar.
- **Correção gama (2,2 ou 2,8):** os LEDs são lineares e o olho não, por isso as cores fracas parecem lavadas e um vermelho
  com um pouco de verde ou azul fica rosa. Com a gama, as cores ficam mais fiéis e as transições mais naturais.
- **Vermelho, verde e azul (%):** balanço de brancos. Baixa o verde e o azul se o vermelho sair rosado, ou o canal que
  domina se o branco sair com cor. O consumo estimado e o limite de corrente já contam com estes ajustes.
- Por MQTT: `"colorOrder": 0..5`, `"gamma": 10|22|28` e `"cal": [100, 90, 80]`.

## Efeitos (24)
Fixo, Piscar, Respirar, Arco-íris, Varrimento, Perseguição, Scanner, Cintilar, Fogo, Meteoro, Estroboscópio,
Ciclo de cor, Teatro arco-íris, Luzes em movimento, Faíscas, Plasma, Confetti, Relâmpago, Vela, Polícia,
Batimento, Malabarismo, Gradiente e Nuvens. Os efeitos novos estão no fim da lista, por isso o que já tinhas
guardado mantém-se. A **velocidade** e a **intensidade** mexem em cada efeito à sua maneira (por exemplo, no
Relâmpago a intensidade é a frequência dos clarões). O Fogo, a Polícia e o Arco-íris usam as suas próprias cores.
Para acrescentar mais, edita `include/LedEffects.h` (o motor, que se testa no PC com `tools/test_led_effects.cpp`) e a
lista `LED_EFFECTS` em `webpanel/js/index.js`.

## Página web
Abre `http://<nome>.local/led` (mesmo utilizador e palavra-passe do painel). Tem ligar/desligar, cor, brilho,
efeito, velocidade, intensidade e número de LEDs, e funciona bem no telemóvel.
API: `GET /led/state` e `POST /led/set` (o mesmo JSON do MQTT).

## Home Assistant
Aparecem sozinhos: uma luz **Fita LED** (cor, brilho, efeito) e dois sliders, **Velocidade** e **Intensidade**.
Efeitos: Static, Blink, Breathe, Rainbow, Color Wipe, Chase, Scan, Twinkle, Fire, Meteor.
Para a Alexa, a integração é pela nuvem: depois de adotada, a fita aparece como luz com ligar/desligar,
brilho e cor. Efeitos e segmentos não têm vocabulário na Alexa e ficam no painel e no Home Assistant.

## MQTT
Comando em `onofre/<chipId>/led/set`, estado em `.../led/state`:

```json
{"state":"ON","brightness":180,"color":{"r":255,"g":0,"b":80},"effect":"Fire","speed":200,"intensity":120,"leds":90}
```
`ON` / `OFF` simples também funciona. O estado é gravado em `/led.json` (3 s depois da última alteração).

## Testar o motor de efeitos no PC
`g++ -std=c++11 -O1 -fsanitize=address,undefined test_effects.cpp -o t && ./t`
