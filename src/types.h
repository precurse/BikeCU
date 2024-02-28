#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <time.h>

enum StateBle
{
  Initializing,
  Searching,
  Connecting,
  Connected,
  Disconnected
};

enum SessionState
{
  NotStarted,
  Running,
  Paused,
  Ended
};

// Live data
struct LiveBikeData
{
  uint16_t header;
  uint16_t speed;   // in (KM/h * 100) i.e. 3600 = 36KM/h
  uint16_t cadence; // in RPM (x2)
  uint16_t power;   // in W
  uint8_t hr;       // in bpm
  uint8_t hrBatt;   // in pct (battery level)
  // Accumuators for calculating averages
  uint32_t speedAccum;
  uint32_t cadenceAccum;
  uint32_t powerAccumu;
  uint32_t hrAccum;

  uint16_t metricCnt;
  uint16_t hrCnt;
};

struct CycleSession
{
  time_t id; // Session ID (Unix Timestamp)
  time_t ts; // Timestamp of metric calculations
  SessionState sessionState;
  uint16_t calories;
  uint16_t distance; // Distance (meters)
  uint16_t duration; // Duration (seconds)
  uint16_t paused_t; // Total time paused (seconds)
  time_t paused_ts;  // Timestamp session paused (Unix Timestamp)
  LiveBikeData data_last;
};

#endif