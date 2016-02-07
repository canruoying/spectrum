/**
 * Arduino Spectrum Analyzer with MSGEQ7 Shield
 * and ILI9341 LCD Screen
 * Author: Canruo Ying
 */

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

//================ MSGEQ7 constants =================

#define SS_STROBE 4
#define SS_RESET 5
#define SS_READONE A0
#define SS_READTWO A1
#define SPECTRUMCOUNT 7

//================  LCD display constants ===========

#define TFT_DC 9
#define TFT_CS 10

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

const int BARNUMBER = 14;
const int MAXBARVAL = 40;
const int BARSPACING = 7;
const int DISPLAYWIDTH = 280;
const int DISPLAYHEIGHT = 200;
const int SCREENWIDTH = 320;
const int SCREENHEIGHT = 240;
const int BARWIDTH = (DISPLAYWIDTH - BARSPACING * (BARNUMBER - 1)) / BARNUMBER;
const int LINESPACING = DISPLAYHEIGHT / MAXBARVAL;
const int ORIGINX = (SCREENWIDTH - (BARWIDTH * BARNUMBER + BARSPACING * (BARNUMBER - 1))) / 2;
const int ORIGINY = SCREENHEIGHT - (SCREENHEIGHT - LINESPACING * MAXBARVAL) / 2;

//================ display color constants ==========

const int PALETTECOUNT = 4;
const double THRESHOLD[PALETTECOUNT] = {1, 0.7, 0.5, 0};
const int TAILBRIGHTNESSOFFSET = 3;

//================ display variables ================

double barValue[BARNUMBER];
double prevBarValue[BARNUMBER];
uint16_t palette[BARNUMBER];
uint16_t prevPalette[BARNUMBER];
uint16_t paletteColor[PALETTECOUNT][MAXBARVAL];

//================ tail trace constants =============

const int TRACETIME = 10;
const double TRACEFALLSPEED = 1;
const uint16_t TRACECOLOR = 0xFFFF;

//================ tail trace variables =============

double traceHeight[BARNUMBER];
double prevTraceHeight[BARNUMBER];
int traceTimer[BARNUMBER];

//================ setup ============================

/* Setup & initialize LCD and MSGEQ7 Shield. Define
 * display colors for different amplitudes.
 */
void setup() {
  // initialize MSGEQ7 shield
  initializeSpectrumShield();

  // initialize and start LCD display
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  // define display colors
  makePalette();
}

//================ main loop ========================

/* Retrieve readings from MSGEQ7 and update display.
 */
void loop(void) {
  // retrieve signals from MSGEQ7
  updateValues();

  // determine the color of each column to be displayed.
  changeColor();

  // draw columns
  updateDisplay();

  // draw tail trace
  updateTrace();

  delay(10);
}

//================ initialize MSGEQ7 ================

/* Setup and start the MSGEQ7 shield. Based on sample code
 * provided by Sparkfun.
 * https://learn.sparkfun.com/tutorials/spectrum-shield-hookup-guide
 */
void initializeSpectrumShield() {
  // set spectrum shield pin configurations
  pinMode(SS_STROBE, OUTPUT);
  pinMode(SS_RESET, OUTPUT);
  pinMode(SS_READONE, INPUT);
  pinMode(SS_READTWO, INPUT);  
  digitalWrite(SS_STROBE, HIGH);
  digitalWrite(SS_RESET, HIGH);

  // initialize MSGEQ7
  digitalWrite(SS_STROBE, LOW);
  delay(1);
  digitalWrite(SS_RESET, HIGH);
  delay(1);
  digitalWrite(SS_STROBE, HIGH);
  delay(1);
  digitalWrite(SS_STROBE, LOW);
  delay(1);
  digitalWrite(SS_RESET, LOW);
}

//================ define display palette ===========

/* Make palette for display columns based on amplitude
 * thresholds. For example, a column with max amplitude
 * is displayed in red while small amplitude columns are
 * displayed in blue.
 */
