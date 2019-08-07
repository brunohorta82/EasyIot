#include "Mqtt.h"
#include "Switches.h"
#include "constants.h"

static WiFiClient espClient;
static PubSubClient mqttClient(espClient);
void processMqttAction(const char *topic, const char *payload)
{
    mqttSwitchControl(getAtualSwitchesConfig(), topic, payload);
}
void callbackMqtt(char *topic, byte *payload, unsigned int length)
{
    Log.notice("%s Message received" CR, tags::mqtt);

    char *payload_as_string = (char *)malloc(length + 1);
    memcpy(payload_as_string, (char *)payload, length);
    payload_as_string[length] = 0;
    Log.notice("%s Topic: %s" CR, tags::mqtt, topic);
    Log.notice("%s Payload: " CR, tags::mqtt, payload_as_string);
    if (strcmp(topic, constantsMqtt::homeassistantOnlineTopic) == 0 && strcmp(payload_as_string, constantsMqtt::availablePayload) == 0)
    {
        initSwitchesHaDiscovery(getAtualSwitchesConfig());
    }
    else
    {
        processMqttAction(topic, payload_as_string);
    }

    free(payload_as_string);
}

String getBaseTopic()
{
    String topic;

    if (strlen(getAtualConfig().mqttUsername) == 0)
    {
        topic.concat("easyiot");
    }
    else
    {
        topic.concat(getAtualConfig().mqttUsername);
    }
    topic.concat("/");
    topic.concat(ESP.getChipId());
    return topic;
}
String getAvailableTopic()
{
    String baseTopic = getBaseTopic();
    baseTopic.concat("/available");
    return baseTopic;
}
String getConfigStatusTopic()
{
    String baseTopic = getBaseTopic();
    baseTopic.concat("/config/status");
    return baseTopic;
}

boolean reconnect()
{

    if (WiFi.status() != WL_CONNECTED || strlen(getAtualConfig().mqttIpDns) == 0)
        return false;
    Log.notice("%s Trying to connect on broker %s" CR, tags::mqtt, getAtualConfig().mqttIpDns);
    char *username = strdup(getAtualConfig().mqttUsername);
    char *password = strdup(getAtualConfig().mqttPassword);
    if (mqttClient.connect(getAtualConfig().chipId, username, password, getAvailableTopic().c_str(), 0, true, constantsMqtt::unavailablePayload, true))
    {
        Log.notice("%s Connected to %s" CR, tags::mqtt, getAtualConfig().mqttIpDns);
        publishOnMqtt(getAvailableTopic().c_str(), constantsMqtt::availablePayload, true);
        publishOnMqtt(getConfigStatusTopic().c_str(), "{\"firmware\":7.0}", true); //TODO generate simple config status
        subscribeOnMqtt(constantsMqtt::homeassistantOnlineTopic);
    }

    return mqttClient.connected();
}

void setupMQTT()
{
    if (strlen(getAtualConfig().mqttIpDns) == 0)
    {
        return;
    }
    Log.notice("%s Setup" CR, tags::mqtt);
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
    if (WiFi.status() != WL_CONNECTED || strlen(getAtualConfig().mqttIpDns) == 0)
        return;
    static unsigned long lastReconnectAttempt = millis();
    if (!mqttClient.connected())
    {
        long now = millis();
        if (now - lastReconnectAttempt > 5000)
        {
            Log.notice("%s Disconnected" CR, tags::mqtt);
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
    if (strlen(getAtualConfig().mqttIpDns) == 0)
    {
        Log.warning("%s Setup required to publish messages" CR, tags::mqtt);
        return;
    }
    if (!mqttClient.connected())
    {
        Log.warning("%s Connection Required" CR, tags::mqtt);
        return;
    }
    static unsigned int retries = 0;
    while (!mqttClient.publish(topic, payload, retain) && retries < 3)
    {
        retries++;
    }
    if (retries < 3)
    {
        Log.error("%s Message %s published to the topic %s successfully" CR, tags::mqtt, payload, topic);
    }
    else
    {
        Log.error("%s Error on try publish to the topic %s with message %s" CR, tags::mqtt, topic, payload);
    }

    retries = 0;
}
bool getMqttState()
{
    return mqttClient.connected();
}
void subscribeOnMqtt(const char *topic)
{
    if (strlen(getAtualConfig().mqttIpDns) == 0)
    {
        Log.warning("%s Setup required to subscrive messages" CR, tags::mqtt);
        return;
    }
    if (!mqttClient.connected())
    {
        Log.warning("%s Required Mqtt connection" CR, tags::mqtt);
        return;
    }
    mqttClient.subscribe(topic);
}
void unsubscribeOnMqtt(const char *topic)
{
    if (mqttClient.connected())
    {
        Log.notice("%s Unsubscribe on topic %s " CR, tags::mqtt, topic);
        mqttClient.unsubscribe(topic);
    }
}
