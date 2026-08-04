#pragma once
#include <stdint.h>
#include "Models.h"

// ---------------- Item catalog ----------------
// Fixed IDs — never renumber. Authoritative table in CATALOG.md. Ranges:
//   0..59    Stella items (chars 0-11, acc 12-26, clothes 27-38, toys 39-48, rooms 49-59)
//   60..120  Hugo items  (chars 60-71, acc 72-87, clothes 88-97, toys 98-109, rooms 110-120)
//   121..126 Shared treats
struct ItemDef {
  uint16_t id;
  ItemCategory category;
  ItemOwner owner;
  const char* name;        // uppercase short label
  uint16_t price;
  uint8_t spriteKey;       // index into sprite/draw dispatch for previews
};
extern const ItemDef ITEMS[];
extern const int ITEM_COUNT;
const ItemDef* itemById(uint16_t id);

// Well-known item ids (defaults / specials)
constexpr uint16_t ITEM_STELLA_DEFAULT_PRINCESS = 0;
constexpr uint16_t ITEM_STELLA_ROOM_CASTLE = 49;   // first Stella room
constexpr uint16_t ITEM_HUGO_DEFAULT_SOCCER = 60;
constexpr uint16_t ITEM_HUGO_ROOM_STADIUM = 110;   // first Hugo room
constexpr uint16_t ITEM_JELLYBEAN_BAG = 121;
constexpr uint16_t ITEM_NONE = 0xFFFF;

// ---------------- Tasks ----------------
struct TaskDef {
  uint8_t id;
  const char* label;       // short uppercase
  uint16_t coins;
  uint8_t iconKey;
};
extern const TaskDef TASKS[];
extern const int TASK_COUNT;
constexpr uint8_t TASK_GET_DRESSED = 1;  // linked to Get Dressed Timer

// ---------------- Dad missions ----------------
struct DadMissionDef {
  uint8_t id;
  const char* text;        // short line, e.g. "FIND SOMETHING RED"
  uint8_t iconKey;
  uint16_t coinsMin, coinsMax;
};
extern const DadMissionDef DAD_MISSIONS[];
extern const int DAD_MISSION_COUNT;

// ---------------- Daily missions ----------------
enum class MissionKind : uint8_t {
  CompleteTask, GetDressed, WalkFrankie, DadMission, FeedBuddy,
  PlayGame,        // param = GameId
  MathsCorrect,    // param = count
  PenaltyGoals, PopBubbles, FindColours, FindDinosaurs, DoDance
};
struct DailyMissionDef {
  uint16_t id;
  MissionKind kind;
  uint8_t param;           // game id or target count depending on kind
  uint8_t target;          // completion count
  const char* text;
  uint8_t iconKey;
  ItemOwner audience;      // which child can roll this mission
};
extern const DailyMissionDef DAILY_MISSIONS[];
extern const int DAILY_MISSION_COUNT;

// ---------------- Dinosaurs ----------------
struct DinoDef {
  uint8_t id;
  const char* name;        // child-friendly label
  bool herbivore;
  bool flies;
  bool horns;
  bool longNeck;
};
extern const DinoDef DINOS[12];

// ---------------- Collectible names ----------------
const char* collectibleName(CollectGroup g, uint8_t idx);
extern const char* JELLYBEAN_COLOURS[10];   // RED..RAINBOW
extern const char* FRANKIE_FINDS[12];
extern const char* SHARK_NAMES[6];
