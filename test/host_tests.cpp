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

using namespace cosc;

static constexpr float kFs = 48000.f;
static int g_failures = 0;

#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            std::printf("  FAIL: " __VA_ARGS__);          \
            std::printf("\n");                            \
            ++g_failures;                                 \
        }                                                 \
    } while (0)

// ---------------------------------------------------------------- WAV writer
static void writeWav(const std::string& path, const std::vector<float>& l, const std::vector<float>& r, float fs) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { std::perror(path.c_str()); return; }
    const uint32_t n = static_cast<uint32_t>(l.size()), ch = 2, bits = 16;
    const uint32_t dataBytes = n * ch * bits / 8, rate = static_cast<uint32_t>(fs);
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); u32(36 + dataBytes); std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16); u16(1); u16(ch); u32(rate); u32(rate * ch * bits / 8); u16(ch * bits / 8); u16(bits);
    std::fwrite("data", 1, 4, f); u32(dataBytes);
    for (uint32_t i = 0; i < n; ++i)
        for (float v : {l[i], r[i]}) {
            // 8 V = full scale; the engine limits itself to about +-6.3 V.
            float s = clampf(v / 8.f, -1.f, 1.f);
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
    const int n = static_cast<int>(seconds * kFs);
    std::vector<float> l(n), r(n);
    Params p;
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
            if (first < 0) first = t; else ++count;
            last = t;
        }
    }
    return count > 0 ? count * fs / (last - first) : 0.0;
}

// ---------------------------------------------------------------- tests
static void testPitchTracking() {
    std::printf("pitch tracking (1V/oct, 0 V = C4)\n");
    static ComplexOsc<4> osc;
    double worst = 0;
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
    std::mt19937 rng(1234);
    std::uniform_real_distribution<float> u(0.f, 1.f);
    float peak = 0.f;
    bool  bad  = false;
    for (int block = 0; block < 2000; ++block) {
        Params p;
        p.pitchV    = -6.f + u(rng) * 14.f;   // way outside normal range
        p.modPitchV = -10.f + u(rng) * 20.f;
        p.ratioLock = u(rng) > 0.5f;
        p.ratio     = quantizeRatio(u(rng));
        p.fmIndex   = u(rng) * 1.5f;          // over-range on purpose
        p.amDepth   = u(rng);
        p.timbre    = u(rng) * 1.5f;
        p.symmetry  = -1.5f + u(rng) * 3.f;
        p.timbreMod = u(rng);
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
    // Limiter ceiling is 6.25 V; band-limiting a limited wave adds ~10% Gibbs overshoot.
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

template <int OS>
static double benchNsPerSample() {
    static ComplexOsc<OS> osc;
    osc.init(kFs);
    Params p;
    p.fmIndex = 0.5f; p.timbre = 0.6f; p.amDepth = 0.3f; p.timbreMod = 0.4f;
    osc.setParams(p);
    const int n = 48000 * 4;
    volatile float sink = 0.f;
    auto t0 = std::chrono::steady_clock::now();
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
    auto foldSweep = [](float t, Params& p) { p.pitchV = 5.5f * t; p.timbre = 0.75f; p.symmetry = 0.2f; };
    render<1>(dir + "/fold_sweep_os1.wav", 6.f, foldSweep);
    render<4>(dir + "/fold_sweep_os4.wav", 6.f, foldSweep);

    // FM index sweep at a fixed, fairly high pitch (C6), ratio 1.5.
    auto fmSweep = [](float t, Params& p) { p.pitchV = 2.f; p.ratio = 1.5f; p.fmIndex = t; };
    render<1>(dir + "/fm_sweep_os1.wav", 6.f, fmSweep);
    render<4>(dir + "/fm_sweep_os4.wav", 6.f, fmSweep);

    // Steady tones for the alias metric in analyze.py (f0 known exactly).
    auto foldStatic = [](float, Params& p) { p.pitchV = 3.5f; p.timbre = 0.75f; p.symmetry = 0.2f; };
    render<1>(dir + "/fold_static_os1.wav", 2.f, foldStatic);
    render<4>(dir + "/fold_static_os4.wav", 2.f, foldStatic);
    auto fmStatic = [](float, Params& p) { p.pitchV = 3.f; p.ratio = 1.5f; p.fmIndex = 1.f; };
    render<1>(dir + "/fm_static_os1.wav", 2.f, fmStatic);
    render<4>(dir + "/fm_static_os4.wav", 2.f, fmStatic);

    // A small musical demo: a minor-pentatonic sequence with evolving timbre.
    render<4>(dir + "/demo.wav", 12.f, [](float t, Params& p) {
        static const float notes[] = {0, 3, 5, 7, 10, 12, 10, 7, 5, 3, 0, -2, 0, 7, 12, 15};
        int   step   = static_cast<int>(t * 48) % 16;
        p.pitchV     = -1.f + notes[step] / 12.f;
        p.ratio      = (t < 0.5f) ? 2.f : 1.5f;
        p.fmIndex    = 0.15f + 0.25f * (0.5f + 0.5f * std::sin(t * 6.283f * 2.f));
        p.timbre     = 0.2f + 0.6f * t;
        p.symmetry   = 0.3f * std::sin(t * 6.283f * 3.f);
        p.timbreMod  = t > 0.66f ? 0.5f : 0.f;
        p.amDepth    = 0.f;
        p.modShape   = ModShape::Sine;
    });
}

int main(int argc, char** argv) {
    std::string out = argc > 1 ? argv[1] : "out";
    testPitchTracking();
    testStabilityUnderAbuse();
    testThroughZero();
    std::printf("benchmark (host CPU, per output sample)\n");
    std::printf("  OS=1: %6.1f ns   OS=2: %6.1f ns   OS=4: %6.1f ns\n",
                benchNsPerSample<1>(), benchNsPerSample<2>(), benchNsPerSample<4>());
    renderAll(out);
    std::printf(g_failures ? "\n%d FAILURE(S)\n" : "\nall tests passed\n", g_failures);
    return g_failures ? 1 : 0;
}
