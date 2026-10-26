; The background music's player (src/music.h): a few bytes of the KERNAL's
; interrupt, once a frame, reading the tune out of attic RAM and striking
; the SID's three voices.
;
; **It runs from HIGH_BSS, $A000-$BFFF**, the one place that is always
; mapped, that the redraw path does not need and that disk access does not
; touch: the code window at $6000 changes under an interrupt, the resident
; program is full, and LOW_FREE is under the KERNAL's disk buffers. So it
; is assembled here, in region 0's rodata, **position-independent** --
; branches only inside it, absolute addresses only for its variables, the
; SID and the vectors -- and music_init() (src/music.c) copies it to
; music_code at boot. HIGH_BSS is not cleared, so nothing here may be
; called before that.
;
; **It reads the tune with the 45GS02's 32-bit pointer** (lda [zp],z),
; from MUSIC_ATTIC, through four bytes of zero page it saves and puts back:
; a DMA job here could land between the stores of one the main program is
; setting up. The KERNAL's interrupt has pushed A, X, Y, Z and B before it
; jumps through $0314, and pulls them on the way out; base page 0 is set
; here, and Z is left 0 for the KERNAL's own handler, which is jumped to
; last.

            .rtmodel version, "1"
            .rtmodel core, "*"

SID_L:      .equ    0xd400          ; SID 1, on the left
SID_R:      .equ    0xd440          ; SID 3, on the right: the same notes
ZP:         .equ    0xfc            ; borrowed during the interrupt
CINV:       .equ    0x0314          ; the KERNAL's interrupt vector

TRIANGLE:   .equ    0x10
GATE:       .equ    0x01

; ------------------------------------------------------- the code

            .section r0text, text
            .public music_blob, music_blob_end
music_blob:
blob:

; music_on(): into the interrupt, if the player wants music and it is not
; there already. Leaves Z alone, as compiled code expects.
on:         lda     music_wanted
            beq     done
            lda     music_hooked
            bne     done
            php
            sei
            lda     CINV
            sta     music_old
            lda     CINV+1
            sta     music_old+1
            lda     #.byte0 music_irq
            sta     CINV
            lda     #.byte1 music_irq
            sta     CINV+1
            plp
            inc     music_hooked
done:       rts

; music_off(): out of it. The notes struck ring on in their release.
off:        lda     music_hooked
            beq     done
            php
            sei
            lda     music_old
            sta     CINV
            lda     music_old+1
            sta     CINV+1
            plp
            dec     music_hooked
            rts

; The interrupt. Last frame's notes are let go first, so that a note is
; struck (gate on: attack 2 ms) and then dies away in its release.
irq:        lda     #0
            tab
            lda     #TRIANGLE
            sta     SID_L+4
            sta     SID_L+11
            sta     SID_L+18
            sta     SID_R+4
            sta     SID_R+11
            sta     SID_R+18
            dec     music_wait
            bne     leave
            ldx     #3
borrow:     lda     zp:ZP,x
            sta     music_zp,x
            lda     music_ptr,x
            sta     zp:ZP,x
            dex
            bpl     borrow
            ldz     #0
next:       lda     [ZP],z
            inz
            cmp     #0x80
            bcc     note
            and     #0x7f
            beq     restart
            sta     music_wait
            tza                     ; the pointer on past what was read
            clc
            adc     zp:ZP
            sta     music_ptr
            lda     zp:ZP+1
            adc     #0
            sta     music_ptr+1
            ldx     #3
give_back:  lda     music_zp,x
            sta     zp:ZP,x
            dex
            bpl     give_back
leave:      ldz     #0
            jmp     (music_old)

; The end: the tune starts at a 64 KB boundary (MUSIC_ATTIC).
restart:    lda     #0
            sta     zp:ZP
            sta     zp:ZP+1
            taz
            bra     next

; A note: A is the voice's registers' offset, 0, 7 or 14, and its
; frequency comes next.
note:       tax
            lda     [ZP],z
            inz
            sta     SID_L,x
            sta     SID_R,x
            lda     [ZP],z
            inz
            sta     SID_L+1,x
            sta     SID_R+1,x
            lda     #TRIANGLE | GATE
            sta     SID_L+4,x
            sta     SID_R+4,x
            bra     next
blob_end:
music_blob_end:

; ------------------------------------------------------- the state

            .section highbss, bss
            .public music_code, music_on, music_off, music_wanted
            .public music_wait, music_ptr, music_hooked

music_code: .space  blob_end - blob
music_on:   .equ    music_code + (on - blob)
music_off:  .equ    music_code + (off - blob)
music_irq:  .equ    music_code + (irq - blob)

music_wanted: .space 1              ; Game -> Options -> Music
music_hooked: .space 1              ; in the interrupt now
music_wait: .space  1               ; frames to the next event
music_ptr:  .space  4               ; the next event, in attic RAM
music_old:  .space  2               ; the KERNAL's handler
music_zp:   .space  4               ; ZP's own bytes, while borrowed
