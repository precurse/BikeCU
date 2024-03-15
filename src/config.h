#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "utils.h"
#include "globals.h"
#include <WiFiManager.h>

#define WIFI_TIMEOUT_MS 20000       // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 30000  // Wait 30 seconds after a failed connection attempt
#define SCREEN_TIMEOUT 300          // Timeout after no BT connection

// TODO: Add to wifimanager
// #define DEBUG                 // toggle debug mode (additional serial output)
#define FEATURE_DISPLAY          // toggle display feature
#define FEATURE_INFLUX           // toggle InfluxDB collection
#define FEATURE_OTA              // toggle OTA support
#define FEATURE_WIFI             // toggle Wifi support

#define NTP_SERVER "ca.pool.ntp.org"
#define BLE_NAME "IC Bike"
#define INFLUX_URL "http://REPLACEME:8086"
#define INFLUX_DB "fitness"
#define INFLUX_PRECISION "s"
#define OTA_NAME "bikecu"
#define OTA_PASSWORD "CHANGEME"
#define WIFIMGR_SSID "BikeCU"

// Display pins
#define TFT_CS 7
#define TFT_RST 10
#define TFT_DC 6
#define TFT_SCK 2
#define TFT_MOSI 3
#define TFT_BACKLIGHT 5

#endif