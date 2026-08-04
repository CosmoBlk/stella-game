#include "App.h"
#include "Config.h"
#include "Content.h"
#include "Managers.h"
#include "Missions.h"
#include "UI.h"

#include <stdio.h>
#include <string.h>

namespace {

const DailyMissionDef* missionById(uint16_t id) {
  for (int i = 0; i < DAILY_MISSION_COUNT; ++i) {
    if (DAILY_MISSIONS[i].id == id) return &DAILY_MISSIONS[i];
  }
  return nullptr;
}

const char* collectibleLabel(uint8_t id) {
  for (uint8_t group = 0; group < static_cast<uint8_t>(CollectGroup::COUNT); ++group) {
    if (id >= COLLECT_BASE[group] && id < COLLECT_BASE[group] + COLLECT_COUNT[group]) {
      return collectibleName(static_cast<CollectGroup>(group), id - COLLECT_BASE[group]);
    }
  }
  return "SURPRISE!";
}

class DailyMissionsScreen final : public Screen {
public:
  void enter() override {
    selected_ = selected_ > 2 ? 0 : selected_;
    celebrationMs_ = 0;
    rewardKind_ = RewardKind::None;
    rewardCollectible_ = 0xFF;
  }

  void update(uint32_t deltaMs) override {
    if (celebrationMs_ > deltaMs) celebrationMs_ -= deltaMs;
    else celebrationMs_ = 0;
  }

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("TODAY'S MISSIONS");

    for (uint8_t slot = 0; slot < 3; ++slot) drawMissionRow(slot, 35 + slot * 55);

    const DailyState& daily = App::profile().daily;
    if (Missions::allDone(App::profile()) && !daily.dailyMissionRewardClaimed) {
      const bool pulse = ((App::nowMs() / 260) & 1) == 0;
      M5Canvas& canvas = Gfx::c();
      canvas.fillRoundRect(66, 194, 188, 10, 5, pulse ? TFT_YELLOW : TFT_ORANGE);
      UI::drawButtonBar("PREV", "CLAIM", "NEXT",
                        IconId::ArrowL, IconId::Gift, IconId::ArrowR);
    } else {
      UI::drawButtonBar("PREV", "OPEN", "NEXT",
                        IconId::ArrowL, IconId::Tick, IconId::ArrowR);
    }

    if (celebrationMs_ > 0) drawCelebration();
  }

  void onButtonA() override {
    if (celebrationMs_ > 0) return;
    selected_ = selected_ == 0 ? 2 : selected_ - 1;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    if (celebrationMs_ > 0) return;
    PlayerProfile& profile = App::profile();
    if (Missions::allDone(profile) && !profile.daily.dailyMissionRewardClaimed) {
      claimReward(profile);
      return;
    }
    openMission();
  }

  void onButtonC() override {
    if (celebrationMs_ > 0) return;
    selected_ = (selected_ + 1) % 3;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "DailyMissions"; }

private:
  enum class RewardKind : uint8_t { None, Coins, Collectible, JellyBeans };

  uint8_t selected_ = 0;
  uint32_t celebrationMs_ = 0;
  RewardKind rewardKind_ = RewardKind::None;
  uint8_t rewardCollectible_ = 0xFF;

  void drawMissionRow(uint8_t slot, int y) const {
    const PlayerProfile& profile = App::profile();
    const DailyMissionDef* mission = missionById(profile.daily.dailyMissionIds[slot]);
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();
    const bool selected = slot == selected_;
    const bool done = profile.daily.dailyMissionDone[slot];
    const int x = selected ? 9 : 15;
    const int width = selected ? 302 : 290;

    UI::drawPanel(x, y, width, 49, theme.panel);
    canvas.drawRoundRect(x, y, width, 49, 11, selected ? theme.accent : theme.accent2);
    if (mission == nullptr) {
      UI::drawBigCentred("NEW MISSION SOON!", y + 24, 1, theme.text);
      return;
    }

    UI::drawIcon(static_cast<IconId>(mission->iconKey), x + 28, y + 24, 31,
                 theme.accent, theme.accent2);
    canvas.setTextDatum(middle_left);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(strlen(mission->text) <= 23 ? 1 : 1);
    canvas.drawString(mission->text, x + 51, y + 14);

    const uint8_t target = mission->target;
    const uint8_t progress = profile.daily.dailyMissionProgress[slot];
    const int dotStart = x + 55;
    const int spacing = target <= 5 ? 18 : 12;
    for (uint8_t dot = 0; dot < target && dot < 10; ++dot) {
      const uint16_t color = dot < progress ? theme.accent : 0xBDF7;
      canvas.fillCircle(dotStart + dot * spacing, y + 34, 4, color);
      canvas.drawCircle(dotStart + dot * spacing, y + 34, 5, theme.accent2);
    }

    char progressText[12];
    snprintf(progressText, sizeof(progressText), "%u/%u", progress, target);
    canvas.setTextDatum(middle_right);
    canvas.setTextColor(theme.accent);
    canvas.setTextSize(1);
    canvas.drawString(progressText, x + width - 12, y + 35);

    if (done) {
      canvas.fillCircle(x + width - 25, y + 17, 16, TFT_DARKGREEN);
      UI::drawTick(x + width - 25, y + 17, 25);
    }
  }

