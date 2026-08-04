#include <cstdio>
#include <cstring>

#include "App.h"
#include "Content.h"
#include "Managers.h"
#include "Sprites.h"
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

constexpr uint8_t CELLS_PER_PAGE = 6;
constexpr int CELL_W = 100;
constexpr int CELL_H = 75;
constexpr uint16_t LOCKED_COLOUR = 0x18C3;

constexpr IconId FRANKIE_FIND_ICONS[12] = {
  IconId::Wand, IconId::Bone, IconId::Butterfly, IconId::Ball,
  IconId::Water, IconId::Flower, IconId::Feather, IconId::Leaf,
  IconId::JellyBean, IconId::Dino, IconId::Circle, IconId::Gift,
};

constexpr IconId DAD_BADGE_ICONS[6] = {
  IconId::Hat, IconId::Collection, IconId::Jump,
  IconId::Pencil, IconId::Paw, IconId::Star,
};

constexpr IconId GAME_BADGE_ICONS[8] = {
  IconId::Ball, IconId::Dino, IconId::Pencil, IconId::Star,
  IconId::Crown, IconId::Flower, IconId::Circle, IconId::Shark,
};

bool belongsToActivePlayer(const ItemDef& item) {
  if (item.owner == ItemOwner::Both) return true;
  return (App::player() == PlayerId::Hugo && item.owner == ItemOwner::HugoOnly) ||
         (App::player() == PlayerId::Stella && item.owner == ItemOwner::StellaOnly);
}

void drawCentred(const char* text, int cx, int y, int size, uint16_t colour) {
  M5Canvas& canvas = Gfx::c();
  canvas.setTextSize(size);
  canvas.setTextColor(colour);
  canvas.setCursor(cx - canvas.textWidth(text) / 2, y);
  canvas.print(text);
}

void drawEntryName(const char* name, int cx, int y, uint16_t colour) {
  M5Canvas& canvas = Gfx::c();
  canvas.setTextSize(1);
  canvas.setTextColor(colour);
  if (canvas.textWidth(name) <= CELL_W - 8) {
    drawCentred(name, cx, y + 4, 1, colour);
    return;
  }

  const size_t length = std::strlen(name);
  size_t split = length / 2;
  for (size_t distance = 0; distance < length / 2; ++distance) {
    if (split >= distance && name[split - distance] == ' ') {
      split -= distance;
      break;
    }
    if (split + distance < length && name[split + distance] == ' ') {
      split += distance;
      break;
    }
  }

  char first[24] = {};
  char second[24] = {};
  const size_t firstLength = split < sizeof(first) - 1 ? split : sizeof(first) - 1;
  std::memcpy(first, name, firstLength);
  const size_t secondStart = name[split] == ' ' ? split + 1 : split;
  std::snprintf(second, sizeof(second), "%s", name + secondStart);
  drawCentred(first, cx, y, 1, colour);
  drawCentred(second, cx, y + 10, 1, colour);
}

IconId categoryIcon(uint8_t category) {
  switch (category) {
    case COLLECTION_CHARACTERS: return IconId::Smiley;
    case COLLECTION_ACCESSORIES: return IconId::Hat;
    case COLLECTION_CLOTHES: return IconId::Shirt;
    case COLLECTION_TOYS: return IconId::Gift;
    case COLLECTION_ROOMS: return IconId::Home;
    case COLLECTION_DINOSAURS: return IconId::Dino;
    case COLLECTION_SHARKS: return IconId::Shark;
    case COLLECTION_JELLY_BEANS: return IconId::JellyBean;
    case COLLECTION_FRANKIE_FINDS: return IconId::Paw;
    case COLLECTION_DAD_BADGES: return IconId::Missions;
    case COLLECTION_GAME_BADGES: return IconId::Games;
    default: return IconId::Collection;
  }
}

