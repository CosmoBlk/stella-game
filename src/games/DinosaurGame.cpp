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

enum class DinoMode : uint8_t {
  FindNamed,
  Silhouette,
  Herbivore,
  Carnivore,
  Flyer,
  Horns,
  LongNeck
};

enum class DinoState : uint8_t { Playing, StickerReveal, Result };

class DinosaurGame final : public Screen {
public:
  void enter() override { startRound(); }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    if (state_ == DinoState::Result) {
      UI::tickCoinAnim(deltaMs);
      return;
    }
    if (state_ == DinoState::StickerReveal) {
      stateMs_ += deltaMs;
      if (stateMs_ >= 2300) {
        state_ = DinoState::Result;
        stateMs_ = 0;
        UI::startCoinAnim(coins_);
      }
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
    UI::drawHeader("FIND THE DINOSAUR");
    M5Canvas& canvas = Gfx::c();
    if (state_ == DinoState::StickerReveal) {
      drawStickerReveal(canvas);
      return;
    }
    if (state_ == DinoState::Result) {
      drawResult(canvas);
      UI::tickCoinAnim(0);
      return;
    }

    drawPrompt(canvas);
    for (uint8_t i = 0; i < choiceCount_; ++i) drawCard(canvas, i);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(1);
    char progress[12];
    snprintf(progress, sizeof(progress), "%u / 5", question_ + 1);
    canvas.drawString(progress, 160, 190);
    UI::drawButtonBar("PICK", choiceCount_ == 3 ? "PICK" : "", "PICK",
                      IconId::Dino, choiceCount_ == 3 ? IconId::Dino : IconId::None,
                      IconId::Dino);
  }

  void onButtonA() override { choose(0); }
  void onButtonB() override {
    if (state_ == DinoState::Result) startRound();
    else if (state_ == DinoState::Playing && choiceCount_ == 3) choose(1);
  }
  void onButtonC() override { choose(choiceCount_ == 2 ? 1 : 2); }

  const char* name() const override { return "DinosaurGame"; }

private:
  DinoState state_ = DinoState::Playing;
  DinoMode mode_ = DinoMode::FindNamed;
  uint8_t choices_[3] = {};
  uint8_t choiceCount_ = 2;
  uint8_t correctIndex_ = 0;
  uint8_t chosenIndex_ = 0;
  uint8_t targetDino_ = 0;
  uint8_t question_ = 0;
  uint8_t correct_ = 0;
  uint8_t coins_ = 0;
  uint8_t sticker_ = 0xFF;
  uint32_t animMs_ = 0;
  uint32_t stateMs_ = 0;
  uint16_t feedbackMs_ = 0;
  bool correctFeedback_ = false;

  void startRound() {
    state_ = DinoState::Playing;
    mode_ = DinoMode::FindNamed;
    question_ = 0;
    correct_ = 0;
    coins_ = 0;
    sticker_ = 0xFF;
    animMs_ = 0;
    stateMs_ = 0;
    feedbackMs_ = 0;
    correctFeedback_ = false;
    App::ctx.gameId = static_cast<uint8_t>(GameId::FindDinosaur);
    App::ctx.lastCollectible = 0xFF;
    makeQuestion();
  }

  DinoMode randomMode() const {
    if (random(100) < 40) return DinoMode::FindNamed;
    return static_cast<DinoMode>(1 + random(6));
  }

  bool matchesMode(uint8_t dinoId) const {
    const DinoDef& dino = DINOS[dinoId];
    switch (mode_) {
      case DinoMode::FindNamed:
      case DinoMode::Silhouette: return dinoId == targetDino_;
      case DinoMode::Herbivore: return dino.herbivore;
      case DinoMode::Carnivore: return !dino.herbivore;
      case DinoMode::Flyer: return dino.flies;
      case DinoMode::Horns: return dino.horns;
      case DinoMode::LongNeck: return dino.longNeck;
    }
    return false;
  }

