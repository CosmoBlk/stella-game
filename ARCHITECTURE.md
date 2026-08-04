# Pocket Buddy — Architecture & Conventions (contract for all builders)

Read PLAN.md for scope. The headers in `src/` are the contract — implement against them
exactly; do not rename or restructure public interfaces without updating this doc.

## Hard conventions
- C++17, Arduino framework, M5Unified only (no direct TFT_eSPI). Board: M5Stack Fire v2.7.
- All drawing goes to the PSRAM canvas `Gfx::c()` (an `M5Canvas`), pushed once per frame by
  `Gfx::present()`. Never draw to `M5.Display` directly outside Gfx.
- Non-blocking everywhere: no `delay()` in gameplay, no busy loops. Timing via millis deltas
  passed into `update(deltaMs)`.
- No dynamic allocation inside the frame loop. Screens are static singletons. Avoid STL
  containers in hot paths; fixed arrays sized by Config.h limits.
- Strings: plain `const char*` literals, uppercase short labels (pre-readers see icons first).
- Every child-facing flow must work with 3 buttons only: A=left/prev, B=confirm/centre,
  C=right/next, hold-B=back (global, unless screen consumes it), hold A+C = player select.
- Kid-safety invariants: no failure punishments, no coin loss, buddy never dies, incorrect
  answers get "GOOD TRY!" + retry, everything recoverable.

## Frame loop (main.cpp)
```
setup: M5.begin (speaker+imu on), Serial 115200, Gfx::init, Save::init, Clock::init,
       Audio::init + applyVolume, Input::init, App::init  -> Boot screen
loop:  deltaMs clamp 0..100; M5.update(); DebugConsole::poll(); Input::poll();
       Clock::update; Buddy::update; Audio::update; Save::update;
       App::tick() { global input dispatch -> screen->update -> screen->draw -> UI coin anim
                     overlay -> Gfx::present() }
       frame pacing to FRAME_MS (subtract work time, min 1ms yield)
```
- App::tick dispatches: `pressedX -> onButtonX`, `longX -> onButtonXLong`, and BEFORE screen
  long-B: if `longB && screen->allowGlobalBack()` -> `App::goBack()` + Sfx::Back.
  `comboAC` -> goTo(PlayerSelect) from anywhere (also saves).
- App logs every transition: `Serial.printf("[screen] %s -> %s\n", from, to)`.

## Screen wiring (self-registration)
- Each screen lives in `src/screens/XxxScreen.cpp` (games in `src/games/`), defines a class
  deriving `Screen` as a file-local static instance and registers it:
  `namespace { MyScreen inst; ScreenRegistrar reg(ScreenId::Xxx, inst); }`.
  App.cpp's registry table is a zero-initialised POD array, so static-init order is safe.
- App.cpp owns the `Screen* table[ScreenId::COUNT]` plus a `parent[]` table for goBack
  (ShopBrowse->ShopCategory, games->GamesMenu, most others ->Home; PlayerSelect no parent).
  `goTo` on an unregistered id logs `[warn] screen missing` and does nothing — and MainMenu
  must never list an unregistered screen (no dead entries, spec requirement).

## Debug console (QA harness) — src/DebugConsole.{h,cpp}
Serial commands (single chars, newline-agnostic):
- `a b c` inject short press; `A B C` inject long press; `x` inject A+C combo
- `h` inject 2s hold-B completion (sets held state so TASK_HOLD flows fire)
- `s` dump active profile (coins, needs, level, day, owned count)
- `d` force daily reset; `t` advance pseudo-clock by 1h; `g` grant 100 coins (QA)
- `?` list commands. Every command echoes `[dbg] ...` so tests can assert.
Injection goes through `Input::inject()` so real and injected input share one path.

## Manager implementation notes
- **Gfx**: canvas createSprite(320,240) with psram; setColorDepth(16). present() = pushSprite(0,0).
- **Input**: track press timestamps; "short press" fires on release if held < LONG_PRESS_MS;
  "long" fires once when held crosses LONG_PRESS_MS (suppresses the short on release).
  heldMsX() live while held (for 2s task hold + hold-progress ring). IMU: lowpass accel
  (alpha ~0.2); moveMagnitude = |accel - gravity-filtered baseline|; shaken() on
  SHAKE_THRESHOLD crossing with 400ms cooldown. inject() sets one-frame flags mirroring
  real events (injected long presses also populate heldMs >= thresholds for that frame —
  for `h`, hold state persists 2100ms so TASK_HOLD_MS flows complete).
