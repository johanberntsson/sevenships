// The city screen, after Civilization's (civ1-reference/civ1-pic02.png):
// the city and its neighbours in the middle, the units in it on the left,
// and what it is building on the right with a CHANGE button. EXIT goes back
// to the map.
#ifndef SEVENSHIPS_CITYSCREEN_H
#define SEVENSHIPS_CITYSCREEN_H

#include "mega65.h"

// Show city `c` until the player leaves it. Runs its own once-a-frame loop,
// like a popup, and takes over the whole screen and sprites 2 and 3, so
// **the caller puts the map screen back afterwards** and redraws the view:
// the patch in the middle is drawn into the map window's tile store.
void city_screen(byte c);

// **The next build popup** (Game -> Options -> Next Build Popup, game.h):
// game_end_turn() notes each of the human's cities that finished something,
// up to BUILT_MAX a turn, and game_run() calls city_next_build() once a
// frame while city_built_count is not 0. The first call brings the first
// city into view; the next, after the redraw, says what it built and
// offers what the computer would build next there (ai_build()) or
// "Something else", the city screen with CHANGE open. Then the next city.
#define BUILT_MAX 8
extern byte city_built_count, city_built[BUILT_MAX], city_built_kind[BUILT_MAX];
void city_next_build(void);

#endif // SEVENSHIPS_CITYSCREEN_H
