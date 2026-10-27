# How it works

Two oscillators, one portable DSP core. The modulator shapes the primary through through-zero FM, a wavefolder and AM or ring mod, and three feedback paths let it misbehave. Everything runs 4× oversampled and is decimated with a 129-tap Kaiser FIR.

<figure class="diagram" markdown>
![Signal flow. V/OCT drives the primary oscillator; MOD V/OCT drives the modulator, whose shape is sine, triangle, saw, square or a Lorenz attractor. The modulator feeds the primary through linear through-zero FM, sets the wavefolder's timbre and the AM or ring depth through knob-controlled paths, and goes straight to MOD OUT. The primary runs through the wavefolder, then AM or ring mod, then the 4x decimator to OUT. Two feedback paths: the primary phase-modulates itself (self-feedback), and the primary frequency-modulates the modulator (chaos).](assets/how-it-works.png)
</figure>

## Layout

```text
dsp/      portable engine: no heap, no platform headers, volts in / volts out
hw/       Electrosmith patch.Init() firmware (libDaisy)
vcv/      VCV Rack 2 plugin
test/     host tests, WAV renders, aliasing analysis
```

The firmware, the plugin and the host tests all include `dsp/` unchanged, so what the tests measure is what the module plays.

## Design notes

**Wavefolder DC.** Symmetry biases a sine folder, which creates DC. A high-pass alone re-centres the wave and pushes peaks to about ±8 V. Because the core is a sine, the DC is exactly `sin(a·b)·J0(a)`, so it is subtracted analytically (a small J0 table), with no limiter and no added aliasing.

**Through-zero FM.** The primary phase increment is allowed to go negative. The phasor wraps in both directions.

**Self-feedback** is phase modulation by the primary's own output, averaged over the last two samples (the DX7 trick). Without the average, feedback "hunts" at Nyquist and turns to hiss far too early.

**Chaos** feeds the primary's output into the modulator's frequency, clamped to ±20 kHz so heavy settings can't push it far past the band limit.

**Unison** stacks up to four primary voices before the wavefolder, so the fold intermodulates them. Every voice gets the same through-zero FM and its own self-feedback history. An idle voice shadows voice 0, so it fades in exactly in phase and only spread or drift pull it away; with no spread, four voices sound like one. Detuned sines this close look like one sine with a beating amplitude A, so the folder's DC becomes `sin(a·b)·J0(a·A)`, with A tracked per sample from the voices' phasors. Without that, the beat leaks through the DC blocker as a slow wobble (0.26 V against 0.05 V in the host tests). A sub one octave down adds its own J0 term.

**Lorenz** is integrated with a midpoint (RK2) step capped at 0.02, with a reset guard, so no setting can make it run away or produce NaN.

**CPU.** Cost is dominated by two 129-tap decimators. If the CPU meter says you need headroom, cascaded half-band filters are the next optimization.
