// CA53Y Comms Display: build settings
// Most people never need to touch this file. The two settings you might change are
// CA53Y_PANEL (which screen your board has) and DISPLAY_ROTATION.
#pragma once

#define FW_VERSION "1.0.0"

// ---------- which board ----------
// The "2.8inch ESP32-32E" board comes in two versions that look the same:
//   0 = E32R28T   (2024 boards, ILI9341 screen)  <- most boards sold as "ESP32-32E 2.8in"
//   1 = E32R28T-1 (2025 boards, ST7789 screen)
// Flashed the wrong one? The screen stays white/black or the colors look negative.
// Switch this, or pick the other file in the web flasher.
#ifndef CA53Y_PANEL
#define CA53Y_PANEL 0
#endif

#ifndef PANEL_INVERT                 // colors look like a photo negative? flip this
#if CA53Y_PANEL == 1
#define PANEL_INVERT true
#else
#define PANEL_INVERT false
#endif
#endif

#ifndef LCD_SPI_HZ                   // lines or speckles on the screen? lower this (20000000 is very safe)
#define LCD_SPI_HZ 27000000
#endif

#define DISPLAY_ROTATION 0           // 0 = portrait, 2 = portrait upside down

// ---------- radio (must match every CA53Y Comms device) ----------
#define ESPNOW_CHANNEL  1            // Wi-Fi channel all CA53Y devices talk on
#define MAX_HOPS        8            // how many repeaters a message may pass through
#define SEND_TRIES      6            // direct-message retries before "not delivered"
#define RETRY_MS        1200         // long enough for a receipt to come back over several hops
#define CHAN_SENDS      2            // channel messages are sent this many times (no receipts)
#define BEACON_MS       3000         // how often we announce ourselves when visible
#define NEARBY_TTL_MS   15000        // drop devices from Nearby after this long without hearing them
#define KDF_ITERS       2000         // PBKDF2 rounds for PINs and channel passphrases

// ---------- limits ----------
#define MAX_CONTACTS    16
#define MAX_CHANNELS    8
#define MAX_NEARBY      20
#define MAX_MSGS        60           // kept in RAM, cleared on restart
#define MAX_OUT         8
#define MAX_TEXT        160          // direct messages, bytes
#define MAX_CHAN_TEXT   140          // channel messages carry the sender's name too

// ---------- board pins (E32R28T / E32R28T-1) ----------
#define PIN_LED_R       22           // RGB LED, common anode: LOW = on
#define PIN_LED_G       16
#define PIN_LED_B       17
#define PIN_SPEAKER     26           // speaker connector (DAC pin)
#define PIN_AMP_EN      4            // audio amp enable, LOW = on
#define PIN_BOOT        0            // hold 5 s to erase everything
