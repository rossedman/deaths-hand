// patch.Init panel -> engine parameters. The firmware calls this every block,
// and the host tests render the documented patches through it, so a patch
// written as knob positions sounds the same in a test as on the module.
#pragma once

#include "ComplexOsc.hpp"
#include "Macros.hpp"

namespace cosc {

// Routing presets cycled by a button tap / gate 2.
struct Routing {
    float amDepth;
    float timbreMod;
};
constexpr Routing kRoutings[] = {
    {0.f, 0.f},  // 0: FM only
    {0.f, 0.6f}, // 1: FM + modulator sweeps the folder
    {0.5f, 0.f}, // 2: FM + classic AM
    {1.f, 0.3f}, // 3: FM + ring mod + a little timbre mod
};
constexpr int kNumRoutings = sizeof(kRoutings) / sizeof(kRoutings[0]);

// Knobs are 0..1 (7 o'clock .. 5 o'clock). CVs are volts at the jacks.
struct PanelState {
    float pitch = 0.5f, flesh = 0.f, malice = 0.f, legion = 0.f; // front knobs
    float ratio = 2.5f / static_cast<float>(kNumRatios);         // shift 1: the 1:1 step
    float shape = 0.f, symmetry = 0.5f, fine = 0.5f;             // shift 2-4
    bool  ratioLock = true;                                      // toggle up
    int   routing   = 0;
    float pitchCv = 0.f, fleshCv = 0.f, maliceCv = 0.f, legionCv = 0.f, symmetryCv = 0.f;
};

inline Params panelToParams(const PanelState& s) {
    Params p;
    // Pitch spans 6 octaves around C4; fine is +-1 semitone, noon = 0.
    p.pitchV    = (s.pitch * 6.f - 3.f) + (s.fine * 2.f - 1.f) / 12.f + s.pitchCv;
    p.ratioLock = s.ratioLock;
    if (s.ratioLock) p.ratio = quantizeRatio(s.ratio);
    else p.modPitchV = s.ratio * 11.f - 8.f; // -8 V .. +3 V: about 1 Hz (LFO) up to C7
    p.modShape = static_cast<ModShape>(static_cast<int>(clampf(s.shape, 0.f, 0.999f) * static_cast<float>(ModShape::Count)));

    // Symmetry trim (centred = none) plus its CV at +-1 per +-5 V. The routing
    // preset sets AM and a base timbre mod; FLESH adds to both.
    const Routing& r = kRoutings[((s.routing % kNumRoutings) + kNumRoutings) % kNumRoutings];
    p.symmetry       = (s.symmetry * 2.f - 1.f) + s.symmetryCv * 0.2f;
    p.amDepth        = r.amDepth;
    p.timbreMod      = r.timbreMod;

    // The character macros: knob plus CV, where +-5 V sweeps the whole range.
    MacroValues m;
    m.flesh  = s.flesh + s.fleshCv * 0.2f;
    m.malice = s.malice + s.maliceCv * 0.2f;
    m.legion = s.legion + s.legionCv * 0.2f;
    applyMacros(m, p);
    return p;
}

} // namespace cosc
