// ComplexOsc — a West-Coast style complex oscillator engine.
//
//        ┌──────────── chaos: primary FMs the modulator back ◄──┐
//        ▼                                                      │
//   modulator osc ──┬──────────────────────────────► mod out    │
//   (incl. Lorenz)  │ FM (linear, through-zero)                 │
//                   ├──► primary osc (sine core, up to 4 detuned voices + sub) ◄─ self-feedback PM
//                   │        │ ─────────────────────────────────┘
//                   ├──► timbre mod ──► wavefolder (timbre / symmetry)
//                   │                     │
//                   └──► AM / ring ──────►×──► DC block ──► primary out
//
// Portable: no heap, no exceptions, no platform headers. Volts in, volts out.
// Everything runs at Oversample x the host rate and is decimated at the end.
#pragma once

#include <initializer_list>

#include "Decimator.hpp"
#include "Folder.hpp"
#include "Math.hpp"
#include "Oscillator.hpp"

namespace cosc {

struct Params {
    float    pitchV    = 0.f;  // primary pitch, 1V/oct, 0 V = C4
    float    modPitchV = 0.f;  // modulator pitch when ratioLock == false
    bool     ratioLock = true; // true: modulator = primary * ratio (harmonic FM)
    float    ratio     = 1.f;  // modulator/primary frequency ratio
    float    fmIndex   = 0.f;  // 0..1 -> 0..kMaxFmIndex
    float    amDepth   = 0.f;  // 0 = none, 0.5 = classic AM, 1 = ring mod
    float    timbre    = 0.f;  // 0..1 wavefold amount
    float    symmetry  = 0.f;  // -1..1 folder bias (even harmonics)
    float    timbreMod = 0.f;  // 0..1 modulator -> timbre depth
    float    feedback  = 0.f;  // 0..1 primary self-feedback (sine -> saw-ish -> noise)
    float    chaos     = 0.f;  // 0..1 primary -> modulator cross-FM
    ModShape modShape  = ModShape::Sine;
    int      voices    = 1;   // 1..4 unison voices in the primary stack
    float    spread    = 0.f; // 0..1 detune between voices (squared: fine near zero)
    float    drift     = 0.f; // 0..1 slow chaotic wander of each extra voice's tuning
    float    sub       = 0.f; // 0..1 level of a sine one octave below, into the folder
};

template <int Oversample = 4, int DecimTaps = 129>
class ComplexOsc {
  public:
    static constexpr float kMaxFmIndex = 8.f; // peak deviation = index * carrier freq
    static constexpr float kOutVolts   = 5.f; // +-5 V, Eurorack audio standard
    static constexpr float kMaxHz      = 20000.f;
    // Self-feedback depth in turns of phase. ~0.2 gives a saw-like spectrum,
    // the top of the range tips into noise. Knob is squared for finer control.
    static constexpr float kMaxFeedback = 0.42f;
    // Cross-FM depth from primary into the modulator (same units as FM index).
    static constexpr float kMaxChaos = 4.f;
    // Unison: up to four primary voices. Spread detunes the extra voices by
    // fixed, non-repeating fractions of kMaxSpreadCents; drift adds a slow
    // chaotic wander (a Lorenz attractor at kDriftHz) on top.
    static constexpr int   kMaxVoices      = 4;
    static constexpr float kMaxSpreadCents = 70.f;
    static constexpr float kMaxDriftCents  = 25.f;
    static constexpr float kDriftHz        = 0.25f;

    void init(float sampleRate) {
        fs_    = sampleRate;
        fsInt_ = sampleRate * Oversample;
        decPrimary_.init();
        decMod_.init();
        folder_.init();
        dc_ = DcBlocker{};
        dc_.setSampleRate(fs_);
        sync();
        smPitch_.setTime(0.001f, fs_);
        smModPitch_.setTime(0.001f, fs_);
        for (Smoother* s : {&smIndex_, &smAm_, &smTimbre_, &smSym_, &smTmod_, &smRatio_, &smFb_, &smChaos_, &smSpread_, &smDrift_, &smSub_})
            s->setTime(0.01f, fs_);
        gainCoeff_ = 1.f - std::exp(-1.f / (0.01f * fs_)); // voices fade in and out over ~10 ms
        setParams(Params{});
        snapSmoothers();
    }

    // Set target values; call once per block (or per sample — it's cheap).
    void setParams(const Params& p) {
        p_         = p;
        mod_.shape = p.modShape;
    }

