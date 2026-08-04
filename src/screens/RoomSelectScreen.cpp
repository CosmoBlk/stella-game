#include "../App.h"
#include "../Config.h"
#include "../Content.h"
#include "../Managers.h"
#include "../Sprites.h"
#include "../UI.h"

namespace {

class RoomSelectScreen final : public Screen {
public:
  void enter() override {
    count_ = 0;
    selected_ = 0;
    animMs_ = 0;
    celebrateMs_ = 0;
    const InventoryState& inventory = App::profile().inventory;
    for (int i = 0; i < ITEM_COUNT && count_ < MAX_ITEMS; ++i) {
      if (ITEMS[i].category == ItemCategory::Room && inventory.owned(ITEMS[i].id)) {
        choices_[count_++] = ITEMS[i].id;
      }
    }
    selectEquipped(inventory.equippedRoom);
  }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    if (celebrateMs_ > deltaMs) celebrateMs_ -= deltaMs;
    else celebrateMs_ = 0;
  }

  void draw() override {
    if (count_ == 0) {
      UI::drawBackground();
      UI::drawHeader("ROOM");
      UI::drawIcon(IconId::Home, 160, 94, 58, UI::theme().accent);
      UI::drawBigCentred("NO ROOMS YET", 145, 2, UI::theme().text);
      return;
    }

    const uint16_t roomId = choices_[selected_];
    const InventoryState& inventory = App::profile().inventory;
    Sprites::drawRoom(roomId);
    UI::drawHeader("ROOM");
    Sprites::drawCharacter(inventory.equippedCharacter, 160, 116, 100,
                           celebrateMs_ > 0 ? Anim::Celebrating : Anim::Idle,
                           animMs_, inventory.equippedAccessory,
                           inventory.equippedClothing);
    drawInfo(roomId, roomId == inventory.equippedRoom);
    if (celebrateMs_ > 0) drawSparkles();
    UI::drawButtonBar("PREV", "EQUIP", "NEXT", IconId::ArrowL,
                      IconId::Sparkle, IconId::ArrowR);
  }

  void onButtonA() override {
    if (count_ == 0 || celebrateMs_ > 0) return;
    selected_ = (selected_ + count_ - 1) % count_;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    if (count_ == 0) return;
    App::profile().inventory.equippedRoom = choices_[selected_];
    celebrateMs_ = 1050;
    animMs_ = 0;
    Audio::play(Sfx::Sparkle);
    Save::requestSave();
  }

  void onButtonC() override {
    if (count_ == 0 || celebrateMs_ > 0) return;
    selected_ = (selected_ + 1) % count_;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "ROOM SELECT"; }

private:
  void selectEquipped(uint16_t equipped) {
    for (int i = 0; i < count_; ++i) {
      if (choices_[i] == equipped) {
        selected_ = i;
        return;
      }
    }
  }

  void drawInfo(uint16_t roomId, bool equipped) const {
    const ItemDef* item = itemById(roomId);
    auto& canvas = Gfx::c();
    canvas.fillRoundRect(36, 164, 248, 34, 9, 0xFFFF);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0x39E7);
    canvas.setTextSize(2);
    canvas.drawString(item ? item->name : "ROOM", 160, 176);
    if (equipped) {
      canvas.fillRoundRect(117, 184, 86, 12, 5, UI::theme().accent);
      canvas.setTextColor(0xFFFF);
      canvas.setTextSize(1);
      canvas.drawString("EQUIPPED", 160, 190);
    }
    canvas.setTextDatum(top_left);
  }

  void drawSparkles() const {
    auto& canvas = Gfx::c();
    static constexpr int X[] = {91, 112, 205, 229, 77, 243};
    static constexpr int Y[] = {61, 42, 45, 72, 119, 126};
    for (int i = 0; i < 6; ++i) {
      const uint16_t color = (i & 1) ? UI::theme().accent2 : 0xFFE0;
      canvas.drawLine(X[i] - 5, Y[i], X[i] + 5, Y[i], color);
      canvas.drawLine(X[i], Y[i] - 5, X[i], Y[i] + 5, color);
      canvas.fillCircle(X[i], Y[i], 2, 0xFFFF);
    }
  }

  uint16_t choices_[MAX_ITEMS] = {};
  int count_ = 0;
  int selected_ = 0;
  uint32_t animMs_ = 0;
  uint32_t celebrateMs_ = 0;
};

RoomSelectScreen instance;
ScreenRegistrar registrar(ScreenId::RoomSelect, instance);

}
