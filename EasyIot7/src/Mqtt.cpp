#include "Mqtt.h"
#include "Switches.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);
std::vector<String> subscriptions;
void processMqttAction(String topic, String payload)
{
    mqttSwitchControl(topic, payload);
}
void callbackMqtt(char *topic, byte *payload, unsigned int length)
{
    String topicStr = String(topic);
    logger(MQTT_TAG, "TOPIC: " + topicStr);
    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++)
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
    if (mqttClient.connect(String(ESP.getChipId()).c_str(), username, password, getAvailableTopic(username).c_str(), 0, true, UNAVAILABLE_PAYLOAD, true))
    {
        logger(MQTT_TAG, "CONNECTED");
        publishOnMqtt(getAvailableTopic(username), AVAILABLE_PAYLOAD, true);
        publishOnMqtt(getConfigStatusTopic(username), getConfigStatus().c_str(), true);
        for(int i = 0 ; i < subscriptions.size(); i++){
            mqttClient.subscribe(subscriptions[i].c_str());
        }
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
    static unsigned int retries = 0;
    while(!mqttClient.publish(topic.c_str(),payload.c_str(), retain) && retries < 3){
        retries++;
    }
    
    retries = 0;
}

void subscribeOnMqtt(String topic)
{
    subscriptions.push_back(topic);
}
