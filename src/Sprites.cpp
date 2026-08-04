#include "Sprites.h"
#include "Assets.h"
#include "Managers.h"
#include <M5Unified.h>
#include <math.h>

namespace {

// -----------------------------------------------------------------------------
// Colour and primitive helpers
// -----------------------------------------------------------------------------

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

constexpr uint16_t C_WHITE = rgb(255, 255, 255);
constexpr uint16_t C_BLACK = rgb(18, 18, 24);
constexpr uint16_t DARK = rgb(38, 38, 56);
constexpr uint16_t C_PINK = rgb(245, 92, 170);
constexpr uint16_t LIGHT_PINK = rgb(255, 190, 222);
constexpr uint16_t C_PURPLE = rgb(128, 72, 190);
constexpr uint16_t LAVENDER = rgb(196, 154, 235);
constexpr uint16_t C_BLUE = rgb(55, 132, 220);
constexpr uint16_t ICE = rgb(164, 229, 246);
constexpr uint16_t TEAL = rgb(38, 174, 166);
constexpr uint16_t C_GREEN = rgb(65, 176, 90);
constexpr uint16_t LIME = rgb(151, 211, 74);
constexpr uint16_t C_YELLOW = rgb(250, 210, 65);
constexpr uint16_t C_GOLD = rgb(235, 173, 45);
constexpr uint16_t C_ORANGE = rgb(238, 130, 55);
constexpr uint16_t C_RED = rgb(213, 55, 61);
constexpr uint16_t C_BROWN = rgb(119, 73, 43);
constexpr uint16_t C_TAN = rgb(214, 161, 105);
constexpr uint16_t GREY = rgb(133, 144, 157);
constexpr uint16_t C_SILVER = rgb(192, 204, 213);

int clampi(int value, int low, int high) {
  return value < low ? low : (value > high ? high : value);
}

int absi(int value) {
  return value < 0 ? -value : value;
}

float clampf(float value, float low, float high) {
  return value < low ? low : (value > high ? high : value);
}

void drawStar(M5Canvas& canvas, int cx, int cy, int radius, uint16_t colour) {
  int inner = radius * 2 / 5;
  int px[10];
  int py[10];
  for (int i = 0; i < 10; ++i) {
    float angle = -1.5707963f + i * 0.6283185f;
    int rr = (i & 1) ? inner : radius;
    px[i] = cx + static_cast<int>(cosf(angle) * rr);
    py[i] = cy + static_cast<int>(sinf(angle) * rr);
  }
  for (int i = 1; i < 9; ++i) {
    canvas.fillTriangle(px[0], py[0], px[i], py[i], px[i + 1], py[i + 1], colour);
  }
}

void drawHeart(M5Canvas& canvas, int cx, int cy, int size, uint16_t colour) {
  int r = size / 4;
  canvas.fillCircle(cx - r, cy - r / 2, r, colour);
  canvas.fillCircle(cx + r, cy - r / 2, r, colour);
  canvas.fillTriangle(cx - size / 2, cy, cx + size / 2, cy, cx, cy + size / 2, colour);
}

void drawCrownShape(M5Canvas& canvas, int cx, int baseY, int width, uint16_t colour) {
  int left = cx - width / 2;
  int top = baseY - width / 2;
  canvas.fillRect(left, baseY - width / 5, width, width / 5, colour);
  canvas.fillTriangle(left, baseY - width / 5, left, top, cx - width / 5, baseY - width / 5, colour);
  canvas.fillTriangle(cx - width / 5, baseY - width / 5, cx, top - width / 6,
                      cx + width / 5, baseY - width / 5, colour);
  canvas.fillTriangle(cx + width / 5, baseY - width / 5, left + width, top,
                      left + width, baseY - width / 5, colour);
  canvas.fillCircle(cx, baseY - width / 10, clampi(width / 16, 1, 4), C_RED);
}

void drawCloud(M5Canvas& canvas, int cx, int cy, int size, uint16_t colour) {
  canvas.fillCircle(cx - size / 4, cy, size / 4, colour);
  canvas.fillCircle(cx, cy - size / 7, size / 3, colour);
  canvas.fillCircle(cx + size / 3, cy, size / 4, colour);
  canvas.fillRoundRect(cx - size / 2, cy, size, size / 3, size / 7, colour);
}

void drawWaves(M5Canvas& canvas, int y, int amplitude, uint16_t colour, int x0 = 0, int x1 = 320) {
  int step = amplitude * 3;
  for (int x = x0; x < x1; x += step) {
    canvas.fillCircle(x + amplitude, y, amplitude, colour);
    canvas.fillRect(x, y, step, amplitude + 2, colour);
  }
}

void drawSparkles(M5Canvas& canvas, int cx, int cy, int spread, uint32_t phase,
                  uint16_t colour) {
  for (int i = 0; i < 5; ++i) {
    int angleStep = static_cast<int>((phase + i * 3) % 12);
    float angle = angleStep * 0.5235988f;
    int distance = spread * (3 + (i & 1)) / 5;
    drawStar(canvas, cx + static_cast<int>(cosf(angle) * distance),
             cy + static_cast<int>(sinf(angle) * distance), clampi(spread / 9, 2, 6), colour);
  }
}

void drawBubble(M5Canvas& canvas, int x, int y, int radius, uint16_t colour) {
  canvas.drawCircle(x, y, radius, colour);
  canvas.fillCircle(x - radius / 3, y - radius / 3, clampi(radius / 5, 1, 3), C_WHITE);
}

void drawBraid(M5Canvas& canvas, int x, int topY, int length, int width, uint16_t colour,
               bool alternate = false) {
  int segments = clampi(length / clampi(width, 3, 8), 4, 14);
  for (int i = 0; i < segments; ++i) {
    int yy = topY + i * length / segments;
    int xx = x + ((i & 1) ? width / 4 : -width / 4);
    uint16_t c = alternate && (i & 1) ? rgb(105, 62, 142) : colour;
    canvas.fillCircle(xx, yy, width / 2, c);
  }
  canvas.fillTriangle(x - width / 2, topY + length, x + width / 2, topY + length,
                      x, topY + length + width, colour);
}

void drawFlower(M5Canvas& canvas, int cx, int cy, int size, uint16_t petal, uint16_t centre) {
  int r = clampi(size / 3, 2, 8);
  canvas.fillCircle(cx - r, cy, r, petal);
  canvas.fillCircle(cx + r, cy, r, petal);
  canvas.fillCircle(cx, cy - r, r, petal);
  canvas.fillCircle(cx, cy + r, r, petal);
  canvas.fillCircle(cx, cy, r, centre);
}

// -----------------------------------------------------------------------------
// Shared buddy renderer
// -----------------------------------------------------------------------------

enum class HairStyle : uint8_t {
  Bob, SideBraid, Flowing, ExtraLong, Updo, Wavy, PurplePlait, Short, Mane, Ears, Mask, Hat,
  Hood, Helmet, Metal, Fur
};

enum class Signature : uint8_t {
  Princess, Ice, Mermaid, LanternHair, BallGown, Island, Mic, Fairy, Unicorn, Bunny, Kitten,
  Soccer, DinoTrainer, TRex, Raptor, Shark, Spider, Cowboy, SpaceRanger, Astronaut, Robot,
  Explorer, Frankie
};

struct CharLook {
  uint16_t skin;
  uint16_t hair;
  uint16_t primary;
  uint16_t secondary;
  HairStyle hairStyle;
  Signature signature;
};

constexpr CharLook CHAR_LOOKS[24] = {
  {rgb(246, 202, 172), C_BROWN, C_PINK, C_GOLD, HairStyle::Bob, Signature::Princess},
  {rgb(249, 222, 205), rgb(244, 238, 205), ICE, rgb(89, 174, 220), HairStyle::SideBraid, Signature::Ice},
  {rgb(248, 203, 169), rgb(188, 47, 42), rgb(135, 62, 161), TEAL, HairStyle::Flowing, Signature::Mermaid},
  {rgb(250, 213, 177), rgb(244, 201, 67), LAVENDER, C_PURPLE, HairStyle::ExtraLong, Signature::LanternHair},
  {rgb(247, 209, 181), rgb(238, 204, 104), rgb(137, 199, 235), C_WHITE, HairStyle::Updo, Signature::BallGown},
  {rgb(177, 111, 74), rgb(54, 37, 30), rgb(177, 54, 53), rgb(245, 221, 167), HairStyle::Wavy, Signature::Island},
  {rgb(232, 187, 157), rgb(45, 35, 58), rgb(50, 45, 66), C_GOLD, HairStyle::PurplePlait, Signature::Mic},
  {rgb(242, 183, 151), rgb(48, 118, 173), TEAL, rgb(113, 216, 196), HairStyle::Flowing, Signature::Mermaid},
  {rgb(247, 207, 174), rgb(238, 151, 66), LAVENDER, C_PINK, HairStyle::Bob, Signature::Fairy},
  {rgb(255, 234, 220), rgb(224, 128, 190), C_WHITE, LAVENDER, HairStyle::Mane, Signature::Unicorn},
  {rgb(250, 221, 204), rgb(244, 189, 198), LIGHT_PINK, C_WHITE, HairStyle::Ears, Signature::Bunny},
  {rgb(239, 204, 178), rgb(80, 58, 48), C_ORANGE, C_WHITE, HairStyle::Ears, Signature::Kitten},
  {rgb(218, 161, 117), rgb(56, 40, 31), C_BLUE, C_WHITE, HairStyle::Short, Signature::Soccer},
  {rgb(225, 174, 127), C_BROWN, C_GREEN, C_ORANGE, HairStyle::Hat, Signature::DinoTrainer},
  {rgb(108, 164, 75), rgb(58, 100, 47), C_GREEN, LIME, HairStyle::Hood, Signature::TRex},
  {rgb(133, 183, 91), rgb(61, 91, 48), rgb(74, 139, 62), C_ORANGE, HairStyle::Hood, Signature::Raptor},
  {rgb(89, 151, 189), rgb(43, 94, 128), C_BLUE, C_WHITE, HairStyle::Hood, Signature::Shark},
  {C_RED, C_BLACK, C_RED, C_BLUE, HairStyle::Mask, Signature::Spider},
  {rgb(235, 184, 135), C_BROWN, C_YELLOW, C_BLUE, HairStyle::Hat, Signature::Cowboy},
  {rgb(224, 192, 162), C_PURPLE, C_WHITE, C_GREEN, HairStyle::Hood, Signature::SpaceRanger},
  {rgb(213, 166, 126), C_BROWN, C_WHITE, C_ORANGE, HairStyle::Helmet, Signature::Astronaut},
  {C_SILVER, GREY, rgb(112, 168, 192), DARK, HairStyle::Metal, Signature::Robot},
  {rgb(202, 143, 99), C_BROWN, C_TAN, C_GREEN, HairStyle::Hat, Signature::Explorer},
  {rgb(204, 151, 103), rgb(68, 54, 48), C_TAN, rgb(67, 54, 50), HairStyle::Fur, Signature::Frankie}
};

const CharLook& lookFor(uint16_t characterItemId) {
  if (characterItemId <= 11) return CHAR_LOOKS[characterItemId];
  if (characterItemId >= 60 && characterItemId <= 71) return CHAR_LOOKS[12 + characterItemId - 60];
  return CHAR_LOOKS[0];
}

struct BuddyPose {
  int bob;
  int sway;
  int bodyLean;
  int leftArm;
  int rightArm;
  int leftLeg;
  int rightLeg;
  bool blink;
  bool halfEyes;
  bool closedEyes;
  bool openSmile;
  bool flatMouth;
  bool chomp;
  bool zzz;
  bool horizontal;
  bool starBurst;
};

BuddyPose poseFor(Anim anim, uint32_t frameMs, int size) {
  uint32_t phase = frameMs / 120;
  int pulse = static_cast<int>(phase & 1) ? 1 : -1;
  BuddyPose pose = {0, 0, 0, 0, 0, 0, 0, false, false, false, false, false, false, false, false, false};
  switch (anim) {
    case Anim::Idle:
      pose.bob = ((phase / 3) & 1) ? -size / 40 : 0;
      pose.blink = phase % 29 == 0;
      break;
    case Anim::Happy:
      pose.bob = (phase % 4 == 1) ? -size / 10 : 0;
      pose.leftArm = -1;
      pose.rightArm = -1;
      pose.openSmile = true;
      break;
    case Anim::Sad:
      pose.bob = size / 20;
      pose.bodyLean = -size / 30;
      pose.leftArm = 1;
      pose.rightArm = 1;
      pose.flatMouth = true;
      break;
    case Anim::Tired:
      pose.sway = pulse * size / 35;
      pose.halfEyes = true;
      pose.zzz = phase % 12 < 7;
      break;
    case Anim::Eating:
      pose.chomp = true;
      pose.openSmile = (phase & 1) == 0;
      pose.rightArm = -1;
      break;
    case Anim::Sleeping:
      pose.bob = size / 15;
      pose.bodyLean = size / 12;
      pose.closedEyes = true;
      pose.zzz = true;
      break;
    case Anim::Celebrating:
      pose.bob = (phase & 1) ? -size / 8 : 0;
      pose.leftArm = -2;
      pose.rightArm = -2;
      pose.openSmile = true;
      pose.starBurst = true;
      break;
    case Anim::Walking:
      pose.sway = pulse * size / 45;
      pose.bodyLean = size / 30;
      pose.leftLeg = pulse;
      pose.rightLeg = -pulse;
      pose.leftArm = -pulse;
      pose.rightArm = pulse;
      break;
    case Anim::Dancing:
      pose.sway = pulse * size / 12;
      pose.leftArm = pulse > 0 ? -2 : 1;
      pose.rightArm = pulse < 0 ? -2 : 1;
      pose.openSmile = true;
      break;
    case Anim::Jumping:
      pose.bob = -size / 7 + absi(static_cast<int>(phase % 4) - 2) * size / 25;
      pose.leftArm = -2;
      pose.rightArm = -2;
      pose.openSmile = true;
      break;
    case Anim::Kicking:
      pose.bodyLean = -size / 18;
      pose.leftArm = -1;
      pose.rightArm = 1;
      pose.rightLeg = phase & 1 ? 3 : 1;
      break;
    case Anim::Swimming:
      pose.horizontal = true;
      pose.sway = pulse * size / 30;
      pose.leftArm = -pulse;
      pose.rightArm = pulse;
      pose.openSmile = true;
      break;
  }
  return pose;
}

void drawWingsBehind(M5Canvas& canvas, int cx, int cy, int w, int h, uint16_t first,
                     uint16_t second) {
  canvas.fillCircle(cx - w / 2, cy - h / 5, w / 3, first);
  canvas.fillCircle(cx + w / 2, cy - h / 5, w / 3, first);
  canvas.fillCircle(cx - w / 2, cy + h / 5, w / 4, second);
  canvas.fillCircle(cx + w / 2, cy + h / 5, w / 4, second);
  canvas.drawLine(cx - w * 3 / 4, cy - h / 4, cx - w / 5, cy + h / 4, C_WHITE);
  canvas.drawLine(cx + w * 3 / 4, cy - h / 4, cx + w / 5, cy + h / 4, C_WHITE);
}

void drawHairBack(M5Canvas& canvas, const CharLook& look, int cx, int headY, int headR,
                  int bodyBottom) {
  switch (look.hairStyle) {
    case HairStyle::Flowing:
      canvas.fillRoundRect(cx - headR - headR / 3, headY - headR / 2,
                           headR * 2 + headR * 2 / 3, bodyBottom - headY,
                           headR / 2, look.hair);
      break;
    case HairStyle::ExtraLong:
      canvas.fillRoundRect(cx - headR - headR / 2, headY - headR / 2,
                           headR * 2 + headR, bodyBottom - headY + headR / 2,
                           headR / 2, look.hair);
      canvas.fillArc(cx, bodyBottom, headR * 3 / 2, headR * 2, 0, 180, look.hair);
      break;
    case HairStyle::Wavy:
      for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 4; ++i) {
          canvas.fillCircle(cx + side * (headR + headR / 4), headY + i * headR / 2,
                            headR / 3, look.hair);
        }
      }
      break;
    case HairStyle::SideBraid:
      drawBraid(canvas, cx + headR, headY + headR / 3, bodyBottom - headY,
                clampi(headR / 2, 4, 12), look.hair);
      break;
    case HairStyle::PurplePlait:
      drawBraid(canvas, cx + headR, headY + headR / 4, bodyBottom - headY + headR / 2,
                clampi(headR / 2, 4, 12), rgb(77, 54, 94), true);
      break;
    default:
      break;
  }
}

