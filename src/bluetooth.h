#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "config.h"
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>

void scanEndedCB(NimBLEScanResults results);

class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
  void onResult(NimBLEAdvertisedDevice* advertisedDevice);
  void onDisconnect(NimBLEClient* pClient);
};

void taskBLE(void* parameter);

void hrNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                      uint8_t* pData,
                      size_t length,
                      bool isNotify);

void hrBattNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                          uint8_t* pData,
                          size_t length,
                          bool isNotify);

void bikeNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                        uint8_t* pData,
                        size_t length,
                        bool isNotify);

bool bleCharNotify(BLEClient* pClient, const NimBLEUUID bleSvcUUID, const NimBLEUUID bleCharUUID, notify_callback bleNotifyCallback = nullptr);

bool bleCharRead(BLEClient* pClient, const NimBLEUUID bleSvcUUID, const NimBLEUUID bleCharUUID, uint8_t* metric);

bool connectToHr(BLEAddress pAddress);

bool connectToBike(BLEAddress pAddress);

#endif