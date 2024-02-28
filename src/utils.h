#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include <time.h>

time_t getUnixTimestamp();
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

#endif