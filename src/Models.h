#pragma once
#include <stdint.h>
#include "Config.h"

enum class PlayerId : uint8_t { Hugo = 0, Stella = 1 };

enum class ItemCategory : uint8_t {
  Character = 0, Accessory, Clothing, Toy, Room, Treat,
  COUNT
};

enum class ItemOwner : uint8_t { HugoOnly = 0, StellaOnly, Both };

enum class GameId : uint8_t {
  PenaltyKick = 0, FindDinosaur, HugoMaths, RollerCoaster,
  PrincessDance, ColourMatch, BubblePop, SharkSwim,
  COUNT
};

// Buddy animation states (sprite system)
enum class Anim : uint8_t {
  Idle = 0, Happy, Sad, Tired, Eating, Sleeping, Celebrating,
  Walking, Dancing, Jumping, Kicking, Swimming
};

// Collectible groups — fixed id ranges inside collectibles[]
enum class CollectGroup : uint8_t {
  Dinosaur = 0,      // ids 0..11   (12 dinosaurs)
  JellyBean,         // ids 12..21  (10 colours)
  FrankieFind,       // ids 22..33  (12 discoveries)
  DadBadge,          // ids 34..39  (6 badges)
  GameBadge,         // ids 40..47  (8 badges, one per game)
  Shark,             // ids 48..53  (6 sharks)
  COUNT
};
constexpr uint8_t COLLECT_BASE[6] = { 0, 12, 22, 34, 40, 48 };
constexpr uint8_t COLLECT_COUNT[6] = { 12, 10, 12, 6, 8, 6 };

// Events the mission/collection systems listen to
enum class GameEvent : uint8_t {
  TaskCompleted = 0, GetDressedDone, FrankieWalked, DadMissionDone,
  BuddyFed, BuddyPlayed, BuddySlept, GamePlayed,
  MathsCorrect, PenaltyGoal, BubblePopped, ColourFound,
  DinosaurFound, DanceDone, CoinsEarned, ItemBought, JellyBeansEaten
};

struct InventoryState {
  uint8_t ownedBits[MAX_ITEMS / 8 + 1];
  uint16_t equippedCharacter;
  uint16_t equippedAccessory;   // 0xFFFF = none
  uint16_t equippedClothing;    // 0xFFFF = none
  uint16_t equippedToy;         // 0xFFFF = none
  uint16_t equippedRoom;
  bool owned(uint16_t id) const { return ownedBits[id >> 3] & (1 << (id & 7)); }
  void setOwned(uint16_t id) { ownedBits[id >> 3] |= (1 << (id & 7)); }
};

struct BuddyState {
  uint8_t hunger;      // 0 = starving, 100 = full
  uint8_t happiness;
  uint8_t energy;
  uint32_t decayAccumMs;   // runtime accumulator toward next decay tick
  uint8_t jellyBeans;      // jelly beans in pouch (from bags/finds)
};

struct DailyState {
  bool completedTasks[MAX_TASKS];
  uint16_t dailyMissionIds[3];
  uint8_t dailyMissionProgress[3];   // progress count toward target
  bool dailyMissionDone[3];
  bool dailyMissionRewardClaimed;
  uint8_t rewardedDadMissions;
  uint8_t recentDadMissions[MAX_DAD_MISSION_RECENT]; // ring buffer of recent mission ids
  uint8_t recentDadHead;
  uint32_t dayNumber;                // increments each reset
};

struct MathsProgress {
  uint8_t currentLevel;              // 1..5, Hugo starts at 2
  uint16_t correctAnswers;
  uint16_t incorrectAnswers;
  uint8_t categoryStrength[8];       // Add,Sub,Mul,Div,Seq,Cmp,Frac,Missing
  uint8_t consecutiveStrongRounds;
  uint8_t consecutiveWeakRounds;
};

struct PlayerProfile {
  uint8_t id;                        // PlayerId
  uint32_t coins;
  InventoryState inventory;
  BuddyState buddy;
  DailyState daily;
  MathsProgress maths;
  uint8_t collectibles[MAX_COLLECTIBLES / 8 + 1];
  uint16_t gameHighScores[MAX_GAMES];
  uint32_t totalTasksCompleted;
  uint32_t totalDadMissionsCompleted;
  uint32_t totalFrankieWalks;
  uint32_t totalGamesPlayed;
  bool hasCollectible(uint8_t id) const { return collectibles[id >> 3] & (1 << (id & 7)); }
  void setCollectible(uint8_t id) { collectibles[id >> 3] |= (1 << (id & 7)); }
  bool hasCollectible(CollectGroup g, uint8_t idx) const {
    return hasCollectible(COLLECT_BASE[(int)g] + idx);
  }
  void setCollectible(CollectGroup g, uint8_t idx) {
    setCollectible(COLLECT_BASE[(int)g] + idx);
  }
};

struct Settings {
  uint8_t volume;        // 0 = off, 1 = quiet, 2 = normal
  uint8_t brightness;    // 0..255
};

struct TimeState {
  uint32_t pseudoClockMin;       // accumulated runtime minutes across all sessions
  uint32_t lastResetPseudoMin;   // pseudo clock at last daily reset
  uint32_t lastSessionMs;        // runtime length of the previous session (boot-guard)
  uint32_t bootCount;
};

struct SaveData {
  uint32_t magic;
  uint16_t version;
  uint16_t _pad;
  PlayerProfile profiles[2];     // [Hugo, Stella]
  Settings settings;
  TimeState time;
  uint32_t crc;                  // CRC32 of all preceding bytes
};
