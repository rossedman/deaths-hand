# Changelog

All notable changes to this project are documented here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow [Semantic Versioning](https://semver.org/).

## 0.2.0

The VCV plugin is version `2.1.0`.

### Added

- **Doombringer**, a four-pole ladder filter, as a second VCV module in the same plugin and a second patch.Init program (`doombringer.bin`), built from the same DSP core style. The loop is solved with no unit delay, so it self-oscillates in tune (0.06 cents from 33 Hz to 8 kHz) at the same level in every mode, and a morph knob moves through LP4, LP2, BP, HP2 and HP4.
- Doombringer's three macros: WRATH (resonance, then drive), RUIN (the feedback folds, the output's level bends the cutoff, then the loop is pushed past the resonance knob) and DOOM (undertones an octave and two below the input, then a lurch of the cutoff). HUNGER feeds the input's envelope to the cutoff, and PING strikes the filter at its cutoff.
- A host test suite for Doombringer (`test/doom_tests.cpp`), five documented patches and three macro sweeps, rendered on the listening page.
- A polyphase interpolator next to the decimator in `dsp/Decimator.hpp`, so a processor can upsample its input.

### Changed

- `make firmware` builds both programs; `make -C hw APP=doombringer` builds the filter alone.
- `make panel` generates both panels and both layout headers, and `make panel-check` checks both.

## 0.1.1

### Fixed

- Releases published from GitHub's UI now get their assets. The workflow uploads into a release that already exists instead of failing, which is what left `v0.1.0` with no downloads.

## 0.1.0 (2026-10-27)

Tagged, but the release has no assets (see 0.1.1); use 0.1.1.

The first public release. The project version lives in `VERSION` and releases are tagged to match (`v0.1.0`). The VCV plugin keeps its own version in `vcv/plugin.json` (`2.0.0`), because VCV Rack 2 refuses to load a plugin whose version doesn't start with `2.`; its package files are named for that version.

### Added

- A through-zero FM complex oscillator with a wavefolder (timbre and symmetry), AM and ring mod, self-feedback, chaos cross-FM and a Lorenz-attractor modulator, all 4x oversampled.
- patch.Init firmware with a shift layer and soft takeover, and a VCV Rack 2 plugin with a generated sigil panel, both built from the same DSP core.
- Host tests for pitch tracking, stability, aliasing, feedback, chaos and long worst-case runs.
- Three character macros on the front panel, each with its own CV jack: FLESH (FM, fold, asymmetry), MALICE (feedback, chaos) and LEGION (sub, voices, drift). Ratio, modulator shape, symmetry and fine tune move to the patch.Init's shift layer, and the VCV panel mirrors the hardware knob for knob.
- A unison stack: up to four detuned primary voices and a sub one octave down, summed into the wavefolder, with spread, Lorenz drift and an envelope-aware DC correction. LEGION plays it.
- A sixth patch, Legion. All six are written as knob positions and rendered through the firmware's own panel mapping.
- Open-source project files, CI for every target, branding and a docs site.
- A listening page in the docs: the six patches, a sweep of each macro and five longer pieces, rendered from the DSP at every docs build, with spectrograms and WAV downloads.
- Signed releases: the firmware `.bin` and the VCV plugin for Linux x64, Windows x64, macOS x64 and macOS arm64, with cosign signatures and build provenance.
