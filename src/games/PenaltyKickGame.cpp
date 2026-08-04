#include <Arduino.h>
#include <M5Unified.h>
#include <stdio.h>
#include "../App.h"
#include "../Managers.h"
#include "../UI.h"
#include "../Sprites.h"

namespace {

constexpr uint32_t COUNT_STEP_MS = 700;
constexpr uint32_t COUNTDOWN_MS = COUNT_STEP_MS * 3;
constexpr int ZONE_X[3] = {82, 160, 238};

enum class KickState : uint8_t { Countdown, Flight, Reaction, Result };
enum class KickOutcome : uint8_t { Goal, Save, Post, Wide };

class PenaltyKickGame final : public Screen {
public:
  void enter() override { startRound(); }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    if (state_ == KickState::Result) {
      UI::tickCoinAnim(deltaMs);
      return;
    }

    stateMs_ += deltaMs;
    if (state_ == KickState::Countdown) {
      const uint8_t step = stateMs_ / COUNT_STEP_MS;
      if (step > countdownStep_ && step < 3) {
        countdownStep_ = step;
        Audio::play(Sfx::Countdown);
      }
      if (stateMs_ >= COUNTDOWN_MS) beginKick();
    } else if (state_ == KickState::Flight) {
      if (!kickPlayed_ && stateMs_ >= 100) {
        kickPlayed_ = true;
        Audio::play(Sfx::Kick);
      }
      if (stateMs_ >= 520) beginReaction();
    }
    else if (state_ == KickState::Reaction) {
      if (outcome_ == KickOutcome::Goal && !cheerPlayed_ && stateMs_ >= 300) {
        cheerPlayed_ = true;
        Audio::play(Sfx::Cheer);
      }
      if (stateMs_ >= 1150) nextKick();
    }
  }

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("PENALTY KICK");
    M5Canvas& canvas = Gfx::c();
    if (state_ == KickState::Result) {
      drawResult(canvas);
      UI::tickCoinAnim(0);
      return;
    }

    drawGoal(canvas);
    drawScore(canvas);
    drawZones(canvas);
    drawPlayersAndBall(canvas);
    drawCountdownOrReaction(canvas);
    UI::drawButtonBar("LEFT", "CENTRE", "RIGHT", IconId::ArrowL, IconId::Ball, IconId::ArrowR);
  }

  void onButtonA() override { selectZone(0); }
  void onButtonB() override {
    if (state_ == KickState::Result) startRound();
    else selectZone(1);
  }
  void onButtonC() override { selectZone(2); }

  const char* name() const override { return "PenaltyKickGame"; }

