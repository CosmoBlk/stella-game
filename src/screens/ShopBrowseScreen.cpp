#include <cstdio>

#include "App.h"
#include "Content.h"
#include "Managers.h"
#include "Sprites.h"
#include "UI.h"

namespace {

bool belongsToActivePlayer(const ItemDef& item) {
  if (item.owner == ItemOwner::Both) return true;
  return (App::player() == PlayerId::Hugo && item.owner == ItemOwner::HugoOnly) ||
         (App::player() == PlayerId::Stella && item.owner == ItemOwner::StellaOnly);
}

bool isEquipped(const ItemDef& item, const InventoryState& inventory) {
  switch (item.category) {
    case ItemCategory::Character: return inventory.equippedCharacter == item.id;
    case ItemCategory::Accessory: return inventory.equippedAccessory == item.id;
    case ItemCategory::Clothing: return inventory.equippedClothing == item.id;
    case ItemCategory::Toy: return inventory.equippedToy == item.id;
    case ItemCategory::Room: return inventory.equippedRoom == item.id;
    case ItemCategory::Treat: return false;
    default: return false;
  }
}

void equipItem(const ItemDef& item, InventoryState& inventory) {
  switch (item.category) {
    case ItemCategory::Character: inventory.equippedCharacter = item.id; break;
    case ItemCategory::Accessory: inventory.equippedAccessory = item.id; break;
    case ItemCategory::Clothing: inventory.equippedClothing = item.id; break;
    case ItemCategory::Toy: inventory.equippedToy = item.id; break;
    case ItemCategory::Room: inventory.equippedRoom = item.id; break;
    case ItemCategory::Treat: break;
    default: break;
  }
}

void drawCentred(const char* text, int cx, int y, int size, uint16_t colour) {
  auto& canvas = Gfx::c();
  canvas.setTextSize(size);
  canvas.setTextColor(colour);
  canvas.setCursor(cx - canvas.textWidth(text) / 2, y);
  canvas.print(text);
}

class ShopBrowseScreen final : public Screen {
public:
  void enter() override {
    count_ = 0;
    const ItemCategory category = static_cast<ItemCategory>(App::ctx.shopCategory);
    for (int index = 0; index < ITEM_COUNT && count_ < MAX_ITEMS; ++index) {
      if (ITEMS[index].category == category && belongsToActivePlayer(ITEMS[index])) {
        itemIds_[count_++] = ITEMS[index].id;
      }
    }
    selected_ = count_ == 0 ? 0 : App::ctx.selectSlot % count_;
    flashMs_ = 0;
  }

  void update(uint32_t deltaMs) override {
    if (flashMs_ > deltaMs) flashMs_ -= deltaMs;
    else flashMs_ = 0;
  }

  void draw() override {
    const Theme& theme = UI::theme();
    auto& canvas = Gfx::c();
    UI::drawBackground();
    UI::drawHeader("SHOP");

    if (count_ == 0) {
      UI::drawBigCentred("NOTHING HERE YET", 105, 2, theme.text);
      UI::drawButtonBar("", "", "", IconId::Back);
      return;
    }

    const ItemDef* item = currentItem();
    const InventoryState& inventory = App::profile().inventory;
    const bool owned = inventory.owned(item->id);
    const bool equipped = isEquipped(*item, inventory);

    UI::drawPanel(57, 36, 206, 133, theme.panel);
    if (item->category == ItemCategory::Character) {
      Sprites::drawCharacter(item->id, 160, 104, 98, Anim::Idle, App::nowMs());
    } else {
      Sprites::drawItemPreview(item->id, 160, 103, 96, App::nowMs());
    }

    if (owned && item->category != ItemCategory::Treat) {
      canvas.fillRoundRect(61, 40, 57, 18, 5, theme.accent);
      drawCentred("OWNED", 89, 45, 1, TFT_WHITE);
    }
    if (equipped) {
      UI::drawIcon(IconId::Star, 244, 50, 20, TFT_YELLOW, theme.accent);
    }

    drawCentred(item->name, 160, 174, 2, theme.text);
    if (owned && item->category != ItemCategory::Treat) {
      drawCentred(equipped ? "EQUIPPED" : "OWNED", 160, 194, 1,
                  equipped ? theme.accent : theme.text);
    } else {
      char price[18];
      std::snprintf(price, sizeof(price), "%u", item->price);
      UI::drawIcon(IconId::Coin, 140, 196, 16, TFT_YELLOW, TFT_ORANGE);
      canvas.setTextSize(2);
      canvas.setTextColor(theme.text);
      canvas.setCursor(151, 190);
      canvas.print(price);
    }

    const char* action = item->category == ItemCategory::Treat ? "BUY" :
                         (owned ? "EQUIP" : "LOOK");
    UI::drawButtonBar("PREV", action, "NEXT",
                      IconId::ArrowL, owned ? IconId::Star : IconId::Shop, IconId::ArrowR);

    if (flashMs_ > 0) {
      canvas.fillRoundRect(86, 84, 148, 42, 10, theme.accent);
      UI::drawIcon(IconId::Star, 106, 105, 23, TFT_YELLOW, TFT_WHITE);
      drawCentred("EQUIPPED!", 170, 97, 2, TFT_WHITE);
    }
  }

  void onButtonA() override { move(-1); }

  void onButtonB() override {
    const ItemDef* item = currentItem();
    if (!item) return;
    App::ctx.shopItemId = item->id;
    App::ctx.selectSlot = selected_;

    if (item->category == ItemCategory::Treat) {
      Audio::play(Sfx::Select);
      App::goTo(ScreenId::PurchaseConfirm);
      return;
    }

    InventoryState& inventory = App::profile().inventory;
    if (!inventory.owned(item->id)) {
      Audio::play(Sfx::Select);
      App::goTo(ScreenId::ShopPreview);
      return;
    }

    equipItem(*item, inventory);
    flashMs_ = 800;
    Audio::play(Sfx::Sparkle);
    Save::requestSave();
  }

  void onButtonC() override { move(1); }

  const char* name() const override { return "ShopBrowse"; }

private:
  const ItemDef* currentItem() const {
    return count_ == 0 ? nullptr : itemById(itemIds_[selected_]);
  }

  void move(int direction) {
    if (count_ == 0) return;
    selected_ = static_cast<uint8_t>((selected_ + count_ + direction) % count_);
    App::ctx.selectSlot = selected_;
    flashMs_ = 0;
    Audio::play(Sfx::Select);
  }

  uint16_t itemIds_[MAX_ITEMS]{};
  uint8_t count_ = 0;
  uint8_t selected_ = 0;
  uint32_t flashMs_ = 0;
};

ShopBrowseScreen instance;
ScreenRegistrar registrar(ScreenId::ShopBrowse, instance);

}
