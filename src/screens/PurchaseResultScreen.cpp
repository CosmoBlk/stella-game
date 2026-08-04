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

class PurchaseResultScreen final : public Screen {
public:
  void enter() override {
    item_ = itemById(App::ctx.shopItemId);
    elapsedMs_ = 0;
    celebrated_ = false;
    Audio::play(Sfx::Purchase);
  }

  void update(uint32_t deltaMs) override {
    elapsedMs_ += deltaMs;
    if (!celebrated_ && elapsedMs_ >= 420) {
      celebrated_ = true;
      Audio::play(Sfx::Celebrate);
    }
    if (elapsedMs_ >= 2400) returnToShop();
  }

  void draw() override {
    const Theme& theme = UI::theme();
    UI::drawBackground();
    UI::drawHeader("IT'S YOURS!");

    UI::drawIcon(IconId::Star, 52, 62, 31, TFT_YELLOW, TFT_WHITE);
    UI::drawIcon(IconId::Sparkle, 269, 67, 27, TFT_WHITE, theme.accent);
    UI::drawIcon(IconId::Star, 39, 145, 20, theme.accent, TFT_YELLOW);
    UI::drawIcon(IconId::Sparkle, 281, 148, 22, TFT_YELLOW, TFT_WHITE);

    UI::drawPanel(70, 45, 180, 126, theme.panel);
    if (item_) {
      if (item_->category == ItemCategory::Character) {
        Sprites::drawCharacter(item_->id, 160, 108, 105, Anim::Celebrating, App::nowMs());
      } else {
        Sprites::drawItemPreview(item_->id, 160, 105, 98, App::nowMs());
      }
      drawCentred(item_->name, 160, 178, 2, theme.text);
    }

    UI::drawButtonBar("BACK", "DONE", "BACK",
                      IconId::Back, IconId::Star, IconId::Back);
  }

  void onButtonA() override { returnToShop(); }
  void onButtonB() override { returnToShop(); }
  void onButtonC() override { returnToShop(); }

  const char* name() const override { return "PurchaseResult"; }

private:
  void returnToShop() {
    Audio::play(Sfx::Select);
    App::goTo(ScreenId::ShopBrowse);
  }

  const ItemDef* item_ = nullptr;
  uint32_t elapsedMs_ = 0;
  bool celebrated_ = false;
};

PurchaseResultScreen instance;
ScreenRegistrar registrar(ScreenId::PurchaseResult, instance);

}
