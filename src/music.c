// The player's start-up (src/music.h), in region 0 with the loaders: the
// player and the tune come with the PRG, in region 0, and are copied out
// to where an interrupt can always reach them.
#include "music.h"

#pragma clang section text="r0text" rodata="r0rodata"

// The tune, after the pragma so that it is region 0's.
#include "music_tune.h"

extern byte music_code[];
extern const byte music_blob[], music_blob_end[];
extern byte music_wait, music_ptr[4];
extern byte music_hooked;

// SID 1 and SID 3, left and right (src/musicplay.s), and a voice's ADSR.
#define SID_LEFT 0xd400u
#define SID_RIGHT 0xd440u
#define SID_AD 5
#define SID_SR 6
#define SID_VOLUME 0x18

// How long each voice rings, the SID's decay and release: 9 is 750 ms,
// 8 300 ms. The note is let go a frame after it is struck, so that its
// decay hardly matters and its release is the pluck's ring.
static const byte ring[3] = { 9, 9, 8 };

void music_init(void)
{
    const byte *from = music_blob;
    byte *to = music_code;
    word sid;
    byte v;

    while (from != music_blob_end) *to++ = *from++;
    // Region 0 is the PRG's own $6000-$7FFF, where the DMA finds it.
    lcopy((word)music_tune, MUSIC_ATTIC, MUSIC_TUNE_SIZE);

    for (sid = SID_LEFT; sid <= SID_RIGHT; sid += SID_RIGHT - SID_LEFT) {
        for (v = 0; v < 3; ++v) {
            POKE(sid + 7 * v + 4, 0);
            POKE(sid + 7 * v + SID_AD, ring[v]);      // attack 2 ms
            POKE(sid + 7 * v + SID_SR, ring[v]);      // sustain 0
        }
        POKE(sid + SID_VOLUME, 15);
    }
    music_ptr[0] = 0;
    music_ptr[1] = 0;
    music_ptr[2] = (byte)(MUSIC_ATTIC >> 16);
    music_ptr[3] = (byte)(MUSIC_ATTIC >> 24);
    music_wait = 1;
    music_hooked = 0;
    music_wanted = 1;
}
