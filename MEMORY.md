# Pocket Buddy — Running Memory

Working log. Newest entries at the bottom. Decisions marked **D**, findings **F**, todos **T**.

## 2026-08-04 — Session start
- **F** Device on `/dev/cu.usbserial-5B090283871` (CH9102, M5Stack Fire v2.7). Codex CLI 0.142.5
  (model gpt-5.6-sol, default effort low → override `-c model_reasoning_effort="xhigh"`).
  Gemini CLI 0.52.0. PlatformIO not present → installing via `pip install --user platformio`.
- **F** `/dev` (scafld) skill not registered in this session → running the equivalent manually:
  Codex builds, Fable integrates/QAs, Gemini+Fable review.
- **D** All art procedural (code-drawn pixel art, palette+silhouette per character); all audio
  synthesized tones. No binary assets, no ripped/copied material. Original artwork only.
- **D** Daily reset rule (no RTC on Fire): reset when 24h continuous runtime OR cold boot whose
  previous session ran >= 10 min. Rapid power-cycles give no credit. Tunable constants in
  TimeManager.
- **D** Phase 1 games: Bubble Pop, Colour Match (Stella) + Penalty Kick, Hugo Maths (Hugo).
  Phase 2: Find the Dinosaur, Princess Dance, Roller Coaster, Shark Swim + Frankie/DadMissions/
  DailyMissions/Collection.
- **D** Debug serial console compiled in: inject buttons over USB serial, log screen transitions —
  this is the on-device QA harness.
- **D** Phase 2 menu entries hidden in Phase 1 build (spec forbids dead menu items).
