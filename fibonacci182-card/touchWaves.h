
const uint8_t waveCount = 8;

// track the XY coordinates and radius of each wave
uint16_t radii[waveCount];
uint8_t waveX[waveCount];
uint8_t waveY[waveCount];
CRGB waveColor[waveCount];

const uint16_t maxRadius = 512;

void touchWaves() {
  // fade all of the LEDs a small amount each frame
  // increasing this number makes the waves fade faster
  fadeToBlackBy(leds, NUM_LEDS, 30);

  for (uint8_t i = 0; i < touchPointCount; i++) {
    // start new waves when there's a new touch
    if (touch[i] > 127 && radii[i] == 0) {
      radii[i] = 32;
      waveX[i] = touchPointX[i];
      waveY[i] = touchPointY[i];
      waveColor[i] = CHSV(random8(), 255, 255);
    }
  }

  activeWaves = false;

  for (uint8_t i = 0; i < waveCount; i++)
  {
    // increment radii if it's already been set in motion
    if (radii[i] > 0 && radii[i] < maxRadius) radii[i] = radii[i] + 8;

    // reset waves already at max
    if (radii[i] >= maxRadius) {
      activeWaves = true;
      radii[i] = 0;
    }

    if (radii[i] == 0)
      continue;

    activeWaves = true;

    CRGB color = waveColor[i];

    uint8_t x = waveX[i];
    uint8_t y = waveY[i];

    // draw waves starting from the corner closest to each touch sensor
    drawCircle(x, y, radii[i], color, 4);
  }
}
