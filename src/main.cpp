#include <M5Unified.h>

// Minimal boot validation build. Replaced by full App in Phase 1.
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  M5.Display.setBrightness(128);
  M5.Display.fillScreen(TFT_NAVY);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextSize(3);
  M5.Display.setCursor(40, 100);
  M5.Display.print("POCKET BUDDY");
  Serial.println("[boot] Pocket Buddy skeleton alive");
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) Serial.println("[btn] A");
  if (M5.BtnB.wasPressed()) Serial.println("[btn] B");
  if (M5.BtnC.wasPressed()) Serial.println("[btn] C");
  delay(10);
}
