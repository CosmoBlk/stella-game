# Pocket Buddy — Stella & Hugo's M5Stack Fire game

Private family build for Hugo (4) and Stella (2). Tamagotchi-style buddies, real-world
tasks and chores, Walk Frankie (IMU), Dad Missions, daily missions, coins + shops,
collection book, and eight mini-games — fully offline on an M5Stack Fire v2.7.

**Private repo — never make public.** The sprite set includes recognisable
licensed-character pixel art for personal family use only.

## Build & flash
```bash
pio run -t upload        # firmware; device on /dev/cu.usbserial-5B090283871
pio run -t uploadfs      # backgrounds (data/ -> LittleFS). Needed on a fresh
                         # device or after art changes. WARNING: wipes the save.
pio device monitor       # 115200 baud
```

## On-device QA
`tools/qa_walkthrough.py` drives the app over USB serial via the debug console
(button injection, movement simulation, state dumps, factory reset):
```bash
python3 tools/qa_walkthrough.py --script smoke    # also: deep, games, phase2, final
```
All five suites (212 assertions) factory-reset the save first and must pass 100%.

Debug console (serial, single chars): `a b c` press · `A B C` long-press · `x` A+C ·
`h` 2s hold-B · `w` fake movement 10s · `s` state dump · `d` force daily reset ·
`g` +100 coins · `0/1/2` volume off/quiet/normal · `r` factory reset ·
`H D F M K G E O L T` screen jumps · `?` help.

## Layout
- `src/` — App core (screen state machine, input, audio, atomic CRC saves, pseudo-clock
  daily reset), `src/screens/` (24 screens), `src/games/` (8 games), `Content.cpp`
  (catalog: 127 items, 15 tasks, 30 dad missions, mission pool, 12 dinos),
  `Sprites.cpp` (procedural art + bitmap blitter), `Assets*.{h,cpp}` (43 generated
  48×48 pixel sprites).
- `tools/gen_sprites.py` — sprite pipeline (OpenAI image API → quantize → C arrays).
  Regenerate/extend with `python3 tools/gen_sprites.py --help`.
- `SPEC.md` / `PLAN.md` / `ARCHITECTURE.md` / `CATALOG.md` / `MEMORY.md` — the brief,
  build plan, contracts, frozen item ids, and the running build log.

## Design invariants (do not break)
- Buddy never dies; no punishments, no coin loss, no dead ends — everything recoverable
  with A/B/C only.
- Every carousel opens at position 0 (kid predictability).
- All profile mutations go through the event funnel (`App::raiseEvent`) — never
  increment profile totals directly in screens.
- Purchases: coins debit is debounced; `saveNow()` only after the item is granted.
- Save writes are atomic (tmp → verify → bak → rename); saves survive battery loss.
