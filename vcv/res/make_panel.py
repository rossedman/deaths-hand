#!/usr/bin/env python3
"""Generates the DEATH'S HAND panel SVG and a matching C++ layout header from one table.

    python3 res/make_panel.py          # writes res/ComplexOsc.svg + src/PanelLayout.hpp

Every control is marked with a sigil instead of a word; the README has the
decoder. Rack's SVG renderer ignores <text>, so the only lettering (the title)
is converted to outlines with matplotlib's TextPath. Dimensions are real
Eurorack (3U = 128.5 mm, 1 HP = 5.08 mm), so this is also a starting point for
a physical front panel.
"""

import math
import random
from pathlib import Path

from matplotlib.font_manager import FontProperties
from matplotlib.path import Path as MPath
from matplotlib.textpath import TextPath

HP = 16
W, H = HP * 5.08, 128.5
COLS = [11.5, 30.8, 50.4, 69.8]

BG = "#0b090a"
BONE = "#d9cdb8"
BONE_DIM = "#6b6156"
BLOOD = "#7a0b10"
BLOOD_HI = "#c4141c"
BLOOD_DARK = "#2a0608"
CIRCLE = "#2b0b0e"  # the faint summoning circle behind the controls

# (kind, enum name, x, y, sigil)
#   kind: knob_l, knob_m, trim, snap, switch, in, out
CONTROLS = [
    ("knob_l", "PITCH_PARAM", COLS[0], 26, "eye"),
    ("knob_m", "FINE_PARAM", COLS[1], 26, "slit_eye"),
    ("knob_l", "MOD_PITCH_PARAM", COLS[2], 26, "vesica"),
    ("knob_l", "FM_INDEX_PARAM", COLS[3], 26, "trident"),
    ("knob_m", "TIMBRE_PARAM", COLS[0], 45, "fold"),
    ("knob_m", "SYMMETRY_PARAM", COLS[1], 45, "hourglass"),
    ("knob_m", "AM_PARAM", COLS[2], 45, "crossed_ring"),
    ("knob_m", "TIMBRE_MOD_PARAM", COLS[3], 45, "serpent"),
    ("knob_m", "FEEDBACK_PARAM", COLS[0], 61, "ouroboros"),
    ("knob_m", "CHAOS_PARAM", COLS[1], 61, "chaos_star"),
    ("snap", "MOD_SHAPE_PARAM", COLS[2], 61, "triad"),
    ("switch", "RATIO_LOCK_PARAM", COLS[3], 61, "key"),
    ("trim", "FM_CV_ATTEN_PARAM", COLS[0], 74, "trident"),
    ("trim", "TIMBRE_CV_ATTEN_PARAM", COLS[1], 74, "fold"),
    ("trim", "FEEDBACK_CV_ATTEN_PARAM", COLS[2], 74, "ouroboros"),
    ("trim", "CHAOS_CV_ATTEN_PARAM", COLS[3], 74, "chaos_star"),
    ("in", "VOCT_INPUT", COLS[0], 89, "eye"),
    ("in", "MOD_VOCT_INPUT", COLS[1], 89, "vesica"),
    ("in", "FM_INDEX_INPUT", COLS[2], 89, "trident"),
    ("in", "TIMBRE_INPUT", COLS[3], 89, "fold"),
    ("in", "SYMMETRY_INPUT", COLS[0], 103, "hourglass"),
    ("in", "AM_INPUT", COLS[1], 103, "crossed_ring"),
    ("in", "EXT_FM_INPUT", COLS[2], 103, "spiral"),
    ("in", "SYNC_INPUT", COLS[3], 103, "pillars"),
    ("in", "FEEDBACK_INPUT", COLS[0], 117, "ouroboros"),
    ("in", "CHAOS_INPUT", COLS[1], 117, "chaos_star"),
    ("out", "MOD_OUTPUT", COLS[2], 117, "vesica_drop"),
    ("out", "PRIMARY_OUTPUT", COLS[3], 117, "drop"),
]

