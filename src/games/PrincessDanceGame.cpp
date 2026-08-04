#include <Arduino.h>
#include <M5Unified.h>
#include <math.h>
#include <stdio.h>
#include "../App.h"
#include "../Content.h"
#include "../Managers.h"
#include "../Sprites.h"
#include "../UI.h"

namespace {

enum class DanceSymbol : uint8_t { PinkStar, BlueSnowflake, PurpleButterfly, YellowCrown, RedHeart, Count };
enum class DanceState : uint8_t { Demo, YourTurn, Input, MoveBurst, SillyWiggle, FoundReveal, Result };

struct SymbolDef {
  const char* name;
  IconId icon;
  uint16_t color;
  uint16_t color2;
};

constexpr SymbolDef SYMBOLS[] = {
  {"PINK STAR", IconId::Star, TFT_PINK, TFT_WHITE},
  {"BLUE SNOWFLAKE", IconId::Sparkle, TFT_CYAN, TFT_WHITE},
  {"PURPLE BUTTERFLY", IconId::Butterfly, TFT_PURPLE, TFT_PINK},
  {"YELLOW CROWN", IconId::Crown, TFT_YELLOW, TFT_ORANGE},
  {"RED HEART", IconId::Heart, TFT_RED, TFT_PINK}
};

class PrincessDanceGame final : public Screen {
public:
  void enter() override { startRound(); }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    if (state_ == DanceState::Result) {
      UI::tickCoinAnim(deltaMs);
      return;
    }

    stateMs_ += deltaMs;
    switch (state_) {
      case DanceState::Demo:
        if (stateMs_ >= 720) {
          stateMs_ = 0;
          ++demoIndex_;
          if (demoIndex_ >= sequenceLength_) state_ = DanceState::YourTurn;
        }
        break;
      case DanceState::YourTurn:
        if (stateMs_ >= 650) {
          state_ = DanceState::Input;
          stateMs_ = 0;
        }
        break;
      case DanceState::MoveBurst:
        if (stateMs_ >= 430) {
          stateMs_ = 0;
          if (inputIndex_ >= sequenceLength_) {
            ++correct_;
            ++prompt_;
            if (prompt_ >= 8) finishRound();
            else makePrompt();
          } else {
            state_ = DanceState::Input;
          }
        }
        break;
      case DanceState::SillyWiggle:
        if (stateMs_ >= 720) restartPrompt();
        break;
      case DanceState::FoundReveal:
        if (stateMs_ >= 2400) {
          state_ = DanceState::Result;
          stateMs_ = 0;
          UI::startCoinAnim(coins_);
        }
        break;
      case DanceState::Input:
      case DanceState::Result:
        break;
    }
  }

  void draw() override {
    drawStage();
    UI::drawHeader("PRINCESS DANCE");
    M5Canvas& canvas = Gfx::c();
    if (state_ == DanceState::FoundReveal) {
      drawFoundReveal(canvas);
      return;
    }
    if (state_ == DanceState::Result) {
      drawResult(canvas);
      UI::tickCoinAnim(0);
      return;
    }

    drawPrompt(canvas);
    drawDancer();
    for (uint8_t index = 0; index < 3; ++index) drawSymbolCard(canvas, index);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(1);
    char progress[12];
    snprintf(progress, sizeof(progress), "%u / 8", prompt_ + 1);
    canvas.drawString(progress, 160, 190);
    UI::drawButtonBar("DANCE", "DANCE", "DANCE", IconId::Note, IconId::Note, IconId::Note);
  }

  void onButtonA() override { press(0); }
  void onButtonB() override {
    if (state_ == DanceState::Result) startRound();
    else press(1);
  }
  void onButtonC() override { press(2); }

  const char* name() const override { return "PrincessDanceGame"; }

private:
  DanceState state_ = DanceState::Demo;
  DanceSymbol cards_[3] = {};
  DanceSymbol sequence_[3] = {};
  uint8_t sequenceLength_ = 1;
  uint8_t demoIndex_ = 0;
  uint8_t inputIndex_ = 0;
  uint8_t prompt_ = 0;
  uint8_t correct_ = 0;
  uint8_t coins_ = 0;
  uint8_t moveIndex_ = 0;
  uint16_t dancerId_ = ITEM_STELLA_DEFAULT_PRINCESS;
  uint16_t foundAccessory_ = ITEM_NONE;
  uint32_t animMs_ = 0;
  uint32_t stateMs_ = 0;

