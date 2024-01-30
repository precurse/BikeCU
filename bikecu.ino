#include <FS.h>
#include <WiFiManager.h>

#include <Adafruit_ST7735.h>
#include <ArduinoOTA.h>
#include "NimBLEDevice.h"
#include "NimBLEAddress.h"
#include "NTPClient.h"
#include "time.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WIFI_TIMEOUT_MS 20000       // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 30000  // Wait 30 seconds after a failed connection attempt
#define SECONDS_PER_HOUR 3600
#define SCREEN_TIMEOUT 300  // 5 minutes

// TODO: Add to wifimanager
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

// https://gist.github.com/sam016/4abe921b5a9ee27f67b3686910293026
// fitness machine service
static BLEUUID fmServiceUUID((uint16_t)0x1826);
// indoor bike data characteristic
static BLEUUID indoorBikeDataCharUUID((uint16_t)0x2ad2);
static BLEUUID hrServiceUUID((uint16_t)0x180d);
static BLEUUID hrCharUUID((uint16_t)0x2a37);
static BLEUUID hrBattServiceUUID((uint16_t)0x180f);
static BLEUUID hrBattCharUUID((uint16_t)0x2a19);

static BLEAddress* pBikeAddress;
static BLEAddress* pHrAddress;
BLEClient* pBikeClient;
BLEClient* pHrClient;

// static BLERemoteCharacteristic* bikeCharacteristic;
static BLERemoteDescriptor* bikeDescriptor;
time_t lastBtConnectTime = 0;
const uint8_t indicationOn[] = { 0x2, 0x0 };

enum StateBle { Initializing,
                Searching,
                Connecting,
                Connected,
                Disconnected };

enum StateSession { NotStarted,
                    Running,
                    Paused,
                    Ended };

// Live data
struct BikeData {
  uint16_t header;
  uint16_t speed;    // in (KM/h * 100) i.e. 3600 = 36KM/h
  uint16_t cadence;  // in RPM (x2)
  uint16_t power;    // in W
  uint8_t hr;        // in bpm
  uint8_t hrBatt;    // in pct (battery level)
  // Accumuators for calculating averages
  uint32_t speedAccum;
  uint32_t cadenceAccum;
  uint32_t powerAccumu;
  uint32_t hrAccum;

  uint16_t metricCnt;
  uint16_t hrCnt;
};


struct CycleSession {
  time_t id;  // Session ID (Unix Timestamp)
  time_t ts;  // Timestamp of metric calculations
  uint16_t calories;
  uint16_t distance;  // Distance (meters)
  uint16_t duration;  // Duration (seconds)
  uint16_t paused_t;  // Total time paused (seconds)
  time_t paused_ts;   // Timestamp session paused (Unix Timestamp)
  BikeData data_last;
};

static BikeData* bikeData;
SemaphoreHandle_t bikeDataMutex;

// Queue for influxdb metrics
QueueHandle_t dataQueue;
// Queue for handling serial output
QueueHandle_t serialQ;

static CycleSession* cycleSession;
static StateSession stateSession;
static StateBle stateBleBike;
static StateBle stateBleHr;

WiFiManager wifiManager;