void makePalette() {
  uint16_t red;
  uint16_t green;
  uint16_t blue;
  for (int p = 0; p < PALETTECOUNT; p++) {
    int maxJ = MAXBARVAL * THRESHOLD[max(0, p-1)];
    red = 0;
    green = 0;
    blue = 0;
    for (int j = 0; j < maxJ; j++) {
      if (p == 0 || p == 1) {
        red = map(j, 0, maxJ + TAILBRIGHTNESSOFFSET, 0x1F, 0);
      }
      if (p == 1 || p == 2) {
        green = map(j, 0, maxJ + TAILBRIGHTNESSOFFSET, 0x3F, 0);
      }
      if (p == 3) {
        blue = map(j, 0, maxJ + TAILBRIGHTNESSOFFSET, 0x1F, 0);
      }
      paletteColor[p][j] = (red << 11) + (green << 5) + blue;
    }
  }
}

//================ retrieve signal and update =======

/* Retrieve frequency amplitude readings from MSGEQ7 and
 * update column height.
 */
void updateValues() {
  for (int i = 0; i < SPECTRUMCOUNT; i++) {
    int l = BARNUMBER - 1 - i;
    prevBarValue[i] = barValue[i];
    prevBarValue[l] = barValue[l];
    barValue[i] = map(analogRead(SS_READONE), 50, 1023, 0, MAXBARVAL);
    barValue[l] = map(analogRead(SS_READTWO), 50, 1023, 0, MAXBARVAL);
    digitalWrite(SS_STROBE, HIGH);
    digitalWrite(SS_STROBE, LOW);
    barValue[i] = (floor(barValue[i]) > MAXBARVAL) ? (double) MAXBARVAL : barValue[i];
    barValue[i] = (floor(barValue[i]) < 0) ? 0 : barValue[i];
    barValue[l] = (floor(barValue[l]) > MAXBARVAL) ? (double) MAXBARVAL : barValue[l];
    barValue[l] = (floor(barValue[l]) < 0) ? 0 : barValue[l];
  }
}

//================ change column color ==============

/* Update the color of each column if needed.
 */
void changeColor() {
  for (int i = 0; i < BARNUMBER; i++) {
    prevPalette[i] = palette[i];
    palette[i] = checkThreshold(barValue[i]);
  }
}

//================ update current display ===========

/* Update the current display to reflect new readings.
 */
void updateDisplay() {
  for (int i = 0; i < BARNUMBER; i++) {
    int prev = floor(prevBarValue[i]);
    int val = floor(barValue[i]);
    int x = ORIGINX + i * (BARSPACING + BARWIDTH);
    if (prevPalette[i] != palette[i]) {
      for (int j = 0; j < prev; j++) {
        int y = ORIGINY - j * LINESPACING;
        tft.drawFastHLine(x, y, BARWIDTH, paletteColor[palette[i]][j]);
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
        tft.drawFastHLine(x, y, BARWIDTH, paletteColor[palette[i]][j]);
      }
    }
  }
}

//================ check amplitude threshold ========

/* Helper function for color updater function.
 */
int checkThreshold(double barValue) {
  for (int p = 0; p < PALETTECOUNT; p++) {
    if (barValue >= THRESHOLD[p] * MAXBARVAL) {
      return p;
    }
  }
  return -1;
}

//================ draw tail trace ==================

/* Update and redraw tail trace to reflect recent max
 * value.
 */
void updateTrace() {
  for (int i = 0; i < BARNUMBER; i++) {
    int val = floor(barValue[i]);
    int x = ORIGINX + i * (BARSPACING + BARWIDTH);
    if (floor(traceHeight[i]) <= val * LINESPACING + 1) {
      traceHeight[i] = val * LINESPACING + 1;
      traceTimer[i] = TRACETIME;
    }
    if (traceTimer[i] > 0) {
      traceTimer[i]--;
    } else {
      traceHeight[i] -= TRACEFALLSPEED;
    }
    if (floor(traceHeight[i]) < floor(prevTraceHeight[i]) || (floor(traceHeight[i]) > floor(prevTraceHeight[i]) && (int) floor(prevTraceHeight[i]) % LINESPACING != 0)) {
      tft.drawFastHLine(x, ORIGINY - floor(prevTraceHeight[i]), BARWIDTH, ILI9341_BLACK);
    }
    tft.drawFastHLine(x, ORIGINY - floor(traceHeight[i]), BARWIDTH, TRACECOLOR);
    prevTraceHeight[i] = traceHeight[i];
  }
}