void drawHairFront(M5Canvas& canvas, const CharLook& look, int cx, int headY, int headR) {
  switch (look.hairStyle) {
    case HairStyle::Bob:
    case HairStyle::Short:
      canvas.fillArc(cx, headY, headR * 3 / 4, headR + 2, 180, 360, look.hair);
      canvas.fillCircle(cx - headR + 2, headY, headR / 3, look.hair);
      canvas.fillCircle(cx + headR - 2, headY, headR / 3, look.hair);
      break;
    case HairStyle::SideBraid:
      canvas.fillArc(cx, headY, headR * 3 / 4, headR + 2, 180, 360, look.hair);
      canvas.fillTriangle(cx - headR, headY - headR / 2, cx + headR / 2, headY - headR,
                          cx - headR / 3, headY + headR / 4, look.hair);
      break;
    case HairStyle::Flowing:
    case HairStyle::ExtraLong:
    case HairStyle::Wavy:
    case HairStyle::PurplePlait:
      canvas.fillArc(cx, headY, headR * 3 / 4, headR + 2, 180, 360, look.hair);
      canvas.fillTriangle(cx - headR, headY - headR / 2, cx + headR / 2, headY - headR,
                          cx - headR / 4, headY + headR / 5, look.hair);
      break;
    case HairStyle::Updo:
      canvas.fillCircle(cx, headY - headR, headR / 2, look.hair);
      canvas.fillArc(cx, headY, headR * 3 / 4, headR + 2, 180, 360, look.hair);
      canvas.fillCircle(cx - headR + 2, headY, headR / 3, look.hair);
      canvas.fillCircle(cx + headR - 2, headY, headR / 3, look.hair);
      break;
    case HairStyle::Mane:
      canvas.fillArc(cx, headY, headR, headR + 4, 180, 360, look.hair);
      break;
    default:
      break;
  }
}

void drawEyes(M5Canvas& canvas, int cx, int y, int eyeR, const BuddyPose& pose,
              uint16_t faceColour, bool maskEyes = false) {
  int gap = eyeR + eyeR / 2;
  if (pose.closedEyes) {
    canvas.drawLine(cx - gap - eyeR, y, cx - gap + eyeR, y + eyeR / 3, C_BLACK);
    canvas.drawLine(cx + gap - eyeR, y + eyeR / 3, cx + gap + eyeR, y, C_BLACK);
    return;
  }
  if (pose.blink) {
    canvas.drawLine(cx - gap - eyeR, y, cx - gap + eyeR, y, C_BLACK);
    canvas.drawLine(cx + gap - eyeR, y, cx + gap + eyeR, y, C_BLACK);
    return;
  }
  if (maskEyes) {
    canvas.fillTriangle(cx - gap - eyeR, y - eyeR, cx - gap + eyeR, y - eyeR / 2,
                        cx - gap + eyeR / 2, y + eyeR, C_WHITE);
    canvas.fillTriangle(cx + gap + eyeR, y - eyeR, cx + gap - eyeR, y - eyeR / 2,
                        cx + gap - eyeR / 2, y + eyeR, C_WHITE);
    return;
  }
  canvas.fillCircle(cx - gap, y, eyeR, C_WHITE);
  canvas.fillCircle(cx + gap, y, eyeR, C_WHITE);
  if (pose.halfEyes) {
    canvas.fillRect(cx - gap - eyeR, y - eyeR, eyeR * 2 + 1, eyeR, faceColour);
    canvas.fillRect(cx + gap - eyeR, y - eyeR, eyeR * 2 + 1, eyeR, faceColour);
  }
  int pupilR = clampi(eyeR / 2, 1, 5);
  canvas.fillCircle(cx - gap, y + eyeR / 5, pupilR, C_BLACK);
  canvas.fillCircle(cx + gap, y + eyeR / 5, pupilR, C_BLACK);
  canvas.fillCircle(cx - gap - pupilR / 3, y, 1, C_WHITE);
  canvas.fillCircle(cx + gap - pupilR / 3, y, 1, C_WHITE);
}

void drawMouth(M5Canvas& canvas, int cx, int y, int width, const BuddyPose& pose) {
  if (pose.chomp) {
    canvas.fillCircle(cx, y, clampi(width / 4, 2, 7), pose.openSmile ? rgb(102, 35, 54) : C_BLACK);
    if (pose.openSmile) canvas.fillRect(cx - width / 5, y - 1, width * 2 / 5, 2, C_WHITE);
  } else if (pose.flatMouth) {
    canvas.drawLine(cx - width / 2, y, cx + width / 2, y, C_BLACK);
  } else if (pose.openSmile) {
    canvas.fillArc(cx, y - width / 5, 0, width / 2, 0, 180, rgb(105, 34, 54));
    canvas.fillCircle(cx, y + width / 5, clampi(width / 7, 1, 4), C_PINK);
  } else {
    canvas.drawLine(cx - width / 3, y, cx, y + width / 4, C_BLACK);
    canvas.drawLine(cx, y + width / 4, cx + width / 3, y, C_BLACK);
  }
}

void drawSignatureBehind(M5Canvas& canvas, const CharLook& look, int cx, int headY,
                         int headR, int torsoY, int bodyH) {
  switch (look.signature) {
    case Signature::Fairy:
      drawWingsBehind(canvas, cx, torsoY + bodyH / 3, headR * 3, bodyH, ICE, LIGHT_PINK);
      break;
    case Signature::Unicorn:
      canvas.fillTriangle(cx - headR / 2, headY - headR, cx, headY - headR * 2,
                          cx + headR / 3, headY - headR, C_GOLD);
      break;
    case Signature::Bunny:
      canvas.fillRoundRect(cx - headR * 3 / 4, headY - headR * 2, headR / 2,
                           headR * 3 / 2, headR / 4, C_WHITE);
      canvas.fillRoundRect(cx + headR / 4, headY - headR * 2, headR / 2,
                           headR * 3 / 2, headR / 4, C_WHITE);
      canvas.fillRoundRect(cx - headR * 5 / 8, headY - headR * 7 / 4, headR / 4,
                           headR, headR / 8, LIGHT_PINK);
      canvas.fillRoundRect(cx + headR * 3 / 8, headY - headR * 7 / 4, headR / 4,
                           headR, headR / 8, LIGHT_PINK);
      break;
    case Signature::Kitten:
      canvas.fillTriangle(cx - headR, headY - headR / 2, cx - headR * 3 / 4,
                          headY - headR * 3 / 2, cx - headR / 4, headY - headR, look.hair);
      canvas.fillTriangle(cx + headR, headY - headR / 2, cx + headR * 3 / 4,
                          headY - headR * 3 / 2, cx + headR / 4, headY - headR, look.hair);
      break;
    case Signature::TRex:
    case Signature::Raptor:
      for (int i = 0; i < 4; ++i) {
        int yy = torsoY + i * bodyH / 4;
        canvas.fillTriangle(cx - headR, yy, cx - headR - headR / 2, yy + headR / 4,
                            cx - headR, yy + headR / 2, C_ORANGE);
      }
      break;
    case Signature::Shark:
      canvas.fillTriangle(cx, torsoY, cx - headR / 2, torsoY + bodyH / 2,
                          cx + headR / 2, torsoY + bodyH / 2, rgb(43, 94, 128));
      break;
    case Signature::SpaceRanger:
      canvas.fillTriangle(cx - headR, torsoY, cx - headR * 2, torsoY + bodyH,
                          cx - headR / 2, torsoY + bodyH / 2, C_GREEN);
      canvas.fillTriangle(cx + headR, torsoY, cx + headR * 2, torsoY + bodyH,
                          cx + headR / 2, torsoY + bodyH / 2, C_GREEN);
      break;
    case Signature::Frankie:
      canvas.fillTriangle(cx - headR, headY - headR / 4, cx - headR * 4 / 5,
                          headY - headR * 3 / 2, cx - headR / 4, headY - headR, look.hair);
      canvas.fillTriangle(cx + headR, headY - headR / 4, cx + headR * 4 / 5,
                          headY - headR * 3 / 2, cx + headR / 4, headY - headR, look.hair);
      break;
    default:
      break;
  }
}

void drawSignatureFront(M5Canvas& canvas, const CharLook& look, int cx, int headY,
                        int headR, int torsoY, int bodyW, int bodyH, uint32_t phase) {
  switch (look.signature) {
    case Signature::Princess:
      drawCrownShape(canvas, cx, headY - headR + 2, headR, C_GOLD);
      drawHeart(canvas, cx, torsoY + bodyH / 3, bodyW / 4, LIGHT_PINK);
      break;
    case Signature::Ice:
      for (int i = 0; i < 4; ++i) {
        drawStar(canvas, cx - bodyW / 3 + i * bodyW / 5,
                 torsoY + bodyH / 4 + (i & 1) * bodyH / 3, clampi(bodyW / 14, 2, 4), C_WHITE);
      }
      break;
    case Signature::Mermaid:
      canvas.fillTriangle(cx - bodyW / 2, torsoY + bodyH, cx, torsoY + bodyH * 3 / 2,
                          cx, torsoY + bodyH, look.secondary);
      canvas.fillTriangle(cx + bodyW / 2, torsoY + bodyH, cx, torsoY + bodyH * 3 / 2,
                          cx, torsoY + bodyH, look.secondary);
      canvas.drawLine(cx - bodyW / 3, torsoY + bodyH / 4, cx + bodyW / 3,
                      torsoY + bodyH / 4, LIGHT_PINK);
      break;
    case Signature::LanternHair:
      drawStar(canvas, cx + bodyW / 3, torsoY + bodyH / 2, bodyW / 10, C_GOLD);
      break;
    case Signature::BallGown:
      canvas.fillCircle(cx, torsoY + bodyH, bodyW / 2, look.primary);
      canvas.drawLine(cx, torsoY + bodyH / 2, cx, torsoY + bodyH + bodyW / 3, C_WHITE);
      break;
    case Signature::Island:
      canvas.drawLine(cx - bodyW / 2, torsoY + bodyH / 3, cx + bodyW / 2,
                      torsoY + bodyH / 3, rgb(245, 221, 167));
      canvas.fillCircle(cx, torsoY, clampi(bodyW / 12, 2, 5), TEAL);
      break;
    case Signature::Mic:
      canvas.drawLine(cx + bodyW / 2, torsoY + bodyH / 3, cx + bodyW * 3 / 4,
                      torsoY + bodyH, C_SILVER);
      canvas.fillCircle(cx + bodyW * 3 / 4, torsoY + bodyH, bodyW / 10, DARK);
      canvas.drawLine(cx - bodyW / 2, torsoY + bodyH / 3, cx + bodyW / 2,
                      torsoY + bodyH / 3, C_GOLD);
      break;
    case Signature::Fairy:
      drawStar(canvas, cx, torsoY + bodyH / 2, bodyW / 9, C_GOLD);
      break;
    case Signature::Unicorn:
      drawStar(canvas, cx, torsoY + bodyH / 3, bodyW / 8, C_PINK);
      break;
    case Signature::Bunny:
      canvas.fillCircle(cx, headY + headR / 3, clampi(headR / 7, 2, 5), LIGHT_PINK);
      break;
    case Signature::Kitten:
      canvas.fillCircle(cx, headY + headR / 3, clampi(headR / 8, 2, 4), C_PINK);
      for (int side = -1; side <= 1; side += 2) {
        canvas.drawLine(cx + side * headR / 5, headY + headR / 3,
                        cx + side * headR, headY + headR / 5, C_BLACK);
        canvas.drawLine(cx + side * headR / 5, headY + headR / 2,
                        cx + side * headR, headY + headR * 2 / 3, C_BLACK);
      }
      break;
    case Signature::Soccer:
      canvas.fillRect(cx - bodyW / 2, torsoY + bodyH / 3, bodyW, bodyH / 5, C_WHITE);
      canvas.fillCircle(cx, torsoY + bodyH / 2, bodyW / 9, C_BLACK);
      break;
    case Signature::DinoTrainer:
      canvas.fillRoundRect(cx - headR, headY - headR - headR / 3, headR * 2,
                           headR / 2, headR / 5, C_TAN);
      canvas.fillRect(cx - headR * 3 / 2, headY - headR, headR * 3, headR / 5, C_TAN);
      canvas.fillCircle(cx, torsoY + bodyH / 3, bodyW / 8, C_ORANGE);
      break;
    case Signature::TRex:
      canvas.fillTriangle(cx - headR / 2, headY - headR, cx, headY - headR * 3 / 2,
                          cx + headR / 2, headY - headR, C_ORANGE);
      canvas.fillTriangle(cx - bodyW / 2, torsoY + bodyH / 2, cx - bodyW,
                          torsoY + bodyH, cx - bodyW / 3, torsoY + bodyH, look.primary);
      break;
    case Signature::Raptor:
      canvas.fillTriangle(cx - bodyW / 2, torsoY + bodyH / 2, cx - bodyW * 5 / 4,
                          torsoY + bodyH * 3 / 4, cx - bodyW / 3, torsoY + bodyH, look.primary);
      canvas.fillTriangle(cx + bodyW / 3, torsoY + bodyH, cx + bodyW / 2,
                          torsoY + bodyH + bodyH / 3, cx + bodyW / 6, torsoY + bodyH, C_WHITE);
      break;
    case Signature::Shark:
      canvas.fillTriangle(cx - headR / 2, headY + headR / 2, cx, headY + headR,
                          cx + headR / 2, headY + headR / 2, C_WHITE);
      for (int i = -2; i <= 2; ++i) {
        canvas.fillTriangle(cx + i * headR / 5, headY + headR / 2,
                            cx + i * headR / 5 + headR / 8, headY + headR * 3 / 4,
                            cx + i * headR / 5 + headR / 4, headY + headR / 2, C_WHITE);
      }
      break;
    case Signature::Spider:
      for (int i = 0; i < 3; ++i) {
        int yy = torsoY + bodyH / 4 + i * bodyH / 4;
        canvas.drawArc(cx, yy, bodyW / 4 + i * 2, bodyW / 2, 180, 360, C_BLACK);
      }
      canvas.drawLine(cx, torsoY, cx, torsoY + bodyH, C_BLACK);
      canvas.drawLine(cx - bodyW / 2, torsoY + bodyH / 2, cx + bodyW / 2,
                      torsoY + bodyH / 2, C_BLACK);
      break;
    case Signature::Cowboy:
      canvas.fillRoundRect(cx - headR, headY - headR - headR / 2, headR * 2,
                           headR / 2, headR / 4, C_BROWN);
      canvas.fillRect(cx - headR * 3 / 2, headY - headR, headR * 3, headR / 4, C_BROWN);
      for (int i = -1; i <= 1; ++i) {
        canvas.drawLine(cx - bodyW / 2, torsoY + bodyH / 4 + i * bodyH / 5,
                        cx + bodyW / 2, torsoY + bodyH / 4 + i * bodyH / 5, C_RED);
      }
      canvas.fillTriangle(cx - bodyW / 2, torsoY, cx - bodyW / 8, torsoY + bodyH,
                          cx, torsoY, C_WHITE);
      canvas.fillTriangle(cx + bodyW / 2, torsoY, cx + bodyW / 8, torsoY + bodyH,
                          cx, torsoY, C_WHITE);
      drawStar(canvas, cx + bodyW / 3, torsoY + bodyH / 3, bodyW / 10, C_GOLD);
      break;
    case Signature::SpaceRanger:
      canvas.fillRect(cx - bodyW / 2, torsoY + bodyH / 5, bodyW, bodyH / 4, C_GREEN);
      canvas.fillCircle(cx - bodyW / 5, torsoY + bodyH / 3, bodyW / 12, C_RED);
      canvas.fillCircle(cx + bodyW / 5, torsoY + bodyH / 3, bodyW / 12, C_BLUE);
      canvas.drawArc(cx, headY, headR + headR / 3, headR + headR / 3, 180, 360, ICE);
      break;
    case Signature::Astronaut:
      canvas.drawArc(cx, headY, headR + headR / 3, headR + headR / 3, 180, 360, ICE);
      canvas.fillRect(cx - bodyW / 3, torsoY + bodyH / 4, bodyW * 2 / 3, bodyH / 4, C_ORANGE);
      break;
    case Signature::Robot:
      canvas.fillRect(cx - headR, headY - headR, headR * 2, headR * 2, C_SILVER);
      canvas.drawLine(cx, headY - headR, cx, headY - headR * 3 / 2, DARK);
      canvas.fillCircle(cx, headY - headR * 3 / 2, clampi(headR / 8, 2, 4), C_RED);
      canvas.fillCircle(cx - headR / 2, headY, headR / 6, C_BLUE);
      canvas.fillCircle(cx + headR / 2, headY, headR / 6, C_BLUE);
      canvas.fillRect(cx - bodyW / 3, torsoY + bodyH / 3, bodyW * 2 / 3, bodyH / 5, DARK);
      break;
    case Signature::Explorer:
      canvas.fillRoundRect(cx - headR, headY - headR - headR / 3, headR * 2,
                           headR / 2, headR / 5, C_TAN);
      canvas.fillRect(cx - headR * 3 / 2, headY - headR, headR * 3, headR / 5, C_TAN);
      canvas.fillRect(cx - bodyW / 2, torsoY + bodyH / 4, bodyW, bodyH / 5, C_GREEN);
      canvas.fillRect(cx - bodyW / 3, torsoY + bodyH / 2, bodyW / 4, bodyH / 4, C_BROWN);
      canvas.fillRect(cx + bodyW / 12, torsoY + bodyH / 2, bodyW / 4, bodyH / 4, C_BROWN);
      break;
    case Signature::Frankie:
      canvas.fillArc(cx, headY + headR / 5, headR / 2, headR * 3 / 4, 180, 360, look.secondary);
      canvas.fillCircle(cx, headY + headR / 3, headR / 5, C_BLACK);
      break;
  }

  if (phase % 31 == 0 && (look.signature == Signature::Ice || look.signature == Signature::Fairy)) {
    drawStar(canvas, cx + bodyW, torsoY, clampi(bodyW / 10, 2, 5), C_WHITE);
  }
}

