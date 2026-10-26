// In region 6 (src/region.h), with the menus' commands, since 26 Oct 2026
// and the slots (region 0 had no room for them): called through the stubs
// in src/dispatch.s, under these names here.
#define save_screen save_screen_r6
#define save_read save_read_r6

#include <fcntl.h>
#include <string.h>
#include <calypsi/stubs.h>

#include "save.h"
#include "ai.h"
#include "city.h"
#include "civ.h"
#include "combat.h"
#include "crisis.h"
#include "emporium.h"
#include "treasury.h"
#include "culture.h"
#include "diplomacy.h"
#include "trade.h"
#include "game.h"
#include "map.h"
#include "unit.h"
#include "road.h"
#include "tablets.h"
#include "input.h"
#include "music.h"
#include "screen.h"
#include "text.h"

#pragma clang section text="r6text" rodata="r6rodata"

// Everything a game is, in file order. The map goes with its explored and
// occupied bits; the terrain in it is the loaded map's, which is the same
// every game. What can be worked out again is left out: the active unit,
// the view's position (game_run() centres on whoever wants orders) and the
// panel.
struct block { void *at; word size; };

static const struct block blocks[] = {
    { map_tile, sizeof map_tile },
    // From the map file and never changed, but saved with the terrain it
    // belongs to.
    { &map_res_count, sizeof map_res_count },
    { map_res_first, sizeof map_res_first },
    { map_res_ax, sizeof map_res_ax },
    { map_res_kind, sizeof map_res_kind },
    { &road_count, sizeof road_count },
    { road_first, sizeof road_first },
    { road_ax, sizeof road_ax },
    { road_mask, sizeof road_mask },
    { unit_kind, sizeof unit_kind },
    { unit_owner, sizeof unit_owner },
    { unit_ax, sizeof unit_ax },
    { unit_ay, sizeof unit_ay },
    { unit_moves, sizeof unit_moves },
    { unit_orders, sizeof unit_orders },
    { unit_work, sizeof unit_work },
    { unit_goto_x, sizeof unit_goto_x },
    { unit_goto_y, sizeof unit_goto_y },
    { ai_heading, sizeof ai_heading },
    { ai_seen, sizeof ai_seen },
    { &city_count, sizeof city_count },
    { &city_founded, sizeof city_founded },
    { city_owner, sizeof city_owner },
    { city_ax, sizeof city_ax },
    { city_ay, sizeof city_ay },
    { city_founder, sizeof city_founder },
    { city_name, sizeof city_name },
    { city_build, sizeof city_build },
    { city_shields, sizeof city_shields },
    { city_size, sizeof city_size },
    { city_food, sizeof city_food },
    { city_worked, sizeof city_worked },
    { city_named, sizeof city_named },
    { combat_war, sizeof combat_war },
    { diplomacy_known, sizeof diplomacy_known },
    { diplomacy_truce, sizeof diplomacy_truce },
    { diplomacy_mood, sizeof diplomacy_mood },
    { deal_from, sizeof deal_from },
    { deal_to, sizeof deal_to },
    { deal_give, sizeof deal_give },
    { deal_back, sizeof deal_back },
    { deal_due, sizeof deal_due },
    { trade_got, sizeof trade_got },
    { trade_partners, sizeof trade_partners },
    { &combat_seed, sizeof combat_seed },
    { &game_level, sizeof game_level },
    { player_civ, sizeof player_civ },
    { &game_turn, sizeof game_turn },
    { &game_year, sizeof game_year },
    { &crisis_drought, sizeof crisis_drought },
    { crisis_cities, sizeof crisis_cities },
    { &crisis_capitals, sizeof crisis_capitals },
    { &emporium_count, sizeof emporium_count },
    { emporium_owner, sizeof emporium_owner },
    { emporium_ax, sizeof emporium_ax },
    { emporium_ay, sizeof emporium_ay },
    { player_gold, sizeof player_gold },
    { city_culture, sizeof city_culture },
    { &wonders_built, sizeof wonders_built },
    { diplomacy_lord, sizeof diplomacy_lord },
    { scribe_pay, sizeof scribe_pay },
    { knowledge, sizeof knowledge },
    { tablets, sizeof tablets },
    { &tablets_first, sizeof tablets_first },
    { player_legacy, sizeof player_legacy },
};
#define BLOCKS (sizeof blocks / sizeof blocks[0])

static const char header[4] = { 'S', 'S', 'V', SAVE_VERSION };

HIGH_BSS byte save_slot;
// The header as read, or as it will be written: the four bytes, then the
// comment. One slot's at a time; the list on the screen is the only copy
// of the others.
HIGH_BSS static char head[sizeof header + SAVE_COMMENT];
#define comment (head + sizeof header)

