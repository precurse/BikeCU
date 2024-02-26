#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"

WiFiManager wifiManager;

// TODO: Move to WifiManager
const char* ota_name = "esp32-bikecu";
const char* ota_password = "foobar";

void taskKeepWifiAlive(void* parameter) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      #ifdef DEBUG
      Serial.print("[WIFI] High water mark (words): ");
      Serial.println(uxTaskGetStackHighWaterMark(NULL));
      #endif
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }
    Serial.println("[WIFI] Connecting");
    WiFi.mode(WIFI_STA);

    //wifiManager.resetSettings(); // wipe settings

    // Setup WifiManager
    bool res;

    // TODO: Options:
      // Bike Name (IC Bike)
      // OTA password
      // HR Monitor on/off
      // InfluxDB on/off
      // InfluxDB destination

    wifiManager.setConfigPortalTimeout(300); // auto close configportal after 5 minutes
    res = wifiManager.autoConnect("BikeCU");
    if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
    }

    unsigned long startAttemptTime = millis();

    // Keep looping while we're not connected and haven't reached the timeout
    while (WiFi.waitForConnectResult() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {}

    // When we couldn't make a WiFi connection (or the timeout expired)
    // sleep for a while and then retry.
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("[WIFI] FAILED");
      vTaskDelay(WIFI_RECOVER_TIME_MS / portTICK_PERIOD_MS);
      continue;
    }
    ArduinoOTA.setHostname(ota_name);
    ArduinoOTA.setPassword(ota_password);
    ArduinoOTA.begin();
    Serial.println("[WIFI] OTA Initialized");
    Serial.println("[WIFI] Connected: ");
    Serial.println(WiFi.localIP());
    configTime(0, 0, NTP_SERVER);
    // Update BT time once time sync'd
    lastBtConnectTime = getUnixTimestamp();
  }
}

String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wifiManager.server->hasArg(name)) {
    value = wifiManager.server->arg(name);
  }
  return value;
}

void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
}
