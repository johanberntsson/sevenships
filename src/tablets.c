// In region 8 (src/region.h), with the treasury, which calls these
// directly.
#include "tablets.h"
#include "civ.h"
#include "culture.h"
#include "diplomacy.h"
#include "popup.h"
#include "treasury.h"

#pragma clang section text="r8text" rodata="r8rodata"

HIGH_BSS byte scribe_pay[PLAYERS];
HIGH_BSS word knowledge[PLAYERS];
HIGH_BSS byte tablets[PLAYERS];
HIGH_BSS byte tablets_first;

// Nothing to announce during Debug/Play 50 Turns, as crisis.c.
#ifdef DEBUG_MENU
#define QUIET diplomacy_auto
#else
#define QUIET 0
#endif

static const word cost[TABLETS] = { 100, 300, 600 };
static const char *const name[TABLETS] = {
    "Linear B", "a law code", "the alphabet"
};
static const char *const does[TABLETS] = {
    "their cities lose a quarter less to corruption",
    "their realm grows larger before it costs them gold",
    "their trade deals bring twice the gold"
};

byte tablet_next(byte p)
{
    byte t;
    for (t = 0; t < TABLETS; ++t)
        if (!(tablets[p] & (1 << t))) return t;
    return TABLET_NONE;
}

word tablet_cost(byte t)
{
    return cost[t];
}

const char *tablet_name(byte t)
{
    return name[t];
}

void tablets_turn(void)
{
    byte p, t, bit, first;
    word pay;

    for (p = 0; p < PLAYERS; ++p) {
        // Nobody pays scribes with nothing left to set down.
        t = tablet_next(p);
        if (t == TABLET_NONE) continue;
        if (p != PLAYER_HUMAN) {
            pay = player_gold[p] / 50;
            scribe_pay[p] = pay > SCRIBES_AI_MAX ? SCRIBES_AI_MAX : (byte)pay;
        }
        pay = scribe_pay[p];
        if (pay > player_gold[p]) pay = player_gold[p];
        player_gold[p] -= pay;
        knowledge[p] += pay;
        if (knowledge[p] < cost[t]) continue;
        knowledge[p] -= cost[t];
        bit = (byte)(1 << t);
        tablets[p] |= bit;
        first = !(tablets_first & bit);
        tablets_first |= bit;
        player_legacy[p] += first ? 2 * LEGACY_TABLET : LEGACY_TABLET;
        // The human hears of its own, and of anybody's that is the first.
        if (QUIET || (p != PLAYER_HUMAN && !first)) continue;
        popup_begin();
        popup_add("The scribes of the ");
        popup_add(player_people(p));
        popup_add(" have set down ");
        popup_add(name[t]);
        popup_add(first ? ", the first in the world: " : ": ");
        popup_add(does[t]);
        popup_add(".");
        popup_show();
    }
}
