#include "App.h"
#include "Managers.h"
#include "Sprites.h"
#include "UI.h"

namespace {

class FrankieWalkIntroScreen final : public Screen {
public:
  void enter() override { elapsedMs_ = 0; }

  void update(uint32_t deltaMs) override { elapsedMs_ += deltaMs; }

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("WALK FRANKIE?");
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();

    canvas.fillRoundRect(18, 35, 284, 158, 18, theme.panel);
    canvas.drawRoundRect(18, 35, 284, 158, 18, theme.accent);

    for (int i = 0; i < 6; ++i) {
      const int x = 42 + i * 47;
      const int y = 169 - (i & 1) * 12;
      UI::drawIcon(IconId::Paw, x, y, 20, theme.accent2, theme.accent);
    }

    const int wag = static_cast<int>((elapsedMs_ / 140) & 1);
    Sprites::drawFrankie(160, 112 + wag, 128, elapsedMs_, true);
    UI::drawIcon(IconId::Heart, 235, 63, 25 + wag * 3, TFT_MAGENTA, TFT_RED);
    UI::drawButtonBar("BACK", "START", "", IconId::Back, IconId::Paw, IconId::None);
  }

  void onButtonA() override {
    Audio::play(Sfx::Back);
    App::goTo(ScreenId::Home);
  }

  void onButtonB() override {
    Audio::play(Sfx::Select);
    App::goTo(ScreenId::FrankieWalk);
  }

  const char* name() const override { return "FrankieWalkIntro"; }

private:
  uint32_t elapsedMs_ = 0;
};

FrankieWalkIntroScreen instance;
ScreenRegistrar registrar(ScreenId::FrankieWalkIntro, instance);

}