  uint8_t randomMatchingDino(bool shouldMatch, uint16_t usedMask) const {
    uint8_t candidates[12];
    uint8_t count = 0;
    for (uint8_t id = 0; id < 12; ++id) {
      if (!(usedMask & (1U << id)) && matchesMode(id) == shouldMatch) candidates[count++] = id;
    }
    return count == 0 ? 0 : candidates[random(count)];
  }

  void makeQuestion() {
    state_ = DinoState::Playing;
    feedbackMs_ = 0;
    correctFeedback_ = false;
    mode_ = randomMode();
    targetDino_ = random(12);
    choiceCount_ = question_ < 2 ? 2 : 3;

    if (mode_ != DinoMode::FindNamed && mode_ != DinoMode::Silhouette) {
      uint8_t matches[12];
      uint8_t matchCount = 0;
      for (uint8_t id = 0; id < 12; ++id) {
        targetDino_ = id;
        if (matchesMode(id)) matches[matchCount++] = id;
      }
      targetDino_ = matches[random(matchCount)];
    }

    correctIndex_ = random(choiceCount_);
    uint16_t usedMask = 0;
    for (uint8_t index = 0; index < choiceCount_; ++index) {
      if (index == correctIndex_) choices_[index] = targetDino_;
      else choices_[index] = randomMatchingDino(false, usedMask | (1U << targetDino_));
      usedMask |= 1U << choices_[index];
    }
  }

  void choose(uint8_t index) {
    if (state_ != DinoState::Playing || feedbackMs_ > 0 || index >= choiceCount_) return;
    chosenIndex_ = index;
    if (index == correctIndex_) {
      ++correct_;
      correctFeedback_ = true;
      feedbackMs_ = 900;
      Audio::play(choices_[index] == 5 ? Sfx::MusicCue : Sfx::Roar);
    } else {
      correctFeedback_ = false;
      feedbackMs_ = 850;
    }
  }

  void finishRound() {
    coins_ = 1 + correct_;
    if (coins_ > 6) coins_ = 6;
    PlayerProfile& profile = App::profile();
    const uint8_t gameIndex = static_cast<uint8_t>(GameId::FindDinosaur);
    if (correct_ > profile.gameHighScores[gameIndex]) profile.gameHighScores[gameIndex] = correct_;
    App::ctx.lastReward = coins_;
    App::awardCoins(coins_);
    App::raiseEvent(GameEvent::DinosaurFound, 0, correct_);
    App::raiseEvent(GameEvent::GamePlayed, gameIndex);

    sticker_ = chooseSticker(profile);
    if (sticker_ != 0xFF) {
      profile.setCollectible(CollectGroup::Dinosaur, sticker_);
      App::ctx.lastCollectible = COLLECT_BASE[static_cast<uint8_t>(CollectGroup::Dinosaur)] + sticker_;
      Save::saveNow();
      state_ = DinoState::StickerReveal;
      stateMs_ = 0;
      Audio::play(Sfx::Discover);
    } else {
      Save::requestSave();
      state_ = DinoState::Result;
      Audio::play(Sfx::Celebrate);
      UI::startCoinAnim(coins_);
    }
  }

  uint8_t chooseSticker(const PlayerProfile& profile) const {
    if (random(100) >= 20) return 0xFF;
    uint8_t unowned[12];
    uint8_t count = 0;
    for (uint8_t id = 0; id < 12; ++id) {
      if (!profile.hasCollectible(CollectGroup::Dinosaur, id)) unowned[count++] = id;
    }
    return count == 0 ? 0xFF : unowned[random(count)];
  }

  const char* promptText() const {
    switch (mode_) {
      case DinoMode::FindNamed: return nullptr;
      case DinoMode::Silhouette: return "FIND THIS SHAPE!";
      case DinoMode::Herbivore: return "WHO EATS PLANTS?";
      case DinoMode::Carnivore: return "WHO EATS MEAT?";
      case DinoMode::Flyer: return "WHO CAN FLY?";
      case DinoMode::Horns: return "WHO HAS HORNS?";
      case DinoMode::LongNeck: return "WHO HAS A LONG NECK?";
    }
    return "FIND THE DINOSAUR!";
  }

