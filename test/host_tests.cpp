// Host-side tests + WAV renders for the portable DSP core.
// Build & run:  make -C test && ./test/build/host_tests test/out
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "../dsp/ComplexOsc.hpp"
#include "../dsp/Pickup.hpp"

using namespace cosc;

static constexpr float kFs        = 48000.f;
static int             g_failures = 0;

#define CHECK(cond, ...)                         \
    do {                                         \
        if (!(cond)) {                           \
            std::printf("  FAIL: " __VA_ARGS__); \
            std::printf("\n");                   \
            ++g_failures;                        \
        }                                        \
    } while (0)

// ---------------------------------------------------------------- WAV writer
static void writeWav(const std::string& path, const std::vector<float>& l, const std::vector<float>& r, float fs) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) {
        std::perror(path.c_str());
        return;
    }
    const uint32_t n = static_cast<uint32_t>(l.size()), ch = 2, bits = 16;
    const uint32_t dataBytes = n * ch * bits / 8, rate = static_cast<uint32_t>(fs);
    auto           u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto           u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f);
    u32(36 + dataBytes);
    std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16);
    u16(1);
    u16(ch);
    u32(rate);
    u32(rate * ch * bits / 8);
    u16(ch * bits / 8);
    u16(bits);
    std::fwrite("data", 1, 4, f);
    u32(dataBytes);
    for (uint32_t i = 0; i < n; ++i)
        for (float v : {l[i], r[i]}) {
            // 8 V = full scale; the engine limits itself to about +-6.3 V.
            float   s = clampf(v / 8.f, -1.f, 1.f);
            int16_t q = static_cast<int16_t>(s * 32767.f);
            std::fwrite(&q, 2, 1, f);
        }
    std::fclose(f);
}

// Render `seconds` of audio, calling `automate(t01, params)` every sample.
template <int OS>
static void render(const std::string& path, float seconds, const std::function<void(float, Params&)>& automate) {
    static ComplexOsc<OS> osc; // static: Decimator buffers are large-ish
    osc.init(kFs);
    const int          n = static_cast<int>(seconds * kFs);
    std::vector<float> l(n), r(n);
    Params             p;
    automate(0.f, p);
    osc.setParams(p);
    osc.snapSmoothers();
    for (int i = 0; i < n; ++i) {
        automate(static_cast<float>(i) / n, p);
        osc.setParams(p);
        osc.process(0.f, l[i], r[i]);
    }
    writeWav(path, l, r, kFs);
    std::printf("  wrote %s\n", path.c_str());
}

// Frequency by interpolated rising zero crossings (skips the first 0.1 s).
static double measureHz(const std::vector<float>& x, float fs) {
    int    start = static_cast<int>(0.1f * fs);
    double first = -1, last = -1;
    int    count = 0;
    for (size_t i = start + 1; i < x.size(); ++i) {
        if (x[i - 1] < 0.f && x[i] >= 0.f) {
            double frac = x[i - 1] / (x[i - 1] - x[i]);
            double t    = (i - 1) + frac;
            if (first < 0) first = t;
            else ++count;
            last = t;
        }
    }
    return count > 0 ? count * fs / (last - first) : 0.0;
}

// ---------------------------------------------------------------- tests
static void testPitchTracking() {
    std::printf("pitch tracking (1V/oct, 0 V = C4)\n");
    static ComplexOsc<4> osc;
    double               worst = 0;
    for (float v = -3.f; v <= 5.01f; v += 1.f) {
        osc.init(kFs);
        Params p;
        p.pitchV = v;
        osc.setParams(p);
        osc.snapSmoothers();
        std::vector<float> out(static_cast<size_t>(kFs * 2)), mod(out.size());
        for (size_t i = 0; i < out.size(); ++i) osc.process(0.f, out[i], mod[i]);
        double want  = kC4Hz * std::pow(2.0, v);
        double got   = measureHz(out, kFs);
        double cents = 1200.0 * std::log2(got / want);
        if (std::fabs(cents) > std::fabs(worst)) worst = cents;
        std::printf("  %+5.1f V  want %9.3f Hz  got %9.3f Hz  %+7.4f cents\n", v, want, got, cents);
        CHECK(std::fabs(cents) < 0.5, "pitch error %.3f cents at %.1f V", cents, v);
    }
    std::printf("  worst error: %+.4f cents\n", worst);
}

