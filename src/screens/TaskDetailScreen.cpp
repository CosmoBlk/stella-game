#include "App.h"
#include "Config.h"
#include "Content.h"
#include "Managers.h"
#include "UI.h"

#include <stdio.h>
#include <string.h>

namespace {

const TaskDef& activeTask() {
  for (int i = 0; i < TASK_COUNT; ++i) {
    if (TASKS[i].id == App::ctx.taskId) return TASKS[i];
  }
  return TASKS[0];
}

class TaskDetailScreen final : public Screen {
public:
  void enter() override {
    lastTickStep_ = 0;
    completed_ = false;
  }

  void update(uint32_t) override {
    const TaskDef& task = activeTask();
    if (completed_ || task.id == TASK_GET_DRESSED || isDone(task)) return;

    const uint32_t held = Input::heldMsB();
    if (held == 0) {
      lastTickStep_ = 0;
      return;
    }

    uint8_t tickStep = 0;
    if (held >= 600) tickStep = 1;
    if (held >= 1100) tickStep = 2;
    if (held >= 1500) tickStep = 3;
    if (held >= 1800) tickStep = 4;
    if (held >= TASK_HOLD_MS) tickStep = 5;
    if (tickStep > lastTickStep_) {
      lastTickStep_ = tickStep;
      Audio::play(Sfx::Tick);
    }

    if (held >= TASK_HOLD_MS) completeTask(task);
  }

  void draw() override {
    const TaskDef& task = activeTask();
    const bool done = isDone(task);
    const Theme& theme = UI::theme();
    M5Canvas& canvas = Gfx::c();

    UI::drawBackground();
    UI::drawHeader("TASK");
    UI::drawIcon(static_cast<IconId>(task.iconKey), 160, 82, 92,
                 theme.accent, theme.accent2);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(strlen(task.label) <= 18 ? 2 : 1);
    canvas.drawString(task.label, 160, 139);

    char rewardText[24];
    snprintf(rewardText, sizeof(rewardText), "+%u COINS", task.coins);
    UI::drawIcon(IconId::Coin, 105, 166, 20, TFT_YELLOW, TFT_ORANGE);
    canvas.setTextDatum(middle_left);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_YELLOW);
    canvas.drawString(rewardText, 119, 167);

    if (done) {
      canvas.fillCircle(258, 82, 34, TFT_DARKGREEN);
      UI::drawTick(258, 82, 54);
      UI::drawIcon(IconId::Tick, 75, 190, 20, TFT_GREEN);
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(TFT_GREEN);
      canvas.setTextSize(2);
      canvas.drawString("DONE TODAY!", 90, 190);
    } else if (task.id == TASK_GET_DRESSED) {
      UI::drawIcon(IconId::Timer, 91, 190, 20, theme.accent);
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(theme.text);
      canvas.setTextSize(2);
      canvas.drawString("OPEN TIMER", 106, 190);
    } else {
      UI::drawIcon(IconId::Tick, 63, 190, 20, theme.accent);
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(theme.text);
      canvas.setTextSize(2);
      canvas.drawString("HOLD B WHEN DONE", 78, 190);
      const float progress = static_cast<float>(Input::heldMsB()) /
                             static_cast<float>(TASK_HOLD_MS);
      UI::drawHoldProgress(progress > 1.0f ? 1.0f : progress);
    }

    UI::drawButtonBar("BACK", done ? "DONE" : (task.id == TASK_GET_DRESSED ? "TIMER" : "HOLD"), "",
                      IconId::Back,
                      done ? IconId::Tick : (task.id == TASK_GET_DRESSED ? IconId::Timer : IconId::Tick),
                      IconId::None);
  }

  void onButtonA() override {
    Audio::play(Sfx::Back);
    App::goTo(ScreenId::Tasks);
  }

  void onButtonB() override {
    const TaskDef& task = activeTask();
    if (!isDone(task) && task.id == TASK_GET_DRESSED) {
      Audio::play(Sfx::Select);
      App::goTo(ScreenId::GetDressedTimer);
    }
  }

  bool allowGlobalBack() const override { return Input::heldMsB() == 0; }
  const char* name() const override { return "TaskDetail"; }

private:
  uint8_t lastTickStep_ = 0;
  bool completed_ = false;

  bool isDone(const TaskDef& task) const {
    return App::profile().daily.completedTasks[task.id];
  }

  void completeTask(const TaskDef& task) {
    completed_ = true;
    PlayerProfile& profile = App::profile();
    profile.daily.completedTasks[task.id] = true;
    App::ctx.lastReward = task.coins;
    App::awardCoins(task.coins);
    App::raiseEvent(GameEvent::TaskCompleted);
    Save::saveNow();
    App::goTo(ScreenId::TaskComplete);
  }
};

TaskDetailScreen instance;
ScreenRegistrar registrar(ScreenId::TaskDetail, instance);

}
