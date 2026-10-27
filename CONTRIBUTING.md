# Contributing

Thanks for helping make Death's Hand worse in the best way. Bug reports, sounds that misbehave in the wrong way, DSP fixes, new tests and better docs are all welcome.

This project follows the [Contributor Covenant code of conduct](https://github.com/rossedman/deaths-hand/blob/main/CODE_OF_CONDUCT.md). By taking part, you agree to uphold it.

## Set up

You need a C++17 compiler, `make`, [uv](https://docs.astral.sh/uv/) and `clang-format` 18 (or let `make` fetch it through uv).

```sh
git clone --recurse-submodules https://github.com/rossedman/deaths-hand
cd deaths-hand
make check
```

Building the other targets needs more:

| Target | Needs | Command |
| --- | --- | --- |
| patch.Init firmware | [Arm GNU toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) (`arm-none-eabi-gcc`), the `lib/libDaisy` submodule | `make firmware` |
| VCV Rack plugin | the [Rack 2 SDK](https://vcvrack.com/downloads/), unpacked; `zstd` | `make vcv RACK_DIR=path/to/Rack-SDK` |
| Spectrograms | nothing extra (uv fetches numpy, scipy, matplotlib) | `make analyze` |
| Docs site | nothing extra (uv fetches MkDocs) | `make docs-serve` |
| Banner, social card, diagram | Chrome or Chromium (fonts are vendored) | `docs/assets/render.sh` |

## Check your work

Run every check CI runs that needs no cross-compiler before you open a pull request:

```sh
make check
```

- `make lint` runs `clang-format --dry-run` on the C++ and ruff on the Python. `make fmt` fixes formatting.
- `make test` builds and runs `test/host_tests.cpp`: 1V/oct tracking, stability under abusive settings, through-zero FM, the feedback spectrum, chaos, Lorenz bounds, 60-second worst-case runs and the shift-layer pickup. It exits non-zero on any failure.
- `make panel-check` fails if `vcv/src/PanelLayout.hpp` has drifted from the table in `vcv/res/make_panel.py`.

CI also builds the firmware and the VCV plugin on every pull request.

## Rules for changes

- **Keep `dsp/` portable.** No heap allocation, no platform headers, volts in and volts out. Both targets and the host tests include it unchanged.
- **Test the sound.** A DSP change comes with a host test that would have caught the bug, or a check that pins the new behaviour. Name measured numbers in the README only when the tests print them.
- **The panel is generated.** Edit the table in `vcv/res/make_panel.py` and run `make panel`; never hand-edit `ComplexOsc.svg` or `PanelLayout.hpp`.
- **Watch the budget.** The patch.Init build must fit its 128 KB of flash and run at 48 kHz in real time. If you add cost to the audio path, measure it with `make -C hw CPU_METER=1`.
- **Record user-facing changes** in `CHANGELOG.md` under "Unreleased".

## Commits and pull requests

Use [Conventional Commits](https://www.conventionalcommits.org/) for commit subjects and pull request titles:

```text
feat(dsp): add a half-band decimator cascade
fix(hw): stop the routing LED flickering when shift is released
docs: describe the Lorenz modulator's rate range
```

Pull requests are squash-merged, using the PR title as the commit subject. Every pull request needs green checks (`lint`, `host tests`, `firmware` and `vcv plugin`) and an approving review from a code owner.

## Questions

Open an [issue](https://github.com/rossedman/deaths-hand/issues) for bugs, ideas and questions. Report security problems privately, as described in the [security policy](https://github.com/rossedman/deaths-hand/blob/main/SECURITY.md).