# Where each control's sigil sits (offset above the control) and how big it is.
SIGIL_DY = {"knob_l": -9.4, "knob_m": -7.6, "trim": -5.4, "snap": -7.8, "switch": -6.8, "in": -7.0, "out": -7.0}
SIGIL_R = {"knob_l": 1.9, "knob_m": 1.7, "trim": 1.2, "snap": 1.7, "switch": 1.6, "in": 1.5, "out": 1.6}
CONTROL_R = {"knob_l": 6.3, "knob_m": 4.8, "trim": 3.0, "snap": 4.8, "switch": 3.8, "in": 4.0, "out": 4.0}

SW = 0.3  # sigil stroke width (mm)


# ----------------------------------------------------------------- primitives
def f(v):
    return f"{v:.3f}"


def stroke(d, color=BONE, w=SW, fill="none", cap="round"):
    return f'<path d="{d}" fill="{fill}" stroke="{color}" stroke-width="{w}" stroke-linecap="{cap}" stroke-linejoin="round"/>'


def circle(cx, cy, r, color=BONE, w=SW, fill="none"):
    return f'<circle cx="{f(cx)}" cy="{f(cy)}" r="{f(r)}" fill="{fill}" stroke="{color}" stroke-width="{w}"/>'


def dot(cx, cy, r, color=BONE):
    return f'<circle cx="{f(cx)}" cy="{f(cy)}" r="{f(r)}" fill="{color}"/>'


def poly(points, closed=False, **kw):
    d = "M" + " L".join(f"{f(x)},{f(y)}" for x, y in points) + (" Z" if closed else "")
    return stroke(d, **kw)


def polar(cx, cy, r, deg):
    a = math.radians(deg)
    return cx + r * math.sin(a), cy - r * math.cos(a)  # 0 deg = up, clockwise


# ----------------------------------------------------------------- sigils
# Each sigil is drawn inside a circle of radius r centred on (cx, cy).
def s_eye(cx, cy, r, color=BONE):
    d = f"M{f(cx - r)},{f(cy)} Q{f(cx)},{f(cy - r * 1.05)} {f(cx + r)},{f(cy)} Q{f(cx)},{f(cy + r * 1.05)} {f(cx - r)},{f(cy)} Z"
    return [stroke(d, color), dot(cx, cy, r * 0.32, color)] + [
        poly([polar(cx, cy - r * 0.55, r * 0.35, a), polar(cx, cy - r * 0.55, r * 0.75, a)], color=color, w=SW * 0.8) for a in (-35, 0, 35)
    ]


def s_slit_eye(cx, cy, r, color=BONE):
    d = f"M{f(cx - r)},{f(cy)} Q{f(cx)},{f(cy - r * 1.05)} {f(cx + r)},{f(cy)} Q{f(cx)},{f(cy + r * 1.05)} {f(cx - r)},{f(cy)} Z"
    slit = f"M{f(cx)},{f(cy - r * 0.42)} Q{f(cx + r * 0.16)},{f(cy)} {f(cx)},{f(cy + r * 0.42)} Q{f(cx - r * 0.16)},{f(cy)} {f(cx)},{f(cy - r * 0.42)} Z"
    return [stroke(d, color), stroke(slit, color, w=0.01, fill=color)]


def s_vesica(cx, cy, r, color=BONE):
    return [circle(cx - r * 0.32, cy, r * 0.62, color), circle(cx + r * 0.32, cy, r * 0.62, color), dot(cx, cy, r * 0.12, color)]


def s_trident(cx, cy, r, color=BONE):
    out = [poly([(cx, cy + r), (cx, cy - r)], color=color)]
    out.append(
        stroke(
            f"M{f(cx - r * 0.7)},{f(cy - r * 0.9)} Q{f(cx - r * 0.7)},{f(cy - r * 0.15)} {f(cx)},{f(cy - r * 0.15)} "
            f"Q{f(cx + r * 0.7)},{f(cy - r * 0.15)} {f(cx + r * 0.7)},{f(cy - r * 0.9)}",
            color,
        )
    )
    for x in (cx - r * 0.7, cx, cx + r * 0.7):  # barbed tips
        out.append(poly([(x - r * 0.18, cy - r * 0.72), (x, cy - r * 1.0), (x + r * 0.18, cy - r * 0.72)], color=color, w=SW * 0.9))
    out.append(poly([(cx - r * 0.3, cy + r * 0.55), (cx + r * 0.3, cy + r * 0.55)], color=color, w=SW * 0.9))
    return out


