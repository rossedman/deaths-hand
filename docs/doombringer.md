# Doombringer

<figure class="diagram" markdown>
![The Doombringer panel: the cutoff and the three macro knobs on top under their sigils, four settings knobs below, the bass switch, the envelope light and the ping button, eight inputs, the hand mark, and three outputs on a red altar, all over a faint summoning circle.](assets/doombringer-panel.png){ width="360" }
</figure>

A four-pole ladder filter that summons what isn't there. It lives in the same repository, the same VCV plugin and the same portable DSP style as Death's Hand, and it reads the same way: the cutoff and three character macros on the front, four settings on the patch.Init's shift layer.

Under the macros it is a careful ladder. The feedback loop is solved without a sample of delay, so it self-oscillates in tune from 33 Hz to 8 kHz (within 0.06 cents) and you can play it as an oscillator. A morph knob moves through five responses: four-pole low-pass, two-pole low-pass, band-pass, two-pole high-pass, four-pole high-pass. The extra parts:

- **WRATH** is the resonance and drive: clean, then ringing, then it screams on its own, then the input stage squares off what you feed it.
- **RUIN** corrupts the resonance: the feedback path folds, the output's own level bends the cutoff, and at the top the loop is pushed past the resonance knob until it breaks free of the input.
- **DOOM** summons undertones: it hears the input's pitch and mixes in squares an octave and two octaves below, which follow the input's envelope and die with it.
- **HUNGER** (shift layer) feeds the input's envelope to the cutoff, opening it on loud notes or, turned the other way, closing it.
- **PING** strikes the filter with a half-cycle at the cutoff, so it rings at the same level at any pitch: a drum with no input at all.

## Reading the panel

There are no words on the panel, only sigils. The top row is the voice: the cutoff and the three macros. Below them are the settings the patch.Init keeps on its shift layer. Each sigil repeats on the jack that modulates it, so the panel reads in columns. Sigils shared with Death's Hand mean the same thing on both.

| Sigil | Means |
|---|---|
| Eclipse (a crescent of shadow) | Cutoff · 1V/oct input |
| Flame | **WRATH** macro · its CV |
| Cracked ring | **RUIN** macro · its CV |
| Falling chevrons | **DOOM** macro · its CV |
| Serpent | Morph: LP4 → LP2 → BP → HP2 → HP4 |
| Spiral | FM depth · FM input |
| Maw | **HUNGER** · the envelope light · the envelope output (red) |
| Slit eye | Fine tune |
| Trident | Bass compensation. The red arrow is up = compensated |
| Lightning bolt | Ping: the button and its trigger input |
| Chalice | Audio input |
| Pillars and lightning | Restart the undertones |
| Blood drop (solid) | Filter out (the morph) |
| Blood drop with four poles | Four-pole low-pass out |

## The macros

Each macro sweeps several parameters, each over its own stretch of the knob. ±5 V at a macro's jack sweeps its whole range.

| Macro | Sweeps | Along the knob |
|---|---|---|
| **WRATH** | resonance, drive | Clean to ringing through the lower 60%. The edge of self-oscillation is at 0.64; above it the filter screams on its own at 4.5 to 5 V. From halfway, a drive stage in front of the ladder squares the input off |
| **RUIN** | feedback fold, self-FM, loop overdrive | The lower half folds the resonance back on itself (metallic) without adding gain, so a ping still dies away. From 0.25 the output's level raises the cutoff: pings fall in pitch as they ring out, and a scream climbs. From 0.55 the loop's gain rises to 1.6× the resonance knob's, and the filter fights the input for the pitch |
| **DOOM** | undertones, lurch | The octave below arrives first, full by 0.35. The second octave below comes in from 0.35 to 0.7. From 0.7 the undertone swings the cutoff by up to 1.5 octaves, so the filter lurches on every other note |

**The input is RUIN's leash.** With RUIN in its upper half, a quiet input (around 1 V) lets the corrupted resonance break free: its pitch and the input's beat against each other and the spectrum goes inharmonic. A hot input (5 V) drags it back into step and the result stays harmonic. Ride the input level, or put a VCA in front, and you play the leash.

## patch.Init control map

Four knobs, two layers. **Hold the button** to reach the shift layer. A parameter only moves once its knob passes through the stored value (soft takeover), so switching layers never makes the sound jump.

