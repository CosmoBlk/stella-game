# Pocket Buddy — Game Specification (from the family brief)

Family device built for two small kids, Hugo and Stella. Offline only. No parent mode, no PINs,
trust-based task completion. Never punish: no buddy death, no lost items, no negative streaks,
no coin loss, no stressful failure states. Big graphics, minimal reading, three-button controls.

## Controls (global)
- A = left/prev · B = confirm/centre · C = right/next
- Hold B (~800ms) = back · Hold A+C = player select screen
- Task/mission completion = hold B for ~2s (with visible progress)
- Tilt games: tilt to steer, B pauses, hold B exits
- No double-clicks, no combos beyond A+C, no text entry

## Profiles
Startup: "WHO'S PLAYING?" — two big cards [HUGO] [STELLA]. A=Hugo, C=Stella, B=confirm
highlighted. Everything is fully separate per profile (coins, buddy, inventory, collections,
tasks, missions, maths level, equipped items). Never mix.

Themes — Hugo: soccer/dinos/sharks/Spider-Man/space/cowboys/robots; blue/green/orange/red.
Stella: princesses/butterflies/sparkles/mermaids/unicorns/Rumi/music; pink/purple/turquoise/gold.
Never colour-only signalling — pair with icons/shapes/labels.

## Home screen
Shows: player name, active character in current room, coins, equipped outfit/accessory,
buddy need icons, three action icons. A=Tasks, B=interact with buddy, C=Games.
Long-press B opens Main Menu: BUDDY, TASKS, GAMES, SHOP, DAD MISSIONS, WALK FRANKIE,
DAILY MISSIONS, COLLECTION, GET DRESSED TIMER, SETTINGS, SWITCH PLAYER (each with icon).

## Buddy system
Needs: Hunger (apple), Happiness (smiley), Energy (moon), 0-100, slow decline over real time.
Buddy never dies/leaves; low needs only change expression (hungry: rubs tummy; bored: asks to
play; tired: yawns/lies down). Return after absence: "I MISSED YOU!". Interactions: Feed, Play,
Sleep, Give toy, Change character/outfit/accessory/room, Give jelly beans.

## Economy
Earn: tiny task 5, normal task 10, bigger task 15-20, Walk Frankie 20, Dad Mission 20-40,
mini-game 1-8, all three daily missions 30, rare collectible bonus 5-15.
Prices: treat 5-10, accessory 20-40, toy 30-60, clothing 40-80, room 60-120, character 80-150,
premium character 120-200. Coins animate into balance. Save immediately on purchase.

## Tasks (once per day each, trust-based, hold-B 2s to claim)
BRUSH TEETH(10) · GET DRESSED(10, links to timer) · PACK AWAY TOYS(15) · PUT SHOES AWAY(5) ·
HELP MAKE THE BED(10) · HELP SET THE TABLE(10) · READ A BOOK(10) · PLAY OUTSIDE(10) ·
HELP FEED FRANKIE(10) · WALK FRANKIE(20) · DIRTY CLOTHES IN BASKET(5) · HELP CLEAN UP(15) ·
CARRY OWN PLATE(5) · PUT PYJAMAS AWAY(5) · WATER A PLANT(10)
Flow: pick task -> full-screen icon+label -> child does it -> hold B 2s -> celebration +
coins -> big tick for the day. Daily reset via stored elapsed time (no internet).

## Get Dressed Timer
Exactly 60s default. Start screen "GET DRESSED!" with hourglass + big 60 + Start on B.
Counts down with huge seconds + animated falling sand; gentle tick every 10s; energetic final
5s (not alarming). At zero: "ALL DRESSED?" A=Not yet, B=Done, C=Add 30 seconds.
Done -> celebration + Get Dressed task reward + mark task done.

## Walking Frankie (French Bulldog)
IMU movement detection (not accurate steps, no GPS). Frankie walks/animates while moving;
paw prints/bones progress path; occasional quiet barks; discoveries during movement:
stick, bone, butterfly, soccer ball, puddle, flower, bird, leaf, jelly bean, toy dinosaur,
shell, small treasure chest. Complete at ~5 min movement. "FRANKIE HAD FUN! +20 COINS".
Occasionally award a collectible.

