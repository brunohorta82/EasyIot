#include "CloudIO.h"
#include "LedStrip.h"
#include "DeviceLog.h"
#include "Irrigation.h"
#include "HomeAssistantMqttDiscovery.h"
#include "ConfigOnofre.h"
#include "CoreWiFi.h"
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "Constants.h"
#include "Actuatores.h"
#include "Sensors.h"
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#endif
#ifdef ESP32
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <atomic>
#endif
extern ConfigOnofre config;
AsyncMqttClient mqttClient;
Ticker checkCloudIOWatchdog;

namespace
{
#ifdef ESP8266
// The ESP8266 has one thread: this POST runs where the web panel, the local
// MQTT and the actuators run. It cannot be moved off, so it is kept short --
// one attempt at six seconds instead of three at twelve plus a fallback, which
// stalled the device for the best part of a minute. The exponential backoff
// then spreads the retries, so a cloud outage costs six seconds every few
// minutes rather than fifty-one every single minute.
const uint16_t kCloudIoRequestTimeoutMs = 6000;
#else
const uint16_t kCloudIoRequestTimeoutMs = 12000;
#endif
#ifdef ESP32
const int32_t kCloudIoConnectTimeoutMs = 8000;
#endif
#ifdef ESP8266
const uint8_t kCloudIoHttpsRetryCount = 1;
#else
const uint8_t kCloudIoHttpsRetryCount = 3;
#endif
const uint16_t kCloudIoRetryBackoffMs = 1500;
const uint32_t kCloudMqttTransitionTimeoutMs = 30000;

enum class CloudMqttTransportEvent : uint8_t
{
  NONE = 0,
  CONNECTED,
  DISCONNECTED
};

enum class CloudMqttTransportState : uint8_t
{
  DISCONNECTED = 0,
  CONNECTING,
  CONNECTED,
  DISCONNECTING
};

// AsyncMqttClient keeps the pointers passed to setClientId(),
// setCredentials(), and setWill(). ConfigOnofre fields can change during a
// live configuration update, so they are not safe backing storage. The active
// slot remains immutable until an asynchronous disconnect has completed; new
// values are staged in the other slot and promoted only while disconnected.
struct CloudMqttRuntimeConfig
{
  char clientId[32]{};
  char username[40]{};
  char password[64]{};
  char healthTopic[128]{};
};

CloudMqttRuntimeConfig cloudMqttRuntimeConfigs[2];
uint8_t activeCloudMqttRuntime = 0;
uint8_t stagedCloudMqttRuntime = 0;
bool activeCloudMqttRuntimeValid = false;
bool stagedCloudMqttRuntimePending = false;
bool cloudMqttCallbacksConfigured = false;
bool cloudMqttSubscriptionsPending = false;
bool cloudMqttReconnectPending = false;
CloudMqttTransportState cloudMqttTransportState = CloudMqttTransportState::DISCONNECTED;
uint32_t cloudMqttTransitionStartedAt = 0;
bool cloudMqttTransitionDeadlineArmed = false;
bool cloudMqttRecoveryQueued = false;

#ifdef ESP32
std::atomic<uint8_t> cloudMqttTransportEvent{static_cast<uint8_t>(CloudMqttTransportEvent::NONE)};
std::atomic<bool> cloudMqttConnectedState{false};
std::atomic<bool> cloudIOWatchdogDue{false};
std::atomic<uint32_t> rejectedCloudCommandCount{0};
std::atomic<uint32_t> overflowedCloudCommandCount{0};
#else
CloudMqttTransportEvent cloudMqttTransportEvent = CloudMqttTransportEvent::NONE;
bool cloudMqttConnectedState = false;
bool cloudIOWatchdogDue = false;
uint32_t rejectedCloudCommandCount = 0;
uint32_t overflowedCloudCommandCount = 0;
#endif

void signalCloudMqttTransportEvent(CloudMqttTransportEvent event, bool connected)
{
#ifdef ESP32
  // A disconnect must block publishers immediately. A connect becomes visible
  // only after the main loop consumes the event and owns subscription setup.
  if (!connected)
    cloudMqttConnectedState.store(false, std::memory_order_release);
  cloudMqttTransportEvent.store(static_cast<uint8_t>(event), std::memory_order_release);
#else
  if (!connected)
    cloudMqttConnectedState = false;
  cloudMqttTransportEvent = event;
#endif
}

CloudMqttTransportEvent takeCloudMqttTransportEvent()
{
#ifdef ESP32
  return static_cast<CloudMqttTransportEvent>(
      cloudMqttTransportEvent.exchange(static_cast<uint8_t>(CloudMqttTransportEvent::NONE),
                                       std::memory_order_acq_rel));
#else
  const CloudMqttTransportEvent event = cloudMqttTransportEvent;
  cloudMqttTransportEvent = CloudMqttTransportEvent::NONE;
  return event;
#endif
}

bool loadCloudMqttConnectedState()
{
#ifdef ESP32
  return cloudMqttConnectedState.load(std::memory_order_acquire);
#else
  return cloudMqttConnectedState;
#endif
}

void storeCloudMqttConnectedState(bool connected)
{
#ifdef ESP32
  cloudMqttConnectedState.store(connected, std::memory_order_release);
#else
  cloudMqttConnectedState = connected;
#endif
}

void enterCloudMqttTransportState(CloudMqttTransportState state)
{
  cloudMqttTransportState = state;
  cloudMqttTransitionDeadlineArmed =
      state == CloudMqttTransportState::CONNECTING ||
      state == CloudMqttTransportState::DISCONNECTING;
  if (cloudMqttTransitionDeadlineArmed)
    cloudMqttTransitionStartedAt = millis();
}

bool cloudMqttTransitionTimedOut()
{
  return cloudMqttTransitionDeadlineArmed &&
         static_cast<uint32_t>(millis() - cloudMqttTransitionStartedAt) >=
             kCloudMqttTransitionTimeoutMs;
}

// How many controlled restarts in a row before the cloud is given up on for
// this boot. Three is enough to clear a genuinely stuck transport; more than
// that is not a stuck transport, it is an outage, and restarting into one is a
// loop the user sees as "the device will not start".
const uint32_t kCloudMqttMaxRecoveryRestarts = 3;
// RTC memory survives ESP.restart() but not a power cut, which is the lifetime
// this counter wants: a run of failed recoveries must stop retrying, and
// unplugging the device must give it a clean start. Flash is the wrong place --
// a restart loop would write to it every thirty seconds, forever.
const uint32_t kCloudRecoveryMagic = 0x0C10DA01;

#ifdef ESP32
RTC_NOINIT_ATTR static uint32_t cloudRecoveryMagicCell;
RTC_NOINIT_ATTR static uint32_t cloudRecoveryRestartCell;
#endif

static uint32_t readCloudRecoveryRestarts()
{
#ifdef ESP32
  if (cloudRecoveryMagicCell != kCloudRecoveryMagic)
  {
    // Uninitialised after a power-on: RTC_NOINIT keeps whatever was in the
    // cells, so the magic is what tells a real count from noise.
    cloudRecoveryMagicCell = kCloudRecoveryMagic;
    cloudRecoveryRestartCell = 0;
  }
  return cloudRecoveryRestartCell;
#else
  uint32_t cells[2] = {0, 0};
  if (!ESP.rtcUserMemoryRead(0, cells, sizeof(cells)) || cells[0] != kCloudRecoveryMagic)
    return 0;
  return cells[1];
#endif
}

static void writeCloudRecoveryRestarts(uint32_t value)
{
#ifdef ESP32
  cloudRecoveryMagicCell = kCloudRecoveryMagic;
  cloudRecoveryRestartCell = value;
#else
  uint32_t cells[2] = {kCloudRecoveryMagic, value};
  ESP.rtcUserMemoryWrite(0, cells, sizeof(cells));
#endif
}

void clearCloudMqttRecoveryRestarts()
{
  if (readCloudRecoveryRestarts() != 0)
    writeCloudRecoveryRestarts(0);
}

void requestCloudMqttRecoveryRestart()
{
  if (cloudMqttRecoveryQueued)
    return;
  cloudMqttRecoveryQueued = true;
  cloudMqttTransitionDeadlineArmed = false;
  cloudMqttSubscriptionsPending = false;
  cloudMqttReconnectPending = false;
  storeCloudMqttConnectedState(false);
#ifdef DEBUG_ONOFRE
  Log.error("%s MQTT transport transition timed out; controlled restart requested" CR,
            tags::cloudIO);
#endif
  // AsyncMqttClient 0.9.0 exposes no attempt identifier and cannot cancel a
  // DNS/TCP attempt before AsyncTCP owns a PCB. An in-place retry can therefore
  // race a late callback. Reboot is the only app-level recovery that destroys
  // every outstanding callback and retained pointer safely.
  //
  // But a reboot only recovers a transport that is stuck on its own. When the
  // cloud itself is unwell, every boot ends here again thirty seconds later and
  // the device never finishes starting -- the worst possible answer, because
  // everything local works perfectly well without the cloud. After a few tries
  // in a row, stop restarting and run without it: cloudMqttRecoveryQueued keeps
  // the transport untouched for the rest of this boot, and late callbacks from
  // the abandoned attempt are ignored exactly as they are before a restart.
  const uint32_t restarts = readCloudRecoveryRestarts() + 1;
  if (restarts > kCloudMqttMaxRecoveryRestarts)
  {
    writeCloudRecoveryRestarts(0);
    deviceLog("cloud mqtt: %d reinicios sem sucesso, a funcionar so localmente",
              (int)kCloudMqttMaxRecoveryRestarts);
    return;
  }
  writeCloudRecoveryRestarts(restarts);
  deviceLog("cloud mqtt: reinicio controlado %d/%d", (int)restarts,
            (int)kCloudMqttMaxRecoveryRestarts);
  config.requestRestart();
}

void signalCloudIOWatchdog()
{
#ifdef ESP32
  cloudIOWatchdogDue.store(true, std::memory_order_release);
#else
  cloudIOWatchdogDue = true;
#endif
}

bool takeCloudIOWatchdogSignal()
{
#ifdef ESP32
  return cloudIOWatchdogDue.exchange(false, std::memory_order_acq_rel);
#else
  const bool due = cloudIOWatchdogDue;
  cloudIOWatchdogDue = false;
  return due;
#endif
}

void incrementRejectedCloudCommandCount()
{
#ifdef ESP32
  rejectedCloudCommandCount.fetch_add(1, std::memory_order_relaxed);
#else
  rejectedCloudCommandCount++;
#endif
}

void incrementOverflowedCloudCommandCount()
{
#ifdef ESP32
  overflowedCloudCommandCount.fetch_add(1, std::memory_order_relaxed);
#else
  overflowedCloudCommandCount++;
#endif
}

uint32_t takeRejectedCloudCommandCount()
{
#ifdef ESP32
  return rejectedCloudCommandCount.exchange(0, std::memory_order_acq_rel);
#else
  const uint32_t count = rejectedCloudCommandCount;
  rejectedCloudCommandCount = 0;
  return count;
#endif
}

uint32_t takeOverflowedCloudCommandCount()
{
#ifdef ESP32
  return overflowedCloudCommandCount.exchange(0, std::memory_order_acq_rel);
#else
  const uint32_t count = overflowedCloudCommandCount;
  overflowedCloudCommandCount = 0;
  return count;
#endif
}

// Caller owns ConfigOnofre's feature-access lease. This function only copies
// data into connector-owned staging storage; connection lifecycle work is
// deliberately deferred until serviceCloudIOMqtt() runs without the lease.
void stageCloudMqttRuntimeLocked()
{
  const uint8_t target = activeCloudMqttRuntimeValid ? (activeCloudMqttRuntime ^ 1u) : 0u;
  CloudMqttRuntimeConfig &runtime = cloudMqttRuntimeConfigs[target];
  strlcpy(runtime.clientId, config.chipId, sizeof(runtime.clientId));
  strlcpy(runtime.username, config.cloudIOUsername, sizeof(runtime.username));
  strlcpy(runtime.password, config.cloudIOPassword, sizeof(runtime.password));
  strlcpy(runtime.healthTopic, config.cloudIOhealthTopic, sizeof(runtime.healthTopic));
  stagedCloudMqttRuntime = target;
  stagedCloudMqttRuntimePending = true;
}

// AsyncMqttClient delivers commands from its networking callback while the
// feature graph can be owned by another top-level operation. MQTT commands use
// QoS 0, so returning when the graph is busy loses them permanently. Keep a
// small fixed SPSC queue and apply it later from the main loop instead. Four
// entries cover short bursts without spending an unbounded amount of ESP8266
// RAM (each entry is 192 bytes).
const uint32_t kCloudCommandQueueCapacity = 4;
const size_t kCloudCommandTopicSize = 128;
const size_t kCloudCommandPayloadSize = 64;

struct PendingCloudCommand
{
  char topic[kCloudCommandTopicSize];
  char payload[kCloudCommandPayloadSize];
};

enum class QueueCloudCommandResult : uint8_t
{
  QUEUED = 0,
  REJECTED,
  FULL
};

PendingCloudCommand pendingCloudCommands[kCloudCommandQueueCapacity];
#ifdef ESP32
std::atomic<uint32_t> pendingCloudCommandWrite{0};
std::atomic<uint32_t> pendingCloudCommandRead{0};
#else
// ESP8266 networking and loop work share the cooperative execution context.
// Plain counters avoid pulling atomic support into that target.
uint32_t pendingCloudCommandWrite = 0;
uint32_t pendingCloudCommandRead = 0;
#endif

uint32_t loadPendingCloudCommandWrite()
{
#ifdef ESP32
  return pendingCloudCommandWrite.load(std::memory_order_acquire);
#else
  return pendingCloudCommandWrite;
#endif
}

uint32_t loadPendingCloudCommandRead()
{
#ifdef ESP32
  return pendingCloudCommandRead.load(std::memory_order_acquire);
#else
  return pendingCloudCommandRead;
#endif
}

void publishPendingCloudCommand(uint32_t writeIndex)
{
#ifdef ESP32
  pendingCloudCommandWrite.store(writeIndex, std::memory_order_release);
#else
  pendingCloudCommandWrite = writeIndex;
#endif
}

void releasePendingCloudCommand(uint32_t readIndex)
{
#ifdef ESP32
  pendingCloudCommandRead.store(readIndex, std::memory_order_release);
#else
  pendingCloudCommandRead = readIndex;
#endif
}

QueueCloudCommandResult queueCloudIOCommand(const char *topic, const char *payload, size_t payloadLength)
{
  if (topic == nullptr || payload == nullptr || payloadLength >= kCloudCommandPayloadSize)
  {
    return QueueCloudCommandResult::REJECTED;
  }
  // The consumer deliberately treats payloads as C strings. Accepting an
  // embedded NUL would make the validated MQTT length disagree with the value
  // later interpreted by strcmp()/controlFeature().
  if (payloadLength > 0 && memchr(payload, '\0', payloadLength) != nullptr)
  {
    return QueueCloudCommandResult::REJECTED;
  }

  size_t topicLength = 0;
  while (topicLength < kCloudCommandTopicSize && topic[topicLength] != '\0')
  {
    topicLength++;
  }
  if (topicLength >= kCloudCommandTopicSize)
  {
    return QueueCloudCommandResult::REJECTED;
  }

#ifdef ESP32
  const uint32_t writeIndex = pendingCloudCommandWrite.load(std::memory_order_relaxed);
#else
  const uint32_t writeIndex = pendingCloudCommandWrite;
#endif
  const uint32_t readIndex = loadPendingCloudCommandRead();
  if (writeIndex - readIndex >= kCloudCommandQueueCapacity)
  {
    return QueueCloudCommandResult::FULL;
  }

  PendingCloudCommand &command = pendingCloudCommands[writeIndex % kCloudCommandQueueCapacity];
  memcpy(command.topic, topic, topicLength);
  command.topic[topicLength] = '\0';
  memcpy(command.payload, payload, payloadLength);
  command.payload[payloadLength] = '\0';
  publishPendingCloudCommand(writeIndex + 1);
  return QueueCloudCommandResult::QUEUED;
}
}