static void testStabilityUnderAbuse() {
    std::printf("stability: random extreme params + external FM\n");
    static ComplexOsc<4> osc;
    osc.init(kFs);
    std::mt19937                          rng(1234);
    std::uniform_real_distribution<float> u(0.f, 1.f);
    float                                 peak = 0.f;
    bool                                  bad  = false;
    for (int block = 0; block < 2000; ++block) {
        Params p;
        p.pitchV    = -6.f + u(rng) * 14.f; // way outside normal range
        p.modPitchV = -10.f + u(rng) * 20.f;
        p.ratioLock = u(rng) > 0.5f;
        p.ratio     = quantizeRatio(u(rng));
        p.fmIndex   = u(rng) * 1.5f; // over-range on purpose
        p.amDepth   = u(rng);
        p.timbre    = u(rng) * 1.5f;
        p.symmetry  = -1.5f + u(rng) * 3.f;
        p.timbreMod = u(rng);
        p.feedback  = u(rng) * 1.5f; // over-range on purpose
        p.chaos     = u(rng) * 1.5f;
        p.modShape  = static_cast<ModShape>(block % static_cast<int>(ModShape::Count));
        osc.setParams(p);
        if (block % 97 == 0) osc.sync();
        for (int i = 0; i < 48; ++i) {
            float a, b, ext = (u(rng) * 2.f - 1.f) * 10.f;
            osc.process(ext, a, b);
            if (!std::isfinite(a) || !std::isfinite(b)) bad = true;
            peak = std::max(peak, std::max(std::fabs(a), std::fabs(b)));
        }
    }
    std::printf("  peak output %.2f V, finite: %s\n", peak, bad ? "NO" : "yes");
    CHECK(!bad, "non-finite output");
    // Limiter ceiling is 6 V; band-limiting a limited wave adds ~15% Gibbs overshoot.
    CHECK(peak < 7.5f, "output peak %.2f V exceeds the +-7.5 V budget", peak);
}

static void testThroughZero() {
    std::printf("through-zero FM: phase must run backwards when deviation > carrier\n");
    // With index=1 (8x deviation) and a 1:1 ratio, the instantaneous increment
    // goes negative for part of every cycle. A unidirectional (non-TZ) phasor
    // would stall there; we just check the output stays lively and finite.
    static ComplexOsc<4> osc;
    osc.init(kFs);
    Params p;
    p.fmIndex = 1.f;
    osc.setParams(p);
    osc.snapSmoothers();
    double energy = 0.0;
    for (int i = 0; i < 48000; ++i) {
        float a, b;
        osc.process(0.f, a, b);
        energy += static_cast<double>(a) * a;
    }
    double rms = std::sqrt(energy / 48000.0);
    std::printf("  rms %.2f V\n", rms);
    CHECK(rms > 1.0, "output collapsed under TZFM (rms %.3f)", rms);
}

// Energy of a Hann-windowed signal that is NOT on harmonics of f0 (0..1).
// f0 must land on an exact FFT bin of the buffer (1 Hz bins for 1 s at 48 kHz).
static double nonHarmonicRatio(const std::vector<float>& x, double f0, std::vector<double>* harmDb = nullptr) {
    const size_t        n = x.size();
    std::vector<double> w(n);
    double              total = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double hann = 0.5 - 0.5 * std::cos(2.0 * 3.14159265358979 * i / n);
        w[i]        = x[i] * hann;
        total += w[i] * w[i];
    }
    total *= static_cast<double>(n); // Parseval: sum |X_k|^2 over all bins
    auto binPower = [&](double k) {  // Goertzel at an exact bin
        double c = 2.0 * std::cos(2.0 * 3.14159265358979 * k / n), s1 = 0, s2 = 0;
        for (double v : w) {
            double s0 = v + c * s1 - s2;
            s2        = s1;
            s1        = s0;
        }
        return s1 * s1 + s2 * s2 - c * s1 * s2;
    };
    double harm = 0.0, fund = 0.0;
    for (int h = 1; h * f0 < kFs / 2 - 2; ++h) {
        double k = h * f0, e = 0.0;
        for (int d = -1; d <= 1; ++d) e += 2.0 * binPower(k + d); // Hann main lobe, +/- freqs
        harm += e;
        if (h == 1) fund = e;
        if (harmDb && h <= 8) harmDb->push_back(10.0 * std::log10(e / fund + 1e-30));
    }
    return std::max(0.0, 1.0 - harm / total);
}

