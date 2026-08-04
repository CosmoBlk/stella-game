#include "Screen.h"
#include "../App.h"
#include "../Content.h"
#include "../Managers.h"
#include "../Sprites.h"
#include "../UI.h"

namespace {

enum class CardKind : uint8_t {
  Home,
  Game,
  Room,
  DressUp,
  JellyBeans,
  Sleep,
  Play,
  Feed,
  Menu,
  Task
};

struct CarouselCard {
  CardKind kind;
  uint8_t value;
};

struct GameCardDef {
  GameId gameId;
  ScreenId screenId;
  const char* label;
  IconId icon;
  uint16_t color;
};

constexpr GameCardDef GAMES[] = {
  {GameId::PenaltyKick, ScreenId::PenaltyKickGame, "PENALTY KICK", IconId::Ball, TFT_GREEN},
  {GameId::FindDinosaur, ScreenId::DinosaurGame, "FIND DINOSAUR", IconId::Dino, TFT_ORANGE},
  {GameId::HugoMaths, ScreenId::MathsGame, "HUGO MATHS", IconId::Pencil, TFT_CYAN},
  {GameId::RollerCoaster, ScreenId::RollerCoasterGame, "ROLLER COASTER", IconId::Star, TFT_YELLOW},
  {GameId::PrincessDance, ScreenId::PrincessDanceGame, "PRINCESS DANCE", IconId::Crown, TFT_MAGENTA},
  {GameId::ColourMatch, ScreenId::ColourMatchGame, "COLOUR MATCH", IconId::Flower, TFT_PURPLE},
  {GameId::BubblePop, ScreenId::BubblePopGame, "BUBBLE POP", IconId::Circle, TFT_SKYBLUE},
  {GameId::SharkSwim, ScreenId::SharkSwimGame, "SHARK SWIM", IconId::Shark, TFT_BLUE}
};

constexpr GameId HUGO_ORDER[] = {
  GameId::PenaltyKick, GameId::HugoMaths, GameId::FindDinosaur, GameId::SharkSwim,
  GameId::RollerCoaster, GameId::BubblePop, GameId::ColourMatch, GameId::PrincessDance
};

constexpr GameId STELLA_ORDER[] = {
  GameId::BubblePop, GameId::ColourMatch, GameId::PrincessDance, GameId::RollerCoaster,
  GameId::PenaltyKick, GameId::HugoMaths, GameId::FindDinosaur, GameId::SharkSwim
};

constexpr int MAX_CARDS = 1 + static_cast<int>(GameId::COUNT) + 7 + 5;
constexpr uint32_t SLIDE_MS = 150;
constexpr int CARD_SPACING = 100;

const GameCardDef* gameDef(GameId id) {
  for (const GameCardDef& game : GAMES) {
    if (game.gameId == id) return &game;
  }
  return nullptr;
}

const TaskDef* taskDef(uint8_t id) {
  for (int i = 0; i < TASK_COUNT; ++i) {
    if (TASKS[i].id == id) return &TASKS[i];
  }
  return nullptr;
}

class HomeScreen final : public Screen {
public:
  void enter() override {
    buildRing();
    selected_ = 0;
    slideMs_ = 0;
    slideDirection_ = 0;
    animMs_ = 0;
    celebrateMs_ = 0;
    rainbowMs_ = 0;
    hintMs_ = 0;
  }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    slideMs_ = slideMs_ > deltaMs ? slideMs_ - deltaMs : 0;
    celebrateMs_ = celebrateMs_ > deltaMs ? celebrateMs_ - deltaMs : 0;
    rainbowMs_ = rainbowMs_ > deltaMs ? rainbowMs_ - deltaMs : 0;
    hintMs_ = hintMs_ > deltaMs ? hintMs_ - deltaMs : 0;
  }