  void startRound() {
    state_ = DanceState::Demo;
    prompt_ = 0;
    correct_ = 0;
    coins_ = 0;
    moveIndex_ = 0;
    foundAccessory_ = ITEM_NONE;
    animMs_ = 0;
    stateMs_ = 0;
    App::ctx.gameId = static_cast<uint8_t>(GameId::PrincessDance);
    App::ctx.lastCollectible = 0xFF;

    const uint16_t equipped = App::profileOf(PlayerId::Stella).inventory.equippedCharacter;
    dancerId_ = equipped >= 1 && equipped <= 6 ? equipped : ITEM_STELLA_DEFAULT_PRINCESS;
    makePrompt();
  }

  void makePrompt() {
    sequenceLength_ = correct_ < 3 ? 1 : (correct_ < 6 ? 2 : 3);
    chooseCards();
    for (uint8_t index = 0; index < sequenceLength_; ++index) sequence_[index] = cards_[random(3)];
    restartPrompt();
  }

  void chooseCards() {
    uint8_t pool[5] = {0, 1, 2, 3, 4};
    for (int index = 4; index > 0; --index) {
      const int swapIndex = random(index + 1);
      const uint8_t value = pool[index];
      pool[index] = pool[swapIndex];
      pool[swapIndex] = value;
    }
    for (uint8_t index = 0; index < 3; ++index) cards_[index] = static_cast<DanceSymbol>(pool[index]);
  }

  void restartPrompt() {
    state_ = DanceState::Demo;
    stateMs_ = 0;
    demoIndex_ = 0;
    inputIndex_ = 0;
  }

  void press(uint8_t cardIndex) {
    if (state_ != DanceState::Input || cardIndex >= 3) return;
    if (cards_[cardIndex] == sequence_[inputIndex_]) {
      ++inputIndex_;
      ++moveIndex_;
      state_ = DanceState::MoveBurst;
      stateMs_ = 0;
      Audio::play(Sfx::MusicCue);
    } else {
      state_ = DanceState::SillyWiggle;
      stateMs_ = 0;
      Audio::play(Sfx::GoodTry);
    }
  }

  void finishRound() {
    coins_ = 1 + correct_ / 2;
    if (coins_ > 5) coins_ = 5;
    PlayerProfile& profile = App::profile();
    const uint8_t gameIndex = static_cast<uint8_t>(GameId::PrincessDance);
    if (correct_ > profile.gameHighScores[gameIndex]) profile.gameHighScores[gameIndex] = correct_;
    App::ctx.lastReward = coins_;
    App::awardCoins(coins_);
    App::raiseEvent(GameEvent::DanceDone, 0, 1);
    App::raiseEvent(GameEvent::GamePlayed, gameIndex);

    foundAccessory_ = chooseAccessory();
    if (foundAccessory_ != ITEM_NONE) {
      App::profileOf(PlayerId::Stella).inventory.setOwned(foundAccessory_);
      Save::saveNow();
      state_ = DanceState::FoundReveal;
      stateMs_ = 0;
      Audio::play(Sfx::Discover);
    } else {
      Save::requestSave();
      state_ = DanceState::Result;
      Audio::play(Sfx::Celebrate);
      UI::startCoinAnim(coins_);
    }
  }

  uint16_t chooseAccessory() const {
    if (random(100) >= 15) return ITEM_NONE;
    const InventoryState& inventory = App::profileOf(PlayerId::Stella).inventory;
    uint16_t unowned[15];
    uint8_t count = 0;
    for (uint16_t id = 12; id <= 26; ++id) {
      if (!inventory.owned(id)) unowned[count++] = id;
    }
    return count == 0 ? ITEM_NONE : unowned[random(count)];
  }

  void drawStage() {
    M5Canvas& canvas = Gfx::c();
    canvas.fillScreen(TFT_BLACK);
    for (int y = 0; y < SCREEN_H; ++y) {
      const uint8_t red = 16 + y * 18 / SCREEN_H;
      const uint8_t blue = 35 + y * 45 / SCREEN_H;
      canvas.drawFastHLine(0, y, SCREEN_W, canvas.color565(red, 7, blue));
    }
    canvas.fillTriangle(112, 24, 208, 24, 270, 186, canvas.color565(54, 42, 80));
    canvas.fillEllipse(160, 167, 105, 23, canvas.color565(94, 45, 108));
    canvas.drawEllipse(160, 167, 105, 23, TFT_PINK);
    for (uint8_t light = 0; light < 5; ++light) {
      canvas.fillCircle(45 + light * 58, 29, 5, light % 2 ? TFT_CYAN : TFT_PINK);
    }
  }

