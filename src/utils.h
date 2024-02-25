#ifndef UTILS_H
#define UTILS_H

#include "config.h"
#include <time.h>

time_t getUnixTimestamp();

void startSession();

void pauseSession();

void resumeSession();

void updateDuration();

void updateDistance(uint16_t speedAvg);

void updateCalories(uint16_t powerAvg);

uint16_t getMetricAvg(uint32_t metricAccum, uint16_t readCnt);

uint8_t getHrAvg();

uint16_t getPowerAvg();

uint16_t getCadenceAvg();

uint16_t getSpeedAvg();

void updateMetrics(time_t ts);

#endif