#include "App.h"
#include "Config.h"
#include "Content.h"
#include "Managers.h"
#include "UI.h"

#include <stdio.h>
#include <string.h>

namespace {

const DadMissionDef& activeMission() {
  for (int i = 0; i < DAD_MISSION_COUNT; ++i) {
    if (DAD_MISSIONS[i].id == App::ctx.dadMissionId) return DAD_MISSIONS[i];
  }
  return DAD_MISSIONS[0];
}

void rememberMission(PlayerProfile& profile, uint8_t id) {
  profile.daily.recentDadMissions[profile.daily.recentDadHead] = id;
  profile.daily.recentDadHead = (profile.daily.recentDadHead + 1) % MAX_DAD_MISSION_RECENT;
}

class DadMissionDetailScreen final : public Screen {
public:
  void enter() override {
    elapsedMs_ = 0;
    lastTickStep_ = 0;
    completed_ = false;
    Audio::play(Sfx::MusicCue);
  }

  void update(uint32_t deltaMs) override {
    elapsedMs_ += deltaMs;
    if (completed_) return;
    const uint32_t held = Input::heldMsB();
    if (held == 0) {
      lastTickStep_ = 0;
      return;
    }

    const uint8_t tickStep = held >= TASK_HOLD_MS ? 4 : static_cast<uint8_t>(held / 500);
    if (tickStep > lastTickStep_) {
      lastTickStep_ = tickStep;
      Audio::play(Sfx::Tick);
    }
    if (held >= TASK_HOLD_MS) complete();
  }

  void draw() override {
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();
    const DadMissionDef& mission = activeMission();

    UI::drawBackground();
    canvas.fillCircle(52, 75, 54, theme.accent2);
    canvas.fillCircle(283, 123, 69, theme.accent);
    canvas.fillRoundRect(16, 34, 288, 158, 20, theme.panel);
    canvas.drawRoundRect(16, 34, 288, 158, 20, theme.accent);
    UI::drawIcon(static_cast<IconId>(mission.iconKey), 160, 91, 100,
                 theme.accent, theme.accent2);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(strlen(mission.text) <= 22 ? 2 : 1);
    canvas.drawString(mission.text, 160, 153);
    canvas.setTextSize(1);
    canvas.setTextColor(theme.accent);
    canvas.drawString("HOLD B WHEN DONE", 160, 181);

    const float progress = static_cast<float>(Input::heldMsB()) /
                           static_cast<float>(TASK_HOLD_MS);
    UI::drawHoldProgress(progress > 1.0f ? 1.0f : progress);
    UI::drawButtonBar("BACK", "HOLD", "", IconId::Back, IconId::Tick, IconId::None);
  }

  void onButtonA() override {
    Audio::play(Sfx::Back);
    App::goTo(ScreenId::DadMission);
  }

  bool allowGlobalBack() const override { return Input::heldMsB() == 0; }
  const char* name() const override { return "DadMissionDetail"; }

private:
  uint32_t elapsedMs_ = 0;
  uint8_t lastTickStep_ = 0;
  bool completed_ = false;

  void complete() {
    completed_ = true;
    PlayerProfile& profile = App::profile();
    const DadMissionDef& mission = activeMission();
    App::ctx.lastReward = 0;
    App::ctx.lastCollectible = 0xFF;
    bool wonBadge = false;

    if (profile.daily.rewardedDadMissions < MAX_DAD_MISSIONS_PER_DAY) {
      const uint16_t reward = static_cast<uint16_t>(random(mission.coinsMin, mission.coinsMax + 1));
      ++profile.daily.rewardedDadMissions;
      App::awardCoins(reward);

      if (random(100) < 15) {
        uint8_t unowned[COLLECT_COUNT[static_cast<uint8_t>(CollectGroup::DadBadge)]];
        uint8_t count = 0;
        for (uint8_t badge = 0; badge < COLLECT_COUNT[static_cast<uint8_t>(CollectGroup::DadBadge)]; ++badge) {
          if (!profile.hasCollectible(CollectGroup::DadBadge, badge)) unowned[count++] = badge;
        }
        if (count > 0) {
          const uint8_t badge = unowned[random(count)];
          profile.setCollectible(CollectGroup::DadBadge, badge);
          App::ctx.lastCollectible = COLLECT_BASE[static_cast<uint8_t>(CollectGroup::DadBadge)] + badge;
          wonBadge = true;
        }
      }
    }

    App::raiseEvent(GameEvent::DadMissionDone);
    rememberMission(profile, mission.id);
    Save::saveNow();
    Audio::play(wonBadge ? Sfx::Celebrate : Sfx::MissionDone);
    App::goTo(ScreenId::DadMissionResult);
  }
};

DadMissionDetailScreen instance;
ScreenRegistrar registrar(ScreenId::DadMissionDetail, instance);

}