  void drawPrompt(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(TFT_WHITE);
    if (state_ == DanceState::Demo) {
      canvas.setTextSize(1);
      canvas.drawString("WATCH!", 160, 39);
      const DanceSymbol symbol = sequence_[demoIndex_ < sequenceLength_ ? demoIndex_ : sequenceLength_ - 1];
      const SymbolDef& def = SYMBOLS[static_cast<uint8_t>(symbol)];
      UI::drawIcon(def.icon, 160, 66, 43, def.color, def.color2);
      canvas.setTextSize(1);
      canvas.drawString(def.name, 160, 91);
    } else if (state_ == DanceState::YourTurn) {
      canvas.setTextSize(3);
      canvas.setTextColor(TFT_YELLOW);
      canvas.drawString("YOUR TURN!", 160, 64);
    } else if (state_ == DanceState::SillyWiggle) {
      canvas.setTextSize(2);
      canvas.setTextColor(TFT_PINK);
      canvas.drawString("SILLY WIGGLE!", 160, 62);
    } else {
      canvas.setTextSize(2);
      canvas.drawString("COPY THE DANCE!", 160, 57);
      for (uint8_t index = 0; index < sequenceLength_; ++index) {
        const uint16_t color = index < inputIndex_ ? TFT_YELLOW : TFT_DARKGREY;
        UI::drawIcon(IconId::Star, 143 + index * 18 - sequenceLength_ * 9, 78, 13, color);
      }
    }
  }

  void drawDancer() {
    const InventoryState& inventory = App::profileOf(PlayerId::Stella).inventory;
    int x = 160;
    int y = 137;
    int size = 76;

    if (state_ == DanceState::MoveBurst) {
      switch (moveIndex_ % 5) {
        case 0: x -= 12; y -= 7; break;
        case 1: x += 12; y -= 4; break;
        case 2: y -= 15; size += 8; break;
        case 3: x += static_cast<int>(sinf(animMs_ * 0.035f) * 16.0f); break;
        default: size += static_cast<int>(sinf(animMs_ * 0.03f) * 7.0f); break;
      }
    } else if (state_ == DanceState::SillyWiggle) {
      x += static_cast<int>(sinf(animMs_ * 0.055f) * 17.0f);
      y += ((animMs_ / 90) % 2) ? 4 : -3;
    }

    Sprites::drawCharacter(dancerId_, x, y, size, Anim::Dancing, animMs_,
                           inventory.equippedAccessory, inventory.equippedClothing);

    if (state_ == DanceState::MoveBurst) {
      for (uint8_t sparkle = 0; sparkle < 6; ++sparkle) {
        UI::drawIcon(IconId::Sparkle, 112 + sparkle * 19, 105 + (sparkle % 2) * 49,
                     12, sparkle % 2 ? TFT_CYAN : TFT_YELLOW);
      }
    }
  }

  void drawSymbolCard(M5Canvas& canvas, uint8_t index) {
    const int x = 8 + index * 104;
    const int y = 143;
    const SymbolDef& def = SYMBOLS[static_cast<uint8_t>(cards_[index])];
    UI::drawPanel(x, y, 96, 42, canvas.color565(38, 20, 55));
    canvas.drawRoundRect(x, y, 96, 42, 10, def.color);
    UI::drawIcon(def.icon, x + 48, y + 20, 31, def.color, def.color2);
  }

  void drawFoundReveal(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(TFT_YELLOW);
    canvas.setTextSize(4);
    canvas.drawString("FOUND!", 160, 52);
    const ItemDef* item = itemById(foundAccessory_);
    const int bounce = static_cast<int>(sinf(animMs_ * 0.012f) * 7.0f);
    Sprites::drawItemPreview(foundAccessory_, 160, 121 + bounce, 103, animMs_);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(2);
    canvas.drawString(item ? item->name : "PRINCESS SURPRISE", 160, 181);
    for (uint8_t sparkle = 0; sparkle < 8; ++sparkle) {
      UI::drawIcon(IconId::Sparkle, 30 + sparkle * 38, 78 + (sparkle % 2) * 70,
                   15, sparkle % 2 ? TFT_PINK : TFT_CYAN);
    }
    canvas.setTextDatum(top_left);
  }

  void drawResult(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(3);
    canvas.drawString("BEAUTIFUL", 160, 51);
    canvas.drawString("DANCING!", 160, 79);
    for (uint8_t index = 0; index < 8; ++index) {
      UI::drawIcon(IconId::Star, 48 + index * 32, 119, 22,
                   index < correct_ ? TFT_YELLOW : TFT_DARKGREY);
    }
    canvas.setTextSize(2);
    char reward[20];
    snprintf(reward, sizeof(reward), "+%u COINS", coins_);
    canvas.drawString(reward, 160, 164);
    UI::drawButtonBar("", "REPLAY", "", IconId::None, IconId::Note, IconId::None);
  }
};

PrincessDanceGame instance;
ScreenRegistrar registrar(ScreenId::PrincessDanceGame, instance);

}