def s_fold(cx, cy, r, color=BONE):
    zig = [(cx - r * 0.75, cy + r * 0.1)]
    for i, x in enumerate((-0.45, -0.15, 0.15, 0.45, 0.75)):
        zig.append((cx + r * x, cy + (-r * 0.5 if i % 2 == 0 else r * 0.5)))
    return [circle(cx, cy, r, color), poly(zig, color=color)]


def s_hourglass(cx, cy, r, color=BONE):
    t, b, wd = cy - r * 0.9, cy + r * 0.9, r * 0.7
    return [
        poly([(cx - wd, t), (cx + wd, t), (cx - wd, b), (cx + wd, b), (cx - wd, t)], color=color),
        poly([(cx - r, t), (cx + r, t)], color=color),
        poly([(cx - r, b), (cx + r, b)], color=color),
        dot(cx, cy + r * 0.55, r * 0.14, color),
    ]


def s_crossed_ring(cx, cy, r, color=BONE):
    k = r * 0.62
    return [circle(cx, cy, r * 0.88, color), poly([(cx - k, cy - k), (cx + k, cy + k)], color=color), poly([(cx + k, cy - k), (cx - k, cy + k)], color=color)]


def s_serpent(cx, cy, r, color=BONE):
    d = (
        f"M{f(cx - r * 0.9)},{f(cy + r * 0.75)} C{f(cx - r * 0.2)},{f(cy + r * 1.1)} {f(cx + r * 0.9)},{f(cy + r * 0.4)} {f(cx)},{f(cy)} "
        f"S{f(cx - r * 0.6)},{f(cy - r * 0.9)} {f(cx + r * 0.65)},{f(cy - r * 0.75)}"
    )
    return [
        stroke(d, color),
        dot(cx + r * 0.72, cy - r * 0.78, r * 0.17, color),
        poly([(cx + r * 0.86, cy - r * 0.8), (cx + r * 1.1, cy - r * 0.95)], color=color, w=SW * 0.7),
    ]


def s_ouroboros(cx, cy, r, color=BONE):
    rr = r * 0.85
    a0, a1 = 40, 340
    x0, y0 = polar(cx, cy, rr, a0)
    x1, y1 = polar(cx, cy, rr, a1)
    arc = f"M{f(x0)},{f(y0)} A{f(rr)},{f(rr)} 0 1 1 {f(x1)},{f(y1)}"
    # A head biting the tail at the gap
    hx, hy = polar(cx, cy, rr, a1)
    tip = polar(cx, cy, rr, 20)
    head = poly([polar(cx, cy, rr * 1.28, a1 + 6), tip, polar(cx, cy, rr * 0.72, a1 + 6)], color=color, closed=True)
    return [stroke(arc, color), head, dot(hx, hy, r * 0.1, BG)]


def s_chaos_star(cx, cy, r, color=BONE):
    out = [dot(cx, cy, r * 0.2, color)]
    for i in range(8):
        a = i * 45
        x0, y0 = polar(cx, cy, r * 0.3, a)
        x1, y1 = polar(cx, cy, r, a)
        out.append(poly([(x0, y0), (x1, y1)], color=color))
        lft, rgt = polar(cx, cy, r * 0.72, a - 14), polar(cx, cy, r * 0.72, a + 14)
        out.append(poly([lft, (x1, y1), rgt], color=color, w=SW * 0.85))
    return out


def s_triad(cx, cy, r, color=BONE):
    pts = [polar(cx, cy, r * 0.95, a) for a in (0, 120, 240)]
    return [circle(cx, cy, r, color), poly(pts, closed=True, color=color), dot(cx, cy, r * 0.13, color)]