uint16_t entryColour(uint8_t index) {
  constexpr uint16_t COLOURS[] = {
    TFT_CYAN, TFT_ORANGE, TFT_GREEN, TFT_MAGENTA,
    TFT_YELLOW, TFT_SKYBLUE, TFT_PURPLE, TFT_RED,
  };
  return COLOURS[index % (sizeof(COLOURS) / sizeof(COLOURS[0]))];
}

class CollectionGridScreen final : public Screen {
public:
  void enter() override {
    category_ = App::ctx.shopCategory;
    itemCount_ = 0;
    entryCount_ = 0;

    if (category_ <= COLLECTION_TREATS) {
      const ItemCategory category = static_cast<ItemCategory>(category_);
      for (int index = 0; index < ITEM_COUNT && itemCount_ < MAX_ITEMS; ++index) {
        if (ITEMS[index].category == category && belongsToActivePlayer(ITEMS[index])) {
          itemIds_[itemCount_++] = ITEMS[index].id;
        }
      }
      entryCount_ = itemCount_;
    } else {
      const uint8_t groupIndex = category_ - COLLECTION_COLLECTIBLE_BASE;
      if (groupIndex < static_cast<uint8_t>(CollectGroup::COUNT)) {
        group_ = static_cast<CollectGroup>(groupIndex);
        entryCount_ = COLLECT_COUNT[groupIndex];
      }
    }

    pageCount_ = entryCount_ == 0 ? 1 : (entryCount_ + CELLS_PER_PAGE - 1) / CELLS_PER_PAGE;
    page_ = App::ctx.selectSlot % pageCount_;
    sparkleMs_ = 0;
  }

  void update(uint32_t deltaMs) override {
    if (sparkleMs_ > deltaMs) sparkleMs_ -= deltaMs;
    else sparkleMs_ = 0;
  }

  void draw() override {
    const Theme& theme = UI::theme();
    M5Canvas& canvas = Gfx::c();
    UI::drawBackground();

    uint8_t found = 0;
    for (uint8_t index = 0; index < entryCount_; ++index) {
      if (isOwned(index)) ++found;
    }
    char header[20];
    std::snprintf(header, sizeof(header), "%u/%u FOUND", found, entryCount_);
    UI::drawHeader(header);

    if (entryCount_ == 0) {
      UI::drawBigCentred("NOTHING HERE YET", 105, 2, theme.text);
      UI::drawButtonBar("PREV", "", "NEXT",
                        IconId::ArrowL, IconId::None, IconId::ArrowR);
      return;
    }

    const uint8_t first = page_ * CELLS_PER_PAGE;
    for (uint8_t slot = 0; slot < CELLS_PER_PAGE; ++slot) {
      const uint8_t index = first + slot;
      if (index >= entryCount_) break;
      const int column = slot % 3;
      const int row = slot / 3;
      const int x = 4 + column * 104;
      const int y = 37 + row * 81;
      drawCell(index, x, y, theme);
    }

    char pageLabel[12];
    std::snprintf(pageLabel, sizeof(pageLabel), "%u/%u", page_ + 1, pageCount_);
    drawCentred(pageLabel, 160, 197, 1, theme.text);
    UI::drawButtonBar("PREV", "SPARKLE", "NEXT",
                      IconId::ArrowL, IconId::Sparkle, IconId::ArrowR);
  }

  void onButtonA() override { movePage(-1); }

  void onButtonB() override {
    const uint8_t first = page_ * CELLS_PER_PAGE;
    const uint8_t last = first + CELLS_PER_PAGE < entryCount_
      ? first + CELLS_PER_PAGE : entryCount_;
    for (uint8_t index = first; index < last; ++index) {
      if (isOwned(index)) {
        sparkleMs_ = 700;
        Audio::play(Sfx::Sparkle);
        return;
      }
    }
  }

  void onButtonC() override { movePage(1); }

  const char* name() const override { return "CollectionGrid"; }

private:
  bool isItemCategory() const { return category_ <= COLLECTION_TREATS; }

  bool isOwned(uint8_t index) const {
    if (isItemCategory()) return App::profile().inventory.owned(itemIds_[index]);
    return App::profile().hasCollectible(group_, index);
  }

