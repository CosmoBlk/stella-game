#include <cstdio>

#include "App.h"
#include "Content.h"
#include "Managers.h"
#include "UI.h"

namespace {

constexpr uint8_t COLLECTION_CHARACTERS = static_cast<uint8_t>(ItemCategory::Character);
constexpr uint8_t COLLECTION_ACCESSORIES = static_cast<uint8_t>(ItemCategory::Accessory);
constexpr uint8_t COLLECTION_CLOTHES = static_cast<uint8_t>(ItemCategory::Clothing);
constexpr uint8_t COLLECTION_TOYS = static_cast<uint8_t>(ItemCategory::Toy);
constexpr uint8_t COLLECTION_ROOMS = static_cast<uint8_t>(ItemCategory::Room);
constexpr uint8_t COLLECTION_TREATS = static_cast<uint8_t>(ItemCategory::Treat);
constexpr uint8_t COLLECTION_COLLECTIBLE_BASE = 100;
constexpr uint8_t COLLECTION_DINOSAURS = COLLECTION_COLLECTIBLE_BASE + static_cast<uint8_t>(CollectGroup::Dinosaur);
constexpr uint8_t COLLECTION_JELLY_BEANS = COLLECTION_COLLECTIBLE_BASE + static_cast<uint8_t>(CollectGroup::JellyBean);
constexpr uint8_t COLLECTION_FRANKIE_FINDS = COLLECTION_COLLECTIBLE_BASE + static_cast<uint8_t>(CollectGroup::FrankieFind);
constexpr uint8_t COLLECTION_DAD_BADGES = COLLECTION_COLLECTIBLE_BASE + static_cast<uint8_t>(CollectGroup::DadBadge);
constexpr uint8_t COLLECTION_GAME_BADGES = COLLECTION_COLLECTIBLE_BASE + static_cast<uint8_t>(CollectGroup::GameBadge);
constexpr uint8_t COLLECTION_SHARKS = COLLECTION_COLLECTIBLE_BASE + static_cast<uint8_t>(CollectGroup::Shark);

struct CategoryDef {
  uint8_t value;
  const char* name;
  IconId icon;
  uint16_t colour;
};

constexpr CategoryDef CATEGORIES[] = {
  {COLLECTION_CHARACTERS, "CHARACTERS", IconId::Smiley, TFT_CYAN},
  {COLLECTION_ACCESSORIES, "ACCESSORIES", IconId::Hat, TFT_ORANGE},
  {COLLECTION_CLOTHES, "CLOTHES", IconId::Shirt, TFT_MAGENTA},
  {COLLECTION_TOYS, "TOYS", IconId::Gift, TFT_YELLOW},
  {COLLECTION_ROOMS, "ROOMS", IconId::Home, TFT_GREEN},
  {COLLECTION_DINOSAURS, "DINOSAURS", IconId::Dino, TFT_ORANGE},
  {COLLECTION_SHARKS, "SHARKS", IconId::Shark, TFT_SKYBLUE},
  {COLLECTION_JELLY_BEANS, "JELLY BEANS", IconId::JellyBean, TFT_MAGENTA},
  {COLLECTION_FRANKIE_FINDS, "FRANKIE FINDS", IconId::Paw, TFT_ORANGE},
  {COLLECTION_DAD_BADGES, "DAD BADGES", IconId::Missions, TFT_YELLOW},
  {COLLECTION_GAME_BADGES, "GAME BADGES", IconId::Games, TFT_GREEN},
};

constexpr uint8_t CATEGORY_COUNT = sizeof(CATEGORIES) / sizeof(CATEGORIES[0]);

bool belongsToActivePlayer(const ItemDef& item) {
  if (item.owner == ItemOwner::Both) return true;
  return (App::player() == PlayerId::Hugo && item.owner == ItemOwner::HugoOnly) ||
         (App::player() == PlayerId::Stella && item.owner == ItemOwner::StellaOnly);
}

void categoryProgress(uint8_t value, uint8_t& found, uint8_t& total) {
  found = 0;
  total = 0;
  const PlayerProfile& profile = App::profile();

  if (value <= COLLECTION_TREATS) {
    const ItemCategory category = static_cast<ItemCategory>(value);
    for (int index = 0; index < ITEM_COUNT; ++index) {
      const ItemDef& item = ITEMS[index];
      if (item.category != category || !belongsToActivePlayer(item)) continue;
      ++total;
      if (profile.inventory.owned(item.id)) ++found;
    }
    return;
  }

  const uint8_t groupIndex = value - COLLECTION_COLLECTIBLE_BASE;
  if (groupIndex >= static_cast<uint8_t>(CollectGroup::COUNT)) return;
  const CollectGroup group = static_cast<CollectGroup>(groupIndex);
  total = COLLECT_COUNT[groupIndex];
  for (uint8_t index = 0; index < total; ++index) {
    if (profile.hasCollectible(group, index)) ++found;
  }
}

void drawCentred(const char* text, int cx, int y, int size, uint16_t colour) {
  M5Canvas& canvas = Gfx::c();
  canvas.setTextSize(size);
  canvas.setTextColor(colour);
  canvas.setCursor(cx - canvas.textWidth(text) / 2, y);
  canvas.print(text);
}

class CollectionCategoryScreen final : public Screen {
public:
  void enter() override {
    selected_ = 0;
    for (uint8_t index = 0; index < CATEGORY_COUNT; ++index) {
      if (CATEGORIES[index].value == App::ctx.shopCategory) {
        selected_ = index;
        break;
      }
    }
  }

  void update(uint32_t) override {}

  void draw() override {
    const Theme& theme = UI::theme();
    M5Canvas& canvas = Gfx::c();
    UI::drawBackground();
    UI::drawHeader("COLLECTION");

    const CategoryDef& category = CATEGORIES[selected_];
    uint8_t found = 0;
    uint8_t total = 0;
    categoryProgress(category.value, found, total);

    UI::drawPanel(47, 40, 226, 151, theme.panel);
    canvas.drawRoundRect(47, 40, 226, 151, 18, category.colour);
    canvas.drawRoundRect(49, 42, 222, 147, 16, category.colour);
    UI::drawIcon(category.icon, 160, 98, 76, category.colour, theme.accent2);
    drawCentred(category.name, 160, 145, 2, theme.text);

    char progress[20];
    std::snprintf(progress, sizeof(progress), "%u/%u FOUND", found, total);
    drawCentred(progress, 160, 171, 1, theme.accent);

    UI::drawButtonBar("PREV", "OPEN", "NEXT",
                      IconId::ArrowL, IconId::Collection, IconId::ArrowR);
  }

  void onButtonA() override {
    selected_ = selected_ == 0 ? CATEGORY_COUNT - 1 : selected_ - 1;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    App::ctx.shopCategory = CATEGORIES[selected_].value;
    App::ctx.selectSlot = 0;
    Audio::play(Sfx::Select);
    App::goTo(ScreenId::CollectionGrid);
  }

  void onButtonC() override {
    selected_ = (selected_ + 1) % CATEGORY_COUNT;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "CollectionCategory"; }

private:
  uint8_t selected_ = 0;
};

CollectionCategoryScreen instance;
ScreenRegistrar registrar(ScreenId::CollectionCategory, instance);

}
