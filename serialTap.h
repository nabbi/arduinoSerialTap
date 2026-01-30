#ifndef SERIAL_TAP_H
#define SERIAL_TAP_H

// --- Serial RX buffer size override ---
// the default arduino rx buffer is 64 bytes, which overflows easily at
// high baud rates. this must be defined before including Arduino.h.
// valid sizes: 64, 128, 256 (must be power of 2)
#define SERIAL_RX_BUFFER_SIZE 256

#include <Arduino.h>

// --- Pin definitions ---
static const uint8_t MODE_PIN = 2;

// --- USB console baud rate ---
// the mega 2560 supports up to 2000000. if your serial monitor can't
// keep up, lower this to 115200.
static const long USB_BAUD = 500000;

// --- Buffer sizes ---
static const int CMD_BUF_SIZE  = 2048;
static const int SEND_BUF_SIZE = 2044;
static const int SETUP_BUF_SIZE = 20;
static const int BAUD_BUF_SIZE = 8;

// --- Timeouts (ms) ---
static const unsigned long CMD_TIMEOUT_MS = 3000;

// --- Globals ---
extern HardwareSerial *s[3];
extern bool injectMode;
extern bool secondDevice;
extern bool firstMessage;
extern bool debug;

#endif // SERIAL_TAP_H
