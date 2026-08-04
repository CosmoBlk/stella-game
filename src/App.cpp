#include "App.h"
#include "Managers.h"
#include "Missions.h"
#include "UI.h"
#include <string.h>

namespace {
Screen* screenTable[static_cast<uint8_t>(ScreenId::COUNT)] = {};
ScreenId parents[static_cast<uint8_t>(ScreenId::COUNT)] = {};
ScreenId activeScreen = ScreenId::Boot;
PlayerId activePlayer = PlayerId::Hugo;
uint32_t frameNow = 0;
uint32_t previousTick = 0;

const char* fallbackName(ScreenId id) {
  static const char* names[] = {
    "BOOT", "PLAYER SELECT", "HOME", "MAIN MENU", "BUDDY", "FEED", "TOY", "SLEEP",
    "CHARACTER", "ACCESSORY", "CLOTHING", "ROOM", "TASKS", "TASK DETAIL", "TASK COMPLETE",
    "GET DRESSED", "FRANKIE INTRO", "FRANKIE WALK", "FRANKIE RESULT", "DAD MISSION",
    "DAD DETAIL", "DAD RESULT", "DAILY MISSIONS", "COLLECTION", "COLLECTION GRID", "GAMES",
    "PENALTY KICK", "DINOSAUR", "MATHS", "ROLLER COASTER", "PRINCESS DANCE", "COLOUR MATCH",
    "BUBBLE POP", "SHARK SWIM", "SHOP", "SHOP BROWSE", "SHOP PREVIEW", "BUY", "PURCHASE",
    "SETTINGS"
  };
  const uint8_t index = static_cast<uint8_t>(id);
  return index < sizeof(names) / sizeof(names[0]) ? names[index] : "UNKNOWN";
}

const char* screenName(ScreenId id) {
  Screen* target = screenTable[static_cast<uint8_t>(id)];
  return target ? target->name() : fallbackName(id);
}

void configureParents() {
  for (uint8_t i = 0; i < static_cast<uint8_t>(ScreenId::COUNT); ++i) parents[i] = ScreenId::Home;
  parents[static_cast<uint8_t>(ScreenId::Boot)] = ScreenId::PlayerSelect;
  parents[static_cast<uint8_t>(ScreenId::PlayerSelect)] = ScreenId::PlayerSelect;
  parents[static_cast<uint8_t>(ScreenId::Home)] = ScreenId::Home;
  parents[static_cast<uint8_t>(ScreenId::MainMenu)] = ScreenId::Home;
  parents[static_cast<uint8_t>(ScreenId::Feed)] = ScreenId::Buddy;
  parents[static_cast<uint8_t>(ScreenId::Toy)] = ScreenId::Buddy;
  parents[static_cast<uint8_t>(ScreenId::Sleep)] = ScreenId::Buddy;
  parents[static_cast<uint8_t>(ScreenId::TaskDetail)] = ScreenId::Tasks;
  parents[static_cast<uint8_t>(ScreenId::TaskComplete)] = ScreenId::Tasks;
  parents[static_cast<uint8_t>(ScreenId::FrankieWalk)] = ScreenId::FrankieWalkIntro;
  parents[static_cast<uint8_t>(ScreenId::FrankieWalkResult)] = ScreenId::FrankieWalkIntro;
  parents[static_cast<uint8_t>(ScreenId::DadMissionDetail)] = ScreenId::DadMission;
  parents[static_cast<uint8_t>(ScreenId::DadMissionResult)] = ScreenId::DadMission;
  parents[static_cast<uint8_t>(ScreenId::CollectionGrid)] = ScreenId::CollectionCategory;
  // Games back out to Home: kids launch them from the home carousel now.
  for (uint8_t i = static_cast<uint8_t>(ScreenId::PenaltyKickGame);
       i <= static_cast<uint8_t>(ScreenId::SharkSwimGame); ++i) {
    parents[i] = ScreenId::Home;
  }
  parents[static_cast<uint8_t>(ScreenId::ShopBrowse)] = ScreenId::ShopCategory;
  parents[static_cast<uint8_t>(ScreenId::ShopPreview)] = ScreenId::ShopBrowse;
  parents[static_cast<uint8_t>(ScreenId::PurchaseConfirm)] = ScreenId::ShopPreview;
  parents[static_cast<uint8_t>(ScreenId::PurchaseResult)] = ScreenId::ShopBrowse;
}
}

App::Ctx App::ctx = {0, 0, 0, 0, 0, 0, 0xFF, 0, false};

void App::init() {
  configureParents();
  frameNow = previousTick = millis();
  activePlayer = PlayerId::Hugo;
  activeScreen = ScreenId::Boot;
  Screen* boot = screen(ScreenId::Boot);
  if (boot) boot->enter();
  else Serial.println("[warn] screen missing: BOOT");
}

