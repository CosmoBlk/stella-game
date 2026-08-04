#!/usr/bin/env python3
"""On-device QA harness for Pocket Buddy (M5Stack Fire).

Drives the app over USB serial using the DebugConsole:
  a/b/c = short press A/B/C   A/B/C = long press   x = A+C combo
  h = 2s hold-B completion    s = state dump   d = force daily reset
  t = +1h pseudo-clock        g = grant 100 coins   ? = help

A test step is (send, expect_regex, timeout_s, note). The harness sends the
chars (empty string = just wait), then scans serial lines until the regex
matches or times out. All traffic is logged to qa_log.txt.

Usage: python3 tools/qa_walkthrough.py [--port PORT] [--script NAME]
"""
import argparse, re, sys, time

try:
    import serial
except ImportError:
    sys.exit("pyserial missing: python3 -m pip install --user pyserial")

PORT = "/dev/cu.usbserial-5B090283871"
BAUD = 115200


class Harness:
    def __init__(self, port):
        self.ser = serial.Serial(port, BAUD, timeout=0.2)
        self.log = open("qa_log.txt", "a")
        self.passed, self.failed = [], []

    def reset_device(self):
        """EN pulse via RTS (esptool-style reset into normal boot)."""
        self.ser.dtr = False
        self.ser.rts = True
        time.sleep(0.1)
        self.ser.rts = False
        time.sleep(0.5)
        self.ser.reset_input_buffer()

    def mute(self):
        """Silence the speaker for QA (persists via settings save)."""
        self.step("0", r"\[dbg\] volume 0", 3, "muted for QA")

    def _readline(self):
        line = self.ser.readline().decode("utf-8", "replace").rstrip()
        if line:
            self.log.write(line + "\n")
            self.log.flush()
        return line

    def step(self, send, expect, timeout=5.0, note=""):
        label = note or f"send {send!r} expect /{expect}/"
        for ch in send:
            self.ser.write(ch.encode())
            self.ser.flush()
            time.sleep(0.35)  # one press per few frames
        if not expect:
            self.passed.append(label)
            return True
        rx = re.compile(expect, re.IGNORECASE)
        deadline = time.time() + timeout
        while time.time() < deadline:
            line = self._readline()
            if line and rx.search(line):
                print(f"  PASS  {label}   [{line}]")
                self.passed.append(label)
                return True
        print(f"  FAIL  {label} (timeout {timeout}s)")
        self.failed.append(label)
        return False

    def drain(self, seconds=1.0):
        deadline = time.time() + seconds
        while time.time() < deadline:
            self._readline()

    def summary(self):
        print(f"\n=== QA: {len(self.passed)} passed, {len(self.failed)} failed ===")
        for f in self.failed:
            print(f"  FAILED: {f}")
        return not self.failed


PSEL = r"->\s*(WHO'?S? ?PLAYING|PLAYER[ _]?SELECT)"


def script_smoke(h: Harness):
    """Boot -> both profiles -> core screens reachable."""
    h.reset_device()
    h.step("", r"\[boot\] Pocket Buddy ready", 15, "boots")
    h.mute()
    h.step("", PSEL, 8, "auto-advances to PlayerSelect")
    h.step("b", r"->\s*HOME", 5, "B selects highlighted profile -> Home")
    h.step("s", r"player=HUGO", 3, "Hugo profile active")
    h.step("B", r"->\s*MAIN[ _]?MENU", 5, "long-B opens MainMenu")
    h.step("B", r"->\s*HOME", 5, "long-B backs out to Home")
    h.step("a", r"->\s*TASKS", 5, "A opens Tasks")
    h.step("B", r"->\s*HOME", 5, "back to Home")
    h.step("c", r"->\s*GAMES", 5, "C opens GamesMenu")
    h.step("B", r"->\s*HOME", 5, "back to Home")
    h.step("b", r"->\s*BUDDY", 5, "B opens Buddy")
    h.step("x", PSEL, 5, "A+C returns to PlayerSelect")
    h.step("c", "", 0, "highlight Stella")
    h.step("b", r"->\s*HOME", 5, "Stella -> Home")
    h.step("s", r"player=STELLA", 3, "Stella profile active")


