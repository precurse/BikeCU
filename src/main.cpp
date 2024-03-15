#include "config.h"
#include "session.h"
#include "utils.h"
#include "bluetooth.h"
#include "display.h"
#include "influxdb.h"
#include "wifi.h"

#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <time.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Globals
QueueHandle_t dataQueue;
QueueHandle_t serialQ;
SemaphoreHandle_t bikeDataMutex;
LiveBikeData bikeData;
CycleSession cycleSession;

void setup()
{
  Serial.begin(115200);
  #ifdef DEBUG
    delay(1000);
  #endif

  // Initialize data types needed
  dataQueue = xQueueCreate(20, sizeof(struct CycleSession));
  serialQ = xQueueCreate(20, sizeof(String) * 100);
  bikeDataMutex = xSemaphoreCreateMutex();
  initializeSession(&bikeData, &cycleSession);

  Serial.println("[SETUP] Creating tasks...");
  // 768 bytes is required for task overhead
  #ifdef FEATURE_WIFI
    xTaskCreate(taskKeepWifiAlive, "WIFI_HANDLE", 2300, NULL, 1, NULL);
  #endif

  xTaskCreate(taskBLE, "BLE_HANDLE", 4000, NULL, 1, NULL);
  xTaskCreate(taskSession, "SESSION_HANDLE", 1000, NULL, 1, NULL);
  
  #ifdef FEATURE_DISPLAY
    xTaskCreate(taskDisplay, "DISPLAY_HANDLE", 1000, NULL, 2, NULL);
  #endif

  // xTaskCreate(taskSerialPrint, "SERIAL_HANDLE", 1000, NULL, 1, NULL);
  #ifdef FEATURE_INFLUX
    xTaskCreate(taskInflux, "INFLUX_HANDLE", 2000, NULL, 1, NULL);
  #endif

  #ifdef FEATURE_OTA
    xTaskCreate(taskOta, "OTA_HANDLE", 1000, NULL, 2, NULL);
  #endif

  // Remove Arduino setup and loop tasks
  vTaskDelete(NULL);
}

void loop()
{
}
