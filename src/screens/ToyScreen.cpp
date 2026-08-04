#include "../App.h"
#include "../Config.h"
#include "../Content.h"
#include "../Managers.h"
#include "../Sprites.h"
#include "../UI.h"

namespace {

uint8_t addNeed(uint8_t value, uint8_t amount) {
  const uint16_t total = static_cast<uint16_t>(value) + amount;
  return total > NEED_MAX ? NEED_MAX : static_cast<uint8_t>(total);
}

class ToyScreen final : public Screen {
public:
  void enter() override {
    count_ = 0;
    selected_ = 0;
    animMs_ = 0;
    playingMs_ = 0;
    buildToys();
    selectEquipped();
  }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    if (playingMs_ > deltaMs) playingMs_ -= deltaMs;
    else playingMs_ = 0;
  }

  void draw() override {
    const InventoryState& inventory = App::profile().inventory;
    Sprites::drawRoom(inventory.equippedRoom);
    UI::drawHeader("CHOOSE A TOY");
    if (count_ == 0) {
      drawEmptyHint();
      UI::drawButtonBar("", "", "", IconId::ArrowL, IconId::None, IconId::ArrowR);
      return;
    }

    const uint16_t toyId = toys_[selected_];
    const Anim anim = playingMs_ > 0 ? Anim::Celebrating : Buddy::moodAnim();
    Sprites::drawCharacter(inventory.equippedCharacter, 112, 116, 100, anim,
                           animMs_, inventory.equippedAccessory,
                           inventory.equippedClothing);
    Sprites::drawItemPreview(toyId, 235, 119, playingMs_ > 0 ? 82 : 70, animMs_);
    drawLabel(toyId);
    UI::drawButtonBar("PREV", "PLAY", "NEXT", IconId::ArrowL,
                      IconId::Gift, IconId::ArrowR);
  }

  void onButtonA() override {
    if (count_ == 0 || playingMs_ > 0) return;
    selected_ = (selected_ + count_ - 1) % count_;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    if (count_ == 0 || playingMs_ > 0) return;
    PlayerProfile& profile = App::profile();
    profile.inventory.equippedToy = toys_[selected_];
    profile.buddy.happiness = addNeed(profile.buddy.happiness, 15);
    playingMs_ = 1600;
    animMs_ = 0;
    Audio::play(Sfx::Celebrate);
    App::raiseEvent(GameEvent::BuddyPlayed);
    Save::requestSave();
  }

  void onButtonC() override {
    if (count_ == 0 || playingMs_ > 0) return;
    selected_ = (selected_ + 1) % count_;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "TOY"; }

private:
  void buildToys() {
    const InventoryState& inventory = App::profile().inventory;
    for (int i = 0; i < ITEM_COUNT && count_ < MAX_ITEMS; ++i) {
      if (ITEMS[i].category == ItemCategory::Toy && inventory.owned(ITEMS[i].id)) {
        toys_[count_++] = ITEMS[i].id;
      }
    }
  }

  void selectEquipped() {
    const uint16_t equipped = App::profile().inventory.equippedToy;
    for (int i = 0; i < count_; ++i) {
      if (toys_[i] == equipped) {
        selected_ = i;
        return;
      }
    }
  }

  void drawLabel(uint16_t toyId) const {
    const ItemDef* item = itemById(toyId);
    auto& canvas = Gfx::c();
    canvas.fillRoundRect(47, 169, 226, 27, 8, 0xFFFF);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0x39E7);
    canvas.setTextSize(2);
    canvas.drawString(playingMs_ > 0 ? "LET'S PLAY!" : (item ? item->name : "TOY"), 160, 182);
    canvas.setTextDatum(top_left);
  }

  void drawEmptyHint() const {
    auto& canvas = Gfx::c();
    canvas.fillRoundRect(42, 59, 236, 124, 14, 0xFFFF);
    UI::drawIcon(IconId::Gift, 160, 92, 45, UI::theme().accent, UI::theme().accent2);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0x39E7);
    canvas.setTextSize(2);
    canvas.drawString("NO TOYS YET", 160, 133);
    canvas.setTextSize(1);
    canvas.setTextColor(UI::theme().accent);
    canvas.drawString("VISIT THE SHOP!", 160, 160);
    canvas.setTextDatum(top_left);
  }

  uint16_t toys_[MAX_ITEMS] = {};
  int count_ = 0;
  int selected_ = 0;
  uint32_t animMs_ = 0;
  uint32_t playingMs_ = 0;
};

ToyScreen instance;
ScreenRegistrar registrar(ScreenId::Toy, instance);

}
