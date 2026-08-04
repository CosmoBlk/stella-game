#include "App.h"
#include "Config.h"
#include "Content.h"
#include "Managers.h"
#include "UI.h"

#include <string.h>

namespace {

bool recentlyUsed(const PlayerProfile& profile, uint8_t id) {
  for (uint8_t recent : profile.daily.recentDadMissions) {
    if (recent == id) return true;
  }
  return false;
}

void rememberMission(PlayerProfile& profile, uint8_t id) {
  profile.daily.recentDadMissions[profile.daily.recentDadHead] = id;
  profile.daily.recentDadHead = (profile.daily.recentDadHead + 1) % MAX_DAD_MISSION_RECENT;
  Save::requestSave();
}

class DadMissionScreen final : public Screen {
public:
  void enter() override { rollMission(); }
  void update(uint32_t) override {}

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("DAD MISSION");
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();
    const DadMissionDef& mission = DAD_MISSIONS[selected_];

    UI::drawPanel(18, 35, 284, 158, theme.panel);
    canvas.drawRoundRect(18, 35, 284, 158, 18, theme.accent);
    UI::drawIcon(static_cast<IconId>(mission.iconKey), 160, 89, 86,
                 theme.accent, theme.accent2);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(strlen(mission.text) <= 22 ? 2 : 1);
    canvas.drawString(mission.text, 160, 156);

    UI::drawButtonBar("BACK", "DO IT", "ANOTHER",
                      IconId::Back, IconId::Tick, IconId::ArrowR);
  }

  void onButtonA() override {
    Audio::play(Sfx::Back);
    App::goTo(ScreenId::Home);
  }

  void onButtonB() override {
    App::ctx.dadMissionId = DAD_MISSIONS[selected_].id;
    Audio::play(Sfx::Select);
    App::goTo(ScreenId::DadMissionDetail);
  }

  void onButtonC() override {
    rememberMission(App::profile(), DAD_MISSIONS[selected_].id);
    rollMission();
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "DadMission"; }

private:
  int selected_ = 0;

  void rollMission() {
    PlayerProfile& profile = App::profile();
    int available = 0;
    for (int i = 0; i < DAD_MISSION_COUNT; ++i) {
      if (!recentlyUsed(profile, DAD_MISSIONS[i].id)) ++available;
    }
    if (available == 0) {
      selected_ = random(DAD_MISSION_COUNT);
      return;
    }
    int pick = random(available);
    for (int i = 0; i < DAD_MISSION_COUNT; ++i) {
      if (recentlyUsed(profile, DAD_MISSIONS[i].id)) continue;
      if (pick-- == 0) {
        selected_ = i;
        return;
      }
    }
  }
};

DadMissionScreen instance;
ScreenRegistrar registrar(ScreenId::DadMission, instance);

}