void drawClothingOverlay(M5Canvas& canvas, uint16_t clothingId, int cx, int torsoY,
                         int bodyW, int bodyH) {
  if (clothingId == 0xFFFF) return;
  int left = cx - bodyW / 2;
  switch (clothingId) {
    case 27:
      canvas.fillRect(left, torsoY, bodyW, bodyH / 6, C_RED);
      canvas.fillRect(left, torsoY + bodyH / 6, bodyW, bodyH / 6, C_ORANGE);
      canvas.fillRect(left, torsoY + bodyH / 3, bodyW, bodyH / 6, C_YELLOW);
      canvas.fillRect(left, torsoY + bodyH / 2, bodyW, bodyH / 6, C_GREEN);
      canvas.fillRect(left, torsoY + bodyH * 2 / 3, bodyW, bodyH / 6, C_BLUE);
      canvas.fillRect(left, torsoY + bodyH * 5 / 6, bodyW, bodyH / 6, C_PURPLE);
      break;
    case 28:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 4, C_PINK);
      drawHeart(canvas, cx, torsoY + bodyH / 2, bodyW / 3, C_GOLD);
      break;
    case 29:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 4, ICE);
      for (int i = 0; i < 4; ++i) drawStar(canvas, left + bodyW / 5 + i * bodyW / 5,
                                           torsoY + bodyH / 3 + (i & 1) * bodyH / 3,
                                           clampi(bodyW / 14, 2, 4), C_WHITE);
      break;
    case 30:
      canvas.fillRect(left, torsoY, bodyW, bodyH / 2, C_PURPLE);
      canvas.fillTriangle(left, torsoY + bodyH / 2, cx, torsoY + bodyH * 3 / 2,
                          left + bodyW, torsoY + bodyH / 2, TEAL);
      break;
    case 31:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 4, LAVENDER);
      canvas.drawLine(left, torsoY + bodyH / 3, left + bodyW, torsoY + bodyH / 3, C_GOLD);
      break;
    case 32:
      canvas.fillCircle(cx, torsoY + bodyH, bodyW * 2 / 3, rgb(137, 199, 235));
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 4, rgb(137, 199, 235));
      break;
    case 33:
      canvas.fillRect(left, torsoY, bodyW, bodyH / 2, rgb(245, 221, 167));
      canvas.fillRect(left, torsoY + bodyH / 2, bodyW, bodyH / 2, C_RED);
      canvas.drawLine(left, torsoY + bodyH / 2, left + bodyW, torsoY + bodyH / 2, TEAL);
      break;
    case 34:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 5, DARK);
      canvas.drawLine(left, torsoY + bodyH / 4, left + bodyW, torsoY + bodyH / 4, C_GOLD);
      canvas.drawLine(left, torsoY + bodyH * 3 / 4, left + bodyW, torsoY + bodyH * 3 / 4, C_GOLD);
      break;
    case 35:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 4, LIGHT_PINK);
      canvas.fillTriangle(left, torsoY + bodyH, cx, torsoY + bodyH + bodyH / 3,
                          left + bodyW, torsoY + bodyH, LAVENDER);
      break;
    case 36:
      canvas.fillRect(left, torsoY, bodyW, bodyH / 2, ICE);
      canvas.fillTriangle(left, torsoY + bodyH / 2, cx, torsoY + bodyH * 3 / 2,
                          left + bodyW, torsoY + bodyH / 2, TEAL);
      break;
    case 37:
    case 97:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 4, rgb(104, 132, 194));
      for (int i = 0; i < 3; ++i) drawStar(canvas, left + bodyW / 4 + i * bodyW / 4,
                                           torsoY + bodyH / 4 + (i & 1) * bodyH / 3,
                                           clampi(bodyW / 16, 2, 4), C_YELLOW);
      break;
    case 38:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 4, C_PINK);
      for (int i = 0; i < 5; ++i) canvas.fillCircle(left + bodyW / 6 + i * bodyW / 6,
                                                    torsoY + bodyH / 2, 2, C_GOLD);
      break;
    case 88:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 5, C_BLUE);
      canvas.fillRect(left, torsoY + bodyH / 3, bodyW, bodyH / 5, C_WHITE);
      canvas.fillCircle(cx, torsoY + bodyH / 2, bodyW / 9, C_BLACK);
      break;
    case 89:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 5, C_GREEN);
      for (int i = 0; i < 4; ++i) canvas.fillTriangle(left, torsoY + i * bodyH / 4,
                                                      left - bodyW / 4, torsoY + i * bodyH / 4 + bodyH / 8,
                                                      left, torsoY + i * bodyH / 4 + bodyH / 4, C_ORANGE);
      break;
    case 90:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 5, C_BLUE);
      canvas.fillTriangle(cx, torsoY, cx - bodyW / 4, torsoY + bodyH / 2,
                          cx + bodyW / 4, torsoY + bodyH / 2, DARK);
      break;
    case 91:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 5, C_RED);
      canvas.fillRect(left, torsoY + bodyH * 2 / 3, bodyW, bodyH / 3, C_BLUE);
      canvas.drawLine(cx, torsoY, cx, torsoY + bodyH, C_BLACK);
      canvas.drawArc(cx, torsoY + bodyH / 2, bodyW / 3, bodyW / 3, 180, 360, C_BLACK);
      break;
    case 92:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 5, C_WHITE);
      canvas.fillRect(left, torsoY + bodyH / 3, bodyW, bodyH / 5, C_ORANGE);
      break;
    case 93:
      canvas.fillRect(left, torsoY, bodyW, bodyH / 2, C_YELLOW);
      canvas.fillRect(left, torsoY + bodyH / 2, bodyW, bodyH / 2, C_BLUE);
      canvas.fillTriangle(left, torsoY, cx - bodyW / 6, torsoY + bodyH, cx, torsoY, C_WHITE);
      canvas.fillTriangle(left + bodyW, torsoY, cx + bodyW / 6, torsoY + bodyH, cx, torsoY, C_WHITE);
      break;
    case 94:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 5, C_WHITE);
      canvas.fillRect(left, torsoY + bodyH / 5, bodyW, bodyH / 4, C_GREEN);
      canvas.drawLine(left, torsoY + bodyH * 3 / 4, left + bodyW, torsoY + bodyH * 3 / 4, C_PURPLE);
      break;
    case 95:
      canvas.fillRoundRect(left, torsoY, bodyW, bodyH, bodyW / 5, C_TAN);
      canvas.fillRect(left, torsoY + bodyH / 3, bodyW, bodyH / 5, C_GREEN);
      canvas.fillRect(left + bodyW / 8, torsoY + bodyH / 2, bodyW / 4, bodyH / 4, C_BROWN);
      canvas.fillRect(left + bodyW * 5 / 8, torsoY + bodyH / 2, bodyW / 4, bodyH / 4, C_BROWN);
      break;
    case 96:
      canvas.fillRect(left, torsoY, bodyW, bodyH, C_SILVER);
      canvas.fillRect(left + bodyW / 5, torsoY + bodyH / 3, bodyW * 3 / 5, bodyH / 4, DARK);
      canvas.fillCircle(cx, torsoY + bodyH / 2, bodyW / 10, C_BLUE);
      break;
    default:
      break;
  }
}