static std::vector<float> renderTone(Params p, int seconds = 1) {
    static ComplexOsc<4> osc;
    osc.init(kFs);
    osc.setParams(p);
    osc.snapSmoothers();
    float a, b;
    for (int i = 0; i < 12000; ++i) osc.process(0.f, a, b); // settle
    std::vector<float> out(static_cast<size_t>(kFs) * seconds);
    for (float& v : out) osc.process(0.f, v, b);
    return out;
}

static void testFeedback() {
    std::printf("self-feedback: sine -> saw-like -> noise\n");
    const float v440      = std::log2(440.f / kC4Hz); // 440 Hz lands on exact 1 Hz bins
    double      prevNoise = -1.0;
    for (float fb : {0.f, 0.5f, 0.75f, 1.f}) {
        Params p;
        p.pitchV   = v440;
        p.feedback = fb;
        std::vector<double> h;
        double              noise = nonHarmonicRatio(renderTone(p), 440.0, &h);
        std::printf("  fb %.2f  H2 %6.1f dB  H3 %6.1f dB  H5 %6.1f dB  non-harmonic %6.1f dB\n", fb, h[1], h[2], h[4], 10.0 * std::log10(noise + 1e-30));
        if (fb == 0.f) CHECK(h[1] < -70.0 && h[2] < -70.0, "fb=0 should be a pure sine");
        if (fb == 0.5f) CHECK(h[1] > -30.0 && h[2] > -40.0, "fb=0.5 should add strong harmonics");
        if (fb == 1.f) CHECK(noise > prevNoise, "max feedback should be noisier than 0.75");
        prevNoise = noise;
    }
}

static void testChaos() {
    std::printf("chaos: cross-FM should break periodicity\n");
    const float v440 = std::log2(440.f / kC4Hz);
    double      base = 0, wild = 0;
    for (float c : {0.f, 1.f}) {
        Params p;
        p.pitchV                 = v440;
        p.ratio                  = 1.f;
        p.fmIndex                = 0.4f;
        p.chaos                  = c;
        double r                 = nonHarmonicRatio(renderTone(p), 440.0);
        (c == 0.f ? base : wild) = r;
        std::printf("  chaos %.0f  non-harmonic %6.1f dB\n", c, 10.0 * std::log10(r + 1e-30));
    }
    CHECK(base < 1e-4, "chaos=0 should stay harmonic");
    CHECK(wild > 100.0 * base && wild > 1e-3, "chaos=1 should produce inharmonic energy");
}

