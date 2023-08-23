#include "Notify.h"
#include "Mqtt.h"
#include "WebServer.h"
void publishMessage(String topic, String payload)
{
    sendToServerEvents(topic, payload.c_str());
}