void drawAccessoryOverlay(M5Canvas& canvas, uint16_t accessoryId, int cx, int headY,
                          int headR, int torsoY, int bodyW, int bodyH) {
  if (accessoryId == 0xFFFF) return;
  switch (accessoryId) {
    case 12: drawCrownShape(canvas, cx, headY - headR + 2, headR * 3 / 2, C_GOLD); break;
    case 13:
      canvas.drawLine(cx + bodyW / 2, torsoY + bodyH / 3, cx + bodyW, torsoY - bodyH / 3, C_PURPLE);
      drawStar(canvas, cx + bodyW, torsoY - bodyH / 3, bodyW / 7, C_GOLD);
      break;
    case 14:
      canvas.drawArc(cx, headY - headR / 2, headR, headR, 190, 350, C_SILVER);
      drawStar(canvas, cx, headY - headR, clampi(headR / 5, 2, 5), ICE);
      break;
    case 15:
      drawBraid(canvas, cx + headR, headY, bodyH + headR, clampi(headR / 2, 4, 12),
                rgb(72, 50, 91), true);
      break;
    case 16:
      drawWingsBehind(canvas, cx, torsoY + bodyH / 3, bodyW * 2, bodyH, C_PINK, ICE);
      break;
    case 17:
      canvas.drawArc(cx, torsoY, bodyW / 2, bodyW / 2, 0, 180, C_GOLD);
      drawStar(canvas, cx, torsoY + bodyW / 2, bodyW / 10, TEAL);
      break;
    case 18:
      for (int i = -2; i <= 2; ++i) drawFlower(canvas, cx + i * headR / 2,
                                               headY - headR, headR / 3,
                                               i & 1 ? C_PINK : C_WHITE, C_YELLOW);
      break;
    case 19:
      canvas.fillTriangle(cx - bodyW / 2, torsoY + bodyH, cx, torsoY + bodyH * 3 / 2,
                          cx, torsoY + bodyH, TEAL);
      canvas.fillTriangle(cx + bodyW / 2, torsoY + bodyH, cx, torsoY + bodyH * 3 / 2,
                          cx, torsoY + bodyH, TEAL);
      break;
    case 20:
      canvas.fillRoundRect(cx - bodyW / 2, torsoY + bodyH, bodyW / 2, bodyH / 4,
                           bodyH / 8, C_PINK);
      canvas.fillRoundRect(cx, torsoY + bodyH, bodyW / 2, bodyH / 4, bodyH / 8, C_PINK);
      drawStar(canvas, cx - bodyW / 4, torsoY + bodyH, 2, C_WHITE);
      drawStar(canvas, cx + bodyW / 4, torsoY + bodyH, 2, C_WHITE);
      break;
    case 21:
      drawStar(canvas, cx - headR / 2, headY, headR / 2, C_PINK);
      drawStar(canvas, cx + headR / 2, headY, headR / 2, C_PINK);
      canvas.drawLine(cx - headR / 6, headY, cx + headR / 6, headY, C_PINK);
      break;
    case 22:
      canvas.drawLine(cx + bodyW / 2, torsoY + bodyH / 2, cx + bodyW, torsoY - bodyH / 3, ICE);
      for (int i = 0; i < 6; ++i) {
        float a = i * 1.0471976f;
        canvas.drawLine(cx + bodyW, torsoY - bodyH / 3,
                        cx + bodyW + static_cast<int>(cosf(a) * bodyW / 5),
                        torsoY - bodyH / 3 + static_cast<int>(sinf(a) * bodyW / 5), C_WHITE);
      }
      break;
    case 23:
      canvas.drawArc(cx, torsoY, bodyW / 2, bodyW / 2, 0, 180, C_WHITE);
      canvas.fillArc(cx, torsoY + bodyW / 2, 0, bodyW / 8, 180, 360, C_PINK);
      break;
    case 24:
      canvas.fillRect(cx + bodyW / 2, torsoY + bodyH / 3, bodyW / 3, bodyH / 2, C_BROWN);
      canvas.fillRect(cx + bodyW / 2 + 2, torsoY + bodyH / 3 + 2,
                      bodyW / 3 - 4, bodyH / 2 - 4, C_YELLOW);
      canvas.drawArc(cx + bodyW * 2 / 3, torsoY + bodyH / 3, bodyW / 6, bodyW / 6,
                     180, 360, C_BROWN);
      break;
    case 25:
      canvas.drawLine(cx + bodyW / 2, torsoY + bodyH / 3, cx + bodyW * 3 / 4,
                      torsoY + bodyH, C_SILVER);
      canvas.fillCircle(cx + bodyW * 3 / 4, torsoY + bodyH, bodyW / 8, DARK);
      break;
    case 26:
      canvas.fillCircle(cx - bodyW * 3 / 4, torsoY + bodyH / 2, bodyW / 5, C_WHITE);
      canvas.fillCircle(cx + bodyW * 3 / 4, torsoY + bodyH / 2, bodyW / 5, C_WHITE);
      break;
    case 72:
      canvas.fillRoundRect(cx - bodyW / 2, torsoY + bodyH, bodyW / 2, bodyH / 4,
                           bodyH / 8, C_BLUE);
      canvas.fillRoundRect(cx, torsoY + bodyH, bodyW / 2, bodyH / 4, bodyH / 8, C_BLUE);
      canvas.drawLine(cx - bodyW / 2, torsoY + bodyH, cx - bodyW / 3, torsoY + bodyH + bodyH / 4, C_WHITE);
      canvas.drawLine(cx + bodyW / 2, torsoY + bodyH, cx + bodyW / 3, torsoY + bodyH + bodyH / 4, C_WHITE);
      break;
    case 73:
      canvas.fillCircle(cx + bodyW, torsoY + bodyH, bodyW / 3, C_WHITE);
      canvas.fillCircle(cx + bodyW, torsoY + bodyH, bodyW / 10, C_BLACK);
      break;
    case 74:
      canvas.fillTriangle(cx - bodyW / 2, torsoY, cx, torsoY + bodyH * 3 / 2,
                          cx + bodyW / 2, torsoY, C_RED);
      break;
    case 75:
      canvas.fillArc(cx, headY, headR * 3 / 4, headR, 180, 360, C_RED);
      canvas.fillTriangle(cx - headR, headY, cx - headR / 5, headY - headR / 2,
                          cx - headR / 4, headY + headR / 2, C_WHITE);
      canvas.fillTriangle(cx + headR, headY, cx + headR / 5, headY - headR / 2,
                          cx + headR / 4, headY + headR / 2, C_WHITE);
      break;
    case 76:
      canvas.fillArc(cx, headY - headR / 2, headR, headR * 5 / 4, 180, 360, C_GREEN);
      for (int i = -2; i <= 2; ++i) canvas.fillTriangle(cx + i * headR / 2, headY - headR,
                                                        cx + i * headR / 2 + headR / 4, headY - headR * 3 / 2,
                                                        cx + i * headR / 2 + headR / 2, headY - headR, C_ORANGE);
      break;
    case 77:
      canvas.fillTriangle(cx, torsoY, cx - bodyW / 3, torsoY + bodyH / 2,
                          cx + bodyW / 3, torsoY + bodyH / 2, C_BLUE);
      break;
    case 78:
      canvas.fillRoundRect(cx - headR, headY - headR - headR / 3, headR * 2,
                           headR / 2, headR / 5, C_TAN);
      canvas.fillRect(cx - headR * 3 / 2, headY - headR, headR * 3, headR / 5, C_TAN);
      break;
    case 79:
      canvas.drawCircle(cx, headY, headR + headR / 3, ICE);
      canvas.drawArc(cx, headY, headR + headR / 2, headR + headR / 2, 180, 360, C_WHITE);
      break;
    case 80:
      canvas.fillCircle(cx + bodyW, torsoY + bodyH / 2, bodyW / 3, C_BLUE);
      drawStar(canvas, cx + bodyW, torsoY + bodyH / 2, bodyW / 5, C_WHITE);
      break;
    case 81:
      canvas.fillRoundRect(cx - bodyW / 2, torsoY + bodyH + bodyH / 4,
                           bodyW, bodyH / 5, bodyH / 10, C_PURPLE);
      canvas.fillCircle(cx - bodyW / 3, torsoY + bodyH + bodyH / 2, bodyH / 10, C_BLACK);
      canvas.fillCircle(cx + bodyW / 3, torsoY + bodyH + bodyH / 2, bodyH / 10, C_BLACK);
      break;
    case 82:
      canvas.fillCircle(cx - bodyW * 3 / 4, torsoY + bodyH / 2, bodyW / 4, C_ORANGE);
      canvas.fillCircle(cx + bodyW * 3 / 4, torsoY + bodyH / 2, bodyW / 4, C_ORANGE);
      break;
    case 83:
      canvas.fillRoundRect(cx - headR, headY - headR - headR / 2, headR * 2,
                           headR / 2, headR / 4, C_BROWN);
      canvas.fillRect(cx - headR * 3 / 2, headY - headR, headR * 3, headR / 4, C_BROWN);
      break;
    case 84: drawStar(canvas, cx + bodyW / 3, torsoY + bodyH / 3, bodyW / 6, C_GOLD); break;
    case 85:
      drawWingsBehind(canvas, cx, torsoY + bodyH / 3, bodyW * 2, bodyH, C_GREEN, C_PURPLE);
      break;
    case 86:
      canvas.fillRoundRect(cx - bodyW * 3 / 4, torsoY + bodyH / 5,
                           bodyW / 2, bodyH * 3 / 4, bodyW / 5, C_GREEN);
      for (int i = 0; i < 3; ++i) canvas.fillTriangle(cx - bodyW * 3 / 4,
                                                      torsoY + bodyH / 4 + i * bodyH / 5,
                                                      cx - bodyW, torsoY + bodyH / 3 + i * bodyH / 5,
                                                      cx - bodyW * 3 / 4, torsoY + bodyH / 2 + i * bodyH / 5, C_ORANGE);
      break;
    case 87:
      canvas.fillArc(cx, headY - headR / 2, headR, headR * 5 / 4, 180, 360, C_BLUE);
      for (int i = -2; i <= 2; ++i) canvas.fillTriangle(cx + i * headR / 3, headY - headR / 2,
                                                        cx + i * headR / 3 + headR / 6, headY - headR / 5,
                                                        cx + i * headR / 3 + headR / 3, headY - headR / 2, C_WHITE);
      break;
    default:
      break;
  }
}

void drawBuddy(uint16_t characterItemId, int cx, int cy, int size, Anim anim,
               uint32_t frameMs, uint16_t accessoryId, uint16_t clothingId) {
  M5Canvas& canvas = Gfx::c();
  const CharLook& look = lookFor(characterItemId);
  BuddyPose pose = poseFor(anim, frameMs, size);
  int headR = clampi(size * 23 / 100, 8, 45);
  int bodyW = clampi(size * 38 / 100, 13, 76);
  int bodyH = clampi(size * 34 / 100, 13, 68);
  int headY = cy - size / 4 + pose.bob;
  int torsoY = headY + headR * 3 / 4;
  cx += pose.sway;

  if (pose.horizontal) {
    int swimCx = cx;
    int swimCy = cy + pose.sway;
    canvas.fillCircle(swimCx + size / 5, swimCy, headR, look.skin);
    canvas.fillRoundRect(swimCx - size / 4, swimCy - bodyH / 2, bodyW, bodyH,
                         bodyH / 2, look.primary);
    drawEyes(canvas, swimCx + size / 4, swimCy - headR / 5, clampi(headR / 4, 2, 8), pose, look.skin,
             look.signature == Signature::Spider);
    drawMouth(canvas, swimCx + size / 4, swimCy + headR / 3, headR / 2, pose);
    canvas.drawLine(swimCx - size / 5, swimCy, swimCx - size / 2, swimCy - size / 5, look.skin);
    canvas.drawLine(swimCx - size / 5, swimCy, swimCx - size / 2, swimCy + size / 5, look.skin);
    canvas.drawLine(swimCx, swimCy + bodyH / 3, swimCx - size / 5, swimCy + size / 3, look.skin);
    drawBubble(canvas, swimCx + size / 2, swimCy - size / 4, clampi(size / 14, 2, 7), ICE);
    drawBubble(canvas, swimCx + size * 3 / 5, swimCy - size / 2, clampi(size / 20, 2, 5), ICE);
    return;
  }

  drawSignatureBehind(canvas, look, cx, headY, headR, torsoY, bodyH);
  if (accessoryId == 16 || accessoryId == 74 || accessoryId == 85) {
    drawAccessoryOverlay(canvas, accessoryId, cx, headY, headR, torsoY, bodyW, bodyH);
  }
  drawHairBack(canvas, look, cx, headY, headR, torsoY + bodyH);

  int armY = torsoY + bodyH / 3;
  int armReach = bodyW / 2 + bodyW / 3;
  int leftHandY = armY + pose.leftArm * bodyH / 4;
  int rightHandY = armY + pose.rightArm * bodyH / 4;
  canvas.drawLine(cx - bodyW / 3, armY, cx - armReach, leftHandY, look.skin);
  canvas.drawLine(cx - bodyW / 3 + 1, armY, cx - armReach + 1, leftHandY, look.skin);
  canvas.drawLine(cx + bodyW / 3, armY, cx + armReach, rightHandY, look.skin);
  canvas.drawLine(cx + bodyW / 3 + 1, armY, cx + armReach + 1, rightHandY, look.skin);
  canvas.fillCircle(cx - armReach, leftHandY, clampi(bodyW / 9, 2, 7), look.skin);
  canvas.fillCircle(cx + armReach, rightHandY, clampi(bodyW / 9, 2, 7), look.skin);

  canvas.fillRoundRect(cx - bodyW / 2 + pose.bodyLean, torsoY, bodyW, bodyH,
                       bodyW / 3, look.primary);
  canvas.fillRect(cx - bodyW / 2 + pose.bodyLean, torsoY + bodyH / 2,
                  bodyW, bodyH / 4, look.secondary);
  drawClothingOverlay(canvas, clothingId, cx + pose.bodyLean, torsoY, bodyW, bodyH);

  int footY = torsoY + bodyH + bodyH / 4;
  int leftFootX = cx - bodyW / 4 + pose.leftLeg * bodyW / 5;
  int rightFootX = cx + bodyW / 4 + pose.rightLeg * bodyW / 5;
  if (pose.rightLeg >= 3) {
    rightFootX += bodyW / 2;
    footY -= bodyH / 2;
  }
  canvas.drawLine(cx - bodyW / 4, torsoY + bodyH, leftFootX, footY, look.secondary);
  canvas.drawLine(cx + bodyW / 4, torsoY + bodyH, rightFootX, footY, look.secondary);
  canvas.fillCircle(leftFootX, footY, clampi(bodyW / 8, 2, 8), look.secondary);
  canvas.fillCircle(rightFootX, footY, clampi(bodyW / 8, 2, 8), look.secondary);

  canvas.fillCircle(cx, headY, headR, look.skin);
  if (look.signature == Signature::Spider) canvas.fillCircle(cx, headY, headR, C_RED);
  if (look.signature == Signature::Robot) canvas.fillRect(cx - headR, headY - headR, headR * 2, headR * 2, C_SILVER);
  drawHairFront(canvas, look, cx, headY, headR);
  drawEyes(canvas, cx, headY, clampi(headR / 4, 2, 9), pose,
           look.signature == Signature::Spider ? C_RED : look.skin,
           look.signature == Signature::Spider);
  if (look.signature != Signature::Spider && look.signature != Signature::Robot) {
    drawMouth(canvas, cx, headY + headR / 2, headR / 2, pose);
  }
  drawSignatureFront(canvas, look, cx, headY, headR, torsoY, bodyW, bodyH, frameMs / 120);
  if (accessoryId != 16 && accessoryId != 74 && accessoryId != 85) {
    drawAccessoryOverlay(canvas, accessoryId, cx, headY, headR, torsoY, bodyW, bodyH);
  }

  if (pose.zzz) {
    int zz = clampi(size / 12, 4, 14);
    for (int i = 0; i < 3; ++i) {
      int x = cx + headR + i * zz;
      int y = headY - headR - i * zz;
      canvas.drawLine(x, y, x + zz, y, C_PURPLE);
      canvas.drawLine(x + zz, y, x, y + zz, C_PURPLE);
      canvas.drawLine(x, y + zz, x + zz, y + zz, C_PURPLE);
    }
  }
  if (pose.chomp) {
    canvas.fillCircle(cx + headR, headY + headR / 2, 2, C_GOLD);
    canvas.fillCircle(cx + headR + bodyW / 4, headY + headR, 2, C_GOLD);
  }
  if (pose.starBurst) drawSparkles(canvas, cx, cy, size * 3 / 5, frameMs / 120, C_GOLD);
}

// -----------------------------------------------------------------------------
// Frankie, dinosaurs, sharks, and sports sprites
// -----------------------------------------------------------------------------