void notifyIrrigationToCloudIO()
{
  if (!cloudIOConnected() || config.cloudIOIrrigationStatusTopic[0] == '\0')
    return;
  JsonDocument doc;
  JsonVariant root = doc.to<JsonObject>();
  irrigation.statusJson(root);
  String payload;
  serializeJson(doc, payload);
  mqttClient.publish(config.cloudIOIrrigationStatusTopic, 0, true, payload.c_str());
}

void notifyIrrigation()
{
  notifyIrrigationToCloudIO();
  publishIrrigationHomeAssistantState();
}

void notifyStateToCloudIO(const char *topic, const char *state)
{
  if (!loadCloudMqttConnectedState())
    return;
  mqttClient.publish(topic, 0, true, state);
}
void subscribeOnMqttCloudIO(const char *topic)
{
  if (!loadCloudMqttConnectedState())
  {
#ifdef DEBUG_ONOFRE
    Log.warning("%s Required Mqtt connection" CR, tags::cloudIO);
#endif
    return;
  }
  mqttClient.subscribe(topic, 0);
}
void onMqttConnect(bool sessionPresent)
{
  (void)sessionPresent;
  // Network callbacks never acquire the feature lease or call back into the
  // client. Main-loop service performs subscriptions and state publication.
  signalCloudMqttTransportEvent(CloudMqttTransportEvent::CONNECTED, true);
}

