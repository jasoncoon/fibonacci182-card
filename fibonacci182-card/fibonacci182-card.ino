/*
   Fibonacci64 Touch Demo: https://github.com/jasoncoon/fibonacci64-touch-demo
   Copyright (C) 2021 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <FastLED.h>  // https://github.com/FastLED/FastLED
#include "Adafruit_FreeTouch.h" //https://github.com/adafruit/Adafruit_FreeTouch
#include "GradientPalettes.h"

FASTLED_USING_NAMESPACE

#define DATA_PIN      A10
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
#define NUM_LEDS      182

#define MILLI_AMPS         1400 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND  1000

CRGB leds[NUM_LEDS];

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];

uint8_t currentPaletteNumber = 0;

CRGBPalette16 currentPalette(CRGB::Black);
CRGBPalette16 targetPalette(gGradientPalettes[0]);

// ten seconds per color palette makes a good demo
// 20-120 is better for deployment
uint8_t secondsPerPalette = 10;

#include "Map.h"

uint8_t brightness = 32;

Adafruit_FreeTouch touch0 = Adafruit_FreeTouch(A0, OVERSAMPLE_4, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch touch1 = Adafruit_FreeTouch(A1, OVERSAMPLE_4, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch touch2 = Adafruit_FreeTouch(A2, OVERSAMPLE_4, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch touch3 = Adafruit_FreeTouch(A3, OVERSAMPLE_4, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch touch4 = Adafruit_FreeTouch(A6, OVERSAMPLE_4, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch touch5 = Adafruit_FreeTouch(A7, OVERSAMPLE_4, RESISTOR_0, FREQ_MODE_NONE);

#define touchPointCount 6

// These values were discovered using the commented-out Serial.print statements in handleTouch below

// minimum values for each touch pad, used to filter out noise
uint16_t touchMin[touchPointCount] = { 558, 259, 418, 368, 368, 368 };

// maximum values for each touch pad, used to determine when a pad is touched
uint16_t touchMax[touchPointCount] = { 1016, 1016, 1016, 1016, 1016, 1016 };

// raw capacitive touch sensor readings
uint16_t touchRaw[touchPointCount] = { 0, 0, 0, 0, 0, 0 };

// capacitive touch sensor readings, mapped/scaled one one byte each (0-255)
uint8_t touch[touchPointCount] = { 0, 0, 0, 0, 0, 0 };

unsigned long touchMillis[touchPointCount] = { 0, 0, 0, 0, 0, 0 };


// coordinates of the touch points
uint8_t touchPointX[touchPointCount] = { 255, 127,   0,   0, 127, 255 };
uint8_t touchPointY[touchPointCount] = {   0,   0,   0, 255, 255, 255 };

unsigned long tapMillis[touchPointCount] = { 0, 0, 0, 0, 0, 0 };

boolean longTouched[touchPointCount] = { false, false, false, false, false, false };

boolean tapped[touchPointCount] = { false, false, false, false, false, false };
boolean doubleTapped[touchPointCount] = { false, false, false, false, false, false };

boolean activeWaves = false;

uint8_t offset = 0;

uint8_t currentPatternIndex = 0;

#include "touchWaves.h"
#include "colorWaves.h"

#include "vector.h"
#include "boid.h"
#include "attractor.h"
#include "attract.h"

typedef void (*Pattern)();
typedef Pattern PatternList[];

PatternList patterns = {
  attract,
  colorWavesFibonacci,
  radiusPalette,
  anglePalette,
  xyPalette,
  xPalette,
  yPalette
};

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

const uint8_t patternCount = ARRAY_SIZE(patterns);

void setup()
{
  Serial.begin(115200);
  //  delay(3000);

  touch0.begin();
  touch1.begin();
  touch2.begin();
  touch3.begin();
  touch4.begin();
  touch5.begin();

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalSMD5050);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  FastLED.setBrightness(brightness);

  startAttract();
}

void loop() {
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random());

  handleTouch();

  for (uint8_t i = 0; i < touchPointCount; i++) {
    if (doubleTapped[i]) {
      tapped[i] = false;
      doubleTapped[i] = false;
      tapMillis[i] = 0;
      currentPatternIndex = (currentPatternIndex + 1) % patternCount;
      break;
    }
  }

  // change to a new cpt-city gradient palette
  EVERY_N_SECONDS( secondsPerPalette ) {
    currentPaletteNumber = addmod8( currentPaletteNumber, 1, gGradientPaletteCount);
    targetPalette = gGradientPalettes[ currentPaletteNumber ];
  }

  EVERY_N_MILLISECONDS(30) {
    // slowly blend the current palette to the next
    nblendPaletteTowardPalette( currentPalette, targetPalette, 8);
    offset++;
  }

  if (!activeWaves)
    patterns[currentPatternIndex]();

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

bool touchChanged = true;

void handleTouch() {
  for (uint8_t i = 0; i < touchPointCount; i++) {
    if (i == 0) touchRaw[i] = touch0.measure();
    else if (i == 1) touchRaw[i] = touch1.measure();
    else if (i == 2) touchRaw[i] = touch2.measure();
    else if (i == 3) touchRaw[i] = touch3.measure();
    else if (i == 4) touchRaw[i] = touch4.measure();
    else if (i == 5) touchRaw[i] = touch5.measure();

    // // uncomment to display raw touch values in the serial monitor/plotter
    //    Serial.print(touchRaw[i]);
    //    Serial.print(" ");

    if (touchRaw[i] < touchMin[i]) {
      touchMin[i] = touchRaw[i];
      touchChanged = true;
    }

    if (touchRaw[i] > touchMax[i]) {
      touchMax[i] = touchRaw[i];
      touchChanged = true;
    }

    boolean touchedBefore = touch[i] > 127;

    touch[i] = map(touchRaw[i], touchMin[i], touchMax[i], 0, 255);

    boolean touched = touch[i] > 127;

    // calculate length of time touched
    if (!touchedBefore && touched) {
      touchMillis[i] = millis();
      Serial.print(i);
      Serial.println(" touched: true");
    } else if (touchedBefore && !touched) {
      touchMillis[i] = 0;
      Serial.print(i);
      Serial.println(" touched: false");
    }

    tapped[i] = false;
    doubleTapped[i] = false;

    // calculate taps & double taps
    if (touchedBefore && !touched && !longTouched[i]) {
      // tapped, check for double-tap
      if (millis() - tapMillis[i] < 2000) {
        doubleTapped[i] = true;
      }

      tapped[i] = true;

      tapMillis[i] = millis();
      Serial.print(i);
      Serial.println(" tapped: true");
    }

    // calculate whether a touch point is being held
    if (!longTouched[i] && touched && millis() - touchMillis[i] > 2000) {
      longTouched[i] = true;
      Serial.print(i);
      Serial.println(" long touched: true");
    } else if (longTouched[i] && !touched) {
      longTouched[i] = false;
      Serial.print(i);
      Serial.println(" long touched: false");
    }

    // // uncomment to display mapped/scaled touch values in the serial monitor/plotter
    //    Serial.print(touch[i]);
    //    Serial.print(" ");
  }

  // // uncomment to display raw and/or mapped/scaled touch values in the serial monitor/plotter
  //  Serial.println();

  // uncomment to display raw, scaled, min, max touch values in the serial monitor/plotter
  //  if (touchChanged) {
  //    for (uint8_t i = 0; i < 4; i++) {
  //      Serial.print(touchRaw[i]);
  //      Serial.print(" ");
  //      Serial.print(touch[i]);
  //      Serial.print(" ");
  //      Serial.print(touchMin[i]);
  //      Serial.print(" ");
  //      Serial.print(touchMax[i]);
  //      Serial.print(" ");
  //    }
  //
  //    Serial.println();
  //
  //    touchChanged = false;
  //  }
}