void drawFrankieInternal(M5Canvas& canvas, int cx, int cy, int size, uint32_t frameMs,
                         bool walking) {
  int phase = static_cast<int>(frameMs / 120);
  int step = walking ? ((phase & 1) ? size / 12 : -size / 12) : 0;
  int bodyW = size * 3 / 5;
  int bodyH = size * 2 / 5;
  int headR = size / 4;
  canvas.fillRoundRect(cx - bodyW / 2, cy - bodyH / 3, bodyW, bodyH, bodyH / 2, C_TAN);
  canvas.fillCircle(cx + bodyW / 3, cy - bodyH / 3, headR, C_TAN);
  canvas.fillTriangle(cx + bodyW / 5, cy - bodyH / 2, cx + bodyW / 4, cy - bodyH,
                      cx + bodyW / 2, cy - bodyH / 2, C_BROWN);
  canvas.fillTriangle(cx + bodyW / 2, cy - bodyH / 2, cx + bodyW * 3 / 5, cy - bodyH,
                      cx + bodyW * 3 / 4, cy - bodyH / 3, C_BROWN);
  canvas.fillArc(cx + bodyW / 3, cy - bodyH / 4, headR / 2, headR * 3 / 4, 180, 360, C_BROWN);
  canvas.fillCircle(cx + bodyW / 2, cy - bodyH / 4, headR / 6, C_BLACK);
  canvas.fillCircle(cx + bodyW / 5, cy - bodyH / 2, headR / 5, C_WHITE);
  canvas.fillCircle(cx + bodyW / 2, cy - bodyH / 2, headR / 5, C_WHITE);
  canvas.fillCircle(cx + bodyW / 5, cy - bodyH / 2, clampi(headR / 10, 1, 3), C_BLACK);
  canvas.fillCircle(cx + bodyW / 2, cy - bodyH / 2, clampi(headR / 10, 1, 3), C_BLACK);
  int legY = cy + bodyH / 2;
  canvas.drawLine(cx - bodyW / 3, cy + bodyH / 4, cx - bodyW / 3 + step, legY, C_BROWN);
  canvas.drawLine(cx, cy + bodyH / 4, cx - step, legY, C_BROWN);
  canvas.drawLine(cx + bodyW / 4, cy + bodyH / 4, cx + bodyW / 4 + step, legY, C_BROWN);
  canvas.drawLine(cx + bodyW / 2, cy + bodyH / 4, cx + bodyW / 2 - step, legY, C_BROWN);
  canvas.drawArc(cx - bodyW / 2, cy - bodyH / 4, bodyW / 4, bodyW / 4, 160, 300, C_TAN);
}

void drawDinoInternal(M5Canvas& canvas, uint8_t dinoId, int cx, int cy, int size,
                      uint16_t frameMs, bool silhouette) {
  dinoId %= 12;
  uint16_t bodyColours[12] = {rgb(76, 151, 68), rgb(93, 170, 79), rgb(79, 151, 102),
    rgb(77, 142, 96), rgb(112, 166, 65), rgb(162, 108, 65), rgb(87, 133, 79),
    rgb(80, 139, 100), rgb(89, 162, 111), rgb(180, 86, 53), rgb(95, 150, 104), rgb(147, 132, 67)};
  uint16_t body = silhouette ? rgb(35, 42, 55) : bodyColours[dinoId];
  uint16_t belly = silhouette ? body : rgb(181, 211, 121);
  int phase = frameMs / 120;
  int stride = (phase & 1) ? size / 14 : -size / 14;
  int bodyW = size * 3 / 5;
  int bodyH = size / 3;
  int headR = size / 6;
  int bodyY = cy;

  if (dinoId == 3 || dinoId == 10) {
    int neckX = cx + bodyW / 3;
    int neckTop = cy - size * 2 / 3;
    canvas.fillRoundRect(neckX, neckTop, size / 5, size * 2 / 3, size / 10, body);
    canvas.fillCircle(neckX + size / 10, neckTop, headR, body);
    canvas.fillCircle(neckX + size / 7, neckTop - headR / 4, silhouette ? 0 : clampi(headR / 7, 1, 3), C_BLACK);
    bodyY += size / 5;
  }

  canvas.fillRoundRect(cx - bodyW / 2, bodyY - bodyH / 2, bodyW, bodyH, bodyH / 2, body);
  canvas.fillArc(cx, bodyY + bodyH / 4, bodyH / 3, bodyW / 3, 0, 180, belly);
  canvas.fillTriangle(cx - bodyW / 2, bodyY - bodyH / 4, cx - bodyW, bodyY,
                      cx - bodyW / 3, bodyY + bodyH / 4, body);

  if (dinoId != 3 && dinoId != 10 && dinoId != 5) {
    int headX = cx + bodyW / 2;
    int headY = bodyY - bodyH / 4;
    canvas.fillCircle(headX, headY, headR, body);
    if (dinoId == 1) {
      canvas.fillCircle(headX - headR / 2, headY, headR, body);
      canvas.fillTriangle(headX, headY - headR, headX + headR / 3, headY - headR * 2,
                          headX + headR / 2, headY - headR / 2, body);
      canvas.fillTriangle(headX + headR / 2, headY - headR / 2, headX + headR * 3 / 2,
                          headY - headR, headX + headR / 2, headY, body);
    }
    if (dinoId == 8) {
      canvas.fillRoundRect(headX - headR, headY - headR, headR * 2, headR / 2,
                           headR / 4, body);
    }
    if (dinoId == 9) {
      canvas.fillTriangle(headX, headY - headR, headX + headR / 2, headY - headR * 2,
                          headX + headR / 2, headY - headR / 2, body);
      canvas.fillTriangle(headX + headR / 3, headY - headR, headX + headR, headY - headR * 3 / 2,
                          headX + headR, headY - headR / 2, body);
    }
    if (dinoId == 11) canvas.fillCircle(headX, headY - headR / 2, headR * 4 / 5, body);
    if (!silhouette) {
      canvas.fillCircle(headX + headR / 3, headY - headR / 3, clampi(headR / 7, 1, 3), C_WHITE);
      canvas.fillCircle(headX + headR / 3, headY - headR / 3, clampi(headR / 12, 1, 2), C_BLACK);
      canvas.drawLine(headX + headR / 4, headY + headR / 3, headX + headR, headY + headR / 3, C_BLACK);
    }
  }

  if (dinoId == 2) {
    for (int i = 0; i < 5; ++i) {
      int x = cx - bodyW / 3 + i * bodyW / 6;
      canvas.fillTriangle(x, bodyY - bodyH / 2, x + bodyW / 12, bodyY - bodyH,
                          x + bodyW / 6, bodyY - bodyH / 2, silhouette ? body : C_ORANGE);
    }
  } else if (dinoId == 6) {
    for (int i = 0; i < 4; ++i) {
      int x = cx - bodyW / 3 + i * bodyW / 5;
      canvas.fillTriangle(x, bodyY - bodyH / 2, x + bodyW / 10, bodyY - bodyH * 3 / 4,
                          x + bodyW / 5, bodyY - bodyH / 2, body);
    }
    canvas.fillCircle(cx - bodyW / 4, bodyY, bodyH / 3, body);
  } else if (dinoId == 7) {
    canvas.fillTriangle(cx - bodyW / 3, bodyY - bodyH / 2, cx, bodyY - size * 2 / 3,
                        cx + bodyW / 3, bodyY - bodyH / 2, silhouette ? body : rgb(208, 113, 65));
  }

  if (dinoId == 5) {
    canvas.fillCircle(cx + bodyW / 3, cy - bodyH / 2, headR, body);
    canvas.fillTriangle(cx - bodyW / 3, cy, cx - bodyW, cy - size / 2,
                        cx, cy - bodyH / 4, body);
    canvas.fillTriangle(cx + bodyW / 4, cy, cx + bodyW, cy - size / 2,
                        cx, cy - bodyH / 4, body);
    canvas.fillTriangle(cx - bodyW / 3, cy, cx - bodyW, cy + size / 3,
                        cx, cy + bodyH / 4, body);
    canvas.fillTriangle(cx + bodyW / 4, cy, cx + bodyW, cy + size / 3,
                        cx, cy + bodyH / 4, body);
  } else {
    int legY = bodyY + bodyH / 2;
    canvas.fillRoundRect(cx - bodyW / 3 + stride, legY, size / 9, size / 3,
                         size / 18, body);
    canvas.fillRoundRect(cx + bodyW / 5 - stride, legY, size / 9, size / 3,
                         size / 18, body);
    if ((dinoId == 4 || dinoId == 9) && !silhouette) {
      canvas.fillTriangle(cx + bodyW / 5 - stride, legY + size / 3,
                          cx + bodyW / 2, legY + size / 3 + size / 8,
                          cx + bodyW / 3, legY + size / 4, C_WHITE);
    }
  }
}

void drawSharkInternal(M5Canvas& canvas, uint8_t sharkId, int cx, int cy, int size,
                       uint32_t frameMs) {
  sharkId %= 6;
  uint16_t tops[6] = {C_BLUE, rgb(77, 143, 164), GREY, rgb(185, 145, 68), rgb(77, 169, 212), C_SILVER};
  uint16_t top = tops[sharkId];
  int phase = frameMs / 120;
  int tailSwing = (phase & 1) ? size / 12 : -size / 12;
  int bodyW = sharkId == 4 ? size / 2 : size * 3 / 4;
  int bodyH = sharkId == 4 ? size / 3 : size * 2 / 5;
  canvas.fillEllipse(cx, cy, bodyW / 2, bodyH / 2, top);
  canvas.fillArc(cx, cy + bodyH / 6, bodyH / 3, bodyW / 2, 0, 180, C_WHITE);
  canvas.fillTriangle(cx - bodyW / 2, cy, cx - bodyW, cy - bodyH / 2 + tailSwing,
                      cx - bodyW, cy + bodyH / 2 + tailSwing, top);
  canvas.fillTriangle(cx, cy - bodyH / 3, cx - bodyW / 5, cy - bodyH,
                      cx + bodyW / 5, cy - bodyH / 3, top);
  canvas.fillTriangle(cx - bodyW / 8, cy + bodyH / 4, cx + bodyW / 5, cy + bodyH,
                      cx + bodyW / 4, cy + bodyH / 4, top);

  if (sharkId == 1) {
    canvas.fillRoundRect(cx + bodyW / 3, cy - bodyH / 2, bodyW / 2, bodyH,
                         bodyH / 3, top);
    canvas.fillCircle(cx + bodyW / 3, cy - bodyH / 3, clampi(size / 32, 1, 3), C_BLACK);
    canvas.fillCircle(cx + bodyW * 5 / 6, cy - bodyH / 3, clampi(size / 32, 1, 3), C_BLACK);
  } else {
    int eyeR = sharkId == 4 ? clampi(size / 16, 3, 9) : clampi(size / 25, 2, 6);
    canvas.fillCircle(cx + bodyW / 3, cy - bodyH / 5, eyeR, C_WHITE);
    canvas.fillCircle(cx + bodyW / 3, cy - bodyH / 5, clampi(eyeR / 2, 1, 4), C_BLACK);
  }
  if (sharkId == 3) {
    for (int i = 0; i < 4; ++i) {
      int x = cx - bodyW / 4 + i * bodyW / 7;
      canvas.drawLine(x, cy - bodyH / 2, x + bodyW / 10, cy + bodyH / 3, C_BROWN);
    }
  }
  if (sharkId == 5) {
    canvas.drawLine(cx - bodyW / 4, cy - bodyH / 2, cx - bodyW / 4, cy + bodyH / 2, DARK);
    canvas.drawLine(cx + bodyW / 8, cy - bodyH / 2, cx + bodyW / 8, cy + bodyH / 2, DARK);
    canvas.drawLine(cx, cy - bodyH, cx, cy - bodyH - size / 5, DARK);
    canvas.fillCircle(cx, cy - bodyH - size / 5, clampi(size / 18, 2, 6), C_RED);
    canvas.fillCircle(cx + bodyW / 3, cy - bodyH / 5, clampi(size / 24, 2, 5), C_RED);
  }
  canvas.drawLine(cx + bodyW / 4, cy + bodyH / 5, cx + bodyW / 2, cy + bodyH / 5, C_BLACK);
  drawBubble(canvas, cx + bodyW * 2 / 3, cy - bodyH / 2, clampi(size / 16, 2, 6), ICE);
}

// -----------------------------------------------------------------------------
// Item preview helpers
// -----------------------------------------------------------------------------

void drawMannequin(M5Canvas& canvas, int cx, int cy, int size, uint16_t accessoryId,
                    uint16_t clothingId) {
  int headR = size / 5;
  int bodyW = size * 2 / 5;
  int bodyH = size * 2 / 5;
  int headY = cy - size / 4;
  int torsoY = headY + headR;
  canvas.fillCircle(cx, headY, headR, rgb(230, 205, 183));
  canvas.fillRoundRect(cx - bodyW / 2, torsoY, bodyW, bodyH, bodyW / 4, rgb(180, 190, 202));
  canvas.fillCircle(cx - headR / 2, headY, clampi(headR / 6, 2, 5), C_WHITE);
  canvas.fillCircle(cx + headR / 2, headY, clampi(headR / 6, 2, 5), C_WHITE);
  canvas.fillCircle(cx - headR / 2, headY, clampi(headR / 12, 1, 3), C_BLACK);
  canvas.fillCircle(cx + headR / 2, headY, clampi(headR / 12, 1, 3), C_BLACK);
  drawClothingOverlay(canvas, clothingId, cx, torsoY, bodyW, bodyH);
  drawAccessoryOverlay(canvas, accessoryId, cx, headY, headR, torsoY, bodyW, bodyH);
}

