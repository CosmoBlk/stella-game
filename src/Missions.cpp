#include "Missions.h"

#include "Content.h"
#include "Managers.h"

namespace {

bool availableTo(const DailyMissionDef& mission, const PlayerProfile& profile) {
  return mission.audience == ItemOwner::Both ||
         (profile.id == static_cast<uint8_t>(PlayerId::Hugo) &&
          mission.audience == ItemOwner::HugoOnly) ||
         (profile.id == static_cast<uint8_t>(PlayerId::Stella) &&
          mission.audience == ItemOwner::StellaOnly);
}

const DailyMissionDef* missionById(uint16_t id) {
  for (int i = 0; i < DAILY_MISSION_COUNT; ++i) {
    if (DAILY_MISSIONS[i].id == id) return &DAILY_MISSIONS[i];
  }
  return nullptr;
}

bool eventMatches(const DailyMissionDef& mission, GameEvent event, uint8_t param) {
  switch (mission.kind) {
    case MissionKind::CompleteTask:
      return event == GameEvent::TaskCompleted;
    case MissionKind::GetDressed:
      return event == GameEvent::GetDressedDone;
    case MissionKind::WalkFrankie:
      return event == GameEvent::FrankieWalked;
    case MissionKind::DadMission:
      return event == GameEvent::DadMissionDone;
    case MissionKind::FeedBuddy:
      return event == GameEvent::BuddyFed || event == GameEvent::JellyBeansEaten;
    case MissionKind::PlayGame:
      return event == GameEvent::GamePlayed && param == mission.param;
    case MissionKind::MathsCorrect:
      return event == GameEvent::MathsCorrect;
    case MissionKind::PenaltyGoals:
      return event == GameEvent::PenaltyGoal;
    case MissionKind::PopBubbles:
      return event == GameEvent::BubblePopped;
    case MissionKind::FindColours:
      return event == GameEvent::ColourFound;
    case MissionKind::FindDinosaurs:
      return event == GameEvent::DinosaurFound;
    case MissionKind::DoDance:
      return event == GameEvent::DanceDone;
  }
  return false;
}

}

namespace Missions {

void rollDaily(PlayerProfile& profile) {
  for (uint8_t slot = 0; slot < 3; ++slot) {
    profile.daily.dailyMissionIds[slot] = 0xFFFF;
    profile.daily.dailyMissionProgress[slot] = 0;
    profile.daily.dailyMissionDone[slot] = false;
  }
  profile.daily.dailyMissionRewardClaimed = false;

  for (uint8_t slot = 0; slot < 3; ++slot) {
    int eligibleCount = 0;
    for (int i = 0; i < DAILY_MISSION_COUNT; ++i) {
      if (!availableTo(DAILY_MISSIONS[i], profile)) continue;
      bool alreadyChosen = false;
      for (uint8_t prior = 0; prior < slot; ++prior) {
        if (profile.daily.dailyMissionIds[prior] == DAILY_MISSIONS[i].id) {
          alreadyChosen = true;
          break;
        }
      }
      if (!alreadyChosen) ++eligibleCount;
    }

    if (eligibleCount == 0) continue;
    int pick = random(eligibleCount);
    for (int i = 0; i < DAILY_MISSION_COUNT; ++i) {
      if (!availableTo(DAILY_MISSIONS[i], profile)) continue;
      bool alreadyChosen = false;
      for (uint8_t prior = 0; prior < slot; ++prior) {
        if (profile.daily.dailyMissionIds[prior] == DAILY_MISSIONS[i].id) {
          alreadyChosen = true;
          break;
        }
      }
      if (alreadyChosen) continue;
      if (pick-- == 0) {
        profile.daily.dailyMissionIds[slot] = DAILY_MISSIONS[i].id;
        break;
      }
    }
  }

  Save::requestSave();
}

void onEvent(PlayerProfile& profile, GameEvent event, uint8_t param, uint8_t count) {
  if (count == 0) return;
  bool changed = false;

  for (uint8_t slot = 0; slot < 3; ++slot) {
    if (profile.daily.dailyMissionDone[slot]) continue;
    const DailyMissionDef* mission = missionById(profile.daily.dailyMissionIds[slot]);
    if (mission == nullptr || !eventMatches(*mission, event, param)) continue;

    const uint16_t progress = static_cast<uint16_t>(profile.daily.dailyMissionProgress[slot]) + count;
    profile.daily.dailyMissionProgress[slot] =
      progress >= mission->target ? mission->target : static_cast<uint8_t>(progress);
    profile.daily.dailyMissionDone[slot] =
      profile.daily.dailyMissionProgress[slot] >= mission->target;
    changed = true;
  }

  if (changed) Save::requestSave();
}

bool allDone(const PlayerProfile& profile) {
  return profile.daily.dailyMissionDone[0] &&
         profile.daily.dailyMissionDone[1] &&
         profile.daily.dailyMissionDone[2];
}

}
