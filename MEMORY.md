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
