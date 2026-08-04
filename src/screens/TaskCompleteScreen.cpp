#include "App.h"
#include "Managers.h"
#include "Sprites.h"
#include "UI.h"

#include <stdio.h>

namespace {

void drawConfetti(uint32_t frameMs) {
  static const int baseX[] = {18, 49, 82, 119, 158, 197, 235, 274, 305, 143, 218, 94};
  static const uint16_t colours[] = {
    TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_GREEN, TFT_ORANGE, TFT_WHITE
  };
  for (int i = 0; i < 12; ++i) {
    const int y = 28 + static_cast<int>((frameMs / 9 + i * 37) % 160);
    const int x = baseX[i] + static_cast<int>((frameMs / 140 + i) % 7) - 3;
    UI::drawIcon(IconId::Star, x, y, 9 + (i % 3) * 3,
                 colours[i % 6], colours[(i + 2) % 6]);
  }
}

class TaskCompleteScreen final : public Screen {
public:
  void enter() override {
    elapsedMs_ = 0;
    Audio::play(Sfx::TaskDone);
  }

  void update(uint32_t deltaMs) override {
    elapsedMs_ += deltaMs;
    if (elapsedMs_ >= 2500) App::goTo(ScreenId::Tasks);
  }

  void draw() override {
    const PlayerProfile& profile = App::profile();
    const Theme& theme = UI::theme();
    M5Canvas& canvas = Gfx::c();

    UI::drawBackground();
    drawConfetti(elapsedMs_);

    Sprites::drawCharacter(profile.inventory.equippedCharacter, 82, 137, 126,
                           Anim::Celebrating, elapsedMs_,
                           profile.inventory.equippedAccessory,
                           profile.inventory.equippedClothing);

    canvas.fillCircle(222, 83, 48, TFT_DARKGREEN);
    UI::drawTick(222, 83, 78);

    UI::drawIcon(IconId::Star, 166, 146, 24, TFT_YELLOW, TFT_ORANGE);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(2);
    canvas.drawString("GREAT JOB!", 235, 146);

    char rewardText[24];
    snprintf(rewardText, sizeof(rewardText), "+%u COINS", App::ctx.lastReward);
    UI::drawIcon(IconId::Coin, 166, 181, 27, TFT_YELLOW, TFT_ORANGE);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(TFT_YELLOW);
    canvas.setTextSize(2);
    canvas.drawString(rewardText, 235, 182);

    UI::drawButtonBar("", "YAY!", "", IconId::None, IconId::Star, IconId::None);
  }

  void onButtonA() override { finish(); }
  void onButtonB() override { finish(); }
  void onButtonC() override { finish(); }
  const char* name() const override { return "TaskComplete"; }

private:
  uint32_t elapsedMs_ = 0;

  void finish() {
    Audio::play(Sfx::Select);
    App::goTo(ScreenId::Tasks);
  }
};

TaskCompleteScreen instance;
ScreenRegistrar registrar(ScreenId::TaskComplete, instance);

}