  void openMission() {
    const DailyMissionDef* mission = missionById(App::profile().daily.dailyMissionIds[selected_]);
    if (mission == nullptr) return;
    App::ctx.fromDailyMissions = true;
    Audio::play(Sfx::Select);

    switch (mission->kind) {
      case MissionKind::CompleteTask:
        App::goTo(ScreenId::Tasks);
        return;
      case MissionKind::GetDressed:
        App::goTo(ScreenId::GetDressedTimer);
        return;
      case MissionKind::WalkFrankie:
        App::goTo(ScreenId::FrankieWalkIntro);
        return;
      case MissionKind::DadMission:
        App::goTo(ScreenId::DadMission);
        return;
      case MissionKind::FeedBuddy:
        App::goTo(ScreenId::Buddy);
        return;
      case MissionKind::PlayGame:
        App::ctx.gameId = mission->param;
        App::goTo(ScreenId::GamesMenu);
        return;
      case MissionKind::MathsCorrect:
        App::ctx.gameId = static_cast<uint8_t>(GameId::HugoMaths);
        App::goTo(ScreenId::GamesMenu);
        return;
      case MissionKind::PenaltyGoals:
        App::ctx.gameId = static_cast<uint8_t>(GameId::PenaltyKick);
        App::goTo(ScreenId::GamesMenu);
        return;
      case MissionKind::PopBubbles:
        App::ctx.gameId = static_cast<uint8_t>(GameId::BubblePop);
        App::goTo(ScreenId::GamesMenu);
        return;
      case MissionKind::FindColours:
        App::ctx.gameId = static_cast<uint8_t>(GameId::ColourMatch);
        App::goTo(ScreenId::GamesMenu);
        return;
      case MissionKind::FindDinosaurs:
        App::ctx.gameId = static_cast<uint8_t>(GameId::FindDinosaur);
        App::goTo(ScreenId::GamesMenu);
        return;
      case MissionKind::DoDance:
        App::ctx.gameId = static_cast<uint8_t>(GameId::PrincessDance);
        App::goTo(ScreenId::GamesMenu);
        return;
    }
  }

  void claimReward(PlayerProfile& profile) {
    profile.daily.dailyMissionRewardClaimed = true;
    rewardKind_ = RewardKind::Coins;
    rewardCollectible_ = 0xFF;

    if (random(100) < 25) {
      if (random(2) == 0 && awardRandomCollectible(profile)) {
        rewardKind_ = RewardKind::Collectible;
      } else {
        rewardKind_ = RewardKind::JellyBeans;
        profile.buddy.jellyBeans = profile.buddy.jellyBeans > 250 ? 255 : profile.buddy.jellyBeans + 5;
        Audio::play(Sfx::JellyBean);
      }
    } else {
      App::awardCoins(COINS_DAILY_MISSIONS_ALL);
    }

    celebrationMs_ = 3200;
    Save::saveNow();
    if (rewardKind_ == RewardKind::Collectible) Audio::play(Sfx::Celebrate);
  }

  bool awardRandomCollectible(PlayerProfile& profile) {
    uint8_t unowned[MAX_COLLECTIBLES];
    uint8_t count = 0;
    for (uint8_t group = 0; group < static_cast<uint8_t>(CollectGroup::COUNT); ++group) {
      for (uint8_t index = 0; index < COLLECT_COUNT[group]; ++index) {
        const uint8_t id = COLLECT_BASE[group] + index;
        if (!profile.hasCollectible(id)) unowned[count++] = id;
      }
    }
    if (count == 0) return false;
    rewardCollectible_ = unowned[random(count)];
    profile.setCollectible(rewardCollectible_);
    App::ctx.lastCollectible = rewardCollectible_;
    return true;
  }

  void drawCelebration() const {
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();
    canvas.fillRoundRect(31, 56, 258, 126, 18, theme.panel);
    canvas.drawRoundRect(31, 56, 258, 126, 18, TFT_YELLOW);
    UI::drawIcon(IconId::Gift, 160, 88, 54, theme.accent, theme.accent2);
    UI::drawBigCentred("ALL THREE DONE!", 122, 2, theme.text);

    if (rewardKind_ == RewardKind::Coins) {
      UI::drawIcon(IconId::Coin, 101, 153, 25, TFT_YELLOW, TFT_ORANGE);
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(TFT_YELLOW);
      canvas.setTextSize(2);
      canvas.drawString("+30 COINS", 119, 154);
    } else if (rewardKind_ == RewardKind::JellyBeans) {
      UI::drawIcon(IconId::JellyBean, 103, 153, 25, TFT_MAGENTA, TFT_CYAN);
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(theme.text);
      canvas.setTextSize(2);
      canvas.drawString("+5 JELLY BEANS", 121, 154);
    } else if (rewardKind_ == RewardKind::Collectible) {
      UI::drawIcon(IconId::Sparkle, 79, 153, 22, TFT_YELLOW, TFT_ORANGE);
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(theme.text);
      canvas.setTextSize(1);
      canvas.drawString(collectibleLabel(rewardCollectible_), 96, 153);
    }
  }
};

DailyMissionsScreen instance;
ScreenRegistrar registrar(ScreenId::DailyMissions, instance);

}
