#ifndef SERIAL_TAP_H
#define SERIAL_TAP_H

#include <Arduino.h>

// --- Pin definitions ---
static const uint8_t MODE_PIN = 2;

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
