// ai.c's helpers that its other files use (src/aibuild.c), and the
// numbers they share. Not for anything else. The helpers are in region 1
// and called through stubs in src/dispatch.s; region 1 is slot 1's alone,
// so a call from a slot 2 region costs no copy.
#ifndef SEVENSHIPS_AIINT_H
#define SEVENSHIPS_AIINT_H

#include "mega65.h"

// A computer player stops building units, settlers apart, at AI_UNIT_BASE
// plus AI_UNITS_A_CITY for each city, up to AI_UNIT_MAX -- a guard and a
// builder a city and a few to explore -- so that three of them and the
// barbarians cannot fill the MAX_UNITS slots every player shares.
#define AI_UNIT_BASE 3
#define AI_UNITS_A_CITY 2
#define AI_UNIT_MAX 12

// No more than AI_CITY_CAP cities, settlers on their way counted;
// MAX_CITIES is shared with the human. 8 since 25 Oct 2026, when the
// reach of two rings came in: at 6 every people was at the cap by turn
// 64-96 and only grew inward after it, and in 255 turns of autoplay on
// seeds 1, 3 and 7 the citizens before the Crisis went from 120 (seed 1,
// at turn 128, when the Mycenaeans fell), 172 and 147 to 194, 183 and 175,
// with all four peoples standing at the end on every seed. CITY_SPACING 5
// instead (aisite.c) was no better, and worse where land is short.
#define AI_CITY_CAP 8

// A player builds a galley once it has this many cities, and cedar.
#define GALLEY_CITIES 3

// Warriors a player keeps exploring, not guarding: one, and one more for
// every three cities. Without them the guards take every warrior built,
// and in 100 turns of autoplay the player had seen 139 of 1100 hexagons of
// a continent everybody shared, and met nobody.
#define EXPLORERS(CITIES) (1 + (CITIES) / 3)

// Guards city c wants (ai.c's guards_wanted()).
byte ai_guards_wanted(byte c);

// The best site for a city within reach of (fx, fy), into (ai_site_x,
// ai_site_y); whether there is one. ai_find_far_site() for player p looks
// over the water as well if it has a galley. In src/aisite.c, region 2,
// out of ai.c to make room in region 1; ai.c calls them through stubs.
// ai.c also leaves its galleys' destinations in ai_site_x and ai_site_y.
byte ai_find_site(byte fx, byte fy);
byte ai_find_far_site(byte p, byte fx, byte fy);
extern byte ai_site_x, ai_site_y;

// Whether a hexagon of city c's RING wants improving and has nobody on it,
// and which, into ai_work_x and ai_work_y: copper or tin to mine first,
// for bronze; then one the city works, as those are what an improvement
// adds to; then any of the city's -- the last only with `passes`
// WORK_ALL, since the builders make the road to the next city in between
// (road_site() in ai.c). In src/aibuild.c, region 2, beside
// improve_best(), since region 1 is full; ai.c calls it through a stub.
#define WORK_WORKED 2
#define WORK_ALL 3
byte ai_work_site(byte c, byte passes);
extern byte ai_work_x, ai_work_y;

// What each of p's cities builds this turn: src/aibuild.c, region 2.
void ai_build(byte p);

#endif // SEVENSHIPS_AIINT_H
