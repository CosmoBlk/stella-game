#include "Managers.h"
#include "Config.h"
#include "Content.h"
#include <LittleFS.h>
#include <string.h>
#include <stddef.h>

namespace {
SaveData saveData = {};
bool mounted = false;
bool dirty = false;
uint32_t dirtyAt = 0;

uint32_t crc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
  }
  return ~crc;
}

uint32_t calculateCrc(const SaveData& data) {
  return crc32(reinterpret_cast<const uint8_t*>(&data), offsetof(SaveData, crc));
}

bool valid(const SaveData& data) {
  return data.magic == SAVE_MAGIC && data.version == SAVE_VERSION && data.crc == calculateCrc(data);
}

bool readSave(const char* path, SaveData& out) {
  File file = LittleFS.open(path, "r");
  if (!file || file.size() != sizeof(SaveData)) {
    if (file) file.close();
    return false;
  }
  const size_t read = file.read(reinterpret_cast<uint8_t*>(&out), sizeof(SaveData));
  file.close();
  return read == sizeof(SaveData) && valid(out);
}

void resetAllDefaults() {
  memset(&saveData, 0, sizeof(saveData));
  saveData.magic = SAVE_MAGIC;
  saveData.version = SAVE_VERSION;
  Save::resetProfileDefaults(saveData.profiles[0], PlayerId::Hugo);
  Save::resetProfileDefaults(saveData.profiles[1], PlayerId::Stella);
  saveData.settings.volume = 2;
  saveData.settings.brightness = 144;
  saveData.time.pseudoClockMin = 0;
  saveData.time.lastResetPseudoMin = 0;
  saveData.time.lastSessionMs = 0;
  saveData.time.bootCount = 0;
  saveData.crc = calculateCrc(saveData);
}
}

namespace Save {

void init() {
  mounted = LittleFS.begin(true);
  if (!mounted) {
    Serial.println("[save] LittleFS mount failed");
    resetAllDefaults();
    return;
  }
  if (readSave(SAVE_PATH, saveData)) {
    Serial.println("[save] loaded");
    return;
  }
  if (readSave(SAVE_BAK_PATH, saveData)) {
    Serial.println("[save] restored backup");
    dirty = true;
    dirtyAt = millis();
    return;
  }
  Serial.println("[save] fresh defaults");
  resetAllDefaults();
  saveNow();
}

SaveData& data() {
  return saveData;
}

void requestSave() {
  dirty = true;
  dirtyAt = millis();
}

void saveNow() {
  if (!mounted) return;
  saveData.magic = SAVE_MAGIC;
  saveData.version = SAVE_VERSION;
  saveData.crc = calculateCrc(saveData);

  LittleFS.remove(SAVE_TMP_PATH);
  File file = LittleFS.open(SAVE_TMP_PATH, "w");
  if (!file) {
    Serial.println("[save] tmp open failed");
    return;
  }
  const size_t written = file.write(reinterpret_cast<const uint8_t*>(&saveData), sizeof(SaveData));
  file.flush();
  file.close();
  if (written != sizeof(SaveData)) {
    LittleFS.remove(SAVE_TMP_PATH);
    Serial.println("[save] tmp write failed");
    return;
  }

  SaveData verify = {};
  if (!readSave(SAVE_TMP_PATH, verify)) {
    LittleFS.remove(SAVE_TMP_PATH);
    Serial.println("[save] tmp verify failed");
    return;
  }

  LittleFS.remove(SAVE_BAK_PATH);
  if (LittleFS.exists(SAVE_PATH) && !LittleFS.rename(SAVE_PATH, SAVE_BAK_PATH)) {
    LittleFS.remove(SAVE_TMP_PATH);
    Serial.println("[save] backup rename failed");
    return;
  }
  if (!LittleFS.rename(SAVE_TMP_PATH, SAVE_PATH)) {
    if (LittleFS.exists(SAVE_BAK_PATH)) LittleFS.rename(SAVE_BAK_PATH, SAVE_PATH);
    Serial.println("[save] main rename failed");
    return;
  }
  dirty = false;
  Serial.println("[save] wrote");
}

void update() {
  if (dirty && millis() - dirtyAt >= 1000) saveNow();
}

void resetProfileDefaults(PlayerProfile& profile, PlayerId id) {
  memset(&profile, 0, sizeof(profile));
  profile.id = static_cast<uint8_t>(id);
  profile.coins = 20;
  profile.buddy.hunger = 80;
  profile.buddy.happiness = 80;
  profile.buddy.energy = 80;
  profile.inventory.equippedAccessory = ITEM_NONE;
  profile.inventory.equippedClothing = ITEM_NONE;
  profile.inventory.equippedToy = ITEM_NONE;
  profile.daily.dayNumber = 1;
  for (uint8_t i = 0; i < 3; ++i) profile.daily.dailyMissionIds[i] = 0xFFFF;
  for (uint8_t i = 0; i < MAX_DAD_MISSION_RECENT; ++i) profile.daily.recentDadMissions[i] = 0xFF;
  profile.maths.currentLevel = id == PlayerId::Hugo ? 2 : 1;

  if (id == PlayerId::Hugo) {
    profile.inventory.equippedCharacter = ITEM_HUGO_DEFAULT_SOCCER;
    profile.inventory.equippedRoom = ITEM_HUGO_ROOM_STADIUM;
    profile.inventory.setOwned(ITEM_HUGO_DEFAULT_SOCCER);
    profile.inventory.setOwned(ITEM_HUGO_ROOM_STADIUM);
  } else {
    profile.inventory.equippedCharacter = ITEM_STELLA_DEFAULT_PRINCESS;
    profile.inventory.equippedRoom = ITEM_STELLA_ROOM_CASTLE;
    profile.inventory.setOwned(ITEM_STELLA_DEFAULT_PRINCESS);
    profile.inventory.setOwned(ITEM_STELLA_ROOM_CASTLE);
  }
}

}