def s_key(cx, cy, r, color=BONE):
    return [
        circle(cx, cy - r * 0.55, r * 0.38, color),
        poly([(cx, cy - r * 0.17), (cx, cy + r)], color=color),
        poly([(cx, cy + r * 0.55), (cx + r * 0.42, cy + r * 0.55)], color=color),
        poly([(cx, cy + r * 0.88), (cx + r * 0.3, cy + r * 0.88)], color=color),
        poly([(cx - r * 0.16, cy - r * 1.2), (cx, cy - r * 1.42), (cx + r * 0.16, cy - r * 1.2)], color=color, w=SW * 0.8),
    ]


def s_spiral(cx, cy, r, color=BONE):
    pts = []
    for i in range(70):
        t = i / 69
        a = t * 3.1 * 360
        pts.append(polar(cx, cy, r * (0.1 + 0.9 * t), a))
    return [poly(pts, color=color)]


def s_pillars(cx, cy, r, color=BONE):
    return [
        poly([(cx - r * 0.75, cy - r), (cx - r * 0.75, cy + r)], color=color),
        poly([(cx + r * 0.75, cy - r), (cx + r * 0.75, cy + r)], color=color),
        poly([(cx + r * 0.3, cy - r * 0.85), (cx - r * 0.25, cy + r * 0.05), (cx + r * 0.25, cy - r * 0.05), (cx - r * 0.3, cy + r * 0.85)], color=color),
    ]


def drop_path(cx, cy, r):
    return (
        f"M{f(cx)},{f(cy - r * 1.1)} C{f(cx + r * 0.25)},{f(cy - r * 0.5)} {f(cx + r * 0.85)},{f(cy)} {f(cx + r * 0.7)},{f(cy + r * 0.45)} "
        f"A{f(r * 0.72)},{f(r * 0.72)} 0 1 1 {f(cx - r * 0.7)},{f(cy + r * 0.45)} "
        f"C{f(cx - r * 0.85)},{f(cy)} {f(cx - r * 0.25)},{f(cy - r * 0.5)} {f(cx)},{f(cy - r * 1.1)} Z"
    )


def s_drop(cx, cy, r, color=BLOOD_HI):
    return [stroke(drop_path(cx, cy, r), color=color, w=0.2, fill=color)]


def s_vesica_drop(cx, cy, r, color=BLOOD_HI):
    return [
        stroke(drop_path(cx, cy, r), color=color, w=SW),
        circle(cx - r * 0.2, cy + r * 0.3, r * 0.32, color, w=SW * 0.8),
        circle(cx + r * 0.2, cy + r * 0.3, r * 0.32, color, w=SW * 0.8),
    ]


SIGILS = {name[2:]: fn for name, fn in globals().items() if name.startswith("s_")}


# ----------------------------------------------------------------- decoration
FONT = FontProperties(family="DejaVu Serif", weight="bold")


def text_path(s, x, y, size, color, tracking=0.0):
    """Text as filled outlines, centred on x, baseline y, with letter spacing (mm)."""
    glyphs, cursor = [], 0.0
    bar = TextPath((0, 0), "|", size=size, prop=FONT).get_extents().width
    for ch in s:
        if ch == " ":
            cursor += size * 0.35 + tracking
            continue
        glyphs.append((TextPath((0, 0), ch, size=size, prop=FONT), cursor))
        adv = TextPath((0, 0), ch + "|", size=size, prop=FONT).get_extents().width - bar
        cursor += adv + tracking
    total = cursor - tracking
    out = []
    for tp, off in glyphs:
        d, i, v, c = [], 0, tp.vertices, tp.codes
        if c is None or len(c) == 0:
            continue

        def pt(k, off=off, v=v):
            return f"{f(x - total / 2 + off + v[k][0])},{f(y - v[k][1])}"

        while i < len(c):
            if c[i] == MPath.MOVETO:
                d.append("M" + pt(i))
                i += 1
            elif c[i] == MPath.LINETO:
                d.append("L" + pt(i))
                i += 1
            elif c[i] == MPath.CURVE3:
                d.append("Q" + pt(i) + " " + pt(i + 1))
                i += 2
            elif c[i] == MPath.CURVE4:
                d.append("C" + pt(i) + " " + pt(i + 1) + " " + pt(i + 2))
                i += 3
            elif c[i] == MPath.CLOSEPOLY:
                d.append("Z")
                i += 1
            else:
                i += 1
        out.append(f'<path d="{"".join(d)}" fill="{color}"/>')
    return out, total


