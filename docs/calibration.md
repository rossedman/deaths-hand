# Calibration

Four constants at the top of `hw/main.cpp` depend on your unit. Check them once after the first flash.

| Constant | What it does | How to set it |
|---|---|---|
| `kAudioFullScaleV` | The volts a full-scale codec sample produces | Set conservatively. If the output is quiet, measure it with a scope and lower the number |
| `kInvertAudio` | Flips the Patch SM's inverting audio stages back | Doesn't matter for tones; it does for a ramp used as CV |
| `kPitchCvScale` / `kPitchCvOffset` | 1V/oct trim for CV_5 | Send exactly 1 V and 3 V into CV_5 and use libDaisy's `VoctCalibration` to get the two numbers |
| `p.ratioLock` | Toggle direction | If "up" turns out to be free mode, flip it |

Rebuild and flash after changing them (`make firmware`, then `make -C hw program-dfu`).