  void drawPrompt(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    if (mode_ == DinoMode::FindNamed) {
      char prompt[34];
      snprintf(prompt, sizeof(prompt), "FIND THE %s!", DINOS[targetDino_].name);
      canvas.setTextSize(strlen(prompt) > 22 ? 1 : 2);
      canvas.drawString(prompt, 160, 51);
    } else {
      canvas.setTextSize(2);
      canvas.drawString(promptText(), 160, 48);
    }
    if (mode_ == DinoMode::Silhouette) {
      Sprites::drawDinoSilhouette(targetDino_, 160, 73, 40);
    } else {
      for (uint8_t star = 0; star < correct_; ++star) {
        UI::drawIcon(IconId::Star, 128 + star * 16, 72, 13, TFT_YELLOW);
      }
    }
  }

  void drawCard(M5Canvas& canvas, uint8_t index) {
    const int cardWidth = choiceCount_ == 2 ? 128 : 96;
    const int x = choiceCount_ == 2 ? 24 + index * 144 : 8 + index * 104;
    int y = 91;
    if (mode_ == DinoMode::Silhouette) y = 101;
    if (feedbackMs_ > 0 && correctFeedback_ && index == correctIndex_) {
      y -= 7 + static_cast<int>(sinf(animMs_ * 0.025f) * 5.0f);
    }

    UI::drawPanel(x, y, cardWidth, 82, UI::theme().panel);
    uint16_t border = UI::theme().accent;
    if (feedbackMs_ > 0 && correctFeedback_ && index == correctIndex_) border = TFT_GREEN;
    canvas.drawRoundRect(x, y, cardWidth, 82, 12, border);

    if (feedbackMs_ > 0 && !correctFeedback_ && index == correctIndex_) {
      const uint16_t glow = ((animMs_ / 180) % 2) ? TFT_YELLOW : TFT_WHITE;
      canvas.drawRoundRect(x + 2, y + 2, cardWidth - 4, 78, 10, glow);
      canvas.drawRoundRect(x + 4, y + 4, cardWidth - 8, 74, 9, glow);
    }

    Sprites::drawDino(choices_[index], x + cardWidth / 2, y + 38, choiceCount_ == 2 ? 65 : 57,
                      animMs_ + index * 130);

    if (feedbackMs_ > 0 && correctFeedback_ && index == correctIndex_) {
      UI::drawIcon(IconId::Star, x + cardWidth - 18, y + 17, 22, TFT_YELLOW);
    }
  }

  void drawStickerReveal(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(3);
    canvas.drawString("DINOSAUR FOUND!", 160, 55);
    const int bounce = static_cast<int>(sinf(animMs_ * 0.012f) * 7.0f);
    Sprites::drawDino(sticker_, 160, 123 + bounce, 112, animMs_);
    canvas.setTextSize(2);
    canvas.drawString(DINOS[sticker_].name, 160, 181);
    for (uint8_t i = 0; i < 7; ++i) {
      UI::drawIcon(IconId::Sparkle, 36 + i * 41, 78 + (i % 2) * 65, 16,
                   i % 2 ? TFT_CYAN : TFT_YELLOW);
    }
  }

  void drawResult(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(3);
    canvas.drawString("DINO EXPERT!", 160, 58);
    for (uint8_t i = 0; i < 5; ++i) {
      UI::drawIcon(IconId::Star, 64 + i * 48, 112, 30, i < correct_ ? TFT_YELLOW : TFT_DARKGREY);
    }
    canvas.setTextSize(2);
    char reward[20];
    snprintf(reward, sizeof(reward), "+%u COINS", coins_);
    canvas.drawString(reward, 160, 164);
    UI::drawButtonBar("", "REPLAY", "", IconId::None, IconId::Dino, IconId::None);
  }
};

DinosaurGame instance;
ScreenRegistrar registrar(ScreenId::DinosaurGame, instance);

}
