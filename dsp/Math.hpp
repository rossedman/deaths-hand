// Small, allocation-free math helpers shared by every target.
// Everything here must compile on a Cortex-M7 (Daisy) and on a desktop.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

namespace cosc {

constexpr float kPi    = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;
constexpr float kC4Hz  = 261.6255653f; // 0 V on a 1V/oct input (matches VCV Rack)

inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// 2^x without powf. Range-reduces to [-0.5, 0.5] and uses a 6th-order
// polynomial: worst-case error ~3e-6 relative (~0.005 cents).
inline float fastExp2(float x) {
    x        = clampf(x, -126.f, 126.f);
    float n  = std::floor(x + 0.5f);
    float f  = x - n;
    float p  = 1.0f + f * (0.69314718f + f * (0.24022651f + f * (0.05550411f + f * (0.00961813f + f * (0.00133336f + f * 0.00015404f)))));
    int32_t bits = (static_cast<int32_t>(n) + 127) << 23;
    float scale;
    std::memcpy(&scale, &bits, sizeof(scale));
    return p * scale;
}

// 1V/oct volts -> Hz, 0 V = C4.
inline float voltsToHz(float v) { return kC4Hz * fastExp2(v); }

// sin(2*pi*turns) for any input. Folds to [-pi/2, pi/2] and uses a 9th-order
// odd polynomial: harmonics sit around -110 dB, plenty for audio.
inline float sinTurns(float turns) {
    float q = turns - std::floor(turns + 0.5f); // [-0.5, 0.5)
    if (q > 0.25f) q = 0.5f - q;
    else if (q < -0.25f) q = -0.5f - q;
    float x  = q * kTwoPi;
    float x2 = x * x;
    return x * (1.f + x2 * (-1.6666667e-1f + x2 * (8.3333333e-3f + x2 * (-1.9841270e-4f + x2 * 2.7557319e-6f))));
}

// PolyBLEP residual for a discontinuity at phase 0. t in [0,1), dt = |phase increment|.
inline float polyBlep(float t, float dt) {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.f;
    }
    if (t > 1.f - dt) {
        t = (t - 1.f) / dt;
        return t * t + t + t + 1.f;
    }
    return 0.f;
}

// One-pole smoother for knob/CV values (kills zipper noise).
struct Smoother {
    float y = 0.f, coeff = 1.f;
    void setTime(float seconds, float sampleRate) {
        coeff = seconds <= 0.f ? 1.f : 1.f - std::exp(-1.f / (seconds * sampleRate));
    }
    float process(float x) { return y += coeff * (x - y); }
    void reset(float x) { y = x; }
};

// One-pole DC blocker (~10 Hz). Outputs on hardware are DC-coupled.
struct DcBlocker {
    float x1 = 0.f, y1 = 0.f, r = 0.9987f;
    void setSampleRate(float sr) { r = 1.f - (kTwoPi * 10.f / sr); }
    float process(float x) {
        float y = x - x1 + r * y1;
        x1 = x;
        y1 = y;
        return y;
    }
};

// Transparent below |x| = knee, then bends smoothly toward +-ceiling.
// Run it at the oversampled rate so the bend itself doesn't alias.
inline float softLimit(float x, float knee = 1.0f, float ceiling = 1.2f) {
    float a = std::fabs(x);
    if (a <= knee) return x;
    float w = ceiling - knee;
    float u = (a - knee) / w;
    u       = u > 3.f ? 1.f : u * (27.f + u * u) / (27.f + 9.f * u * u); // fast tanh
    return std::copysign(knee + w * u, x);
}

} // namespace cosc