def script_deep(h: Harness):
    """Task claim, persistence across reboot, shop purchase."""
    h.reset_device()
    h.step("", r"\[boot\] Pocket Buddy ready", 15, "boots")
    h.mute()
    h.step("", PSEL, 8, "PlayerSelect")
    h.step("b", r"->\s*HOME", 5, "Hugo -> Home")
    # --- task completion ---
    h.step("a", r"->\s*TASKS", 5, "open Tasks")
    h.step("b", r"->\s*TASK", 5, "open first task detail")
    h.step("h", r"->\s*TASK ?COMPLETE", 6, "hold-B claims task")
    h.drain(3.5)  # celebration auto-return
    h.step("s", r"coins=30", 3, "task coins awarded (20+10)")
    # --- shop purchase: jelly bean bag ---
    h.step("B", r"->\s*(HOME|TASKS)", 5, "back toward Home")
    h.drain(0.5)
    h.step("B", r"->\s*MAIN ?MENU", 6, "open MainMenu")
    h.step("ccc", "", 0, "highlight SHOP")
    h.step("b", r"->\s*SHOP", 5, "enter Shop categories")
    h.step("ccccc", "", 0, "highlight TREATS")
    h.step("b", r"->\s*SHOP ?BROWSE", 5, "browse treats")
    h.step("b", r"(BUY|CONFIRM|PREVIEW)", 5, "select jelly bean bag")
    h.step("b", r"(PURCHASE|RESULT|YOURS)", 6, "confirm purchase")
    h.drain(3.0)
    h.step("s", r"coins=20", 3, "paid 10 coins for jelly beans")
    # --- persistence across reboot ---
    h.step("g", r"\[dbg\] grant", 3, "grant 100 coins")
    h.drain(2.5)  # debounced save flush
    h.reset_device()
    h.step("", r"\[boot\] Pocket Buddy ready", 15, "reboots")
    h.mute()
    h.step("", PSEL, 8, "PlayerSelect after reboot")
    h.step("b", r"->\s*HOME", 5, "Hugo -> Home again")
    h.step("s", r"coins=120", 3, "coins persisted across reboot")


def _mash_game(h: Harness, label):
    """Enter highlighted game, mash inputs ~8s, watch for crash-reboot, exit."""
    h.step("b", r"->\s*\w", 5, f"{label}: enter")
    for _ in range(4):
        for ch in "abc":
            h.ser.write(ch.encode())
            time.sleep(0.4)
        h.drain(0.6)
    # a crash would print the boot banner again
    h.step("", "", 0, "")
    crashed = False
    deadline = time.time() + 1.0
    while time.time() < deadline:
        if "Pocket Buddy ready" in (h._readline() or ""):
            crashed = True
    if crashed:
        print(f"  FAIL  {label}: device rebooted (crash!)")
        h.failed.append(f"{label} crashed")
    else:
        h.passed.append(f"{label} survived mash")
    h.step("B", r"->\s*(GamesMenu|GAMES)", 8, f"{label}: exit to GamesMenu")
    h.drain(1.0)


def script_games(h: Harness):
    """Mash-test every Phase 1 game + daily reset cycle."""
    h.reset_device()
    h.step("", r"\[boot\] Pocket Buddy ready", 15, "boots")
    h.mute()
    h.step("", PSEL, 8, "PlayerSelect")
    h.step("b", r"->\s*HOME", 5, "Hugo -> Home")
    h.step("c", r"->\s*GAMES", 5, "open GamesMenu")
    for i in range(4):
        _mash_game(h, f"game{i}")
        h.step("c", "", 0, "next game")
    # --- daily reset cycle ---
    h.step("B", r"->\s*HOME", 6, "back to Home")
    h.step("s", "coins=", 3, "pre-reset state")
    h.step("d", r"daily reset", 3, "force daily reset")
    h.step("s", r"day=2", 3, "day advanced")
    h.step("a", r"->\s*TASKS", 5, "tasks after reset")
    h.step("b", r"->\s*TASK", 5, "open task detail")
    h.step("h", r"->\s*TASK ?COMPLETE", 6, "task claimable again after reset")
    h.drain(3.5)


def _coins(h: Harness):
    """Send 's', parse coins= from the dump."""
    h.drain(0.3)
    h.ser.write(b"s")
    deadline = time.time() + 3
    while time.time() < deadline:
        line = h._readline()
        m = re.search(r"coins=(\d+)", line or "")
        if m:
            return int(m.group(1))
    return None


