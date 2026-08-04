#include <Arduino.h>
#include <M5Unified.h>
#include <stdio.h>
#include <string.h>
#include "../App.h"
#include "../Managers.h"
#include "../UI.h"

namespace {

enum class MathsCategory : uint8_t { Add, Sub, Mul, Div, Seq, Cmp, Frac, Missing, Count };
enum class VisualKind : uint8_t { None, Dots, Groups, Pie };

struct Question {
  char prompt[40];
  char answers[3][12];
  int16_t values[3];
  int16_t correctValue;
  uint8_t correctIndex;
  MathsCategory category;
  VisualKind visual;
  uint8_t visualA;
  uint8_t visualB;
};

class MathsGame final : public Screen {
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
    if (feedbackMs_ == 0 && advanceAfterFeedback_) advanceQuestion();
  }

  void draw() override {
    UI::drawBackground();
    UI::drawHeader("HUGO MATHS");
    M5Canvas& canvas = Gfx::c();
    if (finished_) {
      drawResult(canvas);
      UI::tickCoinAnim(0);
      return;
    }

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(1);
    char levelText[16];
    snprintf(levelText, sizeof(levelText), "LEVEL %u", level_);
    canvas.drawString(levelText, 43, 38);
    char progress[12];
    snprintf(progress, sizeof(progress), "%u / 5", questionNumber_ + 1);
    canvas.drawString(progress, 278, 38);

    canvas.setTextSize(promptTextSize());
    canvas.drawString(question_.prompt, 160, 61);
    drawQuestionVisual(canvas, false);
    if (revealSolution_) drawSolutionHint(canvas);
    drawAnswerCards(canvas);

    if (feedbackMs_ > 0) {
      canvas.setTextSize(2);
      if (showGoodTry_) {
        canvas.setTextColor(TFT_YELLOW);
        canvas.drawString("GOOD TRY!", 160, 94);
      } else if (showCorrect_) {
        canvas.setTextColor(TFT_GREEN);
        canvas.drawString("GREAT!", 160, 94);
      }
    }
    UI::drawButtonBar("A", "B", "C");
  }

  void onButtonA() override { choose(0); }
  void onButtonB() override {
    if (finished_) startRound();
    else choose(1);
  }
  void onButtonC() override { choose(2); }

  const char* name() const override { return "MathsGame"; }

private:
  Question question_ = {};
  uint32_t animMs_ = 0;
  uint16_t feedbackMs_ = 0;
  uint8_t questionNumber_ = 0;
  uint8_t correct_ = 0;
  uint8_t attempts_ = 0;
  uint8_t wrongMask_ = 0;
  uint8_t chosenIndex_ = 0;
  uint8_t level_ = 1;
  uint8_t coins_ = 0;
  bool advanceAfterFeedback_ = false;
  bool showGoodTry_ = false;
  bool showCorrect_ = false;
  bool revealSolution_ = false;
  bool levelUp_ = false;
  bool finished_ = false;

  void startRound() {
    App::ctx.gameId = static_cast<uint8_t>(GameId::HugoMaths);
    MathsProgress& maths = App::profile().maths;
    if (maths.currentLevel < 1 || maths.currentLevel > 5) maths.currentLevel = 2;
    level_ = maths.currentLevel;
    animMs_ = 0;
    questionNumber_ = 0;
    correct_ = 0;
    coins_ = 0;
    levelUp_ = false;
    finished_ = false;
    makeQuestion();
  }

  bool categoryAllowed(MathsCategory category) const {
    if (level_ == 1) return category == MathsCategory::Add || category == MathsCategory::Sub ||
                            category == MathsCategory::Seq || category == MathsCategory::Missing;
    if (level_ == 2) return category == MathsCategory::Add || category == MathsCategory::Sub ||
                            category == MathsCategory::Cmp || category == MathsCategory::Missing;
    if (level_ == 3) return category == MathsCategory::Add || category == MathsCategory::Sub ||
                            category == MathsCategory::Mul || category == MathsCategory::Seq;
    if (level_ == 4) return category == MathsCategory::Add || category == MathsCategory::Sub ||
                            category == MathsCategory::Mul || category == MathsCategory::Div;
    return true;
  }

