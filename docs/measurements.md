# Measurements

Every number here is printed by the host tests (`make test`), which run the same DSP core the firmware and the plugin use. CI runs them on every pull request.

| | |
|---|---|
| 1V/oct tracking, −3 V … +5 V | worst error 0.09 cents |
| Alias energy, heavy fold @ 2.96 kHz | −9 dB → **−84 dB** with 4× |
| Alias energy, FM index 1.0 @ C7 | −40 dB → **−84 dB** with 4× |
| Self-feedback 0 → 0.5 → 1 | pure sine (H2 −117 dB) → saw-like (H2 −10 dB) → noisy |
| 60 s worst-case evil (fb, chaos, Lorenz, all max) | finite, alive, no DC lock-up, at 4 pitches |
| Output range | ±5 V nominal, DC-free, peaks up to ~7.3 V with heavy feedback/chaos |
| patch.Init flash / RAM | 106 KB of 128 KB / 37 KB |

## What the suite checks

Pitch accuracy, stability under abusive parameters and external FM, through-zero behaviour, the feedback spectrum, that chaos actually breaks periodicity, Lorenz boundedness from 0.01 Hz to 20 kHz, 60-second worst-case runs, the shift-layer pickup logic, and output bounds. Any failed check makes `make test` exit non-zero.

`make analyze` also writes spectrograms of the fold and FM sweeps with and without oversampling to `test/out/aliasing.png`, and prints the alias energy of each steady tone:

```text
non-harmonic (alias) energy in steady tones — lower is better
  fold_static OS1    -9.0 dB   OS4   -83.9 dB   improvement  74.9 dB
  fm_static   OS1   -39.7 dB   OS4   -83.9 dB   improvement  44.1 dB
```
