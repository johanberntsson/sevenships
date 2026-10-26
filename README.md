# Seven Ships of the Enemy

A turn-based strategy game for the **MEGA65**, in the mould of
*Civilization*: found cities, work the land around them, explore, trade,
fight and make peace, turn by turn, on a map of hexagons.

![the map view](screenshots/civ-popup.png)

The setting is the eastern Mediterranean of the Late Bronze Age. You lead
one of four peoples — the Mycenaeans, the Hittites, the Egyptians or the
Canaanites — from 1600 BC, through four centuries of palaces, trade in
copper, tin, cedar and purple dye, and letters between kings who call each
other brother. Then, around 1200 BC, the world they built breaks apart: the
Sea Peoples' ships come, the trade routes fail, droughts and earthquakes
strike, and kingdoms fall. The game is about what you build before that,
and what survives it.

The title comes from a letter written by the last king of Ugarit as his
kingdom fell: *"Seven ships of the enemy came, and they did great damage."*

## Playing

The game is the disk image, `release/sevenships.d81`. Mount it on a MEGA65
(or in xemu) and boot it. It can be played with a mouse, a joystick or the
keyboard alone. In xemu, the keymap `release/xemu-keymap.cfg` lets the
PC's keypad move units (7 9 4 6 1 3); HELP is on PgUp. From the
repository's top directory:

```sh
xemu-xmega65 -besure -keymap release/xemu-keymap.cfg -8 release/sevenships.d81
```

**How to play is in the manual, `release/sevenships_manual.pdf`.** It covers
the peoples, the rules, the Crisis, and how a game is won or lost.

To build it from source you need the [Calypsi 6502 tool chain](https://www.calypsi.cc/)
(5.18 or later), Python 3 and Ruby; `make` builds the disk and `make run`
boots it in xemu. `documentation/techreport.md` describes how the program
works.

## Credits

Most of the terrain, city, unit and resource tiles are from [OpenCiv](https://github.com/RyanGrieb/OpenCiv)
by Ryan Grieb, with art commissioned for it from lucasyoung988, brysia and
pratamacam. They are licensed under
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) (see OpenCiv's
[LICENSE](https://github.com/RyanGrieb/OpenCiv/blob/master/LICENSE) and
[issue #89](https://github.com/RyanGrieb/OpenCiv/issues/89)).

They have been modified for this game: scaled to 32×32 pixels, reduced to a
shared 240-colour palette, and widened at run time for the MEGA65's 640×200
display. The cattle, copper and tin (OpenCiv's iron) resources are cut down
to fit the middle of a hexagon, and the olives and wine, and their improved
groves, are OpenCiv's orange trees and orchard recoloured. The Sea
Peoples' raider is OpenCiv's swordsman, the merchant ship its cargo ship,
the chariot its chariot archer, and the emporium its bazaar and the horses
its horses, scaled down to the middle of a hexagon.

Additional graphics and tiles were created by myself for this game.

`tools/diskutil.rb`, which builds the disk image, was written by Fredrik
Ramsberg. Its sector interleave makes the disk load about five times faster
from a real floppy on a MEGA65.

## Licensing

- **Source code:** [MIT License](LICENSE).
- **Game assets:** [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
  Art derived from OpenCiv remains under its original CC BY 4.0 terms; see
  Credits.
