# patch.Init control map

Four knobs, two layers: the front is pitch and the three [character macros](panel.md#the-macros); the shift layer holds the settings. **Hold the button** to reach the shift layer. A parameter only moves once its knob passes through the stored value (soft takeover), so switching layers never makes the sound jump.

| Knob | Front | Hold button (shift) |
|---|---|---|
| 1 (CV_1) + jack CV_5 | Pitch, 6 octaves around C4. Jack is 1V/oct | **Ratio**, 11 steps (0.25, 0.5, 1, 1.5, 2, 3 … 8). With the toggle down: free modulator pitch, ~1 Hz LFO up to C7 |
| 2 (CV_2) + jack CV_6 | **FLESH** | **Modulator shape**: sine, triangle, saw, square, Lorenz (5 zones) |
| 3 (CV_3) + jack CV_7 | **MALICE** | **Symmetry** trim (noon = none), added to FLESH's |
| 4 (CV_4) + jack CV_8 | **LEGION** | **Fine tune**, ±1 semitone (noon = in tune) |

Jacks always drive the front-panel parameter: ±5 V sweeps a macro's whole range.

| Other | Does |
|---|---|
| Button tap | Next routing: FM → FM + timbre mod → FM + AM → FM + ring (LED gets brighter each step) |
| Toggle | Up = ratio-locked modulator, down = free |
| Audio IN L | External FM, audio rate, summed with the modulator |
| Audio IN R | Symmetry CV (±5 V), added to the trim |
| Gate 1 / Gate 2 | Hard sync / next routing |
| Audio OUT L / R | Primary / modulator |
| CV OUT 1 | Modulator as 0–5 V CV (an LFO in free mode, a chaotic one with Lorenz) |
| CV OUT 2 / LED | Routing brightness; full while shift is held |
