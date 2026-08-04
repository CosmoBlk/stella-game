#pragma once
#include <stdint.h>
#include <M5Unified.h>
#include "Models.h"

// Shared UI kit. All draws target Gfx::c().
// Icons are code-drawn (no bitmaps). iconKey values are IconId below.
enum class IconId : uint8_t {
  None = 0, Apple, Smiley, Moon, Coin, Tick, Cross, Star, Heart,
  Ball, Dino, Shark, Crown, Butterfly, Wand, Note, Paw, Bone,
  Timer, Shirt, Book, Bed, Broom, Shoe, Plate, Water, Gift,
  Home, Games, Shop, Missions, Collection, Settings, Swap, Back,
  ArrowL, ArrowR, JellyBean, Sparkle, Sun, Flower, Leaf, Feather,
  Lego, Pencil, Jump, Circle, Triangle, Mic, Rocket, Hat, Web
};

struct Theme {   // per-player look
  uint16_t bgTop, bgBottom;      // vertical gradient
  uint16_t accent, accent2;
  uint16_t text;
  uint16_t panel;                // card/panel fill
};

namespace UI {
  const Theme& theme();                       // active player's theme (Hugo blue/green, Stella pink/purple)
  const Theme& themeOf(PlayerId id);
  void drawBackground();                      // gradient fill
  void drawHeader(const char* title);         // top bar: title + coin balance of active player
  void drawCoinBalance(int x, int y, uint32_t coins);
  void drawButtonBar(const char* a, const char* b, const char* c,
                     IconId ia = IconId::None, IconId ib = IconId::None, IconId ic = IconId::None);
  void drawIcon(IconId id, int cx, int cy, int size, uint16_t color, uint16_t color2 = 0);
  void drawBigCentred(const char* text, int y, int textSize, uint16_t color);
  void drawPanel(int x, int y, int w, int h, uint16_t fill);
  void drawNeedBar(int x, int y, IconId icon, uint8_t value, uint16_t color);
  void drawHoldProgress(float f);             // 0..1 ring/bar for hold-B actions
  void drawTick(int cx, int cy, int size);
  void drawChoiceCards(const char* a, const char* b, const char* c, int highlight); // 3 cards above buttons
  // Coin fly animation: call start, then tickDraw each frame until done
  void startCoinAnim(uint16_t amount);
  bool coinAnimActive();
  void tickCoinAnim(uint32_t deltaMs);
}