// Reason for the last disconnection (AsyncMqttClientDisconnectReason): 0 TCP connection closed,
// 1 protocol version, 2 identifier rejected, 3 server unavailable, 4 malformed credentials,
// 5 not authorized.
static volatile uint8_t cloudMqttLastDisconnectReason = 0;

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  cloudMqttLastDisconnectReason = static_cast<uint8_t>(reason);
  signalCloudMqttTransportEvent(CloudMqttTransportEvent::DISCONNECTED, false);
}
bool cloudIOConnected()
{
  return loadCloudMqttConnectedState();
}

void disconnectCloudIOForUpdate()
{
  // performUpdate() blocks the main service loop. Make the transport unavailable
  // before closing it so no feature publisher can enqueue another packet while
  // ESP8266 is trying to assemble one large free block for BearSSL.
  cloudMqttSubscriptionsPending = false;
  cloudMqttReconnectPending = false;
  storeCloudMqttConnectedState(false);
  mqttClient.disconnect(true);
  // A clean session should leave no QoS state behind. Explicitly release any
  // queued packet now rather than carry its heap across the OTA handshake.
  mqttClient.clearQueue();
  enterCloudMqttTransportState(CloudMqttTransportState::DISCONNECTED);
  // On failure, serviceCloudIOMqtt() resumes on the next main-loop pass and
  // reconnects from the immutable active runtime. Success reboots instead.
  cloudMqttReconnectPending = activeCloudMqttRuntimeValid;
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  (void)properties;
  // Only a complete bounded message is safe to enqueue. Acting on a fragment
  // could turn a partial numeric payload into a valid but wrong command.
  if (index != 0 || len != total || len >= kCloudCommandPayloadSize)
  {
    incrementRejectedCloudCommandCount();
    return;
  }

  const QueueCloudCommandResult result = queueCloudIOCommand(topic, payload, len);
  if (result == QueueCloudCommandResult::FULL)
  {
    incrementOverflowedCloudCommandCount();
    return;
  }
  if (result == QueueCloudCommandResult::REJECTED)
    incrementRejectedCloudCommandCount();
}

