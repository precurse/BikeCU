#ifndef WIFI_H
#define WIFI_H

#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"

void taskOta(void *parameter);
void taskKeepWifiAlive(void* parameter);

String getParam(String name);

void saveParamCallback();

#endif