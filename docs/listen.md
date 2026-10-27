# Listen

Every sound on this page is rendered by the same DSP core the patch.Init firmware and the VCV plugin run, through the same panel mapping, so each one is knob positions and CVs you can dial in yourself. They are dry (no reverb, no effects), mono, normalised to −1 dBFS, and rebuilt from the current code every time the docs are published. Above each player, the spectrogram shows time left to right and frequency from 30 Hz to 16 kHz on a log scale, so the low end isn't squeezed into a sliver; the WAV link downloads the 48 kHz original.

## The three macros

Each macro on its own, swept from nothing to full.


<div class="clip" markdown>
<div class="clip__head" markdown>
**FLESH** C2. FM sidebands arrive, then the fold opens from glassy to snarling, then it tips asymmetric. [How to patch it](panel.md#the-macros).
</div>
![Spectrogram of FLESH: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/flesh_sweep.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/flesh_sweep.mp3"></audio><a class="clip__wav" href="../assets/audio/flesh_sweep.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**MALICE** A1. Sine to saw through the lower half, still harmonic; then over the edge into chaos, and collapse.
</div>
![Spectrogram of MALICE: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/malice_sweep.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/malice_sweep.mp3"></audio><a class="clip__wav" href="../assets/audio/malice_sweep.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**LEGION** C2 with a little FLESH. The sub first, then detuned voices fade in and beat, then drift pulls them apart.
</div>
![Spectrogram of LEGION: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/legion_sweep.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/legion_sweep.mp3"></audio><a class="clip__wav" href="../assets/audio/legion_sweep.wav" download>WAV</a></div>
</div>


## Six evil patches

The patches from the README and [Six evil patches](patches.md), each ten seconds.


<div class="clip" markdown>
<div class="clip__head" markdown>
**1 · Reactor hum** A low drone whose feedback breathes in and out. [How to patch it](patches.md#1-reactor-hum).
</div>
![Spectrogram of 1 · Reactor hum: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/ex1_reactor_hum.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/ex1_reactor_hum.mp3"></audio><a class="clip__wav" href="../assets/audio/ex1_reactor_hum.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**2 · Lorenz growl** A chaotic modulator in audio range, like an engine that won't idle. [How to patch it](patches.md#2-lorenz-growl).
</div>
![Spectrogram of 2 · Lorenz growl: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/ex2_lorenz_growl.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/ex2_lorenz_growl.mp3"></audio><a class="clip__wav" href="../assets/audio/ex2_lorenz_growl.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**3 · Chaos bells** Inharmonic struck metal that gets more unhinged as it goes. [How to patch it](patches.md#3-chaos-bells).
</div>
![Spectrogram of 3 · Chaos bells: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/ex3_chaos_bells.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/ex3_chaos_bells.mp3"></audio><a class="clip__wav" href="../assets/audio/ex3_chaos_bells.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**4 · Feedback scream** The pitch dives while feedback tears into noise. [How to patch it](patches.md#4-feedback-scream).
</div>
![Spectrogram of 4 · Feedback scream: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/ex4_feedback_scream.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/ex4_feedback_scream.mp3"></audio><a class="clip__wav" href="../assets/audio/ex4_feedback_scream.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**5 · Possession** Everything rising, then the modulator turns chaotic. [How to patch it](patches.md#5-possession).
</div>
![Spectrogram of 5 · Possession: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/ex5_possession.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/ex5_possession.mp3"></audio><a class="clip__wav" href="../assets/audio/ex5_possession.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**6 · Legion** One voice becomes a swarm while the fold opens. [How to patch it](patches.md#6-legion).
</div>
![Spectrogram of 6 · Legion: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/ex6_legion.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/ex6_legion.mp3"></audio><a class="clip__wav" href="../assets/audio/ex6_legion.wav" download>WAV</a></div>
</div>


## In a rack

Longer pieces that use the module the way a rack would: an LFO or an envelope into a macro's CV, a sequencer into V/OCT, and for the struck sounds a VCA after the output. The knob positions and modulation are in [`test/patches.hpp`](https://github.com/rossedman/deaths-hand/blob/main/test/patches.hpp).


<div class="clip" markdown>
<div class="clip__head" markdown>
**Altar drone** E1, ring routing, LEGION near full, a 0.1 Hz LFO into FLESH's CV.
</div>
![Spectrogram of Altar drone: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/altar_drone.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/altar_drone.mp3"></audio><a class="clip__wav" href="../assets/audio/altar_drone.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**Hex sequence** 16ths at 128 BPM in E phrygian. Each step's envelope hits FLESH and a VCA while MALICE creeps up.
</div>
![Spectrogram of Hex sequence: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/hex_sequence.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/hex_sequence.mp3"></audio><a class="clip__wav" href="../assets/audio/hex_sequence.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**Lorenz choir** A free modulator on Lorenz just below the note, LEGION full, FLESH breathing slowly.
</div>
![Spectrogram of Lorenz choir: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/lorenz_choir.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/lorenz_choir.mp3"></audio><a class="clip__wav" href="../assets/audio/lorenz_choir.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**Bells of ruin** A 1:7 ratio struck every 1.5 s through a VCA, MALICE climbing through the edge into chaos.
</div>
![Spectrogram of Bells of ruin: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/bells_of_ruin.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/bells_of_ruin.mp3"></audio><a class="clip__wav" href="../assets/audio/bells_of_ruin.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**Descent** Four and a half octaves down while FLESH, LEGION and MALICE all rise into collapse.
</div>
![Spectrogram of Descent: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/descent.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/descent.mp3"></audio><a class="clip__wav" href="../assets/audio/descent.wav" download>WAV</a></div>
</div>

## Doombringer

The ladder filter, rendered the same way through its own panel mapping. Each clip's input is in [`test/doom_patches.hpp`](https://github.com/rossedman/deaths-hand/blob/main/test/doom_patches.hpp): a saw, a sequence, nothing at all, or Death's Hand itself.

### Its three macros

<div class="clip" markdown>
<div class="clip__head" markdown>
**WRATH** A1 saw at 1.5 V. Ringing, then past the edge the filter screams over the saw.
</div>
![Spectrogram of WRATH: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/doom_wrath_sweep.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/doom_wrath_sweep.mp3"></audio><a class="clip__wav" href="../assets/audio/doom_wrath_sweep.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**RUIN** A1 saw at 1 V, WRATH at half. The resonance folds, then bends, then breaks free of the input.
</div>
![Spectrogram of RUIN: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/doom_ruin_sweep.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/doom_ruin_sweep.mp3"></audio><a class="clip__wav" href="../assets/audio/doom_ruin_sweep.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**DOOM** A2 saw. An octave below arrives, then two, then the undertone lurches the cutoff.
</div>
![Spectrogram of DOOM: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/doom_doom_sweep.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/doom_doom_sweep.mp3"></audio><a class="clip__wav" href="../assets/audio/doom_doom_sweep.wav" download>WAV</a></div>
</div>

### Five patches

The patches from [Doombringer](doombringer.md#five-patches), each ten seconds.

<div class="clip" markdown>
<div class="clip__head" markdown>
**1 · Acid rite** A sixteen-step bassline that bites; HUNGER is the envelope, and RUIN comes in halfway. [How to patch it](doombringer.md#1-acid-rite).
</div>
![Spectrogram of 1 · Acid rite: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/doom_p1_acid_rite.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/doom_p1_acid_rite.mp3"></audio><a class="clip__wav" href="../assets/audio/doom_p1_acid_rite.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**2 · Pitch-drop kicks** No input: a clock pings the filter, and RUIN makes each hit fall in pitch. [How to patch it](doombringer.md#2-pitch-drop-kicks).
</div>
![Spectrogram of 2 · Pitch-drop kicks: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/doom_p2_pitch_drop.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/doom_p2_pitch_drop.mp3"></audio><a class="clip__wav" href="../assets/audio/doom_p2_pitch_drop.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**3 · The scream** No input: the filter is the oscillator, sequenced through its cutoff CV while RUIN rises. [How to patch it](doombringer.md#3-the-scream).
</div>
![Spectrogram of 3 · The scream: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/doom_p3_scream.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/doom_p3_scream.mp3"></audio><a class="clip__wav" href="../assets/audio/doom_p3_scream.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**4 · Undertow** A slow riff dragged down an octave and two. [How to patch it](doombringer.md#4-undertow).
</div>
![Spectrogram of 4 · Undertow: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/doom_p4_undertow.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/doom_p4_undertow.mp3"></audio><a class="clip__wav" href="../assets/audio/doom_p4_undertow.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**5 · Death's Hand into Doombringer** The oscillator, folded and snarling, through a sweeping cutoff while RUIN rises. [How to patch it](doombringer.md#5-deaths-hand-into-doombringer).
</div>
![Spectrogram of 5 · Death's Hand into Doombringer: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/doom_p5_deaths_hand.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/doom_p5_deaths_hand.mp3"></audio><a class="clip__wav" href="../assets/audio/doom_p5_deaths_hand.wav" download>WAV</a></div>
</div>
