#pragma once
#include <stdint.h>

// Streamed pixel-art backgrounds: 160x120 RGB565 (little-endian) files in
// LittleFS (data/rooms/room_<id>.bin, data/ui/<name>.bin), prescaled 2x into a
// PSRAM canvas on load, then blitted per frame. Procedural art is the fallback
// whenever a file is missing.
namespace Bg {
  // Ensure the given room's backdrop is loaded (cached). False -> use fallback.
  bool ensureRoom(uint16_t roomItemId);
  // Ensure a UI backdrop ("select", "menu") is loaded. False -> use fallback.
  bool ensureUi(const char* name);
  void draw();          // blit the loaded backdrop full-screen onto Gfx::c()
  void invalidate();    // force reload on next ensure (e.g. after FS update)
}