- **Audio**: tables of {freqHz, ms} steps per Sfx (0 freq = rest); play() starts sequence
  (interrupting), update() advances with M5.Speaker.tone(freq, remaining). Jingles short
  (<1.2s), cheerful, C-major-ish. Volume mapping: {0, 48, 150}.
- **Save**: LittleFS.begin(true). Load: read SaveData, check magic/version/crc; on any
  failure try SAVE_BAK_PATH; else fresh defaults. saveNow(): write tmp, verify readback crc,
  rename old->bak, tmp->main. requestSave() sets dirty flag; update() writes when
  dirty && 1000ms since last change. Defaults: coins 20, needs 80/80/80, maths level 2
  (Hugo), default character+room owned+equipped per player, day 1.
- **Clock**: pseudoClockMin accumulates runtime. On init (boot): bootCount++,
  `if (time.lastSessionMs >= MIN_PREV_SESSION_MS) dailyReset()` ; lastSessionMs zeroed and
  accumulated during session (persisted via debounced saves). Also reset when runtime since
  last reset > DAY_RESET_RUNTIME_MS. dailyReset(): for both profiles — clear completedTasks,
  roll 3 new daily missions (audience-filtered, no duplicates), clear rewardedDadMissions,
  dayNumber++, apply Buddy::applyBootDecay once.
- **Buddy**: needs decay 1 point per NEED_DECAY_INTERVAL_MS of runtime (all three, staggered
  so they don't move in lockstep: hunger 1.0x, happiness 0.8x, energy 0.6x). Actions:
  feed(+30 hunger,+10 happy), jelly beans (+10 happy,+5 hunger, rainbow flag), play(+25 happy,
  -10 energy), sleep(energy->100 over a short sleep screen). Clamp 0..100. moodAnim():
  lowest need under NEED_LOW_THRESHOLD wins (hunger->Sad-hungry look, energy->Tired), else
  happiness>70 -> Happy else Idle.

## UI kit notes
- Layouts: header 0..28px (title + coins right-aligned), content 28..204, button bar 204..240
  with three labelled slots centred at x=53/160/267 above the physical buttons.
- drawChoiceCards: three rounded panels (~92px wide) above the button bar for A/B/C choices;
  highlight index draws accent border + slight raise.
- Icons: simple filled-primitive glyphs, must read at 16px and 48px.
- Coin anim: on awardCoins, 5-8 coin dots arc from centre to header coin count over ~600ms,
  then balance ticks up with Sfx::Coin.

## Content data (Content.cpp)
Implement every table in Content.h: full item catalog from the brief (sections 26-35 + treats),
ids stable per Content.h ranges, prices per section 10 tables. 15 tasks (section 11),
30 dad missions (section 14), daily mission pool (section 15, audience-tagged), 12 dinosaurs
with correct herbivore/flies/horns/longNeck flags, jelly bean colours, Frankie finds, sharks.

## Sprites (Sprites.cpp)
Shared cute buddy body renderer (round body, big eyes, stubby arms/legs) parameterised by a
per-character `CharLook {skin, hair, outfit primary/secondary, signature}` table covering all
24 characters; signature = small code-drawn element (braid, crown, mask, fin, hat, web...).
Anim = positional offsets/eye states/arm poses on the shared body (bounce for Happy/Dancing,
closed eyes + zzz for Sleeping, chomp for Eating...). Dinos/sharks/goalie/Frankie are separate
stylised primitive draws. Rooms are full-screen gradient + 3-5 motif elements. Every function
in Sprites.h must be implemented; keep each character recognisable by palette + silhouette.

## Games contract
Each game is one Screen in src/games/, uses App::ctx.gameId pattern, and MUST:
- run rounds per its brief section, 20-60s, no blocking
- award coins via App::awardCoins at the result step and raise its GameEvents
- call `App::raiseEvent(GameEvent::GamePlayed, (uint8_t)gameId)` once per finished round
- update gameHighScores, handle pause where specced (tilt games: B pauses)
- exit to GamesMenu on hold-B (global back)
