// Doombringer's documented patches and showcase clips, written as patch.Init
// knob positions (0..1 = 7 o'clock .. 5 o'clock), CVs and what goes into the
// input, and rendered through the firmware's own panel mapping. Shared by the
// host tests (test/out/doom_*.wav) and the docs renders (render.cpp).
#pragma once

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include "../dsp/ComplexOsc.hpp"
#include "../dsp/DoomPanel.hpp"
#include "../dsp/Panel.hpp"

namespace doom {

// A band-limited saw (PolyBLEP), the classic thing to filter. +-5 V.
struct Saw {
    float phase = 0.f;
    float tick(float hz, float fs) {
        const float dt = hz / fs;
        phase += dt;
        phase -= std::floor(phase);
        return 5.f * ((2.f * phase - 1.f) - cosc::polyBlep(phase, dt));
    }
};

// One sample of a clip: t in seconds. Fill the panel, set ping on a trigger,
// and return the volts at the audio input.
using Step = std::function<float(float t, PanelState& s, bool& ping)>;

struct DoomClip {
    const char*           file; // doom_<file>.wav
    const char*           name;
    float                 seconds;
    std::function<Step()> make; // a fresh step with its own oscillators and state
};

// True once each time t crosses a multiple of `every` seconds (a clock).
struct Clock {
    int  last = -1;
    bool tick(float t, float every) {
        const int n = static_cast<int>(t / every);
        if (n == last) return false;
        last = n;
        return true;
    }
};

inline const std::vector<DoomClip>& doomClips() {
    static const std::vector<DoomClip> list = {
        // ---- the three macros, each swept from nothing to full
        {"wrath_sweep", "WRATH", 10.f,
         [] {
             auto saw = std::make_shared<Saw>();
             return Step([=](float t, PanelState& s, bool&) { // A1 saw at 1.5 V: ringing, then it screams over the saw
                 s.cutoff = 0.45f;
                 s.wrath  = t / 10.f;
                 return 0.3f * saw->tick(55.f, 48000.f);
             });
         }},
        {"ruin_sweep", "RUIN", 10.f,
         [] {
             auto saw = std::make_shared<Saw>();
             return Step([=](float t, PanelState& s, bool&) { // A1 saw at 1 V, WRATH half: metal, then it bends, then it breaks free
                 s.cutoff = 0.45f;
                 s.wrath  = 0.5f;
                 s.ruin   = t / 10.f;
                 return 0.2f * saw->tick(55.f, 48000.f);
             });
         }},
        {"doom_sweep", "DOOM", 10.f,
         [] {
             auto saw = std::make_shared<Saw>();
             return Step([=](float t, PanelState& s, bool&) { // A2 saw: an octave down, then two, then the lurch
                 s.cutoff = 0.55f;
                 s.wrath  = 0.3f;
                 s.doom   = t / 10.f;
                 return saw->tick(110.f, 48000.f);
             });
         }},
        // ---- the five patches in docs/doombringer.md
        {"p1_acid_rite", "Acid rite", 10.f,
         [] {
             auto saw = std::make_shared<Saw>();
             return Step([=](float t, PanelState& s, bool&) { // a 16-step bassline; HUNGER is the envelope
                 static const float notes[] = {0, 0, 12, 0, 3, 0, 15, 10, 0, 0, 12, 7, 0, 5, 3, 12};
                 const float        st      = t * 8.f;
                 const float        gate    = std::exp(-7.f * (st - std::floor(st)));
                 s.cutoff                   = 0.38f + 0.12f * std::sin(t * 0.6f);
                 s.wrath                    = 0.55f;
                 s.hunger                   = 0.85f;
                 s.ruin                     = t > 5.f ? 0.3f : 0.f;
                 return gate * saw->tick(55.f * std::exp2(notes[static_cast<int>(st) % 16] / 12.f), 48000.f);
             });
         }},
        {"p2_pitch_drop", "Pitch-drop kicks", 10.f,
         [] {
             auto clk = std::make_shared<Clock>();
             return Step([=](float t, PanelState& s, bool& ping) { // no input: PING on every beat
                 ping     = clk->tick(t, 0.5f);
                 s.cutoff = 0.21f;
                 s.wrath  = 0.6f;
                 s.ruin   = 0.42f;
                 return 0.f;
             });
         }},
        {"p3_scream", "The scream", 10.f,
         [] {
             auto clk = std::make_shared<Clock>();
             auto n   = std::make_shared<int>(0);
             return Step([=](float t, PanelState& s, bool&) { // the filter is the oscillator: a sequence into the cutoff CV
                 static const float notes[] = {0, 3, 7, 10, 12, 10, 7, 3};
                 if (clk->tick(t, 0.25f)) ++*n;
                 s.cutoff   = 0.4f;
                 s.cutoffCv = notes[*n % 8] / 12.f;
                 s.wrath    = 0.75f;
                 s.ruin     = 0.15f + 0.5f * t / 10.f;
                 s.morph    = 0.25f;
                 return 0.f;
             });
         }},
        {"p4_undertow", "Undertow", 10.f,
         [] {
             auto saw = std::make_shared<Saw>();
             return Step([=](float t, PanelState& s, bool&) { // a slow riff dragged down an octave and two
                 static const float notes[] = {0, 0, 1, 0, -2, 0, 3, 1};
                 s.cutoff                   = 0.5f;
                 s.wrath                    = 0.45f;
                 s.doom                     = 0.85f;
                 s.hunger                   = 0.65f;
                 const float st             = t * 2.f;
                 const float gate           = std::fmin(1.f, 3.f * std::exp(-1.5f * (st - std::floor(st))));
                 return gate * saw->tick(110.f * std::exp2(notes[static_cast<int>(st) % 8] / 12.f), 48000.f);
             });
         }},
        {"p5_deaths_hand", "Death's Hand into Doombringer", 10.f,
         [] {
             auto osc = std::make_shared<cosc::ComplexOsc<4>>();
             osc->init(48000.f);
             cosc::PanelState o;
             o.pitch  = 1.f / 6.f; // C2
             o.flesh  = 0.55f;
             o.malice = 0.35f;
             osc->setParams(cosc::panelToParams(o));
             osc->snapSmoothers();
             return Step([=](float t, PanelState& s, bool&) { // the oscillator, folded and snarling, into WRATH and RUIN
                 s.cutoff = 0.35f + 0.35f * (0.5f - 0.5f * std::cos(t * 0.63f));
                 s.wrath  = 0.6f;
                 s.ruin   = 0.2f + 0.35f * t / 10.f;
                 s.doom   = 0.3f;
                 float p, m;
                 osc->process(0.f, p, m);
                 return p;
             });
         }},
    };
    return list;
}

// Render a clip in volts. The panel updates once per 48 samples, like the
// firmware's 1 kHz control rate; a ping fires on the sample it is set.
inline std::vector<float> renderClip(const DoomClip& c, float fs) {
    static Engine<4> e;
    e.init(fs);
    Step       step = c.make();
    PanelState s;
    bool       ping = false;
    step(0.f, s, ping);
    e.setParams(panelToParams(s));
    e.snapSmoothers();
    std::vector<float> out(static_cast<size_t>(c.seconds * fs));
    for (size_t i = 0; i < out.size(); ++i) {
        const float t  = static_cast<float>(i) / fs;
        s              = PanelState{};
        ping           = false;
        const float in = step(t, s, ping);
        if (i % 48 == 0) e.setParams(panelToParams(s));
        if (ping) e.ping();
        float lp;
        e.process(in, 0.f, out[i], lp);
    }
    return out;
}

} // namespace doom