def rune(cx, cy, rng, color=BLOOD, w=0.22):
    """An invented rune: 2-4 strokes between points of a 3x4 grid."""
    gx = [cx - 0.7, cx, cx + 0.7]
    gy = [cy - 1.1, cy - 0.37, cy + 0.37, cy + 1.1]
    out = [poly([(cx, cy - 1.1), (cx, cy + 1.1)], color=color, w=w)]  # stave
    for _ in range(rng.randint(1, 3)):
        a = (rng.choice(gx), rng.choice(gy))
        b = (rng.choice(gx), rng.choice(gy))
        if a != b:
            out.append(poly([a, b], color=color, w=w))
    return out


def summoning_circle(cx, cy, r):
    out = [circle(cx, cy, r, CIRCLE, w=0.45), circle(cx, cy, r - 1.4, CIRCLE, w=0.25)]
    pts = [polar(cx, cy, r - 1.4, i * 360 / 7) for i in range(7)]
    star = [pts[(i * 3) % 7] for i in range(8)]  # {7/3} heptagram
    out.append(poly(star, color=CIRCLE, w=0.35))
    rng = random.Random(13)
    for i in range(28):  # runes in the band between the two rings
        a = i * 360 / 28 + 6
        x, y = polar(cx, cy, r - 0.7, a)
        out.append(
            f'<g transform="rotate({f(a)} {f(x)} {f(y)}) translate({f(x)} {f(y)}) scale(0.32) translate({f(-x)} {f(-y)})">'
            + "".join(rune(x, y, rng, color=CIRCLE, w=0.6))
            + "</g>"
        )
    return out


def tick_ring(cx, cy, r, n, color=BLOOD, long_every=None, hi_last=False):
    out = []
    for i in range(n):
        a = -150 + 300 * i / (n - 1)
        long = long_every and i % long_every == 0
        r0, r1 = r + 0.9, r + (2.0 if long else 1.5)
        c = BLOOD_HI if (hi_last and i == n - 1) else color
        out.append(poly([polar(cx, cy, r0, a), polar(cx, cy, r1, a)], color=c, w=0.28 if long else 0.2))
    return out


def drips(x0, x1, y, rng, keep_clear=()):
    out = [poly([(x0, y), (x1, y)], color=BLOOD, w=0.5)]
    x = x0 + 1.5
    while x < x1 - 1.5:
        if any(abs(x - k) < 3.0 for k in keep_clear):  # don't bleed over the sigils
            x += 0.8
            continue
        ln = rng.uniform(1.0, 4.2)
        wd = rng.uniform(0.35, 0.6)
        out.append(
            stroke(
                f"M{f(x - wd)},{f(y)} L{f(x - wd * 0.4)},{f(y + ln)} A{f(wd * 0.75)},{f(wd * 0.75)} 0 1 0 {f(x + wd * 0.4)},{f(y + ln)} L{f(x + wd)},{f(y)} Z",
                color=BLOOD,
                w=0.05,
                fill=BLOOD,
            )
        )
        x += rng.uniform(2.2, 4.5)
    return out


