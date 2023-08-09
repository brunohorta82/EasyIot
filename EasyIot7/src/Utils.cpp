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
String getChipId()
{
#ifdef ESP8266
    return String(ESP.getChipId());
#endif
#ifdef ESP32
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    return String(chipId);
#endif
}
void generateId(String &id, const String &name, int familyCode, size_t maxSize)
{
    id.reserve(maxSize);
    id.concat(getChipId());
    id.concat(name);
    id.concat(familyCode);
    normalize(id);
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
