#include "App.h"
#include "Config.h"
#include "Content.h"
#include "Managers.h"
#include "Sprites.h"
#include "UI.h"

#include <stdio.h>
#include <string.h>

namespace FrankieWalkSession {

uint8_t finds[12] = {};
uint8_t findCount = 0;

uint8_t discoveryCount() { return findCount; }
uint8_t discoveryAt(uint8_t index) { return index < findCount ? finds[index] : 0xFF; }

}

namespace {

uint32_t randomInterval(uint32_t minimum, uint32_t maximum) {
  return minimum + static_cast<uint32_t>(random(static_cast<long>(maximum - minimum + 1)));
}

IconId findIcon(uint8_t find) {
  static constexpr IconId ICONS[12] = {
    IconId::Leaf, IconId::Bone, IconId::Butterfly, IconId::Ball,
    IconId::Water, IconId::Flower, IconId::Feather, IconId::Leaf,
    IconId::JellyBean, IconId::Dino, IconId::Sparkle, IconId::Gift
  };
  return find < 12 ? ICONS[find] : IconId::Gift;
}

class FrankieWalkScreen final : public Screen {
public:
  void enter() override {
    elapsedMs_ = 0;
    movingMs_ = 0;
    moveAverage_ = 0.0f;
    shakeBurstMs_ = 0;
    revealMs_ = 0;
    barkAtMs_ = randomInterval(18000, 42000);
    discoverAtMs_ = randomInterval(30000, 60000);
    paused_ = false;
    finishing_ = false;
    revealFind_ = 0xFF;
    revealCollectible_ = false;
    App::ctx.lastCollectible = 0xFF;
    FrankieWalkSession::findCount = 0;
    for (uint8_t& find : FrankieWalkSession::finds) find = 0xFF;
  }

  void update(uint32_t deltaMs) override {
    elapsedMs_ += deltaMs;
    if (finishing_) return;

    if (Input::shaken()) shakeBurstMs_ = 1100;
    if (shakeBurstMs_ > deltaMs) shakeBurstMs_ -= deltaMs;
    else shakeBurstMs_ = 0;

    const float sample = Input::moveMagnitude();
    const float alpha = static_cast<float>(deltaMs) / static_cast<float>(1000 + deltaMs);
    moveAverage_ += (sample - moveAverage_) * alpha;

    if (revealMs_ > 0) {
      revealMs_ = revealMs_ > deltaMs ? revealMs_ - deltaMs : 0;
      return;
    }
    if (paused_) return;

    moving_ = shakeBurstMs_ > 0 || moveAverage_ > FRANKIE_MOVE_THRESHOLD;
    if (!moving_) return;

    movingMs_ += deltaMs;
    if (movingMs_ >= barkAtMs_) {
      Audio::play(Sfx::Bark);
      barkAtMs_ += randomInterval(22000, 48000);
    }
    if (movingMs_ >= discoverAtMs_) {
      revealDiscovery();
      discoverAtMs_ += randomInterval(30000, 60000);
    }
    if (movingMs_ >= FRANKIE_WALK_TARGET_MS) {
      movingMs_ = FRANKIE_WALK_TARGET_MS;
      finishing_ = true;
      App::goTo(ScreenId::FrankieWalkResult);
    }
  }

  void draw() override {
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();

    canvas.fillRect(0, 0, 320, 126, 0x867D);
    canvas.fillCircle(273, 45, 24, 0xFFE0);
    canvas.fillRect(0, 126, 320, 78, 0x5D67);
    canvas.fillRoundRect(-18, 142, 356, 72, 28, 0xCE79);
    canvas.fillCircle(38, 118, 31, 0x2647);
    canvas.fillRect(33, 111, 10, 39, 0x8A22);
    canvas.fillCircle(286, 114, 35, 0x2E67);
    canvas.fillRect(281, 108, 10, 42, 0x8A22);

    UI::drawHeader("WALK FRANKIE");
    drawProgressTrail();

    const int travel = static_cast<int>((movingMs_ / 55) % 250);
    const int frankieX = moving_ && !paused_ ? 35 + travel : 160;
    Sprites::drawFrankie(frankieX, 145, 91, elapsedMs_, moving_ && !paused_);

    if (!paused_ && !moving_ && revealMs_ == 0) {
      canvas.fillRoundRect(82, 170, 156, 25, 8, 0xFFFF);
      canvas.setTextDatum(middle_center);
      canvas.setTextColor(theme.accent);
      canvas.setTextSize(1);
      canvas.drawString("KEEP WALKING!", 160, 182);
    }

    if (paused_) drawPause();
    if (revealMs_ > 0) drawReveal();
    UI::drawButtonBar("", paused_ ? "RESUME" : "PAUSE", "",
                      IconId::None, IconId::Paw, IconId::None);
  }

