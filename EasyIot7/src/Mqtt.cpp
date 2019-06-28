#include "Mqtt.h"
#define MQTT_TAG "[MQTT]"
WiFiClient espClient;
PubSubClient mqttClient(espClient);
void processMqttAction(String topic, String payload)
{
    //mqttSwitchControl(topic, payload);
}
void callbackMqtt(char *topic, byte *payload, unsigned int length)
{
    String topicStr = String(topic);
    logger(MQTT_TAG, "TOPIC: " + topicStr);
    String payloadStr = "";
    for (int i = 0; i < length; i++)
    {
        payloadStr += (char)payload[i];
    }
    logger(MQTT_TAG, "PAYLOAD: " + payloadStr);
    processMqttAction(topicStr, payloadStr);
}
String getBaseTopic(String username)
{
    if (username == "")
    {
        username = String(HARDWARE);
    }
    return username + "/" + String(ESP.getChipId());
}
String getAvailableTopic(String username)
{
    return getBaseTopic(username) + "/available";
}
String getConfigStatusTopic(String username)
{
    return getBaseTopic(username) + "/config/status";
}

boolean reconnect()
{

    if (WiFi.status() != WL_CONNECTED || String(getAtualConfig().mqttIpDns).equals(""))
        return false;
    logger(MQTT_TAG, "TRY CONNECTION");
    char *username = strdup(getAtualConfig().mqttUsername);
    char *password = strdup(getAtualConfig().mqttPassword);
    if (mqttClient.connect(String(ESP.getChipId()).c_str(), "username", "password", getAvailableTopic(username).c_str(), 0, true, "offline", false))
    {
        logger(MQTT_TAG, "CONNECTED");
        publishOnMqtt(getAvailableTopic(username), "online", true);
       // publishOnMqtt(getConfigStatusTopic(username), getConfigStatus(String(CONFIG_FILENAME).c_str()), true);
    }

    return mqttClient.connected();
}

void setupMQTT()
{
    if (String(getAtualConfig().mqttIpDns).equals(""))
    {
        return;
    }
    logger(MQTT_TAG, "SETUP MQTT");
    if (mqttClient.connected())
    {
        mqttClient.disconnect();
    }
    char *ipDnsMqtt = strdup(getAtualConfig().mqttIpDns);
    int port = getAtualConfig().mqttPort;

    mqttClient.setServer(ipDnsMqtt, port);
    mqttClient.setCallback(callbackMqtt);
}


void loopMqtt()
{
    static unsigned long lastReconnectAttempt = millis();
    if (!mqttClient.connected())
    {
        long now = millis();
        if (now - lastReconnectAttempt > 5000)
        {
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

void publishOnMqtt(String topic, String payload, bool retain)
{
    static unsigned long retries = 0;
      while(!mqttClient.publish(topic.c_str(),payload.c_str(), retain));
    
    retries = 0;
}

void subscribeOnMqtt(String topic)
{
       mqttClient.subscribe(topic.c_str());
}