private:
  KickState state_ = KickState::Countdown;
  KickOutcome outcome_ = KickOutcome::Goal;
  uint32_t stateMs_ = 0;
  uint32_t animMs_ = 0;
  uint32_t lastSelectionMs_ = 0;
  int8_t selected_ = -1;
  int8_t shotDir_ = 1;
  int8_t goalieDir_ = 1;
  uint8_t kick_ = 0;
  uint8_t goals_ = 0;
  uint8_t coins_ = 0;
  bool powerful_ = false;
  bool cheerPlayed_ = false;
  bool kickPlayed_ = false;
  uint8_t countdownStep_ = 0;

  void startRound() {
    App::ctx.gameId = static_cast<uint8_t>(GameId::PenaltyKick);
    animMs_ = 0;
    kick_ = 0;
    goals_ = 0;
    coins_ = 0;
    startCountdown();
  }

  void startCountdown() {
    state_ = KickState::Countdown;
    stateMs_ = 0;
    selected_ = -1;
    lastSelectionMs_ = 0;
    powerful_ = false;
    cheerPlayed_ = false;
    kickPlayed_ = false;
    countdownStep_ = 0;
    Audio::play(Sfx::Countdown);
  }

  void selectZone(int8_t zone) {
    if (state_ != KickState::Countdown) return;
    selected_ = zone;
    lastSelectionMs_ = stateMs_;
    Audio::play(Sfx::Select);
  }

  void beginKick() {
    state_ = KickState::Flight;
    stateMs_ = 0;
    shotDir_ = selected_ < 0 ? 1 : selected_;
    powerful_ = lastSelectionMs_ > 0 && COUNTDOWN_MS - lastSelectionMs_ <= 350;
    const uint8_t goalieRoll = random(100);
    goalieDir_ = goalieRoll < 32 ? 0 : (goalieRoll < 68 ? 1 : 2);

    const uint8_t outcomeRoll = random(100);
    if (shotDir_ != goalieDir_) {
      if (outcomeRoll < 92) outcome_ = KickOutcome::Goal;
      else if (outcomeRoll < 97) outcome_ = KickOutcome::Post;
      else outcome_ = KickOutcome::Wide;
    } else {
      const uint8_t goalChance = powerful_ ? 60 : 30;
      outcome_ = outcomeRoll < goalChance ? KickOutcome::Goal : KickOutcome::Save;
    }
    Audio::play(Sfx::CountdownGo);
  }

  void beginReaction() {
    state_ = KickState::Reaction;
    stateMs_ = 0;
    if (outcome_ == KickOutcome::Goal) {
      ++goals_;
      Audio::play(Sfx::Goal);
    } else if (outcome_ == KickOutcome::Save) {
      Audio::play(Sfx::GoalieSave);
    } else if (outcome_ == KickOutcome::Post) {
      Audio::play(Sfx::Kick);
    } else {
      Audio::play(Sfx::GoodTry);
    }
  }

  void nextKick() {
    ++kick_;
    if (kick_ >= 5) finishRound();
    else startCountdown();
  }

  void finishRound() {
    state_ = KickState::Result;
    stateMs_ = 0;
    if (goals_ <= 1) coins_ = 1;
    else if (goals_ == 2) coins_ = 2;
    else if (goals_ == 3) coins_ = 4;
    else if (goals_ == 4) coins_ = 6;
    else coins_ = 8;

    PlayerProfile& profile = App::profile();
    const uint8_t gameIndex = static_cast<uint8_t>(GameId::PenaltyKick);
    if (goals_ > profile.gameHighScores[gameIndex]) profile.gameHighScores[gameIndex] = goals_;
    App::ctx.lastReward = coins_;
    App::awardCoins(coins_);
    App::raiseEvent(GameEvent::PenaltyGoal, 0, goals_);
    App::raiseEvent(GameEvent::GamePlayed, gameIndex);
    Save::requestSave();
    Audio::play(Sfx::Celebrate);
    UI::startCoinAnim(coins_);
  }

  void drawGoal(M5Canvas& canvas) {
    const int left = 34;
    const int right = 286;
    const int top = 48;
    const int bottom = 133;
    canvas.fillRect(left, top, right - left, bottom - top, TFT_DARKGREEN);
    for (int x = left + 14; x < right; x += 18) canvas.drawLine(x, top, x, bottom, TFT_LIGHTGREY);
    for (int y = top + 12; y < bottom; y += 14) canvas.drawLine(left, y, right, y, TFT_LIGHTGREY);
    if (state_ != KickState::Countdown && outcome_ == KickOutcome::Goal) {
      const int ripple = 2 + static_cast<int>((animMs_ / 80) % 5);
      for (int y = top + 8; y < bottom; y += 16) canvas.drawLine(left + ripple, y, right - ripple, y + ripple, TFT_WHITE);
    }
    canvas.fillRect(left - 5, top - 4, 8, bottom - top + 8, TFT_WHITE);
    canvas.fillRect(right - 3, top - 4, 8, bottom - top + 8, TFT_WHITE);
    canvas.fillRect(left - 5, top - 4, right - left + 10, 8, TFT_WHITE);
  }

  void drawZones(M5Canvas& canvas) {
    for (uint8_t i = 0; i < 3; ++i) {
      uint16_t color = selected_ == i && state_ == KickState::Countdown ? TFT_YELLOW : TFT_CYAN;
      int radius = selected_ == i && state_ == KickState::Countdown ? 18 : 14;
      canvas.drawCircle(ZONE_X[i], 77, radius, color);
      canvas.drawCircle(ZONE_X[i], 77, radius - 3, color);
    }
  }

  void drawPlayersAndBall(M5Canvas& canvas) {
    float flightT = 0.0f;
    if (state_ == KickState::Flight) flightT = stateMs_ / 520.0f;
    else if (state_ == KickState::Reaction) flightT = 1.0f;

    int diveDirection = 0;
    float diveT = 0.0f;
    if (state_ != KickState::Countdown) {
      diveDirection = goalieDir_ - 1;
      diveT = flightT;
    }
    Sprites::drawGoalie(160, 109, 54, diveDirection, diveT);

    const PlayerProfile& hugo = App::profileOf(PlayerId::Hugo);
    Sprites::drawCharacter(hugo.inventory.equippedCharacter, 160, 178, 49,
                           state_ == KickState::Countdown ? Anim::Idle : Anim::Kicking, animMs_);

    int ballX = 160;
    int ballY = 166;
    if (flightT > 0.0f) {
      int targetX = ZONE_X[shotDir_];
      int targetY = 78;
      if (outcome_ == KickOutcome::Post) targetX = shotDir_ == 2 ? 281 : 39;
      if (outcome_ == KickOutcome::Wide) {
        targetX = shotDir_ == 0 ? 15 : (shotDir_ == 2 ? 305 : 300);
        targetY = 66;
      }
      if (outcome_ == KickOutcome::Save && flightT > 0.72f) flightT = 0.72f;
      ballX = 160 + static_cast<int>((targetX - 160) * flightT);
      ballY = 166 + static_cast<int>((targetY - 166) * flightT);
      if (outcome_ == KickOutcome::Post && state_ == KickState::Reaction) {
        ballX += shotDir_ == 0 ? 18 : -18;
        ballY += stateMs_ / 18;
      }
    }
    Sprites::drawSoccerBall(ballX, ballY, powerful_ ? 10 : 9, animMs_);

    if (state_ == KickState::Reaction && outcome_ == KickOutcome::Save) {
      const int handX = 160 + (goalieDir_ - 1) * 31;
      canvas.drawLine(handX, 91, handX + ((stateMs_ / 100) % 2 ? 8 : -8), 79, TFT_WHITE);
    }
  }

  void drawCountdownOrReaction(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    if (state_ == KickState::Countdown) {
      const uint8_t number = 3 - stateMs_ / COUNT_STEP_MS;
      canvas.setTextColor(TFT_YELLOW);
      canvas.setTextSize(4);
      char text[4];
      snprintf(text, sizeof(text), "%u", number);
      canvas.drawString(text, 160, 144);
    } else if (state_ == KickState::Flight && stateMs_ < 180) {
      canvas.setTextColor(TFT_YELLOW);
      canvas.setTextSize(4);
      canvas.drawString("0", 160, 144);
    } else if (state_ == KickState::Reaction) {
      canvas.setTextColor(outcome_ == KickOutcome::Goal ? TFT_YELLOW : TFT_WHITE);
      canvas.setTextSize(3);
      canvas.drawString(outcome_ == KickOutcome::Goal ? "GOAL!" : "SO CLOSE!", 160, 145);
      if (powerful_) {
        canvas.setTextSize(1);
        canvas.drawString("POWER SHOT!", 160, 164);
      }
      if (outcome_ == KickOutcome::Goal) drawCrowd(canvas);
    }
  }

  void drawCrowd(M5Canvas& canvas) {
    const int jump = (stateMs_ / 100) % 2 ? 3 : 0;
    const uint16_t colors[] = {TFT_RED, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_WHITE};
    for (uint8_t i = 0; i < 18; ++i) {
      canvas.fillCircle(13 + i * 17, 37 - ((i + jump) % 2) * 3, 3, colors[i % 5]);
    }
  }

  void drawScore(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(2);
    char score[12];
    snprintf(score, sizeof(score), "%u/5", goals_);
    canvas.drawString(score, 298, 151);
    for (uint8_t i = 0; i < 5; ++i) {
      Sprites::drawSoccerBall(278 + (i % 3) * 15, 171 + (i / 3) * 16, 5,
                              i < kick_ ? 0 : animMs_);
    }
  }

  void drawResult(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(3);
    canvas.drawString("GREAT KICKS!", 160, 58);
    canvas.setTextSize(5);
    char score[12];
    snprintf(score, sizeof(score), "%u / 5", goals_);
    canvas.drawString(score, 160, 111);
    canvas.setTextSize(2);
    char reward[20];
    snprintf(reward, sizeof(reward), "+%u COINS", coins_);
    canvas.drawString(reward, 160, 166);
    UI::drawButtonBar("", "REPLAY", "", IconId::None, IconId::Ball, IconId::None);
  }
};

PenaltyKickGame instance;
ScreenRegistrar registrar(ScreenId::PenaltyKickGame, instance);

}
