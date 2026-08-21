#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

enum class AutoUpdateResult : uint8_t
{
  UPDATED,
  NO_UPDATE,
  FAILED
};
void setupWebPanel();
void startWebserver();
void stopWebserver();
void setupCaptivePortal();
void setupCors();
void webserverServicesLoop();
void sendToServerEvents(String topic, String payload);
AutoUpdateResult performUpdate();

/* Progress of an over-the-air update, so the panel can show a bar and, when it
   fails, the reason. Lives in RAM: a failed update does not reboot, so the panel
   can still come back and read why. */
enum class OtaState
{
  IDLE,
  RUNNING,
  FAILED,
  DONE
};
struct OtaStatus
{
  OtaState state{OtaState::IDLE};
  int percent{0};
  char error[64]{};
};
extern OtaStatus otaStatus;
void otaStatusJson(JsonVariant &root);