void configureCloudMqttCallbacksOnce()
{
  if (cloudMqttCallbacksConfigured)
    return;
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setCleanSession(true);
  mqttClient.setKeepAlive(36);
  mqttClient.setServer(constanstsCloudIO::mqttDns, constanstsCloudIO::mqttPort);
  cloudMqttCallbacksConfigured = true;
}

void promoteStagedCloudMqttRuntime()
{
  activeCloudMqttRuntime = stagedCloudMqttRuntime;
  activeCloudMqttRuntimeValid = true;
  stagedCloudMqttRuntimePending = false;

  CloudMqttRuntimeConfig &runtime = cloudMqttRuntimeConfigs[activeCloudMqttRuntime];
  configureCloudMqttCallbacksOnce();
  mqttClient.setWill(runtime.healthTopic, 0, true, "0");
  mqttClient.setClientId(runtime.clientId);
  mqttClient.setCredentials(runtime.username, runtime.password);
  cloudMqttReconnectPending = runtime.username[0] != '\0' && runtime.password[0] != '\0';
}

void serviceCloudMqttSubscriptions()
{
  if (!cloudMqttSubscriptionsPending)
    return;
  if (!loadCloudMqttConnectedState())
  {
    cloudMqttSubscriptionsPending = false;
    return;
  }
  if (!config.tryBeginFeatureAccess("cloudio"))
    return;

  // publish() returns 0 when the message did not leave — connection down, out of memory —
  // and 1 at QoS 0 when it did.
  if (mqttClient.publish(config.cloudIOhealthTopic, 0, true, "1") != 0)
    deviceLog("cloud presenca enviada");
  else
    deviceLog("cloud presenca NAO enviada (publish falhou)");
  subscribeOnMqttCloudIO(config.cloudIOwriteTopic);
  if (config.cloudIOIrrigationWriteTopic[0] != '\0')
    subscribeOnMqttCloudIO(config.cloudIOIrrigationWriteTopic);
  for (auto &sw : config.actuatores)
  {
    if (sw.isVirtual())
      continue;
    subscribeOnMqttCloudIO(sw.cloudIOwriteTopic);
    notifyStateToCloudIO(sw.cloudIOreadTopic, String(sw.state).c_str());
  }
  if (ledStripCloudWriteTopic()[0] != '\0')
    subscribeOnMqttCloudIO(ledStripCloudWriteTopic());
  config.endFeatureAccess();
  // After the valve states, so an app that reconnects reads the cycle against
  // states it already has rather than against the ones it is about to receive.
  notifyIrrigation();
  cloudMqttSubscriptionsPending = false;
}