| Knob | Front | Hold button (shift) |
|---|---|---|
| 1 (CV_1) + jack CV_5 | **Cutoff**, ten octaves, 16 Hz to 16.7 kHz. Jack is 1V/oct | **Morph**: LP4 → LP2 → BP → HP2 → HP4, continuous |
| 2 (CV_2) + jack CV_6 | **WRATH** | **FM depth** for audio IN R (noon = none; ±4 octaves per 5 V at the ends) |
| 3 (CV_3) + jack CV_7 | **RUIN** | **HUNGER** (noon = none; clockwise the envelope opens the filter, counter-clockwise it closes it) |
| 4 (CV_4) + jack CV_8 | **DOOM** | **Fine tune**, ±1 semitone (noon = in tune) |

Jacks always drive the front-panel parameter.

| Other | Does |
|---|---|
| Button tap | Ping |
| Toggle | Up = bass compensated (the passband holds its level as resonance rises), down = thin (it loses up to 14 dB, like the original ladder) |
| Audio IN L | Audio in |
| Audio IN R | Cutoff FM, audio rate, scaled by the FM depth |
| Gate 1 / Gate 2 | Ping / restart the undertones (both flip-flops high, so they line up with a beat) |
| Audio OUT L / R | Filter (the morph) / four-pole low-pass |
| CV OUT 1 | The input's envelope, 0 to 5 V (the one HUNGER and DOOM follow) |
| CV OUT 2 / LED | The envelope; full while shift is held |

To flash it, follow [Getting started](getting-started.md) with `doombringer.bin`, or from source:

```sh
make firmware                          # both programs: hw/build/deaths-hand.bin and hw/build/doombringer.bin
make -C hw APP=doombringer program-dfu # after BOOT + RESET
```

The calibration constants at the top of `hw/doombringer.cpp` are the same as Death's Hand's (see [Calibration](calibration.md)); copy your numbers across.

## VCV Rack

Doombringer is the second module in the Death's Hand plugin, so installing the plugin installs both. The panel mirrors the patch.Init, and knobs and CVs go through the same `doom::panelToParams` as the firmware.

- No shift layer: morph, FM depth, HUNGER and fine tune each have a knob.
- Ping is a button with a trigger input next to it, and the undertones' restart has its own trigger input.
- Bass compensation is a switch (the trident). Up is compensated.
- The envelope drives a red light and an output jack.
- The FM input has its own jack, scaled by the FM depth knob.

## Five patches

