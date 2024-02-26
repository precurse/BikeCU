#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "utils.h"
#include <WiFiManager.h>

#define DEBUG

#define WIFI_TIMEOUT_MS 20000      // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 30000 // Wait 30 seconds after a failed connection attempt
#define SECONDS_PER_HOUR 3600
#define SCREEN_TIMEOUT 300 // 5 minutes

// TODO: Add to wifimanager
#define NTP_SERVER "ca.pool.ntp.org"
#define BLE_NAME "IC Bike"
#define INFLUX_URL "http://REPLACEME:8086"
#define INFLUX_DB "fitness"
#define INFLUX_PRECISION "s"

#define ENABLE_DISPLAY false
// Display pins
#define TFT_CS 7
#define TFT_RST 10
#define TFT_DC 6
#define TFT_SCK 2
#define TFT_MOSI 3
#define TFT_BACKLIGHT 5

// Live data
struct BikeData
{
  uint16_t header;
  uint16_t speed;   // in (KM/h * 100) i.e. 3600 = 36KM/h
  uint16_t cadence; // in RPM (x2)
  uint16_t power;   // in W
  uint8_t hr;       // in bpm
  uint8_t hrBatt;   // in pct (battery level)
  // Accumuators for calculating averages
  uint32_t speedAccum;
  uint32_t cadenceAccum;
  uint32_t powerAccumu;
  uint32_t hrAccum;

  uint16_t metricCnt;
  uint16_t hrCnt;
};

struct CycleSession
{
  time_t id; // Session ID (Unix Timestamp)
  time_t ts; // Timestamp of metric calculations
  uint16_t calories;
  uint16_t distance; // Distance (meters)
  uint16_t duration; // Duration (seconds)
  uint16_t paused_t; // Total time paused (seconds)
  time_t paused_ts;  // Timestamp session paused (Unix Timestamp)
  BikeData data_last;
};

enum StateBle
{
  Initializing,
  Searching,
  Connecting,
  Connected,
  Disconnected
};

enum StateSession
{
  NotStarted,
  Running,
  Paused,
  Ended
};

extern time_t lastBtConnectTime;
const uint8_t indicationOn[] = {0x2, 0x0};

extern BikeData *bikeData;
extern SemaphoreHandle_t bikeDataMutex;

// Queue for influxdb metrics
extern QueueHandle_t dataQueue;
// Queue for handling serial output
extern QueueHandle_t serialQ;

extern CycleSession *cycleSession;
extern StateSession stateSession;
extern StateBle stateBleBike;
extern StateBle stateBleHr;

extern WiFiManager wifiManager;

#endif