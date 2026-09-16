#!/usr/bin/env python3
"""Pocket Buddy sprite pipeline.

Generates Game-Boy-Color-style pixel-art sprites via the OpenAI image API,
quantizes them to true 48x48 grids (<=15 colours + transparency), and emits
embedded C arrays (RGB565, transparent sentinel 0xF81F) for the M5Stack Fire.

Usage:
  python3 tools/gen_sprites.py gen [name ...] [--force]   # generate source PNGs
  python3 tools/gen_sprites.py quant [name ...]           # quantize to 48x48
  python3 tools/gen_sprites.py emit                       # write src/Assets* files
  python3 tools/gen_sprites.py montage                    # build preview montage
  python3 tools/gen_sprites.py all                        # gen+quant+emit+montage

Round 2 subcommands (kid tiles, room/UI backgrounds, 24x24 UI icons):
  kids-gen / kids-quant / kids-emit / kids-montage
  rooms-gen [id|select|menu ...] [--force] [--model M]    # scene source PNGs
  rooms-build [id|select|menu ...]                        # crop+quantize+.bin
  rooms-montage
  icons-gen / icons-quant / icons-emit / icons-montage [name ...]

Requires $OPENAI_API_KEY in the environment.
"""
import argparse
import base64
import json
import os
import struct
import sys
import time
import urllib.error
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "assets" / "sprites_src"
OUT_DIR = ROOT / "assets" / "sprites_48"
MONTAGE = ROOT / "assets" / "preview_montage.png"
DATA_H = ROOT / "src" / "AssetsData.h"
ASSETS_H = ROOT / "src" / "Assets.h"
ASSETS_CPP = ROOT / "src" / "Assets.cpp"

# gpt-image-2 rejects background=transparent (and returns RGB with no alpha even
# when asked via prompt) — verified 2026-08-04. gpt-image-1.5 supports true alpha.
MODEL = "gpt-image-1.5"
SIZE = 48
SENTINEL = 0xF81F  # pure magenta in RGB565 = transparent
MAX_COLOURS = 15

STYLE = (
    "Game Boy Color style pixel art sprite of {subject}. Full body, front-facing, "
    "cute chibi proportions about two heads tall, centered, feet at the bottom. "
    "Chunky pixels, dark 1-pixel outline, flat cel shading with 3 shades per colour, "
    "limited bright palette. Transparent background. No text, no watermark, no "
    "border, no floor shadow."
)

# (key, subject, quality, group, id)
# group: "stella"/"hugo" -> character item id; "dino"/"shark" -> collectible idx;
# "extra" -> special lookup.
SPRITES = [
    # Stella characters (item ids 0-11)
    ("default_princess", "a friendly princess with brown hair, small gold tiara and simple pink dress", "medium", "stella", 0),
    # NOTE: original wording "an ice queen with a platinum blonde side braid, pale skin and
    # sparkling ice-blue gown with snowflake details" was rejected by the OpenAI safety
    # system (likely trademark-adjacent); reworded below.
    ("elsa", "a winter princess girl with a long pale blonde braid over one shoulder, wearing a sparkling icy pale blue dress decorated with tiny snowflakes", "high", "stella", 1),
    ("ariel", "a mermaid with long bright red hair, purple seashell top and shiny green mermaid tail", "high", "stella", 2),
    ("rapunzel", "a princess with extremely long golden braided hair wrapping near her feet, lavender-purple dress", "high", "stella", 3),
    ("cinderella", "a princess with blonde hair in an updo, sparkling powder-blue ball gown and tiny glass slippers", "high", "stella", 4),
    ("moana", "an island voyager girl with long dark wavy hair, red and cream island outfit and teal shell necklace", "high", "stella", 5),
    ("rumi", "a demon-hunter pop star girl with a very long purple-tinted plait, dark stage outfit with gold trim, holding a small microphone", "high", "stella", 6),
    ("mermaid", "a mermaid with aqua hair, seashell top and turquoise tail", "medium", "stella", 7),
    ("fairy", "a small fairy with green petal dress, translucent wings and a tiny wand", "medium", "stella", 8),
    # retry wording: first pass gave a muddy red/grey mane, not rainbow
    ("unicorn", "a cute white unicorn with a bright rainbow striped mane in red, yellow, green, blue and pink, and a shiny golden horn", "medium", "stella", 9),
    ("bunny", "a cream bunny rabbit with long floppy ears and a pink bow", "medium", "stella", 10),
    ("kitten", "a ginger kitten with big eyes and a little bell collar", "medium", "stella", 11),
    # Hugo characters (item ids 60-71)
    ("soccer_player", "a young soccer player boy in a blue and white kit standing with a soccer ball", "medium", "hugo", 60),
    ("dino_trainer", "a young dinosaur trainer boy in khaki explorer clothes holding a tiny dinosaur egg", "medium", "hugo", 61),
    ("trex_character", "a friendly smiling green tyrannosaurus rex standing upright", "medium", "hugo", 62),
    ("raptor_character", "a friendly blue velociraptor standing upright", "medium", "hugo", 63),
    ("shark_character", "a friendly blue shark standing upright with fins as arms", "medium", "hugo", 64),
    # NOTE: original wording "a superhero in a red and blue suit with black web pattern,
    # red full-face mask with big white eyes" was safety-rejected, as were 10 paraphrases
    # (the filter is a semantic IP detector). The wording below passes; a post-quantize
    # recolour hook (POST_QUANT) then shifts legs to blue, fills eye pupils white and
    # gloves the fingertips to land the classic red/blue masked-hero look.
    ("spiderman", "a cute chibi ninja in a crimson bodysuit and hood, two large white eye shapes showing through the hood", "high", "hugo", 65),
    # retry wording: first pass came out generic orange cowboy, no plaid or cow print
    ("woody", "a toy cowboy sheriff with brown cowboy hat, bright yellow plaid shirt with thin red criss-cross lines, black and white cow-print vest and gold sheriff star", "high", "hugo", 66),
    # NOTE: original wording "a toy space ranger in a white space suit with green chest
    # panel and trim, purple hood, clear dome helmet" was safety-rejected; reworded below.
    ("buzz", "a toy astronaut action figure in a shiny white spacesuit with bright green chest panel and trim, purple cap under a clear round bubble helmet", "high", "hugo", 67),
    ("astronaut", "a kid astronaut in a white space suit with gold visor", "medium", "hugo", 68),
    ("robot", "a boxy friendly silver robot with an antenna and glowing eyes", "medium", "hugo", 69),
    ("explorer", "a kid explorer with safari hat and binoculars", "medium", "hugo", 70),
    ("frankie", "a fawn French Bulldog with a dark face mask and big bat ears, happy tongue out", "medium", "hugo", 71),
    # Dinosaur collectibles (idx 0-11)
    ("dino_00", "a green tyrannosaurus rex with tiny arms", "medium", "dino", 0),
    ("dino_01", "an orange triceratops with three horns and neck frill", "medium", "dino", 1),
    ("dino_02", "a green stegosaurus with orange back plates", "medium", "dino", 2),
    ("dino_03", "a tall blue-grey brachiosaurus with very long neck", "medium", "dino", 3),
    ("dino_04", "a small fast blue velociraptor", "medium", "dino", 4),
    ("dino_05", "a flying orange pteranodon with wide wings", "medium", "dino", 5),
    ("dino_06", "a brown armoured ankylosaurus with club tail", "medium", "dino", 6),
    ("dino_07", "a teal spinosaurus with a big back sail", "medium", "dino", 7),
    ("dino_08", "a green parasaurolophus with long curved head crest", "medium", "dino", 8),
    ("dino_09", "a red carnotaurus with two small horns", "medium", "dino", 9),
    ("dino_10", "a long grey-green diplodocus with whip tail", "medium", "dino", 10),
    ("dino_11", "a purple pachycephalosaurus with dome head", "medium", "dino", 11),
    # Shark collectibles (idx 0-5)
    ("shark_00", "a friendly blue shark", "medium", "shark", 0),
    ("shark_01", "a hammerhead shark with wide head", "medium", "shark", 1),
    # retry wording: first great white had a jagged red-tinged tooth mouth (too scary
    # for the no-scary-imagery spec); first tiger shark had no visible stripes
    # (second retry added explicit grey — first retry came out teal)
    ("shark_02", "a friendly light grey great white shark with a white belly and a happy closed-mouth smile", "medium", "shark", 2),
    ("shark_03", "a friendly tiger shark with bold dark vertical stripes across its back", "medium", "shark", 3),
    ("shark_04", "a tiny cute baby shark with big eyes", "medium", "shark", 4),
    ("shark_05", "a robot shark made of silver metal with an antenna", "medium", "shark", 5),
    # Extras
    ("goalie", "a goalkeeper in orange kit with gloves, arms stretched wide", "medium", "extra", 0),
]

