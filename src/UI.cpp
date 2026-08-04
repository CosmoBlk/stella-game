#include "UI.h"
#include "Assets.h"
#include "Icons.h"
#include "Managers.h"
#include "App.h"
#include "Config.h"
#include <math.h>
#include <stdio.h>

namespace {
constexpr uint16_t COL_WHITE = 0xFFFF;
constexpr uint16_t COL_BLACK = 0x0000;
constexpr uint16_t COL_GOLD = 0xFEC0;
const Theme themes[2] = {
  {0x08D2, 0x03E8, 0xFD20, 0x07EF, COL_WHITE, 0x19AA},
  {0xA817, 0x4813, COL_GOLD, 0x7D7C, COL_WHITE, 0x51B1}
};

bool coinActive = false;
uint32_t coinElapsed = 0;
uint16_t coinAmount = 0;
uint32_t coinAdvancedAt = 0;

uint16_t blend565(uint16_t a, uint16_t b, uint8_t t) {
  const uint16_t ar = (a >> 11) & 0x1F;
  const uint16_t ag = (a >> 5) & 0x3F;
  const uint16_t ab = a & 0x1F;
  const uint16_t br = (b >> 11) & 0x1F;
  const uint16_t bg = (b >> 5) & 0x3F;
  const uint16_t bb = b & 0x1F;
  const uint16_t r = ar + ((int16_t)(br - ar) * t) / 255;
  const uint16_t g = ag + ((int16_t)(bg - ag) * t) / 255;
  const uint16_t blue = ab + ((int16_t)(bb - ab) * t) / 255;
  return (r << 11) | (g << 5) | blue;
}

void line(int x1, int y1, int x2, int y2, uint16_t color, int width = 2) {
  Gfx::c().drawLine(x1, y1, x2, y2, color);
  if (width > 1) Gfx::c().drawLine(x1 + 1, y1, x2 + 1, y2, color);
}

void circleIcon(int cx, int cy, int radius, uint16_t color) {
  Gfx::c().fillCircle(cx, cy, radius, color);
}

void drawPersonFace(int cx, int cy, int size, uint16_t color, bool happy) {
  M5Canvas& canvas = Gfx::c();
  canvas.drawCircle(cx, cy, size / 2, color);
  canvas.fillCircle(cx - size / 6, cy - size / 8, size / 14 + 1, color);
  canvas.fillCircle(cx + size / 6, cy - size / 8, size / 14 + 1, color);
  if (happy) canvas.drawArc(cx, cy + size / 10, size / 4, size / 4 - 2, 25, 155, color);
  else line(cx - size / 5, cy + size / 5, cx + size / 5, cy + size / 5, color);
}

void drawStarShape(int cx, int cy, int size, uint16_t color) {
  M5Canvas& canvas = Gfx::c();
  const int r = size / 2;
  canvas.fillTriangle(cx, cy - r, cx - r / 4, cy - r / 5, cx + r / 4, cy - r / 5, color);
  canvas.fillTriangle(cx - r, cy - r / 6, cx - r / 5, cy, cx - r / 3, cy + r, color);
  canvas.fillTriangle(cx + r, cy - r / 6, cx + r / 5, cy, cx + r / 3, cy + r, color);
  canvas.fillTriangle(cx - r / 2, cy + r / 2, cx, cy + r / 5, cx + r / 2, cy + r / 2, color);
}

void drawHeartShape(int cx, int cy, int size, uint16_t color) {
  const int r = size / 4;
  M5Canvas& canvas = Gfx::c();
  canvas.fillCircle(cx - r, cy - r / 2, r, color);
  canvas.fillCircle(cx + r, cy - r / 2, r, color);
  canvas.fillTriangle(cx - size / 2, cy - r / 2, cx + size / 2, cy - r / 2, cx, cy + size / 2, color);
}

void drawHomeShape(int cx, int cy, int size, uint16_t color) {
  M5Canvas& canvas = Gfx::c();
  canvas.fillTriangle(cx, cy - size / 2, cx - size / 2, cy, cx + size / 2, cy, color);
  canvas.fillRect(cx - size / 3, cy, size * 2 / 3, size / 2, color);
  canvas.fillRect(cx - size / 10, cy + size / 5, size / 5, size / 3, COL_BLACK);
}

void drawArrow(int cx, int cy, int size, uint16_t color, bool right) {
  const int direction = right ? 1 : -1;
  line(cx - direction * size / 3, cy, cx + direction * size / 3, cy, color, 3);
  line(cx + direction * size / 3, cy, cx, cy - size / 3, color, 3);
  line(cx + direction * size / 3, cy, cx, cy + size / 3, color, 3);
}
}

