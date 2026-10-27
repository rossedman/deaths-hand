// The three character macros on the front panel. Each one sweeps several
// engine parameters along its own curve, so one knob (or one CV) tells one story:
//
//   FLESH   brightness: FM sidebands, then the fold, then asymmetry and a moving fold
//   MALICE  instability: self-feedback (sine -> saw), then chaos with its own FM, then collapse
//   LEGION  size: a sub, then detuned voices, then drift
//
// Every curve is piecewise linear through a few breakpoints. The host tests
// check that each macro moves its own measure steadily, with no jumps.
#pragma once

#include "ComplexOsc.hpp"

namespace cosc {

struct Breakpoint {
    float x, y;
};

// Piecewise-linear lookup; flat before the first and after the last point.
template <int N>
inline float curve(const Breakpoint (&pts)[N], float x) {
    if (x <= pts[0].x) return pts[0].y;
    for (int i = 1; i < N; ++i)
        if (x <= pts[i].x) return lerpf(pts[i - 1].y, pts[i].y, (x - pts[i - 1].x) / (pts[i].x - pts[i - 1].x));
    return pts[N - 1].y;
}

namespace macro {
// FLESH
constexpr Breakpoint kFmIndex[]   = {{0.f, 0.f}, {0.35f, 0.35f}, {1.f, 0.6f}};
constexpr Breakpoint kTimbre[]    = {{0.25f, 0.f}, {0.75f, 0.7f}, {1.f, 0.85f}};
constexpr Breakpoint kSymmetry[]  = {{0.6f, 0.f}, {1.f, 0.6f}};
constexpr Breakpoint kTimbreMod[] = {{0.6f, 0.f}, {1.f, 0.5f}};
// MALICE
constexpr Breakpoint kFeedback[] = {{0.f, 0.f}, {0.5f, 0.6f}, {0.85f, 0.7f}, {1.f, 1.f}};
constexpr Breakpoint kChaos[]    = {{0.45f, 0.f}, {0.75f, 0.1f}, {0.9f, 0.4f}, {1.f, 1.f}};
// Chaos only reaches the primary through FM, so MALICE brings its own FM in
// alongside it (added to FLESH's): on its own, the knob still ends in collapse.
constexpr Breakpoint kMaliceFm[] = {{0.45f, 0.f}, {0.75f, 0.06f}, {0.9f, 0.2f}, {1.f, 0.35f}};
// LEGION
constexpr Breakpoint kSub[]    = {{0.f, 0.f}, {0.25f, 0.6f}, {1.f, 0.7f}};
constexpr Breakpoint kVoices[] = {{0.2f, 1.f}, {0.6f, 2.5f}, {0.85f, 4.f}};
constexpr Breakpoint kSpread[] = {{0.2f, 0.f}, {0.6f, 0.3f}, {0.85f, 0.6f}, {1.f, 0.75f}};
constexpr Breakpoint kDrift[]  = {{0.75f, 0.f}, {1.f, 0.8f}};
} // namespace macro

struct MacroValues {
    float flesh  = 0.f; // 0..1, knob + CV
    float malice = 0.f;
    float legion = 0.f;
};

// Writes every parameter the macros own. FM index is FLESH's plus MALICE's
// (the engine clamps it to 1). Symmetry and timbre mod are added to
// what's already in p (the symmetry trim, the routing preset); the rest are set.
inline void applyMacros(const MacroValues& m, Params& p) {
    const float flesh = clampf(m.flesh, 0.f, 1.f), malice = clampf(m.malice, 0.f, 1.f), legion = clampf(m.legion, 0.f, 1.f);
    p.fmIndex = curve(macro::kFmIndex, flesh) + curve(macro::kMaliceFm, malice);
    p.timbre  = curve(macro::kTimbre, flesh);
    p.symmetry += curve(macro::kSymmetry, flesh);
    p.timbreMod += curve(macro::kTimbreMod, flesh);
    p.feedback = curve(macro::kFeedback, malice);
    p.chaos    = curve(macro::kChaos, malice);
    p.sub      = curve(macro::kSub, legion);
    p.voices   = curve(macro::kVoices, legion);
    p.spread   = curve(macro::kSpread, legion);
    p.drift    = curve(macro::kDrift, legion);
}

} // namespace cosc