def script_phase2(h: Harness):
    """Dad missions, Frankie walk, daily missions, collection, all 8 games.

    Uses debug jump commands (H/D/F/M/K/G) so every flow starts deterministic.
    """
    h.reset_device()
    h.step("", r"\[boot\] Pocket Buddy ready", 15, "boots")
    h.mute()
    h.step("", PSEL, 8, "PlayerSelect")
    h.step("b", r"->\s*HOME", 5, "Hugo -> Home")
    # --- menu coverage: order-agnostic sweep of all entries ---
    seen = set()
    for i in range(11):
        h.step("H", "", 0, "")
        h.drain(0.4)
        h.step("B", r"->\s*MAIN ?MENU", 6, f"menu open {i}")
        h.ser.write(b"c" * i and b"")  # placeholder, replaced below
        for _ in range(i):
            h.ser.write(b"c")
            time.sleep(0.3)
        h.ser.write(b"b")
        deadline = time.time() + 5
        target = None
        while time.time() < deadline:
            line = h._readline()
            m = re.search(r"MAIN MENU\s*->\s*(.+)$", line or "", re.IGNORECASE)
            if m:
                target = m.group(1).strip()
                break
        if target:
            seen.add(target)
        h.drain(0.6)
    print(f"  INFO  menu targets: {sorted(seen)}")
    if len(seen) >= 10:
        h.passed.append(f"menu opens {len(seen)} distinct screens")
    else:
        h.failed.append(f"menu only reached {len(seen)} screens: {sorted(seen)}")
    # --- back to Hugo Home (menu sweep may have switched player) ---
    h.step("x", PSEL, 6, "back to PlayerSelect")
    h.step("a", "", 0, "highlight Hugo")
    h.step("b", r"->\s*HOME", 5, "Hugo again")
    # --- dad mission with reward ---
    before = _coins(h)
    h.step("D", r"->\s*DAD", 5, "jump to Dad Missions")
    h.step("b", r"->\s*DAD ?MISSION ?DETAIL", 5, "open mission detail")
    h.step("h", r"->\s*DAD ?MISSION ?RESULT", 6, "hold-B completes dad mission")
    h.drain(3.5)
    after = _coins(h)
    if before is not None and after is not None and 20 <= after - before <= 40:
        print(f"  PASS  dad mission paid {after - before} coins")
        h.passed.append("dad mission reward")
    else:
        print(f"  FAIL  dad mission reward (before={before} after={after})")
        h.failed.append("dad mission reward")
    # --- Frankie walk with simulated movement ---
    h.step("F", r"->\s*FRANKIE", 5, "jump to Frankie intro")
    h.step("b", r"->\s*FRANKIE ?WALK", 5, "start walk")
    h.step("w", r"\[dbg\] movement", 3, "inject movement")
    h.drain(6.0)
    h.step("w", r"\[dbg\] movement", 3, "more movement")
    h.drain(4.0)
    crashed = False
    h.ser.write(b"s")
    deadline = time.time() + 2
    while time.time() < deadline:
        if "Pocket Buddy ready" in (h._readline() or ""):
            crashed = True
    if crashed:
        h.failed.append("frankie walk crashed")
    else:
        h.passed.append("frankie walk survives movement")
    # --- daily missions screen ---
    h.step("M", r"->\s*DAILY", 5, "jump to Daily Missions")
    h.drain(1.0)
    # --- collection book ---
    h.step("K", r"->\s*COLLECTION", 5, "jump to Collection")
    h.step("b", r"->\s*COLLECTION ?GRID", 5, "open a grid")
    h.step("c", "", 0, "page")
    h.step("B", r"->\s*COLLECTION", 5, "back to categories")
    # --- all 8 games, deterministic: fresh GamesMenu jump each time ---
    entered = set()
    for i in range(8):
        h.step("G", r"->\s*GamesMenu", 5, "")
        h.drain(0.6)
        for _ in range(i):
            h.ser.write(b"c")
            time.sleep(0.3)
        h.ser.write(b"b")
        name = None
        deadline = time.time() + 5
        while time.time() < deadline:
            line = h._readline()
            m = re.search(r"->\s*(\w+Game)", line or "")
            if m:
                name = m.group(1)
                break
        if name:
            entered.add(name)
            h.ser.write(b"w")
            for _ in range(3):
                for ch in "abc":
                    h.ser.write(ch.encode())
                    time.sleep(0.3)
                h.drain(0.4)
            crashed = False
            deadline = time.time() + 1.0
            while time.time() < deadline:
                if "Pocket Buddy ready" in (h._readline() or ""):
                    crashed = True
            if crashed:
                print(f"  FAIL  {name} crash-reboot")
                h.failed.append(f"{name} crashed")
            else:
                print(f"  PASS  {name} mash-survived")
                h.passed.append(f"{name} ok")
        else:
            h.failed.append(f"game slot {i} did not open")
        h.drain(0.5)
    print(f"  INFO  distinct games: {sorted(entered)}")
    if len(entered) == 8:
        h.passed.append("all 8 games entered")
    else:
        h.failed.append(f"only {len(entered)}/8 games entered: {sorted(entered)}")


