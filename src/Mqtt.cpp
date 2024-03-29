#include "Mqtt.h"
#include "ConfigOnofre.h"
#include <PubSubClient.h>
#include "Actuatores.h"
#include "CoreWiFi.h"
#include "Constants.h"
#include "HomeAssistantMqttDiscovery.h"
#include "WebServer.h"
extern ConfigOnofre config;
static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

void callbackMqtt(char *topic, byte *payload, unsigned int length)
{
#ifdef DEBUG_ONOFRE
    Log.notice("%s Message received" CR, tags::mqtt);
#endif
    char *payload_as_string = (char *)malloc(length + 1);
    memcpy(payload_as_string, (char *)payload, length);
    payload_as_string[length] = 0;
#ifdef DEBUG_ONOFRE
    Log.notice("%s Topic: %s" CR, tags::mqtt, topic);
    Log.notice("%s Payload: %s" CR, tags::mqtt, payload_as_string);
#endif
    if (homeAssistantOnline(topic, payload_as_string))
    {
        initHomeAssistantDiscovery();
    }
    else
    {
        config.controlFeature(StateOrigin::MQTT, topic, payload_as_string);
    }
    free(payload_as_string);
}

boolean reconnect()
{

    if (!wifiConnected() || strlen(config.mqttIpDns) == 0)
        return false;
#ifdef DEBUG_ONOFRE
    Log.notice("%s Trying to connect on broker %s" CR, tags::mqtt, config.mqttIpDns);
#endif
    sendToServerEvents("mqtt_health", constantsMqtt::unavailablePayload);
    if (mqttClient.connect(config.chipId, String(config.mqttUsername).c_str(), String(config.mqttPassword).c_str(), config.healthTopic, 0, true, constantsMqtt::unavailablePayload, true))
    {
#ifdef DEBUG_ONOFRE
        Log.notice("%s Connected to %s" CR, tags::mqtt, config.mqttIpDns);
#endif
        publishOnMqtt(config.healthTopic, constantsMqtt::availablePayload, true);
        subscribeOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/status").c_str());
        subscribeOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefixLegacy) + "/status").c_str());
        sendToServerEvents("mqtt_health", constantsMqtt::availablePayload);
        for (auto &sw : config.actuatores)
        {
            if (sw.isVirtual())
            {
                continue;
            }
            subscribeOnMqtt(sw.writeTopic);
            sw.notifyState(StateOrigin::MQTT);
        }
        initHomeAssistantDiscovery();
    }

    return mqttConnected();
}

void setupMQTT(bool forceDisconnect)
{

#ifdef DEBUG_ONOFRE
    Log.notice("%s Setup" CR, tags::mqtt);
#endif
    if (mqttConnected() && forceDisconnect)
    {
        mqttClient.disconnect();
    }
    if (strlen(config.mqttIpDns) == 0)
    {
        return;
    }
    mqttClient.setServer(config.mqttIpDns, config.mqttPort);
    mqttClient.setCallback(callbackMqtt);
}

void loopMqtt()
{
    if (!wifiConnected() || strlen(config.mqttIpDns) == 0)
        return;
    static unsigned long lastReconnectAttempt = millis();
    if (!mqttConnected())
    {
        long now = millis();
        if (now - lastReconnectAttempt > 8000)
        {
#ifdef DEBUG_ONOFRE
            Log.notice("%s Disconnected" CR, tags::mqtt);
#endif
            lastReconnectAttempt = now;
            if (reconnect())
            {
                lastReconnectAttempt = 0;
            }
        }
    }
    else
    {
        mqttClient.loop();
    }
}

void publishOnMqtt(const char *topic, const char *payload, bool retain)
{
    if (!mqttConnected())
    {
#ifdef DEBUG_ONOFRE
        Log.warning("%s Connection Required" CR, tags::mqtt);
#endif
        return;
    }
    if (mqttClient.publish(topic, payload, retain))
    {
#ifdef DEBUG_ONOFRE
        Log.error("%s Message %s published to the topic %s successfully" CR, tags::mqtt, payload, topic);
#endif
    }
#ifdef DEBUG_ONOFRE
    else
    {

        Log.error("%s Error on try publish to the topic %s with message %s" CR, tags::mqtt, topic, payload);
    }
#endif
}
bool mqttConnected()
{
    return mqttClient.connected();
}
void subscribeOnMqtt(const char *topic)
{
    if (strlen(config.mqttIpDns) == 0)
    {
#ifdef DEBUG_ONOFRE
        Log.warning("%s Setup required to subscrive messages" CR, tags::mqtt);
#endif
        return;
    }
    if (!mqttConnected())
    {
#ifdef DEBUG_ONOFRE
        Log.warning("%s Required Mqtt connection" CR, tags::mqtt);
#endif
        return;
    }
    mqttClient.subscribe(topic);
}
void unsubscribeOnMqtt(const char *topic)
{
    if (mqttConnected())
    {
#ifdef DEBUG_ONOFRE
        Log.notice("%s Unsubscribe on topic %s " CR, tags::mqtt, topic);
#endif
        mqttClient.unsubscribe(topic);
    }
}
