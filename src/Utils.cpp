#include "Utils.hpp"
void configPIN(uint8_t pin, uint8_t mode)
{
#ifdef ESP8266
    if (pin == 16)
    {
        if (mode == INPUT_PULLUP)
        {
            mode = INPUT_PULLDOWN_16;
        }
    }
#endif
    if (pin == DefaultPins::noGPIO)
    {
        return;
    }
    pinMode(pin, mode);
}

void writeToPIN(uint8_t pin, uint8_t val)
{
    if (pin == DefaultPins::noGPIO)
    {
        return;
    }
    digitalWrite(pin, val);
}

void writePWMToPIN(uint8_t pin, uint8_t percent)
{
    if (pin == DefaultPins::noGPIO)
    {
        return;
    }
#ifdef ESP32
  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    static bool pinSetup[40] = {false};
    if (pin < 40 && !pinSetup[pin])
    {
        ledcAttach(pin, 5000, 8);
        pinSetup[pin] = true;
    }
    ledcWrite(pin, (percent * 255) / 100);
  #else
    uint8_t channel = pin % 16;
    static bool channelSetup[16] = {false};
    if (!channelSetup[channel])
    {
        ledcSetup(channel, 5000, 8);
        ledcAttachPin(pin, channel);
        channelSetup[channel] = true;
    }
    ledcWrite(channel, (percent * 255) / 100);
  #endif
#else
    analogWrite(pin, (percent * 1023) / 100);
#endif
}

bool readPIN(uint8_t pin)
{
    if (pin == DefaultPins::noGPIO)
    {
        return true;
    }
    return digitalRead(pin);
}
int readPINToInt(uint8_t pin)
{
    if (pin == DefaultPins::noGPIO)
    {
        return -1;
    }
    return digitalRead(pin) ? 1 : 0;
}
void normalize(String &inputStr)
{
    inputStr.toLowerCase();
    inputStr.trim();
    inputStr.replace("_", "");
    inputStr.replace(".", "");
    inputStr.replace("/", "");
    inputStr.replace("\\", "");
    inputStr.replace("º", "");
    inputStr.replace("ª", "");
    inputStr.replace("ç", "c");
    inputStr.replace("á", "a");
    inputStr.replace("à", "a");
    inputStr.replace("&", "");
    inputStr.replace("%", "");
    inputStr.replace("$", "");
    inputStr.replace("#", "");
    inputStr.replace("!", "");
    inputStr.replace("+", "");
    inputStr.replace(",", "");
    inputStr.replace("\"", "");
    inputStr.replace(" ", "");
    inputStr.replace("â", "a");
    inputStr.replace("ã", "a");
    inputStr.replace("ú", "u");
    inputStr.replace("ù", "u");
    inputStr.replace("é", "e");
    inputStr.replace("è", "e");
    inputStr.replace("ê", "e");
    inputStr.replace("í", "i");
    inputStr.replace("ì", "i");
    inputStr.replace("õ", "o");
    inputStr.replace("ó", "o");
    inputStr.replace("ò", "o");
    inputStr.replace("@", "o");
    inputStr.replace("|", "");
}

int rssiToWiFiQuality(int rssi)
{
    int quality = 0;

    if (rssi <= -100)
    {
        quality = 0;
    }
    else if (rssi >= -50)
    {
        quality = 100;
    }
    else
    {
        quality = 2 * (rssi + 100);
    }
    return quality;
}

boolean isValidNumber(const char *str)
{
    size_t length = strlen(str);
    for (byte i = 0; i < length; i++)
    {
        if (isDigit(str[i]))
            return true;
    }
    return false;
}