Knob positions are clock faces: 7:00 is fully counter-clockwise, 12:00 is centre, 5:00 is fully clockwise. "S1"–"S4" means hold the button and turn knob 1–4. Untouched shift knobs stay at their defaults: LP4, no FM, no HUNGER, in tune. Each one is rendered on the [listening page](listen.md#doombringer) through the firmware's own panel mapping.

### 1 · Acid rite

A sixteen-step bassline that bites. Patch a sequenced saw through a VCA with a short, snappy envelope into IN L. Cutoff 10:50, WRATH 12:30, S3 (HUNGER) 3:30. Rock the cutoff slowly between 9:40 and 12:00, and halfway through bring **RUIN to 10:00**.

### 2 · Pitch-drop kicks

Nothing in the input: the filter is the drum. A clock into **Gate 1**. Cutoff 9:05, WRATH 1:00, RUIN 11:10. Each ping starts high and falls as it rings out. Turn WRATH up for longer tails, RUIN up for a deeper drop.

### 3 · The scream

Nothing in the input: the filter is the oscillator. A sequencer into **CV_5**. Cutoff 11:00, WRATH 2:30, S1 (morph) 9:30, for LP2. Sweep **RUIN from 8:30 to 1:30**: the tone grows harmonics, then climbs as its own level bends the cutoff.

### 4 · Undertow

A slow riff dragged down an octave and two. A saw sequence at A2 through a VCA with a slow envelope into IN L. Cutoff 12:00, WRATH 11:30, DOOM 3:30, S3 (HUNGER) 1:30.

### 5 · Death's Hand into Doombringer

Death's Hand at C2 (FLESH 12:30, MALICE 10:30) into IN L. WRATH 1:00, DOOM 10:00. Sweep the **cutoff from 10:30 to 2:30 and back**, slowly, while **RUIN rises from 9:00 to 12:30**.

## How it works

```text
                               ┌──── RUIN: the output's level raises the cutoff ◄────┐
                               ▼                                                      │
 in ─► + ─► 4× up ─► drive ─► (−) ─► clip ─► pole ─► pole ─► pole ─► pole ─┬─► taps ─► morph ─► 4× down ─► OUT
       ▲                       ▲                                           │                             └─► LP4
       │                       └──── resonance ◄── RUIN: the feedback folds ┘
       │
 DOOM: the input's pitch divided by 2 and 4, enveloped, mixed back in as undertones
```

The DSP is `dsp/Doombringer.hpp`; the macros and the panel mapping are `dsp/DoomPanel.hpp`.

**A ladder in tune.** Each of the four poles is a trapezoidal (TPT) one-pole, and the loop around them is solved exactly each sample, with no unit delay in the feedback. The cutoff is prewarped with a Padé approximation of the tangent (7×10⁻⁷ relative error at 20 kHz), so resonance peaks, and self-oscillation, land on the cutoff at every pitch.

**A scream at one level.** The input stage clips, which is what bounds self-oscillation. It is linear to half its clip level, so the scream barely grows the further past the edge the resonance goes. Each mode hears the loop through its own gain at the cutoff, so the clip level is set from that gain, and every mode screams at the same level.

**Five responses from four poles.** The morph mixes the input stage and the four pole outputs (Oberheim's trick): LP4 is the last pole, HP4 is the binomial `u − 4y₁ + 6y₂ − 4y₃ + y₄`, and the stops between are their neighbours. Bass compensation wins back the `1 / (1 + k)` the loop costs, on the low-pass taps only, since the high-pass ones never paid it.

**RUIN without lock-up.** A folded feedback path has stable states away from zero, and the filter would latch onto one as a stuck DC level. Only the AC part of the feedback folds; its DC stays linear. The self-FM follows the output's magnitude rather than its sign, so the loop stays symmetric and can't pump itself to one side either.

**DOOM's undertones.** A flip-flop on the input's rising zero crossings is a square an octave down; a second flip-flop on the first is two octaves down. The detector hears the fundamental through two 600 Hz poles, with hysteresis that scales with the envelope, so quiet tails can't chatter it. An edge lands between samples, so that sample takes the average of the two levels across it instead of a hard jump. The squares are scaled by the input's envelope, so silence stays silent, and rounded above 1.2 kHz.

**HUNGER's envelope** is a peak follower with a 1 ms attack and an 80 ms release. At full HUNGER a 5 V envelope moves the cutoff five octaves.

**Drive** is a separate saturator in front of the ladder, `tanh(D·x) / tanh(D)`: a 5 V peak stays 5 V and everything quieter is pushed up toward it, so turning it up adds harmonics, not level.

**PING** is a half-cycle of a sine at the cutoff. A short click would ring louder at high cutoffs than low ones; a half-cycle at the ringing frequency puts the same energy where it rings, at any pitch.

**Oversampling.** The ladder runs at 4×. A filter has to process what comes in, so unlike the oscillator it upsamples first, with a polyphase version of the same 129-tap Kaiser FIR that decimates on the way out. The FM input has its own upsampler, which idles while the FM depth is at noon.

**Waking up.** A whisper of noise (−94 dB) lets self-oscillation start on its own. It grows from there over a few hundred cycles, so at a low cutoff it can take a couple of seconds. Ping it to wake it at once.

## Measured

Every number here is printed by `test/doom_tests.cpp` (`make test`), which runs the same DSP core the firmware and the plugin use.

| | |
|---|---|
| Five responses against the analog shapes, at fc/4, fc and 4fc | within 0.01 dB |
| Self-oscillation, 1V/oct, −3 V … +5 V | worst error 0.06 cents |
| Self-oscillation level, every mode, resonance 1.01 … 1.1 | 3.8 … 5.8 V |
| Passband at resonance 0.95 | 0.0 dB compensated, −13.6 dB thin |
| Ping, 16 Hz … 8 kHz, resonance 0.8 … 0.99 | 4.3 … 4.9 V |
| Drive 0 / 0.5 / 1, 5 V sine | peak 5.00 V; third harmonic −75 / −12 / −10 dB |
| DOOM at full, 220 Hz saw | −1 octave +6 dB, −2 octaves +7 dB re the fundamental; silent 0.5 s after the input stops |
| HUNGER −1 / 0 / +1: fourth harmonic | −52 / −21 / −12 dB re the fundamental |
| RUIN 0.9 into a 1 V / 5 V saw: inharmonic share | 0.91 / 0.0002 |
| 60 s, every macro at full, 4 cutoffs, with and without input | finite, alive, DC under 0.17 V |
| patch.Init flash / RAM | 107 KB of 128 KB / 37 KB (arm-gcc 13.3) |
