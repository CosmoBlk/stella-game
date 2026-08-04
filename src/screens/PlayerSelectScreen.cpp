#include "Screen.h"
#include "../App.h"
#include "../Content.h"
#include "../Sprites.h"
#include "../UI.h"
#include "../Managers.h"

namespace {
class PlayerSelectScreen final : public Screen {
public:
  void enter() override { selected = App::player(); }
  void update(uint32_t) override {}

  void draw() override {
    M5Canvas& canvas = Gfx::c();
    UI::drawBackground();
    UI::drawBigCentred("HUGO + STELLA", 18, 2, UI::theme().text);
    UI::drawBigCentred("WHO'S PLAYING?", 42, 1, UI::theme().accent);
    drawCard(8, PlayerId::Hugo, "HUGO", 0x249F, 0xFD20);
    drawCard(164, PlayerId::Stella, "STELLA", 0xF81F, 0xFEC0);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(1);
    canvas.drawString("A", 66, 194);
    canvas.drawString("B = LET'S GO!", 160, 194);
    canvas.drawString("C", 254, 194);
    canvas.setTextDatum(top_left);
    UI::drawButtonBar("HUGO", "SELECT", "STELLA", IconId::ArrowL, IconId::Tick, IconId::ArrowR);
  }

  void onButtonA() override {
    selected = PlayerId::Hugo;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    App::selectPlayer(selected);
    Audio::play(Sfx::Select);
    App::goTo(ScreenId::Home);
  }

  void onButtonC() override {
    selected = PlayerId::Stella;
    Audio::play(Sfx::Select);
  }

  bool allowGlobalBack() const override { return false; }
  const char* name() const override { return "PLAYER SELECT"; }

private:
  PlayerId selected = PlayerId::Hugo;

  void drawCard(int x, PlayerId id, const char* label, uint16_t fill, uint16_t accent) {
    M5Canvas& canvas = Gfx::c();
    const bool active = selected == id;
    const int y = active ? 55 : 61;
    canvas.fillRoundRect(x, y, 148, 134, 16, fill);
    canvas.drawRoundRect(x, y, 148, 134, 16, active ? accent : 0xFFFF);
    if (active) canvas.drawRoundRect(x + 3, y + 3, 142, 128, 14, accent);
    if (id == PlayerId::Hugo) {
      // TODO(art): swap to artKid()
      Sprites::drawCharacter(ITEM_HUGO_DEFAULT_SOCCER, x + 74, y + 65, 82,
                             Anim::Happy, App::nowMs());
    } else {
      // TODO(art): swap to artKid()
      Sprites::drawCharacter(ITEM_STELLA_DEFAULT_PRINCESS, x + 74, y + 65, 82,
                             Anim::Happy, App::nowMs());
    }
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0xFFFF);
    canvas.setTextSize(2);
    canvas.drawString(label, x + 74, y + 111);
    canvas.setTextDatum(top_left);
  }
};

PlayerSelectScreen instance;
ScreenRegistrar registrar(ScreenId::PlayerSelect, instance);
}
