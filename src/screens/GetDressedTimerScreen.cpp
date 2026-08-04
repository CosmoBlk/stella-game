#include "App.h"
#include "Config.h"
#include "Content.h"
#include "Managers.h"
#include "Sprites.h"
#include "UI.h"

#include <stdio.h>

namespace {

enum class TimerState : uint8_t { Intro, Running, Question, TryAgain, Celebrate };

const TaskDef& getDressedTask() {
  for (int i = 0; i < TASK_COUNT; ++i) {
    if (TASKS[i].id == TASK_GET_DRESSED) return TASKS[i];
  }
  return TASKS[0];
}

void drawConfetti(uint32_t frameMs) {
  static const int baseX[] = {17, 45, 76, 108, 143, 177, 211, 244, 278, 306, 126, 231};
  static const uint16_t colours[] = {
    TFT_YELLOW, TFT_CYAN, TFT_MAGENTA, TFT_GREEN, TFT_ORANGE, TFT_WHITE
  };
  for (int i = 0; i < 12; ++i) {
    const int y = 28 + static_cast<int>((frameMs / 8 + i * 41) % 165);
    const int x = baseX[i] + static_cast<int>((frameMs / 130 + i * 2) % 7) - 3;
    UI::drawIcon(IconId::Star, x, y, 10 + (i % 3) * 3,
                 colours[i % 6], colours[(i + 1) % 6]);
  }
}

class GetDressedTimerScreen final : public Screen {
public:
  void enter() override {
    state_ = TimerState::Intro;
    elapsedMs_ = 0;
    stateMs_ = 0;
    totalMs_ = static_cast<uint32_t>(GET_DRESSED_SECONDS) * 1000UL;
    lastSecond_ = GET_DRESSED_SECONDS;
    paused_ = false;
    rewardedThisRun_ = false;
  }

  void update(uint32_t deltaMs) override {
    stateMs_ += deltaMs;

    if (state_ == TimerState::Running && !paused_) {
      elapsedMs_ += deltaMs;
      if (elapsedMs_ >= totalMs_) {
        elapsedMs_ = totalMs_;
        state_ = TimerState::Question;
        stateMs_ = 0;
        Audio::play(Sfx::TimerEnd);
        return;
      }

      const uint16_t remaining = remainingSeconds();
      if (remaining != lastSecond_) {
        lastSecond_ = remaining;
        if ((remaining > 5 && remaining % 10 == 0) || (remaining > 0 && remaining <= 5)) {
          Audio::play(Sfx::Tick);
        }
      }
    } else if (state_ == TimerState::TryAgain && stateMs_ >= 1500) {
      App::goTo(ScreenId::Tasks);
    } else if (state_ == TimerState::Celebrate && stateMs_ >= 2500) {
      App::goTo(ScreenId::Tasks);
    }
  }

  void draw() override {
    UI::drawBackground();
    switch (state_) {
      case TimerState::Intro: drawIntro(); break;
      case TimerState::Running: drawRunning(); break;
      case TimerState::Question: drawQuestion(); break;
      case TimerState::TryAgain: drawTryAgain(); break;
      case TimerState::Celebrate: drawCelebration(); break;
    }
  }

  void onButtonA() override {
    if (state_ == TimerState::Question) {
      state_ = TimerState::TryAgain;
      stateMs_ = 0;
      Audio::play(Sfx::GoodTry);
    }
  }

  void onButtonB() override {
    if (state_ == TimerState::Intro) {
      startCountdown(GET_DRESSED_SECONDS);
    } else if (state_ == TimerState::Running) {
      paused_ = !paused_;
      Audio::play(Sfx::Select);
    } else if (state_ == TimerState::Question) {
      finishDressing();
    } else if (state_ == TimerState::TryAgain || state_ == TimerState::Celebrate) {
      App::goTo(ScreenId::Tasks);
    }
  }

  void onButtonC() override {
    if (state_ == TimerState::Question) startCountdown(GET_DRESSED_EXTRA_SECONDS);
  }

  const char* name() const override { return "GetDressedTimer"; }

private:
  TimerState state_ = TimerState::Intro;
  uint32_t elapsedMs_ = 0;
  uint32_t totalMs_ = 0;
  uint32_t stateMs_ = 0;
  uint16_t lastSecond_ = 0;
  bool paused_ = false;
  bool rewardedThisRun_ = false;

  uint16_t remainingSeconds() const {
    if (elapsedMs_ >= totalMs_) return 0;
    return static_cast<uint16_t>((totalMs_ - elapsedMs_ + 999UL) / 1000UL);
  }

  void startCountdown(uint16_t seconds) {
    state_ = TimerState::Running;
    elapsedMs_ = 0;
    stateMs_ = 0;
    totalMs_ = static_cast<uint32_t>(seconds) * 1000UL;
    lastSecond_ = seconds;
    paused_ = false;
    Audio::play(Sfx::Select);
  }

