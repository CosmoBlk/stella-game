#pragma once
#include <stdint.h>
#include "Models.h"
#include "screens/Screen.h"

enum class ScreenId : uint8_t {
  Boot = 0, PlayerSelect, Home, MainMenu,
  Buddy, Feed, Toy, Sleep,
  CharacterSelect, AccessorySelect, ClothingSelect, RoomSelect,
  Tasks, TaskDetail, TaskComplete,
  GetDressedTimer,
  FrankieWalkIntro, FrankieWalk, FrankieWalkResult,
  DadMission, DadMissionDetail, DadMissionResult,
  DailyMissions,
  CollectionCategory, CollectionGrid,
  GamesMenu,
  PenaltyKickGame, DinosaurGame, MathsGame, RollerCoasterGame,
  PrincessDanceGame, ColourMatchGame, BubblePopGame, SharkSwimGame,
  ShopCategory, ShopBrowse, ShopPreview, PurchaseConfirm, PurchaseResult,
  Settings,
  COUNT
};

class App {
public:
  static void init();
  static void tick();                       // called from loop()

  static void goTo(ScreenId id);
  static void goBack();                     // pops one level (screens set their own parent)
  static ScreenId current();
  static Screen* screen(ScreenId id);

  // Active player context
  static void selectPlayer(PlayerId id);
  static PlayerId player();
  static PlayerProfile& profile();          // active player's profile
  static PlayerProfile& profileOf(PlayerId id);
  static Settings& settings();

  // Economy + events (single funnel: awards animate, save, and feed missions)
  static void awardCoins(uint16_t amount);
  static bool spendCoins(uint16_t amount);  // false if insufficient
  static void raiseEvent(GameEvent ev, uint8_t param = 0, uint8_t count = 1);

  // Cross-screen scratch (which task/mission/item/game is open, results, etc.)
  struct Ctx {
    uint8_t taskId;
    uint8_t dadMissionId;
    uint16_t shopItemId;
    uint8_t shopCategory;      // ItemCategory
    uint8_t gameId;            // GameId
    uint16_t lastReward;       // coins shown on result screens
    uint8_t lastCollectible;   // 0xFF = none, else collectible id
    uint8_t selectSlot;        // shared browse index
    bool fromDailyMissions;
  };
  static Ctx ctx;

  // Frame draw entry — screens draw onto this canvas via Gfx::c()
  static uint32_t nowMs();
};
