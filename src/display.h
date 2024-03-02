#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include <Adafruit_ST7735.h>

void updateDisplayMetric(char* metric, int val, Adafruit_ST7735* tft);

void taskDisplay(void* parameter);

#endif
