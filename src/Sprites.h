#pragma once
#include <stdint.h>
#include "Models.h"

// All characters are ORIGINAL code-drawn pixel-art: a shared cute buddy body
// (round head/body, big eyes) with per-character palettes + signature elements
// (braid, hat, mask, fin...). Licensed-name characters are recognisable via
// colour palette + silhouette only — no copied artwork.
//
// drawCharacter renders the character for `characterItemId` (item catalog id)
// at centre (cx, cy) with body height ~size px, playing `anim` at `frameMs`.
// Equipped accessory/clothing overlays are layered when combinable.
namespace Sprites {
  void drawCharacter(uint16_t characterItemId, int cx, int cy, int size,
                     Anim anim, uint32_t frameMs,
                     uint16_t accessoryId = 0xFFFF, uint16_t clothingId = 0xFFFF);
  // Pixel-art portrait tiles for the player-select cards (0=Hugo, 1=Stella).
  // Falls back to the default characters when tile art is missing.
  void drawKidTile(uint8_t playerId, int cx, int cy, int size, uint32_t frameMs);
  // Non-character sprites used by games/screens:
  void drawFrankie(int cx, int cy, int size, uint32_t frameMs, bool walking);
  void drawDino(uint8_t dinoId, int cx, int cy, int size, uint16_t frameMs);
  void drawDinoSilhouette(uint8_t dinoId, int cx, int cy, int size);
  void drawShark(uint8_t sharkId, int cx, int cy, int size, uint32_t frameMs);
  void drawGoalie(int cx, int cy, int size, int diveDir, float diveT); // -1/0/1
  void drawSoccerBall(int cx, int cy, int r, uint32_t frameMs);
  void drawItemPreview(uint16_t itemId, int cx, int cy, int size, uint32_t frameMs);
  void drawJellyBean(int cx, int cy, int size, uint8_t colourIdx); // 0..9, 9=rainbow
  void drawHourglass(int cx, int cy, int size, float sandT);       // sandT 0..1 elapsed
  void drawRoom(uint16_t roomItemId);                              // full-screen backdrop
}