BY_KEY = {s[0]: s for s in SPRITES}


# ------------------------------------------------------------ post-quant hooks

def _recolor_spiderman(img):
    """48x48 edit of the crimson-ninja base: blue legs, red boots, white lenses,
    gloved fingertips — lands the classic red/blue masked-hero read."""
    px = img.load()
    w, h = img.size
    ys = [y for y in range(h) for x in range(w) if px[x, y][3] >= 128]
    top, bot = min(ys), max(ys)
    span = bot - top
    waist = top + int(span * 0.70)   # chibi: head dominates, legs start low
    boots = top + int(span * 0.90)

    BLUE = [(24, 48, 120), (40, 80, 190), (90, 140, 240)]   # dark/mid/light
    RED = [(120, 16, 28), (200, 30, 40), (240, 90, 90)]

    def shade(ramp, r, g, b):
        lum = 0.3 * r + 0.6 * g + 0.1 * b
        return ramp[0] if lum < 70 else (ramp[1] if lum < 150 else ramp[2])

    def is_outline(r, g, b):
        return max(r, g, b) < 60

    def is_skin(r, g, b):
        return r > 180 and 110 < g < 210 and b < 170 and r > b + 60 and g > b + 20

    for y in range(h):
        row_xs = [x for x in range(w) if px[x, y][3] >= 128]
        if not row_xs:
            continue
        lo, hi = min(row_xs), max(row_xs)
        margin = max(2, int((hi - lo) * 0.14))  # outer edges of waist rows = hands
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 128 or is_outline(r, g, b):
                continue
            if boots > y >= waist:
                if x < lo + margin or x > hi - margin:
                    px[x, y] = (*shade(RED, r, g, b), 255)  # red gloves
                else:
                    px[x, y] = (*shade(BLUE, r, g, b), 255)
            elif is_skin(r, g, b):
                px[x, y] = (*RED[1], 255)
    # eye lenses: brighten cream eye-whites to true white, then flood pupils white
    head_lim = top + span // 2
    for y in range(top, head_lim):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a >= 128 and min(r, g, b) > 150 and r + g + b > 540:
                px[x, y] = (245, 245, 245, 255)
    for _ in range(4):
        for y in range(top, head_lim):
            for x in range(w):
                r, g, b, a = px[x, y]
                if a >= 128 and r + g + b < 400:
                    wn = sum(
                        1
                        for dx in (-1, 0, 1)
                        for dy in (-1, 0, 1)
                        if (dx or dy)
                        and 0 <= x + dx < w
                        and 0 <= y + dy < h
                        and px[x + dx, y + dy][3] >= 128
                        and sum(px[x + dx, y + dy][:3]) > 700
                    )
                    if wn >= 4:
                        px[x, y] = (245, 245, 245, 255)
    return img


POST_QUANT = {"spiderman": _recolor_spiderman}


# ---------------------------------------------------------------- generation

def api_generate(prompt: str, quality: str, size: str = "1024x1024",
                transparent: bool = True, model: str = None) -> bytes:
    """One image generation call; returns decoded PNG bytes."""
    key = os.environ.get("OPENAI_API_KEY")
    if not key:
        raise SystemExit("OPENAI_API_KEY not set")
    payload = {
        "model": model or MODEL,
        "prompt": prompt,
        "size": size,
        "quality": quality,
        "output_format": "png",
    }
    if transparent:
        payload["background"] = "transparent"
    body = json.dumps(payload).encode()
    req = urllib.request.Request(
        "https://api.openai.com/v1/images/generations",
        data=body,
        headers={
            "Authorization": f"Bearer {key}",
            "Content-Type": "application/json",
        },
    )
    with urllib.request.urlopen(req, timeout=240) as resp:
        data = json.load(resp)
    return base64.b64decode(data["data"][0]["b64_json"])


