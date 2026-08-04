#include "App.h"
#include "Content.h"
#include "Managers.h"
#include "UI.h"

#include <stdio.h>
#include <string.h>

namespace {

class TasksScreen final : public Screen {
public:
  void enter() override {
    if (selected_ >= ACTIVE_TASK_COUNT) selected_ = 0;
  }

  void update(uint32_t) override {}

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("TASKS");

    drawTaskCard(selected_, 36, true);
    drawTaskCard((selected_ + 1) % ACTIVE_TASK_COUNT, 121, false);

    UI::drawButtonBar("PREV", "OPEN", "NEXT",
                      IconId::ArrowL, IconId::Tick, IconId::ArrowR);
  }

  void onButtonA() override {
    selected_ = (selected_ + ACTIVE_TASK_COUNT - 1) % ACTIVE_TASK_COUNT;
    Audio::play(Sfx::Select);
  }

  void onButtonB() override {
    App::ctx.taskId = ACTIVE_TASKS[selected_];
    Audio::play(Sfx::Select);
    App::goTo(ScreenId::TaskDetail);
  }

  void onButtonC() override {
    selected_ = (selected_ + 1) % ACTIVE_TASK_COUNT;
    Audio::play(Sfx::Select);
  }

  const char* name() const override { return "Tasks"; }

private:
  int selected_ = 0;

  void drawTaskCard(int index, int y, bool selected) {
    const TaskDef& task = TASKS[ACTIVE_TASKS[index]];
    const bool done = App::profile().daily.completedTasks[task.id];
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();
    const int x = selected ? 12 : 20;
    const int width = selected ? 296 : 280;
    const int height = 76;

    UI::drawPanel(x, y, width, height, theme.panel);
    canvas.drawRoundRect(x, y, width, height, 13,
                         selected ? theme.accent : theme.accent2);
    if (selected) {
      canvas.drawRoundRect(x + 2, y + 2, width - 4, height - 4, 11, theme.accent);
    }

    UI::drawIcon(static_cast<IconId>(task.iconKey), x + 38, y + 37, 44,
                 theme.accent, theme.accent2);

    canvas.setTextDatum(middle_left);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(strlen(task.label) <= 18 ? 2 : 1);
    canvas.drawString(task.label, x + 70, y + 25);

    char coinText[16];
    snprintf(coinText, sizeof(coinText), "+%u", task.coins);
    UI::drawIcon(IconId::Coin, x + 82, y + 54, 18, TFT_YELLOW, TFT_ORANGE);
    canvas.setTextDatum(middle_left);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_YELLOW);
    canvas.drawString(coinText, x + 96, y + 55);

    if (done) {
      canvas.fillCircle(x + width - 34, y + 37, 28, TFT_DARKGREEN);
      UI::drawTick(x + width - 34, y + 37, 46);
    }
    canvas.setTextDatum(top_left);
  }
};

TasksScreen instance;
ScreenRegistrar registrar(ScreenId::Tasks, instance);

}
