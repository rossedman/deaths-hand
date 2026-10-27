// patch.Init() firmware for the complex oscillator.
// All the sound lives in ../dsp — this file only maps panel + jacks to Params.
//
// Panel map (see README for the full table)
//                           normal                        | hold button (shift)
//   CV_1 knob + CV_5 jack : pitch (jack = 1V/oct)         | RATIO (toggle up) / MOD PITCH (down)
//   CV_2 knob + CV_6 jack : FLESH  (FM, fold, asymmetry)  | MOD SHAPE (5 zones, last = Lorenz)
//   CV_3 knob + CV_7 jack : MALICE (feedback, chaos)      | SYMMETRY trim (noon = 0)
//   CV_4 knob + CV_8 jack : LEGION (sub, voices, drift)   | FINE tune (noon = 0, +-1 semitone)
//   (jacks always drive the front-panel parameter)
//   IN_L        : external FM (audio rate)    IN_R : symmetry CV (+-5 V)
//   GATE_IN_1   : hard sync                   GATE_IN_2 : next routing mode
//   Button tap  : next routing mode           Toggle : up = ratio lock, down = free
//   OUT_L       : primary                     OUT_R : modulator
//   CV_OUT_1    : modulator as 0-5 V CV       CV_OUT_2 (LED) : routing / shift
#include "daisy_patch_sm.h"
#include "util/CpuLoadMeter.h"

#include "../dsp/ComplexOsc.hpp"
#include "../dsp/Panel.hpp"
#include "../dsp/Pickup.hpp"

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

// A tap shorter than this (with no shift knob moved) advances the routing.
constexpr uint32_t kTapMs = 400;

// ---- state ------------------------------------------------------------------
DaisyPatchSM hw;
Switch       button, toggle;

cosc::ComplexOsc<4> osc;
CpuLoadMeter        cpu; // build with CPU_METER=1 to print load over USB serial

using cosc::kNumRoutings;

enum Layer { kNormal = 0, kShift = 1 };
cosc::PickupKnob knobs[2][4]; // [layer][knob]

int      routing      = 0;
bool     gate2Was     = false;
bool     shiftWas     = false;
bool     shiftUsed    = false; // a shift knob was turned during this hold
uint32_t pressStartMs = 0;

inline float rawKnob(int i) { return cosc::clampf(hw.GetAdcValue(CV_1 + i), 0.f, 1.f); }
inline float cvVolts(int idx) { return hw.GetAdcValue(idx) * 5.f; } // CV_5..8: +-5 V

void handleButtonAndLayers() {
    button.Debounce();
    toggle.Debounce();

    const bool shift = button.Pressed();
    if (shift != shiftWas) {
        const int active = shift ? kShift : kNormal;
        for (int i = 0; i < 4; ++i) knobs[active][i].lock(rawKnob(i));
        if (shift) {
            pressStartMs = System::GetNow();
            shiftUsed    = false;
        } else if (!shiftUsed && System::GetNow() - pressStartMs < kTapMs) {
            routing = (routing + 1) % kNumRoutings; // it was a tap
        }
        shiftWas = shift;
    }

    const int active = shift ? kShift : kNormal;
    for (int i = 0; i < 4; ++i)
        if (knobs[active][i].update(rawKnob(i)) && shift) shiftUsed = true;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    cpu.OnBlockStart();
    hw.ProcessAllControls();
    handleButtonAndLayers(); // callback runs at 1 kHz, the debounce rate

    // Gate 2 rising edge advances routing (lets a sequencer switch modes).
    const bool g2 = hw.gate_in_2.State();
    if (g2 && !gate2Was) routing = (routing + 1) % kNumRoutings;
    gate2Was = g2;
    if (hw.gate_in_1.Trig()) osc.sync();

    const cosc::PickupKnob* n = knobs[kNormal];
    const cosc::PickupKnob* s = knobs[kShift];

    cosc::PanelState ps;
    ps.pitch             = n[0].value;
    ps.flesh             = n[1].value;
    ps.malice            = n[2].value;
    ps.legion            = n[3].value;
    ps.ratio             = s[0].value;
    ps.shape             = s[1].value;
    ps.symmetry          = s[2].value;
    ps.fine              = s[3].value;
    ps.ratioLock         = toggle.Pressed();
    ps.routing           = routing;
    ps.pitchCv           = cvVolts(CV_5) * kPitchCvScale + kPitchCvOffset;
    ps.fleshCv           = cvVolts(CV_6);
    ps.maliceCv          = cvVolts(CV_7);
    ps.legionCv          = cvVolts(CV_8);
    const float inSign   = kInvertAudio ? -1.f : 1.f;
    ps.symmetryCv        = in[1][0] * inSign * kAudioFullScaleV; // IN_R, volts
    const cosc::Params p = cosc::panelToParams(ps);
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

    // CV outs are 0..5 V. Modulator as unipolar CV. LED: full while shift is
    // held, otherwise its brightness shows the routing mode.
    hw.WriteCvOut(CV_OUT_1, cosc::clampf(lastMod * 0.5f + 2.5f, 0.f, 5.f));
    hw.WriteCvOut(CV_OUT_2, shiftWas ? 5.f : 1.25f * static_cast<float>(routing) + 0.3f);
    cpu.OnBlockEnd();
}

} // namespace

int main() {
    hw.Init();
    hw.SetAudioBlockSize(48); // 1 ms blocks at 48 kHz
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    button.Init(DaisyPatchSM::B7);
    toggle.Init(DaisyPatchSM::B8);

    // Front knobs start where they physically are. Shift settings start at sane
    // defaults (ratio 1:1 or a slow LFO, sine, no symmetry, in tune) and pick up
    // the first time you hold the button and sweep a knob through that value.
    hw.ProcessAllControls();
    for (int i = 0; i < 4; ++i) {
        knobs[kNormal][i].value = rawKnob(i);
        knobs[kNormal][i].last  = rawKnob(i);
    }
    knobs[kShift][0].value = cosc::PanelState{}.ratio; // the 1:1 step
    knobs[kShift][2].value = 0.5f;
    knobs[kShift][3].value = 0.5f;
    for (int i = 0; i < 4; ++i) knobs[kShift][i].fresh = true;

    osc.init(hw.AudioSampleRate());
    cpu.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
#if CPU_METER
    hw.StartLog(false); // open a serial monitor on the USB port to read it
#endif
    hw.StartAudio(AudioCallback);

    uint32_t lastLog = 0;
    while (true) {
#if CPU_METER
        if (System::GetNow() - lastLog > 1000) {
            lastLog = System::GetNow();
            hw.PrintLine("cpu avg " FLT_FMT3 "%%  max " FLT_FMT3 "%%", FLT_VAR3(cpu.GetAvgCpuLoad() * 100.f), FLT_VAR3(cpu.GetMaxCpuLoad() * 100.f));
        }
#else
        (void)lastLog;
#endif
        System::Delay(10);
    }
}