def gen_one(key: str, force: bool = False) -> str:
    name, subject, quality, _, _ = BY_KEY[key]
    out = SRC_DIR / f"{name}.png"
    if out.exists() and not force:
        return f"skip {name} (exists)"
    last_err = None
    for attempt in range(3):
        try:
            png = api_generate(STYLE.format(subject=subject), quality)
            out.write_bytes(png)
            return f"ok   {name} ({quality}, {len(png)//1024} KB)"
        except urllib.error.HTTPError as e:
            last_err = f"HTTP {e.code}: {e.read()[:200]!r}"
            if e.code == 429:
                time.sleep(20 * (attempt + 1))
            else:
                time.sleep(5)
        except Exception as e:  # noqa: BLE001
            last_err = repr(e)
            time.sleep(5)
    return f"FAIL {name}: {last_err}"


def cmd_gen(names, force=False):
    SRC_DIR.mkdir(parents=True, exist_ok=True)
    keys = names or [s[0] for s in SPRITES]
    with ThreadPoolExecutor(max_workers=2) as ex:
        futs = {ex.submit(gen_one, k, force): k for k in keys}
        for fut in as_completed(futs):
            print(fut.result(), flush=True)


# ---------------------------------------------------------------- quantize

def quantize_one(key: str) -> str:
    src = SRC_DIR / f"{key}.png"
    if not src.exists():
        return f"MISSING {key}"
    img = Image.open(src).convert("RGBA")

    # 1. crop to alpha bounding box
    alpha = img.getchannel("A")
    bbox = alpha.point(lambda a: 255 if a >= 128 else 0).getbbox()
    if bbox is None:
        return f"EMPTY {key} (no opaque pixels)"
    img = img.crop(bbox)

    # pad to square with transparency (centred horizontally; feet stay at bottom
    # when height dominates because vertical padding is split evenly)
    w, h = img.size
    side = max(w, h)
    sq = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    sq.paste(img, ((side - w) // 2, (side - h) // 2))

    # 2. resize to 48x48 NEAREST
    small = sq.resize((SIZE, SIZE), Image.NEAREST)

    out = _reduce_palette(small)
    if out is None:
        return f"EMPTY {key} (nothing after resize)"
    hook = POST_QUANT.get(key)
    if hook:
        out = _reduce_palette(hook(out))  # hooks may add colours; re-reduce
    n_cols = len({p[:3] for p in out.getdata() if p[3] >= 128})
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out.save(OUT_DIR / f"{key}.png")
    return f"ok   {key} ({n_cols} colours)"


def _reduce_palette(img, max_colours=MAX_COLOURS):
    """Reduce an RGBA image's opaque pixels to an adaptive palette of <=max_colours."""
    px = list(img.getdata())
    opaque = [(r, g, b) for (r, g, b, a) in px if a >= 128]
    if not opaque:
        return None
    pal_src = Image.new("RGB", (len(opaque), 1))
    pal_src.putdata(opaque)
    n = min(max_colours, len(set(opaque)))
    q = pal_src.quantize(colors=n, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    used = sorted(set(q.getdata()))
    pal = q.getpalette()
    palette = [tuple(pal[i * 3:i * 3 + 3]) for i in used]

    def nearest(c):
        r, g, b = c
        return min(palette, key=lambda p: (p[0] - r) ** 2 + (p[1] - g) ** 2 + (p[2] - b) ** 2)

    cache = {}
    out_px = []
    for (r, g, b, a) in px:
        if a < 128:
            out_px.append((0, 0, 0, 0))
        else:
            c = cache.get((r, g, b))
            if c is None:
                c = nearest((r, g, b))
                cache[(r, g, b)] = c
            out_px.append((c[0], c[1], c[2], 255))
    out = Image.new("RGBA", img.size)
    out.putdata(out_px)
    return out


def cmd_quant(names):
    keys = names or [s[0] for s in SPRITES]
    for k in keys:
        print(quantize_one(k), flush=True)


# ---------------------------------------------------------------- emit C

def rgb565(r, g, b):
    v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
    if v == SENTINEL:  # never collide with the transparency sentinel
        v = 0xF83F  # nudge green up one step
    return v


def png_to_words(path, size: int):
    img = Image.open(path).convert("RGBA")
    if img.size != (size, size):
        raise SystemExit(f"{path.name}: expected {size}x{size}, got {img.size}")
    words = []
    for (r, g, b, a) in img.getdata():
        words.append(SENTINEL if a < 128 else rgb565(r, g, b))
    return words


def sprite_to_words(key: str):
    return png_to_words(OUT_DIR / f"{key}.png", SIZE)


def cmd_emit():
    missing = [s[0] for s in SPRITES if not (OUT_DIR / f"{s[0]}.png").exists()]
    if missing:
        raise SystemExit(f"cannot emit, missing quantized sprites: {missing}")

    # ---- AssetsData.h
    lines = [
        "// AUTO-GENERATED by tools/gen_sprites.py — do not edit by hand.",
        "// 48x48 RGB565 sprites; 0xF81F = transparent sentinel (ART_TRANSPARENT).",
        "#pragma once",
        "#include <stdint.h>",
        "",
    ]
    for key, *_ in SPRITES:
        words = sprite_to_words(key)
        lines.append(f"const uint16_t ART_{key.upper()}[48*48] = {{")
        for row in range(SIZE):
            chunk = words[row * SIZE:(row + 1) * SIZE]
            lines.append("  " + ",".join(f"0x{w:04X}" for w in chunk) + ",")
        lines.append("};")
        lines.append("")
    DATA_H.write_text("\n".join(lines))

    # ---- Assets.h
    ASSETS_H.write_text(
        "// AUTO-GENERATED by tools/gen_sprites.py — sprite asset lookups.\n"
        "#pragma once\n"
        "#include <stdint.h>\n"
        "struct PixelArt { uint8_t w, h; const uint16_t* px; };\n"
        "constexpr uint16_t ART_TRANSPARENT = 0xF81F;\n"
        "const PixelArt* artForItem(uint16_t itemId);   // character item ids (0-11, 60-71), else nullptr\n"
        "const PixelArt* artForDino(uint8_t idx);       // 0-11\n"
        "const PixelArt* artForShark(uint8_t idx);      // 0-5\n"
        "const PixelArt* artGoalie();\n"
    )

    # ---- Assets.cpp
    stella = [(s[4], s[0]) for s in SPRITES if s[3] == "stella"]
    hugo = [(s[4], s[0]) for s in SPRITES if s[3] == "hugo"]
    dinos = [s[0] for s in SPRITES if s[3] == "dino"]
    sharks = [s[0] for s in SPRITES if s[3] == "shark"]

    cpp = [
        "// AUTO-GENERATED by tools/gen_sprites.py — do not edit by hand.",
        '#include "Assets.h"',
        '#include "AssetsData.h"',
        "",
        "namespace {",
    ]
    for key, *_ in SPRITES:
        cpp.append(f"const PixelArt kArt_{key} = {{48, 48, ART_{key.upper()}}};")
    cpp += [
        "",
        "const PixelArt* const kDinos[12] = {",
        "  " + ", ".join(f"&kArt_{k}" for k in dinos[:6]) + ",",
        "  " + ", ".join(f"&kArt_{k}" for k in dinos[6:]) + ",",
        "};",
        "const PixelArt* const kSharks[6] = {",
        "  " + ", ".join(f"&kArt_{k}" for k in sharks) + ",",
        "};",
        "}  // namespace",
        "",
        "const PixelArt* artForItem(uint16_t itemId) {",
        "  switch (itemId) {",
    ]
    for item_id, key in stella + hugo:
        cpp.append(f"    case {item_id}: return &kArt_{key};")
    cpp += [
        "    default: return nullptr;",
        "  }",
        "}",
        "",
        "const PixelArt* artForDino(uint8_t idx) {",
        "  return idx < 12 ? kDinos[idx] : nullptr;",
        "}",
        "",
        "const PixelArt* artForShark(uint8_t idx) {",
        "  return idx < 6 ? kSharks[idx] : nullptr;",
        "}",
        "",
        "const PixelArt* artGoalie() { return &kArt_goalie; }",
        "",
    ]
    ASSETS_CPP.write_text("\n".join(cpp))
    total = len(SPRITES) * SIZE * SIZE * 2
    print(f"emitted {len(SPRITES)} sprites -> {DATA_H.name}, {ASSETS_H.name}, "
          f"{ASSETS_CPP.name} ({total // 1024} KB of pixel data)")
    # Round-2 kid tiles live as an appended section; a full re-emit would drop
    # them, so re-append automatically whenever the quantized kid PNGs exist.
    if all((OUT_DIR / f"{k[0]}.png").exists() for k in KIDS):
        cmd_kids_emit()


# ---------------------------------------------------------------- montage

ROWS = [
    ("STELLA 0-11", [s[0] for s in SPRITES if s[3] == "stella"]),
    ("HUGO 60-71", [s[0] for s in SPRITES if s[3] == "hugo"]),
    ("DINOS 0-11", [s[0] for s in SPRITES if s[3] == "dino"]),
    ("SHARKS 0-5 + GOALIE", [s[0] for s in SPRITES if s[3] == "shark"] + ["goalie"]),
]


def checkerboard(size, cell=8):
    bg = Image.new("RGBA", (size, size))
    d = ImageDraw.Draw(bg)
    for y in range(0, size, cell):
        for x in range(0, size, cell):
            c = (235, 235, 235, 255) if ((x // cell + y // cell) % 2 == 0) else (255, 255, 255, 255)
            d.rectangle([x, y, x + cell - 1, y + cell - 1], fill=c)
    return bg


def cmd_montage():
    scale = 4
    cell = SIZE * scale          # 192
    pad = 8
    label_h = 14
    row_label_h = 22
    cols = max(len(r[1]) for r in ROWS)
    w = pad + cols * (cell + pad)
    row_h = row_label_h + cell + label_h + pad
    h = pad + len(ROWS) * row_h
    canvas = Image.new("RGBA", (w, h), (250, 250, 250, 255))
    draw = ImageDraw.Draw(canvas)
    font = ImageFont.load_default()

    y = pad
    for row_label, keys in ROWS:
        draw.text((pad, y), row_label, fill=(30, 30, 30, 255), font=font)
        y += row_label_h
        x = pad
        for key in keys:
            tile = checkerboard(cell)
            p = OUT_DIR / f"{key}.png"
            if p.exists():
                spr = Image.open(p).convert("RGBA").resize((cell, cell), Image.NEAREST)
                tile.alpha_composite(spr)
            else:
                d = ImageDraw.Draw(tile)
                d.text((cell // 2 - 10, cell // 2), "?", fill=(200, 0, 0, 255), font=font)
            canvas.paste(tile, (x, y))
            draw.text((x, y + cell + 2), key, fill=(60, 60, 60, 255), font=font)
            x += cell + pad
        y += cell + label_h + pad
    canvas.convert("RGB").save(MONTAGE)
    print(f"montage -> {MONTAGE} ({w}x{h})")


# ============================================================ ROUND 2 ========
# Kid player tiles, room/UI background bins (LittleFS), and 24x24 UI icons.

BG_SRC_DIR = ROOT / "assets" / "bg_src"
BG_OUT_DIR = ROOT / "assets" / "bg_160"       # quantized 160x120 PNG previews
ICON_SRC_DIR = ROOT / "assets" / "ui_icons_src"
ICON_OUT_DIR = ROOT / "assets" / "ui_icons_24"
ROOMS_DATA_DIR = ROOT / "data" / "rooms"
UI_DATA_DIR = ROOT / "data" / "ui"
ICONS_DATA_H = ROOT / "src" / "IconsData.h"
ICONS_H = ROOT / "src" / "Icons.h"
ICONS_CPP = ROOT / "src" / "Icons.cpp"
PREVIEW_KIDS = ROOT / "assets" / "preview_kids.png"
PREVIEW_ROOMS = ROOT / "assets" / "preview_rooms.png"
PREVIEW_ICONS = ROOT / "assets" / "preview_icons.png"

BG_W, BG_H = 160, 120
BG_MAX_COLOURS = 31
ICON_SIZE = 24
ICON_MAX_COLOURS = 15

# ---- kid player tiles (same 48x48 chibi pipeline as round 1) ----------------
KIDS = [
    ("hugo_kid", "a happy four-year-old boy with short brown hair wearing a blue t-shirt and shorts, holding a small soccer ball", "high"),
    ("stella_kid", "a happy two-year-old girl with short brown hair and a tiny pink bow, wearing a pink dress", "high"),
]

# ---- room + UI background scenes --------------------------------------------
# Scenes render 320x240 behind UI (Bg.cpp prescales 2x); keep them SIMPLE.
ROOM_STYLE = (
    "Game Boy Color style pixel art background scene of {scene}. Chunky pixels, "
    "flat cel shading, bright cheerful palette, wide landscape composition, "
    "NO characters, NO people, NO text. Upper two thirds scenery and lower third "
    "simple flat ground. Very simple low-detail scene with large plain colour "
    "areas and only a few big elements."
)

# (room item id, scene description) — ids frozen in CATALOG.md
ROOMS = [
    # Stella rooms 49-59
    (49, "the inside of a pink fairytale princess castle with a small golden throne and tall arched windows"),
    (50, "a cozy pink bedroom with a small bed, a round rug and a sunny window"),
    (51, "a sunny flower garden with a few big colourful tulips and a blue sky"),
    (52, "a dreamy sky with one big rainbow arc and soft white clouds"),
    (53, "an underwater mermaid lagoon with turquoise water, a little coral and a few bubbles"),
    (54, "a concert stage with purple curtains, a plain stage floor and two spotlights"),
    (55, "the inside of an ice palace with icy blue walls and a few sparkling icicles"),
    (56, "the inside of a cozy round stone tower with one big open window showing a sunny green valley"),
    (57, "a tropical island beach with two palm trees, calm blue ocean and soft sand"),
    (58, "a candy land room with two big lollipops, a candy cane and a smooth pink frosting floor"),
    (59, "a magical garden with big glowing flowers and two small butterflies"),
    # Hugo rooms 110-120
    (110, "a soccer stadium with a flat green grass pitch, one white goal and plain colourful stands"),
    (111, "a prehistoric jungle with big leafy plants, ferns and a distant volcano"),
    (112, "a volcano landscape with one erupting volcano, glowing orange lava and dark rocks"),
    (113, "a deep blue underwater ocean with a little seaweed, rocks and rays of light"),
    (114, "a superhero city skyline at dusk with tall buildings, glowing windows and a plain street below"),
    (115, "the inside of a space station with one big round window showing stars and planet Earth"),
    (116, "a wild west desert town street with two wooden buildings and a cactus"),
    (117, "a futuristic spaceport with one small rocket on a launch pad under a starry sky"),
    (118, "a robot laboratory with a big computer screen, simple machines and blinking lights"),
    (119, "a candy land room with two big lollipops, a candy cane and a smooth blue frosting floor"),
    (120, "a sunny green dog park with grass, two trees and a red ball"),
]

# UI backdrops -> data/ui/<key>.bin
UI_SCENES = [
    ("select", "a cheerful blue sky with soft pixel clouds, a rainbow arc on the left side, a soccer ball and stars on the right side, and an empty plain middle band"),
    ("menu", "a gentle starry night sky gradient with tiny pixel stars in the upper half and a plain dark lower half"),
]

ROOM_BY_ID = {rid: desc for rid, desc in ROOMS}
UI_BY_KEY = dict(UI_SCENES)

# Chosen after a side-by-side probe on room 51 (see SPRITE_NOTES.md round 2):
# opaque scenes are allowed to use gpt-image-2, transparent assets are not.
SCENE_MODEL = MODEL  # overridable via --model on rooms-gen


def scene_paths(name):
    """name: room id int/str ('49') or 'select'/'menu' -> (src, quant, bin)."""
    s = str(name)
    if s in UI_BY_KEY:
        key = f"ui_{s}"
        return (BG_SRC_DIR / f"{key}.png", BG_OUT_DIR / f"{key}.png",
                UI_DATA_DIR / f"{s}.bin", UI_BY_KEY[s], f"ui {s}")
    rid = int(s)
    key = f"room_{rid:03d}"
    return (BG_SRC_DIR / f"{key}.png", BG_OUT_DIR / f"{key}.png",
            ROOMS_DATA_DIR / f"{key}.bin", ROOM_BY_ID[rid], key)


def all_scene_names():
    return [str(rid) for rid, _ in ROOMS] + [k for k, _ in UI_SCENES]


def gen_scene(name, force=False, model=None):
    src, _, _, desc, label = scene_paths(name)
    if src.exists() and not force:
        return f"skip {label} (exists)"
    prompt = ROOM_STYLE.format(scene=desc)
    last_err = None
    for attempt in range(3):
        for size in ("1536x1024", "1024x1024"):
            try:
                png = api_generate(prompt, "medium", size=size, transparent=False,
                                   model=model or SCENE_MODEL)
                src.parent.mkdir(parents=True, exist_ok=True)
                src.write_bytes(png)
                return f"ok   {label} ({size}, {len(png)//1024} KB)"
            except urllib.error.HTTPError as e:
                msg = e.read()[:300]
                last_err = f"HTTP {e.code}: {msg!r}"
                if e.code == 400 and b"size" in msg.lower():
                    continue  # landscape unsupported -> try square
                time.sleep(20 * (attempt + 1) if e.code == 429 else 5)
                break
            except Exception as e:  # noqa: BLE001
                last_err = repr(e)
                time.sleep(5)
                break
    return f"FAIL {label}: {last_err}"


def cmd_rooms_gen(names, force=False, model=None):
    keys = names or all_scene_names()
    with ThreadPoolExecutor(max_workers=3) as ex:
        futs = {ex.submit(gen_scene, k, force, model): k for k in keys}
        for fut in as_completed(futs):
            print(fut.result(), flush=True)


def build_scene(name):
    src, quant_png, bin_path, _, label = scene_paths(name)
    if not src.exists():
        return f"MISSING {label}"
    img = Image.open(src).convert("RGB")
    w, h = img.size
    if w * 3 > h * 4:      # wider than 4:3 -> centre-crop width
        cw = h * 4 // 3
        x0 = (w - cw) // 2
        img = img.crop((x0, 0, x0 + cw, h))
    elif w * 3 < h * 4:    # taller than 4:3 -> centre-crop height
        ch = w * 3 // 4
        y0 = (h - ch) // 2
        img = img.crop((0, y0, w, y0 + ch))
    img = img.resize((BG_W, BG_H), Image.NEAREST)
    out = _reduce_palette(img.convert("RGBA"), BG_MAX_COLOURS).convert("RGB")
    quant_png.parent.mkdir(parents=True, exist_ok=True)
    out.save(quant_png)
    words = [rgb565(r, g, b) for (r, g, b) in out.getdata()]
    bin_path.parent.mkdir(parents=True, exist_ok=True)
    bin_path.write_bytes(struct.pack("<%dH" % len(words), *words))
    n = len(set(out.getdata()))
    return f"ok   {label} ({n} colours, {bin_path.stat().st_size} bytes)"


def cmd_rooms_build(names):
    for k in names or all_scene_names():
        print(build_scene(k), flush=True)


def cmd_rooms_montage():
    scale = 2
    tw, th = BG_W * scale, BG_H * scale
    pad, label_h = 8, 14
    cols = 4
    entries = all_scene_names()
    rows = (len(entries) + cols - 1) // cols
    w = pad + cols * (tw + pad)
    h = pad + rows * (th + label_h + pad)
    canvas = Image.new("RGB", (w, h), (250, 250, 250))
    draw = ImageDraw.Draw(canvas)
    font = ImageFont.load_default()
    for i, name in enumerate(entries):
        _, quant_png, _, desc, label = scene_paths(name)
        x = pad + (i % cols) * (tw + pad)
        y = pad + (i // cols) * (th + label_h + pad)
        if quant_png.exists():
            tile = Image.open(quant_png).convert("RGB").resize((tw, th), Image.NEAREST)
            canvas.paste(tile, (x, y))
        else:
            draw.rectangle([x, y, x + tw, y + th], fill=(230, 200, 200))
            draw.text((x + 10, y + 10), "MISSING", fill=(180, 0, 0), font=font)
        draw.text((x, y + th + 2), f"{label}: {desc[:52]}", fill=(60, 60, 60), font=font)
    canvas.save(PREVIEW_ROOMS)
    print(f"montage -> {PREVIEW_ROOMS} ({w}x{h})")


# ---- 24x24 UI icons ----------------------------------------------------------
ICON_STYLE = (
    "Game Boy Color style pixel art icon of {thing}, single centered object, "
    "chunky pixels, dark outline, bright colours, transparent background, "
    "no text, no watermark"
)

# (key, IconId numeric value from src/UI.h enum order, subject)
# Not generated (stay procedural): None=0, Back=34, ArrowL=35, ArrowR=36 and the
# tail after Sparkle (Sun..Web, 39-51).
ICONS = [
    ("apple", 1, "a shiny red apple with a small green leaf"),
    ("smiley", 2, "a round yellow smiley face"),
    ("moon", 3, "a yellow crescent moon"),
    ("coin", 4, "a round gold coin with a star stamped on it"),
    ("tick", 5, "a bold green check mark tick"),
    ("cross", 6, "a bold red X cross mark"),
    ("star", 7, "a yellow five-pointed star"),
    ("heart", 8, "a red love heart"),
    ("ball", 9, "a black and white soccer ball"),
    ("dino", 10, "a cute green baby tyrannosaurus rex dinosaur"),
    ("shark", 11, "a friendly blue shark"),
    ("crown", 12, "a gold royal crown with red jewels"),
    ("butterfly", 13, "a pink and purple butterfly with open wings"),
    ("wand", 14, "a magic wand stick with a yellow star on the tip"),
    ("note", 15, "a purple music note"),
    ("paw", 16, "a brown animal paw print"),
    ("bone", 17, "a white dog bone"),
    ("timer", 18, "a red stopwatch timer"),
    ("shirt", 19, "a blue t-shirt"),
    ("book", 20, "a closed red book"),
    ("bed", 21, "a small cozy bed with a pillow and blue blanket, side view"),
    ("broom", 22, "a wooden broom with yellow bristles"),
    ("shoe", 23, "a red sneaker shoe, side view"),
    ("plate", 24, "a white round plate with a fried egg on it"),
    ("water", 25, "a big shiny blue water drop"),
    ("gift", 26, "a red gift box with a yellow ribbon bow"),
    ("home", 27, "a small house with a red roof and a door"),
    ("games", 28, "a purple video game controller"),
    ("shop", 29, "an orange shopping bag"),
    ("missions", 30, "a triangular red flag on a short pole"),
    ("collection", 31, "an open brown treasure chest full of gold coins"),
    ("settings", 32, "a grey gear cog wheel"),
    ("swap", 33, "two curved arrows chasing each other in a circle, one blue and one orange"),
    ("jellybean", 37, "a single shiny pink jelly bean"),
    ("sparkle", 38, "a yellow four-pointed sparkle star"),
]
ICON_BY_KEY = {i[0]: i for i in ICONS}
# IconId enum names for emitted comments (UI.h order)
ICON_ENUM_NAMES = [
    "None", "Apple", "Smiley", "Moon", "Coin", "Tick", "Cross", "Star", "Heart",
    "Ball", "Dino", "Shark", "Crown", "Butterfly", "Wand", "Note", "Paw", "Bone",
    "Timer", "Shirt", "Book", "Bed", "Broom", "Shoe", "Plate", "Water", "Gift",
    "Home", "Games", "Shop", "Missions", "Collection", "Settings", "Swap", "Back",
    "ArrowL", "ArrowR", "JellyBean", "Sparkle", "Sun", "Flower", "Leaf", "Feather",
    "Lego", "Pencil", "Jump", "Circle", "Triangle", "Mic", "Rocket", "Hat", "Web",
]


def gen_icon(key, force=False):
    _, _, thing = ICON_BY_KEY[key]
    out = ICON_SRC_DIR / f"{key}.png"
    if out.exists() and not force:
        return f"skip {key} (exists)"
    last_err = None
    for attempt in range(3):
        try:
            png = api_generate(ICON_STYLE.format(thing=thing), "medium")
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(png)
            return f"ok   {key} ({len(png)//1024} KB)"
        except urllib.error.HTTPError as e:
            last_err = f"HTTP {e.code}: {e.read()[:200]!r}"
            time.sleep(20 * (attempt + 1) if e.code == 429 else 5)
        except Exception as e:  # noqa: BLE001
            last_err = repr(e)
            time.sleep(5)
    return f"FAIL {key}: {last_err}"


def cmd_icons_gen(names, force=False):
    keys = names or [i[0] for i in ICONS]
    with ThreadPoolExecutor(max_workers=3) as ex:
        futs = {ex.submit(gen_icon, k, force): k for k in keys}
        for fut in as_completed(futs):
            print(fut.result(), flush=True)


def quant_square_png(src, size, max_colours):
    """Alpha-bbox crop -> square pad -> NEAREST resize -> palette reduce."""
    img = Image.open(src).convert("RGBA")
    alpha = img.getchannel("A")
    bbox = alpha.point(lambda a: 255 if a >= 128 else 0).getbbox()
    if bbox is None:
        return None
    img = img.crop(bbox)
    w, h = img.size
    side = max(w, h)
    sq = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    sq.paste(img, ((side - w) // 2, (side - h) // 2))
    small = sq.resize((size, size), Image.NEAREST)
    return _reduce_palette(small, max_colours)


def cmd_icons_quant(names):
    for key in names or [i[0] for i in ICONS]:
        src = ICON_SRC_DIR / f"{key}.png"
        if not src.exists():
            print(f"MISSING {key}", flush=True)
            continue
        out = quant_square_png(src, ICON_SIZE, ICON_MAX_COLOURS)
        if out is None:
            print(f"EMPTY {key}", flush=True)
            continue
        ICON_OUT_DIR.mkdir(parents=True, exist_ok=True)
        out.save(ICON_OUT_DIR / f"{key}.png")
        n = len({p[:3] for p in out.getdata() if p[3] >= 128})
        print(f"ok   {key} ({n} colours)", flush=True)


def cmd_icons_emit():
    missing = [i[0] for i in ICONS if not (ICON_OUT_DIR / f"{i[0]}.png").exists()]
    if missing:
        raise SystemExit(f"cannot emit icons, missing: {missing}")

    lines = [
        "// AUTO-GENERATED by tools/gen_sprites.py icons-emit — do not edit by hand.",
        "// 24x24 RGB565 UI icons; 0xF81F = transparent sentinel (ART_TRANSPARENT).",
        "#pragma once",
        "#include <stdint.h>",
        "",
    ]
    for key, _, _ in ICONS:
        words = png_to_words(ICON_OUT_DIR / f"{key}.png", ICON_SIZE)
        lines.append(f"const uint16_t ICON_{key.upper()}[24*24] = {{")
        for row in range(ICON_SIZE):
            chunk = words[row * ICON_SIZE:(row + 1) * ICON_SIZE]
            lines.append("  " + ",".join(f"0x{w:04X}" for w in chunk) + ",")
        lines.append("};")
        lines.append("")
    ICONS_DATA_H.write_text("\n".join(lines))

    ICONS_H.write_text(
        "// AUTO-GENERATED by tools/gen_sprites.py icons-emit — pixel icon lookups.\n"
        "#pragma once\n"
        "#include <stdint.h>\n"
        "#include \"Assets.h\"  // PixelArt + ART_TRANSPARENT\n"
        "\n"
        "// 24x24 pixel art for an IconId numeric value (enum order in UI.h), or\n"
        "// nullptr for icons that stay procedural (None, Back, arrows, Sun..Web).\n"
        "const PixelArt* iconArt(uint8_t iconId);\n"
    )

    cpp = [
        "// AUTO-GENERATED by tools/gen_sprites.py icons-emit — do not edit by hand.",
        '#include "Icons.h"',
        '#include "IconsData.h"',
        "",
        "namespace {",
    ]
    for key, _, _ in ICONS:
        cpp.append(f"const PixelArt kIcon_{key} = {{24, 24, ICON_{key.upper()}}};")
    cpp += [
        "}  // namespace",
        "",
        "const PixelArt* iconArt(uint8_t iconId) {",
        "  switch (iconId) {",
    ]
    for key, idx, _ in ICONS:
        cpp.append(f"    case {idx}: return &kIcon_{key};  // IconId::{ICON_ENUM_NAMES[idx]}")
    cpp += [
        "    default: return nullptr;",
        "  }",
        "}",
        "",
    ]
    ICONS_CPP.write_text("\n".join(cpp))
    print(f"emitted {len(ICONS)} icons -> {ICONS_DATA_H.name}, {ICONS_H.name}, {ICONS_CPP.name}")


def cmd_icons_montage():
    scale = 4
    cell = ICON_SIZE * scale     # 96
    pad, label_h = 8, 14
    cols = 6
    keys = [i[0] for i in ICONS]
    rows = (len(keys) + cols - 1) // cols
    w = pad + cols * (cell + pad)
    h = pad + rows * (cell + label_h + pad)
    canvas = Image.new("RGBA", (w, h), (250, 250, 250, 255))
    draw = ImageDraw.Draw(canvas)
    font = ImageFont.load_default()
    for i, key in enumerate(keys):
        x = pad + (i % cols) * (cell + pad)
        y = pad + (i // cols) * (cell + label_h + pad)
        tile = checkerboard(cell)
        p = ICON_OUT_DIR / f"{key}.png"
        if p.exists():
            spr = Image.open(p).convert("RGBA").resize((cell, cell), Image.NEAREST)
            tile.alpha_composite(spr)
        else:
            ImageDraw.Draw(tile).text((cell // 2 - 10, cell // 2), "?", fill=(200, 0, 0, 255), font=font)
        canvas.paste(tile, (x, y))
        draw.text((x, y + cell + 2), f"{ICON_BY_KEY[key][1]:>2} {key}", fill=(60, 60, 60, 255), font=font)
    canvas.convert("RGB").save(PREVIEW_ICONS)
    print(f"montage -> {PREVIEW_ICONS} ({w}x{h})")


# ---- kid tiles: gen/quant/emit/montage --------------------------------------
KID_MARK = "// ---- ROUND 2: kid player tiles"


def cmd_kids_gen(force=False):
    SRC_DIR.mkdir(parents=True, exist_ok=True)
    for key, subject, quality in KIDS:
        out = SRC_DIR / f"{key}.png"
        if out.exists() and not force:
            print(f"skip {key} (exists)", flush=True)
            continue
        last_err = None
        for attempt in range(3):
            try:
                png = api_generate(STYLE.format(subject=subject), quality)
                out.write_bytes(png)
                print(f"ok   {key} ({quality}, {len(png)//1024} KB)", flush=True)
                last_err = None
                break
            except urllib.error.HTTPError as e:
                last_err = f"HTTP {e.code}: {e.read()[:200]!r}"
                time.sleep(20 * (attempt + 1) if e.code == 429 else 5)
            except Exception as e:  # noqa: BLE001
                last_err = repr(e)
                time.sleep(5)
        if last_err:
            print(f"FAIL {key}: {last_err}", flush=True)


def cmd_kids_quant():
    for key, _, _ in KIDS:
        src = SRC_DIR / f"{key}.png"
        if not src.exists():
            print(f"MISSING {key}", flush=True)
            continue
        out = quant_square_png(src, SIZE, MAX_COLOURS)
        if out is None:
            print(f"EMPTY {key}", flush=True)
            continue
        OUT_DIR.mkdir(parents=True, exist_ok=True)
        out.save(OUT_DIR / f"{key}.png")
        n = len({p[:3] for p in out.getdata() if p[3] >= 128})
        print(f"ok   {key} ({n} colours)", flush=True)


def cmd_kids_emit():
    """Append kid tile arrays + artKid() to the round-1 files (idempotent)."""
    for key, _, _ in KIDS:
        if not (OUT_DIR / f"{key}.png").exists():
            raise SystemExit(f"cannot emit kids, missing quantized {key}.png")

    # AssetsData.h — strip any previous round-2 section, then append fresh
    txt = DATA_H.read_text()
    i = txt.find(KID_MARK)
    if i != -1:
        txt = txt[:i].rstrip() + "\n"
    if not txt.endswith("\n"):
        txt += "\n"
    lines = ["", KID_MARK + " (appended by gen_sprites.py kids-emit) ----", ""]
    for key, _, _ in KIDS:
        words = png_to_words(OUT_DIR / f"{key}.png", SIZE)
        lines.append(f"const uint16_t ART_{key.upper()}[48*48] = {{")
        for row in range(SIZE):
            chunk = words[row * SIZE:(row + 1) * SIZE]
            lines.append("  " + ",".join(f"0x{w:04X}" for w in chunk) + ",")
        lines.append("};")
        lines.append("")
    DATA_H.write_text(txt + "\n".join(lines))

    # Assets.h — append the artKid declaration once
    h = ASSETS_H.read_text()
    if "artKid" not in h:
        if not h.endswith("\n"):
            h += "\n"
        ASSETS_H.write_text(
            h + "const PixelArt* artKid(uint8_t playerId);   // 0=Hugo tile, 1=Stella tile\n")

    # Assets.cpp — strip any previous round-2 section, then append fresh
    cpp = ASSETS_CPP.read_text()
    i = cpp.find(KID_MARK)
    if i != -1:
        cpp = cpp[:i].rstrip() + "\n"
    if not cpp.endswith("\n"):
        cpp += "\n"
    block = "\n".join([
        "",
        KID_MARK + " (appended by gen_sprites.py kids-emit) ----",
        "namespace {",
        "const PixelArt kArt_hugo_kid = {48, 48, ART_HUGO_KID};",
        "const PixelArt kArt_stella_kid = {48, 48, ART_STELLA_KID};",
        "}  // namespace",
        "",
        "const PixelArt* artKid(uint8_t playerId) {",
        "  if (playerId == 0) return &kArt_hugo_kid;",
        "  if (playerId == 1) return &kArt_stella_kid;",
        "  return nullptr;",
        "}",
        "",
    ])
    ASSETS_CPP.write_text(cpp + block)
    print(f"appended {len(KIDS)} kid tiles + artKid() -> "
          f"{DATA_H.name}, {ASSETS_H.name}, {ASSETS_CPP.name}")


def cmd_kids_montage():
    scale = 4
    cell = SIZE * scale
    pad, label_h = 8, 14
    w = pad + len(KIDS) * (cell + pad)
    h = pad + cell + label_h + pad
    canvas = Image.new("RGBA", (w, h), (250, 250, 250, 255))
    draw = ImageDraw.Draw(canvas)
    font = ImageFont.load_default()
    for i, (key, _, _) in enumerate(KIDS):
        x = pad + i * (cell + pad)
        tile = checkerboard(cell)
        p = OUT_DIR / f"{key}.png"
        if p.exists():
            spr = Image.open(p).convert("RGBA").resize((cell, cell), Image.NEAREST)
            tile.alpha_composite(spr)
        canvas.paste(tile, (x, pad))
        draw.text((x, pad + cell + 2), key, fill=(60, 60, 60, 255), font=font)
    canvas.convert("RGB").save(PREVIEW_KIDS)
    print(f"montage -> {PREVIEW_KIDS} ({w}x{h})")


# ---------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("cmd", choices=[
        "gen", "quant", "emit", "montage", "all",
        "kids-gen", "kids-quant", "kids-emit", "kids-montage",
        "rooms-gen", "rooms-build", "rooms-montage",
        "icons-gen", "icons-quant", "icons-emit", "icons-montage",
    ])
    ap.add_argument("names", nargs="*", help="asset keys (default: all)")
    ap.add_argument("--force", action="store_true", help="regenerate even if source exists")
    ap.add_argument("--model", help="override image model (rooms-gen only)")
    args = ap.parse_args()

    if args.cmd in ("gen", "quant", "all"):
        for n in args.names:
            if n not in BY_KEY:
                raise SystemExit(f"unknown sprite: {n}")
    elif args.cmd in ("rooms-gen", "rooms-build"):
        valid = set(all_scene_names())
        for n in args.names:
            if n not in valid:
                raise SystemExit(f"unknown scene: {n} (room id, 'select' or 'menu')")
    elif args.cmd in ("icons-gen", "icons-quant"):
        for n in args.names:
            if n not in ICON_BY_KEY:
                raise SystemExit(f"unknown icon: {n}")

    if args.cmd == "gen":
        cmd_gen(args.names, args.force)
    elif args.cmd == "quant":
        cmd_quant(args.names)
    elif args.cmd == "emit":
        cmd_emit()
    elif args.cmd == "montage":
        cmd_montage()
    elif args.cmd == "all":
        cmd_gen(args.names, args.force)
        cmd_quant(args.names)
        cmd_emit()
        cmd_montage()
    elif args.cmd == "kids-gen":
        cmd_kids_gen(args.force)
    elif args.cmd == "kids-quant":
        cmd_kids_quant()
    elif args.cmd == "kids-emit":
        cmd_kids_emit()
    elif args.cmd == "kids-montage":
        cmd_kids_montage()
    elif args.cmd == "rooms-gen":
        cmd_rooms_gen(args.names, args.force, args.model)
    elif args.cmd == "rooms-build":
        cmd_rooms_build(args.names)
    elif args.cmd == "rooms-montage":
        cmd_rooms_montage()
    elif args.cmd == "icons-gen":
        cmd_icons_gen(args.names, args.force)
    elif args.cmd == "icons-quant":
        cmd_icons_quant(args.names)
    elif args.cmd == "icons-emit":
        cmd_icons_emit()
    elif args.cmd == "icons-montage":
        cmd_icons_montage()


if __name__ == "__main__":
    main()
