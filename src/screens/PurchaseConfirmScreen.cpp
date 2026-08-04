#include <cstdio>

#include "App.h"
#include "Content.h"
#include "Managers.h"
#include "Sprites.h"
#include "UI.h"

namespace {

void drawCentred(const char* text, int cx, int y, int size, uint16_t colour) {
  auto& canvas = Gfx::c();
  canvas.setTextSize(size);
  canvas.setTextColor(colour);
  canvas.setCursor(cx - canvas.textWidth(text) / 2, y);
  canvas.print(text);
}

void autoEquipIfEmpty(const ItemDef& item, InventoryState& inventory) {
  switch (item.category) {
    case ItemCategory::Character:
      if (inventory.equippedCharacter == ITEM_NONE) inventory.equippedCharacter = item.id;
      break;
    case ItemCategory::Accessory:
      if (inventory.equippedAccessory == ITEM_NONE) inventory.equippedAccessory = item.id;
      break;
    case ItemCategory::Clothing:
      if (inventory.equippedClothing == ITEM_NONE) inventory.equippedClothing = item.id;
      break;
    case ItemCategory::Toy:
      if (inventory.equippedToy == ITEM_NONE) inventory.equippedToy = item.id;
      break;
    case ItemCategory::Room:
      if (inventory.equippedRoom == ITEM_NONE) inventory.equippedRoom = item.id;
      break;
    case ItemCategory::Treat:
      break;
    default:
      break;
  }
}

class PurchaseConfirmScreen final : public Screen {
public:
  void enter() override {
    item_ = itemById(App::ctx.shopItemId);
    insufficientMs_ = 0;
  }

  void update(uint32_t deltaMs) override {
    if (insufficientMs_ > deltaMs) insufficientMs_ -= deltaMs;
    else insufficientMs_ = 0;
  }

  void draw() override {
    const Theme& theme = UI::theme();
    auto& canvas = Gfx::c();
    UI::drawBackground();
    UI::drawHeader("CHECKOUT");

    if (!item_) {
      UI::drawBigCentred("ITEM NOT FOUND", 105, 2, theme.text);
      UI::drawButtonBar("NO", "", "", IconId::Back);
      return;
    }

    const int wobble = insufficientMs_ == 0 ? 0 :
      ((insufficientMs_ / 70) % 2 == 0 ? -5 : 5);
    UI::drawPanel(46 + wobble, 39, 228, 120, theme.panel);
    if (item_->category == ItemCategory::Character) {
      Sprites::drawCharacter(item_->id, 160 + wobble, 98, 87, Anim::Idle, App::nowMs());
    } else {
      Sprites::drawItemPreview(item_->id, 160 + wobble, 98, 84, App::nowMs());
    }

    char question[32];
    std::snprintf(question, sizeof(question), "BUY FOR %u COINS?", item_->price);
    drawCentred(question, 160, 164, 2, theme.text);

    char balance[28];
    std::snprintf(balance, sizeof(balance), "YOU HAVE %lu", static_cast<unsigned long>(App::profile().coins));
    UI::drawIcon(IconId::Coin, 116, 193, 15, TFT_YELLOW, TFT_ORANGE);
    drawCentred(balance, 180, 188, 1, theme.text);

    if (insufficientMs_ > 0) {
      canvas.fillRoundRect(31, 71, 258, 72, 10, theme.accent);
      drawCentred("EARN MORE COINS!", 160, 82, 2, TFT_WHITE);
      UI::drawIcon(IconId::Tick, 91, 119, 27, TFT_WHITE, theme.accent2);
      UI::drawIcon(IconId::Games, 160, 119, 27, TFT_WHITE, theme.accent2);
      UI::drawIcon(IconId::Star, 229, 119, 27, TFT_YELLOW, theme.accent2);
    }

    UI::drawButtonBar("NO", "YES", "", IconId::Cross, IconId::Tick);
  }

  void onButtonA() override {
    Audio::play(Sfx::Back);
    if (item_ && item_->category != ItemCategory::Treat) App::goTo(ScreenId::ShopPreview);
    else App::goTo(ScreenId::ShopBrowse);
  }

  void onButtonB() override {
    if (!item_ || insufficientMs_ > 0) return;
    if (!App::spendCoins(item_->price)) {
      insufficientMs_ = 1200;
      Audio::play(Sfx::GoodTry);
      return;
    }

    PlayerProfile& profile = App::profile();
    if (item_->category == ItemCategory::Treat) {
      if (item_->id == ITEM_JELLYBEAN_BAG) {
        profile.buddy.jellyBeans = profile.buddy.jellyBeans > 245
          ? 255 : profile.buddy.jellyBeans + 10;
      } else {
        Buddy::feed(30, 10);
      }
    } else {
      profile.inventory.setOwned(item_->id);
      autoEquipIfEmpty(*item_, profile.inventory);
    }

    Serial.printf("[shop] bought %s\n", item_->name);
    App::raiseEvent(GameEvent::ItemBought, static_cast<uint8_t>(item_->id));
    Save::saveNow();
    App::goTo(ScreenId::PurchaseResult);
  }

  const char* name() const override { return "PurchaseConfirm"; }

private:
  const ItemDef* item_ = nullptr;
  uint32_t insufficientMs_ = 0;
};

PurchaseConfirmScreen instance;
ScreenRegistrar registrar(ScreenId::PurchaseConfirm, instance);

}
