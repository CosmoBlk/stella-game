#include <Arduino.h>
#include <M5Unified.h>
#include <stdio.h>
#include "../App.h"
#include "../Managers.h"
#include "../UI.h"
#include "../Sprites.h"

namespace {

constexpr uint32_t ROUND_MS = 30000;
constexpr int BUBBLE_X[3] = {53, 160, 267};
constexpr uint16_t BUBBLE_COLORS[] = {
  TFT_PINK, TFT_CYAN, TFT_YELLOW, TFT_GREEN, TFT_MAGENTA, TFT_ORANGE, TFT_SKYBLUE
};

struct Bubble {
  int16_t y;
  uint16_t color;
  uint16_t riseMs;
  uint16_t burstMs;
  uint8_t bonus;
};

class BubblePopGame final : public Screen {
public:
  void enter() override { startRound(); }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    if (finished_) {
      UI::tickCoinAnim(deltaMs);
      return;
    }

    elapsedMs_ += deltaMs;
    for (Bubble& bubble : bubbles_) {
      if (bubble.riseMs > 0) {
        bubble.riseMs = deltaMs >= bubble.riseMs ? 0 : bubble.riseMs - deltaMs;
        bubble.y = 103 + static_cast<int16_t>(bubble.riseMs / 12);
      } else {
        bubble.y = 103 + static_cast<int16_t>((animMs_ / 180 + bubble.color) % 5) - 2;
      }
      if (bubble.burstMs > 0) {
        bubble.burstMs = deltaMs >= bubble.burstMs ? 0 : bubble.burstMs - deltaMs;
      }
    }

    if (elapsedMs_ >= ROUND_MS) finishRound();
  }

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("BUBBLE POP");
    M5Canvas& canvas = Gfx::c();

    if (finished_) {
      drawResult(canvas);
      UI::tickCoinAnim(0);
      return;
    }

    drawSunTimer(canvas);
    for (uint8_t i = 0; i < 3; ++i) drawBubble(canvas, i);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(2);
    char score[16];
    snprintf(score, sizeof(score), "POPS %u", pops_);
    canvas.drawString(score, 160, 176);
    UI::drawButtonBar("POP", "POP", "POP", IconId::Circle, IconId::Circle, IconId::Circle);
  }

  void onButtonA() override { pop(0); }
  void onButtonB() override {
    if (finished_) startRound();
    else pop(1);
  }
  void onButtonC() override { pop(2); }

  const char* name() const override { return "BubblePopGame"; }