  MathsCategory chooseCategory() const {
    const MathsProgress& maths = App::profile().maths;
    uint8_t weakest = 0xFF;
    uint8_t secondWeakest = 0xFF;
    for (uint8_t i = 0; i < static_cast<uint8_t>(MathsCategory::Count); ++i) {
      MathsCategory category = static_cast<MathsCategory>(i);
      if (!categoryAllowed(category)) continue;
      if (weakest == 0xFF || maths.categoryStrength[i] < maths.categoryStrength[weakest]) {
        secondWeakest = weakest;
        weakest = i;
      } else if (secondWeakest == 0xFF || maths.categoryStrength[i] < maths.categoryStrength[secondWeakest]) {
        secondWeakest = i;
      }
    }
    if (random(100) < 65) return static_cast<MathsCategory>(random(2) == 0 || secondWeakest == 0xFF ? weakest : secondWeakest);

    MathsCategory choices[8];
    uint8_t count = 0;
    for (uint8_t i = 0; i < static_cast<uint8_t>(MathsCategory::Count); ++i) {
      MathsCategory category = static_cast<MathsCategory>(i);
      if (categoryAllowed(category)) choices[count++] = category;
    }
    return choices[random(count)];
  }

  void makeQuestion() {
    attempts_ = 0;
    wrongMask_ = 0;
    feedbackMs_ = 0;
    advanceAfterFeedback_ = false;
    showGoodTry_ = false;
    showCorrect_ = false;
    revealSolution_ = false;
    question_.visual = VisualKind::None;
    question_.visualA = 0;
    question_.visualB = 0;
    question_.category = chooseCategory();

    switch (question_.category) {
      case MathsCategory::Add: makeAdd(); break;
      case MathsCategory::Sub: makeSub(); break;
      case MathsCategory::Mul: makeMul(); break;
      case MathsCategory::Div: makeDiv(); break;
      case MathsCategory::Seq: makeSequence(); break;
      case MathsCategory::Cmp: makeComparison(); break;
      case MathsCategory::Frac: makeFraction(); break;
      case MathsCategory::Missing: makeMissing(); break;
      default: makeAdd(); break;
    }
  }

  int maxForLevel() const {
    const int limits[5] = {10, 20, 50, 100, 150};
    return limits[level_ - 1];
  }

  void makeAdd() {
    const int limit = maxForLevel();
    if (level_ == 4 && random(4) == 0) {
      int a = 3 + random(28);
      int b = 2 + random(25);
      int c = 1 + random(a + b);
      question_.correctValue = a + b - c;
      snprintf(question_.prompt, sizeof(question_.prompt), "%d + %d - %d = ?", a, b, c);
      question_.visual = VisualKind::Dots;
      question_.visualA = a > 10 ? 10 : a;
      question_.visualB = b > 10 ? 10 : b;
      makeNumericAnswers();
      return;
    }
    int a = 1 + random(limit - 1);
    int b = 1 + random(limit - a);
    if (level_ == 2 && random(4) == 0) {
      a = 1 + random(10);
      b = a;
    }
    question_.correctValue = a + b;
    snprintf(question_.prompt, sizeof(question_.prompt), "%d + %d = ?", a, b);
    question_.visual = level_ <= 2 ? VisualKind::Dots : VisualKind::None;
    question_.visualA = a > 10 ? 10 : a;
    question_.visualB = b > 10 ? 10 : b;
    makeNumericAnswers();
  }

  void makeSub() {
    const int limit = maxForLevel();
    int a = 2 + random(limit - 1);
    int b = 1 + random(a);
    question_.correctValue = a - b;
    snprintf(question_.prompt, sizeof(question_.prompt), "%d - %d = ?", a, b);
    question_.visual = level_ <= 2 ? VisualKind::Dots : VisualKind::None;
    question_.visualA = a > 10 ? 10 : a;
    question_.visualB = b > 10 ? 10 : b;
    makeNumericAnswers();
  }

