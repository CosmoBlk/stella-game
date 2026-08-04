#pragma once
#include <stdint.h>

// ---------- Screen ----------
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
constexpr int TARGET_FPS = 25;
constexpr uint32_t FRAME_MS = 1000 / TARGET_FPS;

// ---------- Power ----------
constexpr uint32_t IDLE_DIM_MS = 30000;
constexpr uint32_t IDLE_OFF_MS = 300000;
constexpr uint8_t IDLE_DIM_BRIGHTNESS_PCT = 30;
constexpr uint16_t CPU_MHZ = 160;

// ---------- Input ----------
constexpr uint32_t LONG_PRESS_MS = 800;        // navigation long press
constexpr uint32_t TASK_HOLD_MS = 2000;        // hold-B task completion
constexpr uint32_t COMBO_AC_MS = 700;          // A+C together -> player select
constexpr float TILT_DEADZONE = 0.08f;         // g units
constexpr float SHAKE_THRESHOLD = 1.8f;        // g magnitude delta

// ---------- Buddy ----------
constexpr uint8_t NEED_MAX = 100;
constexpr uint32_t NEED_DECAY_INTERVAL_MS = 4UL * 60UL * 1000UL; // -1 need per 4 min runtime
constexpr uint8_t BOOT_GAP_DECAY = 15;         // extra decay applied once per cold boot
constexpr uint8_t NEED_LOW_THRESHOLD = 35;     // below this: sad/hungry/tired expressions

// ---------- Economy ----------
constexpr uint16_t COINS_WALK_FRANKIE = 20;
constexpr uint16_t COINS_DAILY_MISSIONS_ALL = 30;
constexpr uint16_t JELLYBEAN_BAG_PRICE = 10;

// ---------- Daily reset ----------
constexpr uint32_t DAY_RESET_RUNTIME_MS = 24UL * 60UL * 60UL * 1000UL; // continuous-power reset
constexpr uint32_t MIN_PREV_SESSION_MS = 10UL * 60UL * 1000UL;         // boot reset guard
constexpr uint8_t MAX_DAD_MISSIONS_PER_DAY = 3;

// ---------- Limits ----------
constexpr int MAX_ITEMS = 160;
constexpr int MAX_TASKS = 16;
constexpr int MAX_COLLECTIBLES = 96;
constexpr int MAX_GAMES = 8;
constexpr int MAX_DAD_MISSION_RECENT = 8;      // no-repeat window

// ---------- Save ----------
constexpr uint32_t SAVE_MAGIC = 0x50424459;    // "PBDY"
constexpr uint16_t SAVE_VERSION = 2;
constexpr const char* SAVE_PATH = "/save.bin";
constexpr const char* SAVE_TMP_PATH = "/save.tmp";
constexpr const char* SAVE_BAK_PATH = "/save.bak";

// ---------- Timer ----------
constexpr uint16_t GET_DRESSED_SECONDS = 60;
constexpr uint16_t GET_DRESSED_EXTRA_SECONDS = 30;

// ---------- Frankie walk ----------
constexpr uint32_t FRANKIE_WALK_TARGET_MS = 5UL * 60UL * 1000UL; // ~5 min of movement
constexpr float FRANKIE_MOVE_THRESHOLD = 0.15f;                  // accel delta counts as moving