static void testChaosLongRun() {
    std::printf("long-run stability: 60 s of worst-case evil per pitch\n");
    static ComplexOsc<4> osc;
    for (int stack = 0; stack < 2; ++stack)
        for (float v : {-4.f, 0.f, 3.f, 6.f}) {
            osc.init(kFs);
            Params p;
            p.pitchV    = v;
            p.ratioLock = false;
            p.modPitchV = v + 0.37f;
            p.fmIndex   = 1.f;
            p.feedback  = 1.f;
            p.chaos     = 1.f;
            p.timbre    = 1.f;
            p.symmetry  = 0.8f;
            p.timbreMod = 1.f;
            p.amDepth   = 0.5f;
            p.modShape  = ModShape::Lorenz;
            if (stack) { // and the whole unison stack on top
                p.voices = 4;
                p.spread = 1.f;
                p.drift  = 1.f;
                p.sub    = 1.f;
            }
            osc.setParams(p);
            osc.snapSmoothers();
            bool      bad  = false;
            float     peak = 0.f;
            double    e = 0.0, mean = 0.0;
            const int n = static_cast<int>(kFs) * 60, tail = static_cast<int>(kFs);
            for (int i = 0; i < n; ++i) {
                float a, b;
                osc.process(0.f, a, b);
                if (!std::isfinite(a) || !std::isfinite(b)) bad = true;
                peak = std::max(peak, std::max(std::fabs(a), std::fabs(b)));
                if (i >= n - tail) {
                    e += static_cast<double>(a) * a;
                    mean += a;
                }
            }
            double rms = std::sqrt(e / tail);
            mean /= tail;
            std::printf("  %+.0f V%s  peak %.2f V  last-second rms %.2f V  dc %+.3f V  finite %s\n", v, stack ? " stacked" : "", peak, rms, mean,
                        bad ? "NO" : "yes");
            CHECK(!bad, "non-finite output at %.0f V", v);
            CHECK(peak < 7.5f, "peak %.2f V at %.0f V", peak, v);
            CHECK(rms > 0.2, "output died (rms %.3f) at %.0f V", rms, v);
            CHECK(std::fabs(mean) < 0.25, "DC lock-up (%.3f V) at %.0f V", mean, v);
        }
}

static void testLorenz() {
    std::printf("Lorenz modulator: bounded and alive from 0.01 Hz to 20 kHz\n");
    for (float hz : {0.01f, 1.f, 100.f, 1000.f, 20000.f}) {
        ModOscillator m;
        m.shape        = ModShape::Lorenz;
        float       lo = 1.f, hi = -1.f;
        bool        bad = false;
        const float inc = hz / (kFs * 4.f);
        for (int i = 0; i < 192000 * 20; ++i) {
            float y = m.process(inc);
            if (!std::isfinite(y)) bad = true;
            lo = std::min(lo, y);
            hi = std::max(hi, y);
        }
        std::printf("  %8.2f Hz  range [%+.2f, %+.2f]\n", hz, lo, hi);
        CHECK(!bad, "Lorenz non-finite at %.2f Hz", hz);
        if (hz >= 1.f) CHECK(hi - lo > 0.8f, "Lorenz stuck at %.2f Hz", hz);
    }
}

// Short-window RMS envelope: max/min over 50 ms windows (whole cycles at 220 Hz).
static double envelopeSwing(const std::vector<float>& x) {
    const size_t w  = static_cast<size_t>(kFs / 20);
    double       lo = 1e9, hi = 0.0;
    for (size_t i = 0; i + w <= x.size(); i += w) {
        double e = 0.0;
        for (size_t j = i; j < i + w; ++j) e += static_cast<double>(x[j]) * x[j];
        e  = std::sqrt(e / w);
        lo = std::min(lo, e);
        hi = std::max(hi, e);
    }
    return hi / std::max(lo, 1e-9);
}

// Level of one exact 1 Hz bin relative to the whole signal, in dB.
static double binDb(const std::vector<float>& x, double hz) {
    const size_t n = x.size();
    double       c = 2.0 * std::cos(2.0 * 3.14159265358979 * hz / kFs), s1 = 0, s2 = 0, total = 0;
    for (size_t i = 0; i < n; ++i) {
        double v  = x[i] * (0.5 - 0.5 * std::cos(2.0 * 3.14159265358979 * i / n));
        double s0 = v + c * s1 - s2;
        s2        = s1;
        s1        = s0;
        total += v * v;
    }
    double p = (s1 * s1 + s2 * s2 - c * s1 * s2) * 2.0 / n;
    return 10.0 * std::log10(p / total + 1e-30);
}