  void draw() override {
    const PlayerProfile& profile = App::profile();
    Sprites::drawRoom(profile.inventory.equippedRoom);
    UI::drawHeader(App::player() == PlayerId::Hugo ? "HUGO" : "STELLA");
    drawNeedBars(profile.buddy);
    Sprites::drawCharacter(profile.inventory.equippedCharacter, 160, 108, 94,
                           celebrateMs_ > 0 ? Anim::Celebrating : Buddy::moodAnim(),
                           animMs_, profile.inventory.equippedAccessory,
                           profile.inventory.equippedClothing);
    if (celebrateMs_ > 0) drawCelebrateFlash();
    if (rainbowMs_ > 0) drawRainbowFlash();
    if (hintMs_ > 0) drawShopHint();
    drawCarousel();
  }

  void onButtonA() override { move(-1); }
  void onButtonB() override {
    if (slideMs_ == 0 && count_ > 0) activate(cards_[selected_]);
  }
  void onButtonC() override { move(1); }
  void onButtonBLong() override { open(ScreenId::MainMenu); }
  bool allowGlobalBack() const override { return false; }
  const char* name() const override { return "HOME"; }

private:
  void buildRing() {
    count_ = 0;
    auto push = [this](CarouselCard card) {
      if (count_ < MAX_CARDS) cards_[count_++] = card;
    };
    push({CardKind::Home, 0});

    const GameId* order = App::player() == PlayerId::Hugo ? HUGO_ORDER : STELLA_ORDER;
    for (uint8_t i = 0; i < static_cast<uint8_t>(GameId::COUNT); ++i) {
      const GameCardDef* game = gameDef(order[i]);
      if (game != nullptr && App::screen(game->screenId) != nullptr) {
        push({CardKind::Game, static_cast<uint8_t>(game->gameId)});
      }
    }

    push({CardKind::Room, 0});
    push({CardKind::DressUp, 0});
    push({CardKind::JellyBeans, 0});
    push({CardKind::Sleep, 0});
    push({CardKind::Play, 0});
    push({CardKind::Feed, 0});
    push({CardKind::Menu, 0});

    for (int i = ACTIVE_TASK_COUNT - 1; i >= 0; --i) {
      push({CardKind::Task, ACTIVE_TASKS[i]});
    }
  }

  int wrappedIndex(int index) const {
    while (index < 0) index += count_;
    while (index >= count_) index -= count_;
    return index;
  }

  void move(int direction) {
    if (count_ == 0 || slideMs_ > 0) return;
    selected_ = wrappedIndex(selected_ + direction);
    slideDirection_ = direction;
    slideMs_ = SLIDE_MS;
    Audio::play(Sfx::Select);
  }

  void activate(const CarouselCard& card) {
    switch (card.kind) {
      case CardKind::Home:
        open(ScreenId::Buddy);
        break;
      case CardKind::Game: {
        const GameCardDef* game = gameDef(static_cast<GameId>(card.value));
        if (game != nullptr) {
          App::ctx.gameId = card.value;
          open(game->screenId);
        }
        break;
      }
      case CardKind::Room:
        open(ScreenId::RoomSelect);
        break;
      case CardKind::DressUp:
        open(ScreenId::CharacterSelect);
        break;
      case CardKind::JellyBeans:
        if (App::profile().buddy.jellyBeans > 0) {
          Buddy::feedJellyBeans();
          rainbowMs_ = 1350;
          celebrateMs_ = 950;
          animMs_ = 0;
        } else {
          hintMs_ = 2200;
          Audio::play(Sfx::GoodTry);
        }
        break;
      case CardKind::Sleep:
        open(ScreenId::Sleep);
        break;
      case CardKind::Play:
        Buddy::play();
        celebrateMs_ = 1050;
        animMs_ = 0;
        break;
      case CardKind::Feed:
        open(ScreenId::Feed);
        break;
      case CardKind::Menu:
        open(ScreenId::MainMenu);
        break;
      case CardKind::Task:
        App::ctx.taskId = card.value;
        if (card.value == TASK_GET_DRESSED) open(ScreenId::GetDressedTimer);
        else if (card.value == 9) open(ScreenId::FrankieWalkIntro);
        else open(ScreenId::TaskDetail);
        break;
    }
  }

