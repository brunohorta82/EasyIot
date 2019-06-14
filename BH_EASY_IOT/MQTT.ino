#define MQTT_BROKER_PORT 1883
#define MQTT_CONFIG_TOPIC "heleeus/config/#"
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

/**
 * REASONS
  TCP_DISCONNECTED = 0,
  MQTT_UNACCEPTABLE_PROTOCOL_VERSION = 1,
  MQTT_IDENTIFIER_REJECTED = 2,
  MQTT_SERVER_UNAVAILABLE = 3,
  MQTT_MALFORMED_CREDENTIALS = 4,
  MQTT_NOT_AUTHORIZED = 5
 */
char *usernameMqtt = 0;
char *passwordMqtt = 0;
String getBaseTopic()
{
  String username = getConfigJson().get<String>("mqttUsername");
  if (username == "")
  {
    username = String(HARDWARE);
  }
  return username + "/" + String(ESP.getChipId());
}
String getAvailableTopic()
{
  return getBaseTopic() + "/available";
}

typedef struct
{
  String topic;
  String payload;
  bool retain;
} message_t;
std::vector<message_t> _messages;

void onMqttConnect(bool sessionPresent)
{
  logger("[MQTT] Connected to MQTT.");
  mqttClient.publish(getAvailableTopic().c_str(), 0, true, "1");
  reloadMqttSubscriptions();
}

void rebuildSwitchMqttTopics(JsonObject &switchJson)
{
  String ipMqtt = getConfigJson().get<String>("mqttIpDns");
  if (ipMqtt == "")
    return;
    String _id = switchJson.get<String>("id");
    String type = switchJson.get<String>("type");
    String _class = switchJson.get<String>("class");
    switchJson.set("mqttCommandTopic", getBaseTopic() + "/" + _class + "/" + _id + "/set");
    switchJson.set("mqttStateTopic", getBaseTopic() + "/" + _class + "/" + _id + "/state");
    if (type.equals("cover"))
    {
      switchJson.set("mqttPositionStateTopic", getBaseTopic() + "/" + _class + "/" + _id + "/position");
      switchJson.set("mqttPositionCommandTopic", getBaseTopic() + "/" + _class + "/" + _id + "/setposition");
      switchJson.set("mqttPositionStateTopic", getBaseTopic() + "/" + _class + "/" + _id + "/position");
      switchJson.set("mqttTiltStateTopic", getBaseTopic() + "/" + _class + "/" + _id + "/tiltstate");
      switchJson.set("mqttTiltCommandTopic", getBaseTopic() + "/" + _class + "/" + _id + "/tilt");
    }
}
void rebuildSensorMqttTopics(JsonObject &sensorJson) {
String ipMqtt = getConfigJson().get<String>("mqttIpDns");
  if (ipMqtt == "")
    return; 
    JsonArray &functions = sensorJson.get<JsonVariant>("functions");
    for (int i = 0; i < functions.size(); i++)
    {
      JsonObject &f = functions.get<JsonVariant>(i);
      String uniqueName = f.get<String>("uniqueName");
      f.set("mqttStateTopic", getBaseTopic() + "/" + uniqueName + "/state");
    }
  
  
}

void connectToMqtt()
{
  logger("[MQTT] Connecting to MQTT [" + getConfigJson().get<String>("mqttIpDns") + "]...");
  if (!getMqttState())
  {
    mqttClient.connect();
  }
}
bool getMqttState()
{
  return mqttClient.connected();
}
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  logger("[MQTT] Disconnected from MQTT. Reason: " + String(static_cast<uint8_t>(reason)));
  if (WiFi.isConnected())
  {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  String topicStr = String(topic);
  logger("[MQTT] TOPIC: " + topicStr);
  String payloadStr = "";
  for (int i = 0; i < len; i++)
  {
    payloadStr += payload[i];
  }
  logger("[MQTT] PAYLOAD: " + payloadStr);
  processMqttAction(topicStr, payloadStr);
}

void setupMQTT()
{
  if (mqttClient.connected())
  {
    mqttClient.disconnect();
  }
  if (WiFi.status() != WL_CONNECTED || getConfigJson().get<String>("mqttIpDns").equals(""))
    return;

  mqttClient.onConnect(onMqttConnect);
  const static String clientId = String(ESP.getChipId());
  mqttClient.setClientId(clientId.c_str());
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  char *ipDnsMqtt = strdup(getConfigJson().get<String>("mqttIpDns").c_str());

  if (!(getConfigJson().get<String>("mqttUsername").equals("") && getConfigJson().get<String>("mqttPassword").equals("")))
  {
    usernameMqtt = strdup(getConfigJson().get<String>("mqttUsername").c_str());
    passwordMqtt = strdup(getConfigJson().get<String>("mqttPassword").c_str());
    mqttClient.setCredentials(usernameMqtt, passwordMqtt);
  }
  mqttClient.setCleanSession(true);
  mqttClient.setServer(ipDnsMqtt, MQTT_BROKER_PORT);
  connectToMqtt();
  reloadMqttConfiguration = false;
}

void reloadMqttConfig()
{
  reloadMqttConfiguration = true;
}
void publishOnMqttQueue(String topic, String payload, bool retain)
{
  if (mqttClient.connected() && !topic.equals("null"))
  {
    _messages.push_back({topic, payload, retain});
  }
}
void publishOnMqtt(String topic, String payload, bool retain)
{
  if (mqttClient.connected() && !topic.equals("null"))
  {
    mqttClient.publish(topic.c_str(), 0, retain, payload.c_str());
  }
}
void publishOnMqtt(String topic, JsonObject &payloadJson, bool retain)
{
  if (mqttClient.connected() && !topic.equals("null"))
  {
    String payload = "";
    payloadJson.printTo(payload);
    mqttClient.publish(topic.c_str(), 0, retain, payload.c_str());
  }
}
long lastMessage = 0;
void mqttMsgDigest()
{
  if (_messages.empty())
  {
    lastMessage = 0;
    return;
  }
  if (lastMessage + 500 < millis())
  {
    message_t m = _messages.back();
    mqttClient.publish(m.topic.c_str(), 0, true, m.payload.c_str());
    _messages.pop_back();
    lastMessage = millis();
  }
}
void subscribeOnMqtt(String topic)
{
  mqttClient.subscribe(topic.c_str(), 0);
}
void processMqttAction(String topic, String payload)
{
  mqttSwitchControl(topic, payload);
}
