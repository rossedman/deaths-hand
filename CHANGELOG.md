# Changelog

All notable changes to this project are documented here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow [Semantic Versioning](https://semver.org/).

## Unreleased

The first public release. VCV Rack requires plugin versions to start with the Rack major version, so releases are tagged `v2.x.y` to match `vcv/plugin.json`.

### Added

- A through-zero FM complex oscillator with a wavefolder (timbre and symmetry), AM and ring mod, self-feedback, chaos cross-FM and a Lorenz-attractor modulator, all 4x oversampled.
- patch.Init firmware with a shift layer and soft takeover, and a VCV Rack 2 plugin with a generated sigil panel, both built from the same DSP core.
- Host tests for pitch tracking, stability, aliasing, feedback, chaos and long worst-case runs.
- Three character macros on the front panel, each with its own CV jack: FLESH (FM, fold, asymmetry), MALICE (feedback, chaos) and LEGION (sub, voices, drift). Ratio, modulator shape, symmetry and fine tune move to the patch.Init's shift layer, and the VCV panel mirrors the hardware knob for knob.
- A unison stack: up to four detuned primary voices and a sub one octave down, summed into the wavefolder, with spread, Lorenz drift and an envelope-aware DC correction. LEGION plays it.
- A sixth patch, Legion. All six are written as knob positions and rendered through the firmware's own panel mapping.
- Open-source project files, CI for every target, branding and a docs site.
- Signed releases: the firmware `.bin` and the VCV plugin for Linux x64, Windows x64, macOS x64 and macOS arm64, with cosign signatures and build provenance.