## Dad Missions (30)
FIND SOMETHING RED · FIND SOMETHING BLUE · FIND A FEATHER · FIND THREE DIFFERENT LEAVES ·
COUNT FIVE BIRDS · BUILD A TALL LEGO TOWER · DRAW A DINOSAUR · DRAW A PRINCESS ·
DO TEN BIG JUMPS · KICK A BALL TEN TIMES · GIVE FRANKIE A GENTLE PAT ·
FIND A CIRCLE SHAPE · FIND A TRIANGLE SHAPE · TELL DAD YOUR FAVOURITE ANIMAL ·
TELL DAD YOUR FAVOURITE DINOSAUR · DANCE FOR TWENTY SECONDS · HELP DAD CARRY SOMETHING ·
FIND SOMETHING THAT SMELLS NICE · PRETEND TO BE A SHARK · PRETEND TO BE A PRINCESS ·
ROAR LIKE A DINOSAUR · WALK LIKE A T-REX · FIND SOMETHING BEGINNING WITH B ·
FIND SOMETHING SOFT · FIND SOMETHING TALLER THAN YOU · FIND SOMETHING SMALLER THAN YOUR HAND ·
SING A SONG WITH DAD · FIND FIVE JELLY BEAN COLOURS · PUT THREE TOYS BACK ·
MAKE FRANKIE'S FUNNIEST FACE
Flow: random not-recently-used mission -> big illustration + short text + intro sound ->
child does it -> hold B 2s -> 20-40 coins, occasional badge. Max 3 rewarded/day/child
(replayable without reward after).

## Daily Missions
3/day per profile from audience-filtered pool. Hugo pool: solve 5 maths questions, score 2
penalties, find 3 dinosaurs, get-dressed timer, walk Frankie, one dad mission, play shark
game, complete one task. Stella pool: pop 10 bubbles, find 3 colours, one princess dance,
get-dressed timer, walk Frankie, one dad mission, feed buddy, complete one task.
All three done -> claim 30 coins (or collectible/jelly beans). No penalties or streak loss.

## Games (rounds 20-60s)
**Penalty Kick** (Hugo): wide front-facing goal, central goalie, ball on spot, three zones
L/C/R mapped to A/B/C. Countdown 3-2-1-0 shown before each kick; player picks direction any
time during countdown (highlighted); kick fires automatically at 0 (default centre if none).
Goalie picks dive L/C/R. Different direction: 90-95% goal. Same: 25-40% goal. Occasional
post/wide (uncommon). Optional: pick near 0 = powerful shot. 5 penalties/round, show score
X/5, celebrate goals, friendly on saves (never mocking). Coins: 0-1:1, 2:2, 3:4, 4:6, 5:8.

**Find the Dinosaur** (Hugo): 2-3 big dinosaur choices on A/B/C, prompt like "FIND THE
T-REX!". Correct: animates + sound + star. Wrong: no coin loss, gently show correct, retry.
12 dinosaurs (T-Rex, Triceratops, Stegosaurus, Brachiosaurus, Raptor, Pteranodon,
Ankylosaurus, Spinosaurus, Parasaurolophus, Carnotaurus, Diplodocus, Pachycephalosaurus).
Modes: find named / silhouette / footprint / herbivore / carnivore / flyer / horns / long-neck.
5 questions/round; 1-6 coins; chance of dinosaur sticker.

**Hugo Maths** (adaptive, starts Level 2, three answers on A/B/C, no typing):
L1: +/- within 10, missing numbers, count by 2s. L2: +/- within 20, comparisons, doubles.
L3: +/- within 50, sequences, groups-of multiplication. L4: +/- within 100, 2/5/10 times
tables, sharing division, multi-step visual. L5: mixed arithmetic, missing operators, harder
sequences, word problems, picture fractions. 5 questions/round; 3 strong rounds -> level up
fast; repeated struggle -> quiet level down (never discouraging). Wrong answer: "GOOD TRY!"
+ retry/show solution. Rewards: 1 coin finish + up to 7 for accuracy/difficulty; occasional
badge. Track per-category strength (add/sub/mul/div/seq/cmp/frac).

