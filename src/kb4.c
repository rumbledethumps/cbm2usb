/*
 * Copyright (c) 2022 Rumbledethumps
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "tusb.h"

// Debounce and ghost detection added.
// Keycode mapping still naive.

#define KB_CAS_US 6
#define KB_SCAN_INTERVAL_US 200
#define KB_GHOST_US 2000
#define KB_DEBOUNCE_US 5000

#define KB_GHOST_TICKS ((KB_GHOST_US + KB_SCAN_INTERVAL_US - 1) / KB_SCAN_INTERVAL_US)
#define KB_DEBOUNCE_TICKS ((KB_DEBOUNCE_US + KB_SCAN_INTERVAL_US - 1) / KB_SCAN_INTERVAL_US)

static struct
{
    uint8_t status;   // 0=open, 1=pressed, 2-255=ghost
    uint8_t debounce; // countdown to 0
    bool sent;
    hid_keyboard_modifier_bm_t modifier;
} kb_scan[65];

static const uint8_t CBM_TO_KEYCODE[] = {
    HID_KEY_1, HID_KEY_BACKSPACE, HID_KEY_CONTROL_LEFT, HID_KEY_PAUSE,       // 0-3
    HID_KEY_SPACE, HID_KEY_ALT_LEFT, HID_KEY_Q, HID_KEY_2,                   // 4-7
    HID_KEY_3, HID_KEY_W, HID_KEY_A, HID_KEY_SHIFT_LEFT,                     // 8-11
    HID_KEY_Z, HID_KEY_S, HID_KEY_E, HID_KEY_4,                              // 12-15
    HID_KEY_5, HID_KEY_R, HID_KEY_D, HID_KEY_X,                              // 16-19
    HID_KEY_C, HID_KEY_F, HID_KEY_T, HID_KEY_6,                              // 20-23
    HID_KEY_7, HID_KEY_Y, HID_KEY_G, HID_KEY_V,                              // 24-27
    HID_KEY_B, HID_KEY_H, HID_KEY_U, HID_KEY_8,                              // 28-31
    HID_KEY_9, HID_KEY_I, HID_KEY_J, HID_KEY_N,                              // 32-35
    HID_KEY_M, HID_KEY_K, HID_KEY_O, HID_KEY_0,                              // 36-39
    HID_KEY_EQUAL, HID_KEY_P, HID_KEY_L, HID_KEY_COMMA,                      // 40-43
    HID_KEY_PERIOD, HID_KEY_SEMICOLON, HID_KEY_BRACKET_LEFT, HID_KEY_MINUS,  // 44-47
    HID_KEY_GRAVE, HID_KEY_BRACKET_RIGHT, HID_KEY_APOSTROPHE, HID_KEY_SLASH, // 48-51
    HID_KEY_SHIFT_RIGHT, HID_KEY_PAGE_DOWN, HID_KEY_PAGE_UP, HID_KEY_HOME,   // 52-55
    HID_KEY_DELETE, HID_KEY_ENTER, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_DOWN,  // 56-59
    HID_KEY_F1, HID_KEY_F3, HID_KEY_F5, HID_KEY_F7,                          // 60-63
    HID_KEY_BACKSLASH                                                        // 64 (RESTORE)
};

// Translate CBM code into USB HID keyboard modifier bitmap
hid_keyboard_modifier_bm_t cbm_to_modifier(uint8_t cbmcode)
{
    uint8_t keycode = CBM_TO_KEYCODE[cbmcode];
    if (keycode >= HID_KEY_CONTROL_LEFT && keycode <= HID_KEY_GUI_RIGHT)
        return 1 << (keycode & 7);
    return 0;
}

// Translate CBM code into USB HID keyboard code
void cbm_translate(uint8_t *code, hid_keyboard_modifier_bm_t *modifier)
{
    // Most keys work correctly with a simple table translation.
    // However, the VIC/C64 keyboard is not ASCII so most symbols
    // are in a different place. The Keyrah V2b works like this.
    // i.e. Shift-2 will always be an [@] instead of a ["].
    uint8_t cbmcode = *code;
    *code = CBM_TO_KEYCODE[cbmcode];
    // These overrides makes the C64 keyboard suitable for ASCII.
    // Every necessary shift state is mapped so this will also work
    // with VICE. If you make a .VKM file, please send a pull request.
    // See 4.2: https://vice-emu.sourceforge.io/vice_4.html
    const hid_keyboard_modifier_bm_t SHIFT =
        KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT;
    if (*modifier & SHIFT)
        switch (cbmcode)
        {
        case 1: // Shift left arrow -> ESC
            *modifier &= ~SHIFT;
            *code = HID_KEY_ESCAPE;
            break;
        case 7: // Shift 2 quote
            *code = HID_KEY_APOSTROPHE;
            break;
        case 23: // Shift 6 ampersand
            *code = HID_KEY_7;
            break;
        case 24: // Shift 7 apostrophe
            *modifier &= ~SHIFT;
            *code = HID_KEY_APOSTROPHE;
            break;
        case 31: // Shift 8 L paren
            *code = HID_KEY_9;
            break;
        case 32: // Shift 9 R paren
            *code = HID_KEY_0;
            break;
        case 39: // Shift 0
            *code = HID_KEY_F12;
            *modifier &= ~SHIFT;
            break;
        case 40: // Shift Plus
            *modifier &= ~SHIFT;
            *code = HID_KEY_PAGE_UP;
            break;
        case 47: // Shift Minus
            *modifier &= ~SHIFT;
            *code = HID_KEY_PAGE_DOWN;
            break;
        case 45: // Shift @ L bracket
            *modifier &= ~SHIFT;
            *code = HID_KEY_BRACKET_LEFT;
            break;
        case 48: // Shift Sterling Pound underbar
            *code = HID_KEY_MINUS;
            break;
        case 50: // Shift * R Bracket
            *modifier &= ~SHIFT;
            *code = HID_KEY_BRACKET_RIGHT;
            break;
        case 54: // Shift up arrow tilde
            *code = HID_KEY_GRAVE;
            break;
        case 55: // Shift CLR/HOME -> END
            *modifier &= ~SHIFT;
            *code = HID_KEY_END;
            break;
        case 56: // Shift INST/DEL
            *modifier &= ~SHIFT;
            *code = HID_KEY_INSERT;
            break;
        case 58: // Shift CRSR LEFT/RIGHT
            *code = HID_KEY_ARROW_LEFT;
            *modifier &= ~SHIFT;
            break;
        case 59: // Shift CRSR UP/DOWN
            *code = HID_KEY_ARROW_UP;
            *modifier &= ~SHIFT;
            break;
        case 60: // Shift F1/F2
            *code = HID_KEY_F2;
            *modifier &= ~SHIFT;
            break;
        case 61: // Shift F3/F4
            *code = HID_KEY_F4;
            *modifier &= ~SHIFT;
            break;
        case 62: // Shift F5/F6
            *code = HID_KEY_F6;
            *modifier &= ~SHIFT;
            break;
        case 63: // Shift F7/F8
            *code = HID_KEY_F8;
            *modifier &= ~SHIFT;
            break;
        }
    else // Not SHIFT
        switch (cbmcode)
        {
        case 40: // Plus
            *modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            break;
        case 45: // Colon
            *modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            *code = HID_KEY_SEMICOLON;
            break;
        case 46: // @
            *modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            *code = HID_KEY_2;
            break;
        case 49: // *
            *modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            *code = HID_KEY_8;
            break;
        case 50: // Semicolon
            *code = HID_KEY_SEMICOLON;
            break;
        case 54: // Up arrow carat
            *modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            *code = HID_KEY_6;
            break;
        }
    // CTRL-[ is often used on keyboards without a dedicated ESC key.
    // This simulates that keypress, use SHIFT-Left if real ESC needed.
    const hid_keyboard_modifier_bm_t CTRL =
        KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL;
    if (*modifier & CTRL)
        switch (cbmcode)
        {
        case 45: // Colon
        case 46: // @
            *modifier &= ~SHIFT;
            *code = HID_KEY_BRACKET_LEFT;
            break;
        }
    // Equal key is the only one without a SHIFT state.
    switch (cbmcode)
    {
    case 53: // Equal
        *code = HID_KEY_EQUAL;
        *modifier &= ~SHIFT;
        break;
    }
}

static void set_kb_scan(uint idx, bool is_up)
{
    if (kb_scan[idx].debounce)
        --kb_scan[idx].debounce;
    if (is_up)
    {
        if (!kb_scan[idx].debounce)
        {
            kb_scan[idx].status = 0;
            kb_scan[idx].sent = false;
        }
    }
    else
    {
        if (!kb_scan[idx].status)
        {
            kb_scan[idx].status = 1 + KB_GHOST_TICKS;
            kb_scan[idx].debounce = KB_DEBOUNCE_TICKS;
        }
    }
}

void kb_init()
{
    // Using GP16-17 for stdio
    stdio_uart_init_full(uart0, 115200, 16, 17);

    // RESTORE key not part of matrix
    // pin 1 to ground, pin 3 to GP18
    gpio_set_dir(18, GPIO_IN);
    gpio_pull_up(18);
    gpio_init(18);

    // "row data" pins 20-13 on GP0-7
    for (uint i = 0; i < 8; i++)
    {
        gpio_set_dir(i, GPIO_IN);
        gpio_pull_up(i);
        gpio_init(i);
    }

    // "column" pins 12-5 on GP8-15
    for (uint i = 8; i < 16; i++)
    {
        gpio_set_dir(i, GPIO_IN);
        gpio_put(i, false);
        gpio_disable_pulls(i);
        gpio_init(i);
    }

    // activate first column
    gpio_set_dir(8, GPIO_OUT);
}

void kb_task()
{
    static absolute_time_t next_scan_us = {0};
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(now, next_scan_us) > 0)
        return;
    next_scan_us = delayed_by_us(now, KB_SCAN_INTERVAL_US);

    uint8_t kb_col_pop[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t kb_row_pop[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    hid_keyboard_modifier_bm_t modifier = 0;

    // read the matrix, one scan of all columns
    for (uint col = 0; col < 8; col++)
    {
        busy_wait_us_32(KB_CAS_US);
        uint8_t row_data = gpio_get_all();
        gpio_set_dir(8 + col, GPIO_IN);
        if (col == 7)
            gpio_set_dir(8, GPIO_OUT);
        else
            gpio_set_dir(9 + col, GPIO_OUT);
        for (uint row = 0; row < 8; row++)
        {
            uint idx = row * 8 + col;
            set_kb_scan(idx, row_data & (1u << row));

            // current modifier ignores ghosted keys
            if (kb_scan[idx].status == 1)
                modifier |= cbm_to_modifier(idx);

            // population count includes ghosted and bouncing keys
            if (kb_scan[idx].status)
            {
                ++kb_col_pop[col];
                ++kb_row_pop[row];
            }
        }
    }

    // RESTORE key is not in matrix
    set_kb_scan(64, gpio_get(18));
    if (kb_scan[64].status > 1)
    {
        kb_scan[64].status = 1;
        kb_scan[64].modifier = modifier;
    }

    // use pop count to find ghosted keys
    for (uint col = 0; col < 8; col++)
    {
        for (uint row = 0; row < 8; row++)
        {
            uint idx = row * 8 + col;
            if (kb_scan[idx].status > 1)
                if (kb_col_pop[col] > 1 && kb_row_pop[row] > 1)
                {
                    if (kb_scan[idx].debounce)
                        kb_scan[idx].status = 1 + KB_GHOST_TICKS;
                    else if (kb_scan[idx].status > 2)
                        --kb_scan[idx].status;
                }
                else if (--kb_scan[idx].status == 1)
                    kb_scan[idx].modifier = modifier;
        }
    }
}

hid_keyboard_modifier_bm_t kb_report(uint8_t keycode_return[6])
{
    static hid_keyboard_modifier_bm_t modifier;
    static struct
    {
        uint8_t keycode;
        uint8_t cbmcode;
    } codes[6];

    bool modifier_locked = false;
    uint code_count = 0;

    // remove released keys
    while (code_count < 6)
    {
        if (!codes[code_count].keycode)
            break;
        if (codes[code_count].keycode >= HID_KEY_A &&
            kb_scan[codes[code_count].cbmcode].status != 1)
        {
            for (uint j = code_count; j < 5; j++)
                codes[j] = codes[j + 1];
            codes[5].keycode = 0;
            continue;
        }
        code_count++;
    }

    // move keys out of queue
    for (uint cbmcode = 0; cbmcode < 65; cbmcode++)
    {
        if (kb_scan[cbmcode].status == 1 && !kb_scan[cbmcode].sent)
        {
            hid_keyboard_modifier_bm_t cbmmod = cbm_to_modifier(cbmcode);
            if (cbmmod)
            { // modifier key
                modifier |= cbmmod;
                modifier_locked = true;
                kb_scan[cbmcode].sent = true;
            }
            else
            { // regular key
                // check for phantom state
                if (code_count >= 6)
                {
                    for (uint idx = 0; idx < 6; idx++)
                        keycode_return[idx] = 1;
                    return modifier;
                }
                // Pressing + and - simultaneously needs to send a shift
                // for the + and no shift for the -. This is impossible,
                // so we leave one queued for the next report.
                if (!modifier_locked || modifier == kb_scan[cbmcode].modifier)
                {
                    modifier = kb_scan[cbmcode].modifier;
                    modifier_locked = true;
                    codes[code_count].cbmcode = cbmcode;
                    codes[code_count].keycode = cbmcode;

                    cbm_translate(&codes[code_count].keycode, &modifier);

                    kb_scan[cbmcode].sent = true;
                    code_count++;
                }
            }
        }
    }

    // Recompute modifier when no regular keys are pressed
    if (!code_count)
    {
        modifier = 0;
        for (uint idx = 0; idx < 65; idx++)
            if (kb_scan[idx].status == 1)
                modifier |= cbm_to_modifier(idx);
    }

    for (uint idx = 0; idx < 6; idx++)
        keycode_return[idx] = codes[idx].keycode;
    return modifier;
}
