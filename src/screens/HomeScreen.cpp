#include "Screen.h"
#include "../App.h"
#include "../Managers.h"
#include "../Sprites.h"
#include "../UI.h"

namespace {
class HomeScreen final : public Screen {
public:
  void enter() override {}
  void update(uint32_t) override {}

  void draw() override {
    const PlayerProfile& profile = App::profile();
    Sprites::drawRoom(profile.inventory.equippedRoom);
    Sprites::drawCharacter(profile.inventory.equippedCharacter, 176, 126, 112,
                           Buddy::moodAnim(), App::nowMs(), profile.inventory.equippedAccessory,
                           profile.inventory.equippedClothing);
    UI::drawHeader(App::player() == PlayerId::Hugo ? "HUGO" : "STELLA");
    UI::drawPanel(7, 37, 105, 61, UI::theme().panel);
    UI::drawNeedBar(10, 42, IconId::Apple, profile.buddy.hunger, 0xF9A6);
    UI::drawNeedBar(10, 60, IconId::Smiley, profile.buddy.happiness, UI::theme().accent);
    UI::drawNeedBar(10, 78, IconId::Moon, profile.buddy.energy, 0x7DFF);
    UI::drawButtonBar("TASKS", "BUDDY", "GAMES", IconId::Missions, IconId::Heart, IconId::Games);
  }

  void onButtonA() override { open(ScreenId::Tasks); }
  void onButtonB() override { open(ScreenId::Buddy); }
  void onButtonC() override { open(ScreenId::GamesMenu); }
  void onButtonBLong() override { open(ScreenId::MainMenu); }
  bool allowGlobalBack() const override { return false; }
  const char* name() const override { return "HOME"; }

private:
  void open(ScreenId target) {
    if (!App::screen(target)) {
      Serial.println("[warn] screen missing");
      return;
    }
    Audio::play(Sfx::Select);
    App::goTo(target);
  }
};

HomeScreen instance;
ScreenRegistrar registrar(ScreenId::Home, instance);
}
