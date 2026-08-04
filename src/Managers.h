#pragma once
#include <stdint.h>
#include <M5Unified.h>
#include "Models.h"

// ---------------- Display ----------------
// One full-screen canvas in PSRAM. Screens draw in draw() via Gfx::c().
namespace Gfx {
  void init();
  M5Canvas& c();                 // 320x240 16-bit canvas
  void present();                // push canvas to LCD
}

// ---------------- Input ----------------
// Debounced events per frame + raw hold state + IMU.
namespace Input {
  void init();
  void poll();                   // call once per frame before screen update
  bool pressedA(); bool pressedB(); bool pressedC();   // short press (fired on release)
  bool longA(); bool longB(); bool longC();            // fired once at LONG_PRESS_MS
  bool comboAC();                                      // A+C held together
  uint32_t heldMsA(); uint32_t heldMsB(); uint32_t heldMsC(); // 0 when not held
  bool isDownA(); bool isDownB(); bool isDownC();
  float tiltX(); float tiltY(); // filtered accel, g units, +x = tilt right
  float moveMagnitude();        // |accel delta| for walk detection
  bool shaken();                // shake event this frame
  // Debug injection (serial console QA harness)
  void inject(char code);       // 'a','b','c' short; 'A','B','C' long; 'x' comboAC
}

// ---------------- Audio ----------------
enum class Sfx : uint8_t {
  Select, Back, Coin, Purchase, Pop, Kick, Goal, GoalieSave, Cheer,
  Countdown, CountdownGo, Roar, Splash, Sparkle, MusicCue, JellyBean,
  Bark, SleepMusic, MissionDone, TaskDone, Tick, TimerEnd, GoodTry,
  Celebrate, Eat, Discover, LevelUp
};
namespace Audio {
  void init();
  void update();                 // pump non-blocking sequences
  void play(Sfx s);
  void applyVolume(uint8_t volumeSetting);  // 0 off, 1 quiet, 2 normal
}

// ---------------- Save ----------------
namespace Save {
  void init();                   // mount FS, load or create defaults
  SaveData& data();
  void requestSave();            // debounced: writes within ~1s, safe during games
  void saveNow();                // immediate atomic write (purchases etc.)
  void update();                 // pump debounced writes
  void resetProfileDefaults(PlayerProfile& p, PlayerId id);
}

// ---------------- Time / daily reset ----------------
namespace Clock {
  void init();                   // applies boot logic, may trigger daily reset
  void update(uint32_t deltaMs); // accumulate pseudo clock, 24h runtime reset
  uint32_t dayNumber(PlayerId id);
  void forceDailyReset();        // debug console hook
}

// ---------------- Buddy ----------------
namespace Buddy {
  void update(uint32_t deltaMs);        // decay needs over runtime (active profile only)
  void applyBootDecay(PlayerProfile& p);
  void feed(uint8_t hungerUp, uint8_t happyUp);
  void feedJellyBeans();                // pouch -1, rainbow anim trigger
  void play(); void sleep();
  bool isHungry(); bool isBored(); bool isTired();
  Anim moodAnim();                      // idle/happy/sad/tired based on needs
}