  const char* entryName(uint8_t index) const {
    if (isItemCategory()) {
      const ItemDef* item = itemById(itemIds_[index]);
      return item ? item->name : "ITEM";
    }
    const char* name = collectibleName(group_, index);
    return name ? name : "FIND";
  }

  void drawCell(uint8_t index, int x, int y, const Theme& theme) const {
    M5Canvas& canvas = Gfx::c();
    const bool owned = isOwned(index);
    UI::drawPanel(x, y, CELL_W, CELL_H, theme.panel);
    canvas.drawRoundRect(x, y, CELL_W, CELL_H, 9, owned ? theme.accent : LOCKED_COLOUR);

    const int artX = x + CELL_W / 2;
    const int artY = y + 28;
    drawEntryArt(index, artX, artY, 43);

    if (!owned) {
      if (!isItemCategory() && group_ == CollectGroup::Dinosaur) {
        Sprites::drawDinoSilhouette(index, artX, artY, 43);
      } else {
        canvas.fillRoundRect(x + 8, y + 7, CELL_W - 16, 49, 8, LOCKED_COLOUR);
      }
      drawCentred("?", artX, y + 17, 3, TFT_WHITE);
      return;
    }

    drawEntryName(entryName(index), artX, y + 55, theme.text);
    if (sparkleMs_ > 0) drawSparkles(index, artX, artY);
  }

  void drawEntryArt(uint8_t index, int cx, int cy, int size) const {
    if (isItemCategory()) {
      Sprites::drawItemPreview(itemIds_[index], cx, cy, size, App::nowMs());
      return;
    }

    switch (group_) {
      case CollectGroup::Dinosaur:
        Sprites::drawDino(index, cx, cy, size, static_cast<uint16_t>(App::nowMs()));
        break;
      case CollectGroup::Shark:
        Sprites::drawShark(index, cx, cy, size, App::nowMs());
        break;
      case CollectGroup::JellyBean:
        Sprites::drawJellyBean(cx, cy, size, index);
        break;
      case CollectGroup::FrankieFind:
        UI::drawIcon(FRANKIE_FIND_ICONS[index], cx, cy, size,
                     entryColour(index), UI::theme().accent2);
        break;
      case CollectGroup::DadBadge:
        UI::drawIcon(DAD_BADGE_ICONS[index], cx, cy, size,
                     TFT_YELLOW, entryColour(index));
        break;
      case CollectGroup::GameBadge:
        UI::drawIcon(GAME_BADGE_ICONS[index], cx, cy, size,
                     entryColour(index), TFT_YELLOW);
        break;
      default:
        UI::drawIcon(categoryIcon(category_), cx, cy, size,
                     UI::theme().accent, UI::theme().accent2);
        break;
    }
  }

  void drawSparkles(uint8_t index, int cx, int cy) const {
    const uint32_t phase = (App::nowMs() / 90 + index) % 4;
    UI::drawIcon(IconId::Sparkle, cx - 30, cy - 19 + phase, 12, TFT_YELLOW);
    UI::drawIcon(IconId::Sparkle, cx + 31, cy - 13 - phase, 10, TFT_WHITE);
    UI::drawIcon(IconId::Sparkle, cx + 25, cy + 18, 8, UI::theme().accent);
  }

  void movePage(int direction) {
    page_ = static_cast<uint8_t>((page_ + pageCount_ + direction) % pageCount_);
    App::ctx.selectSlot = page_;
    sparkleMs_ = 0;
    Audio::play(Sfx::Select);
  }

  uint16_t itemIds_[MAX_ITEMS]{};
  uint8_t category_ = COLLECTION_CHARACTERS;
  CollectGroup group_ = CollectGroup::Dinosaur;
  uint8_t itemCount_ = 0;
  uint8_t entryCount_ = 0;
  uint8_t pageCount_ = 1;
  uint8_t page_ = 0;
  uint32_t sparkleMs_ = 0;
};

CollectionGridScreen instance;
ScreenRegistrar registrar(ScreenId::CollectionGrid, instance);

}
