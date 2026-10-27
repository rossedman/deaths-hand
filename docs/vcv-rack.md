# VCV Rack

The VCV Rack 2 plugin mirrors the patch.Init. Pitch and the three [character macros](panel.md#the-macros) sit on top, and the settings the hardware keeps on its shift layer get knobs of their own. Knobs and CVs go through the same `cosc::panelToParams` as the firmware, so a patch translates one-to-one.

See [Reading the panel](panel.md) for what each sigil means, and [Getting started](getting-started.md#vcv-rack) to install it.

## Differences from the patch.Init

- No shift layer: ratio, modulator shape, symmetry trim and fine tune each have a knob.
- Routing is a four-position knob (the crossed ring) instead of a button tap, and its jack takes a trigger to step it, like the patch.Init's Gate 2.
- The ratio lock is a switch (the key sigil). Up locks the modulator to the ratio steps; down frees it.
- The symmetry CV has its own jack instead of sharing the patch.Init's audio IN R.

## Building

```sh
scripts/fetch-rack-sdk.sh ../Rack-SDK
make vcv RACK_DIR=../Rack-SDK          # vcv/dist/DeathsHand-<version>-lin-x64.vcvplugin
```

Release builds are Linux x64. For macOS and Windows, build with the SDK for that platform from [vcvrack.com](https://vcvrack.com/downloads/).
