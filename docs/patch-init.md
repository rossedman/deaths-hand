# patch.Init control map

Four knobs, three layers. **Hold the button** to reach the shift layer. A parameter only moves once its knob passes through the stored value (soft takeover), so switching layers never makes the sound jump.

## Knobs

| Knob | Normal | Hold button (shift) |
|---|---|---|
| 1 (CV_1) + jack CV_5 | Pitch, 6 octaves around C4. Jack is 1V/oct | **Feedback**: sine → saw → noise |
| 2 (CV_2) + jack CV_8 | **Toggle up:** FM ratio, 11 steps (0.25, 0.5, 1, 1.5, 2, 3 … 8). **Toggle down:** free modulator pitch, ~1 Hz LFO up to C7 | **Chaos**: primary FMs the modulator back |
| 3 (CV_3) + jack CV_6 | FM index | **Symmetry** (noon = none) |
| 4 (CV_4) + jack CV_7 | Timbre (wavefold) | **Modulator shape**: sine, triangle, saw, square, Lorenz (5 zones) |

Jacks always drive the normal-layer parameter.

## Unison layer

**Hold the button and flip the toggle down.** While the button is held the toggle only picks the layer, so the ratio lock doesn't change; flip it back before you let go to keep the mode you were in. The LED blinks on this layer.

| Knob | Hold button, toggle down |
|---|---|
| 1 | **Voices**: 1, 2, 3 or 4 (4 zones). Extra voices fade in over 10 ms |
| 2 | **Spread**: detune between voices, up to 70 cents. Squared, so the bottom of the knob is chorus and the top is out of tune |
| 3 | **Drift**: a slow Lorenz attractor wanders each extra voice's tuning, up to 25 cents |
| 4 | **Sub**: a sine one octave down, mixed into the wavefolder |

With one voice and no sub, the engine is bit-identical to the plain oscillator. In free mode (toggle down), holding the button goes straight to the unison layer; flip the toggle up while holding to reach feedback, chaos, symmetry and shape.

## Everything else

| Other | Does |
|---|---|
| Button tap | Next routing: FM → FM + timbre mod → FM + AM → FM + ring (LED gets brighter each step) |
| Toggle | Up = ratio-locked modulator, down = free |
| Audio IN L | External FM, audio rate, summed with the modulator |
| Audio IN R | Symmetry CV (±5 V), added to shift-knob 3 |
| Gate 1 / Gate 2 | Hard sync / next routing |
| Audio OUT L / R | Primary / modulator |
| CV OUT 1 | Modulator as 0–5 V CV (an LFO in free mode, a chaotic one with Lorenz) |
| CV OUT 2 / LED | Routing brightness; full on the shift layer, blinking on the unison layer |