    // Jump smoothers to their targets (on init / patch recall) instead of gliding.
    void snapSmoothers() {
        smPitch_.reset(p_.pitchV);
        smModPitch_.reset(p_.modPitchV);
        smIndex_.reset(p_.fmIndex);
        smAm_.reset(p_.amDepth);
        smTimbre_.reset(p_.timbre);
        smSym_.reset(p_.symmetry);
        smTmod_.reset(p_.timbreMod);
        smRatio_.reset(p_.ratio);
        smFb_.reset(p_.feedback);
        smChaos_.reset(p_.chaos);
        smSpread_.reset(p_.spread);
        smDrift_.reset(p_.drift);
        smSub_.reset(p_.sub);
        for (int k = 1; k < kMaxVoices; ++k) gain_[k] = k < p_.voices ? 1.f : 0.f;
    }

    // Hard sync: restart both oscillators.
    // Every voice restarts in phase, so the stack hits together and then spreads.
    void sync() {
        primary_.reset();
        mod_.reset();
        sub_.reset();
        fb1_ = fb2_ = 0.f;
        for (Voice& v : voices_) v = Voice{};
    }

    // Render one output sample. extFmVolts is an optional external FM source
    // (+-5 V full scale) summed with the internal modulator.
    void process(float extFmVolts, float& primaryOutV, float& modOutV) {
        const float pitchV = smPitch_.process(p_.pitchV);
        const float modV   = smModPitch_.process(p_.modPitchV);
        const float index  = clampf(smIndex_.process(p_.fmIndex), 0.f, 1.f) * kMaxFmIndex;
        const float am     = clampf(smAm_.process(p_.amDepth), 0.f, 1.f);
        const float timbre = smTimbre_.process(p_.timbre);
        const float sym    = clampf(smSym_.process(p_.symmetry), -1.f, 1.f);
        const float tmod   = smTmod_.process(p_.timbreMod);
        const float ratio  = smRatio_.process(p_.ratio);
        const float fbK    = clampf(smFb_.process(p_.feedback), 0.f, 1.f);
        const float fbAmt  = fbK * fbK * kMaxFeedback;
        const float chK    = clampf(smChaos_.process(p_.chaos), 0.f, 1.f);
        const float chaos  = chK * chK * kMaxChaos; // squared: fine control near zero

        // Unison stack: per-voice gains glide so changing the voice count never clicks.
        // An inactive voice shadows voice 0, so it starts in phase when it fades in
        // and, with no spread or drift, adds nothing but itself.
        const int nVoices = p_.voices < 1 ? 1 : (p_.voices > kMaxVoices ? kMaxVoices : p_.voices);
        float     gainSum = 1.f;
        bool      stacked = false;
        for (int k = 1; k < kMaxVoices; ++k) {
            const float target = k < nVoices ? 1.f : 0.f;
            gain_[k] += gainCoeff_ * (target - gain_[k]);
            if (std::fabs(target - gain_[k]) < 1e-4f) gain_[k] = target;
            gainSum += gain_[k];
            stacked = stacked || gain_[k] > 0.f;
        }
        const float spreadK = clampf(smSpread_.process(p_.spread), 0.f, 1.f);
        const float driftK  = clampf(smDrift_.process(p_.drift), 0.f, 1.f);
        const float subL    = clampf(smSub_.process(p_.sub), 0.f, 1.f);
        if (stacked) {
            drifter_.process(kDriftHz / fs_);
            const float wander[kMaxVoices] = {0.f, drifter_.x * (1.f / 20.f), drifter_.y * (1.f / 27.f), (drifter_.z - 24.f) * (1.f / 20.f)};
            const float spreadC            = spreadK * spreadK * kMaxSpreadCents;
            const float driftC             = driftK * driftK * kMaxDriftCents;
            for (int k = 1; k < kMaxVoices; ++k) ratio_[k] = fastExp2((spreadC * kSpreadPattern[k] + driftC * wander[k]) * (1.f / 1200.f));
        }
        const bool  extra = stacked || subL > 0.f;
        const float norm  = 1.f / (gainSum + subL);

        const float fc = clampf(voltsToHz(pitchV), 0.05f, kMaxHz);
        const float fm = clampf(p_.ratioLock ? fc * ratio : voltsToHz(modV), 0.01f, kMaxHz);

        const float incC   = fc / fsInt_;
        const float incM   = fm / fsInt_;
        const float ext    = extFmVolts * (1.f / kOutVolts);
        const float maxInc = kMaxHz / fsInt_;

        float yP = 0.f, yM = 0.f;
        for (int i = 0; i < Oversample; ++i) {
            // Chaos: the primary's last output FMs the modulator (through-zero too),
            // so the two oscillators chase each other.
            // Clamp to +-20 kHz so heavy cross-FM can't push it far past the band limit.
            const float incX = clampf(incM * (1.f + chaos * fb1_), -maxInc, maxInc);
            const float m    = softLimit(mod_.process(incX));

            // Linear through-zero FM: increment may go negative.
            const float inc = incC * (1.f + index * (m + ext));
            // Self-feedback phase modulation. Averaging the last two outputs
            // suppresses the Nyquist-rate "hunting" that raw feedback causes.
            const float fbIn = 0.5f * (fb1_ + fb2_);
            const float ph0  = primary_.tick(inc) + fbAmt * fbIn;
            const float s    = sinTurns(ph0);
            fb2_             = fb1_;
            fb1_             = s;

            const float t = clampf(timbre + tmod * m * 0.5f, 0.f, 1.f);
            float       y;
            if (!extra) {
                y = folder_.process(s, t, sym);
            } else {
                // Sum the voices, and track the stack's envelope (|sum of phasors|)
                // so the folder can still remove its DC exactly.
                float sum = s, im = sinTurns(ph0 + 0.25f);
                for (int k = 1; k < kMaxVoices; ++k) {
                    Voice& v = voices_[k];
                    if (gain_[k] == 0.f) {
                        v.phasor.phase = primary_.phase;
                        v.fb1          = fb1_;
                        v.fb2          = fb2_;
                        continue;
                    }
                    const float ph = v.phasor.tick(inc * ratio_[k]) + fbAmt * 0.5f * (v.fb1 + v.fb2);
                    const float sk = sinTurns(ph);
                    v.fb2          = v.fb1;
                    v.fb1          = sk;
                    sum += gain_[k] * sk;
                    im += gain_[k] * sinTurns(ph + 0.25f);
                }
                const float sb = sinTurns(sub_.tick(0.5f * inc));
                const float x  = (sum + subL * sb) * norm;
                y              = folder_.process(x, t, sym, std::sqrt(sum * sum + im * im) * norm, subL * norm);
            }

            y *= 1.f + am * (m - 1.f); // 0: dry, 1: ring mod

            // Safety net only: the folder is DC-free and bounded by design, so
            // this stays transparent unless something upstream misbehaves.
            y = softLimit(y);

            float o;
            if (decPrimary_.push(y, o)) yP = o;
            if (decMod_.push(m, o)) yM = o;
        }

        primaryOutV = dc_.process(yP) * kOutVolts; // residual DC from FM / timbre mod
        modOutV     = yM * kOutVolts;
    }