void App::tick() {
  frameNow = millis();
  uint32_t deltaMs = frameNow - previousTick;
  previousTick = frameNow;
  if (deltaMs > 100) deltaMs = 100;

  Screen* currentScreen = screen(activeScreen);
  if (!currentScreen) {
    Serial.printf("[warn] screen missing: %s\n", screenName(activeScreen));
    Gfx::present();
    return;
  }

  if (Input::comboAC()) {
    Save::saveNow();
    goTo(ScreenId::PlayerSelect);
    currentScreen = screen(activeScreen);
  }
  if (!currentScreen) return;

  if (Input::pressedA()) currentScreen->onButtonA();
  if (Input::pressedB()) currentScreen->onButtonB();
  if (Input::pressedC()) currentScreen->onButtonC();
  if (Input::longA()) currentScreen->onButtonALong();
  if (Input::longB()) {
    if (currentScreen->allowGlobalBack()) {
      Audio::play(Sfx::Back);
      goBack();
    } else {
      currentScreen->onButtonBLong();
    }
  }
  if (Input::longC()) currentScreen->onButtonCLong();

  currentScreen = screen(activeScreen);
  if (!currentScreen) return;
  currentScreen->update(deltaMs);
  currentScreen->draw();
  UI::tickCoinAnim(deltaMs);
  Gfx::present();
}

void App::goTo(ScreenId id) {
  Screen* target = screen(id);
  if (!target) {
    Serial.printf("[warn] screen missing: %s\n", fallbackName(id));
    return;
  }
  Screen* from = screen(activeScreen);
  Serial.printf("[screen] %s -> %s\n", screenName(activeScreen), target->name());
  if (from) from->exit();
  activeScreen = id;
  target->enter();
}

void App::goBack() {
  const ScreenId parent = parents[static_cast<uint8_t>(activeScreen)];
  if (parent != activeScreen) goTo(parent);
}

ScreenId App::current() { return activeScreen; }

Screen* App::screen(ScreenId id) {
  const uint8_t index = static_cast<uint8_t>(id);
  return index < static_cast<uint8_t>(ScreenId::COUNT) ? screenTable[index] : nullptr;
}

void App::selectPlayer(PlayerId id) {
  activePlayer = id;
  ctx = {0, 0, 0, 0, 0, 0, 0xFF, 0, false};  // no stale context across kids
  Save::requestSave();
}

PlayerId App::player() { return activePlayer; }
PlayerProfile& App::profile() { return Save::data().profiles[static_cast<uint8_t>(activePlayer)]; }
PlayerProfile& App::profileOf(PlayerId id) { return Save::data().profiles[static_cast<uint8_t>(id)]; }
Settings& App::settings() { return Save::data().settings; }

void App::awardCoins(uint16_t amount) {
  if (amount == 0) return;
  profile().coins += amount;
  ctx.lastReward = amount;
  UI::startCoinAnim(amount);
  Audio::play(Sfx::Coin);
  raiseEvent(GameEvent::CoinsEarned, 0, amount > 255 ? 255 : static_cast<uint8_t>(amount));
}

bool App::spendCoins(uint16_t amount) {
  if (profile().coins < amount) {
    Audio::play(Sfx::GoodTry);
    return false;
  }
  profile().coins -= amount;
  Audio::play(Sfx::Purchase);
  // Debounced only: purchase flows saveNow() after the item is granted, so a
  // power cut can't persist spent coins without the purchase.
  Save::requestSave();
  return true;
}

void App::raiseEvent(GameEvent event, uint8_t param, uint8_t count) {
  PlayerProfile& active = profile();
  Missions::onEvent(active, event, param, count);
  switch (event) {
    case GameEvent::TaskCompleted: active.totalTasksCompleted += count; break;
    case GameEvent::DadMissionDone: active.totalDadMissionsCompleted += count; break;
    case GameEvent::FrankieWalked:
      active.totalFrankieWalks += count;
      if (!active.daily.completedTasks[9]) active.daily.completedTasks[9] = true;
      break;
    case GameEvent::GamePlayed: active.totalGamesPlayed += count; break;
    case GameEvent::MathsCorrect: active.maths.correctAnswers += count; break;
    case GameEvent::GetDressedDone:
    case GameEvent::BuddyFed:
    case GameEvent::BuddyPlayed:
    case GameEvent::BuddySlept:
    case GameEvent::PenaltyGoal:
    case GameEvent::BubblePopped:
    case GameEvent::ColourFound:
    case GameEvent::DinosaurFound:
    case GameEvent::DanceDone:
    case GameEvent::CoinsEarned:
    case GameEvent::ItemBought:
    case GameEvent::JellyBeansEaten:
      break;
  }
  Save::requestSave();
}

uint32_t App::nowMs() { return frameNow; }

void App::registerScreen(ScreenId id, Screen& registeredScreen) {
  const uint8_t index = static_cast<uint8_t>(id);
  if (index < static_cast<uint8_t>(ScreenId::COUNT)) screenTable[index] = &registeredScreen;
}
