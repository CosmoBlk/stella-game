# Pocket Buddy — Build Plan (M5Stack Fire v2.7)

Private family build for Hugo (4) and Stella (2). Full spec in the original brief; this plan is the execution map.

## Toolchain
- PlatformIO + Arduino framework, board `m5stack-fire` (16MB flash, 8MB PSRAM)
- M5Unified (display/buttons/speaker/IMU) + M5GFX canvas (full-screen sprite in PSRAM)
- LittleFS for versioned save data, NVS Preferences for settings
- Device: /dev/cu.usbserial-5B090283871 (CH9102)
- Builders: Codex CLI (gpt-5.6-sol, xhigh) does most implementation; Fable orchestrates,
  integrates, and QAs; Gemini + Fable review passes.

## Architecture decisions
- **No binary assets.** All art is procedural pixel-art drawn in code (palette + silhouette per
  character). All audio is synthesized tones/melodies via M5Unified Speaker. Keeps the build
  fully self-contained, offline, and artist-free. Original artwork only.
- **One full-screen canvas** (320x240x16bpp in PSRAM), draw-per-frame, push to LCD. Target 25 FPS.
- **Screen state machine** exactly as specced (Screen base class, enter/exit/update/draw/onButton).
  Screens statically allocated; App::goTo(ScreenId).
- **Content catalog** is data tables in Content.cpp: items (fixed IDs), tasks, dad missions,
  daily missions, dinosaurs, jelly beans. Both kids' full catalogs ship in Phase 1 (data is cheap).
- **Save**: single versioned binary blob (both profiles + settings), CRC32, write tmp + rename,
  .bak fallback. Save on every economy/inventory/task event, debounced during games.
- **Daily reset without RTC**: pseudo-clock = accumulated runtime + boot credit. Reset when
  (a) 24h continuous runtime, or (b) cold boot where the *previous* session ran >= 10 min
  (defeats rapid power-cycle farming; morning boot = new day). Documented in MEMORY.md.
- **Debug serial console** (QA harness): serial chars inject button events (a/b/c short,
  A/B/C long, x = A+C combo) and print screen transitions + state dumps. Lets us drive the
  full UI on-device over USB for automated smoke tests. Compiled in always (harmless).

## Phase 1 — Core + cut-down games (flash to device, playable)
1. Scaffold: platformio.ini, partitions (16MB, LittleFS), main.cpp, boot to screen.
2. Core managers: Display/Canvas, Input (debounce, short/long, A+C combo, IMU), Audio (sfx +
   melodies), Save, Time/DailyReset, App state machine, UI kit (button bar, header, big text,
   icon draws).
3. Models + Content catalog (all items/tasks/characters both kids).
4. Sprites: base buddy + per-character palette sprites (both rosters), animations
   (idle/happy/sad/eat/sleep/celebrate/dance/kick).
5. Screens: Boot, PlayerSelect, Home, MainMenu, Buddy (+feed/toy/sleep), Character/Accessory/
   Clothing/Room select, Tasks (+detail/complete), GetDressedTimer, Shop (category/browse/
   preview/confirm/result), Settings, SwitchPlayer.
6. Games (4): Bubble Pop, Colour Match, Penalty Kick, Hugo Maths (adaptive, L2 start).
7. Phase 2 menu entries hidden until Phase 2 lands (no dead menu items at any point).
8. QA: compile clean → flash → serial-console walkthrough of every screen → fix → reflash.

## Phase 2 — Full spec
1. Walk Frankie (IMU movement detector, discoveries, 20 coins).
2. Dad Missions (30 missions, 3/day rewarded, badges).
3. Daily Missions (3/day per profile, 30-coin claim).
4. Remaining games: Find the Dinosaur (12 dinos, modes), Princess Dance, Roller Coaster (tilt),
   Shark Swim (tilt).
5. Collection Book (all categories, silhouettes).
6. Collectibles wiring (game drops, walk discoveries, jelly bean colours, badges).
7. Buddy absence greeting, low-stat reactions polish, coin animations.
8. Full audio pass, settings volume tiers.
9. Reviews: Codex review + Gemini review + Fable pass on the full tree; fix findings.
10. Final flash + full serial QA + acceptance criteria checklist.

## QA approach (continuous)
- `pio run` after every module lands (Codex output never merges unbuilt).
- On-device: flash + scripted serial walkthroughs (screen coverage, buy/equip/persist,
  daily reset simulation via debug command, save-corruption recovery).
- Reviews at: end of Phase 1, end of Phase 2 (Codex `codex exec review` + Gemini + Fable).

## Acceptance
Track against the 38-point checklist in the brief (section 47). Final pass ticks each.