// SEVENSHIPS.SV0 to .SV9. For writing, "@0:" replaces the file if it is
// there, and ",S,W" makes it a SEQ file and opens it for writing, which a
// secondary address of 3 or more would not. On the C stack, not in the
// region: the KERNAL reads the name with its own map.
static int open_slot(char *name, byte slot, byte writing)
{
    static const char write_name[] = "@0:" SAVE_FILE_NAME "0,S,W";
    byte i;

    for (i = 0; i < sizeof write_name; ++i) name[i] = write_name[i];
    name[3 + sizeof SAVE_FILE_NAME - 1] = (char)('0' + slot);
    if (writing)
        return _Stub_open(name, O_WRONLY | O_BINARY);
    name[3 + sizeof SAVE_FILE_NAME] = 0;
    return _Stub_open(name + 3, O_RDONLY | O_BINARY);
}

static byte save_write(void)
{
    char name[sizeof SAVE_FILE_NAME + 8];
    int fd;
    byte i, ok = 0;

    music_off();
    fd = open_slot(name, save_slot, 1);
    if (fd >= 0) {
        for (i = 0; i < sizeof header; ++i) head[i] = header[i];
        ok = _Stub_write(fd, head, sizeof head) == sizeof head;
        for (i = 0; ok && i < BLOCKS; ++i)
            ok = _Stub_write(fd, blocks[i].at, blocks[i].size)
                 == blocks[i].size;
        _Stub_close(fd);
    }
    music_on();
    return ok;
}

// **In pieces under 256 bytes**, because _Stub_read miscounts a request
// that the end of the file meets on a page boundary (map_load() in map.c).
#define PIECE 128

static byte read_block(int fd, byte *at, word size)
{
    byte n;
    while (size) {
        n = size > PIECE ? PIECE : (byte)size;
        if (_Stub_read(fd, at, n) != n) return 0;
        at += n;
        size -= n;
    }
    return 1;
}

// What is in a slot: nothing, a save of another version, or one this build
// can load, with its comment in `head`. The file is left open for
// save_read() if `fd` is not 0.
#define SLOT_EMPTY 0
#define SLOT_OTHER 1
#define SLOT_GAME 2

static byte peek(byte slot, int *fd)
{
    char name[sizeof SAVE_FILE_NAME + 8];
    int f = open_slot(name, slot, 0);
    byte i, got = SLOT_EMPTY;

    if (f < 0) return SLOT_EMPTY;
    if (read_block(f, (byte *)head, sizeof head)) {
        got = SLOT_GAME;
        for (i = 0; i < sizeof header; ++i)
            if (head[i] != header[i]) got = SLOT_OTHER;
        comment[SAVE_COMMENT - 1] = 0;
    }
    if (got == SLOT_GAME && fd) *fd = f;
    else _Stub_close(f);
    return got;
}

byte save_read(void)
{
    int fd;
    byte i, ok = 0;

    music_off();
    if (peek(save_slot, &fd) == SLOT_GAME) {
        for (ok = 1, i = 0; ok && i < BLOCKS; ++i)
            ok = read_block(fd, blocks[i].at, blocks[i].size);
        _Stub_close(fd);
    }
    music_on();
    return ok;
}

// ------------------------------------------------------------ the screen

#define TOP 4                   // slot 0's row; they are two rows apart
#define NUMBER_LEFT 20
#define COMMENT_LEFT 24
#define HINT_ROW 24

static byte length(const char *s)
{
    byte n = 0;
    while (s[n]) ++n;
    return n;
}

static void hint(const char *s)
{
    screen_fill(0, HINT_ROW, SCREEN_COLUMNS, COLOR_LIGHTGREY);
    screen_putsxy((byte)((SCREEN_COLUMNS - length(s)) / 2), HINT_ROW, s,
                  COLOR_LIGHTGREY);
}

static void add(const char *s)
{
    byte n = length(comment);
    while (*s && n < SAVE_COMMENT - 1) comment[n++] = *s++;
    comment[n] = 0;
}

// "Agamemnon - Mycenaeans - 1450 BC", which the player may change.
static void default_comment(void)
{
    byte civ = player_civ[PLAYER_HUMAN];
    char year[8];
    byte i = 4;
    word y = game_year;

    year[i] = 0;
    do { year[--i] = (char)('0' + y % 10); y /= 10; } while (y);
    comment[0] = 0;
    add(civ_leader(civ));
    add(" - ");
    add(civ_people(civ));
    add(" - ");
    add(year + i);
    add(" BC");
}