static void testUnison() {
    std::printf("unison: voices, spread, drift, sub\n");
    const float v220 = std::log2(220.f / kC4Hz);

    // No spread and no drift: extra voices start in phase and stay there, so the
    // stack must sound exactly like one voice, folder and all.
    Params one;
    one.pitchV   = v220;
    one.timbre   = 0.6f;
    one.symmetry = 0.5f;
    one.feedback = 0.3f;
    Params four  = one;
    four.voices  = 4;
    auto  a = renderTone(one), b = renderTone(four);
    float diff = 0.f;
    for (size_t i = 0; i < a.size(); ++i) diff = std::max(diff, std::fabs(a[i] - b[i]));
    std::printf("  4 voices, no spread vs 1 voice: max difference %.2e V\n", diff);
    CHECK(diff < 1e-3f, "an unspread stack should match one voice (diff %.2e V)", diff);

    // Spread: the voices drift in and out of phase, so the level beats.
    Params beat            = one;
    beat.timbre            = 0.f;
    beat.symmetry          = 0.f;
    beat.feedback          = 0.f;
    Params flat            = beat;
    beat.voices            = 2;
    beat.spread            = 0.6f; // ~25 cents: a few beats per second at 220 Hz
    const double swingFlat = envelopeSwing(renderTone(flat, 2)), swingBeat = envelopeSwing(renderTone(beat, 2));
    std::printf("  envelope swing: 1 voice %.2fx, 2 voices spread %.2fx\n", swingFlat, swingBeat);
    CHECK(swingFlat < 1.05, "one voice should be steady (%.2fx)", swingFlat);
    CHECK(swingBeat > 3.0, "spread voices should beat (%.2fx)", swingBeat);

    // Drift alone (no spread) also pulls the voices apart.
    Params drift            = flat;
    drift.voices            = 3;
    drift.drift             = 1.f;
    const double swingDrift = envelopeSwing(renderTone(drift, 4));
    std::printf("  envelope swing: 3 voices, drift only %.2fx\n", swingDrift);
    CHECK(swingDrift > 1.5, "drift should detune the voices (%.2fx)", swingDrift);

    // Sub: a sine one octave down. 220 Hz and 110 Hz land on exact 1 Hz bins.
    Params sub          = flat;
    sub.sub             = 1.f;
    const double subOff = binDb(renderTone(flat), 110.0), subOn = binDb(renderTone(sub), 110.0);
    std::printf("  110 Hz (sub) level: off %.1f dB, on %.1f dB\n", subOff, subOn);
    CHECK(subOff < -80.0, "no sub should mean no 110 Hz (%.1f dB)", subOff);
    CHECK(subOn > -10.0, "the sub should be strong (%.1f dB)", subOn);

    // Everything on with a heavy, biased fold. The folder's DC follows the stack's
    // beating envelope; without the envelope-aware correction it leaks through
    // the DC blocker as a slow wobble (~0.25 V) and pushes the peaks up.
    Params all      = one;
    all.voices      = 4;
    all.spread      = 0.6f;
    all.timbre      = 0.6f;
    all.symmetry    = 0.9f;
    all.feedback    = 0.f;
    auto         y  = renderTone(all, 6);
    const double c  = 1.0 - std::exp(-2.0 * 3.14159265358979 * 5.0 / kFs); // 5 Hz low-pass
    double       lp = 0.0, wob = 0.0;
    float        peak = 0.f;
    for (float v : y) {
        lp += c * (v - lp);
        wob += lp * lp;
        peak = std::max(peak, std::fabs(v));
    }
    wob = std::sqrt(wob / static_cast<double>(y.size()));
    std::printf("  4 voices spread, biased fold: <5 Hz wobble %.3f V  peak %.2f V\n", wob, peak);
    CHECK(wob < 0.1, "stacked fold wobbles at the beat rate (%.3f V)", wob);
    CHECK(peak < 5.5f, "stacked fold peak %.2f V", peak);

    // Changing the voice count fades voices in: no click bigger than the wave's own steps.
    static ComplexOsc<4> osc;
    osc.init(kFs);
    Params p = flat;
    p.spread = 0.6f;
    osc.setParams(p);
    osc.snapSmoothers();
    float prev = 0.f, stepBefore = 0.f, stepAfter = 0.f, o, m;
    for (int i = 0; i < 48000; ++i) {
        if (i == 24000) {
            p.voices = 4;
            osc.setParams(p);
        }
        osc.process(0.f, o, m);
        if (i > 1000) (i < 24000 ? stepBefore : stepAfter) = std::max(i < 24000 ? stepBefore : stepAfter, std::fabs(o - prev));
        prev = o;
    }
    std::printf("  1 -> 4 voices: largest step %.3f V (before %.3f V)\n", stepAfter, stepBefore);
    CHECK(stepAfter < 1.5f * stepBefore, "adding voices clicked (%.3f V vs %.3f V)", stepAfter, stepBefore);
}