void serviceCloudIOMqtt()
{
  // Once recovery is queued, never touch the transport again. The restart
  // request is consumed later in this same main-loop routine.
  if (cloudMqttRecoveryQueued)
    return;

  const CloudMqttTransportEvent event = takeCloudMqttTransportEvent();
  if (event == CloudMqttTransportEvent::CONNECTED)
  {
    enterCloudMqttTransportState(CloudMqttTransportState::CONNECTED);
    cloudMqttReconnectPending = false;
    // A newer runtime may have been staged while this attempt was connecting.
    // Do not expose the stale session to feature publishers; the established
    // transport will be force-disconnected once the feature lease is free.
    if (stagedCloudMqttRuntimePending)
    {
      storeCloudMqttConnectedState(false);
      cloudMqttSubscriptionsPending = false;
    }
    else
    {
      storeCloudMqttConnectedState(true);
      cloudMqttSubscriptionsPending = true;
    }
#ifdef DEBUG_ONOFRE
    Log.warning("%s Connected to MQTT." CR, tags::cloudIO);
    Log.info("----------------------------------------------" CR);
#endif
    deviceLog("cloud mqtt ligado");
    // The transport came up, so whatever the earlier restarts were chasing is
    // gone. Clear the streak or the next unrelated hiccup starts from three.
    clearCloudMqttRecoveryRestarts();
  }
  else if (event == CloudMqttTransportEvent::DISCONNECTED)
  {
    enterCloudMqttTransportState(CloudMqttTransportState::DISCONNECTED);
    cloudMqttSubscriptionsPending = false;
    cloudMqttReconnectPending = activeCloudMqttRuntimeValid && !stagedCloudMqttRuntimePending;
#ifdef DEBUG_ONOFRE
    Log.warning("%s Disconnected from MQTT." CR, tags::cloudIO);
#endif
    deviceLog("cloud mqtt desligado (motivo %d)", (int)cloudMqttLastDisconnectReason);
  }

  if (cloudMqttTransitionTimedOut())
  {
    requestCloudMqttRecoveryRestart();
    return;
  }

  if (stagedCloudMqttRuntimePending)
  {
    // AsyncMqttClient cannot cancel DNS safely before its AsyncTCP client owns
    // a PCB. Wait for CONNECTED/DISCONNECTED (or the controlled timeout)
    // instead of forcing a CONNECTING attempt into an unobservable state.
    if (cloudMqttTransportState == CloudMqttTransportState::CONNECTING)
      return;
    if (cloudMqttTransportState == CloudMqttTransportState::CONNECTED)
    {
      // Force-close an in-flight or established session, but do not reuse its
      // retained pointer storage until onMqttDisconnect confirms completion.
      // Briefly gate feature publishers, mark the link unavailable, then drop
      // the gate before calling into the network client.
      if (!config.tryBeginFeatureAccess("cloudio"))
        return;
      enterCloudMqttTransportState(CloudMqttTransportState::DISCONNECTING);
      cloudMqttSubscriptionsPending = false;
      storeCloudMqttConnectedState(false);
      config.endFeatureAccess();
      mqttClient.disconnect(true);
      return;
    }
    if (cloudMqttTransportState == CloudMqttTransportState::DISCONNECTING)
      return;

    promoteStagedCloudMqttRuntime();
  }

  serviceCloudMqttSubscriptions();

  if (!cloudMqttReconnectPending ||
      cloudMqttTransportState != CloudMqttTransportState::DISCONNECTED ||
      !activeCloudMqttRuntimeValid || !wifiConnected())
    return;

  cloudMqttReconnectPending = false;
  enterCloudMqttTransportState(CloudMqttTransportState::CONNECTING);
  storeCloudMqttConnectedState(false);
#ifdef DEBUG_ONOFRE
  Log.error("%s Connecting to MQTT..." CR, tags::cloudIO);
#endif
  mqttClient.connect();
}

void drainCloudIOCommands()
{
  // Callback-context diagnostics are coalesced and emitted here so logging can
  // never block the AsyncMqttClient networking task.
  const uint32_t rejectedCount = takeRejectedCloudCommandCount();
  const uint32_t overflowedCount = takeOverflowedCloudCommandCount();
#ifdef DEBUG_ONOFRE
  if (rejectedCount > 0)
    Log.warning("%s Rejected %u incomplete, oversized, or invalid MQTT command(s)" CR,
                tags::cloudIO, static_cast<unsigned int>(rejectedCount));
  if (overflowedCount > 0)
    Log.warning("%s Dropped %u MQTT command(s): pending queue capacity is %u" CR,
                tags::cloudIO,
                static_cast<unsigned int>(overflowedCount),
                static_cast<unsigned int>(kCloudCommandQueueCapacity));
#else
  (void)rejectedCount;
  (void)overflowedCount;
#endif

  uint32_t readIndex = loadPendingCloudCommandRead();
  if (readIndex == loadPendingCloudCommandWrite())
  {
    return;
  }

  // A busy graph is not an error: leave the command queued for the next pass.
  // Process at most one command so Cloud MQTT can never monopolize the feature
  // lease during a burst.
  if (!config.tryBeginFeatureAccess("cloudio"))
  {
    return;
  }

  PendingCloudCommand &command = pendingCloudCommands[readIndex % kCloudCommandQueueCapacity];
#ifdef DEBUG_ONOFRE
  Log.info("----------------------------------------------" CR);
  Log.info("%s Message from MQTT. %s %s" CR, tags::cloudIO, command.topic, command.payload);
#endif
  if (strcmp(command.topic, config.cloudIOwriteTopic) == 0)
  {
    if (strcmp(command.payload, "REBOOT") == 0)
    {
      config.requestRestart();
    }
    else if (strcmp(command.payload, "UPDATE") == 0)
    {
      config.requestAutoUpdate();
    }
  }
  else if (strcmp(command.topic, config.cloudIOIrrigationWriteTopic) == 0)
  {
    // RUN:<programId> forces a cycle, STOP ends it. Deliberately not the schedule:
    // editing programs over a retained-message channel would make a lost message
    // look like a deleted program.
    irrigation.command(command.payload);
    notifyIrrigation();
  }
  else if (ledStripHandleMqtt(command.topic, command.payload))
  {
    // consumed by the LED strip
  }
  else
  {
    const ConfigUpdateResult result =
        config.controlFeature(StateOrigin::CLOUDIO, command.topic, command.payload);
    if (result == ConfigUpdateResult::PERSISTENCE_FAILED)
      config.reportControlPersistenceFailure(StateOrigin::CLOUDIO, command.topic);
  }

  releasePendingCloudCommand(readIndex + 1);
  config.endFeatureAccess();
}

