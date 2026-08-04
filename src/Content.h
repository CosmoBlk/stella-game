#pragma once
#include <stdint.h>
#include "Models.h"

// ---------------- Item catalog ----------------
// Fixed IDs — never renumber. Ranges:
//   0..49    Stella items
//   50..109  Hugo items
//   110..129 Shared treats/misc
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
constexpr uint16_t ITEM_STELLA_ROOM_CASTLE = 38;   // first Stella room
constexpr uint16_t ITEM_HUGO_DEFAULT_SOCCER = 50;
constexpr uint16_t ITEM_HUGO_ROOM_STADIUM = 88;    // first Hugo room
constexpr uint16_t ITEM_JELLYBEAN_BAG = 110;
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