static void testLayers() {
    std::printf("patch.Init layers: shift, unison, frozen ratio lock\n");
    LayerSelect l;
    l.update(false, true);
    CHECK(l.layer == kNormal && l.ratioLock, "toggle up, no button: normal layer, locked");
    CHECK(l.update(true, true) && l.layer == kShift, "button + toggle up: shift layer");
    CHECK(l.update(true, false) && l.layer == kUnison, "flip toggle down while holding: unison layer");
    CHECK(l.ratioLock, "ratio lock must not change while the button is held");
    CHECK(l.update(true, true) && l.layer == kShift && l.ratioLock, "flip back up: shift layer again");
    CHECK(l.update(false, true) && l.layer == kNormal && l.ratioLock, "release with toggle up: still locked");
    l.update(false, false);
    CHECK(l.layer == kNormal && !l.ratioLock, "toggle down, no button: free mode");
    CHECK(l.update(true, false) && l.layer == kUnison && !l.ratioLock, "free mode + button: unison layer");
    CHECK(!l.update(true, false), "no change, no layer switch");
    std::printf("  ok\n");
}

static void testPickup() {
    std::printf("shift-layer pickup: no jumps on layer change\n");
    PickupKnob k;
    k.value = 0.3f; // stored parameter
    k.lock(0.8f);   // knob is physically at 0.8 when the layer becomes active
    CHECK(k.locked, "should lock when knob and value disagree");
    k.update(0.7f);
    CHECK(std::fabs(k.value - 0.3f) < 1e-6f, "moved before pickup (value %.3f)", k.value);
    k.update(0.31f); // approaching, not yet crossed (outside 0.02 window? no: 0.01 away)
    CHECK(!k.locked, "should pick up within the 0.02 window");
    k.update(0.5f);
    CHECK(std::fabs(k.value - 0.5f) < 1e-6f, "should track after pickup (value %.3f)", k.value);

    PickupKnob c; // crossing in one jump (fast twist)
    c.value = 0.5f;
    c.lock(0.1f);
    c.update(0.9f);
    CHECK(!c.locked && std::fabs(c.value - 0.9f) < 1e-6f, "should pick up when crossing (value %.3f)", c.value);

    PickupKnob same; // already matching: no lock
    same.value = 0.4f;
    same.lock(0.405f);
    CHECK(!same.locked, "should not lock when already matching");
    PickupKnob fr; // factory default: grab on first deliberate move
    fr.value = 0.f;
    fr.fresh = true;
    fr.lock(0.7f);
    fr.update(0.705f); // jitter: not a move
    CHECK(fr.locked && fr.value == 0.f, "fresh knob grabbed on jitter");
    fr.update(0.75f);
    CHECK(!fr.locked && std::fabs(fr.value - 0.75f) < 1e-6f, "fresh knob should grab on move (%.3f)", fr.value);
    fr.lock(0.2f); // after that it behaves like any other knob
    fr.update(0.3f);
    CHECK(fr.locked && std::fabs(fr.value - 0.75f) < 1e-6f, "touched knob must use crossing pickup");
    std::printf("  ok\n");
}

template <int OS>
static double benchNsPerSample(bool stacked = false) {
    static ComplexOsc<OS> osc;
    osc.init(kFs);
    Params p;
    p.fmIndex   = 0.5f;
    p.timbre    = 0.6f;
    p.amDepth   = 0.3f;
    p.timbreMod = 0.4f;
    if (stacked) {
        p.voices = 4;
        p.spread = 0.5f;
        p.drift  = 0.5f;
        p.sub    = 0.5f;
    }
    osc.setParams(p);
    const int      n    = 48000 * 4;
    volatile float sink = 0.f;
    auto           t0   = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i) {
        float a, b;
        osc.process(0.f, a, b);
        sink = sink + a + b;
    }
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / n;
}

