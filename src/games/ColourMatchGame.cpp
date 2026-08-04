#include <Arduino.h>
#include <M5Unified.h>
#include <stdio.h>
#include "../App.h"
#include "../Managers.h"
#include "../UI.h"

namespace {

enum class ColourId : uint8_t { Pink, Blue, Yellow, Purple, Green, Red, Orange, White, Count };
enum class ObjectId : uint8_t { Crown, Butterfly, Star, Dress, Frog, Apple, Fish, Snowflake };

struct ColourDef {
  const char* name;
  uint16_t value;
};

constexpr ColourDef COLOURS[] = {
  {"PINK", TFT_PINK}, {"BLUE", TFT_BLUE}, {"YELLOW", TFT_YELLOW}, {"PURPLE", TFT_PURPLE},
  {"GREEN", TFT_GREEN}, {"RED", TFT_RED}, {"ORANGE", TFT_ORANGE}, {"WHITE", TFT_WHITE}
};

constexpr ObjectId OBJECTS[] = {
  ObjectId::Crown, ObjectId::Butterfly, ObjectId::Star, ObjectId::Dress,
  ObjectId::Frog, ObjectId::Apple, ObjectId::Fish, ObjectId::Snowflake
};

struct Card {
  ObjectId object;
  ColourId colour;
};

class ColourMatchGame final : public Screen {
public:
  void enter() override { startRound(); }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    if (finished_) {
      UI::tickCoinAnim(deltaMs);
      return;
    }
    if (feedbackMs_ == 0) return;
    feedbackMs_ = deltaMs >= feedbackMs_ ? 0 : feedbackMs_ - deltaMs;
    if (feedbackMs_ == 0 && correctFeedback_) {
      ++question_;
      if (question_ >= 5) finishRound();
      else makeQuestion();
    }
  }

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("COLOUR MATCH");
    M5Canvas& canvas = Gfx::c();
    if (finished_) {
      drawResult(canvas);
      UI::tickCoinAnim(0);
      return;
    }

    const ColourDef& target = COLOURS[static_cast<uint8_t>(target_)];
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(2);
    char prompt[24];
    snprintf(prompt, sizeof(prompt), "FIND %s!", target.name);
    canvas.drawString(prompt, 160, 47);
    canvas.fillCircle(160, 72, 17, target.value);
    canvas.drawCircle(160, 72, 18, target.value == TFT_WHITE ? TFT_DARKGREY : TFT_WHITE);

    for (uint8_t i = 0; i < 3; ++i) drawCard(canvas, i);
    char progress[12];
    snprintf(progress, sizeof(progress), "%u / 5", question_ + 1);
    canvas.setTextSize(1);
    canvas.setTextColor(UI::theme().text);
    canvas.drawString(progress, 160, 190);
    UI::drawButtonBar("PICK", "PICK", "PICK");
  }

  void onButtonA() override { choose(0); }
  void onButtonB() override {
    if (finished_) startRound();
    else choose(1);
  }
  void onButtonC() override { choose(2); }

  const char* name() const override { return "ColourMatchGame"; }