  void makeMul() {
    int a;
    int b;
    if (level_ == 3) {
      a = 2 + random(4);
      b = 2 + random(4);
    } else if (level_ == 4) {
      const int tables[3] = {2, 5, 10};
      a = tables[random(3)];
      b = 1 + random(10);
    } else {
      a = 2 + random(11);
      b = 2 + random(11);
    }
    question_.correctValue = a * b;
    if (level_ == 5 && random(3) == 0) snprintf(question_.prompt, sizeof(question_.prompt), "%d BAGS OF %d = ?", a, b);
    else snprintf(question_.prompt, sizeof(question_.prompt), "%d X %d = ?", a, b);
    question_.visual = VisualKind::Groups;
    question_.visualA = a > 6 ? 6 : a;
    question_.visualB = b > 6 ? 6 : b;
    makeNumericAnswers();
  }

  void makeDiv() {
    const int divisors4[3] = {2, 5, 10};
    int divisor = level_ == 4 ? divisors4[random(3)] : 2 + random(9);
    int answer = 1 + random(level_ == 4 ? 10 : 12);
    int total = divisor * answer;
    question_.correctValue = answer;
    snprintf(question_.prompt, sizeof(question_.prompt), "%d / %d = ?", total, divisor);
    question_.visual = VisualKind::Groups;
    question_.visualA = divisor > 6 ? 6 : divisor;
    question_.visualB = answer > 6 ? 6 : answer;
    makeNumericAnswers();
  }

  void makeSequence() {
    int step = level_ == 1 ? 2 : (level_ == 3 ? 3 + random(4) : 4 + random(8));
    int start = level_ == 1 ? random(3) : 1 + random(20);
    question_.correctValue = start + step * 3;
    snprintf(question_.prompt, sizeof(question_.prompt), "%d, %d, %d, ?", start, start + step, start + step * 2);
    makeNumericAnswers();
  }

  void makeComparison() {
    int a = random(maxForLevel() + 1);
    int b = random(maxForLevel() + 1);
    if (random(5) == 0) b = a;
    question_.correctValue = a > b ? 1 : (a < b ? -1 : 0);
    snprintf(question_.prompt, sizeof(question_.prompt), "%d ? %d", a, b);
    question_.values[0] = -1;
    question_.values[1] = 0;
    question_.values[2] = 1;
    snprintf(question_.answers[0], sizeof(question_.answers[0]), "<");
    snprintf(question_.answers[1], sizeof(question_.answers[1]), "=");
    snprintf(question_.answers[2], sizeof(question_.answers[2]), ">");
    shuffleAnswers();
  }

  void makeFraction() {
    int whole = 2 * (2 + random(19));
    question_.correctValue = whole / 2;
    snprintf(question_.prompt, sizeof(question_.prompt), "HALF OF %d = ?", whole);
    question_.visual = VisualKind::Pie;
    question_.visualA = 2;
    question_.visualB = 1;
    makeNumericAnswers();
  }

  void makeMissing() {
    if (level_ == 5 && random(2) == 0) {
      int a = 2 + random(20);
      int b = 1 + random(a);
      question_.correctValue = 1;
      snprintf(question_.prompt, sizeof(question_.prompt), "%d ? %d = %d", a, b, a + b);
      question_.values[0] = 1;
      question_.values[1] = 2;
      question_.values[2] = 3;
      snprintf(question_.answers[0], sizeof(question_.answers[0]), "+");
      snprintf(question_.answers[1], sizeof(question_.answers[1]), "-");
      snprintf(question_.answers[2], sizeof(question_.answers[2]), "X");
      shuffleAnswers();
      return;
    }
    const int limit = maxForLevel();
    int missing = 1 + random(limit / 2);
    int shown = 1 + random(limit / 2);
    question_.correctValue = missing;
    snprintf(question_.prompt, sizeof(question_.prompt), "? + %d = %d", shown, missing + shown);
    makeNumericAnswers();
  }

  int digitSwap(int value) const {
    if (value < 10 || value > 99) return value + 3;
    return (value % 10) * 10 + value / 10;
  }

  void makeNumericAnswers() {
    const int correct = question_.correctValue;
    int smallOffset = 1 + random(level_ >= 4 ? 5 : 3);
    int firstWrong = correct + (random(2) ? smallOffset : -smallOffset);
    if (firstWrong < 0) firstWrong = correct + smallOffset;
    int secondWrong = digitSwap(correct);
    if (secondWrong == correct || secondWrong == firstWrong || secondWrong < 0) secondWrong = correct + smallOffset + 2;
    question_.values[0] = correct;
    question_.values[1] = firstWrong;
    question_.values[2] = secondWrong;
    for (uint8_t i = 0; i < 3; ++i) snprintf(question_.answers[i], sizeof(question_.answers[i]), "%d", question_.values[i]);
    shuffleAnswers();
  }

