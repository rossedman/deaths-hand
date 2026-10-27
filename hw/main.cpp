// patch.Init() firmware for the complex oscillator.
// All the sound lives in ../dsp — this file only maps panel + jacks to Params.
//
// Panel map (see README for the full table)
//   CV_1 knob + CV_5 jack : primary pitch (knob = coarse, jack = 1V/oct)
//   CV_2 knob + CV_8 jack : ratio (toggle up) / modulator pitch (toggle down)
//   CV_3 knob + CV_6 jack : FM index
//   CV_4 knob + CV_7 jack : timbre (wavefold)
//   IN_L                  : external FM (audio rate, summed with modulator)
//   IN_R                  : symmetry CV (+-5 V)
//   GATE_IN_1             : hard sync
//   GATE_IN_2             : next routing mode (same as a short button press)
//   Button B7             : short press = next routing mode, hold = next mod shape
//   Toggle B8             : up = ratio-locked modulator, down = free modulator
//   OUT_L                 : primary out      OUT_R : modulator out
//   CV_OUT_1              : modulator as unipolar CV (0-5 V LFO in free mode)
//   CV_OUT_2 (LED)        : brightness shows the routing mode
#include "daisy_patch_sm.h"
#include "util/CpuLoadMeter.h"

#include "../dsp/ComplexOsc.hpp"

using namespace daisy;
using namespace patch_sm;

namespace {

// ---- hardware calibration -------------------------------------------------
// Volts that correspond to a full-scale (+-1.0) codec sample. Measure your
// unit with a scope or a VCV/other module and adjust. Too high = quieter (safe).
constexpr float kAudioFullScaleV = 9.f;
// The Patch SM's analog audio stages invert polarity; flip it back so a rising
// ramp out of the DSP is a rising ramp at the jack. Irrelevant for pure tones.
constexpr bool kInvertAudio = true;
// 1V/oct trim for CV_5. Fill these from a two-point calibration
// (send exactly 1 V and 3 V; libDaisy's VoctCalibration does the math).
constexpr float kPitchCvScale  = 1.f;
constexpr float kPitchCvOffset = 0.f;

constexpr uint32_t kLongPressMs = 600;

// ---- state ------------------------------------------------------------------
DaisyPatchSM hw;
Switch       button, toggle;

cosc::ComplexOsc<4> osc;
CpuLoadMeter        cpu; // build with CPU_METER=1 to print load over USB serial

// Routing presets cycled by the button / gate 2.
struct Routing {
    float amDepth;
    float timbreMod;
};
constexpr Routing kRoutings[] = {
    {0.f, 0.f},   // 0: FM only
    {0.f, 0.6f},  // 1: FM + modulator sweeps the folder
    {0.5f, 0.f},  // 2: FM + classic AM
    {1.f, 0.3f},  // 3: FM + ring mod + a little timbre mod
};
constexpr int kNumRoutings = sizeof(kRoutings) / sizeof(kRoutings[0]);

volatile int  routing  = 0;
volatile int  modShape = 0;
volatile bool gate2Was = false;

inline float knob(int idx) { return cosc::clampf(hw.GetAdcValue(idx), 0.f, 1.f); }
inline float cvVolts(int idx) { return hw.GetAdcValue(idx) * 5.f; } // CV_5..8: +-5 V

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    cpu.OnBlockStart();
    hw.ProcessAllControls();

    // Gate 2 rising edge advances routing (lets a sequencer switch modes).
    const bool g2 = hw.gate_in_2.State();
    if (g2 && !gate2Was) routing = (routing + 1) % kNumRoutings;
    gate2Was = g2;
    if (hw.gate_in_1.Trig()) osc.sync();

    cosc::Params p;
    const float pitchCv = cvVolts(CV_5) * kPitchCvScale + kPitchCvOffset;
    p.pitchV            = (knob(CV_1) * 6.f - 3.f) + pitchCv; // knob spans 6 octaves around C4

    p.ratioLock = toggle.Pressed();
    if (p.ratioLock) {
        p.ratio = cosc::quantizeRatio(knob(CV_2) + cvVolts(CV_8) * 0.1f);
    } else {
        // -8 V .. +3 V: about 1 Hz (LFO) up to C7. CV_8 is 1V/oct on top.
        p.modPitchV = (knob(CV_2) * 11.f - 8.f) + cvVolts(CV_8);
    }

    p.fmIndex  = knob(CV_3) + cvVolts(CV_6) * 0.2f;
    p.timbre   = knob(CV_4) + cvVolts(CV_7) * 0.2f;

    const Routing& r = kRoutings[routing];
    p.amDepth   = r.amDepth;
    p.timbreMod = r.timbreMod;
    p.modShape  = static_cast<cosc::ModShape>(modShape);

    const float inSign = kInvertAudio ? -1.f : 1.f;
    // Symmetry from IN_R, read once per block (it's a CV, not audio).
    p.symmetry = in[1][0] * inSign * kAudioFullScaleV * 0.2f;

    osc.setParams(p);

    const float outScale = (kInvertAudio ? -1.f : 1.f) / kAudioFullScaleV;
    float       lastMod  = 0.f;
    for (size_t i = 0; i < size; ++i) {
        const float extFm = in[0][i] * inSign * kAudioFullScaleV;
        float       primary, mod;
        osc.process(extFm, primary, mod);
        out[0][i] = primary * outScale;
        out[1][i] = mod * outScale;
        lastMod   = mod;
    }

    // CV outs are 0..5 V. Modulator as unipolar CV, LED shows routing.
    hw.WriteCvOut(CV_OUT_1, cosc::clampf(lastMod * 0.5f + 2.5f, 0.f, 5.f));
    hw.WriteCvOut(CV_OUT_2, 1.25f * static_cast<float>(routing) + 0.3f);
    cpu.OnBlockEnd();
}

} // namespace

int main() {
    hw.Init();
    hw.SetAudioBlockSize(48); // 1 ms blocks at 48 kHz
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    button.Init(DaisyPatchSM::B7);
    toggle.Init(DaisyPatchSM::B8);

    osc.init(hw.AudioSampleRate());
    cpu.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
#if CPU_METER
    hw.StartLog(false); // open a serial monitor on the USB port to read it
#endif
    hw.StartAudio(AudioCallback);

    uint32_t heldMs = 0, lastLog = 0;
    bool     longFired = false;
    while (true) {
        button.Debounce();
        toggle.Debounce();

        if (button.Pressed()) {
            heldMs = static_cast<uint32_t>(button.TimeHeldMs());
            if (!longFired && heldMs >= kLongPressMs) {
                modShape  = (modShape + 1) % static_cast<int>(cosc::ModShape::Count);
                longFired = true;
            }
        }
        if (button.FallingEdge()) {
            if (!longFired) routing = (routing + 1) % kNumRoutings;
            longFired = false;
        }
#if CPU_METER
        if (System::GetNow() - lastLog > 1000) {
            lastLog = System::GetNow();
            hw.PrintLine("cpu avg " FLT_FMT3 "%%  max " FLT_FMT3 "%%",
                         FLT_VAR3(cpu.GetAvgCpuLoad() * 100.f), FLT_VAR3(cpu.GetMaxCpuLoad() * 100.f));
        }
#else
        (void)lastLog;
#endif
        System::Delay(1);
    }
}
