#pragma once
#include <Arduino.h>
void setupWebPanel();
void startWebserver();
void stopWebserver();
void setupCaptivePortal();
void setupCors();
void webserverServicesLoop();
void sendToServerEvents(String topic, String payload);
void performUpdate();
