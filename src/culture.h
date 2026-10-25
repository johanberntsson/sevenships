// Buildings and wonders (23 Oct 2026; vision.md section 5B, the legacy
// victory): a city's temple, and the Lion Gate of Mycenae, the first
// wonder; since 25 Oct 2026 the Palace of Knossos, the Temple of Karnak
// and the Great Harbour of Ugarit too; and, later the same day, the
// stele, the edubba and the feasting hall, city buildings like the temple.
//
// **They are built as units are**, as kinds after the units' (KIND_TEMPLE,
// KIND_LION_GATE in unit.h), and city_produce() hands a finished one to
// culture_built(). What a city has is city_culture[] (city.h), a bit a
// building, kept when the city is taken.
//
// **The temple** is Civilization's: 40 shields, and 1 gold a turn upkeep
// (treasury.h), the treasury's first expense; with too little gold it is
// sold, as Civilization sells a building. It has no happiness to make, so
// what it gives is LEGACY_TEMPLE legacy points.
//
// **The Lion Gate**, a wonder: one in the world, 160 shields, no upkeep,
// announced to everybody when built, and every other city building it
// changes to warriors, keeping its shields, as in Civilization. It doubles
// the defence of every unit in its city -- the walls of Mycenae; its
// nearest in Civilization is the Great Wall, which acts as city walls --
// and gives LEGACY_WONDER legacy points, Civilization's score for a wonder.
//
// **The other wonders** are built and announced the same way, and give the
// same legacy; what each does is for the people that holds its city now
// (culture_holder() in city.h), as Civilization's wonders are:
// - **the Palace of Knossos** halves the corruption of every one of its
//   cities (treasury.c): Civilization's courthouse, everywhere;
// - **the Temple of Karnak**: its temples cost no upkeep (treasury.c) and
//   each it builds gives twice the legacy (culture.c), as Civilization's
//   Oracle doubles what temples do;
// - **the Great Harbour of Ugarit**, in a coastal city only: its ships
//   have a move more (unit_begin_turn() in unit.c), as Civilization's
//   Lighthouse and Magellan's give.
//
// **The city buildings** (25 Oct 2026, the user's: the temple was the
// only one) are one a city, like the temple, and **every one of them adds
// to its city's holder's legacy over time**: CULTURE_LEGACY() points every
// LEGACY_TURNS turns, in treasury_turn(). With too little gold for its
// upkeep, any of them is sold. Besides the temple:
// - **the stele**, a king's boast of his victories cut in stone (30
//   shields, no upkeep): twice its legacy while its people is at war with
//   another;
// - **the edubba**, the scribal school, the "house of tablets" (60, 1
//   gold): a point of knowledge a turn and a fifth of the scribes' pay
//   more (tablets.c);
// - **the feasting hall**, the megaron's hearth (60, 2 gold): twice the
//   legacy of the others.
//
// **Legacy points** (player_legacy[]) are the builder's for good, whatever
// becomes of the building or the city afterwards: they count in the score,
// and the highest at the end is the legacy victory, even for a people that
// has fallen (score.c). A city building's legacy over time is its holder's
// at the time.
//
// **In region 6** (src/region.h), called through a stub from city.c.
#ifndef SEVENSHIPS_CULTURE_H
#define SEVENSHIPS_CULTURE_H

#include "mega65.h"
#include "unit.h"

// A bit of city_culture[] a building, in KIND_* order from the temple; and
// a bit of wonders_built a wonder, from the Lion Gate.
#define CULTURE_BIT(K) ((byte)(1 << ((K) - KIND_TEMPLE)))
#define WONDER_BIT(K) ((byte)(1 << ((K) - KIND_LION_GATE)))
#define CITY_TEMPLE CULTURE_BIT(KIND_TEMPLE)
#define CITY_LION_GATE CULTURE_BIT(KIND_LION_GATE)
#define CITY_KNOSSOS CULTURE_BIT(KIND_KNOSSOS)
#define CITY_KARNAK CULTURE_BIT(KIND_KARNAK)
#define CITY_UGARIT CULTURE_BIT(KIND_UGARIT)
#define CITY_STELE CULTURE_BIT(KIND_STELE)
#define CITY_EDUBBA CULTURE_BIT(KIND_EDUBBA)
#define CITY_HALL CULTURE_BIT(KIND_HALL)

#define LEGACY_TEMPLE 3                 // when built
#define LEGACY_WONDER 20
#define LEGACY_TURNS 32                 // a city building's, over time

// A city building's upkeep a turn, and its legacy every LEGACY_TURNS.
#define CULTURE_UPKEEP(K)                                                   \
    ((K) == KIND_STELE ? 0 : (K) == KIND_HALL ? 2 : 1)
#define CULTURE_LEGACY(K) ((K) == KIND_HALL ? 2 : 1)

extern byte wonders_built;              // saved
extern word player_legacy[PLAYERS];     // saved

// Whether city c may build `kind`, a building: a city building where there
// is none, a wonder nobody has built -- the harbour in a coastal city.
#define CULTURE_CAN(C, K)                                                   \
    (!KIND_IS_WONDER(K) ? !(city_culture[C] & CULTURE_BIT(K))               \
     : !(wonders_built & WONDER_BIT(K))                                     \
       && ((K) != KIND_UGARIT || city_coastal(C)))

// City c has finished building `kind`. city.c's.
void culture_built(byte c, byte kind);

#endif // SEVENSHIPS_CULTURE_H