def _dump(h: Harness):
    """Send 's', return the full dump line."""
    h.drain(0.3)
    h.ser.write(b"s")
    deadline = time.time() + 3
    while time.time() < deadline:
        line = h._readline()
        if "player=" in (line or ""):
            return line
    return ""


def _browse_to(h: Harness, name):
    """In ShopBrowse: press C until '[shop] browsing <name>' is current."""
    h.drain(0.5)
    # trigger a fresh browsing line without moving: enter() already logged; ask by moving fwd/back
    for _ in range(30):
        h.ser.write(b"c")
        deadline = time.time() + 3
        current = None
        while time.time() < deadline:
            line = h._readline()
            m = re.search(r"\[shop\] browsing (.+)$", line or "")
            if m:
                current = m.group(1).strip()
                break
        if current == name:
            print(f"  PASS  browsed to {name}")
            h.passed.append(f"browse {name}")
            return True
        time.sleep(0.1)
    print(f"  FAIL  could not browse to {name}")
    h.failed.append(f"browse {name}")
    return False


def _buy(h: Harness, name):
    """From ShopBrowse on target item: press B through preview/confirm until bought."""
    bought = False
    for _ in range(4):
        h.ser.write(b"b")
        deadline = time.time() + 3
        while time.time() < deadline:
            line = h._readline()
            if re.search(r"\[shop\] bought " + re.escape(name), line or ""):
                bought = True
                break
        if bought:
            break
    if bought:
        print(f"  PASS  bought {name}")
        h.passed.append(f"buy {name}")
    else:
        print(f"  FAIL  buy {name}")
        h.failed.append(f"buy {name}")
    h.drain(3.2)  # let the result celebration finish/auto-return to browse
    return bought


def _open_shop_category(h: Harness, presses):
    h.step("H", "", 0, "jump home")
    h.drain(0.6)
    h.step("B", r"->\s*MAIN ?MENU", 6, "menu")
    h.step("ccc", "", 0, "to SHOP")
    h.step("b", r"ShopCategory|SHOP ?CATEGORY", 5, "shop categories")
    for _ in range(presses):
        h.ser.write(b"c")
        time.sleep(0.3)
    h.step("b", r"->\s*SHOP ?BROWSE", 5, "browse")