    float sampleRate() const { return fs_; }

  private:
    Params p_;
    float  fs_ = 48000.f, fsInt_ = 48000.f * Oversample;

    Phasor                           primary_;
    ModOscillator                    mod_;
    Folder                           folder_;
    DcBlocker                        dc_;
    Decimator<Oversample, DecimTaps> decPrimary_, decMod_;

    Smoother smPitch_, smModPitch_, smIndex_, smAm_, smTimbre_, smSym_, smTmod_, smRatio_, smFb_, smChaos_, smSpread_, smDrift_, smSub_;
    float    fb1_ = 0.f, fb2_ = 0.f; // last two primary core outputs (feedback + chaos)

    // Unison voices 1..3 (voice 0 is primary_ above, so one voice is exactly the
    // original engine). Each has its own self-feedback history.
    struct Voice {
        Phasor phasor;
        float  fb1 = 0.f, fb2 = 0.f;
    };
    // Detune per voice as a fraction of the spread: symmetric-ish and never a
    // simple ratio, so the beating doesn't fall into a repeating pattern.
    static constexpr float kSpreadPattern[kMaxVoices] = {0.f, 1.f, -0.86f, 0.41f};
    Voice                  voices_[kMaxVoices];
    float                  gain_[kMaxVoices]  = {1.f, 0.f, 0.f, 0.f};
    float                  ratio_[kMaxVoices] = {1.f, 1.f, 1.f, 1.f};
    float                  gainCoeff_         = 1.f;
    Phasor                 sub_;
    LorenzOsc              drifter_;
};

// Musically useful FM ratios for a quantized "ratio" knob (harmonic spectra).
constexpr float kRatios[]  = {0.25f, 0.5f, 1.f, 1.5f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
constexpr int   kNumRatios = sizeof(kRatios) / sizeof(kRatios[0]);

inline float quantizeRatio(float knob01) {
    int i = static_cast<int>(clampf(knob01, 0.f, 0.9999f) * kNumRatios);
    return kRatios[i];
}

} // namespace cosc
