// Soft takeover for knobs shared between two parameter layers.
#pragma once

#include <cmath>

namespace cosc {

// Each knob drives two parameters. When the layer changes, the knob's physical
// position no longer matches the parameter, so the parameter stays put until
// the knob passes through its stored value ("pickup"). No sudden jumps.
//
// Exception: a parameter still at its factory default ("fresh") is grabbed as
// soon as the knob is deliberately moved, so you don't have to hunt for zero
// the first time you reach for feedback.
struct PickupKnob {
    float value   = 0.f;
    float last    = 0.f;
    float lockPos = 0.f;
    bool  locked  = false;
    bool  fresh   = false;

    void lock(float raw) {
        last    = raw;
        lockPos = raw;
        locked  = std::fabs(raw - value) > 0.02f;
    }
    // Returns true if the knob moved the parameter this call.
    bool update(float raw) {
        bool moved = false;
        if (locked) {
            const bool crossed = (raw - value) * (last - value) <= 0.f;
            const bool grab    = fresh && std::fabs(raw - lockPos) > 0.02f;
            if (crossed || grab || std::fabs(raw - value) < 0.02f) locked = false;
        }
        if (!locked && std::fabs(raw - value) > 0.0005f) {
            moved = std::fabs(raw - value) > 0.01f;
            value = raw;
            fresh = false;
        }
        last = raw;
        return moved;
    }
};

// Which knob layer is live on the patch.Init, and the ratio-lock state.
//
//   button up                    -> normal layer; the toggle sets ratio lock
//   button held, toggle up       -> shift layer (feedback, chaos, symmetry, shape)
//   button held, toggle down     -> unison layer (voices, spread, drift, sub)
//
// While the button is held the toggle only picks the layer, so ratio lock is
// frozen at its value from before the press. Flip the toggle back before
// letting go to keep the mode you were in; on release it follows the toggle.
enum Layer { kNormal = 0, kShift = 1, kUnison = 2, kNumLayers = 3 };

struct LayerSelect {
    int  layer     = kNormal;
    bool ratioLock = true;

    // Returns true when the live layer changed this call.
    bool update(bool shiftHeld, bool toggleUp) {
        if (!shiftHeld) ratioLock = toggleUp;
        const int  next    = !shiftHeld ? kNormal : (toggleUp ? kShift : kUnison);
        const bool changed = next != layer;
        layer              = next;
        return changed;
    }
};

} // namespace cosc