// ---------------------------------------------------------------- renders
static void renderAll(const std::string& dir) {
    std::printf("renders\n");
    // Pitch sweep with heavy folding: aliasing shows up as lines falling in the spectrogram.
    auto foldSweep = [](float t, Params& p) {
        p.pitchV   = 5.5f * t;
        p.timbre   = 0.75f;
        p.symmetry = 0.2f;
    };
    render<1>(dir + "/fold_sweep_os1.wav", 6.f, foldSweep);
    render<4>(dir + "/fold_sweep_os4.wav", 6.f, foldSweep);

    // FM index sweep at a fixed, fairly high pitch (C6), ratio 1.5.
    auto fmSweep = [](float t, Params& p) {
        p.pitchV  = 2.f;
        p.ratio   = 1.5f;
        p.fmIndex = t;
    };
    render<1>(dir + "/fm_sweep_os1.wav", 6.f, fmSweep);
    render<4>(dir + "/fm_sweep_os4.wav", 6.f, fmSweep);

    // Steady tones for the alias metric in analyze.py (f0 known exactly).
    auto foldStatic = [](float, Params& p) {
        p.pitchV   = 3.5f;
        p.timbre   = 0.75f;
        p.symmetry = 0.2f;
    };
    render<1>(dir + "/fold_static_os1.wav", 2.f, foldStatic);
    render<4>(dir + "/fold_static_os4.wav", 2.f, foldStatic);
    auto fmStatic = [](float, Params& p) {
        p.pitchV  = 3.f;
        p.ratio   = 1.5f;
        p.fmIndex = 1.f;
    };
    render<1>(dir + "/fm_static_os1.wav", 2.f, fmStatic);
    render<4>(dir + "/fm_static_os4.wav", 2.f, fmStatic);

    // A small musical demo: a minor-pentatonic sequence with evolving timbre.
    render<4>(dir + "/demo.wav", 12.f, [](float t, Params& p) {
        static const float notes[] = {0, 3, 5, 7, 10, 12, 10, 7, 5, 3, 0, -2, 0, 7, 12, 15};
        int                step    = static_cast<int>(t * 48) % 16;
        p.pitchV                   = -1.f + notes[step] / 12.f;
        p.ratio                    = (t < 0.5f) ? 2.f : 1.5f;
        p.fmIndex                  = 0.15f + 0.25f * (0.5f + 0.5f * std::sin(t * 6.283f * 2.f));
        p.timbre                   = 0.2f + 0.6f * t;
        p.symmetry                 = 0.3f * std::sin(t * 6.283f * 3.f);
        p.timbreMod                = t > 0.66f ? 0.5f : 0.f;
        p.amDepth                  = 0.f;
        p.modShape                 = ModShape::Sine;
    });

    // New evil: feedback sweep, chaos sweep.
    render<4>(dir + "/feedback_sweep.wav", 8.f, [](float t, Params& p) {
        p.pitchV   = -1.f;
        p.feedback = t;
    });
    render<4>(dir + "/chaos_sweep.wav", 8.f, [](float t, Params& p) {
        p.pitchV  = -1.f;
        p.ratio   = 1.f;
        p.fmIndex = 0.35f;
        p.chaos   = t;
    });
}

// ---------------------------------------------------------------- examples
// Six presets, each written as patch.Init knob positions (0..1 = 7 o'clock..5 o'clock)
// so they can be dialed in by hand. See README "Six evil patches".
struct Example {
    const char*                         file;
    const char*                         name;
    std::function<void(float, Params&)> automate;
};

