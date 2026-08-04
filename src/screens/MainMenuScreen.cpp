#include "Screen.h"
#include "../App.h"
#include "../Managers.h"
#include "../UI.h"

namespace {
struct MenuDef {
  ScreenId screen;
  const char* label;
  IconId icon;
};

const MenuDef definitions[] = {
  {ScreenId::Buddy, "BUDDY", IconId::Heart},
  {ScreenId::Tasks, "TASKS", IconId::Missions},
  {ScreenId::GamesMenu, "GAMES", IconId::Games},
  {ScreenId::ShopCategory, "SHOP", IconId::Shop},
  {ScreenId::DadMission, "DAD MISSIONS", IconId::Star},
  {ScreenId::FrankieWalkIntro, "WALK FRANKIE", IconId::Paw},
  {ScreenId::DailyMissions, "DAILY MISSIONS", IconId::Tick},
  {ScreenId::CollectionCategory, "COLLECTION", IconId::Collection},
  {ScreenId::GetDressedTimer, "GET DRESSED", IconId::Timer},
  {ScreenId::Settings, "SETTINGS", IconId::Settings},
  {ScreenId::PlayerSelect, "SWITCH PLAYER", IconId::Swap}
};

class MainMenuScreen final : public Screen {
public:
  void enter() override {
    count = 0;
    for (uint8_t i = 0; i < sizeof(definitions) / sizeof(definitions[0]); ++i) {
      if (App::screen(definitions[i].screen)) visible[count++] = i;
    }
    if (selection >= count) selection = 0;
  }

  void update(uint32_t) override {}

  void draw() override {
    M5Canvas& canvas = Gfx::c();
    UI::drawBackground();
    UI::drawHeader("MAIN MENU");
    if (count == 0) {
      UI::drawBigCentred("MORE SOON!", 115, 2, UI::theme().text);
    } else {
      const uint8_t first = selection > 2 ? selection - 2 : 0;
      for (uint8_t row = 0; row < 5 && first + row < count; ++row) {
        const uint8_t itemIndex = first + row;
        const MenuDef& item = definitions[visible[itemIndex]];
        const int y = 34 + row * 33;
        if (itemIndex == selection) {
          canvas.fillRoundRect(8, y, 304, 29, 8, UI::theme().panel);
          canvas.drawRoundRect(8, y, 304, 29, 8, UI::theme().accent);
        }
        UI::drawIcon(item.icon, 28, y + 14, 20, UI::theme().accent, UI::theme().accent2);
        canvas.setTextDatum(middle_left);
        canvas.setTextColor(UI::theme().text);
        canvas.setTextSize(1);
        canvas.drawString(item.label, 48, y + 14);
        UI::drawIcon(IconId::ArrowR, 292, y + 14, 14, UI::theme().text);
      }
    }
    canvas.setTextDatum(top_left);
    UI::drawButtonBar("UP", "OPEN", "DOWN", IconId::ArrowL, IconId::Tick, IconId::ArrowR);
  }

  void onButtonA() override {
    if (count == 0) return;
    selection = selection == 0 ? count - 1 : selection - 1;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    if (count == 0) return;
    Audio::play(Sfx::Select);
    App::goTo(definitions[visible[selection]].screen);
  }

  void onButtonC() override {
    if (count == 0) return;
    selection = (selection + 1) % count;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "MAIN MENU"; }

private:
  uint8_t visible[sizeof(definitions) / sizeof(definitions[0])] = {};
  uint8_t count = 0;
  uint8_t selection = 0;
};

MainMenuScreen instance;
ScreenRegistrar registrar(ScreenId::MainMenu, instance);
}
