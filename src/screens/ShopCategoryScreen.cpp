#include "App.h"
#include "Managers.h"
#include "UI.h"

namespace {

constexpr const char* CATEGORY_NAMES[] = {
  "CHARACTERS", "ACCESSORIES", "CLOTHES", "TOYS", "ROOMS", "TREATS"
};

constexpr IconId CATEGORY_ICONS[] = {
  IconId::Smiley, IconId::Hat, IconId::Shirt,
  IconId::Gift, IconId::Home, IconId::Apple
};

void drawCentred(const char* text, int cx, int y, int size, uint16_t colour) {
  auto& canvas = Gfx::c();
  canvas.setTextSize(size);
  canvas.setTextColor(colour);
  canvas.setCursor(cx - canvas.textWidth(text) / 2, y);
  canvas.print(text);
}

class ShopCategoryScreen final : public Screen {
public:
  void enter() override {
    selected_ = 0;  // always open at CHARACTERS — predictable for little kids
  }

  void update(uint32_t) override {}

  void draw() override {
    const Theme& theme = UI::theme();
    auto& canvas = Gfx::c();
    UI::drawBackground();
    UI::drawHeader("SHOP");

    UI::drawPanel(225, 31, 86, 25, theme.panel);
    UI::drawCoinBalance(233, 35, App::profile().coins);

    for (uint8_t index = 0; index < 6; ++index) {
      const int column = index % 3;
      const int row = index / 3;
      const int x = 7 + column * 104;
      const int y = 61 + row * 67;
      const bool selected = index == selected_;

      UI::drawPanel(x, y, 98, 61, selected ? theme.accent2 : theme.panel);
      if (selected) canvas.drawRoundRect(x, y, 98, 61, 8, theme.accent);
      UI::drawIcon(CATEGORY_ICONS[index], x + 49, y + 24, 29,
                   selected ? TFT_WHITE : theme.accent, theme.accent2);
      drawCentred(CATEGORY_NAMES[index], x + 49, y + 45, 1,
                  selected ? TFT_WHITE : theme.text);
    }

    UI::drawButtonBar("PREV", "OPEN", "NEXT",
                      IconId::ArrowL, IconId::Shop, IconId::ArrowR);
  }

  void onButtonA() override {
    selected_ = selected_ == 0 ? 5 : selected_ - 1;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    App::ctx.shopCategory = selected_;
    App::ctx.selectSlot = 0;
    Audio::play(Sfx::Select);
    App::goTo(ScreenId::ShopBrowse);
  }

  void onButtonC() override {
    selected_ = (selected_ + 1) % 6;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "ShopCategory"; }

private:
  uint8_t selected_ = 0;
};

ShopCategoryScreen instance;
ScreenRegistrar registrar(ScreenId::ShopCategory, instance);

}
