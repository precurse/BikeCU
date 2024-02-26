#include "config.h"
#include <NimBLEUUID.h>
#include <NimBLEAddress.h>
#include <NimBLEClient.h>
#include "NimBLEDevice.h"

time_t lastBtConnectTime = 0;
StateBle stateBleBike;
StateBle stateBleHr;

// https://gist.github.com/sam016/4abe921b5a9ee27f67b3686910293026
// fitness machine service
static NimBLEUUID fmServiceUUID((uint16_t)0x1826);
// indoor bike data characteristic
static NimBLEUUID indoorBikeDataCharUUID((uint16_t)0x2ad2);
static NimBLEUUID hrServiceUUID((uint16_t)0x180d);
static NimBLEUUID hrCharUUID((uint16_t)0x2a37);
static NimBLEUUID hrBattServiceUUID((uint16_t)0x180f);
static NimBLEUUID hrBattCharUUID((uint16_t)0x2a19);

static NimBLEAddress *pBikeAddress;
static NimBLEAddress *pHrAddress;
NimBLEClient *pBikeClient;
NimBLEClient *pHrClient;

// static BLERemoteCharacteristic* bikeCharacteristic;
static NimBLERemoteDescriptor *bikeDescriptor;

void scanEndedCB(NimBLEScanResults results)
{
  Serial.println("Scan Ended");
}

void bikeNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                        uint8_t *pData,
                        size_t length,
                        bool isNotify)
{
  lastBtConnectTime = getUnixTimestamp();

  // TODO: Make more dynamic. Check header flags

  if (length >= 9)
  {
    // Extracting values directly by casting bytes
    if (xSemaphoreTake(bikeDataMutex, portMAX_DELAY) == pdTRUE)
    {
      bikeData->header = *((uint16_t *)&pData[0]);
      bikeData->speed = *((uint16_t *)&pData[2]);
      bikeData->cadence = *((uint16_t *)&pData[4]);
      bikeData->power = *((uint16_t *)&pData[6]);
      uint8_t hrLive = pData[8];
      // Only use Bike HR data if present
      if (hrLive > 0)
      {
        bikeData->hr = hrLive; // Fourth unsigned char (1 byte)
      }

      // if session is running, track accumulators for averages
      if (stateSession == Running)
      {
        bikeData->speedAccum += bikeData->speed;
        bikeData->cadenceAccum += bikeData->cadence;
        bikeData->powerAccumu += bikeData->power;
        bikeData->metricCnt++;

        // HR may not be present
        if (hrLive > 0)
        {
          bikeData->hrAccum += bikeData->hr;
          bikeData->hrCnt++;
        }
      }
      // Done with updating data
      xSemaphoreGive(bikeDataMutex);
    }
  }
}

bool bleCharNotify(BLEClient *pClient, const NimBLEUUID bleSvcUUID, const NimBLEUUID bleCharUUID, notify_callback bleNotifyCallback = nullptr)
{
  BLERemoteService *pRemoteService = pClient->getService(bleSvcUUID);
  Serial.println("[BLECharNotify] Begin");
  if (pRemoteService == nullptr)
  {
    Serial.print("[BLE] Failed to find service UUID");
    Serial.println(bleSvcUUID.toString().c_str());
    return false;
  }
  Serial.println("[BLECharNotify] found serviceUUID");

  BLERemoteCharacteristic *bleChar = pRemoteService->getCharacteristic(bleCharUUID);

  if (bleChar == nullptr)
  {
    Serial.print("[BLE] Failed to find characteristic UUID");
    Serial.println(bleCharUUID.toString().c_str());
    return false;
  }
  Serial.println("[BLECharNotify] found charUUID");

  if (bleChar->canNotify())
  {
    if (!bleChar->subscribe(true, bleNotifyCallback))
    {
      return false;
    }
  }
  return true;
}

void hrNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                      uint8_t *pData,
                      size_t length,
                      bool isNotify)
{
  // Serial.print("[BLEData] HR length: ");
  // Serial.println(length);
  // HR has an 8-bit header
  // https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.characteristic.heart_rate_measurement.xml
  if (length >= 2)
  {
    // We should really check the header whether HR is uint8 or uint16
    uint8_t header = pData[0];

    if (xSemaphoreTake(bikeDataMutex, portMAX_DELAY) == pdTRUE)
    {
      uint8_t hrLive = pData[1];

      bikeData->hr = hrLive;
      if (stateSession == Running)
      {
        bikeData->hrAccum += hrLive;
        bikeData->hrCnt++;
      }
      // Done with bike data
      xSemaphoreGive(bikeDataMutex);
    }
  }
}

bool connectToBike(BLEAddress pAddress)
{
  pBikeClient = BLEDevice::createClient();
  Serial.println("[BLE] Trying to connect to bike");
  pBikeClient->connect(pAddress, true);
  stateBleBike = Connected;
  Serial.println("[BLE]  - Connected to server");

  // Subscribe to Fitness Machine indoor bike data
  if (!bleCharNotify(pBikeClient, fmServiceUUID, indoorBikeDataCharUUID, bikeNotifyCallback))
  {
    Serial.println("[BLE]  - Subscribed to Fitness Machine");
    if (pBikeClient != nullptr)
    {
      Serial.println("[BLE]  - Fitness machine had null pointer");
      NimBLEDevice::deleteClient(pBikeClient);
    }
    return false;
  }
  return true;
}

bool bleCharRead(BLEClient *pClient, const NimBLEUUID bleSvcUUID, const NimBLEUUID bleCharUUID, uint8_t *metric)
{
  BLERemoteService *pRemoteService = pClient->getService(bleSvcUUID);

  if (pRemoteService == nullptr)
  {
    Serial.print("[BLE] Failed to find service UUID");
    Serial.println(bleSvcUUID.toString().c_str());
    return false;
  }

  BLERemoteCharacteristic *bleChar = pRemoteService->getCharacteristic(bleCharUUID);

  if (bleChar == nullptr)
  {
    Serial.print("[BLE] Failed to find characteristic UUID");
    Serial.println(bleCharUUID.toString().c_str());
    return false;
  }

  // Ensure we can read characteristic
  if (bleChar->canRead())
  {
    *metric = bleChar->readValue<uint8_t>();
  }
  return true;
}

void hrBattNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                          uint8_t *pData,
                          size_t length,
                          bool isNotify)
{
  Serial.print("[BLEData] HR Batt length: ");
  Serial.println(length);
  // org.bluetooth.unit.percentage is 8-bit
  // https://github.com/sputnikdev/bluetooth-gatt-parser/blob/master/src/main/resources/gatt/characteristic/org.bluetooth.characteristic.battery_level.xml
  if (length >= 1)
  {
    // uint8_t header = pData[0];
    if (xSemaphoreTake(bikeDataMutex, portMAX_DELAY) == pdTRUE)
    {
      bikeData->hrBatt = pData[0];
      // Done with updating data
      xSemaphoreGive(bikeDataMutex);
    }
  }
}

bool connectToHr(BLEAddress pAddress)
{
  pHrClient = BLEDevice::createClient();
  pHrClient->connect(pAddress, false);
  stateBleHr = Connected;
  Serial.println("[BLE] Connected to HR device");

  // Subscribe to HR metricsew
  if (!bleCharNotify(pHrClient, hrServiceUUID, hrCharUUID, hrNotifyCallback))
  {
    if (pHrClient != nullptr)
    {
      NimBLEDevice::deleteClient(pHrClient);
    }
    return false;
  }

  // Read HR battery at start
  bleCharRead(pHrClient, hrBattServiceUUID, hrBattCharUUID, &bikeData->hrBatt);

  // HR battery updates will then be pushed
  bleCharNotify(pHrClient, hrBattServiceUUID, hrBattCharUUID, hrBattNotifyCallback);

  return true;
}

// Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{
  void onResult(NimBLEAdvertisedDevice *advertisedDevice)
  {
    // Bike Connect
    if (advertisedDevice->getName() == BLE_NAME && stateBleBike == Searching && advertisedDevice->isAdvertisingService(fmServiceUUID))
    { // Check if the name of the advertiser matches
      // if (advertisedDevice->isAdvertisingService(fmServiceUUID) && stateBleBike == Searching) {
      stateBleBike = Connecting;
      advertisedDevice->getScan()->stop();                           // Scan can be stopped, we found what we are looking for
      pBikeAddress = new BLEAddress(advertisedDevice->getAddress()); // Address of advertiser is the one we need
      Serial.print("[BLE] Bike found: ");
      Serial.println(advertisedDevice->toString().c_str());
      Serial.print("[BLE] Bike RSSI: ");
      Serial.println(advertisedDevice->getRSSI());
    }
    // Connect to HR
    if (advertisedDevice->isAdvertisingService(hrServiceUUID) && stateBleHr == Searching)
    {
      stateBleHr = Connecting;
      pHrAddress = new BLEAddress(advertisedDevice->getAddress());
      Serial.print("[BLE] HR device found: ");
      Serial.println(advertisedDevice->toString().c_str());
      Serial.print("[BLE] HR RSSI: ");
      Serial.println(advertisedDevice->getRSSI());
    }
  }

  void onDisconnect(NimBLEClient *pClient)
  {
    // Fix for HR and Bike
    stateBleBike = Disconnected;
    NimBLEDevice::getScan()->start(0, scanEndedCB);
  }
};

void taskBLE(void *parameter)
{
  Serial.println("Starting BLE");
  stateBleBike = Initializing;
  lastBtConnectTime = getUnixTimestamp();
  BLEDevice::init("");
  BLEScan *pScan = BLEDevice::getScan();
  stateBleBike = Searching;
  stateBleHr = Searching;
  pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

  /** Set scan interval (how often) and window (how long) in milliseconds */
  pScan->setInterval(45);
  pScan->setWindow(15);

  pScan->setActiveScan(true);
  pScan->start(0, scanEndedCB);
  while (1)
  {
    // If bike found, connect
    if (stateBleBike == Connecting)
    {
      // Connect to bike if not already connected
      if (connectToBike(*pBikeAddress))
      {
        Serial.println("[BLE] Bike Connected!");
        stateBleBike = Connected;
      }
      else
      {
        stateBleBike = Disconnected;
      }
      // Continue rescanning
      pScan->start(0, scanEndedCB);
    }

    // If HR found, connect
    if (stateBleHr == Connecting)
    {
      if (connectToHr(*pHrAddress))
      {
        Serial.println("[BLE] Hr Connected!");
        stateBleHr = Connected;
      }
      else
      {
        stateBleHr = Disconnected;
      }
      // Continue rescanning
      pScan->start(0, scanEndedCB);
    }

    // Stop rescanning if all devices connected
    if (stateBleBike == Connected && stateBleHr == Connected)
    {
      pScan->stop();
    }
    // Check if bike and HR still connected
    if (stateBleBike == Connected)
    {
      // Check if still connected
      if (!pBikeClient->isConnected())
      {
        stateBleBike = Disconnected;
        NimBLEDevice::deleteClient(pBikeClient);
        pScan->start(0, scanEndedCB);
      }
    }
    if (stateBleHr == Connected)
    {
      // Check if still connected
      if (!pHrClient->isConnected())
      {
        stateBleHr = Disconnected;
        NimBLEDevice::deleteClient(pHrClient);
        pScan->start(0, scanEndedCB);
      }
    }
    // Serial.println("[BLE] Ending taskBLE loop");
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    // print remaining
    // Serial.print("[BLE] High water mark (words): ");
    // Serial.println(uxTaskGetStackHighWaterMark(NULL));
  }
}