# ----------------------------------------------------------------- panel
def svg():
    rng = random.Random(666)
    out = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{f(W)}mm" height="{H}mm" viewBox="0 0 {f(W)} {H}">',
        f'<rect width="{f(W)}" height="{H}" fill="{BG}"/>',
        f'<rect x="1.4" y="1.4" width="{f(W - 2.8)}" height="{H - 2.8}" fill="none" stroke="{BLOOD}" stroke-width="0.35"/>',
        f'<rect x="2.1" y="2.1" width="{f(W - 4.2)}" height="{H - 4.2}" fill="none" stroke="{BLOOD_DARK}" stroke-width="0.2"/>',
    ]
    out += summoning_circle(W / 2, 47, 31.5)

    # Edge runes, both sides, between the screws.
    y = 15.0
    while y < 113.0:
        out += rune(3.75, y, rng)
        out += rune(W - 3.75, y, rng)
        y += 3.55

    # Title with flanking marks.
    title, tw = text_path("DEATH'S HAND", W / 2, 10.6, size=3.5, color=BONE, tracking=0.55)
    out += title
    for side in (-1, 1):
        x = W / 2 + side * (tw / 2 + 3.2)
        out += s_chaos_star(x, 9.4, 1.15, color=BLOOD_HI)

    # Divider between controls and jacks: a chain of diamonds.
    for i in range(23):
        x = 6.2 + i * (W - 12.4) / 22
        out.append(poly([(x, 79.3), (x + 0.5, 80), (x, 80.7), (x - 0.5, 80), (x, 79.3)], color=BLOOD, w=0.2))

    # Output altar.
    ax0, ax1 = COLS[2] - 7.5, COLS[3] + 7.5
    out.append(f'<rect x="{f(ax0)}" y="108.6" width="{f(ax1 - ax0)}" height="16.4" rx="1.0" fill="{BLOOD_DARK}" stroke="{BLOOD}" stroke-width="0.35"/>')
    out += drips(ax0 + 0.6, ax1 - 0.6, 108.6, random.Random(7), keep_clear=(COLS[2], COLS[3]))

    for kind, _, x, y0, sigil in CONTROLS:
        cr = CONTROL_R[kind]
        if kind in ("knob_l", "knob_m"):
            out += tick_ring(x, y0, cr, 11, long_every=5)
        elif kind == "snap":
            out += tick_ring(x, y0, cr, 5, color=BONE_DIM, hi_last=True)  # the last stop is Lorenz
        elif kind == "trim":
            out.append(circle(x, y0, cr + 0.8, BLOOD, w=0.2))
        elif kind == "in":
            out.append(circle(x, y0, cr + 1.0, BONE_DIM, w=0.25))
        elif kind == "out":
            out.append(circle(x, y0, cr + 1.1, BLOOD_HI, w=0.45, fill="#4a0a0d"))
        elif kind == "switch":
            # Up = ratio lock: a small closed triangle above, open below.
            out.append(poly([(x - 0.7, y0 - 4.6), (x, y0 - 5.4), (x + 0.7, y0 - 4.6)], closed=True, color=BLOOD_HI, w=0.2, fill=BLOOD_HI))
            out.append(poly([(x - 0.7, y0 + 4.6), (x, y0 + 5.4), (x + 0.7, y0 + 4.6)], closed=True, color=BLOOD, w=0.2))
        color = BLOOD_HI if kind == "out" else BONE
        out += SIGILS[sigil](x, y0 + SIGIL_DY[kind], SIGIL_R[kind], color)

    out.append("</svg>")
    return "\n".join(out)


def header():
    lines = [
        "// Generated by res/make_panel.py — edit the table there, not here.",
        "#pragma once",
        "",
        "namespace panel {",
        "struct Pos { float x, y; }; // millimetres from the top-left",
    ]
    for _kind, name, x, y, _ in CONTROLS:
        lines.append(f"constexpr Pos {name} = {{{x:.2f}f, {y:.2f}f}};")
    lines.append("} // namespace panel")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    root = Path(__file__).resolve().parent.parent
    (root / "res" / "ComplexOsc.svg").write_text(svg())
    (root / "src" / "PanelLayout.hpp").write_text(header())
    print(f"wrote {root / 'res/ComplexOsc.svg'} and {root / 'src/PanelLayout.hpp'} ({HP} HP)")
