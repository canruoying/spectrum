#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "EasingLibrary.h"

#define TFT_DC 9
#define TFT_CS 10

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

const int BARNUMBER = 14;
const int MAXBARVAL = 30;
const int BARSPACING = 7;
const int DISPLAYWIDTH = 280;
const int DISPLAYHEIGHT = 200;
const int SCREENWIDTH = 320;
const int SCREENHEIGHT = 240;
const int SAMPLEFRAMERATE = 5;

const int BARWIDTH = (DISPLAYWIDTH - BARSPACING * (BARNUMBER - 1)) / BARNUMBER;
const int LINESPACING = DISPLAYHEIGHT / MAXBARVAL;
const int ORIGINX = (SCREENWIDTH - (BARWIDTH * BARNUMBER + BARSPACING * (BARNUMBER - 1))) / 2;
const int ORIGINY = SCREENHEIGHT - (SCREENHEIGHT - DISPLAYHEIGHT) / 2;

double barValue[BARNUMBER];
double prevBarValue[BARNUMBER];
double targetValue[BARNUMBER];
double savedPositions[BARNUMBER][SAMPLEFRAMERATE];
uint16_t palette[BARNUMBER];
uint16_t prevPalette[BARNUMBER];
uint16_t shade[MAXBARVAL];

int t;
int flag;

void setup() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  for (int j = 0; j < MAXBARVAL; j++) {
    shade[j] = map(j, 0, MAXBARVAL - 1, 0, 0x1f);
    shade[j] = 0;
  }

  flag = 0;
}

void loop(void) {
  if (t % SAMPLEFRAMERATE == 0) {
    t = 0;
    updateTarget();
    calculatePositions();
    flag++;
    changeColor();
  } else {
    delay(10);
  }
  updateValues();
  updateDisplay();
  t++;
}

void changeColor() {
  for (int i = 0; i < BARNUMBER; i++) {
    prevPalette[i] = palette[i];
    if (targetValue[i] > 0.9 * MAXBARVAL) {
      palette[i] = 0xF800;
    } else if (targetValue[i] > 0.5 * MAXBARVAL) {
      palette[i] = 0xFFE0;
    } else if (targetValue[i] > 0.3 * MAXBARVAL) {
      palette[i] = 0x07E0;
    } else {
      palette[i] = 0x001F;
    }
  }
}

void updateTarget() {
  for (int i = 0; i < BARNUMBER; i++) {
    targetValue[i] = exp(random(log((double) MAXBARVAL) * 100.0) / 100.0);
//    targetValue[BARNUMBER - 1 - i] = targetValue[i];
  }
}

void calculatePositions() {
  for (int i = 0; i < BARNUMBER; i++) {
    ExponentialEase ex;
    ex.setDuration(SAMPLEFRAMERATE);
    ex.setTotalChangeInPosition(targetValue[i] - barValue[i]);
    for (int j = 0; j < SAMPLEFRAMERATE; j++) {
      savedPositions[i][j] = barValue[i] + ex.easeOut(j);
    }
  }
}

void updateValues() {
  for (int i = 0; i < BARNUMBER; i++) {
    prevBarValue[i] = barValue[i];
    barValue[i] = savedPositions[i][t];
  }
}

void updateDisplay() {
  for (int i = 0; i < BARNUMBER; i++) {
    int prev = floor(prevBarValue[i]);
    int val = floor(barValue[i]);
    int x = ORIGINX + i * (BARSPACING + BARWIDTH);
    if (prevPalette[i] != palette[i]) {
      for (int j = 0; j < prev; j++) {
        int y = ORIGINY - j * LINESPACING;
        tft.drawFastHLine(x, y, BARWIDTH, palette[i] + shade[j]);
      }
      prevPalette[i] = palette[i];
    }
    if (val < prev) {
      for (int j = prev; j >= val; j--) {
        int y = ORIGINY - j * LINESPACING;
        tft.drawFastHLine(x, y, BARWIDTH, ILI9341_BLACK);
      }
    } else if (val > prev) {
        for (int j = prev; j < val; j++) {
        int y = ORIGINY - j * LINESPACING;
        tft.drawFastHLine(x, y, BARWIDTH, palette[i] + shade[j]);
      }
    }
  }
}
