#include "session.h"

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
    #ifdef FEATURE_WIFI
    if (getUnixTimestamp() < 1000)
    {
      Serial.println("[SESSION] Waiting for NTP to sync");
      vTaskDelay(500 / portTICK_PERIOD_MS);
      continue;
    }
    #endif

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