#ifndef UTILS_H
#define UTILS_H

#include "config.h"
#include "types.h"
#include "constants.h"
#include <time.h>

time_t getUnixTimestamp();
void initializeBikeData(LiveBikeData* bikeData);
void initializeSession(LiveBikeData* bd, CycleSession* cs);
void printBikeData(CycleSession* cs);
void startSession(CycleSession* cs);
void pauseSession(CycleSession* cs);
void resumeSession(CycleSession* cs);
void updateDuration(CycleSession* cs);
void updateDistance(CycleSession* cs, uint16_t speedAvg);
void updateCalories(CycleSession* cs, uint16_t powerAvg);
uint16_t getMetricAvg(uint32_t metricAccum, uint16_t readCnt);
uint8_t getHrAvg(CycleSession* cs);
uint16_t getPowerAvg(CycleSession* cs);
uint16_t getCadenceAvg(CycleSession* cs);
uint16_t getSpeedAvg(CycleSession* cs);
void updateMetrics(CycleSession* cs, time_t ts);
void SerialQueueSend(String msg);
void SerialQueueSend(int msg);
void taskSerialPrint(void *parameter);

#endif