#!/usr/bin/env python3
"""The background music: a harp piece for three SID voices, and its
encoding for the player in src/musicplay.s.

    mkmusic.py build/music.h            the tune as a C array
    mkmusic.py --wav preview.wav        as the SID would play it, roughly

**The piece is our own**, written on 26 Oct 2026 in the style of a
recording of the Hurrian Hymn to Nikkal (Ugarit, about 1400 BC) that the
user found: B minor, a quarter note of 22 frames (about 136 a minute), a
plucked ostinato on B with the F# below, and a tune that steps down
E-D-C#-B over it. The recording was analysed for the key, the tempo and
the texture, and none of its notes are used: the ancient tune is free,
but that arrangement of it is not.

Voice 1 is the tune, voice 2 the harp's dyads (the chord's third and
fifth on beats 1 and 3) and voice 3 the bass ostinato (root, root, fifth,
root). Every voice is a triangle wave that the SID lets go of a frame
after striking it, so that each note dies away in the release, like a
plucked string (src/musicplay.s).

**The stream** is what the player reads out of attic RAM once a frame:

    v lo hi     v 0, 7 or 14: strike the voice whose registers are v
                from the SID's, at frequency hi:lo
    $80 | n     n 1-127: wait n frames before the next event
    $80         the end: go back to the start

so a chord is its notes and then one wait. The frequencies are for a
PAL C64's SID clock, 985248 Hz.
"""
import sys, wave

FRAMES_EIGHTH = 11              # 50 frames a second: an eighth 0.22 s
SID_CLOCK = 985248

NOTE = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7,
        'G#': 8, 'A': 9, 'A#': 10, 'B': 11}


def midi(name):
    return 12 * (int(name[-1]) + 1) + NOTE[name[:-1]]


def parse(phrase):
    """'E4:2 D4:2 -:2 | ...': a note and its length in eighths, - a rest."""
    out = []
    for tok in phrase.split():
        if tok == '|':
            continue
        name, length = tok.split(':')
        out.append((None if name == '-' else midi(name), int(length)))
    return out


# The tune, a phrase of four bars of 4/4 a line, lengths in eighths.
PHRASES = {
    'A1': "E4:2 D4:2 C#4:2 B3:2 | D4:3 C#4:1 B3:4 | F#4:2 E4:2 D4:2 E4:2 | C#4:6 -:2",
    'A2': "E4:2 D4:2 C#4:2 B3:2 | D4:3 E4:1 F#4:4 | E4:2 D4:2 C#4:2 A3:2 | B3:6 -:2",
    'B1': "F#4:2 A4:2 B4:4 | A4:2 F#4:2 E4:4 | D4:2 E4:2 F#4:2 E4:2 | D4:4 C#4:4",
    'B2': "F#4:2 A4:2 B4:2 C#5:2 | B4:2 A4:2 F#4:4 | E4:2 D4:2 C#4:2 D4:2 | B3:6 -:2",
    'REST': "-:8 | -:8",
}
# A chord a bar, under each phrase.
CHORDS = {
    'A1': 'Bm Bm D A', 'A2': 'Bm D A Bm', 'B1': 'D Bm G A',
    'B2': 'D Bm Em Bm', 'REST': 'Bm Bm',
}
FORM = ['REST', 'A1', 'A2', 'B1', 'B2', 'A1', 'A2', 'REST']
TRIAD = {'Bm': ('B', 'D', 'F#'), 'D': ('D', 'F#', 'A'), 'A': ('A', 'C#', 'E'),
         'G': ('G', 'B', 'D'), 'Em': ('E', 'G', 'B')}


def voices():
    tune, dyads, bass = [], [], []
    for p in FORM:
        tune += parse(PHRASES[p])
        for chord in CHORDS[p].split():
            root, third, fifth = TRIAD[chord]
            bass += [(midi(root + '2'), 2), (midi(root + '2'), 2),
                     (midi(fifth + '2'), 2), (midi(root + '2'), 2)]
            dyads += [(midi(third + '3'), 4), (midi(fifth + '3'), 4)]
    lengths = {sum(l for _, l in v) for v in (tune, dyads, bass)}
    assert len(lengths) == 1, "the voices are not the same length"
    return tune, dyads, bass


def sid_freq(m):
    return round(440 * 2 ** ((m - 69) / 12) * 16777216 / SID_CLOCK)


def encode(vs):
    """The stream: every note on, by its frame, then a wait to the next."""
    events = {}
    for v, notes in enumerate(vs):
        t = 0
        for m, length in notes:
            if m is not None:
                events.setdefault(t, []).append((v, m))
            t += length * FRAMES_EIGHTH
    end = t
    out = bytearray()
    times = sorted(events)
    if times[0] != 0:
        times.insert(0, 0)
        events[0] = []
    for i, t in enumerate(times):
        for v, m in events[t]:
            f = sid_freq(m)
            out += bytes((7 * v, f & 0xff, f >> 8))
        wait = (times[i + 1] if i + 1 < len(times) else end) - t
        while wait:
            n = min(wait, 127)
            out.append(0x80 | n)
            wait -= n
    out.append(0x80)
    return out


def preview(vs, path):
    """A rough idea of the SID's sound: triangle waves, struck and let go."""
    import numpy as np
    rate = 44100
    release = (0.75, 0.75, 0.3)          # src/music.c's release 9, 9 and 8
    level = (0.4, 0.25, 0.28)
    second = FRAMES_EIGHTH / 50
    total = sum(l for _, l in vs[0]) * second
    out = np.zeros(int(rate * (total + 2)))
    for v, notes in enumerate(vs):
        t = 0
        for m, length in notes:
            if m is not None:
                f = sid_freq(m) * SID_CLOCK / 16777216
                n = int(rate * release[v] * 1.5)
                tt = np.arange(n) / rate
                tri = 2 * np.abs(2 * ((tt * f) % 1) - 1) - 1
                env = np.minimum(tt / 0.002, 1) * np.exp(-tt / (release[v] / 4))
                i = int(t * rate)
                seg = out[i:i + n]
                seg += (tri * env * level[v])[:len(seg)]
            t += length * second
    out = np.convolve(out, np.ones(4) / 4, 'same')
    out = np.int16(np.clip(out, -1, 1) * 28000)
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(out.tobytes())


def main():
    vs = voices()
    if sys.argv[1] == '--wav':
        preview(vs, sys.argv[2])
        return
    data = encode(vs)
    lines = ['// Made by tools/mkmusic.py; do not edit.',
             '#define MUSIC_TUNE_SIZE %d' % len(data),
             'static const byte music_tune[MUSIC_TUNE_SIZE] = {']
    for i in range(0, len(data), 16):
        lines.append('    ' + ', '.join('0x%02x' % b for b in data[i:i + 16]) + ',')
    lines.append('};')
    text = '\n'.join(lines) + '\n'
    try:
        if open(sys.argv[1]).read() == text:
            return
    except OSError:
        pass
    open(sys.argv[1], 'w').write(text)


if __name__ == '__main__':
    main()