namespace UI {

const Theme& theme() { return themeOf(App::player()); }
const Theme& themeOf(PlayerId id) { return themes[static_cast<uint8_t>(id)]; }

void drawBackground() {
  M5Canvas& canvas = Gfx::c();
  const Theme& active = theme();
  for (int y = 0; y < SCREEN_H; y += 2) {
    const uint8_t amount = static_cast<uint8_t>((y * 255) / (SCREEN_H - 1));
    canvas.fillRect(0, y, SCREEN_W, 2, blend565(active.bgTop, active.bgBottom, amount));
  }
}

void drawHeader(const char* title) {
  M5Canvas& canvas = Gfx::c();
  const Theme& active = theme();
  canvas.fillRect(0, 0, SCREEN_W, 28, blend565(active.bgTop, COL_BLACK, 70));
  canvas.setTextDatum(middle_left);
  canvas.setTextColor(active.text);
  canvas.setTextSize(2);
  canvas.drawString(title, 10, 14);
  drawCoinBalance(250, 14, App::profile().coins);
  canvas.setTextDatum(top_left);
}

void drawCoinBalance(int x, int y, uint32_t coins) {
  M5Canvas& canvas = Gfx::c();
  char amount[12];
  snprintf(amount, sizeof(amount), "%lu", static_cast<unsigned long>(coins));
  drawIcon(IconId::Coin, x, y, 17, COL_GOLD, 0xFDA0);
  canvas.setTextDatum(middle_left);
  canvas.setTextColor(COL_WHITE);
  canvas.setTextSize(2);
  canvas.drawString(amount, x + 12, y);
  canvas.setTextDatum(top_left);
}

void drawButtonBar(const char* a, const char* b, const char* c, IconId ia, IconId ib, IconId ic) {
  M5Canvas& canvas = Gfx::c();
  const Theme& active = theme();
  canvas.fillRect(0, 204, SCREEN_W, 36, blend565(active.panel, COL_BLACK, 45));
  canvas.drawFastHLine(0, 204, SCREEN_W, active.accent);
  const char* labels[3] = {a, b, c};
  const IconId icons[3] = {ia, ib, ic};
  const int centres[3] = {53, 160, 267};
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(active.text);
  canvas.setTextSize(1);
  for (uint8_t i = 0; i < 3; ++i) {
    if (icons[i] != IconId::None) drawIcon(icons[i], centres[i], 214, 14, active.accent, active.accent2);
    if (labels[i] && labels[i][0]) canvas.drawString(labels[i], centres[i], 230);
  }
  canvas.setTextDatum(top_left);
}

void drawIcon(IconId id, int cx, int cy, int size, uint16_t color, uint16_t color2) {
  if (id == IconId::None) return;
  if (color2 == 0) color2 = color;
  M5Canvas& canvas = Gfx::c();
  // Pixel-art icon bitmaps take priority; the primitive glyphs below remain
  // the fallback for icons without art (arrows, shapes, tail entries).
  if (const PixelArt* art = iconArt(static_cast<uint8_t>(id))) {
    const int dh = size;
    const int dw = size * art->w / art->h;
    const int x0 = cx - dw / 2;
    const int y0 = cy - dh / 2;
    for (int dy = 0; dy < dh; ++dy) {
      const int sy = dy * art->h / dh;
      for (int dx = 0; dx < dw; ++dx) {
        const uint16_t p = art->px[sy * art->w + dx * art->w / dw];
        if (p != ART_TRANSPARENT) canvas.drawPixel(x0 + dx, y0 + dy, p);
      }
    }
    return;
  }
  const int half = size / 2;
  const int q = size / 4;
  switch (id) {
    case IconId::Apple:
      canvas.fillCircle(cx - q / 2, cy + 1, q + 2, color);
      canvas.fillCircle(cx + q / 2, cy + 1, q + 2, color);
      line(cx, cy - q, cx + 2, cy - half, color2);
      canvas.fillEllipse(cx + q, cy - q, q, q / 2 + 1, color2);
      break;
    case IconId::Smiley: drawPersonFace(cx, cy, size, color, true); break;
    case IconId::Moon:
      canvas.fillCircle(cx, cy, half, color);
      canvas.fillCircle(cx + q, cy - q / 2, half, theme().panel);
      break;
    case IconId::Coin:
      canvas.fillCircle(cx, cy, half, color);
      canvas.drawCircle(cx, cy, half - 2, color2);
      canvas.drawFastVLine(cx, cy - q, q * 2, color2);
      break;
    case IconId::Tick: drawTick(cx, cy, size); break;
    case IconId::Cross:
      line(cx - q, cy - q, cx + q, cy + q, color, 3);
      line(cx + q, cy - q, cx - q, cy + q, color, 3);
      break;
    case IconId::Star: case IconId::Sparkle: drawStarShape(cx, cy, size, color); break;
    case IconId::Heart: drawHeartShape(cx, cy, size, color); break;
    case IconId::Ball:
      canvas.drawCircle(cx, cy, half, color);
      canvas.fillCircle(cx, cy, q / 2 + 1, color2);
      for (uint8_t i = 0; i < 5; ++i) {
        const float angle = i * 1.256637f - 1.5708f;
        line(cx + cosf(angle) * q / 2, cy + sinf(angle) * q / 2,
             cx + cosf(angle) * (half - 1), cy + sinf(angle) * (half - 1), color2);
      }
      break;
    case IconId::Dino:
      canvas.fillEllipse(cx, cy + q / 2, half, q, color);
      canvas.fillCircle(cx + q, cy - q, q, color);
      canvas.fillTriangle(cx - half, cy, cx - half - q, cy - q, cx - q, cy + q, color);
      canvas.fillCircle(cx + q + 2, cy - q - 1, 1, COL_BLACK);
      break;
    case IconId::Shark:
      canvas.fillEllipse(cx, cy, half, q, color);
      canvas.fillTriangle(cx - half, cy, cx - half - q, cy - q, cx - half - q, cy + q, color);
      canvas.fillTriangle(cx, cy - q, cx + q / 2, cy - half, cx + q, cy - q, color2);
      canvas.fillCircle(cx + q, cy - 2, 1, COL_BLACK);
      break;
    case IconId::Crown:
      canvas.fillRect(cx - half, cy, size, q, color);
      canvas.fillTriangle(cx - half, cy, cx - half, cy - half, cx - q, cy, color);
      canvas.fillTriangle(cx - q, cy, cx, cy - half, cx + q, cy, color);
      canvas.fillTriangle(cx + q, cy, cx + half, cy - half, cx + half, cy, color);
      break;
    case IconId::Butterfly:
      canvas.fillEllipse(cx - q, cy - q / 2, q, half, color);
      canvas.fillEllipse(cx + q, cy - q / 2, q, half, color2);
      canvas.fillEllipse(cx, cy, 2, q, COL_WHITE);
      break;
    case IconId::Wand:
      line(cx - q, cy + q, cx + q, cy - q, color, 3);
      drawStarShape(cx + q, cy - q, q + 3, color2);
      break;
    case IconId::Note:
      line(cx + q, cy - half, cx + q, cy + q, color, 3);
      line(cx + q, cy - half, cx - q, cy - q, color, 3);
      canvas.fillCircle(cx - q, cy + q, q / 2 + 2, color);
      canvas.fillCircle(cx + q, cy + q, q / 2 + 2, color);
      break;
    case IconId::Paw:
      canvas.fillEllipse(cx, cy + q, q + 2, q, color);
      canvas.fillCircle(cx - q, cy - q, q / 2 + 1, color);
      canvas.fillCircle(cx, cy - half + 2, q / 2 + 1, color);
      canvas.fillCircle(cx + q, cy - q, q / 2 + 1, color);
      break;
    case IconId::Bone:
      line(cx - q, cy + q, cx + q, cy - q, color, 4);
      canvas.fillCircle(cx - q - 2, cy + q - 2, q / 2 + 2, color);
      canvas.fillCircle(cx + q + 2, cy - q + 2, q / 2 + 2, color);
      break;
    case IconId::Timer:
      canvas.drawCircle(cx, cy + 1, half - 1, color);
      canvas.fillRect(cx - q / 2, cy - half - 3, q, 4, color);
      line(cx, cy, cx, cy - q, color2);
      line(cx, cy, cx + q, cy + q / 2, color2);
      break;
    case IconId::Shirt:
      canvas.fillRect(cx - q, cy - q, q * 2, size - q, color);
      canvas.fillTriangle(cx - q, cy - q, cx - half, cy, cx - q, cy + q, color);
      canvas.fillTriangle(cx + q, cy - q, cx + half, cy, cx + q, cy + q, color);
      break;
    case IconId::Book:
      canvas.fillRoundRect(cx - half, cy - q, half - 1, q * 2, 2, color);
      canvas.fillRoundRect(cx + 1, cy - q, half - 1, q * 2, 2, color2);
      line(cx, cy - q, cx, cy + q, COL_WHITE);
      break;
    case IconId::Bed:
      canvas.fillRect(cx - half, cy, size, q, color);
      canvas.fillRect(cx - half, cy - q, q, q, color2);
      line(cx - half, cy + q, cx - half, cy + half, color);
      line(cx + half, cy + q, cx + half, cy + half, color);
      break;
    case IconId::Broom:
      line(cx + q, cy - half, cx - q, cy + q, color, 3);
      canvas.fillTriangle(cx - q, cy + q, cx - half, cy + half, cx + q, cy + half, color2);
      break;
    case IconId::Shoe:
      canvas.fillRoundRect(cx - half, cy, size, q + 3, q / 2, color);
      canvas.fillRect(cx - q, cy - q, q, q, color);
      break;
    case IconId::Plate:
      canvas.fillCircle(cx, cy, half, color);
      canvas.fillCircle(cx, cy, q, theme().panel);
      break;
    case IconId::Water:
      canvas.fillTriangle(cx, cy - half, cx - q, cy + q, cx + q, cy + q, color);
      canvas.fillCircle(cx, cy + q, q, color);
      break;
    case IconId::Gift:
      canvas.fillRect(cx - half, cy - q, size, q * 2, color);
      canvas.fillRect(cx - half, cy - q - 3, size, 4, color2);
      canvas.fillRect(cx - 2, cy - q - 3, 4, q * 2 + 3, color2);
      canvas.drawCircle(cx - q, cy - half + 2, q, color2);
      canvas.drawCircle(cx + q, cy - half + 2, q, color2);
      break;
    case IconId::Home: drawHomeShape(cx, cy, size, color); break;
    case IconId::Games:
      canvas.fillRoundRect(cx - half, cy - q, size, q * 2, q, color);
      canvas.fillCircle(cx + q, cy, 2, color2);
      canvas.fillCircle(cx + q + 4, cy - 4, 2, color2);
      line(cx - q - 4, cy, cx - q + 4, cy, color2);
      line(cx - q, cy - 4, cx - q, cy + 4, color2);
      break;
    case IconId::Shop:
      canvas.drawRoundRect(cx - half, cy - q, size, q * 2 + 3, 3, color);
      canvas.drawArc(cx, cy - q, q, q - 2, 180, 360, color);
      break;
    case IconId::Missions:
      canvas.fillRoundRect(cx - half, cy - half, size, size, 3, color);
      drawTick(cx, cy, q * 2);
      break;
    case IconId::Collection:
      canvas.fillRect(cx - half, cy - half, size, size, color);
      canvas.fillRect(cx - half + 3, cy - half + 3, size / 2 - 4, size / 2 - 4, color2);
      canvas.fillRect(cx + 1, cy + 1, size / 2 - 4, size / 2 - 4, color2);
      break;
    case IconId::Settings:
      canvas.drawCircle(cx, cy, q, color);
      canvas.fillCircle(cx, cy, q / 2, color);
      for (uint8_t i = 0; i < 8; ++i) {
        const float angle = i * 0.785398f;
        line(cx + cosf(angle) * q, cy + sinf(angle) * q,
             cx + cosf(angle) * half, cy + sinf(angle) * half, color, 3);
      }
      break;
    case IconId::Swap:
      drawArrow(cx, cy - q / 2, size, color, true);
      drawArrow(cx, cy + q / 2, size, color2, false);
      break;
    case IconId::Back: case IconId::ArrowL: drawArrow(cx, cy, size, color, false); break;
    case IconId::ArrowR: drawArrow(cx, cy, size, color, true); break;
    case IconId::JellyBean:
      canvas.fillEllipse(cx, cy, q, half, color);
      canvas.drawArc(cx, cy, q, q - 2, 210, 340, color2);
      break;
    case IconId::Sun:
      canvas.fillCircle(cx, cy, q, color);
      for (uint8_t i = 0; i < 8; ++i) {
        const float angle = i * 0.785398f;
        line(cx + cosf(angle) * (q + 2), cy + sinf(angle) * (q + 2),
             cx + cosf(angle) * half, cy + sinf(angle) * half, color);
      }
      break;
    case IconId::Flower:
      for (uint8_t i = 0; i < 6; ++i) {
        const float angle = i * 1.0472f;
        canvas.fillCircle(cx + cosf(angle) * q, cy + sinf(angle) * q, q / 2 + 1, color);
      }
      canvas.fillCircle(cx, cy, q / 2 + 1, color2);
      break;
    case IconId::Leaf:
      canvas.fillEllipse(cx, cy, half, q, color);
      line(cx - half, cy + q, cx + half, cy - q, color2);
      break;
    case IconId::Feather:
      canvas.fillEllipse(cx, cy, q, half, color);
      line(cx - q, cy + half, cx + q, cy - half, color2);
      break;
    case IconId::Lego:
      canvas.fillRect(cx - half, cy - q, size, q * 2, color);
      canvas.fillRect(cx - q - 2, cy - half, q, q, color2);
      canvas.fillRect(cx + 2, cy - half, q, q, color2);
      break;
    case IconId::Pencil:
      line(cx - half, cy + half, cx + half, cy - half, color, 4);
      canvas.fillTriangle(cx + q, cy - q, cx + half, cy - half, cx + half - 2, cy - q + 5, color2);
      break;
    case IconId::Jump:
      drawArrow(cx, cy, size, color, false);
      canvas.drawArc(cx, cy + q, half, q, 200, 340, color2);
      break;
    case IconId::Circle: canvas.drawCircle(cx, cy, half, color); break;
    case IconId::Triangle: canvas.drawTriangle(cx, cy - half, cx - half, cy + half, cx + half, cy + half, color); break;
    case IconId::Mic:
      canvas.fillRoundRect(cx - q, cy - half, q * 2, half + q, q, color);
      canvas.drawArc(cx, cy, half, half - 2, 0, 180, color2);
      line(cx, cy + q, cx, cy + half, color2);
      break;
    case IconId::Rocket:
      canvas.fillEllipse(cx, cy - q / 2, q, half, color);
      canvas.fillTriangle(cx - q, cy, cx - half, cy + q, cx - q, cy + q, color2);
      canvas.fillTriangle(cx + q, cy, cx + half, cy + q, cx + q, cy + q, color2);
      canvas.fillTriangle(cx - q / 2, cy + q, cx + q / 2, cy + q, cx, cy + half + q, COL_GOLD);
      break;
    case IconId::Hat:
      canvas.fillRect(cx - half, cy + q, size, q / 2 + 2, color);
      canvas.fillRoundRect(cx - q, cy - half, q * 2, half + q, q, color2);
      break;
    case IconId::Web:
      for (uint8_t i = 0; i < 8; ++i) {
        const float angle = i * 0.785398f;
        line(cx, cy, cx + cosf(angle) * half, cy + sinf(angle) * half, color);
      }
      canvas.drawCircle(cx, cy, q, color);
      canvas.drawCircle(cx, cy, half, color);
      break;
    case IconId::None: break;
  }
}

void drawBigCentred(const char* text, int y, int textSize, uint16_t color) {
  M5Canvas& canvas = Gfx::c();
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(color);
  canvas.setTextSize(textSize);
  canvas.drawString(text, SCREEN_W / 2, y);
  canvas.setTextDatum(top_left);
}

void drawPanel(int x, int y, int w, int h, uint16_t fill) {
  M5Canvas& canvas = Gfx::c();
  canvas.fillRoundRect(x, y, w, h, 10, fill);
  canvas.drawRoundRect(x, y, w, h, 10, blend565(fill, COL_WHITE, 55));
}

void drawNeedBar(int x, int y, IconId icon, uint8_t value, uint16_t color) {
  M5Canvas& canvas = Gfx::c();
  drawIcon(icon, x + 9, y + 7, 15, color);
  canvas.fillRoundRect(x + 22, y + 1, 74, 12, 6, blend565(theme().panel, COL_BLACK, 45));
  const int width = (70 * (value > 100 ? 100 : value)) / 100;
  if (width > 0) canvas.fillRoundRect(x + 24, y + 3, width, 8, 4, color);
}

void drawHoldProgress(float fraction) {
  if (fraction < 0.0f) fraction = 0.0f;
  if (fraction > 1.0f) fraction = 1.0f;
  M5Canvas& canvas = Gfx::c();
  canvas.drawCircle(160, 178, 20, blend565(theme().panel, COL_WHITE, 50));
  const int endAngle = static_cast<int>(fraction * 360.0f);
  if (endAngle > 0) canvas.drawArc(160, 178, 20, 15, 0, endAngle, theme().accent);
  drawIcon(IconId::Tick, 160, 178, 16, COL_WHITE);
}

void drawTick(int cx, int cy, int size) {
  line(cx - size / 2, cy, cx - size / 8, cy + size / 3, COL_WHITE, 3);
  line(cx - size / 8, cy + size / 3, cx + size / 2, cy - size / 3, COL_WHITE, 3);
}

void drawChoiceCards(const char* a, const char* b, const char* c, int highlight) {
  M5Canvas& canvas = Gfx::c();
  const char* labels[3] = {a, b, c};
  for (uint8_t i = 0; i < 3; ++i) {
    const int x = 8 + i * 104;
    const int y = i == highlight ? 135 : 141;
    drawPanel(x, y, 96, 56, theme().panel);
    if (i == highlight) canvas.drawRoundRect(x - 2, y - 2, 100, 60, 12, theme().accent);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(theme().text);
    canvas.setTextSize(1);
    canvas.drawString(labels[i] ? labels[i] : "", x + 48, y + 28);
  }
  canvas.setTextDatum(top_left);
}

void startCoinAnim(uint16_t amount) {
  coinActive = true;
  coinElapsed = 0;
  coinAmount = amount;
  coinAdvancedAt = 0;
}

bool coinAnimActive() { return coinActive; }

void tickCoinAnim(uint32_t deltaMs) {
  if (!coinActive) return;
  if (deltaMs > 0 && coinAdvancedAt != App::nowMs()) {
    coinElapsed += deltaMs;
    coinAdvancedAt = App::nowMs();
  }
  const float progress = coinElapsed / 650.0f;
  if (progress >= 1.0f) {
    coinActive = false;
    return;
  }
  M5Canvas& canvas = Gfx::c();
  for (uint8_t i = 0; i < 7; ++i) {
    float local = progress * 1.35f - i * 0.055f;
    if (local < 0.0f || local > 1.0f) continue;
    const float startX = 132.0f + i * 9.0f;
    const float x = startX + (250.0f - startX) * local;
    const float y = 150.0f + (14.0f - 150.0f) * local - sinf(local * 3.14159f) * 45.0f;
    canvas.fillCircle(static_cast<int>(x), static_cast<int>(y), 4, COL_GOLD);
    canvas.drawCircle(static_cast<int>(x), static_cast<int>(y), 4, 0xFDA0);
  }
  char reward[12];
  snprintf(reward, sizeof(reward), "+%u", coinAmount);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(COL_GOLD);
  canvas.setTextSize(2);
  canvas.drawString(reward, 160, 116);
  canvas.setTextDatum(top_left);
}

}
