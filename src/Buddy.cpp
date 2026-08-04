#include "Managers.h"
#include "App.h"
#include "Config.h"

namespace {
uint32_t decayTicks = 0;

uint8_t addClamped(uint8_t value, uint8_t amount) {
  const uint16_t result = static_cast<uint16_t>(value) + amount;
  return result > NEED_MAX ? NEED_MAX : static_cast<uint8_t>(result);
}

uint8_t subtractClamped(uint8_t value, uint8_t amount) {
  return value > amount ? value - amount : 0;
}
}

namespace Buddy {

void update(uint32_t deltaMs) {
  BuddyState& buddy = App::profile().buddy;
  buddy.decayAccumMs += deltaMs;
  bool changed = false;
  while (buddy.decayAccumMs >= NEED_DECAY_INTERVAL_MS) {
    buddy.decayAccumMs -= NEED_DECAY_INTERVAL_MS;
    ++decayTicks;
    buddy.hunger = subtractClamped(buddy.hunger, 1);
    if (decayTicks % 5 != 0) buddy.happiness = subtractClamped(buddy.happiness, 1);
    if (decayTicks % 5 < 3) buddy.energy = subtractClamped(buddy.energy, 1);
    changed = true;
  }
  if (changed) Save::requestSave();
}

void applyBootDecay(PlayerProfile& profile) {
  profile.buddy.hunger = subtractClamped(profile.buddy.hunger, BOOT_GAP_DECAY);
  profile.buddy.happiness = subtractClamped(profile.buddy.happiness, BOOT_GAP_DECAY);
  profile.buddy.energy = subtractClamped(profile.buddy.energy, BOOT_GAP_DECAY);
}

void feed(uint8_t hungerUp, uint8_t happyUp) {
  BuddyState& buddy = App::profile().buddy;
  buddy.hunger = addClamped(buddy.hunger, hungerUp);
  buddy.happiness = addClamped(buddy.happiness, happyUp);
  Audio::play(Sfx::Eat);
  App::raiseEvent(GameEvent::BuddyFed);
}

void feedJellyBeans() {
  BuddyState& buddy = App::profile().buddy;
  if (buddy.jellyBeans == 0) return;
  --buddy.jellyBeans;
  buddy.happiness = addClamped(buddy.happiness, 10);
  buddy.hunger = addClamped(buddy.hunger, 5);
  Audio::play(Sfx::JellyBean);
  App::raiseEvent(GameEvent::JellyBeansEaten);
}

void play() {
  BuddyState& buddy = App::profile().buddy;
  buddy.happiness = addClamped(buddy.happiness, 25);
  buddy.energy = subtractClamped(buddy.energy, 10);
  Audio::play(Sfx::Cheer);
  App::raiseEvent(GameEvent::BuddyPlayed);
}

void sleep() {
  App::profile().buddy.energy = NEED_MAX;
  Audio::play(Sfx::SleepMusic);
  App::raiseEvent(GameEvent::BuddySlept);
}

bool isHungry() { return App::profile().buddy.hunger < NEED_LOW_THRESHOLD; }
bool isBored() { return App::profile().buddy.happiness < NEED_LOW_THRESHOLD; }
bool isTired() { return App::profile().buddy.energy < NEED_LOW_THRESHOLD; }

Anim moodAnim() {
  const BuddyState& buddy = App::profile().buddy;
  if (buddy.hunger < NEED_LOW_THRESHOLD && buddy.hunger <= buddy.happiness && buddy.hunger <= buddy.energy) return Anim::Sad;
  if (buddy.energy < NEED_LOW_THRESHOLD && buddy.energy <= buddy.happiness) return Anim::Tired;
  if (buddy.happiness < NEED_LOW_THRESHOLD) return Anim::Sad;
  return buddy.happiness > 70 ? Anim::Happy : Anim::Idle;
}

}
