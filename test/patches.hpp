// The documented patches, written as patch.Init knob positions (0..1 = 7 o'clock
// .. 5 o'clock) and CVs, and rendered through the firmware's own panel mapping.
// Shared by the host tests (test/out/ex*.wav) and the docs renders (render.cpp).
#pragma once

#include <cmath>
#include <functional>
#include <vector>

#include "../dsp/Panel.hpp"

namespace cosc {

// The six patches in the README: 10 s each, automate(t) gets t = 0..1 over the clip.
struct Patch {
    const char*                             file;
    const char*                             name;
    std::function<void(float, PanelState&)> automate;
};

inline const std::vector<Patch>& patches() {
    static const std::vector<Patch> list = {
        {"ex1_reactor_hum.wav", "Reactor hum",
         [](float t, PanelState& s) {  // low drone; rock MALICE to make the feedback breathe
             s.pitch    = 1.f / 6.f;   // C2
             s.ratio    = 1.5f / 11.f; // 1:2
             s.flesh    = 0.35f;
             s.malice   = 0.3f + 0.2f * std::sin(t * 6.283f * 1.5f);
             s.legion   = 0.12f;
             s.symmetry = 0.65f;
         }},
        {"ex2_lorenz_growl.wav", "Lorenz growl",
         [](float t, PanelState& s) { // chaotic modulator in audio range: an engine that won't idle
             s.pitch     = 0.25f;
             s.ratioLock = false;
             s.ratio     = 0.618f; // free modulator, about 113 Hz
             s.shape     = 0.9f;   // Lorenz
             s.flesh     = 0.4f + 0.2f * t;
             s.routing   = 1;
         }},
        {"ex3_chaos_bells.wav", "Chaos bells",
         [](float t, PanelState& s) { // sequencer -> CV_5, decaying envelope -> CV_6 (FLESH), MALICE rises
             static const float notes[] = {0, 7, 3, 10, 5, 12, 1, 8};
             const float        step    = t * 16.f;
             const float        env     = std::exp(-5.f * (step - std::floor(step)));
             s.pitch                    = 0.5f;
             s.pitchCv                  = notes[static_cast<int>(step) % 8] / 12.f;
             s.ratio                    = 5.5f / 11.f; // 1:3
             s.flesh                    = 0.05f;
             s.fleshCv                  = 2.5f * env;
             s.malice                   = 0.45f + 0.4f * t;
             s.routing                  = 2;
         }},
        {"ex4_feedback_scream.wav", "Feedback scream",
         [](float t, PanelState& s) { // pitch dives while MALICE pushes past saw into noise
             s.pitch    = 0.917f - 0.667f * t;
             s.ratio    = 3.5f / 11.f; // 1:1.5
             s.flesh    = 0.6f;
             s.malice   = 0.5f + 0.5f * t;
             s.symmetry = 0.3f;
         }},
        {"ex5_possession.wav", "Possession",
         [](float t, PanelState& s) { // everything rising; the modulator turns to Lorenz halfway
             s.pitch    = 1.f / 3.f;
             s.ratio    = 4.5f / 11.f;            // 1:2
             s.shape    = t < 0.5f ? 0.7f : 0.9f; // square, then Lorenz
             s.flesh    = 0.45f + 0.45f * t;
             s.malice   = 0.25f + 0.6f * t;
             s.legion   = 0.3f;
             s.symmetry = 0.75f;
             s.routing  = 3;
         }},
        {"ex6_legion.wav", "Legion",
         [](float t, PanelState& s) { // one voice becomes a swarm while the fold opens
             s.pitch  = 1.f / 6.f;
             s.flesh  = 0.3f + 0.3f * t;
             s.malice = 0.2f;
             s.legion = 0.15f + 0.85f * t;
         }},
    };
    return list;
}

// Showcase clips for the docs: each macro on its own, then patches that use the
// module the way a rack would, with an envelope-driven VCA after the oscillator
// where noted. automate(t) gets t in seconds; vca (0..1) multiplies the output.
struct Clip {
    const char*                                     file;
    const char*                                     name;
    float                                           seconds;
    std::function<void(float, PanelState&, float&)> automate;
};

inline float clipRamp(float t, float t0, float t1) { return std::fmin(1.f, std::fmax(0.f, (t - t0) / (t1 - t0))); }
inline float pitchKnob(float volts) { return (volts + 3.f) / 6.f; } // 0 V = C4

inline const std::vector<Clip>& showcase() {
    static const std::vector<Clip> list = {
        {"flesh_sweep", "FLESH, 0 to full", 12.f,
         [](float t, PanelState& s, float&) { // C2. FM sidebands arrive, then the fold opens, then it tips asymmetric.
             s.pitch = pitchKnob(-2.f);
             s.flesh = clipRamp(t, 0.5f, 11.5f);
         }},
        {"malice_sweep", "MALICE, 0 to full", 12.f,
         [](float t, PanelState& s, float&) { // A1. Sine to saw through the lower half, then over the edge into chaos and collapse.
             s.pitch  = pitchKnob(-2.25f);
             s.flesh  = 0.15f;
             s.malice = clipRamp(t, 0.5f, 11.5f);
         }},
        {"legion_sweep", "LEGION, 0 to full", 14.f,
         [](float t, PanelState& s, float&) { // C2 with a little FLESH. The sub, then voices fading in and beating, then drift.
             s.pitch  = pitchKnob(-2.f);
             s.flesh  = 0.45f;
             s.legion = clipRamp(t, 0.5f, 13.f);
         }},
        {"altar_drone", "Altar drone", 20.f,
         [](float t, PanelState& s, float&) { // E1, ring routing, LEGION near full, a 0.1 Hz LFO into FLESH's CV.
             s.pitch   = pitchKnob(-3.f + 4.f / 12.f);
             s.ratio   = 3.5f / 11.f;
             s.routing = 3;
             s.legion  = 0.9f;
             s.malice  = 0.35f;
             s.flesh   = 0.35f;
             s.fleshCv = 1.5f * std::sin(6.2832f * 0.1f * t);
         }},
        {"hex_sequence", "Hex sequence", 15.f,
         [](float t, PanelState& s, float& vca) { // 16ths at 128 BPM in E phrygian; each step's envelope hits FLESH and a VCA while MALICE creeps up.
             static const int notes[16] = {0, 0, 12, 1, 0, 7, 0, 5, 0, 0, 12, 1, 3, 1, 0, -2};
             const float      step      = t * 128.f / 60.f * 4.f, ph = step - std::floor(step);
             const float      env       = std::exp(-7.f * ph);
             s.pitch                    = pitchKnob(-2.f + 4.f / 12.f);
             s.pitchCv                  = static_cast<float>(notes[static_cast<int>(step) % 16]) / 12.f;
             s.ratio                    = 4.5f / 11.f; // 1:2
             s.flesh                    = 0.15f;
             s.fleshCv                  = 3.f * env;
             s.malice                   = 0.25f + 0.35f * clipRamp(t, 4.f, 14.f);
             s.legion                   = 0.45f;
             vca                        = std::fmin(1.f, ph / 0.03f) * (0.15f + 0.85f * std::exp(-4.f * ph));
         }},
        {"lorenz_choir", "Lorenz choir", 18.f,
         [](float t, PanelState& s, float&) { // A free modulator on Lorenz just below the note, LEGION full, FLESH breathing slowly.
             s.pitch     = pitchKnob(-1.f);
             s.ratioLock = false;
             s.ratio     = (-1.2f + 8.f) / 11.f;
             s.shape     = 0.9f;
             s.routing   = 1;
             s.legion    = 1.f;
             s.flesh     = 0.25f + 0.3f * (0.5f - 0.5f * std::cos(6.2832f * t / 18.f));
         }},
        {"bells_of_ruin", "Bells of ruin", 16.f,
         [](float t, PanelState& s, float& vca) { // A 1:7 ratio struck every 1.5 s through a VCA, MALICE climbing through the edge.
             static const int notes[4] = {0, -5, 3, -2};
             const float      ph       = std::fmod(t, 1.5f);
             const float      env      = std::exp(-2.2f * ph);
             s.pitch                   = pitchKnob(0.f);
             s.pitchCv                 = static_cast<float>(notes[static_cast<int>(t / 1.5f) % 4]) / 12.f;
             s.ratio                   = 9.5f / 11.f; // 1:7
             s.routing                 = 2;
             s.flesh                   = 0.1f;
             s.fleshCv                 = 2.f * env;
             s.malice                  = 0.3f + 0.5f * clipRamp(t, 2.f, 15.f);
             vca                       = std::fmin(1.f, ph / 0.004f) * std::exp(-1.6f * ph);
         }},
        {"descent", "Descent", 14.f,
         [](float t, PanelState& s, float&) { // Four and a half octaves down while FLESH, LEGION and MALICE all rise into collapse.
             const float r = clipRamp(t, 0.5f, 13.f);
             s.pitch       = pitchKnob(2.f - 4.5f * r);
             s.ratio       = 3.5f / 11.f;
             s.routing     = 3;
             s.flesh       = 0.3f + 0.6f * r;
             s.malice      = 0.2f + 0.8f * r * r;
             s.legion      = 0.2f + 0.7f * r;
             s.symmetry    = 0.7f;
         }},
    };
    return list;
}

} // namespace cosc
