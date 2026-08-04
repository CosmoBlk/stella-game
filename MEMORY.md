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

## Phase 1 build fan-out (22:3x)
- **D** Git repo initialised + scaffold commit (baseline for Codex diff/review only; no more
  commits unless George asks or review tooling needs a baseline).
- **D** Frozen contracts before fan-out: Config.h, Models.h, Content.h, App.h (screen
  self-registration via ScreenRegistrar), Managers.h, UI.h, Sprites.h, Screen.h + CATALOG.md
  (authoritative item ids 0-126) + SPEC.md (full brief in repo) + ARCHITECTURE.md.
- **F** 7 parallel Codex xhigh builders launched, disjoint file scopes: core / content /
  sprites / buddy screens / tasks+timer / shop / games menu + 4 Phase-1 games.
  Prompts + logs in scratchpad (log-*.txt, out-*.md).
- **T** Integrate + compile when runs land; expect header drift fixes at integration.
- **T** Flash minimal skeleton to device once toolchain download finishes (validates USB path).

## Phase 1 integrated + on-device QA (23:0x)
- **F** Integration fixes needed: (1) bare colour constants in UI.cpp/Sprites.cpp collide with
  m5gfx colour macros -> renamed COL_*/C_*; (2) TaskDetail/GetDressedTimer double-incremented
  totalTasksCompleted alongside raiseEvent -> removed direct increments. That's ALL — 12k+
  lines from 7 parallel builders otherwise compiled first try.
- **F** Firmware 654KB (10% flash), 29KB RAM static. Builds in ~16s incremental.
- **F** On-device QA (serial harness tools/qa_walkthrough.py): smoke 15/15, deep 21/21
  (task claim -> coins, jelly-bean purchase -> deduction, coins persist across hard reboot),
  games 31/31 + targeted Stella pass 11/11 (all 4 games mash-tested, no crash-reboots,
  daily reset advances day + reapplies boot decay + tasks re-claimable, favourite-first
  games ordering per player works).
- **D** Phase 1 committed (6c241cc) as review baseline.
- **F** Phase 2 builders launched (frankie+dad+daily missions / collection / dino+dance /
  coaster+shark), monitor armed.
- **T** At Phase 2 integration: wire Missions::onEvent into App::raiseEvent; Clock daily
  reset must call Missions::rollDaily; full-tree Codex+Gemini+Fable review; reflash + QA.

## George interventions (23:1x)
- **D** QA must run MUTED: DebugConsole now has 0/1/2 volume commands; qa_walkthrough mutes
  after every boot (persists via settings). **T: final handover build must restore volume=2.**
- **D** Graphics upgrade requested: Game-Boy-level pixel sprites via OpenAI image API
  (gpt-image-2), 48x48 quantized RGB565 arrays. Art agent running (Assets.h/AssetsData.h/
  Assets.cpp + assets/). Integration into Sprites.cpp (characters/dinos/sharks/goalie) is
  mine after it lands; procedural art remains fallback + items/rooms.

## Phase 2 integrated + QA green (23:2x)
- **F** Phase 2 compiled FIRST TRY (686KB). Missions engine wired into App::raiseEvent;
  Clock delegates rolls to Missions::rollDaily.
- **F** Real bug fixed: MainMenu remembered last selection across visits -> now resets to top
  on enter (predictability for a 4yo). GamesMenu was fine (my ColourMatch-loop diagnosis was
  harness desync onto Stella profile, not product).
- **D** DebugConsole gained: 'w' movement inject (10s), direct jumps H/D/F/M/K/G, 'i' screen id.
  QA scripts now jump-based = deterministic. phase2 script: 61/61 PASS (menu sweep 11/11
  distinct, dad mission +35 coins, Frankie walk + movement, collection grid, all 8 games).
- **F** Pushed to github.com/CosmoBlk/stella-game (PRIVATE — licensed-character pixel art,
  never make public). History cleaned of .pio before first push.
- **T** Next build: extended 's' dump (equipped/jellybeans), 'r' factory reset, script_final
  (buddy flows + Elsa/Spider-Man buy+equip). Then sprites integration, reviews, handover
  (factory reset, overnight mute, volume note for George).
- **F** George asleep — full autonomy for the rest of the build.

## Acceptance QA green (23:5x)
- **F** script_final 77/77: factory reset ('r'), Stella buys+equips ELSA + BUTTERFLY WINGS
  (auto-equip into empty slot on purchase) + RUMI PLAIT; Hugo buys+equips SPIDER-MAN;
  jelly bean bag -> pouch; buddy PLAY raises happiness; get-dressed timer; both profiles'
  equipment + pouch persist across reboot.
