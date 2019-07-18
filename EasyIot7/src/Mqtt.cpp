#include "Mqtt.h"
#include "Switches.h"
#define MQTT_TAG "[MQTT]"
WiFiClient espClient;
PubSubClient mqttClient(espClient);
void processMqttAction(String topic, String payload)
{
    mqttSwitchControl(topic, payload);
}
void callbackMqtt(char *topic, byte *payload, unsigned int length)
{
    logger(MQTT_TAG, "MESSSAGE RECEIVEID");
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
String getBaseTopic()
{
    String username = String(getAtualConfig().mqttUsername);
    if (username == "")
    {
        username = "easyiot";
    }
    return username + "/" + String(ESP.getChipId());
}
String getAvailableTopic()
{
    String username = String(getAtualConfig().mqttUsername);
    return getBaseTopic() + "/available";
}
String getConfigStatusTopic()
{
    String username = String(getAtualConfig().mqttUsername);
    return getBaseTopic() + "/config/status";
}

boolean reconnect()
{

    if (WiFi.status() != WL_CONNECTED || String(getAtualConfig().mqttIpDns).equals(""))
        return false;
    logger(MQTT_TAG, "TRY CONNECTION " + String(getAtualConfig().mqttIpDns));
    char *username = strdup(getAtualConfig().mqttUsername);
    char *password = strdup(getAtualConfig().mqttPassword);
    if (mqttClient.connect(String(ESP.getChipId()).c_str(), username, password, getAvailableTopic().c_str(), 0, true, UNAVAILABLE_PAYLOAD, true))
    {
        logger(MQTT_TAG, "CONNECTED");
        publishOnMqtt(getAvailableTopic(), AVAILABLE_PAYLOAD, true);
        publishOnMqtt(getConfigStatusTopic(), getConfigStatus().c_str(), true);
        initSwitchesMqttAndDiscovery();
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
    if (WiFi.status() != WL_CONNECTED || String(getAtualConfig().mqttIpDns).equals(""))
        return;
    static unsigned long lastReconnectAttempt = millis();
    if (!mqttClient.connected())
    {
        long now = millis();
        if (now - lastReconnectAttempt > 5000)
        {
            logger(MQTT_TAG, "MQTT Disconnected");
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
    if (strlen(getAtualConfig().mqttIpDns) == 0)
    {
        logger(MQTT_TAG, "Configure Mqtt to publish messages");
        return;
    }
    if (!mqttClient.connected())
    {
        logger(MQTT_TAG, "Mqtt not connected, can't publish message.");
        return;
    }
    static unsigned int retries = 0;
    while (!mqttClient.publish(topic.c_str(), payload.c_str(), retain) && retries < 3)
    {
        retries++;
    }
    if (retries < 3)
    {
        logger(MQTT_TAG, "Publish in " + topic + " message " + payload);
    }
    else
    {
        logger(MQTT_TAG, "Error on try publish in " + topic + " message " + payload);
    }

    retries = 0;
}
bool getMqttState(){
    return mqttClient.connected();
}
void subscribeOnMqtt(String topic)
{
    mqttClient.subscribe(topic.c_str());
}
void unsubscribeOnMqtt(String topic)
{
    if (mqttClient.connected())
    {
        logger(MQTT_TAG, "Unsubscribe on " + topic);
        mqttClient.unsubscribe(topic.c_str());
    }
}
