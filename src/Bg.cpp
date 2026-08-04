#include "Bg.h"
#include "Managers.h"
#include <LittleFS.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr int SRC_W = 160;
constexpr int SRC_H = 120;
constexpr size_t SRC_BYTES = SRC_W * SRC_H * 2;

M5Canvas* bgCanvas = nullptr;   // 320x240 prescaled backdrop
char loadedKey[24] = "";

bool loadFile(const char* path) {
  File f = LittleFS.open(path, "r");
  if (!f || f.size() != SRC_BYTES) {
    if (f) f.close();
    return false;
  }
  if (bgCanvas == nullptr) {
    bgCanvas = new M5Canvas(&Gfx::c());
    bgCanvas->setPsram(true);
    bgCanvas->setColorDepth(16);
    if (!bgCanvas->createSprite(SRC_W * 2, SRC_H * 2)) {
      delete bgCanvas;
      bgCanvas = nullptr;
      f.close();
      Serial.println("[bg] canvas alloc failed");
      return false;
    }
  }
  static uint16_t row[SRC_W];
  for (int y = 0; y < SRC_H; ++y) {
    if (f.read(reinterpret_cast<uint8_t*>(row), sizeof(row)) != sizeof(row)) {
      f.close();
      return false;
    }
    for (int x = 0; x < SRC_W; ++x) {
      const uint16_t c = row[x];
      const int dx = x * 2;
      const int dy = y * 2;
      bgCanvas->drawPixel(dx, dy, c);
      bgCanvas->drawPixel(dx + 1, dy, c);
      bgCanvas->drawPixel(dx, dy + 1, c);
      bgCanvas->drawPixel(dx + 1, dy + 1, c);
    }
  }
  f.close();
  return true;
}

bool ensure(const char* key, const char* path) {
  if (strcmp(loadedKey, key) == 0 && bgCanvas != nullptr) return true;
  if (loadFile(path)) {
    snprintf(loadedKey, sizeof(loadedKey), "%s", key);
    Serial.printf("[bg] loaded %s\n", path);
    return true;
  }
  loadedKey[0] = '\0';
  return false;
}

}

namespace Bg {

bool ensureRoom(uint16_t roomItemId) {
  char key[24], path[40];
  snprintf(key, sizeof(key), "r%u", roomItemId);
  snprintf(path, sizeof(path), "/rooms/room_%03u.bin", roomItemId);
  return ensure(key, path);
}

bool ensureUi(const char* name) {
  char key[24], path[40];
  snprintf(key, sizeof(key), "u%s", name);
  snprintf(path, sizeof(path), "/ui/%s.bin", name);
  return ensure(key, path);
}

void draw() {
  if (bgCanvas != nullptr) bgCanvas->pushSprite(&Gfx::c(), 0, 0);
}

void invalidate() {
  loadedKey[0] = '\0';
}

}
