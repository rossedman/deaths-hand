# Five evil patches

Five starting points, rendered in `test/out/ex*.wav` by the same engine the firmware runs (`make test` writes them).

Knob positions are clock faces: 7:00 is fully counter-clockwise, 12:00 is centre, 5:00 is fully clockwise. "S1"–"S4" means hold the button and turn knob 1–4. Routing is the number of taps after power-on.

## 1 · Reactor hum

A low drone whose feedback breathes in and out.

Toggle up, routing 0. K1 8:45, K2 8:15 (ratio ½), K3 8:15, K4 10:30, S3 1:30. Rock **S1 slowly between 10:00 and 3:00**.

## 2 · Lorenz growl

A chaotic modulator in audio range, like an engine that won't idle.

**Toggle down**, routing 1. K1 9:30, K2 1:15, K4 11:00, S4 4:00 (Lorenz). Ease **K3 from 10:00 to 12:00**.

## 3 · Chaos bells

Inharmonic struck metal that gets more unhinged as it goes.

Toggle up, routing 2. K1 12:00, K2 12:00 (ratio 3), K3 7:00, K4 8:00. Sequencer into **CV_5**, a fast decaying envelope (0–2.5 V) into **CV_6**, trigger both together. Sweep **S2 from 9:00 to 3:00**.

## 4 · Feedback scream

The pitch dives while feedback pushes past saw into noise.

Toggle up, routing 0. K2 10:15 (ratio 1.5), K3 9:00, K4 1:00, S3 10:00. Together, sweep **K1 from 4:15 to 9:30** and **S1 from 1:00 to 5:00**, or send a falling CV into CV_5.

## 5 · Possession

Everything on: ring mod, feedback, rising chaos, then the modulator turns chaotic.

Toggle up, routing 3. K1 10:15, K2 11:00 (ratio 2), K3 11:30, S1 12:00, S3 2:30, S4 2:00 (square). Sweep **K4 from 10:00 to 3:00** and **S2 from 7:00 to 2:00**, then **flip S4 to 4:00** (Lorenz) halfway.
