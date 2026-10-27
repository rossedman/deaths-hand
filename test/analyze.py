#!/usr/bin/env python3
"""Spectrograms + an aliasing metric for the host renders.

usage: python3 analyze.py out/
Writes out/aliasing.png and prints how much non-harmonic energy each render has.
"""
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy.io import wavfile
from scipy.signal import spectrogram


def load(path):
    fs, x = wavfile.read(path)
    return fs, x[:, 0].astype(np.float64) / 32768.0  # left = primary out


def spec(ax, path, title):
    fs, x = load(path)
    f, t, s = spectrogram(x, fs, nperseg=2048, noverlap=1536, window="blackmanharris")
    db = 10 * np.log10(s + 1e-14)
    ax.pcolormesh(t, f / 1000, db, vmin=db.max() - 100, vmax=db.max(), shading="auto", cmap="magma")
    ax.set_title(title, fontsize=10)
    ax.set_ylabel("kHz")
    ax.set_ylim(0, fs / 2000)


C4 = 261.6255653


def alias_db(path, f0):
    """Energy that is NOT on the harmonic series of f0, in dB relative to total.
    For a steady tone every legit partial is k*f0; anything else is aliasing."""
    fs, x = load(path)
    seg = x[int(0.25 * fs) :]  # skip start-up
    seg = seg * np.blackman(len(seg))
    mag = np.abs(np.fft.rfft(seg)) ** 2
    freqs = np.fft.rfftfreq(len(seg), 1 / fs)
    harm = np.zeros_like(mag, dtype=bool)
    harm |= freqs < 20  # DC / sub-audio
    k = 1
    while k * f0 < fs / 2:
        harm |= np.abs(freqs - k * f0) < 8  # +-8 Hz around each harmonic
        k += 1
    # Only count the audible band: the decimator's 20-24 kHz transition band
    # may hold a little folded energy, but nobody hears it.
    audible = freqs < 20000
    return 10 * np.log10(mag[~harm & audible].sum() / mag.sum() + 1e-20)


def main(out):
    out = Path(out)
    pairs = [("fold_sweep", "Wavefolder, pitch sweep 0→5.5 V"), ("fm_sweep", "FM index sweep @ C6, ratio 1.5")]
    fig, axes = plt.subplots(2, 2, figsize=(11, 7), sharex=True, constrained_layout=True)
    for row, (name, label) in enumerate(pairs):
        for col, os_ in enumerate((1, 4)):
            p = out / f"{name}_os{os_}.wav"
            spec(axes[row][col], p, f"{label}\n{'no oversampling' if os_ == 1 else '4× oversampled'}")
    for ax in axes[-1]:
        ax.set_xlabel("seconds")
    fig.suptitle("Aliasing: falling/mirrored lines are aliases", fontsize=12)
    fig.savefig(out / "aliasing.png", dpi=110)

    # fold_static: 3.5 V. fm_static: C7 with ratio 1.5 -> partials on multiples of C7/2.
    statics = [("fold_static", C4 * 2**3.5), ("fm_static", C4 * 8 / 2)]
    print("non-harmonic (alias) energy in steady tones — lower is better")
    for name, f0 in statics:
        a1, a4 = alias_db(out / f"{name}_os1.wav", f0), alias_db(out / f"{name}_os4.wav", f0)
        print(f"  {name:11s} OS1 {a1:7.1f} dB   OS4 {a4:7.1f} dB   improvement {a1 - a4:5.1f} dB")
    print(f"wrote {out / 'aliasing.png'}")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "out")