// Watchdog backoff after an HTTP 204 ("not adopted") config response. A 204 can
// be transient (server deploy, database pressure, device row being migrated), so
// the watchdog must NEVER stop permanently — it just waits this many ticks
// (1 tick = 60 s) before asking again. 0 = no backoff.
static uint8_t cloudSyncBackoffTicks = 0;
// Consecutive failed config syncs. A failure used to carry no backoff at all:
// the watchdog asked again every 60 s for as long as the cloud was unwell, and
// each attempt blocks the main loop. Doubling the wait turns a permanent outage
// from a device-wide stall into an occasional one.
static uint8_t cloudSyncFailureStreak = 0;
// Minutes, since one watchdog tick is 60 s. Thirty matches the "not adopted"
// backoff, which is already the longest this device ever waits to try again.
const uint8_t kCloudSyncMaxBackoffTicks = 30;

static uint8_t nextCloudSyncBackoffTicks()
{
  if (cloudSyncFailureStreak < 255)
    cloudSyncFailureStreak++;
  // 1, 2, 4, 8, 16, then capped. The first failure still retries after a
  // minute, so a brief blip costs nothing.
  uint16_t ticks = 1;
  for (uint8_t i = 1; i < cloudSyncFailureStreak && ticks < kCloudSyncMaxBackoffTicks; i++)
    ticks <<= 1;
  return ticks > kCloudSyncMaxBackoffTicks ? kCloudSyncMaxBackoffTicks : (uint8_t)ticks;
}

void watchdogTimer()
{
  // Ticker callbacks may run outside the normal loop context. Do not log, touch
  // Wi-Fi/MQTT clients, or mutate ConfigOnofre here.
  signalCloudIOWatchdog();
}

void serviceCloudIOWatchdog()
{
  if (!takeCloudIOWatchdogSignal())
    return;
#ifdef DEBUG_ONOFRE
  Log.info("%s CloudIO Watchdog running." CR, tags::cloudIO);
#endif
  if (!wifiConnected())
  {
#ifdef DEBUG_ONOFRE
    Log.warning("%s No Internet Connection." CR, tags::cloudIO);
#endif
    return;
  }
  if (cloudIOConnected())
  {
    // Presence (a retained "1") used to be sent once, when the connection opened. If the
    // server loses it — broker restart, retained messages cleared — the cloud panel shows
    // the device offline until it reconnects, even with the connection up. Renew it every
    // five minutes (1 tick = 60 s): it is one small message.
    static uint8_t presenceTicks = 0;
    if (++presenceTicks >= 5 && config.cloudIOhealthTopic[0] != '\0')
    {
      presenceTicks = 0;
      if (mqttClient.publish(config.cloudIOhealthTopic, 0, true, "1") == 0)
        deviceLog("cloud presenca NAO renovada (publish falhou)");
    }
#ifdef DEBUG_ONOFRE
    Log.info("%s CloudIO OK." CR, tags::cloudIO);
#endif
    return;
  }
  if (cloudSyncBackoffTicks > 0)
  {
    cloudSyncBackoffTicks--;
#ifdef DEBUG_ONOFRE
    Log.info("%s CloudIO sync backoff: %d min left." CR, tags::cloudIO, cloudSyncBackoffTicks);
#endif
    return;
  }
#ifdef DEBUG_ONOFRE
  Log.info("%s CloudIO Reconnect." CR, tags::cloudIO);
#endif
  config.requestCloudIOSync();
}
void ConfigOnofre::startCloudIOWatchdog()
{
#ifdef DEBUG_ONOFRE
  Log.info("%s CloudIO Watchdog Started" CR, tags::cloudIO);
#endif
  checkCloudIOWatchdog.attach_ms(60000, watchdogTimer);
}
void ConfigOnofre::stopCloudIOWatchdog()
{
#ifdef DEBUG_ONOFRE
  Log.info("%s CloudIO Watchdog Stopped" CR, tags::cloudIO);
#endif
  checkCloudIOWatchdog.detach();
  // Ignore a tick already delivered just before detach. A future start gets a
  // fresh 60-second interval rather than stale work from access-point mode.
  takeCloudIOWatchdogSignal();
}
#ifdef ESP32
static TaskHandle_t cloudSyncTaskHandle = nullptr;

static void cloudSyncTask(void *)
{
  for (;;)
  {
    // Sleeps until someone asks for a sync, so an idle device pays nothing.
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    connectToCloudIO();
  }
}

/**
 * Hand the config sync to its own task.
 *
 * The POST is synchronous and can take tens of seconds against a cloud that
 * accepts the connection and then does not answer -- which is what an unwell
 * server looks like, as opposed to one that is simply down and refuses at once.
 * Run from loop(), that stalls the web panel, the local MQTT and the actuators
 * with it, so physical switches stop responding because a server far away is
 * unhealthy. Nothing local depends on the cloud, so nothing local should wait
 * for it.
 *
 * Safe to run here because connectToCloudIO already takes the feature lease
 * around every read and write of the feature graph and releases it before any
 * TLS or HTTP work, and because the result reaches the main loop the way it
 * always did: through the staged-runtime flag that serviceCloudIOMqtt consumes.
 *
 * Returns false when the task could not be created, so the caller can fall
 * back to the old inline behaviour rather than never syncing at all.
 */
