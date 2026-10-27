# complex-osc

A West-Coast style complex oscillator — two oscillators where the modulator
shapes the primary through **through-zero FM**, a **wavefolder** (timbre +
symmetry), and **AM / ring mod**. One portable DSP core, three targets:

```
dsp/      portable engine — no heap, no platform headers, volts in / volts out
hw/       Electrosmith patch.Init() firmware (libDaisy)
vcv/      VCV Rack 2 plugin
test/     host tests, WAV renders, aliasing analysis
```

```
modulator ──┬───────────────────────────────────────► MOD OUT
            │ linear through-zero FM
            ├──► primary (sine core)
            │        │
            ├──► timbre mod ─► wavefolder (timbre, symmetry; exact DC removal)
            │                    │
            └──► AM / ring ─────►× ─► 4× decimator ─► OUT
```

Everything runs 4× oversampled and is decimated with a 129-tap Kaiser FIR.

## Measured (host tests)

| | |
|---|---|
| 1V/oct tracking, −3 V … +5 V | worst error 0.09 cents |
| Alias energy, heavy fold @ 2.96 kHz | −9 dB → **−84 dB** with 4× |
| Alias energy, FM index 1.0 @ C7 | −40 dB → **−84 dB** with 4× |
| Output range | ±5 V nominal, DC-free, peaks < 6 V |
| patch.Init flash / RAM | 104 KB of 128 KB / 37 KB |

## Flash your patch.Init

### Fastest: prebuilt binary
1. Put the Patch SM into its bootloader: hold **BOOT**, tap **RESET**, release BOOT
   (both buttons are on the Patch SM board on the back of the module).
2. Open Electrosmith's **Daisy Web Programmer** in Chrome (it uses WebUSB), connect
   the module over USB, choose `complex-osc.bin` and flash.

### From source (macOS)
```sh
brew install --cask gcc-arm-embedded
brew install dfu-util
git submodule update --init --recursive     # pulls lib/libDaisy
make -C lib/libDaisy                        # once
make -C hw                                  # → hw/build/complex-osc.bin
make -C hw program-dfu                      # after BOOT + RESET
```

`make -C hw CPU_METER=1` builds a variant that prints DSP load once a second over
USB serial (`screen /dev/tty.usbmodem* 115200`). That's the number to watch as
the engine grows.

## patch.Init control map

| Control | Does |
|---|---|
| Knob 1 (CV_1) + jack CV_5 | Primary pitch. Knob spans 6 octaves around C4, jack is 1V/oct |
| Knob 2 (CV_2) + jack CV_8 | **Toggle up:** FM ratio (0.25, 0.5, 1, 1.5, 2 … 8). **Toggle down:** free modulator pitch, ~1 Hz LFO up to C7; jack is 1V/oct |
| Knob 3 (CV_3) + jack CV_6 | FM index |
| Knob 4 (CV_4) + jack CV_7 | Timbre (wavefold) |
| Audio IN L | External FM, audio rate, summed with the modulator |
| Audio IN R | Symmetry CV (±5 V) |
| Gate 1 | Hard sync (restarts both oscillators) |
| Gate 2 | Next routing mode |
| Button, short press | Next routing mode: FM → FM+timbre-mod → FM+AM → FM+ring |
| Button, hold 0.6 s | Next modulator shape: sine → triangle → saw → square |
| Toggle | Up = ratio-locked modulator, down = free |
| Audio OUT L / R | Primary / modulator |
| CV OUT 1 | Modulator as 0–5 V CV (an LFO in free mode) |
| CV OUT 2 / LED | Brightness shows the routing mode |

### Things to verify on your unit (top of `hw/main.cpp`)
- `kAudioFullScaleV`: the volts a full-scale codec sample produces. It's set
  conservatively, so if the output is quiet, measure it and lower the number.
- `kInvertAudio`: the Patch SM's audio stages invert polarity. It doesn't matter
  for tones, but it does for a ramp used as CV.
- `kPitchCvScale` / `kPitchCvOffset`: 1V/oct trim. Send exactly 1 V and 3 V into
  CV_5 and use libDaisy's `VoctCalibration` to get the two numbers.
- Toggle direction: if "up" turns out to be free mode, flip `p.ratioLock`.

## Host tests
```sh
make -C test analyze    # tests + renders + spectrograms (needs numpy/scipy/matplotlib)
```
Writes `test/out/*.wav` (including `demo.wav`) and `test/out/aliasing.png`.
The suite checks pitch accuracy, stability under abusive parameters and
external FM, through-zero behaviour, and output bounds.

## VCV Rack
```sh
# download the Rack 2 SDK from vcvrack.com and unzip it next to this repo
make -C vcv RACK_DIR=../../Rack-SDK install
```
The panel is generated: `python3 vcv/res/make_panel.py` writes both
`res/ComplexOsc.svg` and `src/PanelLayout.hpp` from one table, so the widget
positions can't drift from the artwork. It uses real Eurorack dimensions
(16 HP × 128.5 mm), so it's also a starting point for a physical panel.

## Design notes
- **Wavefolder DC:** symmetry biases a sine folder, which creates DC. A
  high-pass alone re-centres the wave and pushes peaks to about ±8 V. Because
  the core is a sine, the DC is exactly `sin(a·b)·J0(a)`, so it's subtracted
  analytically (a small J0 table), with no limiter and no added aliasing.
- **Through-zero FM:** the primary phase increment is allowed to go negative.
  The phasor wraps in both directions.
- **CPU:** cost is dominated by two 129-tap decimators. If the CPU meter says
  you need headroom, cascaded half-band filters are the next optimization.
