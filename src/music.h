// The background music (26 Oct 2026, the user's): a harp piece for the
// SID's three voices, tools/mkmusic.py's, played by a few bytes of the
// KERNAL's interrupt once a frame (src/musicplay.s) from start-up to the quit.
//
// **It stops for every disk access after boot**: the KERNAL's disk code
// maps memory its own way, and an interrupt then could not count on
// finding the player. So whatever reads or writes the disk calls
// music_off() first and music_on() after -- save.c, and score.c round the
// end's picture. **A new disk access needs the same.**
//
// The player and its state are in HIGH_BSS, where music_init() copies it
// at boot, and music_on() and music_off() are its entries there: **neither
// may be called before music_init()**, which sheet_load() calls, region 0's
// last load. Callable from any region, since HIGH_BSS is always mapped.
#ifndef SEVENSHIPS_MUSIC_H
#define SEVENSHIPS_MUSIC_H

#include "mega65.h"

// The tune in attic RAM, at a 64 KB boundary (the player relies on it),
// above the code regions (REGION_ATTIC).
#define MUSIC_ATTIC 0x8020000UL

// Copy the player into HIGH_BSS and the tune into attic, and set up the
// SID: wanted, but not yet playing. Region 0's, at boot.
void music_init(void);

// Start, if Game -> Options -> Music is on and it is not playing; and
// stop, leaving the notes struck to ring out. Each may be called again.
void music_on(void);
void music_off(void);

// Game -> Options -> Music: 1 to play. A preference, not saved.
extern byte music_wanted;

#endif // SEVENSHIPS_MUSIC_H