bool dispatchCloudIOSync()
{
  if (cloudSyncTaskHandle == nullptr)
  {
    // 10 KB: mbedTLS handshakes are the deepest thing that runs here.
    if (xTaskCreate(cloudSyncTask, "CloudIO-Sync", 10240, nullptr, 1, &cloudSyncTaskHandle) != pdPASS)
    {
      cloudSyncTaskHandle = nullptr;
      return false;
    }
  }
  // Already syncing: the notification is remembered, so one more pass follows
  // the current one instead of being lost or queueing without bound.
  xTaskNotifyGive(cloudSyncTaskHandle);
  return true;
}
#endif

void connectToCloudIO()
{

  if (!wifiConnected())
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s WIFI DISCONNECTED" CR, tags::cloudIO);
#endif
    return;
  }
  String payload = "";
  {
    JsonDocument requestDoc;
    JsonVariant root = requestDoc.to<JsonVariant>();
    // Snapshot the vectors under a short lease, then release before any TLS,
    // HTTP, delay, or response parsing work.
    if (!config.tryBeginFeatureAccess("cloudio"))
    {
#ifdef DEBUG_ONOFRE
      Log.warning("%s Feature configuration busy; CloudIO snapshot deferred" CR, tags::cloudIO);
#endif
      config.requestCloudIOSync();
      return;
    }
    config.json(root, false);
    config.endFeatureAccess();
    // CloudIO backend expects numeric firmware format (e.g. 9.17).
    // Keep local/UI version tags (e.g. 9.17-dev), but strip suffix for this API call.
    String firmwareForCloud = String(VERSION);
    int versionSuffixIndex = firmwareForCloud.indexOf('-');
    if (versionSuffixIndex > 0)
    {
      firmwareForCloud = firmwareForCloud.substring(0, versionSuffixIndex);
    }
    root["firmware"] = firmwareForCloud;
    serializeJson(requestDoc, payload);
  }
  String responsePayload = "";
  int httpCode = -1;
  const String requestUrl = String(constanstsCloudIO::configUrl);
  String fallbackUrl = requestUrl;
  bool usedHttpFallback = false;

  if (fallbackUrl.startsWith("https://"))
  {
    fallbackUrl.replace("https://", "http://");
  }

  auto postCloudConfig = [&](const String &url, const bool useHttps, const uint8_t attempt, const uint8_t totalAttempts) -> int
  {
    HTTPClient request;
    request.setReuse(false);
    request.setTimeout(kCloudIoRequestTimeoutMs);
#ifdef ESP32
    request.setConnectTimeout(kCloudIoConnectTimeoutMs);
#endif
    bool beginOk = false;
    int responseCode = -1;

    if (useHttps)
    {
#ifdef ESP8266
      if (ESP.getFreeHeap() >= constanstsCloudIO::cloudTlsMinimumFreeHeap &&
          ESP.getMaxFreeBlockSize() >= constanstsCloudIO::cloudTlsMinimumMaxBlock)
      {
        BearSSL::WiFiClientSecure client;
        client.setInsecure();
        client.setBufferSizes(constanstsCloudIO::cloudTlsReceiveBufferSize,
                              constanstsCloudIO::tlsTransmitBufferSize);
        beginOk = request.begin(client, url);
        if (beginOk)
        {
          request.addHeader("Content-Type", "application/json");
          responseCode = request.POST(payload.c_str());
          if (responseCode == HTTP_CODE_OK)
          {
            responsePayload = request.getString();
          }
        }
      }
      else
      {
        responseCode = -1;
#ifdef DEBUG_ONOFRE
        Log.warning("%s [HTTP] HTTPS skipped: insufficient TLS memory heap=%u maxBlock=%u fragmentation=%u%%" CR,
                    tags::cloudIO,
                    ESP.getFreeHeap(),
                    ESP.getMaxFreeBlockSize(),
                    ESP.getHeapFragmentation());
#endif
      }
#else
      WiFiClientSecure client;
      client.setInsecure();
      beginOk = request.begin(client, url);
      if (beginOk)
      {
        request.addHeader("Content-Type", "application/json");
        responseCode = request.POST(payload.c_str());
        if (responseCode == HTTP_CODE_OK)
        {
          responsePayload = request.getString();
        }
      }
#endif
    }
    else
    {
      WiFiClient client;
      beginOk = request.begin(client, url);
      if (beginOk)
      {
        request.addHeader("Content-Type", "application/json");
        responseCode = request.POST(payload.c_str());
        if (responseCode == HTTP_CODE_OK)
        {
          responsePayload = request.getString();
        }
      }
    }
    request.end();

    if (!beginOk)
    {
      responseCode = -1;
    }

#ifdef DEBUG_ONOFRE
    if (responseCode < 0)
    {
      const String errorLabel = HTTPClient::errorToString(responseCode);
      Log.warning("%s [HTTP] %s attempt %u/%u failed: code=%d error=%s rssi=%d" CR,
                  tags::cloudIO,
                  useHttps ? "HTTPS" : "HTTP",
                  attempt,
                  totalAttempts,
                  responseCode,
                  errorLabel.c_str(),
                  WiFi.RSSI());
    }
    else
    {
      Log.info("%s [HTTP] %s attempt %u/%u result: %d" CR,
               tags::cloudIO,
               useHttps ? "HTTPS" : "HTTP",
               attempt,
               totalAttempts,
               responseCode);
    }
#endif

    return responseCode;
  };

  const bool supportsHttps = requestUrl.startsWith("https://");
  if (supportsHttps)
  {
    for (uint8_t attempt = 1; attempt <= kCloudIoHttpsRetryCount; attempt++)
    {
      httpCode = postCloudConfig(requestUrl, true, attempt, kCloudIoHttpsRetryCount);
      if (httpCode >= 0)
      {
        break;
      }
      if (attempt < kCloudIoHttpsRetryCount)
      {
        delay(kCloudIoRetryBackoffMs);
      }
    }

    // Fallback to plain HTTP only when HTTPS repeatedly fails to establish.
    if (httpCode < 0)
    {
      usedHttpFallback = true;
      httpCode = postCloudConfig(fallbackUrl, false, 1, 1);
    }
  }
  else
  {
    httpCode = postCloudConfig(requestUrl, false, 1, 1);
  }

