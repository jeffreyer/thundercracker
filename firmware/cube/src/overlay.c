/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */
 
/*
 * Manually overlaid data memory.
 *
 * Many of our global variables are defined here, so that we can
 * control their memory layout. We can reuse the same memory for
 * video modes that we know are mutually exclusive, for example,
 * or for operations (like draw_*) that are mutually exclusive
 * with graphics rendering.
 *
 * Overlay contents below are grouped by size. We try to keep word
 * vars with word vars, and byte vars with byte vars. Bits are
 * declared in our separate bit-addressable segment.
 *
 * We also overlay variables onto unused SFRs. This has to be
 * done at the original declaration; so this file can't declare
 * the overlaid SFRs, but this comment attempts to keep a canonical
 * list of such overlaid registers:
 *
 *   C9     MPAGE      x_bg0_wrap
 *   B4     RTC2CMP0   x_bg0_first_addr
 *   B5     RTC2CMP1   x_bg0_last_w
 *   A1     PWMDC0     x_bg0_first_w
 *   A2     PWMDC1     y_bg0_addr_l
 *   C2     CCL1       x_bg1_first_addr
 *   C3     CCH1       x_bg1_last_addr
 *   C4     CCL2       y_bg1_addr_l
 *   C5     CCH2       y_bg1_bit_index
 *   C6     CCL3       x_bg1_shift
 *   C7     CCH3       y_spr_active
 */

static void overlay_memory() __naked {
    __asm

    .area DSEG    (DATA)

_y_bg0_map::
    .ds 2
_y_bg1_map::
    .ds 2

_y_spr_line::
    .ds 1
_y_spr_line_limit::
    .ds 1

_x_spr::            ; 20 bytes
_bg2_state::        ; 14 bytes
_lcd_window_x::
    .ds 1           ; 0
_lcd_window_y::
    .ds 1           ; 1
_draw_xy::
    .ds 2           ; 2
_draw_attr::
    .ds 1           ; 4
    .ds 20-5        ; 5

    .area BSEG    (BIT)

_x_bg1_rshift::
    .ds 1
_x_bg1_lshift::
    .ds 1
_x_bg1_offset_bit0::
    .ds 1
_x_bg1_offset_bit1::
    .ds 1
_x_bg1_offset_bit2::
    .ds 1
_y_bg1_empty::
    .ds 1

    __endasm ;
}