  void onButtonB() override {
    if (revealMs_ > 0) {
      revealMs_ = 0;
      return;
    }
    paused_ = !paused_;
    moving_ = false;
    Audio::play(Sfx::Select);
  }

  void onButtonBLong() override {
    Audio::play(Sfx::Back);
    App::goTo(ScreenId::FrankieWalkIntro);
  }

  bool allowGlobalBack() const override { return false; }
  const char* name() const override { return "FrankieWalk"; }

private:
  uint32_t elapsedMs_ = 0;
  uint32_t movingMs_ = 0;
  uint32_t shakeBurstMs_ = 0;
  uint32_t revealMs_ = 0;
  uint32_t barkAtMs_ = 0;
  uint32_t discoverAtMs_ = 0;
  float moveAverage_ = 0.0f;
  bool moving_ = false;
  bool paused_ = false;
  bool finishing_ = false;
  uint8_t revealFind_ = 0xFF;
  bool revealCollectible_ = false;

  void drawProgressTrail() const {
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();
    const uint8_t filled = static_cast<uint8_t>((movingMs_ * 12UL) / FRANKIE_WALK_TARGET_MS);
    canvas.fillRoundRect(10, 31, 300, 27, 9, 0xFFFF);
    for (uint8_t i = 0; i < 12; ++i) {
      const uint16_t color = i < filled ? theme.accent : 0xBDF7;
      UI::drawIcon(IconId::Paw, 24 + i * 25, 44, 15, color, theme.accent2);
    }
  }

  void drawPause() const {
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();
    canvas.fillRoundRect(61, 72, 198, 78, 14, theme.panel);
    canvas.drawRoundRect(61, 72, 198, 78, 14, theme.accent);
    UI::drawIcon(IconId::Paw, 95, 111, 33, theme.accent, theme.accent2);
    canvas.setTextDatum(middle_left);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(2);
    canvas.drawString("PAUSED", 122, 98);
    canvas.setTextSize(1);
    canvas.drawString("HOLD B TO EXIT", 122, 124);
  }

  void drawReveal() const {
    M5Canvas& canvas = Gfx::c();
    const Theme& theme = UI::theme();
    canvas.fillRoundRect(28, 61, 264, 126, 18, theme.panel);
    canvas.drawRoundRect(28, 61, 264, 126, 18, TFT_YELLOW);
    UI::drawIcon(findIcon(revealFind_), 82, 115, 68, theme.accent, theme.accent2);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(theme.text);
    canvas.setTextSize(1);
    canvas.drawString("FRANKIE FOUND", 205, 89);
    canvas.setTextSize(strlen(FRANKIE_FINDS[revealFind_]) <= 14 ? 2 : 1);
    canvas.drawString(FRANKIE_FINDS[revealFind_], 205, 118);
    if (revealCollectible_) {
      UI::drawIcon(IconId::Sparkle, 153, 151, 20, TFT_YELLOW, TFT_ORANGE);
      canvas.setTextDatum(middle_left);
      canvas.setTextSize(1);
      canvas.setTextColor(TFT_YELLOW);
      canvas.drawString("NEW COLLECTIBLE!", 169, 151);
    }
  }

  void revealDiscovery() {
    uint8_t available[12];
    uint8_t availableCount = 0;
    for (uint8_t candidate = 0; candidate < 12; ++candidate) {
      bool used = false;
      for (uint8_t i = 0; i < FrankieWalkSession::findCount; ++i) {
        if (FrankieWalkSession::finds[i] == candidate) used = true;
      }
      if (!used) available[availableCount++] = candidate;
    }
    if (availableCount == 0) return;

    revealFind_ = available[random(availableCount)];
    FrankieWalkSession::finds[FrankieWalkSession::findCount++] = revealFind_;
    revealCollectible_ = false;
    if (random(100) < 25 && !App::profile().hasCollectible(CollectGroup::FrankieFind, revealFind_)) {
      App::profile().setCollectible(CollectGroup::FrankieFind, revealFind_);
      App::ctx.lastCollectible = COLLECT_BASE[static_cast<uint8_t>(CollectGroup::FrankieFind)] + revealFind_;
      revealCollectible_ = true;
      Save::requestSave();
    }
    revealMs_ = 2600;
    Audio::play(Sfx::Discover);
  }
};

FrankieWalkScreen instance;
ScreenRegistrar registrar(ScreenId::FrankieWalk, instance);

}
