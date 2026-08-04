#include "../App.h"
#include "../Config.h"
#include "../Content.h"
#include "../Managers.h"
#include "../Sprites.h"
#include "../UI.h"
#include <stdio.h>

namespace {

class FeedScreen final : public Screen {
public:
  void enter() override {
    count_ = 0;
    selected_ = 0;
    animMs_ = 0;
    eatingMs_ = 0;
    rebuildAfterEating_ = false;
    buildFeedables();
  }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    if (eatingMs_ > deltaMs) {
      eatingMs_ -= deltaMs;
    } else if (eatingMs_ > 0) {
      eatingMs_ = 0;
      if (rebuildAfterEating_) {
        rebuildAfterEating_ = false;
        buildFeedables();
        if (count_ == 0) selected_ = 0;
        else if (selected_ >= count_) selected_ = count_ - 1;
      }
    }
  }

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("FEED BUDDY");
    if (count_ == 0) {
      drawEmptyHint();
      UI::drawButtonBar("", "", "", IconId::ArrowL, IconId::None, IconId::ArrowR);
      return;
    }

    const InventoryState& inventory = App::profile().inventory;
    const uint16_t itemId = feedables_[selected_];
    const Anim anim = eatingMs_ > 0 ? Anim::Eating : Buddy::moodAnim();
    Sprites::drawCharacter(inventory.equippedCharacter, 103, 112, 91, anim,
                           animMs_, inventory.equippedAccessory,
                           inventory.equippedClothing);
    drawFood(itemId);
    drawLabel(itemId);
    UI::drawButtonBar("PREV", "FEED", "NEXT", IconId::ArrowL,
                      IconId::Apple, IconId::ArrowR);
  }

  void onButtonA() override {
    if (count_ == 0 || eatingMs_ > 0) return;
    selected_ = (selected_ + count_ - 1) % count_;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    if (count_ == 0 || eatingMs_ > 0) return;
    const uint16_t itemId = feedables_[selected_];
    if (itemId == ITEM_JELLYBEAN_BAG) {
      Buddy::feedJellyBeans();
      App::raiseEvent(GameEvent::BuddyFed);
      rebuildAfterEating_ = true;
    } else {
      Buddy::feed(30, 10);
    }
    eatingMs_ = 1500;
    animMs_ = 0;
    Save::requestSave();
  }

  void onButtonC() override {
    if (count_ == 0 || eatingMs_ > 0) return;
    selected_ = (selected_ + 1) % count_;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "FEED"; }

private:
  void buildFeedables() {
    count_ = 0;
    const PlayerProfile& profile = App::profile();
    if (profile.buddy.jellyBeans > 0) feedables_[count_++] = ITEM_JELLYBEAN_BAG;
    for (int i = 0; i < ITEM_COUNT && count_ < MAX_ITEMS + 1; ++i) {
      const ItemDef& item = ITEMS[i];
      if (item.category != ItemCategory::Treat || item.id == ITEM_JELLYBEAN_BAG) continue;
      if (profile.inventory.owned(item.id)) feedables_[count_++] = item.id;
    }
  }

  void drawFood(uint16_t itemId) const {
    auto& canvas = Gfx::c();
    UI::drawPanel(177, 55, 118, 105, UI::theme().panel);
    if (itemId == ITEM_JELLYBEAN_BAG) {
      Sprites::drawJellyBean(236, 101, 42, static_cast<uint8_t>((animMs_ / 180) % 10));
      canvas.setTextDatum(middle_center);
      canvas.setTextColor(UI::theme().text);
      canvas.setTextSize(2);
      char countLabel[12];
      snprintf(countLabel, sizeof(countLabel), "X %u", App::profile().buddy.jellyBeans);
      canvas.drawString(countLabel, 236, 143);
      canvas.setTextDatum(top_left);
    } else {
      Sprites::drawItemPreview(itemId, 236, 106, 78, animMs_);
    }
  }

  void drawLabel(uint16_t itemId) const {
    const ItemDef* item = itemById(itemId);
    const char* label = itemId == ITEM_JELLYBEAN_BAG ? "JELLY BEANS" : (item ? item->name : "TREAT");
    auto& canvas = Gfx::c();
    canvas.fillRoundRect(42, 169, 236, 27, 8, UI::theme().panel);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(2);
    canvas.drawString(eatingMs_ > 0 ? "YUM YUM!" : label, 160, 182);
    canvas.setTextDatum(top_left);
  }

  void drawEmptyHint() const {
    UI::drawIcon(IconId::Shop, 160, 84, 48, UI::theme().accent, UI::theme().accent2);
    UI::drawBigCentred("NO TREATS YET", 126, 2, UI::theme().text);
    UI::drawBigCentred("VISIT THE SHOP!", 158, 1, UI::theme().accent);
  }

  uint16_t feedables_[MAX_ITEMS + 1] = {};
  int count_ = 0;
  int selected_ = 0;
  uint32_t animMs_ = 0;
  uint32_t eatingMs_ = 0;
  bool rebuildAfterEating_ = false;
};

FeedScreen instance;
ScreenRegistrar registrar(ScreenId::Feed, instance);

}
