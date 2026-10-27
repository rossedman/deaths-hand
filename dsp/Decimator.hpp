// FIR decimator for oversampled synthesis.
//
// The engine *generates* audio at Factor x the output rate (so there's no
// upsampler — oscillators can simply run faster). Before going back to the
// output rate we low-pass at the output Nyquist and keep every Factor-th sample.
// Taps are designed once at init with a Kaiser-windowed sinc; no heap, fixed size.
#pragma once

#include "Math.hpp"

namespace cosc {

template <int Factor, int Taps>
class Decimator {
    static_assert(Factor >= 1, "Factor must be >= 1");
    static_assert(Taps % 2 == 1, "use an odd tap count (linear phase, integer delay)");

  public:
    // stopbandDb sets the Kaiser beta. ~90 dB keeps aliases below the noise floor
    // of a 24-bit codec in practice.
    void init(float stopbandDb = 90.f) {
        const double beta   = kaiserBeta(stopbandDb);
        const double cutoff = 0.5 / Factor;  // normalized to the oversampled rate
        const double i0b    = besselI0(beta);
        const int    mid    = Taps / 2;
        double       h[Taps];
        double       sum    = 0.0;
        for (int n = 0; n < Taps; ++n) {
            int    k    = n - mid;
            double sinc = k == 0 ? 2.0 * cutoff : std::sin(2.0 * 3.14159265358979 * cutoff * k) / (3.14159265358979 * k);
            double r    = static_cast<double>(k) / mid;
            double w    = besselI0(beta * std::sqrt(1.0 - r * r)) / i0b;
            h[n]        = sinc * w;
            sum += h[n];
        }
        for (int n = 0; n < Taps; ++n) h_[n] = static_cast<float>(h[n] / sum); // unity DC gain
        reset();
    }

    void reset() {
        for (float& s : buf_) s = 0.f;
        pos_   = 0;
        phase_ = 0;
    }

    // Push one oversampled sample. Returns true when an output is ready in `out`.
    bool push(float x, float& out) {
        buf_[pos_]        = x;
        buf_[pos_ + Taps] = x; // mirrored so the convolution never wraps
        if (++pos_ == Taps) pos_ = 0;
        if (++phase_ < Factor) return false;
        phase_ = 0;
        // Oldest sample is at pos_, newest at pos_ + Taps - 1.
        const float* p   = &buf_[pos_];
        float        acc = 0.f;
        for (int n = 0; n < Taps; ++n) acc += h_[n] * p[n];
        out = acc;
        return true;
    }

    static constexpr int latency() { return Taps / 2; } // in oversampled samples

  private:
    static double besselI0(double x) {
        double sum = 1.0, term = 1.0, q = x * x / 4.0;
        for (int k = 1; k < 64; ++k) {
            term *= q / (static_cast<double>(k) * k);
            sum += term;
            if (term < 1e-12 * sum) break;
        }
        return sum;
    }
    static double kaiserBeta(double a) {
        if (a > 50.0) return 0.1102 * (a - 8.7);
        if (a >= 21.0) return 0.5842 * std::pow(a - 21.0, 0.4) + 0.07886 * (a - 21.0);
        return 0.0;
    }

    float h_[Taps]       = {};
    float buf_[2 * Taps] = {};
    int   pos_ = 0, phase_ = 0;
};

// Factor 1 = no oversampling (handy for A/B-testing aliasing).
template <int Taps>
class Decimator<1, Taps> {
  public:
    void init(float = 0.f) {}
    void reset() {}
    bool push(float x, float& out) { out = x; return true; }
    static constexpr int latency() { return 0; }
};

} // namespace cosc