void initializeBikeData() {
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

void initializeSession() {
  stateSession = NotStarted;

  initializeBikeData();

  cycleSession->id = 0;
  cycleSession->calories = 0;
  cycleSession->distance = 0;
  cycleSession->duration = 0;
  cycleSession->paused_t = 0;
  cycleSession->paused_ts = 0;
}

// TODO: Fix
void SerialQueueSend(String msg) {
  xQueueSend(serialQ, &msg, portMAX_DELAY);
}

void SerialQueueSend(int msg) {
  String strMsg = String(msg);
  xQueueSend(serialQ, &strMsg, portMAX_DELAY);
}

time_t getUnixTimestamp() {
  time_t now;
  time(&now);
  return now;
}

void startSession() {
  stateSession = Running;
  cycleSession->id = getUnixTimestamp();
}

void pauseSession() {
  stateSession = Paused;
  cycleSession->paused_ts = getUnixTimestamp();
}

void resumeSession() {
  stateSession = Running;
  // Calculate paused duration seconds and add to paused time
  time_t ts = getUnixTimestamp();
  uint16_t elapsed = ts - cycleSession->paused_ts;
  cycleSession->paused_t += elapsed;
}

void updateDuration() {
  time_t curTime = getUnixTimestamp();
  cycleSession->duration = curTime - cycleSession->id - cycleSession->paused_t;
}

void updateDistance(uint16_t speedAvg) {
  // Remember: Speed avg in (km/h * 100)
  // Duration in seconds (divide by 3600 to get hours)
  uint32_t tmpDist = (speedAvg * cycleSession->duration) / SECONDS_PER_HOUR;
  cycleSession->distance = static_cast<uint16_t>(tmpDist);
}

void updateCalories(uint16_t powerAvg) {
  // Energy (kcal) = Power (watts) * Time (hours) * 3.6
  
  uint32_t temp_kcal = static_cast<uint32_t>(powerAvg) * 36 * cycleSession->duration;

  // Only need to track whole calories
  cycleSession->calories = static_cast<uint16_t>(temp_kcal / (SECONDS_PER_HOUR * 10));
}

uint16_t getMetricAvg(uint32_t metricAccum, uint16_t readCnt) {
  uint32_t tmpAvg = 0;
  if (readCnt > 0) {
    tmpAvg = metricAccum / readCnt;
  }
  return static_cast<uint16_t>(tmpAvg);
}

uint8_t getHrAvg() {
  return getMetricAvg(cycleSession->data_last.hrAccum, cycleSession->data_last.hrCnt);
}

uint16_t getPowerAvg() {
  return getMetricAvg(cycleSession->data_last.powerAccumu, cycleSession->data_last.metricCnt);
}

uint16_t getCadenceAvg() {
  return getMetricAvg(cycleSession->data_last.cadenceAccum, cycleSession->data_last.metricCnt);
}

uint16_t getSpeedAvg() {
  return getMetricAvg(cycleSession->data_last.speedAccum, cycleSession->data_last.metricCnt);
}

void updateMetrics(time_t ts) {
  cycleSession->ts = ts;
  uint16_t speedAvg = getSpeedAvg();
  uint16_t powerAvg = getPowerAvg();

  // Update calculated fields
  updateDuration();
  updateDistance(speedAvg);
  // Update calories last (relies on duration)
  updateCalories(powerAvg);
}

void taskSession(void* parameter) {
  uint16_t speed;
  uint16_t power;
  uint16_t cadence;
  uint8_t hr;

  time_t last_metric_ts;  // used to track last time a metric was passed to queue

  for (;;) {
    // Wait for NTP
    if (getUnixTimestamp() < 1000) {
      Serial.println("[SESSION] Waiting for NTP to sync");
      vTaskDelay(500 / portTICK_PERIOD_MS);
      continue;
    }

    if (stateSession == NotStarted && bikeData->speed > 0) {
      startSession();
    } else if (stateSession == Paused && bikeData->speed > 0) {
      resumeSession();
    } else if (stateSession == Running && bikeData->speed == 0) {
      pauseSession();
      Serial.println("[SESSION] Session paused");
      continue;
    } else if (stateSession == Paused && bikeData->speed == 0) {
      // End session if paused for 5 minutes
      if (getUnixTimestamp() - cycleSession->paused_ts > 300) {
        stateSession = Ended;

        // Wait for 1 second before restarting
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP.restart();
      }
      continue;
    } else if (stateSession == NotStarted) {
      continue;
    }

    // Get live bike metrics for session
    if (xSemaphoreTake(bikeDataMutex, portMAX_DELAY) == pdTRUE) {
      cycleSession->data_last = *bikeData;
      xSemaphoreGive(bikeDataMutex);
    } else {
      char* msg = "[Session] Failed to get mutex";
      xQueueSend(serialQ, msg, portMAX_DELAY);
      continue;
    }
    BikeData* reading = &cycleSession->data_last;
    time_t ts = getUnixTimestamp();

    //updateMetrics(reading->speed, reading->power, reading->cadence, reading->hr, ts);
    updateMetrics(ts);

    // printBikeData();

    // Only write to queue 1x a second
    if (ts > last_metric_ts) {
      xQueueSend(dataQueue, cycleSession, portMAX_DELAY);
    }

    // BLE data returns data every 1/3 of a second
    vTaskDelay(400 / portTICK_PERIOD_MS);
  }
}

void printBikeData() {
  Serial.print("Power: ");
  Serial.print(cycleSession->data_last.power);
  Serial.print(" avg: ");
  Serial.println(getPowerAvg());
  Serial.print("Speed: ");
  Serial.print(cycleSession->data_last.speed);
  Serial.print(" avg: ");
  Serial.println(getSpeedAvg());
  Serial.print("HR: ");
  Serial.println(cycleSession->data_last.hr);
  Serial.print("Cadence: ");
  Serial.print(cycleSession->data_last.cadence);
  Serial.print(" avg: ");
  Serial.println(getCadenceAvg());
  Serial.print("Distance: ");
  Serial.println(cycleSession->distance);
  Serial.print("Duration: ");
  Serial.println(cycleSession->duration);
  Serial.print("Calories: ");
  Serial.println(cycleSession->calories);
}

void taskSerialPrint(void* parameter) {
  String item;
  for (;;) {
    if (xQueueReceive(serialQ, &item, 0) == pdTRUE) {
      Serial.println(item);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void taskOta(void* parameter) {
  for (;;) {
    ArduinoOTA.handle();
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize data types needed
  dataQueue = xQueueCreate(20, sizeof(struct CycleSession));
  serialQ = xQueueCreate(20, sizeof(String) * 100);
  bikeData = new BikeData();
  cycleSession = new CycleSession();
  bikeDataMutex = xSemaphoreCreateMutex();
  initializeSession();

  Serial.println("[SETUP] Creating tasks...");
  // 768 bytes is required for task overhead
  xTaskCreate(taskKeepWifiAlive, "WIFI_HANDLE", 2300, NULL, 1, NULL);
  xTaskCreate(taskBLE, "BLE_HANDLE", 4000, NULL, 1, NULL);
  xTaskCreate(taskSession, "SESSION_HANDLE", 1000, NULL, 1, NULL);
  xTaskCreate(taskDisplay, "DISPLAY_HANDLE", 1000, NULL, 2, NULL);
  xTaskCreate(taskSerialPrint, "SERIAL_HANDLE", 1000, NULL, 1, NULL);
  xTaskCreate(taskInflux, "INFLUX_HANDLE", 2000, NULL, 1, NULL);
  xTaskCreate(taskOta, "OTA_HANDLE", 10000, NULL, 1, NULL);

  // Remove Arduino setup and loop tasks
  vTaskDelete(NULL);
}

void loop() {
  ArduinoOTA.handle();
}