void drawToyPreview(M5Canvas& canvas, uint16_t itemId, int cx, int cy, int size,
                    uint32_t frameMs) {
  int s = size;
  switch (itemId) {
    case 39:
      canvas.fillRect(cx - s / 3, cy - s / 5, s * 2 / 3, s / 2, C_PINK);
      for (int side = -1; side <= 1; side += 2) {
        canvas.fillRect(cx + side * s / 3 - s / 8, cy - s / 2, s / 4, s / 2, LAVENDER);
        canvas.fillTriangle(cx + side * s / 3 - s / 8, cy - s / 2,
                            cx + side * s / 3, cy - s * 3 / 4,
                            cx + side * s / 3 + s / 8, cy - s / 2, C_GOLD);
      }
      canvas.fillRoundRect(cx - s / 10, cy + s / 8, s / 5, s / 4, s / 10, DARK);
      break;
    case 40:
      canvas.fillEllipse(cx, cy + s / 5, s / 3, s / 8, C_WHITE);
      canvas.fillRoundRect(cx - s / 6, cy - s / 5, s / 3, s / 3, s / 8, C_PINK);
      canvas.drawArc(cx + s / 5, cy - s / 20, s / 6, s / 6, 270, 90, C_PINK);
      canvas.fillCircle(cx + s / 3, cy + s / 5, s / 8, ICE);
      break;
    case 41:
      canvas.drawLine(cx - s / 3, cy + s / 3, cx + s / 4, cy - s / 3, C_PURPLE);
      drawStar(canvas, cx + s / 3, cy - s / 2, s / 4, C_GOLD);
      break;
    case 42:
      canvas.fillCircle(cx, cy - s / 5, s / 4, C_WHITE);
      canvas.fillRoundRect(cx - s / 4, cy, s / 2, s / 3, s / 6, C_WHITE);
      canvas.fillTriangle(cx - s / 8, cy - s * 2 / 5, cx, cy - s * 3 / 4,
                          cx + s / 8, cy - s * 2 / 5, C_GOLD);
      canvas.fillCircle(cx + s / 8, cy - s / 5, s / 16, C_BLACK);
      break;
    case 43:
      drawBuddy(0, cx, cy, s * 3 / 4, Anim::Idle, frameMs, 0xFFFF, 28);
      break;
    case 44:
      canvas.fillRoundRect(cx - s / 3, cy - s / 4, s * 2 / 3, s / 2, s / 10, LAVENDER);
      canvas.fillRect(cx - s / 4, cy - s / 6, s / 2, s / 3, C_GOLD);
      drawStar(canvas, cx, cy, s / 7, C_PINK);
      canvas.drawLine(cx + s / 4, cy - s / 4, cx + s / 3, cy - s / 2, C_SILVER);
      break;
    case 45:
    case 104:
      canvas.fillRoundRect(cx - s / 4, cy - s / 3, s / 2, s * 2 / 3, s / 8, ICE);
      canvas.fillRect(cx - s / 3, cy - s / 2, s * 2 / 3, s / 6, C_GOLD);
      for (int i = 0; i < 6; ++i) {
        uint16_t colours[6] = {C_RED, C_BLUE, C_GREEN, C_YELLOW, C_PINK, C_PURPLE};
        canvas.fillCircle(cx - s / 7 + (i % 3) * s / 7,
                          cy - s / 8 + (i / 3) * s / 4, s / 16, colours[i]);
      }
      break;
    case 46:
      canvas.fillCircle(cx, cy - s / 10, s / 3, ICE);
      canvas.fillRect(cx - s / 3, cy + s / 5, s * 2 / 3, s / 5, C_PURPLE);
      drawCloud(canvas, cx, cy, s / 3, C_WHITE);
      drawStar(canvas, cx, cy - s / 5, s / 10, C_GOLD);
      break;
    case 47:
      canvas.fillArc(cx, cy + s / 5, s / 6, s / 2, 180, 360, C_PINK);
      for (int i = -2; i <= 2; ++i) canvas.drawLine(cx, cy + s / 5,
                                                    cx + i * s / 6, cy - s / 4, C_WHITE);
      break;
    case 48:
      drawWingsBehind(canvas, cx, cy, s, s * 2 / 3, C_PINK, ICE);
      canvas.fillCircle(cx, cy, s / 10, DARK);
      break;
    case 98:
      canvas.fillCircle(cx, cy, s / 3, C_WHITE);
      for (int i = 0; i < 5; ++i) {
        float angle = i * 1.256637f - 1.5707963f;
        canvas.fillCircle(cx + static_cast<int>(cosf(angle) * s / 5),
                          cy + static_cast<int>(sinf(angle) * s / 5), s / 12, C_BLACK);
      }
      break;
    case 99:
      drawDinoInternal(canvas, 0, cx - s / 5, cy, s / 2, frameMs, false);
      drawDinoInternal(canvas, 1, cx + s / 4, cy + s / 5, s / 3, frameMs, false);
      break;
    case 100: drawSharkInternal(canvas, 0, cx, cy, s, frameMs); break;
    case 101:
      canvas.fillRect(cx - s / 4, cy - s / 3, s / 2, s / 2, C_SILVER);
      canvas.fillRect(cx - s / 3, cy + s / 6, s * 2 / 3, s / 3, GREY);
      canvas.fillCircle(cx - s / 9, cy - s / 6, s / 14, C_BLUE);
      canvas.fillCircle(cx + s / 9, cy - s / 6, s / 14, C_BLUE);
      canvas.drawLine(cx, cy - s / 3, cx, cy - s / 2, DARK);
      canvas.fillCircle(cx, cy - s / 2, s / 16, C_RED);
      break;
    case 102:
      canvas.fillRoundRect(cx - s / 2, cy - s / 10, s, s / 5, s / 10, C_PURPLE);
      canvas.fillCircle(cx - s / 3, cy + s / 6, s / 10, C_BLACK);
      canvas.fillCircle(cx + s / 3, cy + s / 6, s / 10, C_BLACK);
      break;
    case 103:
      canvas.fillRoundRect(cx - s / 2, cy - s / 5, s, s / 3, s / 8, C_RED);
      canvas.fillRect(cx - s / 5, cy - s / 2, s * 2 / 5, s / 3, ICE);
      canvas.fillCircle(cx - s / 3, cy + s / 6, s / 8, C_BLACK);
      canvas.fillCircle(cx + s / 3, cy + s / 6, s / 8, C_BLACK);
      break;
    case 105:
      for (int i = 0; i < 3; ++i) canvas.drawCircle(cx, cy, s / 4 + i * s / 12, C_BROWN);
      canvas.drawLine(cx + s / 3, cy + s / 3, cx + s / 2, cy + s / 2, C_BROWN);
      break;
    case 106:
      canvas.fillRoundRect(cx - s / 5, cy - s / 2, s * 2 / 5, s * 3 / 4, s / 5, C_WHITE);
      canvas.fillTriangle(cx - s / 5, cy - s / 3, cx, cy - s * 3 / 4,
                          cx + s / 5, cy - s / 3, C_RED);
      canvas.fillTriangle(cx - s / 5, cy + s / 5, cx - s / 2, cy + s / 2,
                          cx - s / 5, cy + s / 2, C_BLUE);
      canvas.fillTriangle(cx + s / 5, cy + s / 5, cx + s / 2, cy + s / 2,
                          cx + s / 5, cy + s / 2, C_BLUE);
      canvas.fillTriangle(cx - s / 8, cy + s / 4, cx, cy + s * 3 / 4,
                          cx + s / 8, cy + s / 4, C_ORANGE);
      break;
    case 107:
      canvas.drawRect(cx - s / 2, cy - s / 2, s, s, C_WHITE);
      for (int i = 1; i < 4; ++i) canvas.drawLine(cx - s / 2, cy - s / 2 + i * s / 4,
                                                  cx + s / 2, cy - s / 2 + i * s / 4, GREY);
      for (int i = 1; i < 4; ++i) canvas.drawLine(cx - s / 2 + i * s / 4, cy - s / 2,
                                                  cx - s / 2 + i * s / 4, cy + s / 2, GREY);
      break;
    case 108:
      canvas.fillEllipse(cx, cy, s / 3, s / 2, rgb(214, 196, 151));
      for (int i = 0; i < 6; ++i) canvas.fillCircle(cx - s / 6 + (i % 3) * s / 6,
                                                    cy - s / 4 + (i / 3) * s / 3,
                                                    s / 20, C_GREEN);
      break;
    case 109:
      canvas.fillRoundRect(cx - s / 2, cy - s / 4, s, s / 2, s / 12, C_BROWN);
      canvas.fillArc(cx, cy - s / 4, s / 4, s / 2, 180, 360, C_TAN);
      canvas.fillRect(cx - s / 10, cy - s / 10, s / 5, s / 4, C_GOLD);
      break;
    default:
      break;
  }
}

void drawTreatPreview(M5Canvas& canvas, uint16_t itemId, int cx, int cy, int size) {
  switch (itemId) {
    case 121:
      canvas.fillRoundRect(cx - size / 3, cy - size / 2, size * 2 / 3, size,
                           size / 8, C_PINK);
      for (int i = 0; i < 5; ++i) canvas.fillCircle(cx - size / 5 + (i % 3) * size / 5,
                                                    cy - size / 5 + (i / 3) * size / 3,
                                                    size / 12, i & 1 ? C_YELLOW : C_PURPLE);
      break;
    case 122:
      canvas.fillCircle(cx, cy, size / 3, C_RED);
      canvas.fillCircle(cx + size / 5, cy - size / 8, size / 4, C_RED);
      canvas.drawLine(cx, cy - size / 3, cx + size / 8, cy - size / 2, C_BROWN);
      canvas.fillEllipse(cx + size / 5, cy - size / 2, size / 5, size / 10, C_GREEN);
      break;
    case 123:
      canvas.fillTriangle(cx - size / 3, cy, cx + size / 3, cy, cx + size / 4,
                          cy + size / 2, C_ORANGE);
      canvas.fillCircle(cx, cy - size / 8, size / 3, C_PINK);
      canvas.fillCircle(cx, cy - size / 3, size / 6, C_WHITE);
      break;
    case 124:
      canvas.fillTriangle(cx - size / 4, cy, cx + size / 4, cy, cx, cy + size / 2, C_ORANGE);
      canvas.fillCircle(cx, cy - size / 5, size / 3, ICE);
      canvas.fillCircle(cx - size / 8, cy - size / 3, size / 5, C_PINK);
      break;
    case 125:
      canvas.fillCircle(cx, cy, size / 2, rgb(205, 143, 72));
      for (int i = 0; i < 6; ++i) {
        float angle = i * 1.0471976f;
        canvas.fillCircle(cx + static_cast<int>(cosf(angle) * size / 4),
                          cy + static_cast<int>(sinf(angle) * size / 4), size / 18, C_BROWN);
      }
      break;
    case 126:
      canvas.fillTriangle(cx - size / 3, cy - size / 4, cx + size / 3, cy - size / 4,
                          cx, cy + size / 2, C_RED);
      canvas.fillCircle(cx, cy - size / 8, size / 3, C_RED);
      for (int i = -1; i <= 1; ++i) canvas.fillTriangle(cx + i * size / 6, cy - size / 3,
                                                        cx + i * size / 6 + size / 8, cy - size / 2,
                                                        cx + i * size / 6 + size / 4, cy - size / 3, C_GREEN);
      break;
  }
}

// -----------------------------------------------------------------------------
// Room rendering
// -----------------------------------------------------------------------------

void drawGradient(M5Canvas& canvas, uint16_t top, uint16_t bottom, int horizon) {
  for (int i = 0; i < 12; ++i) {
    uint8_t tr = static_cast<uint8_t>((top >> 11) << 3);
    uint8_t tg = static_cast<uint8_t>(((top >> 5) & 0x3F) << 2);
    uint8_t tb = static_cast<uint8_t>((top & 0x1F) << 3);
    uint8_t br = static_cast<uint8_t>((bottom >> 11) << 3);
    uint8_t bg = static_cast<uint8_t>(((bottom >> 5) & 0x3F) << 2);
    uint8_t bb = static_cast<uint8_t>((bottom & 0x1F) << 3);
    uint16_t colour = rgb(tr + (br - tr) * i / 11, tg + (bg - tg) * i / 11,
                          tb + (bb - tb) * i / 11);
    int y0 = i * horizon / 12;
    int y1 = (i + 1) * horizon / 12;
    canvas.fillRect(0, y0, 320, y1 - y0 + 1, colour);
  }
}

void drawCastleMotif(M5Canvas& canvas, uint16_t wall, uint16_t roof) {
  canvas.fillRect(72, 102, 176, 105, wall);
  canvas.fillRect(45, 82, 55, 125, wall);
  canvas.fillRect(220, 82, 55, 125, wall);
  canvas.fillTriangle(40, 82, 72, 42, 105, 82, roof);
  canvas.fillTriangle(215, 82, 247, 42, 280, 82, roof);
  canvas.fillTriangle(120, 102, 160, 55, 200, 102, roof);
  canvas.fillRoundRect(141, 158, 38, 49, 18, DARK);
  canvas.fillRect(66, 112, 13, 24, ICE);
  canvas.fillRect(241, 112, 13, 24, ICE);
  canvas.drawLine(72, 42, 72, 27, DARK);
  canvas.fillTriangle(72, 27, 94, 34, 72, 40, C_GOLD);
}

void drawPalm(M5Canvas& canvas, int x, int y, int height) {
  canvas.drawLine(x, y, x + height / 8, y - height, C_BROWN);
  int tx = x + height / 8;
  int ty = y - height;
  for (int i = 0; i < 6; ++i) {
    float a = -2.8f + i * 0.55f;
    canvas.drawLine(tx, ty, tx + static_cast<int>(cosf(a) * height / 2),
                    ty + static_cast<int>(sinf(a) * height / 3), C_GREEN);
    canvas.drawLine(tx + 1, ty, tx + 1 + static_cast<int>(cosf(a) * height / 2),
                    ty + static_cast<int>(sinf(a) * height / 3), C_GREEN);
  }
}

void drawRoomMini(M5Canvas& canvas, uint16_t roomId, int cx, int cy, int size) {
  int w = size;
  int h = size * 3 / 4;
  uint16_t sky = roomId == 113 ? C_BLUE : (roomId == 115 || roomId == 117 ? DARK : ICE);
  canvas.fillRoundRect(cx - w / 2, cy - h / 2, w, h, w / 10, sky);
  canvas.drawRoundRect(cx - w / 2, cy - h / 2, w, h, w / 10, C_WHITE);
  switch (roomId) {
    case 49: drawCrownShape(canvas, cx, cy, w / 2, C_GOLD); break;
    case 50: canvas.fillRoundRect(cx - w / 3, cy, w * 2 / 3, h / 3, h / 8, C_PINK); break;
    case 51: case 59: drawFlower(canvas, cx, cy, w / 3, C_PINK, C_YELLOW); break;
    case 52: drawCloud(canvas, cx, cy, w / 2, C_WHITE); drawWaves(canvas, cy + h / 4, h / 12, C_PINK, cx - w / 2, cx + w / 2); break;
    case 53: case 113: drawWaves(canvas, cy, h / 10, TEAL, cx - w / 2, cx + w / 2); drawBubble(canvas, cx, cy - h / 5, h / 10, C_WHITE); break;
    case 54: canvas.fillRect(cx - w / 3, cy, w * 2 / 3, h / 4, C_PURPLE); canvas.drawLine(cx, cy, cx, cy - h / 3, C_SILVER); break;
    case 55: canvas.fillTriangle(cx - w / 3, cy + h / 4, cx, cy - h / 3, cx + w / 3, cy + h / 4, C_WHITE); break;
    case 56: canvas.fillRect(cx - w / 7, cy - h / 3, w * 2 / 7, h * 2 / 3, C_TAN); canvas.fillTriangle(cx - w / 5, cy - h / 3, cx, cy - h / 2, cx + w / 5, cy - h / 3, C_PURPLE); break;
    case 57: case 112: drawPalm(canvas, cx, cy + h / 3, h / 2); break;
    case 58: case 119: drawTreatPreview(canvas, 125, cx, cy, w / 2); break;
    case 110: canvas.fillRect(cx - w / 2, cy + h / 5, w, h / 4, C_GREEN); canvas.fillCircle(cx, cy, h / 7, C_WHITE); break;
    case 111: drawDinoInternal(canvas, 0, cx, cy, w / 2, 0, false); break;
    case 114: for (int i = -1; i <= 1; ++i) canvas.fillRect(cx + i * w / 4 - w / 12, cy - h / 3 + absi(i) * h / 5, w / 6, h * 2 / 3, GREY); break;
    case 115: case 117: canvas.fillCircle(cx, cy, h / 4, C_BLUE); drawStar(canvas, cx + w / 3, cy - h / 4, h / 10, C_WHITE); break;
    case 116: canvas.fillRect(cx - w / 2, cy, w, h / 3, C_TAN); canvas.fillTriangle(cx - w / 4, cy, cx - w / 8, cy - h / 3, cx, cy, C_BROWN); break;
    case 118: canvas.fillRect(cx - w / 3, cy - h / 4, w * 2 / 3, h / 2, C_SILVER); canvas.fillCircle(cx, cy, h / 8, C_BLUE); break;
    case 120: drawFrankieInternal(canvas, cx, cy, w / 2, 0, false); break;
  }
}

