#include "Managers.h"

namespace {
struct ToneStep { uint16_t frequency; uint16_t durationMs; };
struct ToneSequence { const ToneStep* steps; uint8_t count; };

#define SEQ(name, ...) const ToneStep name[] = { __VA_ARGS__ }
SEQ(selectSeq, {660,55}, {880,60});
SEQ(backSeq, {520,65}, {390,80});
SEQ(coinSeq, {784,70}, {988,80}, {1319,120});
SEQ(purchaseSeq, {523,70}, {659,70}, {784,80}, {1047,160});
SEQ(popSeq, {880,35}, {0,20}, {1175,45});
SEQ(kickSeq, {180,45}, {110,65});
SEQ(goalSeq, {523,70}, {659,70}, {784,80}, {1047,100}, {1319,180});
SEQ(saveSeq, {330,80}, {247,100});
SEQ(cheerSeq, {659,70}, {784,70}, {988,80}, {1047,140});
SEQ(countdownSeq, {440,90});
SEQ(goSeq, {880,70}, {1047,130});
SEQ(roarSeq, {130,90}, {110,90}, {98,120});
SEQ(splashSeq, {300,35}, {220,45}, {160,70});
SEQ(sparkleSeq, {1047,45}, {1319,45}, {1568,100});
SEQ(musicSeq, {523,80}, {659,80}, {784,120});
SEQ(jellySeq, {659,50}, {784,50}, {988,50}, {1319,120});
SEQ(barkSeq, {440,65}, {0,35}, {523,75});
SEQ(sleepSeq, {523,150}, {440,150}, {392,220});
SEQ(missionSeq, {523,70}, {659,70}, {784,90}, {988,90}, {1319,180});
SEQ(taskSeq, {523,65}, {659,65}, {784,65}, {1047,100}, {1319,160});
SEQ(tickSeq, {880,35});
SEQ(timerEndSeq, {784,90}, {988,90}, {1175,90}, {1568,200});
SEQ(goodTrySeq, {392,80}, {523,100}, {659,130});
SEQ(celebrateSeq, {523,60}, {659,60}, {784,60}, {1047,90}, {784,60}, {1319,190});
SEQ(eatSeq, {280,45}, {360,45}, {440,70});
SEQ(discoverSeq, {784,55}, {988,55}, {1175,70}, {1568,160});
SEQ(levelSeq, {523,65}, {659,65}, {784,65}, {988,65}, {1319,200});
#undef SEQ

const ToneSequence sequences[] = {
  {selectSeq, 2}, {backSeq, 2}, {coinSeq, 3}, {purchaseSeq, 4}, {popSeq, 3},
  {kickSeq, 2}, {goalSeq, 5}, {saveSeq, 2}, {cheerSeq, 4}, {countdownSeq, 1},
  {goSeq, 2}, {roarSeq, 3}, {splashSeq, 3}, {sparkleSeq, 3}, {musicSeq, 3},
  {jellySeq, 4}, {barkSeq, 3}, {sleepSeq, 3}, {missionSeq, 5}, {taskSeq, 5},
  {tickSeq, 1}, {timerEndSeq, 4}, {goodTrySeq, 3}, {celebrateSeq, 6}, {eatSeq, 3},
  {discoverSeq, 4}, {levelSeq, 5}
};

const ToneSequence* activeSequence = nullptr;
uint8_t activeStep = 0;
uint32_t stepEndsAt = 0;
uint8_t volume = 150;

void startStep(uint32_t now) {
  if (!activeSequence || activeStep >= activeSequence->count) {
    activeSequence = nullptr;
    M5.Speaker.stop();
    return;
  }
  const ToneStep& step = activeSequence->steps[activeStep];
  stepEndsAt = now + step.durationMs;
  if (step.frequency == 0 || volume == 0) M5.Speaker.stop();
  else M5.Speaker.tone(step.frequency, step.durationMs);
}
}

namespace Audio {

void init() {
  M5.Speaker.begin();
  M5.Speaker.setVolume(volume);
}

void update() {
  if (!activeSequence) return;
  const uint32_t now = millis();
  if ((int32_t)(now - stepEndsAt) >= 0) {
    ++activeStep;
    startStep(now);
  }
}

void play(Sfx sfx) {
  const uint8_t index = static_cast<uint8_t>(sfx);
  if (index >= sizeof(sequences) / sizeof(sequences[0]) || volume == 0) return;
  activeSequence = &sequences[index];
  activeStep = 0;
  startStep(millis());
}

void applyVolume(uint8_t volumeSetting) {
  static const uint8_t levels[3] = {0, 48, 150};
  if (volumeSetting > 2) volumeSetting = 2;
  volume = levels[volumeSetting];
  M5.Speaker.setVolume(volume);
  if (volume == 0) {
    activeSequence = nullptr;
    M5.Speaker.stop();
  }
}

}