  void shuffleAnswers() {
    for (int i = 2; i > 0; --i) {
      const int j = random(i + 1);
      int16_t value = question_.values[i];
      question_.values[i] = question_.values[j];
      question_.values[j] = value;
      char label[12];
      snprintf(label, sizeof(label), "%s", question_.answers[i]);
      snprintf(question_.answers[i], sizeof(question_.answers[i]), "%s", question_.answers[j]);
      snprintf(question_.answers[j], sizeof(question_.answers[j]), "%s", label);
    }
    for (uint8_t i = 0; i < 3; ++i) {
      if (question_.values[i] == question_.correctValue) question_.correctIndex = i;
    }
  }

  void choose(uint8_t index) {
    if (finished_ || feedbackMs_ > 0 || (wrongMask_ & (1 << index))) return;
    chosenIndex_ = index;
    if (index == question_.correctIndex) {
      ++correct_;
      adjustStrength(question_.category, 5);
      showCorrect_ = true;
      showGoodTry_ = false;
      revealSolution_ = false;
      advanceAfterFeedback_ = true;
      feedbackMs_ = 650;
      Audio::play(Sfx::Sparkle);
    } else {
      ++attempts_;
      ++App::profile().maths.incorrectAnswers;
      wrongMask_ |= 1 << index;
      showGoodTry_ = true;
      showCorrect_ = false;
      Audio::play(Sfx::GoodTry);
      if (attempts_ == 1) {
        advanceAfterFeedback_ = false;
        feedbackMs_ = 600;
      } else {
        adjustStrength(question_.category, -5);
        revealSolution_ = true;
        advanceAfterFeedback_ = true;
        feedbackMs_ = 1250;
      }
    }
    Save::requestSave();
  }

  void adjustStrength(MathsCategory category, int amount) {
    uint8_t& strength = App::profile().maths.categoryStrength[static_cast<uint8_t>(category)];
    int value = static_cast<int>(strength) + amount;
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    strength = value;
  }

  void advanceQuestion() {
    ++questionNumber_;
    if (questionNumber_ >= 5) finishRound();
    else makeQuestion();
  }

  void finishRound() {
    finished_ = true;
    MathsProgress& maths = App::profile().maths;
    if (correct_ >= 4) {
      ++maths.consecutiveStrongRounds;
      maths.consecutiveWeakRounds = 0;
      if (maths.consecutiveStrongRounds >= 3 && maths.currentLevel < 5) {
        ++maths.currentLevel;
        maths.consecutiveStrongRounds = 0;
        levelUp_ = true;
      }
    } else if (correct_ <= 1) {
      ++maths.consecutiveWeakRounds;
      maths.consecutiveStrongRounds = 0;
      if (maths.consecutiveWeakRounds >= 2 && maths.currentLevel > 1) {
        --maths.currentLevel;
        maths.consecutiveWeakRounds = 0;
      }
    } else {
      maths.consecutiveStrongRounds = 0;
      maths.consecutiveWeakRounds = 0;
    }

    uint8_t bonus = correct_ * level_;
    if (bonus > 7) bonus = 7;
    coins_ = 1 + bonus;
    PlayerProfile& profile = App::profile();
    const uint8_t gameIndex = static_cast<uint8_t>(GameId::HugoMaths);
    if (correct_ > profile.gameHighScores[gameIndex]) profile.gameHighScores[gameIndex] = correct_;
    App::ctx.lastReward = coins_;
    App::awardCoins(coins_);
    App::raiseEvent(GameEvent::MathsCorrect, 0, correct_);
    App::raiseEvent(GameEvent::GamePlayed, gameIndex);
    Save::requestSave();
    Audio::play(levelUp_ ? Sfx::LevelUp : Sfx::Celebrate);
    UI::startCoinAnim(coins_);
  }

  uint8_t promptTextSize() const {
    const size_t length = strlen(question_.prompt);
    return length > 22 ? 1 : (length > 14 ? 2 : 3);
  }