def script_final(h: Harness):
    """Acceptance flows: factory reset, licensed buys + equips, jelly beans,
    buddy interactions, timer, persistence."""
    h.reset_device()
    h.step("", r"\[boot\] Pocket Buddy ready", 15, "boots")
    h.step("r", r"\[dbg\] factory reset", 5, "factory reset")
    h.mute()
    # ---------- STELLA ----------
    h.step("c", "", 0, "highlight Stella")
    h.step("b", r"->\s*HOME", 5, "Stella -> Home")
    h.step("ggg", "", 0, "grant 300 coins")
    h.drain(1.2)
    _open_shop_category(h, 0)          # CHARACTERS
    if _browse_to(h, "ELSA"):
        _buy(h, "ELSA")
    h.step("E", r"CHARACTER", 5, "character select")
    h.step("c", "", 0, "next character")
    h.step("b", "", 0, "equip")
    h.drain(1.5)
    d = _dump(h)
    if re.search(r"eq=1/", d):
        print("  PASS  Stella equipped ELSA")
        h.passed.append("stella equips elsa")
    else:
        print(f"  FAIL  Stella equip Elsa: {d}")
        h.failed.append("stella equips elsa")
    _open_shop_category(h, 1)          # ACCESSORIES
    if _browse_to(h, "BUTTERFLY WINGS"):
        _buy(h, "BUTTERFLY WINGS")     # auto-equips into empty slot
    h.drain(1.0)
    d = _dump(h)
    if "/16/" in d.split("eq=")[-1]:
        print("  PASS  BUTTERFLY WINGS equipped (auto)")
        h.passed.append("stella wings equipped")
    else:
        print(f"  FAIL  wings: {d}")
        h.failed.append("stella wings equipped")
    _open_shop_category(h, 1)          # ACCESSORIES again
    if _browse_to(h, "RUMI PLAIT"):
        _buy(h, "RUMI PLAIT")
    h.step("O", r"ACCESSORY", 5, "accessory select")
    h.step("cc", "", 0, "to plait (none,plait,wings)")
    h.step("b", "", 0, "equip plait")
    h.drain(1.5)
    d = _dump(h)
    if "/15/" in d.split("eq=")[-1]:
        print("  PASS  Stella equipped RUMI PLAIT")
        h.passed.append("stella equips rumi plait")
    else:
        print(f"  FAIL  rumi plait: {d}")
        h.failed.append("stella equips rumi plait")
    # ---------- HUGO ----------
    h.step("x", PSEL, 6, "to PlayerSelect")
    h.step("a", "", 0, "highlight Hugo")
    h.step("b", r"->\s*HOME", 5, "Hugo -> Home")
    h.step("ggg", "", 0, "grant 300")
    h.drain(1.2)
    _open_shop_category(h, 0)          # CHARACTERS
    if _browse_to(h, "SPIDER-MAN"):
        _buy(h, "SPIDER-MAN")
    h.step("E", r"CHARACTER", 5, "character select")
    h.step("c", "", 0, "next")
    h.step("b", "", 0, "equip")
    h.drain(1.5)
    d = _dump(h)
    if re.search(r"eq=65/", d):
        print("  PASS  Hugo equipped SPIDER-MAN")
        h.passed.append("hugo equips spiderman")
    else:
        print(f"  FAIL  spiderman: {d}")
        h.failed.append("hugo equips spiderman")
    _open_shop_category(h, 5)          # TREATS
    if _browse_to(h, "BAG OF JELLY BEANS"):
        _buy(h, "BAG OF JELLY BEANS")
    d = _dump(h)
    if re.search(r"jb=[1-9]", d):
        print("  PASS  jelly beans in pouch")
        h.passed.append("jelly beans bought")
    else:
        print(f"  FAIL  jelly beans: {d}")
        h.failed.append("jelly beans bought")
    # buddy: feed jelly beans via carousel (FEED,PLAY,SLEEP,TOY,JELLY...)
    h.step("H", "", 0, "home")
    h.drain(0.5)
    before = _dump(h)
    h.step("b", r"->\s*BUDDY", 5, "buddy screen")
    h.step("c", "", 0, "carousel: PLAY")
    h.step("b", "", 0, "PLAY")
    h.drain(2.0)
    after = _dump(h)
    mb = re.search(r"needs=\d+/(\d+)", before)
    ma = re.search(r"needs=\d+/(\d+)", after)
    if mb and ma and int(ma.group(1)) > int(mb.group(1)):
        print("  PASS  buddy PLAY raised happiness")
        h.passed.append("buddy play")
    else:
        print(f"  FAIL  buddy play: {before} -> {after}")
        h.failed.append("buddy play")
    # get dressed timer opens
    h.step("T", r"GET ?DRESSED", 5, "timer screen")
    h.step("b", "", 0, "start timer")
    h.drain(3.0)
    h.step("B", "", 0, "leave timer")
    h.drain(1.5)
    # persistence across reboot
    h.drain(2.0)
    h.reset_device()
    h.step("", r"\[boot\] Pocket Buddy ready", 15, "reboots")
    h.mute()
    h.step("", PSEL, 8, "PlayerSelect")
    h.step("a", "", 0, "Hugo")
    h.step("b", r"->\s*HOME", 5, "Hugo home")
    d = _dump(h)
    if re.search(r"eq=65/", d) and re.search(r"jb=[1-9]", d):
        print("  PASS  Spider-Man + jelly beans persisted")
        h.passed.append("persistence")
    else:
        print(f"  FAIL  persistence: {d}")
        h.failed.append("persistence")
    h.step("x", PSEL, 5, "player select")
    h.step("c", "", 0, "Stella")
    h.step("b", r"->\s*HOME", 5, "Stella home")
    d = _dump(h)
    if re.search(r"eq=1/", d) and "/15/" in d.split("eq=")[-1]:
        print("  PASS  Elsa + Rumi Plait persisted for Stella")
        h.passed.append("stella persistence")
    else:
        print(f"  FAIL  stella persistence: {d}")
        h.failed.append("stella persistence")


SCRIPTS = {"smoke": script_smoke, "deep": script_deep, "games": script_games,
           "phase2": script_phase2, "final": script_final}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default=PORT)
    ap.add_argument("--script", default="smoke", choices=sorted(SCRIPTS))
    args = ap.parse_args()
    h = Harness(args.port)
    print(f"=== running {args.script} on {args.port} ===")
    SCRIPTS[args.script](h)
    ok = h.summary()
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
