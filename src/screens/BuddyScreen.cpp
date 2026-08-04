#include "../App.h"
#include "../Config.h"
#include "../Managers.h"
#include "../Sprites.h"
#include "../UI.h"

namespace {

enum class BuddyAction : uint8_t {
  Feed = 0,
  Play,
  Sleep,
  Toy,
  JellyBeans,
  Character,
  Clothes,
  Accessory,
  Room,
  Count
};

struct ActionDef {
  const char* label;
  IconId icon;
  ScreenId target;
};

constexpr ActionDef ACTIONS[] = {
  {"FEED", IconId::Apple, ScreenId::Feed},
  {"PLAY", IconId::Smiley, ScreenId::Buddy},
  {"SLEEP", IconId::Moon, ScreenId::Sleep},
  {"TOY", IconId::Gift, ScreenId::Toy},
  {"JELLY BEANS", IconId::JellyBean, ScreenId::Buddy},
  {"CHARACTER", IconId::Smiley, ScreenId::CharacterSelect},
  {"CLOTHES", IconId::Shirt, ScreenId::ClothingSelect},
  {"ACCESSORY", IconId::Hat, ScreenId::AccessorySelect},
  {"ROOM", IconId::Home, ScreenId::RoomSelect},
};

constexpr int ACTION_COUNT = static_cast<int>(BuddyAction::Count);

class BuddyScreen final : public Screen {
public:
  void enter() override {
    selected_ = 0;
    anim_ = Buddy::moodAnim();
    animMs_ = 0;
    reactionMs_ = 0;
    hintMs_ = 0;
    rainbowMs_ = 0;
  }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    reactionMs_ = (reactionMs_ + deltaMs) % 4200;
    if (hintMs_ > deltaMs) hintMs_ -= deltaMs;
    else hintMs_ = 0;
    if (rainbowMs_ > deltaMs) rainbowMs_ -= deltaMs;
    else rainbowMs_ = 0;
    if (burstMs_ > deltaMs) burstMs_ -= deltaMs;
    else {
      burstMs_ = 0;
      anim_ = Buddy::moodAnim();
    }
  }

  void draw() override {
    const PlayerProfile& profile = App::profile();
    const InventoryState& inventory = profile.inventory;
    Sprites::drawRoom(inventory.equippedRoom);
    drawNeedBars(profile.buddy);

    Sprites::drawCharacter(inventory.equippedCharacter, 160, 113, 108,
                           anim_, animMs_, inventory.equippedAccessory,
                           inventory.equippedClothing);
    drawLowNeedReaction();
    if (rainbowMs_ > 0) drawRainbowSparkles();
    if (hintMs_ > 0) drawHint();
    drawCarousel();
  }

  void onButtonA() override {
    selected_ = (selected_ + ACTION_COUNT - 1) % ACTION_COUNT;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    const BuddyAction action = static_cast<BuddyAction>(selected_);
    if (action == BuddyAction::Play) {
      Buddy::play();
      Save::requestSave();
      anim_ = Anim::Celebrating;
      burstMs_ = 1300;
      animMs_ = 0;
      Audio::play(Sfx::Celebrate);
      return;
    }
    if (action == BuddyAction::JellyBeans) {
      if (App::profile().buddy.jellyBeans > 0) {
        Buddy::feedJellyBeans();
        Save::requestSave();
        anim_ = Anim::Celebrating;
        burstMs_ = 1500;
        rainbowMs_ = 1500;
        animMs_ = 0;
      } else {
        hintMs_ = 2400;
        Audio::play(Sfx::GoodTry);
      }
      return;
    }
    App::goTo(ACTIONS[selected_].target);
  }

  void onButtonC() override {
    selected_ = (selected_ + 1) % ACTION_COUNT;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "BUDDY"; }

private:
  void drawNeedBars(const BuddyState& buddy) const {
    auto& canvas = Gfx::c();
    canvas.fillRoundRect(4, 3, 312, 28, 8, 0x18C3);
    UI::drawNeedBar(8, 7, IconId::Apple, buddy.hunger, 0xFBE0);
    UI::drawNeedBar(110, 7, IconId::Smiley, buddy.happiness, 0xFFE0);
    UI::drawNeedBar(212, 7, IconId::Moon, buddy.energy, 0x7DFF);
  }