private:
  Bubble bubbles_[3] = {};
  uint32_t elapsedMs_ = 0;
  uint32_t animMs_ = 0;
  uint16_t pops_ = 0;
  uint8_t coins_ = 0;
  bool finished_ = false;

  void startRound() {
    elapsedMs_ = 0;
    animMs_ = 0;
    pops_ = 0;
    coins_ = 0;
    finished_ = false;
    App::ctx.gameId = static_cast<uint8_t>(GameId::BubblePop);
    for (uint8_t i = 0; i < 3; ++i) replaceBubble(i, i * 120);
  }

  void replaceBubble(uint8_t index, uint16_t extraRise = 0) {
    Bubble& bubble = bubbles_[index];
    bubble.color = BUBBLE_COLORS[random(sizeof(BUBBLE_COLORS) / sizeof(BUBBLE_COLORS[0]))];
    bubble.riseMs = 360 + extraRise;
    bubble.y = 133;
    bubble.burstMs = 0;
    bubble.bonus = 0;
  }

  void pop(uint8_t index) {
    if (finished_) return;
    ++pops_;
    Audio::play(Sfx::Pop);
    Bubble& bubble = bubbles_[index];
    bubble.burstMs = 280;
    bubble.bonus = random(100) < 25 ? static_cast<uint8_t>(1 + random(5)) : 0;
    bubble.riseMs = 420;
    bubble.y = 138;
    bubble.color = BUBBLE_COLORS[random(sizeof(BUBBLE_COLORS) / sizeof(BUBBLE_COLORS[0]))];
  }

  void finishRound() {
    finished_ = true;
    coins_ = pops_ / 5;
    if (coins_ < 1) coins_ = 1;
    if (coins_ > 8) coins_ = 8;
    PlayerProfile& profile = App::profile();
    const uint8_t gameIndex = static_cast<uint8_t>(GameId::BubblePop);
    if (pops_ > profile.gameHighScores[gameIndex]) profile.gameHighScores[gameIndex] = pops_;
    App::ctx.lastReward = coins_;
    App::awardCoins(coins_);
    App::raiseEvent(GameEvent::BubblePopped, 0, pops_ > 255 ? 255 : static_cast<uint8_t>(pops_));
    App::raiseEvent(GameEvent::GamePlayed, gameIndex);
    Save::requestSave();
    Audio::play(Sfx::Celebrate);
    UI::startCoinAnim(coins_);
  }

  void drawSunTimer(M5Canvas& canvas) {
    const float remaining = 1.0f - static_cast<float>(elapsedMs_) / ROUND_MS;
    const int radius = 8 + static_cast<int>(remaining * 10.0f);
    const int cx = 292;
    const int cy = 48;
    canvas.fillCircle(cx, cy, radius, TFT_YELLOW);
    for (int i = 0; i < 8; ++i) {
      const float angle = i * 0.785398f;
      canvas.drawLine(cx + cosf(angle) * (radius + 2), cy + sinf(angle) * (radius + 2),
                      cx + cosf(angle) * (radius + 7), cy + sinf(angle) * (radius + 7), TFT_ORANGE);
    }
    canvas.fillCircle(cx - 5, cy - 2, 1, TFT_BLACK);
    canvas.fillCircle(cx + 5, cy - 2, 1, TFT_BLACK);
    canvas.drawLine(cx - 4, cy + 5, cx + 4, cy + 5, TFT_BLACK);
  }

  void drawBubble(M5Canvas& canvas, uint8_t index) {
    const Bubble& bubble = bubbles_[index];
    const int x = BUBBLE_X[index];
    canvas.fillCircle(x, bubble.y, 34, bubble.color);
    canvas.drawCircle(x, bubble.y, 34, TFT_WHITE);
    canvas.drawCircle(x, bubble.y, 30, TFT_WHITE);
    canvas.fillCircle(x - 11, bubble.y - 12, 7, TFT_WHITE);
    canvas.fillCircle(x - 8, bubble.y - 9, 4, bubble.color);
    if (bubble.burstMs > 0) {
      const int spread = 12 + (280 - bubble.burstMs) / 8;
      for (uint8_t ray = 0; ray < 8; ++ray) {
        const float angle = ray * 0.785398f;
        canvas.fillCircle(x + cosf(angle) * spread, 103 + sinf(angle) * spread, 3, bubble.color);
      }
      drawBonus(index, x, 103);
    }
  }

  void drawBonus(uint8_t index, int x, int y) {
    const uint8_t bonus = bubbles_[index].bonus;
    if (bonus == 0) return;
    if (bonus == 1) UI::drawIcon(IconId::Star, x, y, 28, TFT_YELLOW);
    else if (bonus == 2) UI::drawIcon(IconId::Butterfly, x, y, 28, TFT_MAGENTA, TFT_CYAN);
    else if (bonus == 3) UI::drawIcon(IconId::Coin, x, y, 28, TFT_YELLOW, TFT_ORANGE);
    else if (bonus == 4) Sprites::drawJellyBean(x, y, 28, random(10));
    else Sprites::drawCharacter(App::profile().inventory.equippedCharacter, x, y, 30,
                                Anim::Celebrating, animMs_);
  }

  void drawResult(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(2);
    canvas.drawString("YOU POPPED", 160, 68);
    canvas.setTextSize(5);
    char count[12];
    snprintf(count, sizeof(count), "%u!", pops_);
    canvas.drawString(count, 160, 113);
    canvas.setTextSize(2);
    char reward[20];
    snprintf(reward, sizeof(reward), "+%u COINS", coins_);
    canvas.drawString(reward, 160, 164);
    for (uint8_t i = 0; i < 8; ++i) {
      UI::drawIcon(IconId::Sparkle, 35 + i * 36, 42 + (i % 2) * 18, 12, TFT_YELLOW);
    }
    UI::drawButtonBar("", "REPLAY", "", IconId::None, IconId::Games, IconId::None);
  }
};

BubblePopGame instance;
ScreenRegistrar registrar(ScreenId::BubblePopGame, instance);

}
