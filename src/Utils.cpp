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
