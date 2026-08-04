#include <M5Unified.h>
#include <stdio.h>
#include "../App.h"
#include "../Managers.h"
#include "../UI.h"

namespace {

struct GameEntry {
  GameId gameId;
  ScreenId screenId;
  const char* name;
  IconId icon;
  uint16_t color;
};

constexpr GameEntry ALL_GAMES[] = {
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

const GameEntry* entryFor(GameId id) {
  for (const GameEntry& entry : ALL_GAMES) {
    if (entry.gameId == id) return &entry;
  }
  return nullptr;
}

class GamesMenuScreen final : public Screen {
public:
  void enter() override {
    count_ = 0;
    selected_ = 0;
    const GameId* order = App::player() == PlayerId::Hugo ? HUGO_ORDER : STELLA_ORDER;
    for (uint8_t i = 0; i < static_cast<uint8_t>(GameId::COUNT); ++i) {
      const GameEntry* entry = entryFor(order[i]);
      if (entry != nullptr && App::screen(entry->screenId) != nullptr) {
        available_[count_++] = entry;
      }
    }
  }

  void update(uint32_t) override {}

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("GAMES");
    M5Canvas& canvas = Gfx::c();

    if (count_ == 0) {
      UI::drawBigCentred("MORE GAMES SOON!", 104, 2, UI::theme().text);
      UI::drawButtonBar("", "", "");
      return;
    }

    const GameEntry& game = *available_[selected_];
    UI::drawPanel(47, 40, 226, 151, UI::theme().panel);
    canvas.drawRoundRect(47, 40, 226, 151, 18, game.color);
    canvas.drawRoundRect(49, 42, 222, 147, 16, game.color);
    UI::drawIcon(game.icon, 160, 101, 72, game.color, UI::theme().accent2);
    UI::drawBigCentred(game.name, 153, 2, UI::theme().text);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(1);
    char page[12];
    snprintf(page, sizeof(page), "%u / %u", selected_ + 1, count_);
    canvas.drawString(page, 160, 181);
    UI::drawButtonBar("PREV", "PLAY", "NEXT", IconId::ArrowL, IconId::Games, IconId::ArrowR);
  }

  void onButtonA() override {
    if (count_ == 0) return;
    selected_ = selected_ == 0 ? count_ - 1 : selected_ - 1;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    if (count_ == 0) return;
    const GameEntry& game = *available_[selected_];
    App::ctx.gameId = static_cast<uint8_t>(game.gameId);
    Audio::play(Sfx::Select);
    App::goTo(game.screenId);
  }

  void onButtonC() override {
    if (count_ == 0) return;
    selected_ = (selected_ + 1) % count_;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "GamesMenu"; }

private:
  const GameEntry* available_[static_cast<uint8_t>(GameId::COUNT)] = {};
  uint8_t count_ = 0;
  uint8_t selected_ = 0;
};

GamesMenuScreen instance;
ScreenRegistrar registrar(ScreenId::GamesMenu, instance);

}
