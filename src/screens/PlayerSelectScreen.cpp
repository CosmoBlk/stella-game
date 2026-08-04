#include "Screen.h"
#include "../App.h"
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
    UI::drawBigCentred("WHO'S PLAYING?", 30, 2, UI::theme().text);
    drawCard(18, PlayerId::Hugo, "HUGO", IconId::Ball, 0x249F, 0xFD20);
    drawCard(166, PlayerId::Stella, "STELLA", IconId::Crown, 0xF81F, 0xFEC0);
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

  void drawCard(int x, PlayerId id, const char* label, IconId icon, uint16_t fill, uint16_t accent) {
    M5Canvas& canvas = Gfx::c();
    const bool active = selected == id;
    const int y = active ? 57 : 63;
    canvas.fillRoundRect(x, y, 136, 122, 14, fill);
    canvas.drawRoundRect(x, y, 136, 122, 14, active ? accent : 0xFFFF);
    if (active) canvas.drawRoundRect(x + 3, y + 3, 130, 116, 12, accent);
    UI::drawIcon(icon, x + 68, y + 45, 54, 0xFFFF, accent);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0xFFFF);
    canvas.setTextSize(2);
    canvas.drawString(label, x + 68, y + 92);
    canvas.setTextDatum(top_left);
  }
};

PlayerSelectScreen instance;
ScreenRegistrar registrar(ScreenId::PlayerSelect, instance);
}
