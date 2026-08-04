#include "App.h"
#include "Config.h"
#include "Content.h"
#include "Managers.h"
#include "Sprites.h"
#include "UI.h"

#include <stdio.h>

namespace FrankieWalkSession {
uint8_t discoveryCount();
uint8_t discoveryAt(uint8_t index);
}

namespace {

class FrankieWalkResultScreen final : public Screen {
public:
  void enter() override {
    elapsedMs_ = 0;
    leaving_ = false;
    App::awardCoins(COINS_WALK_FRANKIE);
    App::raiseEvent(GameEvent::FrankieWalked);
    ++App::profile().totalFrankieWalks;
    Save::saveNow();
    Audio::play(Sfx::Celebrate);
  }

  void update(uint32_t deltaMs) override {
    elapsedMs_ += deltaMs;
    if (elapsedMs_ >= 5200) finish();
  }

  void draw() override {
    UI::drawBackground();
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();

    UI::drawBigCentred("FRANKIE HAD FUN!", 29, 2, theme.text);
    Sprites::drawFrankie(76, 102, 112, elapsedMs_, true);

    canvas.fillRoundRect(137, 48, 165, 61, 14, theme.panel);
    UI::drawIcon(IconId::Coin, 165, 78, 30, TFT_YELLOW, TFT_ORANGE);
    canvas.setTextDatum(middle_left);
    canvas.setTextColor(TFT_YELLOW);
    canvas.setTextSize(2);
    canvas.drawString("+20 COINS", 187, 79);

    canvas.fillRoundRect(16, 118, 288, 79, 12, theme.panel);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(theme.accent);
    canvas.setTextSize(1);
    canvas.drawString("FRANKIE FOUND", 160, 130);

    const uint8_t count = FrankieWalkSession::discoveryCount();
    if (count == 0) {
      canvas.setTextColor(theme.text);
      canvas.drawString("A LOVELY WALK!", 160, 159);
    } else {
      for (uint8_t i = 0; i < count && i < 12; ++i) {
        const uint8_t find = FrankieWalkSession::discoveryAt(i);
        const int column = i / 6;
        const int row = i % 6;
        const int x = 28 + column * 143;
        const int y = 143 + row * 9;
        UI::drawIcon(IconId::Paw, x, y, 8, theme.accent2, theme.accent);
        canvas.setTextDatum(middle_left);
        canvas.setTextColor(theme.text);
        canvas.setTextSize(1);
        canvas.drawString(FRANKIE_FINDS[find], x + 8, y);
      }
    }

    UI::drawButtonBar("", "HOME", "", IconId::None, IconId::Home, IconId::None);
  }

  void onButtonB() override { finish(); }
  const char* name() const override { return "FrankieWalkResult"; }

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

FrankieWalkResultScreen instance;
ScreenRegistrar registrar(ScreenId::FrankieWalkResult, instance);

}
