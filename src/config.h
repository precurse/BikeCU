#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "utils.h"
#include "globals.h"
#include <WiFiManager.h>

#define DEBUG true

#define STANDALONE_MODE false       // toggle standalone mode (no wifi connection)

#define WIFI_TIMEOUT_MS 20000      // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 30000 // Wait 30 seconds after a failed connection attempt
#define SECONDS_PER_HOUR 3600
#define SCREEN_TIMEOUT 300 // 5 minutes

// TODO: Add to wifimanager
#define ENABLE_DISPLAY true
#define ENABLE_INFLUX true
#define NTP_SERVER "ca.pool.ntp.org"
#define BLE_NAME "IC Bike"
#define INFLUX_URL "http://REPLACEME:8086"
#define INFLUX_DB "fitness"
#define INFLUX_PRECISION "s"

// Display pins
#define TFT_CS 7
#define TFT_RST 10
#define TFT_DC 6
#define TFT_SCK 2
#define TFT_MOSI 3
#define TFT_BACKLIGHT 5

#endif