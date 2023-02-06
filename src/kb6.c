/*
 * Copyright (c) 2022 Rumbledethumps
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "tusb.h"

// Debounce and ghost detection added.
// Keycode mappings for ASCII, VICE, and MiSTer.
// Includes edge case for CRSR keys.

#define KB_CAS_US 6
#define KB_SCAN_INTERVAL_US 200
#define KB_GHOST_US 2000
#define KB_DEBOUNCE_US 5000

#define KB_GHOST_TICKS ((KB_GHOST_US + KB_SCAN_INTERVAL_US - 1) / KB_SCAN_INTERVAL_US)
#define KB_DEBOUNCE_TICKS ((KB_DEBOUNCE_US + KB_SCAN_INTERVAL_US - 1) / KB_SCAN_INTERVAL_US)

// Until MiSTer allows for custom remapping, we do a toggle.
// MiSTer global keyboard remapping will not do what we need.
static bool is_mister = false; // can be true if you prefer

static struct cbm_scan
{
    uint8_t status;   // 0=open, 1=pressed, 2-255=ghost
    uint8_t debounce; // countdown to 0
    bool sent;
    hid_keyboard_modifier_bm_t modifier;
} cbm_scan[65];

// Default keycode translations are the positional mapping used by MiSTer.
// These keycodes are unique to how the Pi Pico is wired and scanned.
static const uint8_t CBM_TO_HID[] = {
    HID_KEY_1, HID_KEY_GRAVE, HID_KEY_CONTROL_LEFT, HID_KEY_ESCAPE,              // 0-3
    HID_KEY_SPACE, HID_KEY_ALT_LEFT, HID_KEY_Q, HID_KEY_2,                       // 4-7
    HID_KEY_3, HID_KEY_W, HID_KEY_A, HID_KEY_SHIFT_LEFT,                         // 8-11
    HID_KEY_Z, HID_KEY_S, HID_KEY_E, HID_KEY_4,                                  // 12-15
    HID_KEY_5, HID_KEY_R, HID_KEY_D, HID_KEY_X,                                  // 16-19
    HID_KEY_C, HID_KEY_F, HID_KEY_T, HID_KEY_6,                                  // 20-23
    HID_KEY_7, HID_KEY_Y, HID_KEY_G, HID_KEY_V,                                  // 24-27
    HID_KEY_B, HID_KEY_H, HID_KEY_U, HID_KEY_8,                                  // 28-31
    HID_KEY_9, HID_KEY_I, HID_KEY_J, HID_KEY_N,                                  // 32-35
    HID_KEY_M, HID_KEY_K, HID_KEY_O, HID_KEY_0,                                  // 36-39
    HID_KEY_EQUAL, HID_KEY_P, HID_KEY_L, HID_KEY_COMMA,                          // 40-43
    HID_KEY_PERIOD, HID_KEY_SEMICOLON, HID_KEY_BRACKET_LEFT, HID_KEY_MINUS,      // 44-47
    HID_KEY_BACKSLASH, HID_KEY_BRACKET_RIGHT, HID_KEY_APOSTROPHE, HID_KEY_SLASH, // 48-51
    HID_KEY_SHIFT_RIGHT, HID_KEY_END, HID_KEY_PAGE_DOWN, HID_KEY_HOME,           // 52-55
    HID_KEY_DELETE, HID_KEY_ENTER, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_DOWN,      // 56-59
    HID_KEY_F1, HID_KEY_F3, HID_KEY_F5, HID_KEY_F7,                              // 60-63
    HID_KEY_F11                                                                  // 64
};
// All the keys except letters
#define CBM_KEY_1 0
#define CBM_KEY_2 7
#define CBM_KEY_3 8
#define CBM_KEY_4 15
#define CBM_KEY_5 16
#define CBM_KEY_6 23
#define CBM_KEY_7 24
#define CBM_KEY_8 31
#define CBM_KEY_9 32
#define CBM_KEY_0 39
#define CBM_KEY_ARROW_LEFT 1
#define CBM_KEY_CONTROL_LEFT 2
#define CBM_KEY_RUN_STOP 3
#define CBM_KEY_SPACE 4
#define CBM_KEY_CBM 5 // C= key
#define CBM_KEY_SHIFT_LEFT 11
#define CBM_KEY_PLUS 40
#define CBM_KEY_COMMA 43
#define CBM_KEY_PERIOD 44
#define CBM_KEY_COLON 45
#define CBM_KEY_COMMERCIAL_AT 46
#define CBM_KEY_MINUS 47
#define CBM_KEY_STERLING 48
#define CBM_KEY_ASTERISK 49
#define CBM_KEY_SEMICOLON 50
#define CBM_KEY_SLASH 51
#define CBM_KEY_SHIFT_RIGHT 52
#define CBM_KEY_EQUAL 53
#define CBM_KEY_ARROW_UP 54
#define CBM_KEY_HOME 55
#define CBM_KEY_DEL 56
#define CBM_KEY_RETURN 57
#define CBM_KEY_CRSR_RIGHT 58
#define CBM_KEY_CRSR_DOWN 59
#define CBM_KEY_F1 60
#define CBM_KEY_F3 61
#define CBM_KEY_F5 62
#define CBM_KEY_F7 63
#define CBM_KEY_RESTORE 64

// Translate CBM code into USB HID keyboard modifier bitmap
static hid_keyboard_modifier_bm_t cbm_to_modifier(uint8_t cbmcode)
{
    uint8_t keycode = CBM_TO_HID[cbmcode];
    if (!is_mister && keycode == HID_KEY_ALT_LEFT)
        return 0;
    if (keycode >= HID_KEY_CONTROL_LEFT && keycode <= HID_KEY_GUI_RIGHT)
        return 1 << (keycode & 7);
    return 0;
}

// These overrides makes the C64 keyboard suitable for ASCII.
static void cbm_translate_ascii(uint8_t *code, hid_keyboard_modifier_bm_t *modifier)
{
    uint8_t cbmcode = *code;
    *code = CBM_TO_HID[cbmcode];
    if (*modifier == (KEYBOARD_MODIFIER_LEFTCTRL |
                      KEYBOARD_MODIFIER_LEFTSHIFT |
                      KEYBOARD_MODIFIER_RIGHTSHIFT))
        switch (cbmcode)
        {
        case CBM_KEY_STERLING:
            *code = HID_KEY_SHIFT_RIGHT;
            is_mister = true;
            return;
        case CBM_KEY_DEL:
            *modifier = KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTALT;
            *code = HID_KEY_DELETE;
            return;
        case CBM_KEY_F1:
            *code = HID_KEY_F9;
            *modifier = 0;
            break;
        case CBM_KEY_F3:
            *code = HID_KEY_F10;
            *modifier = 0;
            break;
        case CBM_KEY_F5:
            *code = HID_KEY_F11;
            *modifier = 0;
            break;
        case CBM_KEY_F7:
            *code = HID_KEY_F12;
            *modifier = 0;
            break;
        }
    const hid_keyboard_modifier_bm_t SHIFT =
        KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT;
    if (*modifier & SHIFT)
        switch (cbmcode)
        {
        case CBM_KEY_2: // "
            *code = HID_KEY_APOSTROPHE;
            break;
        case CBM_KEY_6: // &
            *code = HID_KEY_7;
            break;
        case CBM_KEY_7: // '
            *code = HID_KEY_APOSTROPHE;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_8: // (
            *code = HID_KEY_9;
            break;
        case CBM_KEY_9: // )
            *code = HID_KEY_0;
            break;
        case CBM_KEY_0:
            *code = HID_KEY_F12;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_PLUS:
            *code = HID_KEY_PAGE_UP;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_MINUS:
            *code = HID_KEY_PAGE_DOWN;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_COLON:
            *code = HID_KEY_BRACKET_LEFT;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_STERLING:
            *code = HID_KEY_MINUS; // _
            break;
        case CBM_KEY_SEMICOLON:
            *code = HID_KEY_BRACKET_RIGHT;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_ARROW_UP:
            *code = HID_KEY_GRAVE; // ~
            break;
        case CBM_KEY_HOME:
            *code = HID_KEY_END;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_DEL:
            *code = HID_KEY_INSERT;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_CRSR_RIGHT:
            *code = HID_KEY_ARROW_LEFT;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_CRSR_DOWN:
            *code = HID_KEY_ARROW_UP;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_F1:
            *code = HID_KEY_F2;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_F3:
            *code = HID_KEY_F4;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_F5:
            *code = HID_KEY_F6;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_F7:
            *code = HID_KEY_F8;
            *modifier &= ~SHIFT;
            break;
        }
    else // Not SHIFT
        switch (cbmcode)
        {
        case CBM_KEY_PLUS:
            *code = HID_KEY_EQUAL;
            *modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            break;
        case CBM_KEY_MINUS:
            *code = HID_KEY_MINUS;
            break;
        case CBM_KEY_COLON:
            *code = HID_KEY_SEMICOLON;
            *modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            break;
        case CBM_KEY_COMMERCIAL_AT:
            *code = HID_KEY_2;
            *modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            break;
        case CBM_KEY_STERLING:
            *code = HID_KEY_GRAVE;
            break;
        case CBM_KEY_ASTERISK:
            *code = HID_KEY_8;
            *modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            break;
        case CBM_KEY_SEMICOLON:
            *code = HID_KEY_SEMICOLON;
            break;
        case CBM_KEY_ARROW_UP: // ^
            *code = HID_KEY_6;
            *modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            break;
        case CBM_KEY_DEL:
            *code = HID_KEY_BACKSPACE;
            break;
        }
    // Overrides for both SHIFT states.
    switch (cbmcode)
    {
    case CBM_KEY_ARROW_LEFT:
        *code = HID_KEY_DELETE;
        break;
    case CBM_KEY_CBM:
        *code = HID_KEY_TAB;
        break;
    case CBM_KEY_RESTORE:
        *code = HID_KEY_BACKSLASH;
        break;
    case CBM_KEY_EQUAL:
        *code = HID_KEY_EQUAL;
        *modifier &= ~SHIFT;
        break;
    }
}

static void cbm_translate_mister(uint8_t *code, hid_keyboard_modifier_bm_t *modifier)
{
    uint8_t cbmcode = *code;
    *code = CBM_TO_HID[cbmcode];
    if (*modifier == (KEYBOARD_MODIFIER_LEFTCTRL |
                      KEYBOARD_MODIFIER_LEFTSHIFT |
                      KEYBOARD_MODIFIER_RIGHTSHIFT))
        switch (cbmcode)
        {
        case CBM_KEY_STERLING:
            *code = HID_KEY_SHIFT_RIGHT;
            is_mister = false;
            return;
        case CBM_KEY_DEL:
            *modifier = KEYBOARD_MODIFIER_LEFTCTRL |
                        KEYBOARD_MODIFIER_LEFTALT |
                        KEYBOARD_MODIFIER_RIGHTALT;
            *code = HID_KEY_ALT_RIGHT;
            return;
        }
    const hid_keyboard_modifier_bm_t SHIFT =
        KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT;
    if (*modifier & SHIFT)
        switch (cbmcode)
        {
        case CBM_KEY_6: // &
            *code = HID_KEY_7;
            break;
        case CBM_KEY_7: // '
            *code = HID_KEY_6;
            break;
        case CBM_KEY_8: // (
            *code = HID_KEY_9;
            break;
        case CBM_KEY_9: // )
            *code = HID_KEY_0;
            break;
        case CBM_KEY_0:
            *code = HID_KEY_F12;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_CRSR_RIGHT:
            *code = HID_KEY_ARROW_LEFT;
            *modifier &= ~SHIFT;
            break;
        case CBM_KEY_CRSR_DOWN:
            *code = HID_KEY_ARROW_UP;
            *modifier &= ~SHIFT;
            break;
        }
}

static void set_cbm_scan(uint idx, bool is_up)
{
    if (cbm_scan[idx].debounce)
        --cbm_scan[idx].debounce;
    if (is_up)
    {
        if (!cbm_scan[idx].debounce)
        {
            cbm_scan[idx].status = 0;
            cbm_scan[idx].sent = false;
        }
    }
    else
    {
        if (!cbm_scan[idx].status)
        {
            cbm_scan[idx].status = 1 + KB_GHOST_TICKS;
            cbm_scan[idx].debounce = KB_DEBOUNCE_TICKS;
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
        gpio_set_dir(8 + col, GPIO_OUT);
        busy_wait_us_32(KB_CAS_US);
        uint8_t row_data = gpio_get_all();
        gpio_set_dir(8 + col, GPIO_IN);
        for (uint row = 0; row < 8; row++)
        {
            uint idx = row * 8 + col;
            set_cbm_scan(idx, row_data & (1u << row));

            // current modifier ignores ghosted keys
            if (cbm_scan[idx].status == 1)
                modifier |= cbm_to_modifier(idx);

            // population count includes ghosted and bouncing keys
            if (cbm_scan[idx].status)
            {
                ++kb_col_pop[col];
                ++kb_row_pop[row];
            }
        }
    }

    // RESTORE key is not in matrix
    set_cbm_scan(CBM_KEY_RESTORE, gpio_get(18));
    if (cbm_scan[CBM_KEY_RESTORE].status > 1)
    {
        cbm_scan[CBM_KEY_RESTORE].status = 1;
        cbm_scan[CBM_KEY_RESTORE].modifier = modifier;
    }

    // use pop count to find ghosted keys
    for (uint col = 0; col < 8; col++)
    {
        for (uint row = 0; row < 8; row++)
        {
            uint idx = row * 8 + col;
            if (cbm_scan[idx].status > 1)
                if (kb_col_pop[col] > 1 && kb_row_pop[row] > 1)
                {
                    if (cbm_scan[idx].debounce)
                        cbm_scan[idx].status = 1 + KB_GHOST_TICKS;
                    else if (cbm_scan[idx].status > 2)
                        --cbm_scan[idx].status;
                }
                else if (--cbm_scan[idx].status == 1)
                    cbm_scan[idx].modifier = modifier;
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
            cbm_scan[codes[code_count].cbmcode].status != 1)
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
        if (cbm_scan[cbmcode].status == 1 && !cbm_scan[cbmcode].sent)
        {
            if (!cbm_to_modifier(cbmcode)) // regular keys only
            {
                // check for phantom state
                if (code_count >= 6)
                {
                    for (uint idx = 0; idx < 6; idx++)
                        keycode_return[idx] = 1;
                    return 0;
                }
                // Pressing + and - in the same report period needs to send a shift
                // for the + and no shift for the -. This is impossible,
                // so we leave one queued for the next report.
                hid_keyboard_modifier_bm_t this_modifier = cbm_scan[cbmcode].modifier;
                if (!modifier_locked || modifier == this_modifier)
                {
                    uint8_t keycode = cbmcode;
                    if (is_mister)
                        cbm_translate_mister(&keycode, &this_modifier);
                    else
                        cbm_translate_ascii(&keycode, &this_modifier);
                    // Pressing ; and ; simultaneously is the same key with
                    // different shift states. When this is detected, release
                    // the held key so it can be repressed in the next repoort.
                    bool ok = true;
                    for (uint i = 0; i < 6; i++)
                    {
                        if (codes[i].keycode == keycode)
                        {
                            for (uint j = i; j < 5; j++)
                                codes[j] = codes[j + 1];
                            codes[5].keycode = 0;
                            --code_count;
                            ok = false;
                        }
                    }
                    if (ok)
                    {
                        modifier = this_modifier;
                        modifier_locked = true;
                        codes[code_count].cbmcode = cbmcode;
                        codes[code_count].keycode = keycode;
                        cbm_scan[cbmcode].sent = true;
                        code_count++;
                    }
                }
            }
        }
    }

    // Recompute modifiers in certain situations.
    static hid_keyboard_modifier_bm_t previous_modifier = 0;
    if (!modifier_locked)
    {
        hid_keyboard_modifier_bm_t current_modifier = 0;
        for (uint idx = 0; idx < 65; idx++)
            if (cbm_scan[idx].status == 1)
                current_modifier |= cbm_to_modifier(idx);
        if (code_count == 0)
            modifier = current_modifier;
        if (code_count == 1 && previous_modifier != current_modifier)
        {
            // Changing the SHIFT state while a key is being held down
            // should usually do nothing, but we need special handling
            // in a couple of situations unique to the CBM keyboard.
            switch (codes[0].cbmcode)
            {
            // This fixes "C= then SHIFT" when the C= is TAB
            case CBM_KEY_CBM:
                modifier = current_modifier;
                break;
            // This fixes holding CRSR key while SHIFT changes
            case CBM_KEY_CRSR_DOWN:
            case CBM_KEY_CRSR_RIGHT:
                code_count--;
                set_cbm_scan(codes[code_count].cbmcode, true);
                codes[code_count].keycode = 0;
                break;
            }
        }
        previous_modifier = current_modifier;
    }

    // Return new report
    for (uint idx = 0; idx < 6; idx++)
        keycode_return[idx] = codes[idx].keycode;
    return modifier;
}