// ---------- Pixel-art bitmap rendering (generated assets) ----------
// Nearest-neighbour scaled blit with transparency key; optional 90-degree
// rotation (swimming) and flat-colour override (silhouettes).
void blitArt(M5Canvas& canvas, const PixelArt* art, int cx, int cy, int size,
             bool rot90 = false, uint16_t flatColor = 0, bool useFlat = false) {
  if (art == nullptr || art->h == 0) return;
  const int dh = size;
  const int dw = size * art->w / art->h;
  const int x0 = cx - dw / 2;
  const int y0 = cy - dh / 2;
  for (int dy = 0; dy < dh; ++dy) {
    const int sy = dy * art->h / dh;
    for (int dx = 0; dx < dw; ++dx) {
      const int sx = dx * art->w / dw;
      const uint16_t p = rot90 ? art->px[(art->h - 1 - sx) * art->w + sy]
                               : art->px[sy * art->w + sx];
      if (p != ART_TRANSPARENT) {
        canvas.drawPixel(x0 + dx, y0 + dy, useFlat ? flatColor : p);
      }
    }
  }
}

void drawBitmapBuddy(M5Canvas& canvas, const PixelArt* art, int cx, int cy, int size,
                     Anim anim, uint32_t frameMs, uint16_t accessoryId,
                     uint16_t clothingId) {
  (void)clothingId;  // bitmap characters wear their painted outfit
  BuddyPose pose = poseFor(anim, frameMs, size);
  const int bx = cx + pose.sway;
  const int by = cy + pose.bob;
  // same geometry the procedural body uses, so overlays land in place
  const int headR = clampi(size * 23 / 100, 8, 45);
  const int bodyW = clampi(size * 38 / 100, 13, 76);
  const int bodyH = clampi(size * 34 / 100, 13, 68);
  const int headY = by - size / 4;
  const int torsoY = headY + headR * 3 / 4;

  const bool backAccessory = accessoryId == 16 || accessoryId == 74 || accessoryId == 85;
  if (backAccessory) {
    drawAccessoryOverlay(canvas, accessoryId, bx, headY, headR, torsoY, bodyW, bodyH);
  }
  blitArt(canvas, art, bx, by, size, pose.horizontal);
  if (!backAccessory && accessoryId != 0xFFFF) {
    drawAccessoryOverlay(canvas, accessoryId, bx, headY, headR, torsoY, bodyW, bodyH);
  }

  if (pose.zzz) {
    const int zz = clampi(size / 12, 4, 14);
    for (int i = 0; i < 3; ++i) {
      const int x = bx + headR + i * zz;
      const int y = headY - headR - i * zz;
      canvas.drawLine(x, y, x + zz, y, C_PURPLE);
      canvas.drawLine(x + zz, y, x, y + zz, C_PURPLE);
      canvas.drawLine(x, y + zz, x + zz, y + zz, C_PURPLE);
    }
  }
  if (pose.chomp) {
    canvas.fillCircle(bx + headR, headY + headR / 2, 2, C_GOLD);
    canvas.fillCircle(bx + headR + bodyW / 4, headY + headR, 2, C_GOLD);
  }
  if (pose.starBurst) drawSparkles(canvas, bx, by, size * 3 / 5, frameMs / 120, C_GOLD);
}

}  // namespace

