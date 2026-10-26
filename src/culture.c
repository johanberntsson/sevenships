// In region 8 (src/region.h; region 6 until 26 Oct 2026, when the next
// build popup's option needed its room): called through the stub in src/dispatch.s,
// under this name here.
#define culture_built culture_built_r8

#include "culture.h"
#include "city.h"
#include "unit.h"
#include "civ.h"
#include "diplomacy.h"
#include "popup.h"

#pragma clang section text="r8text" rodata="r8rodata"

HIGH_BSS byte wonders_built;
HIGH_BSS word player_legacy[PLAYERS];

// Nothing to announce during Debug/Play 50 Turns, as crisis.c.
#ifdef DEBUG_MENU
#define QUIET diplomacy_auto
#else
#define QUIET 0
#endif

// Each wonder in full and what it does, from the Lion Gate, for the popup.
static const char *const wonder_name[] = {
    "the Lion Gate", "the Palace of Knossos", "the Temple of Karnak",
    "the Great Harbour of Ugarit"
};
static const char *const wonder_does[] = {
    "the walls of their city",
    "and their cities lose half as much to corruption",
    "and their temples cost them nothing",
    "and their ships sail further"
};

void culture_built(byte c, byte kind)
{
    byte p = city_owner[c], i, w;

    city_build[c] = KIND_WARRIOR;           // what next is the player's
    if (p >= PLAYERS) return;               // the rebels' (crisis.c)
    city_culture[c] |= CULTURE_BIT(kind);
    if (!KIND_IS_WONDER(kind)) {
        // A temple's legacy when built, twice as much with the Temple of
        // Karnak; the rest is over time (treasury.c).
        if (kind == KIND_TEMPLE)
            player_legacy[p] += culture_holder(CITY_KARNAK) == p
                                ? 2 * LEGACY_TEMPLE : LEGACY_TEMPLE;
        return;
    }
    wonders_built |= WONDER_BIT(kind);
    player_legacy[p] += LEGACY_WONDER;
    for (i = 0; i < city_count; ++i)
        if (city_build[i] == kind) city_build[i] = KIND_WARRIOR;
    if (QUIET) return;
    w = kind - KIND_LION_GATE;
    popup_begin();
    popup_add("The ");
    popup_add(player_people(p));
    popup_add(" have built ");
    popup_add(wonder_name[w]);
    popup_add(" in ");
    popup_add(city_name_of(c));
    popup_add(": a wonder of the age, ");
    popup_add(wonder_does[w]);
    popup_add(".");
    popup_show();
}
