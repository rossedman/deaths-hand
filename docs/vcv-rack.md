# VCV Rack

The VCV Rack 2 plugin runs the same DSP core as the firmware, with every parameter on its own knob instead of a shift layer, plus CV inputs and attenuverters for FM index, timbre, feedback and chaos.

See [Reading the panel](panel.md) for what each sigil means, and [Getting started](getting-started.md#vcv-rack) to install it.

## Differences from the patch.Init

- No shift layer: feedback, chaos, symmetry and the modulator shape each have a knob.
- The ratio lock is a switch (the key sigil). Up locks the modulator to the ratio steps; down frees it.
- AM → ring mod is a continuous knob (the crossed ring) rather than a routing step, and the serpent knob sets how much the modulator drives the timbre.
- Separate CV inputs for symmetry, AM, feedback and chaos, and a dedicated MOD V/OCT input.

## Building

```sh
scripts/fetch-rack-sdk.sh ../Rack-SDK
make vcv RACK_DIR=../Rack-SDK          # vcv/dist/DeathsHand-<version>-lin-x64.vcvplugin
```

Release builds are Linux x64. For macOS and Windows, build with the SDK for that platform from [vcvrack.com](https://vcvrack.com/downloads/).
