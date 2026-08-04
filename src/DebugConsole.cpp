#include "DebugConsole.h"
#include "Managers.h"
#include "App.h"
#include "Config.h"

namespace {
void printHelp() {
  Serial.println("[dbg] a/b/c PRESS  A/B/C LONG  x A+C  h HOLD-B");
  Serial.println("[dbg] s STATE  d DAILY RESET  t +1H  g +100 COINS  0/1/2 VOLUME  ? HELP");
}

void dumpState() {
  const PlayerProfile& profile = App::profile();
  uint16_t ownedCount = 0;
  for (uint16_t id = 0; id < MAX_ITEMS; ++id) {
    if (profile.inventory.owned(id)) ++ownedCount;
  }
  Serial.printf("[dbg] player=%s coins=%lu needs=%u/%u/%u level=%u day=%lu owned=%u\n",
                App::player() == PlayerId::Hugo ? "HUGO" : "STELLA",
                static_cast<unsigned long>(profile.coins), profile.buddy.hunger,
                profile.buddy.happiness, profile.buddy.energy, profile.maths.currentLevel,
                static_cast<unsigned long>(profile.daily.dayNumber), ownedCount);
}
}

namespace DebugConsole {

void poll() {
  while (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());
    if (command == '\r' || command == '\n' || command == ' ' || command == '\t') continue;
    switch (command) {
      case 'a': case 'b': case 'c': case 'A': case 'B': case 'C': case 'x': case 'h':
        Input::inject(command);
        Serial.printf("[dbg] inject %c\n", command);
        break;
      case 's':
        Serial.println("[dbg] state");
        dumpState();
        break;
      case 'd':
        Serial.println("[dbg] daily reset");
        Clock::forceDailyReset();
        break;
      case 't':
        Save::data().time.pseudoClockMin += 60;
        Save::requestSave();
        Serial.println("[dbg] clock +1h");
        break;
      case 'g':
        Serial.println("[dbg] grant 100 coins");
        App::awardCoins(100);
        break;
      case 'w':
        Input::injectMovement(10000);
        Serial.println("[dbg] movement 10s");
        break;
      case '0': case '1': case '2':
        Save::data().settings.volume = command - '0';
        Audio::applyVolume(Save::data().settings.volume);
        Save::requestSave();
        Serial.printf("[dbg] volume %c\n", command);
        break;
      case '?':
        Serial.println("[dbg] help");
        printHelp();
        break;
      default:
        Serial.printf("[dbg] unknown %c\n", command);
        break;
    }
  }
}

}
