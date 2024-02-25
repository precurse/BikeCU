#include "config.h"
#include <HTTPClient.h>

void taskInflux(void* parameter) {
  int batchCount;
  String url = String(INFLUX_URL) + "/api/v2/write?bucket=" + INFLUX_DB + "&precision=" + INFLUX_PRECISION;

  for (;;) {
    batchCount = uxQueueMessagesWaiting(dataQueue);

    // Wait for at least 5 metrics
    if (batchCount > 5) {
      String data = "";

      for (int i = 0; i < batchCount; i++) {
        CycleSession session;
        xQueueReceive(dataQueue, &session, 0);

        data += String(INFLUX_DB) + ",id=" + String(session.id) + " "
                + "power_last=" + String(session.data_last.power) + ","
                + "power_avg=" + String(getPowerAvg()) + ","
                + "cadence_last=" + String(session.data_last.cadence / 2) + ","
                + "cadence_avg=" + String(getCadenceAvg() / 2) + ","
                + "speed_last=" + String(session.data_last.speed) + ","
                + "speed_avg=" + String(getSpeedAvg()) + ","
                + "calories=" + String(session.calories) + ","
                + "distance=" + String(session.distance) + ","
                + "duration=" + String(session.duration);

        // Don't send HR if not detected
        if (session.data_last.hrCnt > 0) {
          data += String(",hr_last=") + String(session.data_last.hr) + ","
                  + "hr_avg=" + String(getHrAvg());
          // bytesWritten += snprintf(data + bytesWritten, bs - bytesWritten,
          //                          ",hr_last=%d,hr_avg=%d",
          //                          session.data_last.hr, session.hr_avg);
        }

        data += String(" ") + String(session.ts) + "\n";
      }

      // Send to influx
      HTTPClient http;
      http.begin(url);

      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpCode = http.POST(data);
      // TODO: Do some error handling
      if (httpCode != 204)
        Serial.println("HTTP Response code: " + String(httpCode));
    }
    // Serial.println("[Influx] High water mark (words): ");
    // Serial.println(uxTaskGetStackHighWaterMark(NULL));

    // Run every second
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}