#include "../App.h"
#include "../Config.h"
#include "../Content.h"
#include "../Managers.h"
#include "../Sprites.h"
#include "../UI.h"

namespace {

class AccessorySelectScreen final : public Screen {
public:
  void enter() override {
    count_ = 1;
    selected_ = 0;
    animMs_ = 0;
    celebrateMs_ = 0;
    choices_[0] = ITEM_NONE;
    const InventoryState& inventory = App::profile().inventory;
    for (int i = 0; i < ITEM_COUNT && count_ < MAX_ITEMS + 1; ++i) {
      if (ITEMS[i].category == ItemCategory::Accessory && inventory.owned(ITEMS[i].id)) {
        choices_[count_++] = ITEMS[i].id;
      }
    }
    selectEquipped(inventory.equippedAccessory);
  }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    if (celebrateMs_ > deltaMs) celebrateMs_ -= deltaMs;
    else celebrateMs_ = 0;
  }

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("ACCESSORY");
    const uint16_t itemId = choices_[selected_];
    UI::drawPanel(63, 43, 194, 119, UI::theme().panel);
    if (itemId == ITEM_NONE) drawNonePreview();
    else Sprites::drawItemPreview(itemId, 160, 101, 96, animMs_);
    drawInfo(itemId, itemId == App::profile().inventory.equippedAccessory);
    if (celebrateMs_ > 0) drawSparkles();
    UI::drawButtonBar("PREV", "EQUIP", "NEXT", IconId::ArrowL,
                      IconId::Sparkle, IconId::ArrowR);
  }

  void onButtonA() override {
    if (celebrateMs_ > 0) return;
    selected_ = (selected_ + count_ - 1) % count_;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    App::profile().inventory.equippedAccessory = choices_[selected_];
    celebrateMs_ = 950;
    Audio::play(Sfx::Sparkle);
    Save::requestSave();
  }

  void onButtonC() override {
    if (celebrateMs_ > 0) return;
    selected_ = (selected_ + 1) % count_;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "ACCESSORY SELECT"; }

private:
  void selectEquipped(uint16_t equipped) {
    for (int i = 0; i < count_; ++i) {
      if (choices_[i] == equipped) {
        selected_ = i;
        return;
      }
    }
  }

  void drawNonePreview() const {
    auto& canvas = Gfx::c();
    UI::drawIcon(IconId::Hat, 160, 101, 65, UI::theme().accent);
    canvas.drawLine(124, 137, 196, 65, 0xF800);
    canvas.drawLine(127, 140, 199, 68, 0xFFFF);
  }

  void drawInfo(uint16_t itemId, bool equipped) const {
    const ItemDef* item = itemById(itemId);
    auto& canvas = Gfx::c();
    canvas.fillRoundRect(36, 167, 248, 31, 9, UI::theme().panel);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(2);
    canvas.drawString(itemId == ITEM_NONE ? "NONE" : (item ? item->name : "ACCESSORY"), 160, 177);
    if (equipped) {
      canvas.fillRoundRect(117, 185, 86, 12, 5, UI::theme().accent);
      canvas.setTextColor(0xFFFF);
      canvas.setTextSize(1);
      canvas.drawString("EQUIPPED", 160, 191);
    }
    canvas.setTextDatum(top_left);
  }

  void drawSparkles() const {
    auto& canvas = Gfx::c();
    static constexpr int X[] = {83, 108, 211, 237, 96, 224};
    static constexpr int Y[] = {72, 47, 49, 79, 137, 135};
    for (int i = 0; i < 6; ++i) {
      const uint16_t color = (i & 1) ? UI::theme().accent2 : 0xFFE0;
      canvas.drawLine(X[i] - 5, Y[i], X[i] + 5, Y[i], color);
      canvas.drawLine(X[i], Y[i] - 5, X[i], Y[i] + 5, color);
    }
  }

  uint16_t choices_[MAX_ITEMS + 1] = {};
  int count_ = 0;
  int selected_ = 0;
  uint32_t animMs_ = 0;
  uint32_t celebrateMs_ = 0;
};

AccessorySelectScreen instance;
ScreenRegistrar registrar(ScreenId::AccessorySelect, instance);

}
