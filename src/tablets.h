// The clay tablets of knowledge (25 Oct 2026, vision.md 5B, the legacy
// victory's other half): **gold pays scribes, and scribes record
// tablets**, rather than a share of trade going to science as in
// Civilization -- the user's choice, since gold piled up with little to
// spend it on.
//
// **Each player pays its scribes scribe_pay[] gold a turn**, or what it
// has if less, and **each gold is a point of knowledge**. The human sets
// the pay on Advisors -> Finance with + and -; it starts at 1. The
// computer players pay a 50th of their treasury, at most SCRIBES_AI_MAX.
// Nobody pays once all of them are set down.
// At TABLET_COST(t) points a people records tablet t, in turn, and the
// points go into it:
// - **Linear B**, 100: accounts are kept, and its cities lose a quarter
//   less to corruption (treasury.c);
// - **a law code**, 300, Hammurabi's kind: LAW_FREE cities more before
//   the realm costs gold (treasury.c);
// - **the alphabet**, 600, Ugarit's: its trade deals bring twice the gold
//   (treasury.c).
// Each gives LEGACY_TABLET legacy points, twice that to the first people
// in the world to record it (tablets_first), which the human hears of in a
// popup; the human hears of its own too.
//
// **In region 8** (src/region.h), with the treasury, which calls
// tablets_turn() at the end of a turn and shows them on Advisors ->
// Finance.
#ifndef SEVENSHIPS_TABLETS_H
#define SEVENSHIPS_TABLETS_H

#include "mega65.h"
#include "unit.h"

#define TABLETS 3
#define TABLET_LINEAR_B 0x01
#define TABLET_LAW_CODE 0x02
#define TABLET_ALPHABET 0x04
#define TABLET_NONE TABLETS             // tablet_next() with all recorded
#define LEGACY_TABLET 10
#define SCRIBES_MAX 99                  // the most the human can pay
#define SCRIBES_AI_MAX 10
#define SCRIBES_START 1                 // the human's, at a new game
#define LAW_FREE 5

// HIGH_BSS, saved; game_init() resets them.
extern byte scribe_pay[PLAYERS];
extern word knowledge[PLAYERS];
extern byte tablets[PLAYERS];           // TABLET_* bits recorded
extern byte tablets_first;              // ... by anybody

// The next tablet p is to record, 0 to TABLETS - 1, or TABLET_NONE; its
// cost and its name.
byte tablet_next(byte p);
word tablet_cost(byte t);
const char *tablet_name(byte t);

// A turn's scribes for every player: their pay out of the treasury, the
// knowledge, and any tablet recorded. From treasury_turn().
void tablets_turn(void);

#endif // SEVENSHIPS_TABLETS_H
