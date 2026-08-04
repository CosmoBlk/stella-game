#include <Arduino.h>
#include <M5Unified.h>
#include "Config.h"
#include "Managers.h"
#include "App.h"
#include "DebugConsole.h"

namespace {
uint32_t previousFrameMs = 0;
}

void setup() {
  auto config = M5.config();
  config.internal_spk = true;
  config.internal_imu = true;
  M5.begin(config);
  Serial.begin(115200);

  Gfx::init();
  Save::init();
  Clock::init();
  Audio::init();
  Power::init();
  Audio::applyVolume(Save::data().settings.volume);
  M5.Display.setBrightness(Save::data().settings.brightness);
  Input::init();
  App::init();
  previousFrameMs = millis();
  Serial.println("[boot] Pocket Buddy ready");
}

void loop() {
  const uint32_t frameStartedMs = millis();
  uint32_t deltaMs = frameStartedMs - previousFrameMs;
  previousFrameMs = frameStartedMs;
  if (deltaMs > 100) deltaMs = 100;

  M5.update();
  DebugConsole::poll();
  Input::poll();
  Power::update(deltaMs);
  Clock::update(deltaMs);
  Buddy::update(deltaMs);
  Audio::update();
  Save::update();
  App::tick();

  const uint32_t workMs = millis() - frameStartedMs;
  const uint32_t waitMs = workMs < FRAME_MS ? FRAME_MS - workMs : 1;
  vTaskDelay(pdMS_TO_TICKS(waitMs));
}
