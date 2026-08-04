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

class ShopPreviewScreen final : public Screen {
public:
  void enter() override { item_ = itemById(App::ctx.shopItemId); }

  void update(uint32_t) override {}

  void draw() override {
    const Theme& theme = UI::theme();
    const InventoryState& inventory = App::profile().inventory;

    if (item_ && item_->category == ItemCategory::Room) {
      Sprites::drawRoom(item_->id);
    } else {
      UI::drawBackground();
    }
    UI::drawHeader("TAKE A LOOK!");

    if (!item_) {
      UI::drawBigCentred("ITEM NOT FOUND", 105, 2, theme.text);
      UI::drawButtonBar("BACK", "", "", IconId::Back);
      return;
    }

    switch (item_->category) {
      case ItemCategory::Character:
        Sprites::drawCharacter(item_->id, 160, 111, 145, Anim::Happy, App::nowMs(),
                               inventory.equippedAccessory, inventory.equippedClothing);
        break;
      case ItemCategory::Accessory:
        Sprites::drawCharacter(inventory.equippedCharacter, 160, 111, 145, Anim::Happy,
                               App::nowMs(), item_->id, inventory.equippedClothing);
        break;
      case ItemCategory::Clothing:
        Sprites::drawCharacter(inventory.equippedCharacter, 160, 111, 145, Anim::Happy,
                               App::nowMs(), inventory.equippedAccessory, item_->id);
        break;
      case ItemCategory::Room:
        UI::drawPanel(20, 154, 280, 39, theme.panel);
        Sprites::drawCharacter(inventory.equippedCharacter, 160, 125, 91, Anim::Happy,
                               App::nowMs(), inventory.equippedAccessory,
                               inventory.equippedClothing);
        break;
      case ItemCategory::Toy:
      case ItemCategory::Treat:
        UI::drawPanel(27, 42, 266, 126, theme.panel);
        Sprites::drawItemPreview(item_->id, 105, 106, 112, App::nowMs());
        Sprites::drawCharacter(inventory.equippedCharacter, 230, 116, 91, Anim::Happy,
                               App::nowMs(), inventory.equippedAccessory,
                               inventory.equippedClothing);
        break;
      default:
        break;
    }

    drawCentred(item_->name, 160, 177, 2, theme.text);
    UI::drawButtonBar("BACK", "BUY", "", IconId::Back, IconId::Shop);
  }

  void onButtonA() override {
    Audio::play(Sfx::Back);
    App::goTo(ScreenId::ShopBrowse);
  }

  void onButtonB() override {
    if (!item_) return;
    Audio::play(Sfx::Select);
    App::goTo(ScreenId::PurchaseConfirm);
  }

  const char* name() const override { return "ShopPreview"; }

private:
  const ItemDef* item_ = nullptr;
};

ShopPreviewScreen instance;
ScreenRegistrar registrar(ScreenId::ShopPreview, instance);

}