  void drawNeedBars(const BuddyState& buddy) const {
    M5Canvas& canvas = Gfx::c();
    canvas.fillRoundRect(3, 32, 314, 19, 7, 0x18C3);
    UI::drawNeedBar(5, 35, IconId::Apple, buddy.hunger, 0xF9A6);
    UI::drawNeedBar(108, 35, IconId::Smiley, buddy.happiness, UI::theme().accent);
    UI::drawNeedBar(211, 35, IconId::Moon, buddy.energy, 0x7DFF);
  }

  void drawCarousel() const {
    M5Canvas& canvas = Gfx::c();
    canvas.fillRoundRect(0, 162, 320, 78, 12, 0x18C3);

    int offset = 0;
    if (slideMs_ > 0) {
      offset = (slideDirection_ * CARD_SPACING * static_cast<int>(slideMs_)) /
               static_cast<int>(SLIDE_MS);
    }

    int highlightRel = 0;
    int closest = 1000;
    for (int rel = -2; rel <= 2; ++rel) {
      const int position = rel * CARD_SPACING + offset;
      const int distance = position < 0 ? -position : position;
      if (distance < closest) {
        closest = distance;
        highlightRel = rel;
      }
    }

    for (int rel = -2; rel <= 2; ++rel) {
      if (rel == highlightRel) continue;
      drawCard(cards_[wrappedIndex(selected_ + rel)], 160 + rel * CARD_SPACING + offset, false);
    }
    drawCard(cards_[wrappedIndex(selected_ + highlightRel)],
             160 + highlightRel * CARD_SPACING + offset, true);
  }

  void drawCard(const CarouselCard& card, int centerX, bool highlighted) const {
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();
    const int width = highlighted ? 104 : 68;
    const int height = highlighted ? 70 : 54;
    const int x = centerX - width / 2;
    const int y = 165 + (70 - height) / 2;
    const uint16_t fill = highlighted ? theme.panel : 0x2945;
    canvas.fillRoundRect(x, y, width, height, highlighted ? 12 : 9, fill);
    canvas.drawRoundRect(x, y, width, height, highlighted ? 12 : 9,
                         highlighted ? theme.accent : 0x7BEF);
    if (highlighted) canvas.drawRoundRect(x + 2, y + 2, width - 4, height - 4, 10, theme.accent2);

    const int iconY = highlighted ? 188 : 198;
    drawCardIcon(card, centerX, iconY, highlighted ? 36 : 27);

    if (highlighted) {
      canvas.setTextDatum(middle_center);
      canvas.setTextColor(theme.text);
      canvas.setTextSize(1);
      canvas.drawString(cardLabel(card), centerX, 225);
      canvas.setTextDatum(top_left);
    }
  }

  void drawCardIcon(const CarouselCard& card, int centerX, int centerY, int size) const {
    const Theme& theme = UI::theme();
    if (card.kind == CardKind::Home) {
      const InventoryState& inventory = App::profile().inventory;
      Sprites::drawCharacter(inventory.equippedCharacter, centerX, centerY + 5, size + 8,
                             Anim::Happy, animMs_, inventory.equippedAccessory,
                             inventory.equippedClothing);
      return;
    }
    if (card.kind == CardKind::Game) {
      const GameCardDef* game = gameDef(static_cast<GameId>(card.value));
      if (game != nullptr) UI::drawIcon(game->icon, centerX, centerY, size, game->color, theme.accent2);
      return;
    }
    if (card.kind == CardKind::Task) {
      const TaskDef* task = taskDef(card.value);
      if (task != nullptr) {
        UI::drawIcon(static_cast<IconId>(task->iconKey), centerX, centerY, size,
                     theme.accent, theme.accent2);
        if (App::profile().daily.completedTasks[card.value]) {
          Gfx::c().fillCircle(centerX + size / 3, centerY - size / 3, size / 3, TFT_DARKGREEN);
          UI::drawTick(centerX + size / 3, centerY - size / 3, size / 2);
        }
      }
      return;
    }

    IconId icon = IconId::Heart;
    switch (card.kind) {
      case CardKind::Room: icon = IconId::Home; break;
      case CardKind::DressUp: icon = IconId::Shirt; break;
      case CardKind::JellyBeans: icon = IconId::JellyBean; break;
      case CardKind::Sleep: icon = IconId::Moon; break;
      case CardKind::Play: icon = IconId::Smiley; break;
      case CardKind::Feed: icon = IconId::Apple; break;
      case CardKind::Menu: icon = IconId::Games; break;
      default: break;
    }
    UI::drawIcon(icon, centerX, centerY, size, theme.accent, theme.accent2);
  }