#ifdef DEBUG_ONOFRE
  Log.info("%s [HTTP] Request result: %d (fallback=%d)" CR, tags::cloudIO, httpCode, usedHttpFallback ? 1 : 0);
  Log.info("----------------------------------------------" CR);
#endif
  if (httpCode == HTTP_CODE_NO_CONTENT)
    deviceLog("cloud: nao adotado (204), nova tentativa em 30 min");
  else if (httpCode == HTTP_CODE_OK)
    deviceLog("cloud: servidor respondeu 200%s", usedHttpFallback ? " (por HTTP)" : "");
  else
    deviceLog("cloud: pedido falhou (%d)%s", httpCode, usedHttpFallback ? " mesmo por HTTP" : "");
  if (httpCode == HTTP_CODE_NO_CONTENT)
  {
    if (!config.tryBeginFeatureAccess("cloudio"))
    {
      config.requestCloudIOSync();
      return;
    }
    config.cloudIOReady = false;
    config.endFeatureAccess();
#ifdef DEBUG_ONOFRE
    Log.info("%s [HTTP] Device not adopted" CR, tags::cloudIO);
#endif
    // Not adopted (or a transient server condition reported as 204). Never stop
    // the watchdog permanently — that stranded devices offline until a manual
    // power cycle. Back off for ~30 min and let the watchdog ask again.
    cloudSyncBackoffTicks = 30;
    return;
  }
  if (httpCode != HTTP_CODE_OK)
  {
    if (!config.tryBeginFeatureAccess("cloudio"))
    {
      config.requestCloudIOSync();
      return;
    }
    config.cloudIOReady = false;
    config.endFeatureAccess();
    cloudSyncBackoffTicks = nextCloudSyncBackoffTicks();
    deviceLog("cloud: nova tentativa em %d min", (int)cloudSyncBackoffTicks);
    return;
  }
  else if (httpCode == HTTP_CODE_OK)
  {
    cloudSyncBackoffTicks = 0;
    cloudSyncFailureStreak = 0;
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, responsePayload);
    if (error)
    {
      config.requestCloudIOSync();
      return;
    }
    // Applying credentials and all per-feature topics is one atomic view of
    // the returned configuration. Never wait in this networking callback.
    if (!config.tryBeginFeatureAccess("cloudio"))
    {
#ifdef DEBUG_ONOFRE
      Log.warning("%s Feature configuration busy; CloudIO apply deferred" CR, tags::cloudIO);
#endif
      config.requestCloudIOSync();
      return;
    }
    strlcpy(config.cloudIOUsername, doc["username"] | "", sizeof(config.cloudIOUsername));
    strlcpy(config.cloudIOPassword, doc["password"] | "", sizeof(config.cloudIOPassword));
    config.cloudIOReady = true;
    snprintf(config.cloudIOhealthTopic, sizeof(config.cloudIOhealthTopic), "%s/%s/available", config.cloudIOUsername, config.chipId);
    snprintf(config.cloudIOwriteTopic, sizeof(config.cloudIOwriteTopic), "%s/%s/config/set", config.cloudIOUsername, config.chipId);
    snprintf(config.cloudIOIrrigationStatusTopic, sizeof(config.cloudIOIrrigationStatusTopic),
             "%s/%s/irrigation/status", config.cloudIOUsername, config.chipId);
    snprintf(config.cloudIOIrrigationWriteTopic, sizeof(config.cloudIOIrrigationWriteTopic),
             "%s/%s/irrigation/set", config.cloudIOUsername, config.chipId);
    for (auto &sw : config.actuatores)
    {
      String family = sw.familyToText();
      family.toLowerCase();
      snprintf(sw.cloudIOwriteTopic, sizeof(sw.cloudIOwriteTopic), "%s/%s/%s/%s/set", config.cloudIOUsername, config.chipId, family.c_str(), sw.uniqueId);
      snprintf(sw.cloudIOreadTopic, sizeof(sw.cloudIOreadTopic), "%s/%s/%s/%s/status", config.cloudIOUsername, config.chipId, family.c_str(), sw.uniqueId);
    }
    for (auto &ss : config.sensors)
    {
      String family = ss.familyToText();
      family.toLowerCase();
      snprintf(ss.cloudIOreadTopic, sizeof(ss.cloudIOreadTopic), "%s/%s/%s/%s/metrics", config.cloudIOUsername, config.chipId, family.c_str(), ss.uniqueId);
    }
    // The strip is not in config.actuatores, so it needs its topics built here too.
    ledStripCloudTopics(config.cloudIOUsername, config.chipId);
    const bool credentialsReady = strlen(config.cloudIOUsername) > 0 && strlen(config.cloudIOPassword) > 0;
    stageCloudMqttRuntimeLocked();
    config.endFeatureAccess();
    if (credentialsReady)
    {
#ifdef DEBUG_ONOFRE
      Log.info("%s SETUP MQTT CLOUD" CR, tags::cloudIO);
#endif
    }
  }
}
