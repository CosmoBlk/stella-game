#include "Managers.h"
#include "Config.h"
#include <math.h>

namespace {
struct ButtonState {
  bool down;
  bool longSent;
  uint32_t downAt;
  bool pressedEvent;
  bool longEvent;
};

ButtonState buttons[3] = {};
bool comboEvent = false;
bool comboLatched = false;
uint32_t comboAt = 0;

bool pendingShort[3] = {};
bool pendingLong[3] = {};
bool pendingCombo = false;
uint32_t injectedMoveUntil = 0;
uint32_t injectedHeldMs[3] = {};
uint32_t injectedHoldBUntil = 0;

float filteredX = 0.0f;
float filteredY = 0.0f;
float filteredZ = 1.0f;
float baselineX = 0.0f;
float baselineY = 0.0f;
float baselineZ = 1.0f;
float movement = 0.0f;
bool shakeEvent = false;
bool shakeAbove = false;
uint32_t lastShakeMs = 0;

bool physicalDown(uint8_t index) {
  if (index == 0) return M5.BtnA.isPressed();
  if (index == 1) return M5.BtnB.isPressed();
  return M5.BtnC.isPressed();
}

void updateButton(uint8_t index, bool down, uint32_t now) {
  ButtonState& state = buttons[index];
  if (down && !state.down) {
    state.down = true;
    state.longSent = false;
    state.downAt = now;
  } else if (!down && state.down) {
    if (!state.longSent && now - state.downAt < LONG_PRESS_MS) {
      state.pressedEvent = true;
    }
    state.down = false;
    state.longSent = false;
  }

  if (state.down && !state.longSent && now - state.downAt >= LONG_PRESS_MS) {
    state.longSent = true;
    state.longEvent = true;
  }
}

uint32_t heldMs(uint8_t index) {
  const ButtonState& state = buttons[index];
  if (state.down) return millis() - state.downAt;
  return injectedHeldMs[index];
}
}

namespace Input {

void init() {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < 3; ++i) {
    buttons[i] = {};
    if (physicalDown(i)) {
      buttons[i].down = true;
      buttons[i].downAt = now;
    }
  }
  float ax = 0.0f, ay = 0.0f, az = 1.0f;
  if (M5.Imu.getAccelData(&ax, &ay, &az)) {
    filteredX = baselineX = ax;
    filteredY = baselineY = ay;
    filteredZ = baselineZ = az;
  }
}

void poll() {
  const uint32_t now = millis();
  comboEvent = pendingCombo;
  pendingCombo = false;
  shakeEvent = false;

  for (uint8_t i = 0; i < 3; ++i) {
    injectedHeldMs[i] = pendingLong[i] ? LONG_PRESS_MS : 0;
    buttons[i].pressedEvent = pendingShort[i];
    buttons[i].longEvent = pendingLong[i];
    pendingShort[i] = false;
    pendingLong[i] = false;
  }

  bool downA = physicalDown(0);
  bool downB = physicalDown(1) || (injectedHoldBUntil != 0 && (int32_t)(injectedHoldBUntil - now) > 0);
  bool downC = physicalDown(2);
  if (injectedHoldBUntil != 0 && !downB) injectedHoldBUntil = 0;

  updateButton(0, downA, now);
  updateButton(1, downB, now);
  updateButton(2, downC, now);

  if (downA && downC) {
    if (comboAt == 0) comboAt = now;
    if (!comboLatched && now - comboAt >= COMBO_AC_MS) {
      comboLatched = true;
      comboEvent = true;
      buttons[0].longSent = true;
      buttons[2].longSent = true;
    }
  } else {
    comboAt = 0;
    comboLatched = false;
  }

  float ax = 0.0f, ay = 0.0f, az = 1.0f;
  if (M5.Imu.getAccelData(&ax, &ay, &az)) {
    constexpr float alpha = 0.2f;
    constexpr float baseAlpha = 0.025f;
    filteredX += (ax - filteredX) * alpha;
    filteredY += (ay - filteredY) * alpha;
    filteredZ += (az - filteredZ) * alpha;
    baselineX += (filteredX - baselineX) * baseAlpha;
    baselineY += (filteredY - baselineY) * baseAlpha;
    baselineZ += (filteredZ - baselineZ) * baseAlpha;
    const float dx = filteredX - baselineX;
    const float dy = filteredY - baselineY;
    const float dz = filteredZ - baselineZ;
    movement = sqrtf(dx * dx + dy * dy + dz * dz);
    const bool above = movement >= SHAKE_THRESHOLD;
    if (above && !shakeAbove && now - lastShakeMs >= 400) {
      shakeEvent = true;
      lastShakeMs = now;
    }
    shakeAbove = above;
  }
}

bool pressedA() { return buttons[0].pressedEvent; }
bool pressedB() { return buttons[1].pressedEvent; }
bool pressedC() { return buttons[2].pressedEvent; }
bool longA() { return buttons[0].longEvent; }
bool longB() { return buttons[1].longEvent; }
bool longC() { return buttons[2].longEvent; }
bool comboAC() { return comboEvent; }
uint32_t heldMsA() { return heldMs(0); }
uint32_t heldMsB() { return heldMs(1); }
uint32_t heldMsC() { return heldMs(2); }
bool isDownA() { return buttons[0].down; }
bool isDownB() { return buttons[1].down; }
bool isDownC() { return buttons[2].down; }
float tiltX() { return fabsf(filteredX) < TILT_DEADZONE ? 0.0f : filteredX; }
float tiltY() { return fabsf(filteredY) < TILT_DEADZONE ? 0.0f : filteredY; }
float moveMagnitude() {
  if ((int32_t)(injectedMoveUntil - millis()) > 0) {
    return movement > 0.5f ? movement : 0.5f;
  }
  return movement;
}
bool shaken() { return shakeEvent; }

void injectMovement(uint32_t ms) {
  injectedMoveUntil = millis() + ms;
}

void inject(char code) {
  switch (code) {
    case 'a': pendingShort[0] = true; break;
    case 'b': pendingShort[1] = true; break;
    case 'c': pendingShort[2] = true; break;
    case 'A': pendingLong[0] = true; break;
    case 'B': pendingLong[1] = true; break;
    case 'C': pendingLong[2] = true; break;
    case 'x': pendingCombo = true; break;
    case 'h':
      injectedHoldBUntil = millis() + 2100;
      buttons[1].down = true;
      buttons[1].longSent = true;
      buttons[1].downAt = millis() - TASK_HOLD_MS;
      break;
    default: break;
  }
}

}