  void drawCarousel() const {
    auto& canvas = Gfx::c();
    const int previous = (selected_ + ACTION_COUNT - 1) % ACTION_COUNT;
    const int next = (selected_ + 1) % ACTION_COUNT;
    canvas.fillRoundRect(4, 174, 312, 62, 10, 0x18C3);
    canvas.drawRoundRect(105, 177, 110, 55, 9, UI::theme().accent);

    UI::drawIcon(ACTIONS[previous].icon, 53, 196, 20, 0xBDF7);
    UI::drawIcon(ACTIONS[selected_].icon, 160, 195, 27, UI::theme().accent2);
    UI::drawIcon(ACTIONS[next].icon, 267, 196, 20, 0xBDF7);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0xBDF7);
    canvas.setTextSize(1);
    canvas.drawString(ACTIONS[previous].label, 53, 221);
    canvas.drawString(ACTIONS[next].label, 267, 221);
    canvas.setTextColor(0xFFFF);
    canvas.drawString(ACTIONS[selected_].label, 160, 221);
    canvas.setTextDatum(top_left);
  }

  void drawLowNeedReaction() const {
    if (reactionMs_ > 950 || burstMs_ > 0) return;
    auto& canvas = Gfx::c();
    if (Buddy::isHungry()) {
      canvas.drawArc(160, 137, 23, 19, 205, 335, 0xFBE0);
      canvas.drawArc(160, 137, 27, 24, 205, 335, 0xFFFF);
    } else if (Buddy::isTired()) {
      canvas.fillEllipse(160, 121, 8, 4, 0x0000);
      canvas.drawEllipse(160, 121, 9, 5, 0xFFFF);
    } else if (Buddy::isBored()) {
      canvas.fillCircle(218, 71, 19, 0xFFFF);
      canvas.fillCircle(200, 91, 6, 0xFFFF);
      canvas.setTextDatum(middle_center);
      canvas.setTextColor(UI::theme().accent);
      canvas.setTextSize(2);
      canvas.drawString("?", 218, 70);
      canvas.setTextDatum(top_left);
    }
  }

  void drawRainbowSparkles() const {
    static constexpr int X[] = {92, 116, 143, 177, 204, 229, 76, 244};
    static constexpr int Y[] = {67, 49, 61, 47, 68, 91, 103, 124};
    static constexpr uint16_t C[] = {
      0xF800, 0xFD20, 0xFFE0, 0x07E0, 0x07FF, 0x001F, 0xF81F, 0xFFFF
    };
    auto& canvas = Gfx::c();
    const int phase = static_cast<int>((rainbowMs_ / 90) & 3);
    for (int i = 0; i < 8; ++i) {
      const int radius = 2 + ((i + phase) & 1);
      canvas.fillCircle(X[i], Y[i], radius, C[(i + phase) & 7]);
      canvas.drawLine(X[i] - 5, Y[i], X[i] + 5, Y[i], C[(i + phase) & 7]);
      canvas.drawLine(X[i], Y[i] - 5, X[i], Y[i] + 5, C[(i + phase) & 7]);
    }
  }

  void drawHint() const {
    auto& canvas = Gfx::c();
    canvas.fillRoundRect(55, 142, 210, 27, 8, 0xFFFF);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0x39E7);
    canvas.setTextSize(1);
    canvas.drawString("BUY SOME AT THE SHOP", 160, 155);
    canvas.setTextDatum(top_left);
  }

  int selected_ = 0;
  Anim anim_ = Anim::Idle;
  uint32_t animMs_ = 0;
  uint32_t burstMs_ = 0;
  uint32_t reactionMs_ = 0;
  uint32_t hintMs_ = 0;
  uint32_t rainbowMs_ = 0;
};

BuddyScreen instance;
ScreenRegistrar registrar(ScreenId::Buddy, instance);

}
