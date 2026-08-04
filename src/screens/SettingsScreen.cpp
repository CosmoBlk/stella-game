#include "Screen.h"
#include "../App.h"
#include "../Managers.h"
#include "../UI.h"

namespace {
constexpr uint8_t brightnessLevels[3] = {64, 144, 255};

class SettingsScreen final : public Screen {
public:
  void enter() override {
    selection = 0;
    brightnessStep = nearestBrightness(App::settings().brightness);
  }

  void update(uint32_t) override {}

  void draw() override {
    M5Canvas& canvas = Gfx::c();
    UI::drawBackground();
    UI::drawHeader("SETTINGS");
    drawRow(45, 0, IconId::Note, "VOLUME", volumeLabel());
    drawRow(112, 1, IconId::Sun, "BRIGHTNESS", brightnessLabel());
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(1);
    canvas.drawString("B CHANGES THE SETTING", 160, 184);
    canvas.setTextDatum(top_left);
    UI::drawButtonBar("UP", "CHANGE", "DOWN", IconId::ArrowL, IconId::Settings, IconId::ArrowR);
  }

  void onButtonA() override {
    selection = selection == 0 ? 1 : 0;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    if (selection == 0) {
      App::settings().volume = (App::settings().volume + 1) % 3;
      Audio::applyVolume(App::settings().volume);
      Audio::play(Sfx::Select);
    } else {
      brightnessStep = (brightnessStep + 1) % 3;
      App::settings().brightness = brightnessLevels[brightnessStep];
      M5.Display.setBrightness(App::settings().brightness);
      Audio::play(Sfx::Select);
    }
    Save::requestSave();
  }

  void onButtonC() override {
    selection = selection == 0 ? 1 : 0;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "SETTINGS"; }

private:
  uint8_t selection = 0;
  uint8_t brightnessStep = 1;

  uint8_t nearestBrightness(uint8_t value) const {
    if (value < 104) return 0;
    if (value < 200) return 1;
    return 2;
  }

  const char* volumeLabel() const {
    static const char* labels[3] = {"OFF", "QUIET", "NORMAL"};
    const uint8_t value = App::settings().volume > 2 ? 2 : App::settings().volume;
    return labels[value];
  }

  const char* brightnessLabel() const {
    static const char* labels[3] = {"LOW", "MEDIUM", "HIGH"};
    return labels[brightnessStep];
  }

  void drawRow(int y, uint8_t row, IconId icon, const char* label, const char* value) {
    M5Canvas& canvas = Gfx::c();
    UI::drawPanel(20, y, 280, 54, UI::theme().panel);
    if (selection == row) canvas.drawRoundRect(17, y - 3, 286, 60, 12, UI::theme().accent);
    UI::drawIcon(icon, 49, y + 27, 30, UI::theme().accent, UI::theme().accent2);
    canvas.setTextDatum(middle_left);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(1);
    canvas.drawString(label, 78, y + 18);
    canvas.setTextSize(2);
    canvas.drawString(value, 78, y + 37);
    canvas.setTextDatum(top_left);
  }
};

SettingsScreen instance;
ScreenRegistrar registrar(ScreenId::Settings, instance);
}
