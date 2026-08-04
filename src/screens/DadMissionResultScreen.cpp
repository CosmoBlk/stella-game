#include "App.h"
#include "Content.h"
#include "Managers.h"
#include "UI.h"

#include <stdio.h>

namespace {

class DadMissionResultScreen final : public Screen {
public:
  void enter() override {
    elapsedMs_ = 0;
    leaving_ = false;
  }

  void update(uint32_t deltaMs) override {
    elapsedMs_ += deltaMs;
    if (elapsedMs_ >= 4300) finish();
  }

  void draw() override {
    UI::drawBackground();
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();

    for (int i = 0; i < 10; ++i) {
      const int x = 20 + i * 31;
      const int y = 37 + static_cast<int>((elapsedMs_ / 12 + i * 29) % 142);
      UI::drawIcon(IconId::Star, x, y, 10 + (i & 1) * 4,
                   i & 1 ? TFT_YELLOW : theme.accent2, theme.accent);
    }

    canvas.fillRoundRect(38, 42, 244, 146, 20, theme.panel);
    canvas.drawRoundRect(38, 42, 244, 146, 20, theme.accent);
    UI::drawTick(160, 82, 67);
    UI::drawBigCentred("MISSION DONE!", 119, 2, theme.text);

    if (App::ctx.lastReward > 0) {
      char reward[24];
      snprintf(reward, sizeof(reward), "+%u COINS", App::ctx.lastReward);
      UI::drawIcon(IconId::Coin, 104, 151, 25, TFT_YELLOW, TFT_ORANGE);
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(TFT_YELLOW);
      canvas.setTextSize(2);
      canvas.drawString(reward, 121, 152);
    } else {
      UI::drawBigCentred("JUST FOR FUN!", 151, 2, theme.accent);
    }

    const uint8_t badgeBase = COLLECT_BASE[static_cast<uint8_t>(CollectGroup::DadBadge)];
    if (App::ctx.lastCollectible >= badgeBase &&
        App::ctx.lastCollectible < badgeBase + COLLECT_COUNT[static_cast<uint8_t>(CollectGroup::DadBadge)]) {
      const uint8_t badge = App::ctx.lastCollectible - badgeBase;
      UI::drawIcon(IconId::Sparkle, 87, 177, 18, TFT_YELLOW, TFT_ORANGE);
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(theme.text);
      canvas.setTextSize(1);
      canvas.drawString(collectibleName(CollectGroup::DadBadge, badge), 101, 177);
    }

    UI::drawButtonBar("", "HOME", "", IconId::None, IconId::Home, IconId::None);
  }

  void onButtonB() override { finish(); }
  const char* name() const override { return "DadMissionResult"; }

private:
  uint32_t elapsedMs_ = 0;
  bool leaving_ = false;

  void finish() {
    if (leaving_) return;
    leaving_ = true;
    Audio::play(Sfx::Select);
    App::goTo(ScreenId::Home);
  }
};

DadMissionResultScreen instance;
ScreenRegistrar registrar(ScreenId::DadMissionResult, instance);

}
