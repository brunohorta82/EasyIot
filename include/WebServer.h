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
void initializeWebAuthCredentials();
bool startWebserver();
void stopWebserver();
void setupCaptivePortal();
void stopCaptivePortal();
void setupCors();
void webserverServicesLoop();
void sendToServerEvents(String topic, String payload);
AutoUpdateResult performUpdate();
bool storeOtaFailureForRestart();
void restoreOtaFailureStatus();

/* Progress of an over-the-air update, so the panel can show a bar and, when it
   fails, the reason. ESP8266 briefly journals a failure across its controlled
   recovery reboot because port 80 must remain closed while BearSSL owns heap. */
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
