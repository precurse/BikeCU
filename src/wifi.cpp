#include "config.h"

#ifdef FEATURE_WIFI
#include <WiFi.h>

#ifdef FEATURE_OTA
#include <ArduinoOTA.h>
#endif

WiFiManager wifiManager;
WiFiManagerParameter custom_btle_name("bikeName", "Bike BT Name", "", 40);

void taskOta(void *parameter)
{
  for (;;)
  {
    ArduinoOTA.handle();
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

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

    // wifiManager.resetSettings(); // wipe settings

    // Setup WifiManager
    bool res;

    // TODO: Options:
      // Bike Name (IC Bike)
      // OTA password
      // HR Monitor on/off
      // InfluxDB on/off
      // InfluxDB destination
    // wifiManager.addParameter(&custom_btle_name);

    wifiManager.setConfigPortalTimeout(300); // auto close configportal after 5 minutes
  // std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
  // wifiManager.setMenu(menu);

    res = wifiManager.autoConnect(WIFIMGR_SSID);
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
    #ifdef FEATURE_OTA
    ArduinoOTA.setHostname(OTA_NAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();
    Serial.println("[WIFI] OTA Initialized");
    #endif

    Serial.println("[WIFI] Connected");
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
#endif