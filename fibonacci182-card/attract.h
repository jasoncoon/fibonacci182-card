/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

const int count = 2;
Attractor attractor = Attractor(128, 128);

void startAttract() {
  int direction = random(0, 2);
  if (direction == 0)
    direction = -1;

  for (int i = 0; i < count; i++) {
    Boid boid = Boid(128, 240 - (i * 16));
    boid.mass = 1; // random(0.1, 2);
    boid.velocity.x = ((float) random(60, 70)) / 100.0;
    boid.velocity.x *= direction;
    boid.velocity.y = 0;
    boid.colorIndex = i * 32;
    boids[i] = boid;
    //dim = random(170, 250);
  }
}

void attract() {
  fadeToBlackBy(leds, NUM_LEDS, 2);

  int touchIndex = -1;
  for (uint8_t i = 0; i < touchPointCount; i++) {
    if (touch[i] > 127) {
      touchIndex = i;
      break;
    }
  }

  if (touchIndex > -1 && touchIndex < touchPointCount) {
    if (thickness < 24)
      thickness++;
    attractor.location = PVector(touchPointX[touchIndex], touchPointY[touchIndex]);
  } else {
    if (thickness > 8)
      thickness--;
    attractor.location = PVector(128, 128);
  }

  for (int i = 0; i < count; i++) {
    Boid boid = boids[i];

    PVector force = attractor.attract(boid);
    boid.applyForce(force);
    boid.bounceOffBorders(0.25);

    boid.update();
    CRGB color = ColorFromPalette(currentPalette, boid.colorIndex);
    addColorXY(boid.location.x, boid.location.y, color, thickness);

    boids[i] = boid;
  }
}