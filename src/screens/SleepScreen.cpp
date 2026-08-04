#include "../App.h"
#include "../Config.h"
#include "../Managers.h"
#include "../Sprites.h"
#include "../UI.h"

namespace {

constexpr uint32_t SLEEP_DURATION_MS = 6000;
constexpr uint32_t RESTED_HOLD_MS = 1700;

class SleepScreen final : public Screen {
public:
  void enter() override {
    elapsedMs_ = 0;
    restedMs_ = 0;
    startEnergy_ = App::profile().buddy.energy;
    completed_ = false;
    eventRaised_ = false;
    Audio::play(Sfx::SleepMusic);
  }

  void exit() override {
    raiseSleepEvent();
    Save::requestSave();
  }

  void update(uint32_t deltaMs) override {
    if (!completed_) {
      elapsedMs_ += deltaMs;
      if (elapsedMs_ > SLEEP_DURATION_MS) elapsedMs_ = SLEEP_DURATION_MS;
      const uint32_t gain = static_cast<uint32_t>(NEED_MAX - startEnergy_) * elapsedMs_;
      App::profile().buddy.energy = static_cast<uint8_t>(startEnergy_ + gain / SLEEP_DURATION_MS);
      if (elapsedMs_ >= SLEEP_DURATION_MS) {
        App::profile().buddy.energy = NEED_MAX;
        completed_ = true;
        raiseSleepEvent();
        Audio::play(Sfx::Sparkle);
      }
    } else {
      restedMs_ += deltaMs;
      if (restedMs_ >= RESTED_HOLD_MS) App::goBack();
    }
  }

  void draw() override {
    const InventoryState& inventory = App::profile().inventory;
    Sprites::drawRoom(inventory.equippedRoom);
    drawDarkOverlay();
    drawNightSky();
    Sprites::drawCharacter(inventory.equippedCharacter, 160, 125, 105,
                           Anim::Sleeping, elapsedMs_, inventory.equippedAccessory,
                           inventory.equippedClothing);
    drawZzz();
    drawMoonProgress();
    if (completed_) drawRestedMessage();
  }

  void onButtonA() override { wakeEarly(); }
  void onButtonB() override { wakeEarly(); }
  void onButtonC() override { wakeEarly(); }

  const char* name() const override { return "SLEEP"; }

private:
  void wakeEarly() {
    if (completed_) {
      App::goBack();
      return;
    }
    raiseSleepEvent();
    Save::requestSave();
    App::goBack();
  }

  void raiseSleepEvent() {
    if (eventRaised_) return;
    eventRaised_ = true;
    App::raiseEvent(GameEvent::BuddySlept);
    Save::requestSave();
  }

  void drawDarkOverlay() const {
    auto& canvas = Gfx::c();
    for (int y = 0; y < SCREEN_H; y += 3) canvas.fillRect(0, y, SCREEN_W, 2, 0x0008);
  }

  void drawNightSky() const {
    static constexpr int STAR_X[] = {28, 56, 88, 225, 258, 293, 193};
    static constexpr int STAR_Y[] = {38, 72, 47, 42, 76, 33, 65};
    auto& canvas = Gfx::c();
    canvas.fillCircle(267, 48, 25, 0xFFE0);
    canvas.fillCircle(278, 39, 24, 0x0008);
    for (int i = 0; i < 7; ++i) {
      canvas.fillCircle(STAR_X[i], STAR_Y[i], 2, 0xFFFF);
      canvas.drawLine(STAR_X[i] - 4, STAR_Y[i], STAR_X[i] + 4, STAR_Y[i], 0xBDF7);
      canvas.drawLine(STAR_X[i], STAR_Y[i] - 4, STAR_X[i], STAR_Y[i] + 4, 0xBDF7);
    }
  }

  void drawZzz() const {
    auto& canvas = Gfx::c();
    const int bob = static_cast<int>((elapsedMs_ / 300) & 1);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0xBDF7);
    canvas.setTextSize(1);
    canvas.drawString("Z", 210, 109 - bob);
    canvas.setTextSize(2);
    canvas.drawString("Z", 226, 89 - bob);
    canvas.setTextSize(3);
    canvas.drawString("Z", 246, 65 - bob);
    canvas.setTextDatum(top_left);
  }

  void drawMoonProgress() const {
    auto& canvas = Gfx::c();
    const int width = static_cast<int>(App::profile().buddy.energy) * 220 / NEED_MAX;
    canvas.fillRoundRect(42, 183, 236, 17, 8, 0x2104);
    if (width > 0) canvas.fillRoundRect(50, 188, width, 7, 4, 0xFFE0);
    UI::drawIcon(IconId::Moon, 31, 191, 19, 0xFFE0);
  }

  void drawRestedMessage() const {
    auto& canvas = Gfx::c();
    canvas.fillRoundRect(70, 32, 180, 34, 10, 0xFFFF);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0x39E7);
    canvas.setTextSize(2);
    canvas.drawString("ALL RESTED!", 160, 49);
    canvas.setTextDatum(top_left);
  }

  uint32_t elapsedMs_ = 0;
  uint32_t restedMs_ = 0;
  uint8_t startEnergy_ = 0;
  bool completed_ = false;
  bool eventRaised_ = false;
};

SleepScreen instance;
ScreenRegistrar registrar(ScreenId::Sleep, instance);

}