  void finishDressing() {
    const TaskDef& task = getDressedTask();
    PlayerProfile& profile = App::profile();
    if (!profile.daily.completedTasks[TASK_GET_DRESSED]) {
      profile.daily.completedTasks[TASK_GET_DRESSED] = true;
      rewardedThisRun_ = true;
      App::ctx.lastReward = task.coins;
      App::awardCoins(task.coins);
      App::raiseEvent(GameEvent::GetDressedDone);
      App::raiseEvent(GameEvent::TaskCompleted);
      Save::saveNow();
    }
    state_ = TimerState::Celebrate;
    stateMs_ = 0;
    Audio::play(Sfx::Celebrate);
  }

  void drawIntro() {
    const Theme& theme = UI::theme();
    M5Canvas& canvas = Gfx::c();
    UI::drawHeader("GET DRESSED!");
    Sprites::drawHourglass(107, 103, 112, 0.0f);
    UI::drawIcon(IconId::Timer, 208, 66, 25, theme.accent);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(7);
    canvas.drawString("60", 218, 117);
    UI::drawButtonBar("", "START", "", IconId::None, IconId::Timer, IconId::None);
  }

  void drawRunning() {
    const Theme& theme = UI::theme();
    M5Canvas& canvas = Gfx::c();
    UI::drawHeader("GET DRESSED!");
    const float sandT = totalMs_ == 0 ? 1.0f :
      static_cast<float>(elapsedMs_) / static_cast<float>(totalMs_);
    Sprites::drawHourglass(66, 104, 82, sandT);

    char secondsText[8];
    snprintf(secondsText, sizeof(secondsText), "%u", remainingSeconds());
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(7);
    canvas.drawString(secondsText, 202, 103);
    UI::drawIcon(IconId::Timer, 138, 158, 20, theme.accent);
    canvas.setTextDatum(middle_left);
    canvas.setTextSize(2);
    canvas.drawString("SECONDS LEFT", 153, 159);

    if (paused_) {
      canvas.fillRoundRect(113, 70, 178, 66, 12, theme.panel);
      canvas.drawRoundRect(113, 70, 178, 66, 12, theme.accent);
      UI::drawIcon(IconId::Timer, 139, 103, 24, theme.accent);
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(theme.text);
      canvas.setTextSize(3);
      canvas.drawString("PAUSED", 158, 104);
    }

    UI::drawButtonBar("", paused_ ? "RESUME" : "PAUSE", "",
                      IconId::None, IconId::Timer, IconId::None);
  }

  void drawQuestion() {
    const Theme& theme = UI::theme();
    UI::drawHeader("ALL DRESSED?");
    UI::drawIcon(IconId::Shirt, 160, 77, 74, theme.accent, theme.accent2);
    UI::drawChoiceCards("NOT YET", "DONE!", "+30 SECONDS", 1);
    UI::drawButtonBar("NOT YET", "DONE!", "+30 SEC",
                      IconId::Back, IconId::Tick, IconId::Timer);
  }

  void drawTryAgain() {
    const Theme& theme = UI::theme();
    M5Canvas& canvas = Gfx::c();
    UI::drawHeader("NICE TRY!");
    UI::drawIcon(IconId::Heart, 160, 86, 80, theme.accent, theme.accent2);
    UI::drawIcon(IconId::Star, 76, 146, 24, TFT_YELLOW, TFT_ORANGE);
    canvas.setTextDatum(middle_left);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(3);
    canvas.drawString("TRY AGAIN SOON!", 94, 147);
    UI::drawButtonBar("", "OK", "", IconId::None, IconId::Heart, IconId::None);
  }

  void drawCelebration() {
    const PlayerProfile& profile = App::profile();
    const Theme& theme = UI::theme();
    M5Canvas& canvas = Gfx::c();
    drawConfetti(stateMs_);
    Sprites::drawCharacter(profile.inventory.equippedCharacter, 82, 137, 126,
                           Anim::Celebrating, stateMs_,
                           profile.inventory.equippedAccessory,
                           profile.inventory.equippedClothing);
    canvas.fillCircle(225, 78, 44, TFT_DARKGREEN);
    UI::drawTick(225, 78, 72);
    UI::drawIcon(IconId::Shirt, 166, 139, 26, theme.accent, theme.accent2);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(2);
    canvas.drawString("ALL DRESSED!", 236, 140);

    if (rewardedThisRun_) {
      char rewardText[24];
      snprintf(rewardText, sizeof(rewardText), "+%u COINS", getDressedTask().coins);
      UI::drawIcon(IconId::Coin, 166, 178, 26, TFT_YELLOW, TFT_ORANGE);
      canvas.setTextDatum(middle_center);
      canvas.setTextColor(TFT_YELLOW);
      canvas.setTextSize(2);
      canvas.drawString(rewardText, 236, 179);
    } else {
      UI::drawIcon(IconId::Tick, 166, 178, 24, TFT_GREEN);
      canvas.setTextDatum(middle_center);
      canvas.setTextColor(TFT_GREEN);
      canvas.setTextSize(2);
      canvas.drawString("DONE TODAY!", 236, 179);
    }
    UI::drawButtonBar("", "YAY!", "", IconId::None, IconId::Star, IconId::None);
  }
};

GetDressedTimerScreen instance;
ScreenRegistrar registrar(ScreenId::GetDressedTimer, instance);

}
