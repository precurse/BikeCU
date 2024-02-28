#include "config.h"
#include "utils.h"

#include <FS.h>
#include <Adafruit_ST7735.h>
#include <ArduinoOTA.h>
#include "NTPClient.h"
#include "time.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <bluetooth.h>
#include <display.h>
#include <influxdb.h>
#include <wifi.h>

// Globals
QueueHandle_t dataQueue;
QueueHandle_t serialQ;
SemaphoreHandle_t bikeDataMutex = xSemaphoreCreateMutex();
LiveBikeData bikeData;
CycleSession cycleSession;

void initializeBikeData(LiveBikeData* bikeData)
{
  bikeData->header = 0;
  bikeData->speed = 0;
  bikeData->cadence = 0;
  bikeData->power = 0;
  bikeData->hr = 0;
  bikeData->hrBatt = 0;
  bikeData->speedAccum = 0;
  bikeData->cadenceAccum = 0;
  bikeData->powerAccumu = 0;
  bikeData->metricCnt = 0;
  bikeData->hrAccum = 0;
  bikeData->hrCnt = 0;
}

void initializeSession(LiveBikeData* bd, CycleSession* cs)
{
  cs->sessionState = NotStarted;
  cs->id = 0;
  cs->calories = 0;
  cs->distance = 0;
  cs->duration = 0;
  cs->paused_t = 0;
  cs->paused_ts = 0;
  initializeBikeData(bd);
}

// TODO: Fix
void SerialQueueSend(String msg)
{
  xQueueSend(serialQ, &msg, portMAX_DELAY);
}

void SerialQueueSend(int msg)
{
  String strMsg = String(msg);
  xQueueSend(serialQ, &strMsg, portMAX_DELAY);
}

void printBikeData(CycleSession* cs)
{
  Serial.print("Power: ");
  Serial.print(cs->data_last.power);
  Serial.print(" avg: ");
  Serial.println(getPowerAvg(cs));
  Serial.print("Speed: ");
  Serial.print(cs->data_last.speed);
  Serial.print(" avg: ");
  Serial.println(getSpeedAvg(cs));
  Serial.print("HR: ");
  Serial.println(cs->data_last.hr);
  Serial.print("Cadence: ");
  Serial.print(cs->data_last.cadence);
  Serial.print(" avg: ");
  Serial.println(getCadenceAvg(cs));
  Serial.print("Distance: ");
  Serial.println(cs->distance);
  Serial.print("Duration: ");
  Serial.println(cs->duration);
  Serial.print("Calories: ");
  Serial.println(cs->calories);
}

void taskSession(void *parameter)
{
  uint16_t speed;
  uint16_t power;
  uint16_t cadence;
  uint8_t hr;

  time_t last_metric_ts; // used to track last time a metric was passed to queue

  for (;;)
  {
    // Wait for NTP and wifi, since timestamp needed for session identifier
    if (getUnixTimestamp() < 1000)
    {
      Serial.println("[SESSION] Waiting for NTP to sync");
      vTaskDelay(500 / portTICK_PERIOD_MS);
      continue;
    }

    if (cycleSession.sessionState == NotStarted && bikeData.speed > 0)
    {
      startSession(&cycleSession);
    }
    else if (cycleSession.sessionState == Paused && bikeData.speed > 0)
    {
      resumeSession(&cycleSession);
    }
    else if (cycleSession.sessionState == Running && bikeData.speed == 0)
    {
      pauseSession(&cycleSession);
      Serial.println("[SESSION] Session paused");
      continue;
    }
    else if (cycleSession.sessionState == Paused && bikeData.speed == 0)
    {
      // End session if paused for 5 minutes
      if (getUnixTimestamp() - cycleSession.paused_ts > 300)
      {
        cycleSession.sessionState = Ended;

        // Wait for 1 second before restarting
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP.restart();
      }
      continue;
    }
    else if (cycleSession.sessionState == NotStarted)
    {
      continue;
    }

    // Get live bike metrics for session
    if (xSemaphoreTake(bikeDataMutex, portMAX_DELAY) == pdTRUE)
    {
      cycleSession.data_last = bikeData;
      xSemaphoreGive(bikeDataMutex);
    }
    else
    {
      const char *msg = "[Session] Failed to get mutex";
      xQueueSend(serialQ, msg, portMAX_DELAY);
      continue;
    }
    LiveBikeData *reading = &cycleSession.data_last;
    time_t ts = getUnixTimestamp();

    updateMetrics(&cycleSession, ts);

    printBikeData(&cycleSession);

    // Only write to queue 1x a second
    if (ts > last_metric_ts)
    {
      // If queue full, prune off earliest item
      if (!uxQueueSpacesAvailable(dataQueue) > 0) {
        Serial.println("[SESSION] dataQueue full. Removing oldest");
        CycleSession pruneSession;
        xQueueReceive(dataQueue, &pruneSession, 0);
      }

      xQueueSend(dataQueue, &cycleSession, portMAX_DELAY);
    }

    #ifdef DEBUG
    Serial.print("[SESSION] High water mark (words): ");
    Serial.println(uxTaskGetStackHighWaterMark(NULL));
    #endif

    // BLE data returns data every 1/3 of a second
    vTaskDelay(400 / portTICK_PERIOD_MS);
  }
}

void taskSerialPrint(void *parameter)
{
  String item;
  for (;;)
  {
    if (xQueueReceive(serialQ, &item, 0) == pdTRUE)
    {
      Serial.println(item);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void taskOta(void *parameter)
{
  for (;;)
  {
    ArduinoOTA.handle();
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  #ifdef DEBUG
  if (DEBUG) {
        delay(1000);
  }
  #endif

  // Initialize data types needed
  dataQueue = xQueueCreate(20, sizeof(struct CycleSession));
  serialQ = xQueueCreate(20, sizeof(String) * 100);
  // bikeDataMutex = xSemaphoreCreateMutex();
  initializeSession(&bikeData, &cycleSession);

  Serial.println("[SETUP] Creating tasks...");
  // 768 bytes is required for task overhead
  xTaskCreate(taskKeepWifiAlive, "WIFI_HANDLE", 2300, NULL, 1, NULL);
  xTaskCreate(taskBLE, "BLE_HANDLE", 4000, NULL, 1, NULL);
  xTaskCreate(taskSession, "SESSION_HANDLE", 1000, NULL, 1, NULL);
  #ifdef ENABLE_DISPLAY
  if (ENABLE_DISPLAY) {
        xTaskCreate(taskDisplay, "DISPLAY_HANDLE", 1000, NULL, 2, NULL);
  }
  #endif

  // xTaskCreate(taskSerialPrint, "SERIAL_HANDLE", 1000, NULL, 1, NULL);
  #ifdef ENABLE_INFLUX
  if (ENABLE_INFLUX) {
    xTaskCreate(taskInflux, "INFLUX_HANDLE", 2000, NULL, 1, NULL);
  }
  #endif
  xTaskCreate(taskOta, "OTA_HANDLE", 10000, NULL, 1, NULL);

  // Remove Arduino setup and loop tasks
  vTaskDelete(NULL);
}

void loop()
{
  ArduinoOTA.handle();
}
