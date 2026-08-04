#include <Arduino.h>
#include <M5Unified.h>
#include <math.h>
#include <stdio.h>
#include "../App.h"
#include "../Managers.h"
#include "../UI.h"
#include "../Sprites.h"

namespace {

constexpr uint32_t ROUND_MS = 40000;
constexpr uint32_t SPAWN_MS = 690;
constexpr uint8_t ENTITY_COUNT = 12;
constexpr uint8_t SPARKLE_COUNT = 8;
constexpr int CART_X = 68;
constexpr int LANE_Y[3] = {103, 142, 181};

uint16_t rgb(uint8_t red, uint8_t green, uint8_t blue) {
  return static_cast<uint16_t>(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
}

enum class CoasterState : uint8_t { Running, Paused, Result };
enum class EntityKind : uint8_t { Star, Coin, JellyBean, Butterfly, DinoEgg, SoccerBall, Rock, Cloud, Bush };

struct Entity {
  float x;
  uint8_t lane;
  EntityKind kind;
  uint8_t colour;
  bool active;
};

struct Sparkle {
  int16_t x;
  int16_t y;
  uint16_t lifeMs;
  uint8_t phase;
};

class RollerCoasterGame final : public Screen {
public:
  void enter() override { startRound(); }

  void update(uint32_t deltaMs) override {
    animMs_ += deltaMs;
    updateSparkles(deltaMs);
    if (state_ == CoasterState::Result) return;
    if (state_ == CoasterState::Paused) return;
    Power::noteActivity();  // tilt gameplay must never idle-dim or sleep

    elapsedMs_ += deltaMs;
    scroll_ += deltaMs * 0.095f;
    if (invulnerableMs_ > 0) invulnerableMs_ = deltaMs >= invulnerableMs_ ? 0 : invulnerableMs_ - deltaMs;
    if (hopMs_ > 0) hopMs_ = deltaMs >= hopMs_ ? 0 : hopMs_ - deltaMs;

    float tilt = Input::tiltX();
    if (fabsf(tilt) < TILT_DEADZONE) tilt = 0.0f;
    if (tilt < -0.18f) targetLane_ = 0;
    else if (tilt > 0.18f) targetLane_ = 2;
    else targetLane_ = 1;
    targetLaneValue_ += (targetLane_ - targetLaneValue_) * 0.16f;
    cartY_ += (laneY(targetLaneValue_) - cartY_) * 0.18f;

    spawnAccumMs_ += deltaMs;
    if (spawnAccumMs_ >= SPAWN_MS) {
      spawnAccumMs_ -= SPAWN_MS;
      spawnEntity();
    }

    for (Entity& entity : entities_) {
      if (!entity.active) continue;
      entity.x -= deltaMs * 0.105f;
      if (entity.x < -24) {
        entity.active = false;
        continue;
      }
      const int entityY = trackY(entity.lane, entity.x);
      if (fabsf(entity.x - CART_X) < 22.0f && fabsf(entityY - cartY_) < 23.0f) collect(entity, entityY);
    }

    if (elapsedMs_ >= ROUND_MS) finishRound();
  }

  void draw() override {
    M5Canvas& canvas = Gfx::c();
    if (state_ == CoasterState::Result) {
      UI::drawBackground();
      UI::drawHeader("ROLLER COASTER");
      drawResult(canvas);
      return;
    }

    drawEnvironment(canvas);
    drawTrack(canvas);
    drawEntities(canvas);
    drawCart(canvas);
    drawSparkles(canvas);
    drawHud(canvas);
    if (state_ == CoasterState::Paused) drawPaused(canvas);
  }

  void onButtonB() override {
    if (state_ == CoasterState::Result) {
      startRound();
      return;
    }
    state_ = state_ == CoasterState::Paused ? CoasterState::Running : CoasterState::Paused;
    Audio::play(Sfx::Select);
  }

  void onButtonC() override {
    if (state_ == CoasterState::Result) App::goBack();
  }

  const char* name() const override { return "RollerCoasterGame"; }

private:
  Entity entities_[ENTITY_COUNT] = {};
  Sparkle sparkles_[SPARKLE_COUNT] = {};
  CoasterState state_ = CoasterState::Running;
  uint32_t elapsedMs_ = 0;
  uint32_t animMs_ = 0;
  uint32_t spawnAccumMs_ = 0;
  uint16_t invulnerableMs_ = 0;
  uint16_t hopMs_ = 0;
  float scroll_ = 0.0f;
  float cartY_ = LANE_Y[1];
  float targetLaneValue_ = 1.0f;
  uint8_t targetLane_ = 1;
  uint16_t score_ = 0;
  uint16_t collected_ = 0;
  uint8_t jellyBeans_ = 0;
  uint8_t coins_ = 0;

  void startRound() {
    state_ = CoasterState::Running;
    elapsedMs_ = 0;
    animMs_ = 0;
    spawnAccumMs_ = 0;
    invulnerableMs_ = 0;
    hopMs_ = 0;
    scroll_ = 0.0f;
    cartY_ = LANE_Y[1];
    targetLaneValue_ = 1.0f;
    targetLane_ = 1;
    score_ = 0;
    collected_ = 0;
    jellyBeans_ = 0;
    coins_ = 0;
    for (Entity& entity : entities_) entity.active = false;
    for (Sparkle& sparkle : sparkles_) sparkle.lifeMs = 0;
    App::ctx.gameId = static_cast<uint8_t>(GameId::RollerCoaster);
  }

  float laneY(float lane) const {
    if (lane <= 1.0f) return LANE_Y[0] + (LANE_Y[1] - LANE_Y[0]) * lane;
    return LANE_Y[1] + (LANE_Y[2] - LANE_Y[1]) * (lane - 1.0f);
  }

  int trackY(uint8_t lane, float x) const {
    return LANE_Y[lane] + static_cast<int>(sinf((x + scroll_) * 0.035f) * 7.0f);
  }

  bool isObstacle(EntityKind kind) const {
    return kind == EntityKind::Rock || kind == EntityKind::Cloud || kind == EntityKind::Bush;
  }

  void spawnEntity() {
    Entity* slot = nullptr;
    for (Entity& entity : entities_) {
      if (!entity.active) {
        slot = &entity;
        break;
      }
    }
    if (!slot) return;

    const uint8_t roll = random(100);
    if (roll < 13) slot->kind = EntityKind::Star;
    else if (roll < 26) slot->kind = EntityKind::Coin;
    else if (roll < 37) slot->kind = EntityKind::JellyBean;
    else if (roll < 48) slot->kind = EntityKind::Butterfly;
    else if (roll < 59) slot->kind = EntityKind::DinoEgg;
    else if (roll < 70) slot->kind = EntityKind::SoccerBall;
    else if (roll < 80) slot->kind = EntityKind::Rock;
    else if (roll < 90) slot->kind = EntityKind::Cloud;
    else slot->kind = EntityKind::Bush;
    slot->x = 342.0f + random(35);
    slot->lane = random(3);
    slot->colour = random(10);
    slot->active = true;
  }

  void collect(Entity& entity, int entityY) {
    if (isObstacle(entity.kind)) {
      if (invulnerableMs_ > 0) return;
      invulnerableMs_ = 1050;
      hopMs_ = 720;
      Audio::play(Sfx::GoodTry);
      addSparkles(CART_X + 9, static_cast<int>(cartY_) - 4, rgb(255, 210, 80));
      entity.active = false;
      return;
    }

    entity.active = false;
    ++score_;
    ++collected_;
    if (entity.kind == EntityKind::JellyBean) {
      ++jellyBeans_;
      Audio::play(Sfx::JellyBean);
    } else {
      Audio::play(entity.kind == EntityKind::Coin ? Sfx::Coin : Sfx::Sparkle);
    }
    addSparkles(static_cast<int>(entity.x), entityY, rgb(255, 245, 120));
  }

  void addSparkles(int x, int y, uint16_t color) {
    for (uint8_t i = 0; i < SPARKLE_COUNT; ++i) {
      if (sparkles_[i].lifeMs != 0) continue;
      sparkles_[i].x = x;
      sparkles_[i].y = y;
      sparkles_[i].lifeMs = 420;
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

  void finishRound() {
    state_ = CoasterState::Result;
    coins_ = collected_ / 4;
    if (coins_ < 1) coins_ = 1;
    if (coins_ > 8) coins_ = 8;
    PlayerProfile& profile = App::profile();
    const uint16_t pouch = profile.buddy.jellyBeans + jellyBeans_;
    profile.buddy.jellyBeans = pouch > 255 ? 255 : static_cast<uint8_t>(pouch);
    const uint8_t gameIndex = static_cast<uint8_t>(GameId::RollerCoaster);
    if (score_ > profile.gameHighScores[gameIndex]) profile.gameHighScores[gameIndex] = score_;
    App::ctx.lastReward = coins_;
    App::awardCoins(coins_);
    App::raiseEvent(GameEvent::GamePlayed, gameIndex);
    Save::requestSave();
    Audio::play(Sfx::Celebrate);
  }

  uint8_t roomFamily() const {
    const uint16_t room = App::profile().inventory.equippedRoom;
    if (room == 53 || room == 113) return 3;
    if (room == 54) return 9;
    if (room == 58 || room == 119) return 5;
    if (room == 110 || room == 120) return 2;
    if (room == 111 || room == 112) return 1;
    if (room == 114) return 6;
    if (room == 115 || room == 118) return 4;
    if (room == 116) return 7;
    if (room == 117) return 8;
    return 0;
  }

  void drawEnvironment(M5Canvas& canvas) {
    const uint8_t family = roomFamily();
    const uint16_t sky[] = {
      rgb(103, 56, 150), rgb(60, 139, 91), rgb(48, 128, 183), rgb(27, 115, 161), rgb(25, 27, 75),
      rgb(232, 124, 174), rgb(42, 50, 91), rgb(210, 145, 66), rgb(23, 39, 78), rgb(69, 32, 91)
    };
    canvas.fillScreen(sky[family]);
    canvas.fillRect(0, 84, SCREEN_W, 156, family == 3 ? rgb(25, 91, 121) : rgb(50, 75, 75));
    const int farShift = static_cast<int>(scroll_ * 0.12f) % 96;
    const int nearShift = static_cast<int>(scroll_ * 0.25f) % 80;

    for (int x = -96 - farShift; x < SCREEN_W + 96; x += 96) {
      if (family == 0) {
        canvas.fillRect(x + 25, 43, 48, 45, rgb(220, 172, 206));
        canvas.fillTriangle(x + 20, 43, x + 49, 18, x + 78, 43, rgb(242, 205, 99));
      } else if (family == 1) {
        canvas.fillTriangle(x, 88, x + 48, 30, x + 95, 88, rgb(37, 94, 53));
        canvas.fillCircle(x + 22, 72, 20, rgb(57, 150, 72));
      } else if (family == 2) {
        canvas.fillRect(x + 4, 52, 88, 35, rgb(185, 203, 218));
        for (int i = 0; i < 6; ++i) canvas.fillCircle(x + 12 + i * 14, 61, 3, i % 2 ? TFT_YELLOW : TFT_RED);
      } else if (family == 3) {
        canvas.drawCircle(x + 30, 50, 8, rgb(125, 220, 235));
        canvas.drawCircle(x + 65, 70, 5, rgb(125, 220, 235));
      } else if (family == 4 || family == 8) {
        canvas.fillCircle(x + 25, 37, 2, TFT_WHITE);
        canvas.fillCircle(x + 70, 61, 2, TFT_YELLOW);
        canvas.fillTriangle(x + 44, 80, x + 55, 45, x + 66, 80, rgb(205, 213, 226));
      } else if (family == 5) {
        canvas.fillRect(x + 44, 38, 8, 49, rgb(230, 239, 245));
        canvas.fillCircle(x + 48, 34, 18, rgb(255, 214, 231));
      } else if (family == 6) {
        canvas.fillRect(x + 8, 43, 28, 45, rgb(34, 38, 61));
        canvas.fillRect(x + 45, 28, 39, 60, rgb(26, 31, 51));
        canvas.drawLine(x + 20, 28, x + 80, 77, TFT_WHITE);
      } else if (family == 7) {
        canvas.fillRect(x + 39, 43, 22, 45, rgb(139, 80, 44));
        canvas.fillTriangle(x + 23, 43, x + 50, 26, x + 77, 43, rgb(105, 57, 35));
      } else {
        canvas.fillRect(x + 4, 58, 88, 29, rgb(91, 38, 110));
        canvas.fillCircle(x + 24, 54, 7, TFT_CYAN);
        canvas.fillCircle(x + 70, 54, 7, TFT_PINK);
      }
    }
    for (int x = -80 - nearShift; x < SCREEN_W + 80; x += 80) {
      canvas.fillTriangle(x, 95, x + 40, 68, x + 80, 95, family == 3 ? rgb(33, 117, 112) : rgb(39, 105, 65));
    }
  }

  void drawTrack(M5Canvas& canvas) {
    canvas.fillRect(0, 90, SCREEN_W, 133, rgb(68, 69, 73));
    for (uint8_t lane = 0; lane < 3; ++lane) {
      int previousY = trackY(lane, 0);
      for (int x = 4; x < SCREEN_W; x += 4) {
        const int y = trackY(lane, x);
        canvas.drawLine(x - 4, previousY - 9, x, y - 9, rgb(205, 207, 215));
        canvas.drawLine(x - 4, previousY + 9, x, y + 9, rgb(205, 207, 215));
        previousY = y;
      }
      const int tieShift = static_cast<int>(scroll_) % 22;
      for (int x = -22 - tieShift; x < SCREEN_W + 22; x += 22) {
        const int y = trackY(lane, x);
        canvas.drawLine(x, y - 13, x, y + 13, rgb(143, 92, 53));
      }
    }
  }

  void drawEntities(M5Canvas& canvas) {
    for (const Entity& entity : entities_) {
      if (!entity.active) continue;
      const int x = static_cast<int>(entity.x);
      const int y = trackY(entity.lane, entity.x);
      switch (entity.kind) {
        case EntityKind::Star: UI::drawIcon(IconId::Star, x, y, 25, TFT_YELLOW); break;
        case EntityKind::Coin: UI::drawIcon(IconId::Coin, x, y, 24, TFT_YELLOW, rgb(238, 151, 35)); break;
        case EntityKind::JellyBean: Sprites::drawJellyBean(x, y, 23, entity.colour); break;
        case EntityKind::Butterfly: UI::drawIcon(IconId::Butterfly, x, y, 26, TFT_PINK, TFT_CYAN); break;
        case EntityKind::DinoEgg:
          canvas.fillCircle(x, y, 11, rgb(232, 232, 189));
          canvas.fillCircle(x - 4, y - 3, 2, rgb(82, 153, 83));
          canvas.fillCircle(x + 5, y + 4, 3, rgb(82, 153, 83));
          break;
        case EntityKind::SoccerBall: Sprites::drawSoccerBall(x, y, 10, animMs_); break;
        case EntityKind::Rock:
          canvas.fillTriangle(x - 13, y + 9, x - 5, y - 11, x + 15, y + 9, rgb(119, 122, 128));
          break;
        case EntityKind::Cloud:
          canvas.fillCircle(x - 8, y + 2, 8, TFT_WHITE);
          canvas.fillCircle(x, y - 4, 11, TFT_WHITE);
          canvas.fillCircle(x + 10, y + 2, 8, TFT_WHITE);
          break;
        case EntityKind::Bush:
          canvas.fillCircle(x - 7, y + 1, 9, rgb(48, 142, 66));
          canvas.fillCircle(x + 6, y - 2, 11, rgb(58, 161, 73));
          break;
      }
    }
  }

  void drawCart(M5Canvas& canvas) {
    int hop = 0;
    if (hopMs_ > 0) hop = static_cast<int>(sinf((720 - hopMs_) * 0.014f) * 14.0f);
    const int wobble = hopMs_ > 0 && ((hopMs_ / 70) % 2) ? 4 : -4;
    const int y = static_cast<int>(cartY_) - hop;
    const bool blink = invulnerableMs_ > 0 && ((animMs_ / 80) % 2);
    if (!blink) {
      canvas.fillRoundRect(CART_X - 25, y - 8, 50, 24, 7, rgb(218, 58, 73));
      canvas.fillRect(CART_X - 20, y - 13, 40, 9, rgb(246, 171, 61));
      canvas.fillCircle(CART_X - 15, y + 16, 6, rgb(35, 38, 45));
      canvas.fillCircle(CART_X + 15, y + 16, 6, rgb(35, 38, 45));
      const PlayerProfile& profile = App::profile();
      Sprites::drawCharacter(profile.inventory.equippedCharacter, CART_X + (hopMs_ ? wobble : 0), y - 22, 34,
                             hopMs_ ? Anim::Jumping : Anim::Happy, animMs_,
                             profile.inventory.equippedAccessory, profile.inventory.equippedClothing);
    }
  }

  void drawSparkles(M5Canvas& canvas) {
    for (const Sparkle& sparkle : sparkles_) {
      if (sparkle.lifeMs == 0) continue;
      const int distance = (420 - sparkle.lifeMs) / 45;
      const int x = sparkle.x + ((sparkle.phase % 3) - 1) * distance;
      const int y = sparkle.y + ((sparkle.phase / 3) - 1) * distance;
      UI::drawIcon(IconId::Sparkle, x, y, 10, TFT_YELLOW);
    }
  }

  void drawHud(M5Canvas& canvas) {
    canvas.fillRoundRect(6, 5, 308, 27, 8, rgb(27, 31, 48));
    canvas.setTextDatum(middle_left);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(1);
    canvas.drawString("ROLLER COASTER", 14, 18);
    char score[20];
    snprintf(score, sizeof(score), "SCORE %u", score_);
    canvas.drawString(score, 132, 18);
    const uint32_t seconds = (ROUND_MS - elapsedMs_ + 999) / 1000;
    char time[8];
    snprintf(time, sizeof(time), "%luS", static_cast<unsigned long>(seconds));
    canvas.drawString(time, 277, 18);
    canvas.fillRect(8, 35, 304, 4, rgb(48, 55, 72));
    canvas.fillRect(8, 35, static_cast<int>(304.0f * (ROUND_MS - elapsedMs_) / ROUND_MS), 4, TFT_YELLOW);
    canvas.setTextDatum(middle_center);
    canvas.drawString("B PAUSE", 160, 231);
    canvas.setTextDatum(top_left);
  }

  void drawPaused(M5Canvas& canvas) {
    canvas.fillRoundRect(65, 76, 190, 88, 14, rgb(25, 29, 47));
    canvas.drawRoundRect(65, 76, 190, 88, 14, TFT_YELLOW);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(3);
    canvas.drawString("PAUSED", 160, 108);
    canvas.setTextSize(1);
    canvas.drawString("B TO RIDE  HOLD B TO EXIT", 160, 142);
    canvas.setTextDatum(top_left);
  }

  void drawResult(M5Canvas& canvas) {
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(UI::theme().text);
    canvas.setTextSize(3);
    canvas.drawString("WHAT A RIDE!", 160, 54);
    canvas.setTextSize(2);
    char total[22];
    snprintf(total, sizeof(total), "COLLECTED %u", collected_);
    canvas.drawString(total, 160, 92);
    char reward[20];
    snprintf(reward, sizeof(reward), "+%u COINS", coins_);
    canvas.drawString(reward, 160, 127);
    char beans[28];
    snprintf(beans, sizeof(beans), "+%u JELLY BEANS", jellyBeans_);
    canvas.drawString(beans, 160, 158);
    UI::drawButtonBar("", "REPLAY", "BACK", IconId::None, IconId::Games, IconId::Back);
  }
};

RollerCoasterGame instance;
ScreenRegistrar registrar(ScreenId::RollerCoasterGame, instance);

}
