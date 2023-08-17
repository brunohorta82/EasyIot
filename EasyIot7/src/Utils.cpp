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
    if (pin == constantsConfig::noGPIO)
    {
        return;
    }
    pinMode(pin, mode);
}

void writeToPIN(uint8_t pin, uint8_t val)
{
    if (pin == constantsConfig::noGPIO)
    {
        return;
    }
    digitalWrite(pin, val);
}

bool readPIN(uint8_t pin)
{
    if (pin == constantsConfig::noGPIO)
    {
        return true;
    }
    return digitalRead(pin);
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
