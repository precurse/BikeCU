#ifndef GLOBAL_H
#define GLOBAL_H

#include <time.h>
#include <WiFiManager.h>
#include "types.h"

extern WiFiManagerParameter custom_btle_name;

extern time_t lastBtConnectTime;

extern LiveBikeData bikeData;
extern SemaphoreHandle_t bikeDataMutex;

// Queue for influxdb metrics
extern QueueHandle_t dataQueue;
// Queue for handling serial output
extern QueueHandle_t serialQ;

extern CycleSession cycleSession;
extern StateBle stateBleBike;
extern StateBle stateBleHr;

extern WiFiManager wifiManager;

#endif