// Saving and loading a game: ten slots on drive 8, SEVENSHIPS.SV0 to .SV9,
// picked on a screen of their own (save_screen()) that lists each slot's
// comment.
//
// **A file is the program's own memory**, every array and counter a game
// is made of written out as it stands, in the order of the table in
// save.c, after a header: "SSV", SAVE_VERSION and the comment. So a save
// only fits the build that wrote it, or one whose state is laid out the
// same; **anything added to a game's state goes in the table, and changing
// what is in it means a new SAVE_VERSION**, so that an old file is refused
// rather than read into the wrong places (the screen says "from another
// version").
//
// Per-turn code, in a file of its own, like ai.c.
#ifndef SEVENSHIPS_SAVE_H
#define SEVENSHIPS_SAVE_H

#include "mega65.h"

#define SAVE_FILE_NAME "SEVENSHIPS.SV"  // and the slot's digit
#define SAVE_VERSION 23
#define SAVE_SLOTS 10
#define SAVE_COMMENT 36         // with its 0: "Suppiluliuma - Hittites - 1600 BC"
#define SAVE_NONE 0xff

// The slot save_read() reads, which save_screen() sets.
extern byte save_slot;

// The slots on the whole screen, for saving (`saving` non-zero) or for
// loading: the player picks one, and to save types a comment, which starts
// as the leader, the people and the year. SAVE_NONE if the player backs out
// with Esc; otherwise, saving, non-zero if the save went, and loading, 1
// with save_slot set to a slot that holds a game of this version. The
// caller puts its own screen back. A full disk is not noticed: that is in
// the drive's error channel, which nothing here reads.
byte save_screen(byte saving);

// Read save_slot over the game game_init() set up. Non-zero if it went; if
// not, what was there is half overwritten and only fit for another
// game_init().
byte save_read(void);

#endif // SEVENSHIPS_SAVE_H