  void drawAnswerCards(M5Canvas& canvas) {
    for (uint8_t i = 0; i < 3; ++i) {
      const int x = 8 + i * 104;
      const int y = 112;
      const bool dimmed = wrongMask_ & (1 << i);
      uint16_t fill = dimmed ? TFT_DARKGREY : UI::theme().panel;
      uint16_t border = UI::theme().accent;
      if ((showCorrect_ && chosenIndex_ == i) || (revealSolution_ && question_.correctIndex == i)) border = TFT_GREEN;
      UI::drawPanel(x, y, 96, 66, fill);
      canvas.drawRoundRect(x, y, 96, 66, 12, border);
      canvas.setTextDatum(middle_center);
      canvas.setTextColor(dimmed ? TFT_LIGHTGREY : UI::theme().text);
      canvas.setTextSize(3);
      canvas.drawString(question_.answers[i], x + 48, y + 34);
      if (dimmed) canvas.drawLine(x + 21, y + 47, x + 75, y + 19, TFT_LIGHTGREY);
      if ((showCorrect_ && chosenIndex_ == i) || (revealSolution_ && question_.correctIndex == i)) UI::drawTick(x + 79, y + 14, 16);
    }
  }

  void drawQuestionVisual(M5Canvas& canvas, bool solution) {
    if (question_.visual == VisualKind::None) return;
    const int y = solution ? 152 : 91;
    if (question_.visual == VisualKind::Dots) {
      for (uint8_t i = 0; i < question_.visualA; ++i) canvas.fillCircle(90 + (i % 10) * 7, y + (i / 10) * 7, 2, TFT_CYAN);
      for (uint8_t i = 0; i < question_.visualB; ++i) canvas.fillCircle(174 + (i % 10) * 7, y + (i / 10) * 7, 2, TFT_YELLOW);
    } else if (question_.visual == VisualKind::Groups) {
      const uint8_t groups = question_.visualA;
      const uint8_t each = question_.visualB;
      for (uint8_t group = 0; group < groups; ++group) {
        int cx = 70 + group * (180 / (groups > 1 ? groups - 1 : 1));
        canvas.drawCircle(cx, y, 10, TFT_CYAN);
        for (uint8_t dot = 0; dot < each; ++dot) canvas.fillCircle(cx - 5 + (dot % 3) * 5, y - 4 + (dot / 3) * 7, 2, TFT_YELLOW);
      }
    } else if (question_.visual == VisualKind::Pie) {
      canvas.fillCircle(160, y, 16, TFT_YELLOW);
      canvas.fillRect(160, y - 16, 17, 33, UI::theme().panel);
      canvas.drawCircle(160, y, 16, TFT_WHITE);
      canvas.drawLine(160, y - 16, 160, y + 16, TFT_WHITE);
    }
  }

  void drawSolutionHint(M5Canvas& canvas) {
    const int count = question_.correctValue < 0 ? 0 : (question_.correctValue > 12 ? 12 : question_.correctValue);
    for (int i = 0; i < count; ++i) {
      canvas.fillCircle(121 + (i % 6) * 15, 96 + (i / 6) * 8, 3,
                        i % 2 ? TFT_YELLOW : TFT_CYAN);
    }
  }

  void drawResult(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(levelUp_ ? 3 : 2);
    canvas.drawString(levelUp_ ? "LEVEL UP!" : "GREAT MATHS!", 160, 51);
    for (uint8_t i = 0; i < 5; ++i) {
      UI::drawIcon(IconId::Star, 64 + i * 48, 103, 31, i < correct_ ? TFT_YELLOW : TFT_DARKGREY);
    }
    canvas.setTextSize(2);
    char score[20];
    snprintf(score, sizeof(score), "%u CORRECT", correct_);
    canvas.drawString(score, 160, 145);
    char reward[20];
    snprintf(reward, sizeof(reward), "+%u COINS", coins_);
    canvas.drawString(reward, 160, 172);
    UI::drawButtonBar("", "REPLAY", "", IconId::None, IconId::Book, IconId::None);
  }
};

MathsGame instance;
ScreenRegistrar registrar(ScreenId::MathsGame, instance);

}