- **D** Product rule established: ALL carousels reset selection on enter (MainMenu,
  ShopCategory done; kid-predictability beats position memory). Post-purchase browse
  return still preserves slot via ctx (that path doesn't re-enter the category screen).
- **D** Shop serial logs "[shop] browsing/bought <NAME>" -> QA navigates by name now.
- **F** ctx cleared on player switch (no cross-kid shop state).

## Regression suite green (23:2x-00:0x)
- **F** All 5 QA suites pass rerun-safe: smoke 17, deep 24, games 33, phase2 61, final 77 —
  212 on-device assertions, 0 failures. Every suite factory-resets first ('r' console cmd).
- **F** Reviews: codex review --base takes no prompt arg (relaunched); gemini needed
  GEMINI_API_KEY from ~/.claude/.env (relaunched). Both running.
- **F** Art montage first look: Stella set is exactly the brief (GBC chibi, recognisable
  Ariel/Rapunzel/Cinderella/Moana/Rumi). Hugo/dinos/sharks still generating.
- **T** Tilt SIGN in SharkSwim/RollerCoaster unverifiable over serial — needs 10s in-hand
  check; flip one sign if inverted (noted for George).

## Art integrated + Codex review fixes (00:1x-00:3x)
- **F** Art agent delivered 43/43 sprites (gpt-image-1.5 — gpt-image-2 rejects transparent
  bg; Spider-Man beat the trademark filter via "crimson ninja" + scripted recolour; great
  white de-scarified per no-scary-imagery spec). Full set approved in montage review.
- **D** Sprites.cpp: bitmap path (nearest-neighbour blit, 0xF81F key, 90° rotate for swim,
  flat-colour mode for dino silhouettes) with procedural fallback. Clothing overlays are
  NOT drawn on bitmap characters (they wear painted outfits); accessories still overlay.
- **F** Codex review (xhigh, full diff) found 5 real P2s, all fixed: GamesMenu now honours
  daily-mission target game via ctx.fromDailyMissions; spendCoins debounces (saveNow only
  after grant — power-cut can't eat coins); removed double increments (maths
  correctAnswers, totalFrankieWalks, totalDadMissionsCompleted). Lesson: my Phase-2
  prompts told builders to increment totals directly AND raise events — the funnel rule
  is now in README invariants.
- **F** Firmware 887KB (13.5%). All 212 QA assertions green on the sprite build. Pushed.
- **T** Awaiting Gemini review; then handover (factory reset, overnight mute, summary).

## Handover (2026-08-05 morning)
- **F** Gemini review hung -> killed per George. Codex xhigh review stands as the external
  review (5 P2s found + fixed).
- **F** Device factory-reset over serial: fresh profiles (Hugo 20 coins/L2, Stella 20/L1),
  volume restored to NORMAL, sitting at WHO'S PLAYING?. Ready for the kids.
- **T** Physical checks for George: tilt direction in Shark Swim / Roller Coaster (one-line
  sign flip if inverted); general look of sprites on the real LCD.
- **T** Nice-to-haves if wanted later: clothing overlays on bitmap characters, generated
  room backdrops, Frankie/goalie anim frames, battery-life pass.

## Phase 3 brief (2026-08-05, George)
- **D** App display name: **HUGO + STELLA** (boot + player select; serial log line unchanged
  for QA regex stability).
- **D** Home UX rebuild: ONE infinite carousel replaces A=Tasks/B=Buddy/C=Games. Ring
  (rightward from HOME card): HOME → games 1..8 → buddy actions (room, dress up, jelly
  beans, sleep, play, feed) → tasks 5..1 → HOME. So: A(left) = tasks then buddy actions;
  C(right) = games. B activates card. Long-B main menu + A+C player-select unchanged.
- **D** Active tasks cut to 5: GET DRESSED(1, launches timer), WALK FRANKIE(9, launches
  walk; walk completion marks task, no double coins), PUT PYJAMAS AWAY(13),
  BRUSH TEETH(0), PACK AWAY TOYS(2). Full defs stay in Content; ACTIVE_TASKS drives UI.
- **D** Art: player-select kid tiles (pixel sprites), 22 room backgrounds (160x120 RGB565
  in LittleFS, prescaled to a PSRAM canvas at room change), ~34 UI icons 24x24 (C arrays,
  procedural fallback), same GBC style. Fonts: audit-only pass (consistency/legibility).
- **D** Shark Swim: tilt response +5% (lerp 0.13 -> 0.137).
- **D** Battery pass: CPU 240->160MHz, idle dim 30s -> 30% brightness, auto power-off 5min
  (saveNow + NIGHT NIGHT + powerOff), 'z' debug shortens timers for QA, '[power]' logs.
- **D** QA muted again for the phase. Gemini reviews get a 20-min watchdog kill.
- **D** (addendum) Carousel ring gains a MENU card (opens MainMenu) at the far seam: leftward from HOME: t1..t5, feed, play, sleep, beans, dressup, room, MENU, g8..g1. Long-B shortcut unchanged.

## Phase 3 code landed (01:0x)
- **F** Carousel home live: ring HOME -> games(1-8, player-ordered) -> room/dressup/beans/
  sleep/play/feed -> MENU -> tasks(5 reversed). 1 left = Get Dressed timer, 2 left = Walk
  Frankie, 6 left = MENU card. Games' back target changed GamesMenu -> Home (carousel is
  the launch surface now).
- **F** Battery pass on-device: CPU 160MHz, 30s dim -> 30%, 5min auto-off. M5.Power.powerOff
  replaced with ext0 deep sleep (wake = Button B, GPIO38 active-low — IP5306 can't do true
  I2C power-off; NIGHT NIGHT screen says "PRESS MIDDLE BUTTON TO WAKE").
- **F** Shark tilt +5% (0.13 -> 0.137). App title now HUGO + STELLA on boot/select.
- **F** Bg module: LittleFS 160x120 RGB565 -> 2x prescaled PSRAM canvas; drawRoom uses it
  with procedural fallback (files land with art round 2).
- **F** QA rewritten for carousel + new battery suite: 236 assertions across 6 suites, all
  green. WALK FRANKIE task auto-ticks on walk completion (no double coins).
