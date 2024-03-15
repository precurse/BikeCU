#include "display.h"

void updateDisplayMetric(const char* metric, int val, Adafruit_ST7735* tft) {
  int x = 0;
  int y = 0;
  const char* disp_metric;
  
  if (strcmp(metric, "PWR") == 0) {
    disp_metric = metric;
    x = 5;
    y = 10;
  }

  if (strcmp(metric, "PWRAVG") == 0) {
    disp_metric = "/";
    x = 60;
    y = 10;
  }
  if (strcmp(metric, "PWRMAX") == 0) {
    disp_metric = "/";
    x = 120;
    y = 10;
  }

  if (strcmp(metric, "HR") == 0) {
    disp_metric = metric;
    x = 5;
    y = 30;
  }
  if (strcmp(metric, "HRMAX") == 0) {
    disp_metric = "(M)";
    x = 60;
    y = 30;
  }

  if (strcmp(metric, "RPM") == 0) {
    disp_metric = metric;
    x = 5;
    y = 50;
  }

  if (strcmp(metric, "RPMAVG") == 0) {
    disp_metric = "(A)";
    x = 60;
    y = 50;
  }

  if (strcmp(metric, "SPD") == 0) {
    disp_metric = metric;
    x = 5;
    y = 70;
  }

  if (strcmp(metric, "SPDAVG") == 0) {
    disp_metric = "(A)";
    x = 60;
    y = 70;
  }

  if (strcmp(metric, "DIST") == 0) {
    disp_metric = metric;
    x = 5;
    y = 90;
  }

  if (x != 0 && y != 0) {
    tft->setCursor(x, y);
    tft->print(disp_metric);
    // Padding for 1, 2 and 3 digit values
    if (val < 10) {
      tft->print("  ");
    } else if (val < 100) {
      tft->print(" ");
    }
    tft->print(" ");

    tft->println(val);
  }
}

void taskDisplay(void* parameter) {
  Adafruit_ST7735* tft = new Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);
  bool displayOn = true;
  tft->initR(INITR_BLACKTAB);
  tft->setRotation(1);
  tft->fillScreen(ST7735_BLACK);
  tft->setTextColor(0xFFFF, 0x0000);

  int val = 0;
  int pwr_max = 0;
  int hr_max = 0;

  for (;;) {
    // https://cdn-learn.adafruit.com/downloads/pdf/adafruit-gfx-graphics-library.pdf

    if (getUnixTimestamp() > (lastBtConnectTime + SCREEN_TIMEOUT) && displayOn) {
      // Turn off display if reached timeout
      digitalWrite(TFT_BACKLIGHT, LOW);
      displayOn = false;
    } else if (getUnixTimestamp() < (lastBtConnectTime + SCREEN_TIMEOUT) && !displayOn) {
      // Turn on display if it's off
      digitalWrite(TFT_BACKLIGHT, HIGH);
      displayOn = true;
    }

    val = bikeData.power;
    updateDisplayMetric("PWR", val, tft);
    if (val > pwr_max) {
      pwr_max = val;
    }

    val = getPowerAvg(&cycleSession);
    updateDisplayMetric("PWRAVG", val, tft);
    updateDisplayMetric("PWRMAX", pwr_max, tft);

    val = bikeData.hr;
    updateDisplayMetric("HR", val, tft);
    if (val > hr_max) {
      hr_max = val;
    }
    updateDisplayMetric("HRMAX", hr_max, tft);

    val = bikeData.cadence / 2;
    updateDisplayMetric("RPM", val, tft);

    val = getCadenceAvg(&cycleSession) / 2;  // Cadence must be divided by 2
    updateDisplayMetric("RPMAVG", val, tft);

    val = bikeData.speed;
    updateDisplayMetric("SPD", val, tft);

    val = getSpeedAvg(&cycleSession);
    updateDisplayMetric("SPDAVG", val, tft);

    val = cycleSession.distance;
    updateDisplayMetric("DIST", val, tft);

    // Last line
    tft->setCursor(5, 120);
    tft->print("BT: ");
    switch (stateBleBike) {
      case Initializing:
        tft->println("I");
        break;
      case Searching:
        tft->println("S");
        break;
      case Connecting:
        tft->println("F");
        break;
      case Connected:
        tft->println("C");
        break;
      case Disconnected:
        tft->println("D");
        break;
      default:
        tft->println("?");
    }

    tft->setCursor(90, 120);
    tft->print("B%:");

    val = bikeData.hrBatt;
    if (val < 10) {
      tft->print("   ");
    } else if (val < 100) {
      tft->print("  ");
    } else {
      tft->print(" ");
    }
    // Don't display value if 0
    if (val > 0) {
      tft->print(bikeData.hrBatt);
    }

    tft->setCursor(140, 120);
    //tft->print("S: ");
    switch (cycleSession.sessionState) {
      case NotStarted:
        tft->println("NS");
        break;
      case Running:
        tft->println("RUN");
        break;
      case Paused:
        tft->println("PAU");
        break;
      case Ended:
        tft->println("END");
        break;
      default:
        tft->println("??");
    }
    tft->println();

    // Update every 500ms
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
