#include "Managers.h"
#include "Missions.h"
#include "Content.h"
#include "Config.h"
#include <string.h>

namespace {
uint32_t minuteAccumMs = 0;

void resetDaily() {
  for (uint8_t index = 0; index < 2; ++index) {
    PlayerProfile& profile = Save::data().profiles[index];
    memset(profile.daily.completedTasks, 0, sizeof(profile.daily.completedTasks));
    memset(profile.daily.dailyMissionProgress, 0, sizeof(profile.daily.dailyMissionProgress));
    memset(profile.daily.dailyMissionDone, 0, sizeof(profile.daily.dailyMissionDone));
    profile.daily.dailyMissionRewardClaimed = false;
    profile.daily.rewardedDadMissions = 0;
    ++profile.daily.dayNumber;
    Missions::rollDaily(profile);
    Buddy::applyBootDecay(profile);
  }
  Save::data().time.lastResetPseudoMin = Save::data().time.pseudoClockMin;
  Save::requestSave();
  Serial.println("[clock] daily reset");
}
}

namespace Clock {

void init() {
  TimeState& time = Save::data().time;
  ++time.bootCount;
  randomSeed(micros() ^ time.bootCount);
  for (uint8_t index = 0; index < 2; ++index) {
    PlayerProfile& profile = Save::data().profiles[index];
    if (profile.daily.dailyMissionIds[0] == 0xFFFF) {
      Missions::rollDaily(profile);
    }
  }
  const bool newDay = time.lastSessionMs >= MIN_PREV_SESSION_MS;
  time.lastSessionMs = 0;
  if (newDay) resetDaily();
  else Save::requestSave();
}

void update(uint32_t deltaMs) {
  TimeState& time = Save::data().time;
  time.lastSessionMs += deltaMs;
  minuteAccumMs += deltaMs;
  if (minuteAccumMs >= 60000) {
    const uint32_t minutes = minuteAccumMs / 60000;
    minuteAccumMs %= 60000;
    time.pseudoClockMin += minutes;
    Save::requestSave();
  }
  const uint32_t elapsedMinutes = time.pseudoClockMin - time.lastResetPseudoMin;
  if (elapsedMinutes >= DAY_RESET_RUNTIME_MS / 60000UL) resetDaily();
}

uint32_t dayNumber(PlayerId id) {
  return Save::data().profiles[static_cast<uint8_t>(id)].daily.dayNumber;
}

void forceDailyReset() {
  resetDaily();
}

}
