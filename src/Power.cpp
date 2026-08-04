#include <Arduino.h>
#include "Managers.h"
#include "Config.h"

namespace {
constexpr uint32_t DIM_FADE_MS = 500;

uint32_t idleMs = 0;
uint32_t dimAfterMs = IDLE_DIM_MS;
uint32_t offAfterMs = IDLE_OFF_MS;
uint32_t dimFadeMs = 0;
bool dimmed = false;
bool poweringOff = false;

uint8_t userBrightness() {
  return Save::data().settings.brightness;
}

uint8_t dimBrightness() {
  return static_cast<uint8_t>(static_cast<uint16_t>(userBrightness()) *
                              IDLE_DIM_BRIGHTNESS_PCT / 100);
}

void drawNightNight() {
  M5Canvas& canvas = Gfx::c();
  constexpr uint16_t NIGHT = 0x0862;
  constexpr uint16_t MOON = 0xFFE0;
  constexpr uint16_t STAR = 0xFEA0;
  constexpr uint16_t WHITE = 0xFFFF;

  canvas.fillScreen(NIGHT);

  canvas.fillCircle(242, 77, 35, MOON);
  canvas.fillCircle(257, 65, 34, NIGHT);

  canvas.fillTriangle(82, 57, 72, 87, 92, 87, STAR);
  canvas.fillTriangle(52, 72, 82, 82, 82, 62, STAR);
  canvas.fillTriangle(112, 72, 82, 82, 82, 62, STAR);
  canvas.fillTriangle(62, 105, 82, 82, 74, 78, STAR);
  canvas.fillTriangle(102, 105, 82, 82, 90, 78, STAR);
  canvas.drawLine(70, 78, 76, 80, NIGHT);
  canvas.drawLine(88, 80, 94, 78, NIGHT);
  canvas.drawArc(82, 89, 7, 5, 20, 160, NIGHT);

  canvas.setTextColor(WHITE, NIGHT);
  canvas.setTextDatum(middle_center);
  canvas.setTextSize(2);
  canvas.drawString("NIGHT NIGHT!", SCREEN_W / 2, 164);
  canvas.setTextSize(1);
  canvas.drawString("PRESS MIDDLE BUTTON TO WAKE", SCREEN_W / 2, 197);
  canvas.setTextDatum(top_left);
  Gfx::present();
}

void powerOff() {
  poweringOff = true;
  Save::saveNow();
  drawNightNight();
  Serial.println("[power] off");
  M5.Speaker.stop();
  delay(1500);
  M5.Display.sleep();
  M5.Display.setBrightness(0);
  // Deep sleep, wake on Button B (GPIO38, active low). IP5306 has no true
  // power-off over I2C; deep sleep ~1-2mA is the practical off state.
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_38, 0);
  esp_deep_sleep_start();
}
}

namespace Power {

void init() {
  setCpuFrequencyMhz(CPU_MHZ);
  idleMs = 0;
  Serial.printf("[power] cpu %uMHz\n", CPU_MHZ);
}

void update(uint32_t deltaMs) {
  if (poweringOff) return;

  idleMs += deltaMs;
  if (idleMs >= offAfterMs) {
    powerOff();
    return;
  }

  if (!dimmed && idleMs >= dimAfterMs) {
    dimmed = true;
    dimFadeMs = 0;
    Serial.println("[power] dim");
  }

  if (dimmed && dimFadeMs < DIM_FADE_MS) {
    dimFadeMs += deltaMs;
    if (dimFadeMs > DIM_FADE_MS) dimFadeMs = DIM_FADE_MS;
    const uint8_t bright = userBrightness();
    const uint8_t target = dimBrightness();
    const uint8_t faded = bright - static_cast<uint16_t>(bright - target) * dimFadeMs / DIM_FADE_MS;
    M5.Display.setBrightness(faded);
  }
}

void noteActivity() {
  idleMs = 0;
  if (dimmed) {
    dimmed = false;
    dimFadeMs = 0;
    M5.Display.setBrightness(userBrightness());
  }
}

void debugShortTimers() {
  dimAfterMs = 8000;
  offAfterMs = 20000;
  noteActivity();
  Serial.println("[power] short timers");
}

}
