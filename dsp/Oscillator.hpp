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

// Lorenz attractor used as an oscillator. "Frequency" sets integration speed,
// so it tracks pitch loosely while never quite repeating: a breathing, growling
// modulator, or an organic drifting LFO at low rates.
struct LorenzOsc {
    // Classic chaotic parameters.
    static constexpr float kSigma = 10.f, kRho = 28.f, kBeta = 8.f / 3.f;
    // Roughly one loop around a lobe per 0.75 time units -> pitch-ish tracking.
    static constexpr float kTimePerCycle = 0.75f;
    // Largest step we trust the RK2 integrator with; above this, speed saturates.
    static constexpr float kMaxStep = 0.02f;

    float x = 1.f, y = 1.f, z = 20.f;

    // inc = cycles per sample (sign ignored). Returns about -1..1.
    float process(float inc) {
        float h = clampf(std::fabs(inc) * kTimePerCycle, 0.f, kMaxStep);
        // Midpoint (RK2): stable at kMaxStep where Euler would spiral out.
        float dx1 = kSigma * (y - x), dy1 = x * (kRho - z) - y, dz1 = x * y - kBeta * z;
        float xm = x + 0.5f * h * dx1, ym = y + 0.5f * h * dy1, zm = z + 0.5f * h * dz1;
        x += h * (kSigma * (ym - xm));
        y += h * (xm * (kRho - zm) - ym);
        z += h * (xm * ym - kBeta * zm);
        // Guard: chaos is welcome, NaN and runaway are not.
        if (!(std::fabs(x) < 100.f && std::fabs(y) < 100.f && z > -10.f && z < 120.f)) reset();
        return clampf(x * (1.f / 20.f), -1.f, 1.f);
    }
    void reset() { x = 1.f; y = 1.f; z = 20.f; }
};

enum class ModShape : uint8_t { Sine = 0, Triangle, Saw, Square, Lorenz, Count };

// Modulator oscillator: four classic shapes, band-limited with PolyBLEP
// (saw/square) so its own output is clean even before oversampling, plus Lorenz.
struct ModOscillator {
    Phasor    phasor;
    LorenzOsc lorenz;
    ModShape  shape = ModShape::Sine;

    // inc = freq / sampleRate; may go negative under cross-modulation. Returns -1..1.
    float process(float inc) {
        if (shape == ModShape::Lorenz) return lorenz.process(inc);
        float t  = phasor.tick(inc);
        float dt = clampf(std::fabs(inc), 0.f, 0.5f);
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
    void reset() {
        phasor.reset();
        lorenz.reset();
    }
};

} // namespace cosc
