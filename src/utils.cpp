#include "config.h"
#include <time.h>

time_t getUnixTimestamp() {
  time_t now;
  time(&now);
  return now;
}

void startSession() {
  stateSession = Running;
  cycleSession->id = getUnixTimestamp();
}

void pauseSession() {
  stateSession = Paused;
  cycleSession->paused_ts = getUnixTimestamp();
}

void resumeSession() {
  stateSession = Running;
  // Calculate paused duration seconds and add to paused time
  time_t ts = getUnixTimestamp();
  uint16_t elapsed = ts - cycleSession->paused_ts;
  cycleSession->paused_t += elapsed;
}

void updateDuration() {
  time_t curTime = getUnixTimestamp();
  cycleSession->duration = curTime - cycleSession->id - cycleSession->paused_t;
}

void updateDistance(uint16_t speedAvg) {
  // Remember: Speed avg in (km/h * 100)
  // Duration in seconds (divide by 3600 to get hours)
  uint32_t tmpDist = (speedAvg * cycleSession->duration) / SECONDS_PER_HOUR;
  cycleSession->distance = static_cast<uint16_t>(tmpDist);
}

void updateCalories(uint16_t powerAvg) {
  // Energy (kcal) = Power (watts) * Time (hours) * 3.6
  
  uint32_t temp_kcal = static_cast<uint32_t>(powerAvg) * 36 * cycleSession->duration;

  // Only need to track whole calories
  cycleSession->calories = static_cast<uint16_t>(temp_kcal / (SECONDS_PER_HOUR * 10));
}

uint16_t getMetricAvg(uint32_t metricAccum, uint16_t readCnt) {
  uint32_t tmpAvg = 0;
  if (readCnt > 0) {
    tmpAvg = metricAccum / readCnt;
  }
  return static_cast<uint16_t>(tmpAvg);
}

uint8_t getHrAvg() {
  return getMetricAvg(cycleSession->data_last.hrAccum, cycleSession->data_last.hrCnt);
}

uint16_t getPowerAvg() {
  return getMetricAvg(cycleSession->data_last.powerAccumu, cycleSession->data_last.metricCnt);
}

uint16_t getCadenceAvg() {
  return getMetricAvg(cycleSession->data_last.cadenceAccum, cycleSession->data_last.metricCnt);
}

uint16_t getSpeedAvg() {
  return getMetricAvg(cycleSession->data_last.speedAccum, cycleSession->data_last.metricCnt);
}

void updateMetrics(time_t ts) {
  cycleSession->ts = ts;
  uint16_t speedAvg = getSpeedAvg();
  uint16_t powerAvg = getPowerAvg();

  // Update calculated fields
  updateDuration();
  updateDistance(speedAvg);
  // Update calories last (relies on duration)
  updateCalories(powerAvg);
}