#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>
#include <NimBLEUUID.h>

// Define flags for BLE Fitness Machine header fields
const uint16_t IS_MORE_DATA = 0x0001;
const uint16_t IS_AVERAGE_SPEED_PRESENT_MASK = 0x0002;
const uint16_t IS_INSTANTANEOUS_CADENCE_PRESENT_MASK = 0x0004;
const uint16_t IS_AVERAGE_CADENCE_PRESENT_MASK = 0x0008;
const uint16_t IS_TOTAL_DISTANCE_PRESENT_MASK = 0x0010;
const uint16_t IS_RESISTANCE_LEVEL_PRESENT = 0x0020;
const uint16_t IS_INSTANTANEOUS_POWER_PRESENT = 0x0040;
const uint16_t IS_AVERAGE_POWER_PRESENT = 0x0080;
const uint16_t IS_EXPENDED_ENERGY_PRESENT = 0x0100;
const uint16_t IS_HEART_RATE_PRESENT = 0x0200;
const uint16_t IS_METABOLIC_EQUIVALENT_PRESENT = 0x0400;
const uint16_t IS_ELAPSED_TIME_PRESENT = 0x0800;
const uint16_t IS_REMAINING_TIME_PRESENT = 0x1000;

// https://gist.github.com/sam016/4abe921b5a9ee27f67b3686910293026
// fitness machine service
static NimBLEUUID fmServiceUUID((uint16_t)0x1826);
// indoor bike data characteristic
static NimBLEUUID indoorBikeDataCharUUID((uint16_t)0x2ad2);
static NimBLEUUID hrServiceUUID((uint16_t)0x180d);
static NimBLEUUID hrCharUUID((uint16_t)0x2a37);
static NimBLEUUID hrBattServiceUUID((uint16_t)0x180f);
static NimBLEUUID hrBattCharUUID((uint16_t)0x2a19);

const uint8_t indicationOn[] = {0x2, 0x0};

#define SECONDS_PER_HOUR 3600

#endif