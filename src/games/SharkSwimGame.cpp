#include <Arduino.h>
#include <M5Unified.h>
#include <math.h>
#include <stdio.h>
#include "../App.h"
#include "../Managers.h"
#include "../UI.h"
#include "../Sprites.h"
#include "../Content.h"

namespace {

constexpr uint32_t ROUND_MS = 40000;
constexpr uint32_t SPAWN_MS = 650;
constexpr uint8_t ENTITY_COUNT = 14;
constexpr uint8_t BUBBLE_COUNT = 18;
constexpr uint8_t SPARKLE_COUNT = 8;
constexpr int SHARK_X = 72;

uint16_t rgb(uint8_t red, uint8_t green, uint8_t blue) {
  return static_cast<uint16_t>(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
}

enum class SwimState : uint8_t { Intro, Running, Paused, Result };
enum class SeaKind : uint8_t { Fish, Bubble, JellyBean, Chest, Seaweed, Rock, Jellyfish };

struct SeaEntity {
  float x;
  int16_t y;
  SeaKind kind;
  uint8_t colour;
  bool active;
};

struct OceanBubble {
  int16_t x;
  int16_t y;
  uint8_t radius;
  uint8_t speed;
};

struct Sparkle {
  int16_t x;
  int16_t y;
  uint16_t lifeMs;
  uint8_t phase;
};

class SharkSwimGame final : public Screen {
public:
  void enter() override { showIntro(); }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    updateSparkles(deltaMs);
    updateReveal(deltaMs);
    if (state_ == SwimState::Intro || state_ == SwimState::Result || state_ == SwimState::Paused) return;

    Power::noteActivity();  // tilt gameplay must never idle-dim or sleep
    elapsedMs_ += deltaMs;
    scroll_ += deltaMs * 0.08f;
    if (invulnerableMs_ > 0) invulnerableMs_ = deltaMs >= invulnerableMs_ ? 0 : invulnerableMs_ - deltaMs;
    if (dazeMs_ > 0) dazeMs_ = deltaMs >= dazeMs_ ? 0 : dazeMs_ - deltaMs;
    updateOceanBubbles(deltaMs);

    float tilt = Input::tiltY();
    if (fabsf(tilt) < TILT_DEADZONE) tilt = 0.0f;
    if (tilt < -0.85f) tilt = -0.85f;
    if (tilt > 0.85f) tilt = 0.85f;
    const float targetY = 124.0f + tilt * 112.0f;
    sharkY_ += (targetY - sharkY_) * 0.137f;
    if (sharkY_ < 58.0f) sharkY_ = 58.0f;
    if (sharkY_ > 205.0f) sharkY_ = 205.0f;

    spawnAccumMs_ += deltaMs;
    if (spawnAccumMs_ >= SPAWN_MS) {
      spawnAccumMs_ -= SPAWN_MS;
      spawnEntity();
    }

    for (SeaEntity& entity : entities_) {
      if (!entity.active) continue;
      const float speed = entity.kind == SeaKind::Bubble ? 0.075f : 0.095f;
      entity.x -= deltaMs * speed;
      if (entity.x < -30) {
        entity.active = false;
        continue;
      }
      if (fabsf(entity.x - SHARK_X) < collisionWidth(entity.kind) &&
          fabsf(entity.y - sharkY_) < collisionHeight(entity.kind)) {
        touch(entity);
      }
    }

    if (elapsedMs_ >= ROUND_MS) finishRound();
  }

  void draw() override {
    M5Canvas& canvas = Gfx::c();
    if (state_ == SwimState::Intro) {
      drawIntro(canvas);
      return;
    }
    if (state_ == SwimState::Result) {
      UI::drawBackground();
      UI::drawHeader("SHARK SWIM");
      drawResult(canvas);
      return;
    }

    drawOcean(canvas);
    drawEntities(canvas);
    drawShark(canvas);
    drawSparkles(canvas);
    drawHud(canvas);
    drawReveal(canvas);
    if (state_ == SwimState::Paused) drawPaused(canvas);
  }

  void onButtonA() override {
    if (state_ == SwimState::Intro) startRound();
  }

  void onButtonB() override {
    if (state_ == SwimState::Result) {
      showIntro();
      return;
    }
    if (state_ == SwimState::Running) {
      state_ = SwimState::Paused;
      Audio::play(Sfx::Select);
    } else if (state_ == SwimState::Paused) {
      state_ = SwimState::Running;
      Audio::play(Sfx::Select);
    }
  }

  void onButtonC() override {
    if (state_ == SwimState::Intro) cycleShark();
    else if (state_ == SwimState::Result) App::goBack();
  }

  const char* name() const override { return "SharkSwimGame"; }

private:
  SeaEntity entities_[ENTITY_COUNT] = {};
  OceanBubble bubbles_[BUBBLE_COUNT] = {};
  Sparkle sparkles_[SPARKLE_COUNT] = {};
  SwimState state_ = SwimState::Intro;
  uint32_t elapsedMs_ = 0;
  uint32_t animMs_ = 0;
  uint32_t spawnAccumMs_ = 0;
  uint16_t invulnerableMs_ = 0;
  uint16_t dazeMs_ = 0;
  uint16_t revealMs_ = 0;
  float scroll_ = 0.0f;
  float sharkY_ = 124.0f;
  uint16_t score_ = 0;
  uint8_t jellyBeans_ = 0;
  uint8_t coins_ = 0;
  uint8_t selectedShark_ = 0;
  uint8_t revealCollectible_ = 0xFF;
  CollectGroup revealGroup_ = CollectGroup::Shark;
  char revealText_[28] = {};

  void showIntro() {
    state_ = SwimState::Intro;
    animMs_ = 0;
    selectedShark_ = firstUnlockedShark();
    revealMs_ = 0;
    resetOceanBubbles();
    App::ctx.gameId = static_cast<uint8_t>(GameId::SharkSwim);
  }

  uint8_t firstUnlockedShark() const {
    const PlayerProfile& profile = App::profile();
    for (uint8_t shark = 0; shark < 6; ++shark) {
      if (profile.hasCollectible(CollectGroup::Shark, shark)) return shark;
    }
    return 0;
  }

  bool sharkAvailable(uint8_t shark) const {
    const PlayerProfile& profile = App::profile();
    if (profile.hasCollectible(CollectGroup::Shark, shark)) return true;
    if (shark != 0) return false;
    for (uint8_t i = 0; i < 6; ++i) {
      if (profile.hasCollectible(CollectGroup::Shark, i)) return false;
    }
    return true;
  }

  void cycleShark() {
    for (uint8_t step = 1; step <= 6; ++step) {
      const uint8_t candidate = (selectedShark_ + step) % 6;
      if (sharkAvailable(candidate)) {
        selectedShark_ = candidate;
        Audio::play(Sfx::Select);
        return;
      }
    }
  }

  void startRound() {
    state_ = SwimState::Running;
    elapsedMs_ = 0;
    animMs_ = 0;
    spawnAccumMs_ = 0;
    invulnerableMs_ = 0;
    dazeMs_ = 0;
    revealMs_ = 0;
    scroll_ = 0.0f;
    sharkY_ = 124.0f;
    score_ = 0;
    jellyBeans_ = 0;
    coins_ = 0;
    revealCollectible_ = 0xFF;
    revealText_[0] = '\0';
    for (SeaEntity& entity : entities_) entity.active = false;
    for (Sparkle& sparkle : sparkles_) sparkle.lifeMs = 0;
    resetOceanBubbles();
    Audio::play(Sfx::Splash);
  }

  void resetOceanBubbles() {
    for (OceanBubble& bubble : bubbles_) {
      bubble.x = random(SCREEN_W);
      bubble.y = random(45, SCREEN_H);
      bubble.radius = 1 + random(4);
      bubble.speed = 1 + random(3);
    }
  }

  void updateOceanBubbles(uint32_t deltaMs) {
    for (OceanBubble& bubble : bubbles_) {
      bubble.x -= static_cast<int>((deltaMs + 20) * bubble.speed / 45);
      bubble.y -= static_cast<int>((deltaMs + 30) * bubble.speed / 70);
      if (bubble.x < -5 || bubble.y < 40) {
        bubble.x = SCREEN_W + random(45);
        bubble.y = random(90, SCREEN_H);
      }
    }
  }

  void spawnEntity() {
    SeaEntity* slot = nullptr;
    for (SeaEntity& entity : entities_) {
      if (!entity.active) {
        slot = &entity;
        break;
      }
    }
    if (!slot) return;

    const uint8_t roll = random(100);
    if (roll < 30) slot->kind = SeaKind::Fish;
    else if (roll < 47) slot->kind = SeaKind::Bubble;
    else if (roll < 58) slot->kind = SeaKind::JellyBean;
    else if (roll < 63) slot->kind = SeaKind::Chest;
    else if (roll < 77) slot->kind = SeaKind::Seaweed;
    else if (roll < 89) slot->kind = SeaKind::Rock;
    else slot->kind = SeaKind::Jellyfish;
    slot->x = 344.0f + random(36);
    slot->colour = random(10);
    if (slot->kind == SeaKind::Seaweed) slot->y = random(137, 195);
    else if (slot->kind == SeaKind::Rock) slot->y = random(178, 211);
    else slot->y = random(58, 204);
    slot->active = true;
  }

  bool isHazard(SeaKind kind) const {
    return kind == SeaKind::Seaweed || kind == SeaKind::Rock || kind == SeaKind::Jellyfish;
  }

  float collisionWidth(SeaKind kind) const {
    return kind == SeaKind::Seaweed ? 24.0f : (kind == SeaKind::Chest ? 25.0f : 22.0f);
  }

  float collisionHeight(SeaKind kind) const {
    return kind == SeaKind::Seaweed ? 34.0f : (kind == SeaKind::Rock ? 23.0f : 21.0f);
  }

  void touch(SeaEntity& entity) {
    if (isHazard(entity.kind)) {
      if (invulnerableMs_ > 0) return;
      entity.active = false;
      invulnerableMs_ = 1100;
      dazeMs_ = 760;
      sharkY_ += sharkY_ < 125.0f ? 18.0f : -18.0f;
      Audio::play(Sfx::GoodTry);
      addSparkle(SHARK_X + 20, static_cast<int>(sharkY_), TFT_YELLOW);
      return;
    }

    entity.active = false;
    if (entity.kind == SeaKind::Fish) {
      score_ += 2;
      Audio::play(Sfx::Sparkle);
    } else if (entity.kind == SeaKind::Bubble) {
      ++score_;
      Audio::play(Sfx::Pop);
    } else if (entity.kind == SeaKind::JellyBean) {
      ++score_;
      ++jellyBeans_;
      Audio::play(Sfx::JellyBean);
    } else {
      score_ += 3;
      openTreasure();
    }
    addSparkle(static_cast<int>(entity.x), entity.y, TFT_CYAN);
  }

  void openTreasure() {
    revealCollectible_ = 0xFF;
    if (random(100) < 30 && chooseUnownedCollectible()) {
      PlayerProfile& profile = App::profile();
      profile.setCollectible(revealGroup_, revealCollectible_);
      App::ctx.lastCollectible = COLLECT_BASE[static_cast<uint8_t>(revealGroup_)] + revealCollectible_;
      const char* found = collectibleName(revealGroup_, revealCollectible_);
      snprintf(revealText_, sizeof(revealText_), "FOUND %s!", found ? found : "TREASURE");
      Audio::play(Sfx::Discover);
    } else {
      score_ += 5;
      snprintf(revealText_, sizeof(revealText_), "COIN TREASURE!");
      Audio::play(Sfx::Coin);
    }
    revealMs_ = 1450;
  }

  bool chooseUnownedCollectible() {
    uint8_t sharkOptions[6];
    uint8_t beanOptions[10];
    uint8_t sharkCount = 0;
    uint8_t beanCount = 0;
    const PlayerProfile& profile = App::profile();
    for (uint8_t i = 0; i < 6; ++i) {
      if (!profile.hasCollectible(CollectGroup::Shark, i)) sharkOptions[sharkCount++] = i;
    }
    for (uint8_t i = 0; i < 10; ++i) {
      if (!profile.hasCollectible(CollectGroup::JellyBean, i)) beanOptions[beanCount++] = i;
    }
    if (sharkCount == 0 && beanCount == 0) return false;
    if (sharkCount > 0 && (beanCount == 0 || random(2) == 0)) {
      revealGroup_ = CollectGroup::Shark;
      revealCollectible_ = sharkOptions[random(sharkCount)];
    } else {
      revealGroup_ = CollectGroup::JellyBean;
      revealCollectible_ = beanOptions[random(beanCount)];
    }
    return true;
  }

  void addSparkle(int x, int y, uint16_t color) {
    for (uint8_t i = 0; i < SPARKLE_COUNT; ++i) {
      if (sparkles_[i].lifeMs != 0) continue;
      sparkles_[i].x = x;
      sparkles_[i].y = y;
      sparkles_[i].lifeMs = 440;
      sparkles_[i].phase = i + (color & 3);
      return;
    }
  }

  void updateSparkles(uint32_t deltaMs) {
    for (Sparkle& sparkle : sparkles_) {
      if (sparkle.lifeMs == 0) continue;
      sparkle.lifeMs = deltaMs >= sparkle.lifeMs ? 0 : sparkle.lifeMs - deltaMs;
    }
  }

  void updateReveal(uint32_t deltaMs) {
    if (revealMs_ == 0 || state_ == SwimState::Paused) return;
    revealMs_ = deltaMs >= revealMs_ ? 0 : revealMs_ - deltaMs;
  }

  void finishRound() {
    state_ = SwimState::Result;
    coins_ = score_ / 5;
    if (coins_ < 1) coins_ = 1;
    if (coins_ > 8) coins_ = 8;
    PlayerProfile& profile = App::profile();
    const uint16_t pouch = profile.buddy.jellyBeans + jellyBeans_;
    profile.buddy.jellyBeans = pouch > 255 ? 255 : static_cast<uint8_t>(pouch);
    const uint8_t gameIndex = static_cast<uint8_t>(GameId::SharkSwim);
    if (score_ > profile.gameHighScores[gameIndex]) profile.gameHighScores[gameIndex] = score_;
    App::ctx.lastReward = coins_;
    App::awardCoins(coins_);
    App::raiseEvent(GameEvent::GamePlayed, gameIndex);
    Save::requestSave();
    Audio::play(Sfx::Celebrate);
  }

  void drawIntro(M5Canvas& canvas) {
    drawOcean(canvas);
    canvas.fillRoundRect(25, 38, 270, 169, 16, rgb(13, 59, 91));
    canvas.drawRoundRect(25, 38, 270, 169, 16, TFT_CYAN);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(2);
    canvas.drawString("SHARK SWIM", 160, 61);
    Sprites::drawShark(selectedShark_, 160, 116, 77, animMs_);
    canvas.setTextSize(2);
    canvas.drawString(SHARK_NAMES[selectedShark_], 160, 159);
    canvas.setTextSize(1);
    canvas.drawString("TILT TO SWIM  B PAUSES", 160, 181);
    UI::drawButtonBar("SWIM", "", "SHARK", IconId::Shark, IconId::None, IconId::ArrowR);
  }

  void drawOcean(M5Canvas& canvas) {
    canvas.fillScreen(rgb(19, 111, 159));
    canvas.fillRect(0, 38, SCREEN_W, 202, rgb(12, 91, 139));
    for (int ray = 0; ray < 5; ++ray) {
      const int x = 18 + ray * 72 - static_cast<int>(scroll_ * 0.08f) % 72;
      canvas.fillTriangle(x, 38, x + 23, 38, x + 74, 219, rgb(23, 116, 157));
    }
    const int ridgeShift = static_cast<int>(scroll_ * 0.18f) % 110;
    for (int x = -110 - ridgeShift; x < SCREEN_W + 110; x += 110) {
      canvas.fillTriangle(x, 221, x + 55, 180, x + 110, 221, rgb(19, 75, 102));
    }
    canvas.fillRect(0, 219, SCREEN_W, 21, rgb(202, 179, 111));
    for (const OceanBubble& bubble : bubbles_) {
      canvas.drawCircle(bubble.x, bubble.y, bubble.radius, rgb(117, 211, 232));
    }
  }

  void drawEntities(M5Canvas& canvas) {
    for (const SeaEntity& entity : entities_) {
      if (!entity.active) continue;
      const int x = static_cast<int>(entity.x);
      switch (entity.kind) {
        case SeaKind::Fish: drawFish(canvas, x, entity.y, entity.colour); break;
        case SeaKind::Bubble:
          canvas.drawCircle(x, entity.y, 12, rgb(149, 225, 240));
          canvas.drawCircle(x - 3, entity.y - 3, 4, TFT_WHITE);
          break;
        case SeaKind::JellyBean: Sprites::drawJellyBean(x, entity.y, 23, entity.colour); break;
        case SeaKind::Chest: drawChest(canvas, x, entity.y); break;
        case SeaKind::Seaweed: drawSeaweed(canvas, x, entity.y); break;
        case SeaKind::Rock:
          canvas.fillTriangle(x - 18, entity.y + 13, x - 7, entity.y - 13, x + 20, entity.y + 13, rgb(103, 112, 119));
          canvas.drawLine(x - 7, entity.y - 4, x + 8, entity.y + 7, rgb(151, 160, 166));
          break;
        case SeaKind::Jellyfish: drawJellyfish(canvas, x, entity.y); break;
      }
    }
  }

  void drawFish(M5Canvas& canvas, int x, int y, uint8_t colour) {
    const uint16_t colors[] = {TFT_YELLOW, TFT_ORANGE, TFT_PINK, TFT_CYAN, rgb(118, 224, 126)};
    const uint16_t color = colors[colour % 5];
    canvas.fillCircle(x, y, 9, color);
    canvas.fillTriangle(x + 8, y, x + 19, y - 9, x + 19, y + 9, color);
    canvas.fillCircle(x - 4, y - 3, 2, TFT_WHITE);
    canvas.fillCircle(x - 4, y - 3, 1, TFT_BLACK);
  }

  void drawChest(M5Canvas& canvas, int x, int y) {
    canvas.fillRoundRect(x - 16, y - 10, 32, 24, 5, rgb(147, 81, 35));
    canvas.fillRoundRect(x - 16, y - 15, 32, 12, 6, rgb(190, 112, 44));
    canvas.fillRect(x - 2, y - 6, 5, 13, TFT_YELLOW);
    canvas.fillCircle(x, y, 3, TFT_BLACK);
  }

  void drawSeaweed(M5Canvas& canvas, int x, int y) {
    const uint16_t green = rgb(39, 145, 82);
    canvas.fillRect(x - 5, y - 31, 9, 48, green);
    canvas.fillCircle(x - 10, y - 20, 9, green);
    canvas.fillCircle(x + 8, y - 8, 9, green);
    canvas.fillCircle(x - 7, y + 4, 8, green);
  }

  void drawJellyfish(M5Canvas& canvas, int x, int y) {
    const uint16_t pink = rgb(233, 133, 207);
    canvas.fillCircle(x, y - 4, 12, pink);
    canvas.fillRect(x - 12, y - 4, 24, 8, pink);
    for (int i = -8; i <= 8; i += 8) {
      const int wave = ((animMs_ / 100 + i) % 2) ? 4 : -4;
      canvas.drawLine(x + i, y + 4, x + i + wave, y + 18, pink);
    }
    canvas.fillCircle(x - 4, y - 6, 2, TFT_WHITE);
    canvas.fillCircle(x + 4, y - 6, 2, TFT_WHITE);
  }

  void drawShark(M5Canvas& canvas) {
    const int wobble = dazeMs_ > 0 && ((dazeMs_ / 65) % 2) ? 5 : -5;
    const int bounce = dazeMs_ > 0 ? static_cast<int>(sinf((760 - dazeMs_) * 0.016f) * 8.0f) : 0;
    const bool blink = invulnerableMs_ > 0 && ((animMs_ / 85) % 2);
    if (!blink) Sprites::drawShark(selectedShark_, SHARK_X, static_cast<int>(sharkY_) + bounce, 61, animMs_ + wobble * 20);
    if (dazeMs_ > 0) {
      UI::drawIcon(IconId::Sparkle, SHARK_X + 13 + wobble, static_cast<int>(sharkY_) - 30, 13, TFT_YELLOW);
      canvas.drawCircle(SHARK_X - 10, static_cast<int>(sharkY_) - 29, 3, TFT_CYAN);
    }
  }

  void drawSparkles(M5Canvas& canvas) {
    for (const Sparkle& sparkle : sparkles_) {
      if (sparkle.lifeMs == 0) continue;
      const int distance = (440 - sparkle.lifeMs) / 46;
      const int x = sparkle.x + ((sparkle.phase % 3) - 1) * distance;
      const int y = sparkle.y + ((sparkle.phase / 3) - 1) * distance;
      UI::drawIcon(IconId::Sparkle, x, y, 10, TFT_YELLOW);
    }
  }

  void drawHud(M5Canvas& canvas) {
    canvas.fillRoundRect(6, 5, 308, 28, 8, rgb(9, 51, 80));
    canvas.setTextDatum(middle_left);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(1);
    canvas.drawString("SHARK SWIM", 14, 19);
    char score[18];
    snprintf(score, sizeof(score), "SCORE %u", score_);
    canvas.drawString(score, 127, 19);
    const uint32_t seconds = (ROUND_MS - elapsedMs_ + 999) / 1000;
    char time[8];
    snprintf(time, sizeof(time), "%luS", static_cast<unsigned long>(seconds));
    canvas.drawString(time, 277, 19);
    canvas.fillRect(8, 36, 304, 4, rgb(19, 73, 101));
    canvas.fillRect(8, 36, static_cast<int>(304.0f * (ROUND_MS - elapsedMs_) / ROUND_MS), 4, TFT_CYAN);
    canvas.setTextDatum(middle_center);
    canvas.drawString("B PAUSE", 160, 231);
    canvas.setTextDatum(top_left);
  }

  void drawReveal(M5Canvas& canvas) {
    if (revealMs_ == 0) return;
    canvas.fillRoundRect(69, 49, 182, 40, 10, rgb(244, 207, 82));
    canvas.drawRoundRect(69, 49, 182, 40, 10, TFT_WHITE);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(rgb(39, 46, 66));
    canvas.setTextSize(1);
    canvas.drawString(revealText_, 160, 70);
    canvas.setTextDatum(top_left);
  }

  void drawPaused(M5Canvas& canvas) {
    canvas.fillRoundRect(65, 76, 190, 88, 14, rgb(8, 45, 73));
    canvas.drawRoundRect(65, 76, 190, 88, 14, TFT_CYAN);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(3);
    canvas.drawString("PAUSED", 160, 108);
    canvas.setTextSize(1);
    canvas.drawString("B TO SWIM  HOLD B TO EXIT", 160, 142);
    canvas.setTextDatum(top_left);
  }

  void drawResult(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(3);
    canvas.drawString("GREAT SWIMMING!", 160, 52);
    Sprites::drawShark(selectedShark_, 160, 91, 58, animMs_);
    canvas.setTextSize(2);
    char score[18];
    snprintf(score, sizeof(score), "SCORE %u", score_);
    canvas.drawString(score, 160, 127);
    char reward[20];
    snprintf(reward, sizeof(reward), "+%u COINS", coins_);
    canvas.drawString(reward, 160, 153);
    char beans[28];
    snprintf(beans, sizeof(beans), "+%u JELLY BEANS", jellyBeans_);
    canvas.drawString(beans, 160, 178);
    UI::drawButtonBar("", "REPLAY", "BACK", IconId::None, IconId::Games, IconId::Back);
  }
};

SharkSwimGame instance;
ScreenRegistrar registrar(ScreenId::SharkSwimGame, instance);

}
