# Reading the panel

There are no words on the panel, only sigils. Each knob's sigil is repeated on the jack and the attenuverter that modulate it, so the panel reads in columns.

<figure class="diagram" markdown>
![The Death's Hand panel: sixteen controls marked with sigils in four columns over a faint summoning circle, rune columns down both edges, and the two outputs on a red altar at the bottom right.](assets/panel.png){ width="360" }
</figure>

| Sigil | Means |
|---|---|
| Eye (lashes) | Pitch · V/OCT input |
| Slit eye | Fine tune |
| Vesica (two rings) | Ratio / modulator pitch · MOD V/OCT input |
| Trident | FM index · its CV and attenuverter |
| Zigzag in a circle | Timbre (fold) · its CV and attenuverter |
| Hourglass | Symmetry · its CV |
| Crossed ring ⊗ | AM → ring mod · its CV |
| Serpent | Modulator → timbre |
| Ouroboros | Self-feedback · its CV and attenuverter |
| Eight-arrow star | Chaos · its CV and attenuverter |
| Triangle in a circle | Modulator shape. The red last stop is Lorenz |
| Key | Ratio lock. The red arrow is up = locked |
| Spiral | External FM input |
| Pillars and lightning | Sync input |
| Blood drop (solid) | Primary out |
| Blood drop with rings | Modulator out |

Bone rings mark inputs, red rings mark outputs, and the outputs sit on the altar. The edge runes and the summoning circle are decoration.

## Where the panel comes from

The panel is generated: `make panel` runs `vcv/res/make_panel.py`, which writes both `vcv/res/ComplexOsc.svg` and `vcv/src/PanelLayout.hpp` from one table, so the widget positions can't drift from the artwork (CI checks this with `make panel-check`). Every sigil is drawn from code, with no fonts or images, so it renders in Rack and scales cleanly to a printed panel. It uses real Eurorack dimensions (16 HP × 128.5 mm), so it is also a starting point for a physical panel.