  const char* cardLabel(const CarouselCard& card) const {
    if (card.kind == CardKind::Game) {
      const GameCardDef* game = gameDef(static_cast<GameId>(card.value));
      return game != nullptr ? game->label : "GAME";
    }
    if (card.kind == CardKind::Task) {
      const TaskDef* task = taskDef(card.value);
      return task != nullptr ? task->label : "TASK";
    }
    switch (card.kind) {
      case CardKind::Home: return "HOME";
      case CardKind::Room: return "ROOM";
      case CardKind::DressUp: return "DRESS UP";
      case CardKind::JellyBeans: return "JELLY BEANS";
      case CardKind::Sleep: return "SLEEP";
      case CardKind::Play: return "PLAY";
      case CardKind::Feed: return "FEED";
      case CardKind::Menu: return "MENU";
      default: return "";
    }
  }

  void drawCelebrateFlash() const {
    M5Canvas& canvas = Gfx::c();
    const uint16_t color = ((celebrateMs_ / 90) & 1) ? TFT_YELLOW : UI::theme().accent2;
    canvas.drawRoundRect(5, 54, 310, 104, 18, color);
    canvas.drawRoundRect(8, 57, 304, 98, 16, color);
    UI::drawIcon(IconId::Sparkle, 76, 80, 20, color);
    UI::drawIcon(IconId::Sparkle, 244, 93, 24, color);
  }

  void drawRainbowFlash() const {
    static constexpr int X[] = {75, 96, 126, 160, 194, 224, 246};
    static constexpr int Y[] = {119, 78, 62, 72, 59, 83, 124};
    static constexpr uint16_t COLORS[] = {
      TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE, TFT_MAGENTA
    };
    M5Canvas& canvas = Gfx::c();
    const int phase = static_cast<int>((rainbowMs_ / 80) % 7);
    for (int i = 0; i < 7; ++i) {
      const uint16_t color = COLORS[(i + phase) % 7];
      canvas.fillCircle(X[i], Y[i], 3, color);
      canvas.drawLine(X[i] - 5, Y[i], X[i] + 5, Y[i], color);
      canvas.drawLine(X[i], Y[i] - 5, X[i], Y[i] + 5, color);
    }
  }

  void drawShopHint() const {
    M5Canvas& canvas = Gfx::c();
    canvas.fillRoundRect(75, 130, 170, 25, 8, TFT_WHITE);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(0x39E7);
    canvas.setTextSize(1);
    canvas.drawString("BUY AT THE SHOP", 160, 143);
    canvas.setTextDatum(top_left);
  }

  void open(ScreenId target) {
    if (!App::screen(target)) {
      Serial.println("[warn] screen missing");
      return;
    }
    Audio::play(Sfx::Select);
    App::goTo(target);
  }

  CarouselCard cards_[MAX_CARDS] = {};
  int count_ = 0;
  int selected_ = 0;
  int slideDirection_ = 0;
  uint32_t slideMs_ = 0;
  uint32_t animMs_ = 0;
  uint32_t celebrateMs_ = 0;
  uint32_t rainbowMs_ = 0;
  uint32_t hintMs_ = 0;
};

HomeScreen instance;
ScreenRegistrar registrar(ScreenId::Home, instance);
}