static void renderExamples(const std::string& dir) {
    std::printf("examples\n");
    // Every value below is reachable on patch.Init: routing values match the
    // button presets exactly, and motion is a knob turn or a patched CV.
    const Example examples[] = {
        {"ex1_reactor_hum.wav", "Reactor hum",
         [](float t, Params& p) { // low drone; rock shift-K1 to make the feedback breathe
             p.pitchV   = -2.f;
             p.ratio    = 0.5f;
             p.fmIndex  = 0.12f;
             p.timbre   = 0.35f;
             p.feedback = 0.55f + 0.25f * std::sin(t * 6.283f * 1.5f);
             p.symmetry = 0.3f; // routing 0: FM only
         }},
        {"ex2_lorenz_growl.wav", "Lorenz growl",
         [](float t, Params& p) { // chaotic modulator in audio range: an engine that won't idle
             p.pitchV    = -1.5f;
             p.ratioLock = false;
             p.modPitchV = -1.2f;
             p.modShape  = ModShape::Lorenz;
             p.fmIndex   = 0.3f + 0.2f * t;
             p.timbre    = 0.4f;
             p.timbreMod = 0.6f; // routing 1: FM + timbre mod
         }},
        {"ex3_chaos_bells.wav", "Chaos bells",
         [](float t, Params& p) { // sequencer -> CV_5, decaying envelope -> CV_6, chaos rises
             static const float notes[] = {0, 7, 3, 10, 5, 12, 1, 8};
             float              step    = t * 16.f;
             int                i       = static_cast<int>(step) % 8;
             float              env     = std::exp(-5.f * (step - std::floor(step)));
             p.pitchV                   = notes[i] / 12.f;
             p.ratio                    = 3.f;
             p.fmIndex                  = 0.5f * env;
             p.chaos                    = 0.2f + 0.6f * t;
             p.timbre                   = 0.1f;
             p.amDepth                  = 0.5f; // routing 2: FM + AM
         }},
        {"ex4_feedback_scream.wav", "Feedback scream",
         [](float t, Params& p) { // pitch dives while feedback pushes past saw into noise
             p.pitchV   = 2.5f - 4.f * t;
             p.feedback = 0.6f + 0.4f * t;
             p.fmIndex  = 0.2f;
             p.ratio    = 1.5f;
             p.timbre   = 0.6f;
             p.symmetry = -0.4f; // routing 0
         }},
        {"ex5_possession.wav", "Possession",
         [](float t, Params& p) { // everything on; flip the modulator to Lorenz halfway
             p.pitchV    = -1.f;
             p.ratio     = 2.f;
             p.fmIndex   = 0.45f;
             p.feedback  = 0.5f;
             p.chaos     = 0.7f * t;
             p.timbre    = 0.3f + 0.5f * t;
             p.symmetry  = 0.5f;
             p.amDepth   = 1.f;
             p.timbreMod = 0.3f; // routing 3: FM + ring
             p.modShape  = (t < 0.5f) ? ModShape::Square : ModShape::Lorenz;
         }},
        {"ex6_legion.wav", "Legion",
         [](float t, Params& p) { // four drifting voices, a sub rising underneath, the fold opening
             p.pitchV   = -2.f;
             p.ratio    = 1.f;
             p.fmIndex  = 0.15f;
             p.feedback = 0.2f;
             p.timbre   = 0.25f + 0.35f * t;
             p.voices   = 4;
             p.spread   = 0.4f;
             p.drift    = 0.5f;
             p.sub      = 0.7f * t; // routing 0
         }},
    };
    for (const Example& e : examples) {
        std::printf("  %s\n", e.name);
        render<4>(dir + "/" + e.file, 10.f, e.automate);
    }
}

int main(int argc, char** argv) {
    std::string out = argc > 1 ? argv[1] : "out";
    testPitchTracking();
    testStabilityUnderAbuse();
    testThroughZero();
    testFeedback();
    testChaos();
    testLorenz();
    testChaosLongRun();
    testUnison();
    testLayers();
    testPickup();
    std::printf("benchmark (host CPU, per output sample)\n");
    std::printf("  OS=1: %6.1f ns   OS=2: %6.1f ns   OS=4: %6.1f ns\n", benchNsPerSample<1>(), benchNsPerSample<2>(), benchNsPerSample<4>());
    std::printf("  OS=4, 4 voices + sub: %6.1f ns\n", benchNsPerSample<4>(true));
    renderAll(out);
    renderExamples(out);
    std::printf(g_failures ? "\n%d FAILURE(S)\n" : "\nall tests passed\n", g_failures);
    return g_failures ? 1 : 0;
}
