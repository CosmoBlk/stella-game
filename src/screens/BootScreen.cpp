#include "Screen.h"
#include "../App.h"
#include "../UI.h"
#include "../Managers.h"
#include <math.h>

namespace {
class BootScreen final : public Screen {
public:
  void enter() override {
    elapsedMs = 0;
    finished = false;
  }

  void update(uint32_t deltaMs) override {
    elapsedMs += deltaMs;
    if (!finished && elapsedMs >= 2000) {
      finished = true;
      App::goTo(ScreenId::PlayerSelect);
    }
  }

  void draw() override {
    UI::drawBackground();
    const int bounce = static_cast<int>(sinf(elapsedMs * 0.008f) * 10.0f);
    UI::drawIcon(IconId::Star, 160, 66 + bounce, 45, 0xFEC0, 0xFD20);
    UI::drawBigCentred("POCKET BUDDY", 126, 3, UI::theme().text);
    UI::drawBigCentred("HELLO, FRIEND!", 164, 1, UI::theme().accent);
  }

  bool allowGlobalBack() const override { return false; }
  const char* name() const override { return "BOOT"; }

private:
  uint32_t elapsedMs = 0;
  bool finished = false;
};

BootScreen instance;
ScreenRegistrar registrar(ScreenId::Boot, instance);
}