private:
  Card cards_[3] = {};
  ColourId target_ = ColourId::Pink;
  uint32_t animMs_ = 0;
  uint16_t feedbackMs_ = 0;
  uint8_t question_ = 0;
  uint8_t correct_ = 0;
  uint8_t correctIndex_ = 0;
  uint8_t chosenIndex_ = 0;
  uint8_t coins_ = 0;
  bool correctFeedback_ = false;
  bool finished_ = false;

  void startRound() {
    animMs_ = 0;
    feedbackMs_ = 0;
    question_ = 0;
    correct_ = 0;
    coins_ = 0;
    finished_ = false;
    App::ctx.gameId = static_cast<uint8_t>(GameId::ColourMatch);
    makeQuestion();
  }

  void makeQuestion() {
    feedbackMs_ = 0;
    correctFeedback_ = false;
    target_ = static_cast<ColourId>(random(static_cast<uint8_t>(ColourId::Count)));
    correctIndex_ = random(3);
    uint8_t usedObjects = 0;
    for (uint8_t i = 0; i < 3; ++i) {
      uint8_t objectIndex;
      do objectIndex = random(8); while (usedObjects & (1 << objectIndex));
      usedObjects |= 1 << objectIndex;
      cards_[i].object = OBJECTS[objectIndex];
      if (i == correctIndex_) {
        cards_[i].colour = target_;
      } else {
        ColourId colour;
        do colour = static_cast<ColourId>(random(static_cast<uint8_t>(ColourId::Count)));
        while (colour == target_ || (i == 2 && colour == cards_[0].colour));
        cards_[i].colour = colour;
      }
    }
  }

  void choose(uint8_t index) {
    if (finished_ || feedbackMs_ > 0) return;
    chosenIndex_ = index;
    if (index == correctIndex_) {
      ++correct_;
      correctFeedback_ = true;
      feedbackMs_ = 700;
      Audio::play(Sfx::Sparkle);
    } else {
      correctFeedback_ = false;
      feedbackMs_ = 520;
      Audio::play(Sfx::GoodTry);
    }
  }

  void finishRound() {
    finished_ = true;
    coins_ = 1 + correct_;
    if (coins_ > 6) coins_ = 6;
    PlayerProfile& profile = App::profile();
    const uint8_t gameIndex = static_cast<uint8_t>(GameId::ColourMatch);
    if (correct_ > profile.gameHighScores[gameIndex]) profile.gameHighScores[gameIndex] = correct_;
    App::ctx.lastReward = coins_;
    App::awardCoins(coins_);
    App::raiseEvent(GameEvent::ColourFound, 0, correct_);
    App::raiseEvent(GameEvent::GamePlayed, gameIndex);
    Save::requestSave();
    Audio::play(Sfx::Celebrate);
    UI::startCoinAnim(coins_);
  }

  void drawCard(M5Canvas& canvas, uint8_t index) {
    const int x = 8 + index * 104;
    int wobble = 0;
    if (feedbackMs_ > 0 && !correctFeedback_ && chosenIndex_ == index) {
      wobble = ((feedbackMs_ / 55) % 2 == 0) ? -4 : 4;
    }
    const int y = 96 + wobble;
    UI::drawPanel(x, y, 96, 81, UI::theme().panel);
    uint16_t border = UI::theme().accent;
    if (feedbackMs_ > 0 && correctFeedback_ && chosenIndex_ == index) border = TFT_GREEN;
    canvas.drawRoundRect(x, y, 96, 81, 12, border);
    drawObject(cards_[index].object, x + 48, y + 39, 51,
               COLOURS[static_cast<uint8_t>(cards_[index].colour)].value);
    if (feedbackMs_ > 0 && correctFeedback_ && chosenIndex_ == index) {
      UI::drawIcon(IconId::Sparkle, x + 78, y + 16, 18, TFT_YELLOW);
      UI::drawTick(x + 18, y + 16, 17);
    }
  }

  void drawObject(ObjectId object, int cx, int cy, int size, uint16_t colour) {
    switch (object) {
      case ObjectId::Crown: UI::drawIcon(IconId::Crown, cx, cy, size, colour, TFT_YELLOW); break;
      case ObjectId::Butterfly: UI::drawIcon(IconId::Butterfly, cx, cy, size, colour, TFT_WHITE); break;
      case ObjectId::Star: UI::drawIcon(IconId::Star, cx, cy, size, colour); break;
      case ObjectId::Dress: UI::drawIcon(IconId::Shirt, cx, cy, size, colour, TFT_WHITE); break;
      case ObjectId::Frog: drawFrog(cx, cy, size, colour); break;
      case ObjectId::Apple: UI::drawIcon(IconId::Apple, cx, cy, size, colour, TFT_GREEN); break;
      case ObjectId::Fish: drawFish(cx, cy, size, colour); break;
      case ObjectId::Snowflake: drawSnowflake(cx, cy, size, colour); break;
    }
  }

  void drawFrog(int cx, int cy, int size, uint16_t colour) {
    M5Canvas& canvas = Gfx::c();
    const int r = size / 3;
    canvas.fillCircle(cx, cy + 3, r, colour);
    canvas.fillCircle(cx - r + 5, cy - r + 7, r / 2, colour);
    canvas.fillCircle(cx + r - 5, cy - r + 7, r / 2, colour);
    canvas.fillCircle(cx - r + 5, cy - r + 7, 3, TFT_WHITE);
    canvas.fillCircle(cx + r - 5, cy - r + 7, 3, TFT_WHITE);
    canvas.drawLine(cx - 8, cy + 11, cx + 8, cy + 11, TFT_BLACK);
  }

  void drawFish(int cx, int cy, int size, uint16_t colour) {
    M5Canvas& canvas = Gfx::c();
    const int r = size / 3;
    canvas.fillCircle(cx - 3, cy, r, colour);
    canvas.fillTriangle(cx + r - 4, cy, cx + r + 14, cy - 13, cx + r + 14, cy + 13, colour);
    canvas.fillCircle(cx - 10, cy - 4, 3, TFT_WHITE);
    canvas.fillCircle(cx - 10, cy - 4, 1, TFT_BLACK);
  }

  void drawSnowflake(int cx, int cy, int size, uint16_t colour) {
    M5Canvas& canvas = Gfx::c();
    const int r = size / 2;
    canvas.drawLine(cx - r, cy, cx + r, cy, colour);
    canvas.drawLine(cx, cy - r, cx, cy + r, colour);
    canvas.drawLine(cx - r * 7 / 10, cy - r * 7 / 10, cx + r * 7 / 10, cy + r * 7 / 10, colour);
    canvas.drawLine(cx + r * 7 / 10, cy - r * 7 / 10, cx - r * 7 / 10, cy + r * 7 / 10, colour);
    canvas.fillCircle(cx, cy, 4, colour);
  }

  void drawResult(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(3);
    canvas.drawString("GREAT MATCHING!", 160, 62);
    for (uint8_t i = 0; i < 5; ++i) {
      UI::drawIcon(IconId::Star, 64 + i * 48, 112, 28, i < correct_ ? TFT_YELLOW : TFT_DARKGREY);
    }
    canvas.setTextSize(2);
    char reward[20];
    snprintf(reward, sizeof(reward), "+%u COINS", coins_);
    canvas.drawString(reward, 160, 163);
    UI::drawButtonBar("", "REPLAY", "", IconId::None, IconId::Games, IconId::None);
  }
};

ColourMatchGame instance;
ScreenRegistrar registrar(ScreenId::ColourMatchGame, instance);

}
