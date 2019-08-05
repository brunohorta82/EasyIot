#ifndef CONSTANTS_H
#define CONSTANTS_H
namespace tags
{
constexpr const char* system{"[SYSTEM]"}; 
constexpr const char* config{"[CONFIG]"} ;
constexpr const char* mqtt{"[MQTT]"} ;
constexpr const char* wifi{"[WIFI]"} ;
constexpr const char* discovery{"[DISCOVERY]"} ;
constexpr const char* sensors{"[SENSORS]"} ;
constexpr const char* switches{"[SWITCHES]"} ;
constexpr const char* alexa{"[ALEXA]"} ;
constexpr const char* webserver{"[WEBSERVER]"} ;
}
namespace constantsConfig
{
constexpr const char* updateURL {"http://release.bhonofre.pt/firmware.bin"}; 
constexpr const char* mqttCloudURL {"mqtt.bhonofre.pt"}; 
constexpr const char* configFileName  {"/config_bhonofre.json"}; 
constexpr const char* newID {"NEW"}; 
constexpr  int noGPIO {99}; 
constexpr const char* homeassistantOnlineTopic {"hass/status"}; 
//AP PASSWORD  
constexpr const char* apSecret {"EasyIot@"}; 
 
}
#endif