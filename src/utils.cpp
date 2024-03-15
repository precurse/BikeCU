#include "config.h"
#include "types.h"
#include "constants.h"
#include <time.h>

time_t getUnixTimestamp() {
  time_t now;
  time(&now);
  return now;
}

void startSession(CycleSession* cs) {
  cs->sessionState = Running;
  cs->id = getUnixTimestamp();
}

void pauseSession(CycleSession* cs) {
  cs->sessionState = Paused;
  cs->paused_ts = getUnixTimestamp();
}

void resumeSession(CycleSession* cs) {
  cs->sessionState = Running;
  // Calculate paused duration seconds and add to paused time
  time_t ts = getUnixTimestamp();
  uint16_t elapsed = ts - cs->paused_ts;
  cs->paused_t += elapsed;
}

void updateDuration(CycleSession* cs) {
  time_t curTime = getUnixTimestamp();
  cs->duration = curTime - cs->id - cs->paused_t;
}

void updateDistance(CycleSession* cs, uint16_t speedAvg) {
  // Remember: Speed avg in (km/h * 100)
  // Duration in seconds (divide by 3600 to get hours)
  uint32_t tmpDist = (speedAvg * cs->duration) / SECONDS_PER_HOUR;
  cs->distance = static_cast<uint16_t>(tmpDist);
}

void updateCalories(CycleSession* cs, uint16_t powerAvg) {
  // Energy (kcal) = Power (watts) * Time (hours) * 3.6
  
  uint32_t temp_kcal = static_cast<uint32_t>(powerAvg) * 36 * cs->duration;

  // Only need to track whole calories
  cs->calories = static_cast<uint16_t>(temp_kcal / (SECONDS_PER_HOUR * 10));
}

uint16_t getMetricAvg(uint32_t metricAccum, uint16_t readCnt) {
  uint32_t tmpAvg = 0;
  if (readCnt > 0) {
    tmpAvg = metricAccum / readCnt;
  }
  return static_cast<uint16_t>(tmpAvg);
}

uint8_t getHrAvg(CycleSession* cs) {
  return getMetricAvg(cs->data_last.hrAccum, cs->data_last.hrCnt);
}

uint16_t getPowerAvg(CycleSession* cs) {
  return getMetricAvg(cs->data_last.powerAccumu, cs->data_last.metricCnt);
}

uint16_t getCadenceAvg(CycleSession* cs) {
  return getMetricAvg(cs->data_last.cadenceAccum, cs->data_last.metricCnt);
}

uint16_t getSpeedAvg(CycleSession* cs) {
  return getMetricAvg(cs->data_last.speedAccum, cs->data_last.metricCnt);
}

void updateMetrics(CycleSession* cs, time_t ts) {
  cs->ts = ts;
  uint16_t speedAvg = getSpeedAvg(cs);
  uint16_t powerAvg = getPowerAvg(cs);

  // Update calculated fields
  updateDuration(cs);
  updateDistance(cs, speedAvg);
  // Update calories last (relies on duration)
  updateCalories(cs, powerAvg);
}
