# Changelog

All notable changes to this project are documented here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow [Semantic Versioning](https://semver.org/).

## Unreleased

The first public release. VCV Rack requires plugin versions to start with the Rack major version, so releases are tagged `v2.x.y` to match `vcv/plugin.json`.

### Added

- A through-zero FM complex oscillator with a wavefolder (timbre and symmetry), AM and ring mod, self-feedback, chaos cross-FM and a Lorenz-attractor modulator, all 4x oversampled.
- patch.Init firmware with a shift layer and soft takeover, and a VCV Rack 2 plugin with a generated sigil panel, both built from the same DSP core.
- Host tests for pitch tracking, stability, aliasing, feedback, chaos and long worst-case runs.
- Open-source project files, CI for every target, signed releases, branding and a docs site.
