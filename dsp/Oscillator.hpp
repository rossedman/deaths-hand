// Phase accumulator + waveform generators.
#pragma once

#include "Math.hpp"

namespace cosc {

// Bidirectional phase accumulator. A negative increment runs the phase
// backwards, which is what makes through-zero FM work.
struct Phasor {
    float phase = 0.f;
    float tick(float inc) {
        phase += inc;
        phase -= std::floor(phase);
        return phase;
    }
    void reset(float p = 0.f) { phase = p; }
};

enum class ModShape : uint8_t { Sine = 0, Triangle, Saw, Square, Count };

// Modulator oscillator: four classic shapes, band-limited with PolyBLEP
// (saw/square) so its own output is clean even before oversampling.
struct ModOscillator {
    Phasor   phasor;
    ModShape shape = ModShape::Sine;

    // inc = freq / sampleRate (always positive here). Returns -1..1.
    float process(float inc) {
        float t  = phasor.tick(inc);
        float dt = inc < 0.f ? -inc : inc;
        switch (shape) {
            case ModShape::Sine: return sinTurns(t);
            case ModShape::Triangle: return 1.f - 4.f * std::fabs(t - 0.5f);
            case ModShape::Saw: return (2.f * t - 1.f) - polyBlep(t, dt);
            case ModShape::Square: {
                float sq = t < 0.5f ? 1.f : -1.f;
                float t2 = t + 0.5f;
                t2 -= std::floor(t2);
                return sq + polyBlep(t, dt) - polyBlep(t2, dt);
            }
            default: return 0.f;
        }
    }
    void reset() { phasor.reset(); }
};

} // namespace cosc
