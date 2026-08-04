#pragma once

#include "Models.h"

namespace Missions {
  void rollDaily(PlayerProfile& p);
  void onEvent(PlayerProfile& p, GameEvent ev, uint8_t param, uint8_t count);
  bool allDone(const PlayerProfile& p);
}