static void draw_comment(byte slot, byte colour, byte editing)
{
    byte y = TOP + 2 * slot, n = length(comment);

    screen_fill(COMMENT_LEFT, y, SCREEN_COLUMNS - COMMENT_LEFT, colour);
    screen_putsxy(COMMENT_LEFT, y, comment, colour);
    if (editing)
        screen_putcxy(COMMENT_LEFT + n, y, ' ', ATTR_REVERSE | COLOR_WHITE);
}

static void mark(byte slot, byte on)
{
    screen_putsxy(NUMBER_LEFT - 2, TOP + 2 * slot, on ? ">" : " ",
                  COLOR_YELLOW);
}

// A slot's row as peek() found it, with its comment in `head`.
static void draw_slot(byte slot, byte got)
{
    if (got == SLOT_GAME) {
        draw_comment(slot, COLOR_LIGHTGREEN, 0);
        return;
    }
    comment[0] = 0;
    add(got == SLOT_EMPTY ? "empty" : "from another version");
    draw_comment(slot, COLOR_GREY, 0);
}

// The comment typed on the slot's row: Return keeps it, Esc gives up.
static byte edit(byte slot)
{
    byte key, n;

    default_comment();
    hint("Type a comment, then Return to save or Esc to go back");
    for (;;) {
        draw_comment(slot, COLOR_WHITE, 1);
        do {
            screen_wait_raster(SCREEN_ROW_LINE(SCREEN_ROWS - 1));
            key = input_poll();
        } while (!key);
        n = length(comment);
        if (key == KEY_RETURN) return 1;
        if (key == KEY_ESC || key == KEY_RUNSTOP) return 0;
        if (key == KEY_DEL) {
            if (n) comment[n - 1] = 0;
        } else if (key >= ' ' && key <= 'z' && n < SAVE_COMMENT - 1) {
            comment[n] = (char)key;
            comment[n + 1] = 0;
        }
    }
}

byte save_screen(byte saving)
{
    byte slot, got[SAVE_SLOTS], key, row;
    // The slot used last, if any; HIGH_BSS is not cleared, so it may be
    // anything at first.
    byte selected = save_slot < SAVE_SLOTS ? save_slot : 0;

    sprite_hide(2);             // the minimap's outline, if it is up
    sprite_hide(3);
    screen_clear();
    hint("Reading the disk...");
    screen_putsxy(saving ? 35 : 32, 1, saving ? "Save Game" : "Load Saved Game",
                  COLOR_WHITE);
    music_off();
    for (slot = 0; slot < SAVE_SLOTS; ++slot) {
        screen_putcxy(NUMBER_LEFT, TOP + 2 * slot, (char)('0' + slot),
                      COLOR_YELLOW);
        got[slot] = peek(slot, 0);
        draw_slot(slot, got[slot]);
    }
    music_on();

again:
    hint(saving ? "Pick a slot to save in, or Esc to go back"
                : "Pick a game to load, or Esc to go back");
    mark(selected, 1);
    for (;;) {
        screen_wait_raster(SCREEN_ROW_LINE(SCREEN_ROWS - 1));
        key = input_poll();
        slot = SAVE_NONE;
        if (input_moved() || key == KEY_CLICK) {
            row = (byte)(input_y >> 3);
            if (row >= TOP && row < TOP + 2 * SAVE_SLOTS && !(row & 1))
                slot = (byte)((row - TOP) >> 1);
            if (slot != SAVE_NONE && slot != selected) {
                mark(selected, 0);
                mark(selected = slot, 1);
            }
            if (key != KEY_CLICK) continue;
        }
        if (key == KEY_UP && selected > 0) {
            mark(selected--, 0);
            mark(selected, 1);
        } else if (key == KEY_DOWN && selected < SAVE_SLOTS - 1) {
            mark(selected++, 0);
            mark(selected, 1);
        } else if (key == KEY_ESC || key == KEY_RUNSTOP) {
            return SAVE_NONE;
        } else if (key >= '0' && key < '0' + SAVE_SLOTS) {
            mark(selected, 0);
            mark(selected = key - '0', 1);
            slot = selected;
        } else if (key == KEY_RETURN || key == KEY_SPACE) {
            slot = selected;
        }
        if (slot == SAVE_NONE) continue;

        save_slot = slot;
        if (!saving) {
            if (got[slot] == SLOT_GAME) return 1;
            continue;
        }
        if (!edit(slot)) {
            // What was on the row.
            if (got[slot] == SLOT_GAME) {
                music_off();
                peek(slot, 0);
                music_on();
            }
            draw_slot(slot, got[slot]);
            goto again;
        }
        draw_comment(slot, COLOR_WHITE, 0);
        hint("Saving...");
        return save_write();
    }
}
