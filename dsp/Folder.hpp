// Wavefolder for the primary oscillator's "timbre" stage.
//
// A sine-shaped transfer curve folds smoothly (no hard corners), so harmonics
// fall off much faster than with a triangle folder — kinder to aliasing.
// Symmetry adds a bias before folding -> even harmonics.
//
// The bias also creates DC, and simply high-passing it away shifts the wave
// so peaks blow past +-1 (and clip a DC-coupled codec). Because the core is a
// sine, the DC is known in closed form:
//
//     mean over theta of sin(a*sin(theta) + a*b) = sin(a*b) * J0(a)
//
// so we subtract it exactly and rescale, keeping the output DC-free and inside
// +-1 with no limiter (limiters add corners, corners alias).
#pragma once

#include "Math.hpp"

namespace cosc {

class Folder {
  public:
    static constexpr float kMaxDrive = 8.f;

    // Builds the J0 lookup table. Call once at init (uses std::cos, not RT-safe).
    void init() {
        for (int i = 0; i <= kTableSize; ++i) {
            double    a = static_cast<double>(i) * kTableMax / kTableSize, sum = 0.0;
            const int steps = 256; // J0(a) = 1/pi * integral_0^pi cos(a sin t) dt
            for (int k = 0; k < steps; ++k) {
                double t = (k + 0.5) * 3.14159265358979 / steps;
                sum += std::cos(a * std::sin(t));
            }
            j0_[i] = static_cast<float>(sum / steps);
        }
    }

    // x: -1..1 (the sine core), timbre: 0..1, symmetry: -1..1. Returns -1..1.
    float process(float x, float timbre, float symmetry) const {
        const float drive = 1.f + clampf(timbre, 0.f, 1.f) * (kMaxDrive - 1.f);
        const float b     = symmetry * 0.5f;
        // sin(pi/2 * drive * (x + b)) in turns: at drive=1 a +-1 input maps to +-1.
        const float folded = sinTurns(0.25f * drive * (x + b));
        // Fade the folder in over the bottom of the knob so timbre=0 and
        // symmetry=0 give a pure sine.
        const float wet = clampf(timbre * 6.666f + std::fabs(symmetry) * 4.f, 0.f, 1.f);

        const float a  = 0.5f * kPi * drive;
        const float dc = wet * sinTurns(0.25f * drive * b) * j0(a);
        return (lerpf(x, folded, wet) - dc) / (1.f + std::fabs(dc));
    }

  private:
    static constexpr int   kTableSize = 256;
    static constexpr float kTableMax  = 0.5f * kPi * kMaxDrive + 0.1f;

    float j0(float a) const {
        float pos = clampf(a, 0.f, kTableMax) * (kTableSize / kTableMax);
        int   i   = static_cast<int>(pos);
        if (i >= kTableSize) return j0_[kTableSize];
        return lerpf(j0_[i], j0_[i + 1], pos - static_cast<float>(i));
    }

    float j0_[kTableSize + 1] = {};
};

} // namespace cosc
