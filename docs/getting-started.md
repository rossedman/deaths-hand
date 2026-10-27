# Getting started

Flash the firmware onto an Electrosmith patch.Init, or load the plugin into VCV Rack. Both run the same DSP core, so a patch you find in one sounds the same in the other. There are two modules, Death's Hand (the oscillator) and [Doombringer](doombringer.md) (the filter): one plugin holds both, and each is its own patch.Init program.

## patch.Init: the prebuilt binary

1. Download `deaths-hand.bin` (or `doombringer.bin` for the filter) from the [latest release](https://github.com/rossedman/deaths-hand/releases/latest).
2. Put the Patch SM into its bootloader: hold **BOOT**, tap **RESET**, release **BOOT**. Both buttons are on the Patch SM board on the back of the module.
3. Open Electrosmith's [Daisy Web Programmer](https://electro-smith.github.io/Programmer/) in Chrome (it uses WebUSB), connect the module over USB, choose the `.bin` and flash.

Every release asset is signed. To check yours before it touches the hardware, see [Verifying releases](security.md#verifying-releases).

## patch.Init: from source

On macOS:

```sh
brew install --cask gcc-arm-embedded
brew install dfu-util
git clone --recurse-submodules https://github.com/rossedman/deaths-hand
cd deaths-hand
make firmware                # libDaisy, then hw/build/deaths-hand.bin and hw/build/doombringer.bin
make -C hw program-dfu       # after BOOT + RESET; add APP=doombringer for the filter
```

On Linux, install the [Arm GNU toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) and `dfu-util` from your package manager instead.

`make -C hw CPU_METER=1` builds a variant that prints DSP load once a second over USB serial (`screen /dev/tty.usbmodem* 115200`).

## VCV Rack

Download the `.vcvplugin` file for your platform (`lin-x64`, `win-x64`, `mac-x64` or `mac-arm64`) from the [latest release](https://github.com/rossedman/deaths-hand/releases/latest) and drop it into your Rack user folder's `plugins` directory, or build it:

```sh
scripts/fetch-rack-sdk.sh ../Rack-SDK lin-x64   # or win-x64, mac-x64, mac-arm64
make vcv RACK_DIR=../Rack-SDK
make -C vcv RACK_DIR=../../Rack-SDK install
```

## Next

- [Reading the panel](panel.md): what each sigil means.
- [patch.Init control map](patch-init.md): four knobs, two layers.
- [Six evil patches](patches.md) to start from.