**Roller Coaster** (both): auto-moving cart, tilt L/R to steer, collect stars/jelly beans/
coins/butterflies/eggs/balls, gentle obstacles bounce funny (no game over). 30-45s rounds.
Environments themed per catalog rooms.

**Princess Dance** (Stella): 1-3 big coloured symbols aligned to A/B/C (pink star, blue
snowflake, purple butterfly, yellow crown, red heart); pressing makes the selected princess
dance. Starts single-symbol, later 2-3 step sequences. No punishment. Dancers: Elsa, Ariel,
Rapunzel, Cinderella, Moana, Rumi. 1-5 coins; chance of accessory find/dance unlock.

**Colour Match** (Stella): "FIND PINK!" big target; three objects on A/B/C (pink crown, blue
butterfly, yellow star, purple dress, green frog, red apple, orange fish, white snowflake...).
Colour + shape paired. No penalty. Playable without reading.

**Bubble Pop** (Stella, also Hugo): three big bubbles on A/B/C; press pops with soft pop +
instant replacement; sometimes reveals star/butterfly/coin/jelly bean/tiny character. ~30s
rounds, no loss condition. Operable by a 2-year-old.

**Shark Swim** (Hugo): tilt-controlled friendly shark; collect fish/bubbles/treasure/jelly
beans; avoid seaweed/rocks/jellyfish (bounce, never end). Treasure chests may hold
collectibles. 30-45s. Unlockable sharks: blue, hammerhead, great white, tiger, baby-style,
robot. No blood/hunting imagery.

## Shop
Categories: CHARACTERS, ACCESSORIES, CLOTHES, TOYS, ROOMS, TREATS. Item: big preview, name,
price, owned/equipped indicators. A=prev, C=next, B=preview/buy/equip, hold B=back.
Confirm: "BUY FOR 80 COINS?" -> "IT'S YOURS!". Save immediately.

## Jelly beans
Both kids. Bag = 10 coins. Feeding: +happiness, slight +hunger, rainbow animation, cheerful
sound. Colours: red, blue, green, yellow, orange, pink, purple, white, black, rainbow (rare).
Also: game collectibles, Frankie discoveries, coaster pickups, daily rewards, collection book.

## Collection book (per player, free to view)
Categories: characters, accessories, clothing, toys, rooms, dinosaurs, sharks, jelly beans,
Frankie discoveries, dad badges, game badges. Locked = silhouette + "?", e.g. "DINOSAURS
7/12 FOUND".

## Audio (synthesized, original)
Button select, coin, purchase, pop, kick, goal, save, cheer, countdown, roar, splash,
sparkle, Rumi music cue, jelly bean, bark, sleep music, mission complete, task complete,
hourglass tick, timer end. Volume: Off/Quiet/Normal in Settings (standard option, no parent
mode).

## Acceptance criteria (47-point condensed)
Separate working profiles; buddy feed/play/sleep, never dies; tasks daily-claim-once;
timer 60->0 with hourglass + add-30; Frankie responds to real movement; dad missions reward;
3 daily missions each; penalty kick full flow with goalie + countdown + posts/misses;
maths starts above counting and adapts across 6 categories; 12 dinosaurs; coaster + shark
tilt; princess dance sequences; colour match + bubble pop pre-reader operable; Stella can buy
and use Elsa/Ariel/Rapunzel/Cinderella/Moana/Rumi + Rumi Plait + Butterfly Wings; Hugo can
buy and use Spider-Man/Woody/Buzz/Shark; jelly beans purchasable and usable by both; all
purchases + coins persist after restart/battery loss; no internet; no dead menu items; fully
responsive on the Fire.