namespace Sprites {

void drawCharacter(uint16_t characterItemId, int cx, int cy, int size, Anim anim,
                   uint32_t frameMs, uint16_t accessoryId, uint16_t clothingId) {
  const PixelArt* art = artForItem(characterItemId);
  if (art != nullptr) {
    drawBitmapBuddy(Gfx::c(), art, cx, cy, size, anim, frameMs, accessoryId, clothingId);
    return;
  }
  drawBuddy(characterItemId, cx, cy, size, anim, frameMs, accessoryId, clothingId);
}

void drawFrankie(int cx, int cy, int size, uint32_t frameMs, bool walking) {
  drawFrankieInternal(Gfx::c(), cx, cy, size, frameMs, walking);
}

void drawDino(uint8_t dinoId, int cx, int cy, int size, uint16_t frameMs) {
  const PixelArt* art = artForDino(dinoId);
  if (art != nullptr) {
    const int bob = (frameMs / 300) % 2 == 0 ? 0 : clampi(size / 24, 1, 3);
    blitArt(Gfx::c(), art, cx, cy + bob, size);
    return;
  }
  drawDinoInternal(Gfx::c(), dinoId, cx, cy, size, frameMs, false);
}

void drawDinoSilhouette(uint8_t dinoId, int cx, int cy, int size) {
  const PixelArt* art = artForDino(dinoId);
  if (art != nullptr) {
    blitArt(Gfx::c(), art, cx, cy, size, false, DARK, true);
    return;
  }
  drawDinoInternal(Gfx::c(), dinoId, cx, cy, size, 0, true);
}

void drawShark(uint8_t sharkId, int cx, int cy, int size, uint32_t frameMs) {
  const PixelArt* art = artForShark(sharkId);
  if (art != nullptr) {
    const int bob = static_cast<int>((frameMs / 250) % 3) - 1;
    blitArt(Gfx::c(), art, cx, cy + bob, size);
    return;
  }
  drawSharkInternal(Gfx::c(), sharkId, cx, cy, size, frameMs);
}

void drawGoalie(int cx, int cy, int size, int diveDir, float diveT) {
  M5Canvas& canvas = Gfx::c();
  float t = clampf(diveT, 0.0f, 1.0f);
  {
    const PixelArt* art = artGoalie();
    if (art != nullptr) {
      const int dir0 = diveDir < 0 ? -1 : (diveDir > 0 ? 1 : 0);
      const int leanX = static_cast<int>(dir0 * t * size / 2);
      const int leanY = static_cast<int>(t * size / 6);
      blitArt(canvas, art, cx + leanX, cy + leanY, size);
      return;
    }
  }
  int dir = diveDir < 0 ? -1 : (diveDir > 0 ? 1 : 0);
  int leanX = static_cast<int>(dir * t * size / 3);
  int leanY = static_cast<int>(t * size / 8);
  int headR = size / 6;
  int bodyW = size / 3;
  int bodyH = size / 2;
  int headX = cx + leanX;
  int headY = cy - size / 3 + leanY;
  canvas.fillCircle(headX, headY, headR, rgb(221, 166, 122));
  canvas.fillArc(headX, headY - headR / 2, headR * 2 / 3, headR, 180, 360, C_BROWN);
  canvas.fillCircle(headX - headR / 3, headY, clampi(headR / 8, 1, 3), C_BLACK);
  canvas.fillCircle(headX + headR / 3, headY, clampi(headR / 8, 1, 3), C_BLACK);
  canvas.fillRoundRect(cx - bodyW / 2 + leanX / 2, cy - size / 6 + leanY,
                       bodyW, bodyH, bodyW / 4, C_ORANGE);
  canvas.fillRect(cx - bodyW / 2 + leanX / 2, cy + bodyH / 5 + leanY,
                  bodyW, bodyH / 3, DARK);
  int armSpread = size / 2 + static_cast<int>(t * size / 3);
  int armY = cy - size / 12 + leanY;
  int leftY = armY - dir * static_cast<int>(t * size / 5);
  int rightY = armY + dir * static_cast<int>(t * size / 5);
  canvas.drawLine(cx + leanX / 2, armY, cx - armSpread + leanX, leftY, rgb(221, 166, 122));
  canvas.drawLine(cx + leanX / 2, armY, cx + armSpread + leanX, rightY, rgb(221, 166, 122));
  canvas.fillCircle(cx - armSpread + leanX, leftY, size / 9, C_BLUE);
  canvas.fillCircle(cx + armSpread + leanX, rightY, size / 9, C_BLUE);
  canvas.drawLine(cx - bodyW / 4 + leanX / 2, cy + bodyH / 2, cx - bodyW / 3 + leanX,
                  cy + size / 2 + leanY, DARK);
  canvas.drawLine(cx + bodyW / 4 + leanX / 2, cy + bodyH / 2, cx + bodyW / 3 + leanX,
                  cy + size / 2 + leanY, DARK);
}

void drawSoccerBall(int cx, int cy, int r, uint32_t frameMs) {
  M5Canvas& canvas = Gfx::c();
  int phase = static_cast<int>((frameMs / 80) % 12);
  canvas.fillCircle(cx, cy, r, C_WHITE);
  canvas.drawCircle(cx, cy, r, GREY);
  float rotation = phase * 0.5235988f;
  canvas.fillCircle(cx + static_cast<int>(cosf(rotation) * r / 4),
                    cy + static_cast<int>(sinf(rotation) * r / 4), r / 4, C_BLACK);
  for (int i = 0; i < 5; ++i) {
    float angle = rotation + i * 1.256637f;
    int dotX = cx + static_cast<int>(cosf(angle) * r * 2 / 3);
    int dotY = cy + static_cast<int>(sinf(angle) * r * 2 / 3);
    canvas.fillCircle(dotX, dotY, clampi(r / 7, 1, 5), C_BLACK);
    canvas.drawLine(cx, cy, dotX, dotY, GREY);
  }
}

void drawItemPreview(uint16_t itemId, int cx, int cy, int size, uint32_t frameMs) {
  M5Canvas& canvas = Gfx::c();
  if (itemId <= 11 || (itemId >= 60 && itemId <= 71)) {
    drawBuddy(itemId, cx, cy, size, Anim::Idle, frameMs, 0xFFFF, 0xFFFF);
  } else if ((itemId >= 12 && itemId <= 26) || (itemId >= 72 && itemId <= 87)) {
    drawMannequin(canvas, cx, cy, size, itemId, 0xFFFF);
  } else if ((itemId >= 27 && itemId <= 38) || (itemId >= 88 && itemId <= 97)) {
    drawMannequin(canvas, cx, cy, size, 0xFFFF, itemId);
  } else if ((itemId >= 39 && itemId <= 48) || (itemId >= 98 && itemId <= 109)) {
    drawToyPreview(canvas, itemId, cx, cy, size, frameMs);
  } else if ((itemId >= 49 && itemId <= 59) || (itemId >= 110 && itemId <= 120)) {
    drawRoomMini(canvas, itemId, cx, cy, size);
  } else if (itemId >= 121 && itemId <= 126) {
    drawTreatPreview(canvas, itemId, cx, cy, size);
  } else {
    canvas.fillCircle(cx, cy, size / 3, GREY);
    drawStar(canvas, cx, cy, size / 5, C_WHITE);
  }
}

void drawJellyBean(int cx, int cy, int size, uint8_t colourIdx) {
  M5Canvas& canvas = Gfx::c();
  constexpr uint16_t colours[10] = {C_RED, C_BLUE, C_GREEN, C_YELLOW, C_ORANGE, C_PINK, C_PURPLE,
                                    C_WHITE, C_BLACK, C_PINK};
  colourIdx %= 10;
  int w = size * 3 / 5;
  int h = size;
  if (colourIdx == 9) {
    canvas.fillRoundRect(cx - w / 2, cy - h / 2, w, h, w / 2, C_RED);
    uint16_t rainbow[6] = {C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_BLUE, C_PURPLE};
    for (int i = 0; i < 6; ++i) {
      int y0 = cy - h / 2 + i * h / 6;
      canvas.fillRect(cx - w / 2, y0, w, h / 6 + 1, rainbow[i]);
    }
    canvas.fillCircle(cx - w / 2, cy - h / 2 + w / 2, w / 2, C_RED);
    canvas.fillCircle(cx + w / 2, cy + h / 2 - w / 2, w / 2, C_PURPLE);
  } else {
    canvas.fillRoundRect(cx - w / 2, cy - h / 2, w, h, w / 2, colours[colourIdx]);
    canvas.fillCircle(cx - w / 2, cy - h / 2 + w / 2, w / 2, colours[colourIdx]);
    canvas.fillCircle(cx + w / 2, cy + h / 2 - w / 2, w / 2, colours[colourIdx]);
  }
  canvas.fillEllipse(cx - w / 5, cy - h / 4, clampi(w / 7, 2, 7), clampi(h / 9, 2, 8), C_WHITE);
}

void drawHourglass(int cx, int cy, int size, float sandT) {
  M5Canvas& canvas = Gfx::c();
  float elapsed = clampf(sandT, 0.0f, 1.0f);
  float remaining = 1.0f - elapsed;
  int w = size * 3 / 5;
  int h = size;
  int left = cx - w / 2;
  int top = cy - h / 2;
  canvas.fillRoundRect(left - size / 12, top, w + size / 6, size / 8, size / 18, C_BROWN);
  canvas.fillRoundRect(left - size / 12, top + h - size / 8, w + size / 6,
                       size / 8, size / 18, C_BROWN);
  canvas.drawLine(left, top + size / 8, cx - size / 12, cy, C_SILVER);
  canvas.drawLine(left + w, top + size / 8, cx + size / 12, cy, C_SILVER);
  canvas.drawLine(cx - size / 12, cy, left, top + h - size / 8, C_SILVER);
  canvas.drawLine(cx + size / 12, cy, left + w, top + h - size / 8, C_SILVER);
  int chamberH = h / 2 - size / 8;
  int topSandH = static_cast<int>(chamberH * remaining);
  if (topSandH > 0) {
    canvas.fillTriangle(left + size / 12, top + size / 8,
                        left + w - size / 12, top + size / 8,
                        cx, top + size / 8 + topSandH, C_GOLD);
  }
  int bottomSandH = static_cast<int>(chamberH * elapsed);
  if (bottomSandH > 0) {
    canvas.fillTriangle(cx, top + h - size / 8 - bottomSandH,
                        left + size / 12, top + h - size / 8,
                        left + w - size / 12, top + h - size / 8, C_GOLD);
  }
  if (elapsed > 0.0f && elapsed < 1.0f) {
    canvas.drawLine(cx, cy - size / 20, cx, cy + size / 5, C_GOLD);
    canvas.fillCircle(cx - size / 12, cy + size / 5, clampi(size / 40, 1, 3), C_GOLD);
    canvas.fillCircle(cx + size / 10, cy + size / 4, clampi(size / 40, 1, 3), C_GOLD);
  }
}

void drawRoom(uint16_t roomItemId) {
  M5Canvas& canvas = Gfx::c();
  uint16_t id = roomItemId;
  if (!((id >= 49 && id <= 59) || (id >= 110 && id <= 120))) id = 49;

  switch (id) {
    case 49:
      drawGradient(canvas, rgb(132, 201, 244), rgb(245, 184, 219), 174);
      canvas.fillRect(0, 174, 320, 66, rgb(99, 186, 98));
      drawCloud(canvas, 55, 40, 55, C_WHITE); drawCloud(canvas, 270, 65, 40, C_WHITE);
      drawCastleMotif(canvas, LIGHT_PINK, C_PURPLE);
      break;
    case 50:
      drawGradient(canvas, rgb(255, 205, 229), rgb(245, 161, 206), 170);
      canvas.fillRect(0, 170, 320, 70, rgb(184, 112, 166));
      canvas.fillRoundRect(35, 105, 130, 82, 16, C_PINK);
      canvas.fillRect(45, 90, 55, 25, C_WHITE);
      canvas.fillRect(190, 35, 92, 102, rgb(238, 220, 247));
      canvas.fillRect(200, 45, 72, 82, ICE);
      drawHeart(canvas, 243, 83, 28, C_PINK);
      canvas.fillCircle(245, 175, 28, LIGHT_PINK);
      break;
    case 51:
    case 59:
      drawGradient(canvas, rgb(145, 215, 247), rgb(230, 239, 190), 150);
      canvas.fillRect(0, 150, 320, 90, rgb(80, 178, 84));
      drawCloud(canvas, 54, 42, 48, C_WHITE); drawCloud(canvas, 253, 52, 55, C_WHITE);
      for (int i = 0; i < 7; ++i) {
        int x = 25 + i * 46;
        canvas.drawLine(x, 190, x, 150 + (i & 1) * 14, C_GREEN);
        drawFlower(canvas, x, 150 + (i & 1) * 14, 18, i & 1 ? C_PINK : C_PURPLE, C_YELLOW);
      }
      if (id == 59) {
        drawWingsBehind(canvas, 76, 95, 45, 38, C_PINK, ICE);
        drawWingsBehind(canvas, 250, 115, 38, 30, C_YELLOW, C_PURPLE);
      }
      break;
    case 52:
      drawGradient(canvas, rgb(102, 183, 246), rgb(219, 236, 255), 240);
      drawCloud(canvas, 60, 65, 85, C_WHITE); drawCloud(canvas, 250, 80, 100, C_WHITE);
      drawCloud(canvas, 155, 173, 105, C_WHITE);
      canvas.drawArc(160, 155, 100, 120, 190, 350, C_RED);
      canvas.drawArc(160, 155, 92, 112, 190, 350, C_ORANGE);
      canvas.drawArc(160, 155, 84, 104, 190, 350, C_YELLOW);
      canvas.drawArc(160, 155, 76, 96, 190, 350, C_GREEN);
      canvas.drawArc(160, 155, 68, 88, 190, 350, C_BLUE);
      canvas.drawArc(160, 155, 60, 80, 190, 350, C_PURPLE);
      break;
    case 53:
      drawGradient(canvas, rgb(75, 185, 224), rgb(25, 96, 148), 240);
      drawWaves(canvas, 35, 12, ICE); drawWaves(canvas, 70, 10, rgb(52, 154, 192));
      for (int i = 0; i < 8; ++i) drawBubble(canvas, 25 + i * 41, 55 + (i % 3) * 42,
                                             4 + (i % 3) * 2, C_WHITE);
      for (int i = 0; i < 5; ++i) {
        int x = 25 + i * 72;
        canvas.drawLine(x, 240, x + 12, 172, C_GREEN);
        canvas.drawLine(x + 12, 205, x - 5, 183, C_GREEN);
      }
      canvas.fillArc(235, 205, 14, 45, 180, 360, C_PINK);
      break;
    case 54:
      drawGradient(canvas, rgb(37, 25, 55), rgb(81, 35, 92), 180);
      canvas.fillRect(0, 180, 320, 60, DARK);
      canvas.fillTriangle(0, 0, 95, 180, 0, 180, C_PURPLE);
      canvas.fillTriangle(320, 0, 225, 180, 320, 180, C_PURPLE);
      canvas.fillCircle(160, 70, 42, C_PINK);
      drawStar(canvas, 160, 70, 25, C_GOLD);
      canvas.fillRoundRect(70, 166, 180, 30, 12, rgb(49, 45, 65));
      canvas.drawLine(160, 166, 160, 105, C_SILVER);
      canvas.fillCircle(160, 102, 8, DARK);
      for (int i = 0; i < 6; ++i) drawStar(canvas, 30 + i * 53, 28 + (i & 1) * 22, 5, C_GOLD);
      break;
    case 55:
      drawGradient(canvas, rgb(105, 187, 235), rgb(210, 242, 250), 185);
      canvas.fillRect(0, 185, 320, 55, C_WHITE);
      for (int i = 0; i < 5; ++i) {
        int x = 30 + i * 66;
        canvas.fillTriangle(x - 35, 185, x, 55 + (i & 1) * 30, x + 35, 185, ICE);
        canvas.drawLine(x, 65 + (i & 1) * 30, x, 178, C_WHITE);
      }
      canvas.fillRoundRect(120, 108, 80, 88, 12, rgb(139, 213, 239));
      canvas.fillTriangle(112, 108, 160, 45, 208, 108, C_WHITE);
      drawStar(canvas, 160, 142, 13, C_WHITE);
      break;
    case 56:
      drawGradient(canvas, rgb(161, 205, 246), rgb(240, 207, 172), 170);
      canvas.fillRect(0, 170, 320, 70, rgb(95, 176, 89));
      canvas.fillRoundRect(123, 45, 74, 158, 18, C_TAN);
      canvas.fillTriangle(112, 52, 160, 8, 208, 52, C_PURPLE);
      canvas.fillRoundRect(143, 80, 34, 48, 16, DARK);
      canvas.drawLine(175, 95, 265, 183, C_GOLD);
      canvas.drawArc(235, 188, 30, 65, 180, 360, C_GOLD);
      drawCloud(canvas, 48, 50, 55, C_WHITE);
      break;
    case 57:
      drawGradient(canvas, rgb(103, 196, 237), rgb(248, 200, 124), 145);
      canvas.fillRect(0, 145, 320, 95, rgb(237, 203, 126));
      drawWaves(canvas, 125, 10, TEAL);
      drawPalm(canvas, 58, 218, 105); drawPalm(canvas, 260, 220, 90);
      canvas.fillCircle(273, 42, 25, C_YELLOW);
      canvas.fillRoundRect(122, 176, 75, 12, 5, C_BROWN);
      canvas.fillTriangle(159, 176, 142, 143, 176, 176, C_RED);
      break;
    case 58:
    case 119:
      drawGradient(canvas, rgb(250, 183, 221), rgb(167, 123, 218), 170);
      canvas.fillRect(0, 170, 320, 70, rgb(224, 142, 113));
      for (int i = 0; i < 6; ++i) {
        uint16_t colours[6] = {C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_BLUE, C_PURPLE};
        canvas.fillCircle(35 + i * 51, 55 + (i & 1) * 45, 20, colours[i]);
        canvas.drawLine(35 + i * 51, 75 + (i & 1) * 45, 35 + i * 51, 155, C_WHITE);
      }
      canvas.fillRoundRect(98, 146, 124, 56, 14, C_PINK);
      drawTreatPreview(canvas, 123, 160, 149, 52);
      canvas.fillCircle(45, 205, 22, rgb(205, 143, 72));
      canvas.fillCircle(275, 205, 22, ICE);
      break;
    case 110:
      drawGradient(canvas, rgb(83, 169, 232), rgb(205, 234, 249), 105);
      canvas.fillRect(0, 105, 320, 135, rgb(55, 164, 74));
      for (int i = 0; i < 8; ++i) {
        canvas.fillRect(i * 40, 92, 20, 13, C_RED);
        canvas.fillRect(i * 40 + 20, 92, 20, 13, C_WHITE);
      }
      canvas.drawRect(15, 118, 290, 110, C_WHITE);
      canvas.drawLine(160, 118, 160, 228, C_WHITE);
      canvas.drawCircle(160, 173, 28, C_WHITE);
      canvas.drawRect(15, 145, 48, 55, C_WHITE);
      canvas.drawRect(257, 145, 48, 55, C_WHITE);
      drawSoccerBall(160, 188, 13, 0);
      break;
    case 111:
      drawGradient(canvas, rgb(79, 162, 159), rgb(169, 210, 124), 150);
      canvas.fillRect(0, 150, 320, 90, rgb(66, 128, 62));
      for (int i = 0; i < 6; ++i) {
        int x = 20 + i * 60;
        canvas.fillRect(x, 90 + (i & 1) * 20, 12, 150, C_BROWN);
        canvas.fillCircle(x + 6, 75 + (i & 1) * 20, 37, C_GREEN);
      }
      drawDinoInternal(canvas, 0, 185, 175, 100, 0, false);
      drawDinoInternal(canvas, 5, 75, 90, 55, 0, false);
      break;
    case 112:
      drawGradient(canvas, rgb(239, 139, 72), rgb(105, 55, 54), 175);
      canvas.fillRect(0, 175, 320, 65, rgb(83, 84, 53));
      canvas.fillTriangle(55, 184, 160, 46, 270, 184, rgb(84, 67, 65));
      canvas.fillTriangle(126, 91, 160, 46, 195, 91, C_RED);
      canvas.fillTriangle(141, 92, 160, 46, 178, 92, C_ORANGE);
      canvas.fillRect(151, 73, 18, 105, C_RED);
      canvas.fillCircle(160, 42, 24, rgb(91, 73, 76));
      drawPalm(canvas, 38, 226, 75); drawPalm(canvas, 285, 228, 65);
      break;
    case 113:
      drawGradient(canvas, rgb(47, 151, 203), rgb(14, 64, 112), 240);
      drawWaves(canvas, 34, 11, ICE); drawWaves(canvas, 70, 9, C_BLUE);
      drawSharkInternal(canvas, 1, 182, 117, 105, 0);
      drawSharkInternal(canvas, 4, 70, 80, 48, 0);
      for (int i = 0; i < 6; ++i) drawBubble(canvas, 25 + i * 53, 45 + (i % 3) * 58,
                                             4 + (i & 1) * 3, C_WHITE);
      for (int i = 0; i < 4; ++i) canvas.drawLine(20 + i * 93, 240, 35 + i * 93, 178, C_GREEN);
      break;
    case 114:
      drawGradient(canvas, rgb(64, 78, 127), rgb(224, 128, 108), 170);
      canvas.fillRect(0, 170, 320, 70, DARK);
      for (int i = 0; i < 7; ++i) {
        int width = 34 + (i % 3) * 11;
        int height = 70 + (i % 4) * 25;
        int x = i * 48 - 8;
        canvas.fillRect(x, 170 - height, width, height, rgb(51, 58, 77));
        for (int wy = 170 - height + 12; wy < 160; wy += 18)
          canvas.fillRect(x + 8, wy, 7, 8, (wy / 18 + i) & 1 ? C_YELLOW : C_BLUE);
      }
      canvas.drawLine(0, 35, 320, 175, C_BLACK);
      canvas.drawLine(0, 175, 320, 35, C_BLACK);
      canvas.drawArc(160, 105, 55, 55, 180, 360, C_BLACK);
      break;
    case 115:
      canvas.fillScreen(rgb(12, 19, 43));
      for (int i = 0; i < 25; ++i) canvas.fillCircle((i * 47) % 320, (i * 73) % 220,
                                                     1 + (i % 2), C_WHITE);
      canvas.fillCircle(70, 68, 38, C_BLUE);
      canvas.fillArc(70, 68, 45, 55, 170, 350, C_SILVER);
      canvas.fillCircle(258, 48, 22, C_ORANGE);
      canvas.fillRoundRect(48, 132, 224, 78, 20, C_SILVER);
      canvas.fillCircle(160, 168, 48, ICE);
      canvas.drawLine(112, 168, 208, 168, DARK);
      canvas.drawLine(160, 120, 160, 216, DARK);
      break;
    case 116:
      drawGradient(canvas, rgb(244, 174, 91), rgb(249, 218, 159), 165);
      canvas.fillRect(0, 165, 320, 75, rgb(211, 155, 84));
      canvas.fillRect(20, 88, 80, 84, C_BROWN);
      canvas.fillTriangle(12, 88, 60, 58, 108, 88, C_RED);
      canvas.fillRect(220, 104, 82, 68, C_TAN);
      canvas.fillTriangle(212, 104, 261, 70, 310, 104, C_BROWN);
      canvas.fillRect(130, 105, 60, 67, rgb(122, 84, 56));
      canvas.fillRect(150, 130, 20, 42, DARK);
      for (int i = 0; i < 4; ++i) canvas.fillRect(i * 90, 186, 55, 8, C_BROWN);
      canvas.drawCircle(260, 58, 24, C_BROWN);
      break;
    case 117:
      canvas.fillScreen(rgb(16, 19, 47));
      for (int i = 0; i < 22; ++i) drawStar(canvas, (i * 67) % 320, (i * 43) % 155,
                                            2 + i % 3, C_WHITE);
      canvas.fillRect(0, 172, 320, 68, rgb(73, 79, 98));
      canvas.fillRoundRect(82, 112, 156, 72, 24, C_WHITE);
      canvas.fillRect(112, 132, 96, 26, C_GREEN);
      canvas.fillTriangle(82, 148, 40, 188, 94, 176, C_PURPLE);
      canvas.fillTriangle(238, 148, 280, 188, 226, 176, C_PURPLE);
      canvas.fillCircle(160, 91, 42, ICE);
      canvas.fillCircle(269, 55, 31, C_BLUE);
      break;
    case 118:
      drawGradient(canvas, rgb(89, 121, 151), rgb(196, 216, 224), 180);
      canvas.fillRect(0, 180, 320, 60, GREY);
      canvas.fillRoundRect(20, 52, 90, 105, 12, C_SILVER);
      canvas.fillRect(30, 65, 70, 42, DARK);
      canvas.fillCircle(48, 128, 9, C_RED);
      canvas.fillCircle(76, 128, 9, C_GREEN);
      canvas.fillCircle(99, 128, 6, C_BLUE);
      canvas.fillRoundRect(205, 45, 92, 112, 12, rgb(111, 139, 156));
      for (int i = 0; i < 4; ++i) canvas.drawLine(217, 65 + i * 21, 285, 65 + i * 21, ICE);
      drawBuddy(69, 160, 155, 80, Anim::Idle, 0, 0xFFFF, 0xFFFF);
      break;
    case 120:
      drawGradient(canvas, rgb(119, 202, 244), rgb(225, 239, 195), 155);
      canvas.fillRect(0, 155, 320, 85, rgb(83, 176, 82));
      drawCloud(canvas, 50, 42, 54, C_WHITE); drawCloud(canvas, 268, 55, 45, C_WHITE);
      canvas.fillCircle(45, 175, 35, rgb(52, 134, 64));
      canvas.fillCircle(276, 170, 40, rgb(52, 134, 64));
      canvas.fillRect(137, 110, 46, 75, C_TAN);
      canvas.fillTriangle(126, 110, 160, 78, 194, 110, C_RED);
      canvas.fillRoundRect(146, 145, 28, 40, 13, DARK);
      drawFrankieInternal(canvas, 92, 197, 76, 0, true);
      canvas.fillCircle(239, 205, 13, C_WHITE);
      canvas.fillCircle(239, 205, 4, C_BLACK);
      break;
  }
}

}  // namespace Sprites
