#include <avr/wdt.h>
#include "serialTap.h"

// --- Global variable definitions (declared extern in serialTap.h) ---
HardwareSerial *s[3] = { &Serial, &Serial1, &Serial2 };
bool injectMode = false;
bool secondDevice = false;
bool firstMessage = true;
bool debug = false;

//
// newer arduinos keep the watchdog enabled after reset, so we need to
// disable it manually in the pre boot stage
//
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

void wdt_init(void) {
  MCUSR = 0;
  wdt_disable();

  return;
}

void configurePorts(const char arg[], const int i) {
  char baudC[BAUD_BUF_SIZE] = { '\0' };
  char config[4] = { '\0' };
  long baud = 0;

  //
  // BAUDRATE is the only parameter that's variable in length.
  // all other parameters and syntax characters amount to 9 chars.
  // if the user supplies less than 10 characters, the input cannot
  // be valid so we assign default values instead.
  //
  if (i > 9) {
    int blen = i - 9;
    if (blen > (int)sizeof(baudC) - 1) blen = (int)sizeof(baudC) - 1;
    if (blen < 0) blen = 0;

    memcpy(baudC, arg + 3, blen);
    baudC[blen] = '\0';
    baud = atol(baudC);
    //
    // the configuration always has a length of 3 and is located 4
    // chars before the newline character.
    //
    memcpy(config, arg + i - 4, 3);
    config[3] = '\0';
  } else {
    config[0] = '\0';
  }

  uint8_t c;
  if (strcmp(config, "5N1") == 0)
    c = SERIAL_5N1;
  else if (strcmp(config, "5N2") == 0)
    c = SERIAL_5N2;
  else if (strcmp(config, "5E1") == 0)
    c = SERIAL_5E1;
  else if (strcmp(config, "5E2") == 0)
    c = SERIAL_5E2;
  else if (strcmp(config, "5O1") == 0)
    c = SERIAL_5O1;
  else if (strcmp(config, "5O2") == 0)
    c = SERIAL_5O2;
  else if (strcmp(config, "6N1") == 0)
    c = SERIAL_6N1;
  else if (strcmp(config, "6N2") == 0)
    c = SERIAL_6N2;
  else if (strcmp(config, "6E1") == 0)
    c = SERIAL_6E1;
  else if (strcmp(config, "6E2") == 0)
    c = SERIAL_6E2;
  else if (strcmp(config, "6O1") == 0)
    c = SERIAL_6O1;
  else if (strcmp(config, "6O2") == 0)
    c = SERIAL_6O2;
  else if (strcmp(config, "7N1") == 0)
    c = SERIAL_7N1;
  else if (strcmp(config, "7N2") == 0)
    c = SERIAL_7N2;
  else if (strcmp(config, "7E1") == 0)
    c = SERIAL_7E1;
  else if (strcmp(config, "7E2") == 0)
    c = SERIAL_7E2;
  else if (strcmp(config, "7O1") == 0)
    c = SERIAL_7O1;
  else if (strcmp(config, "7O2") == 0)
    c = SERIAL_7O2;
  else if (strcmp(config, "8N1") == 0)
    c = SERIAL_8N1;
  else if (strcmp(config, "8N2") == 0)
    c = SERIAL_8N2;
  else if (strcmp(config, "8E1") == 0)
    c = SERIAL_8E1;
  else if (strcmp(config, "8E2") == 0)
    c = SERIAL_8E2;
  else if (strcmp(config, "8O1") == 0)
    c = SERIAL_8O1;
  else if (strcmp(config, "8O2") == 0)
    c = SERIAL_8O2;
  else {
    //
    // default case. we get here when the user supplies an
    // unsupported configuration.
    //
    c = SERIAL_8N1;
    config[0] = '8';
    config[1] = 'N';
    config[2] = '1';
    config[3] = '\0';
  }

  if (baud < 1 || baud > 2000000)
    baud = 9600;

  s[0]->print("initialising serial ports with these settings: ");
  s[0]->print(baud);
  s[0]->print(" ");
  s[0]->print(config);
  s[0]->println();

  s[1]->begin(baud, c);
  s[2]->begin(baud, c);
}

void help() {
  s[0]->println();
  s[0]->println();
  s[0]->println("--- ARDUINO MEGA SERIAL TAP HELP ---");
  s[0]->println();
  s[0]->println("c (BAUDRATE, CONFIGURATION)   -   configure the RS-232 tap ports");
  s[0]->println("    BAUDRATE can be a number between 1 and 2,000,000");
  s[0]->println("    CONFIGURATION is a 3-character-combination of data bits, parity and stop bits");
  s[0]->println("        supported data bits: 5, 6, 7, 8");
  s[0]->println("        supported parities:  N (none), E (even parity), O (odd parity)");
  s[0]->println("        supported stop bits: 1, 2");
  s[0]->println();
  s[0]->println("    examples:");
  s[0]->println("        c (115200, 8N1)");
  s[0]->println("        c (5579, 6E2)");
  s[0]->println();
  s[0]->println();
  s[0]->println("1 (STRING) / 2 (STRING)   -   inject data into serial communication between tapped devices");
  s[0]->println("    1 (): send data to serial device 1");
  s[0]->println("    2 (): send data to serial device 2");
  s[0]->println();
  s[0]->println("    C escape sequences are supported (except for bytes and unicode)");
  s[0]->println("    a carriage return is appended automatically unless the payload already ends with \\r or \\n");
  s[0]->println();
  s[0]->println("    examples:");
  s[0]->println("        1 (Hello, World!)");
  s[0]->println("        2 (printf (\"Hello, robot!\\n\"))");
  s[0]->println();
  s[0]->println();
  s[0]->println("m (MODE)   -   switch between inject and realtime mode");
  s[0]->println("    change the operating mode of the serial tap.");
  s[0]->println("    in inject mode, all serial data is relayed between the two serial");
  s[0]->println("    devices by the arduino. this allows you to inject data into the communication.");
  s[0]->println();
  s[0]->println("    in realtime mode (the default), both participants have a direct electrical connection to each other.");
  s[0]->println("    this is useful for communication where timing is critical. injection is not possible");
  s[0]->println("    in this mode.");
  s[0]->println();
  s[0]->println("    MODE can be either inject or realtime");
  s[0]->println("        if no mode or an invalid mode is supplied, it will simply print the current mode.");
  s[0]->println();
  s[0]->println();
  s[0]->println("h ()   -   display this help message");
  s[0]->println();
  s[0]->println();
  s[0]->println("debug ()   -   toggle debug mode");
  s[0]->println("    toggle echo of commands received");
  s[0]->println();
  s[0]->println();
  s[0]->println("reset ()   -   reset arduino");
  s[0]->println("    this can be useful when you don't have access to the physical reset button");
}

int setupTrap() {
  int i = 0;
  char arg[SETUP_BUF_SIZE] = { '\0' };
  unsigned long time = 0;

  while (true) {
    if (s[0]->available() > 0) {
      time = millis();
      //
      // we read the input stream byte by byte and cast it to a character.
      // we expect the command to end with a newline character, if we don't
      // get one, we want to stop reading after 20 characters so we don't
      // overflow the array.
      //
      if (i >= (int)sizeof(arg) - 1) {
        break;
      }
      arg[i] = (char)(s[0]->read());
      if (arg[i] == '\n' || arg[i] == '\r') {
        break;
      }
      ++i;
    }
    //
    // if there's no new data after 3 seconds, we assume the data
    // transmission is over.
    //
    else if (time > 0 && millis() - time > CMD_TIMEOUT_MS) {
      break;
    }
  }

  arg[i] = '\0';

  if (strstr(arg, "h ()") == &arg[0]) {
    help();
    return 1;
  } else {
    configurePorts(arg, i);
    return 0;
  }
}

void softReset() {
  cli();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void modeSwitch(const char arg[]) {
  if (strstr(arg, "inject") == &arg[3]) {
    if (injectMode) {
      s[0]->println("device is already in inject mode");
    } else {
      digitalWrite(MODE_PIN, HIGH);
      injectMode = true;
      s[0]->println("device is now in inject mode");
    }
  } else if (strstr(arg, "realtime") == &arg[3]) {
    if (!injectMode) {
      s[0]->println("device is already in realtime mode");
    } else {
      digitalWrite(MODE_PIN, LOW);
      injectMode = false;
      s[0]->println("device is now in realtime mode");
    }
  } else {
    if (injectMode) {
      s[0]->println("device is currently in inject mode");
    } else {
      s[0]->println("device is currently in realtime mode");
    }
  }
}

//
// relay data between serial1 and serial2, writing tap output to the
// console. extracted so it can be called during command input without
// stalling the forwarding path.
//
void relay() {
  if (s[1]->available() > 0) {
    if (secondDevice) {
      secondDevice = false;
      s[0]->print("\n1: ");
    } else if (firstMessage) {
      firstMessage = false;
      s[0]->print("\n1: ");
    }

    static uint8_t buf1[SERIAL_RX_BUFFER_SIZE];
    int n = min(s[1]->available(), (int)sizeof(buf1));
    for (int i = 0; i < n; ++i) {
      buf1[i] = (uint8_t)s[1]->read();
    }
    if (injectMode) {
      s[2]->write(buf1, n);
    }
    for (int i = 0; i < n; ++i) {
      if (debug) {
        s[0]->print("<");
        s[0]->print(buf1[i], HEX);
        s[0]->print(">");
      }
      if (buf1[i] == '\r') {
        s[0]->write('\r');
        s[0]->write('\n');
      } else {
        s[0]->write(buf1[i]);
      }
    }
  }

  if (s[2]->available() > 0) {
    if (!secondDevice) {
      secondDevice = true;
      firstMessage = false;
      s[0]->print("\n2: ");
    } else if (firstMessage) {
      firstMessage = false;
      s[0]->print("\n2: ");
    }

    static uint8_t buf2[SERIAL_RX_BUFFER_SIZE];
    int n = min(s[2]->available(), (int)sizeof(buf2));
    for (int i = 0; i < n; ++i) {
      buf2[i] = (uint8_t)s[2]->read();
    }
    if (injectMode) {
      s[1]->write(buf2, n);
    }
    for (int i = 0; i < n; ++i) {
      if (debug) {
        s[0]->print("<");
        s[0]->print(buf2[i], HEX);
        s[0]->print(">");
      }
      if (buf2[i] == '\r') {
        s[0]->write('\r');
        s[0]->write('\n');
      } else {
        s[0]->write(buf2[i]);
      }
    }
  }
}

void setup() {
  int trapState = 1;

  s[0]->begin(USB_BAUD);
  s[0]->println("--- ARDUINO MEGA SERIAL TAP ---");
  s[0]->print("USB baud rate: ");
  s[0]->println(USB_BAUD);
  s[0]->println("to configure, type \"c (BAUDRATE, CONFIGURATION)\"");
  s[0]->println("for a list of available commands and further explanation, type \"h ()\"");
  //
  // we start the arduino in realtime mode
  //
  pinMode(MODE_PIN, OUTPUT);
  digitalWrite(MODE_PIN, LOW);
  //
  // make sure we stay in setup until a configuration is set.
  // calling h () should not trigger a jump to loop ().
  //
  while (trapState == 1) {
    trapState = setupTrap();
  }
}

void loop() {
  //
  // static buffers: avoids placing ~4KB on the stack every loop() call,
  // which risks stack-heap collision on the Mega's 8KB SRAM.
  //
  static char arg[CMD_BUF_SIZE];
  static char send[SEND_BUF_SIZE];

  if (s[0]->available() > 0) {
    int i = 0;
    unsigned long time = 0;
    char currentByte = '\0';
    bool escape = false;
    bool flagged = false;
    bool autoCR = true;

    memset(arg, 0, sizeof(arg));
    memset(send, 0, sizeof(send));

    while (true) {
      if (s[0]->available() > 0) {
        //
        // break before array overflow.
        //
        if (i >= (int)sizeof(arg) - 1) {
          flagged = true;
          break;
        }

        time = millis();
        currentByte = (char)(s[0]->read());

        if (debug) {
          // echo local commands typed
          if (currentByte == '\r' || currentByte == '\n') {
            s[0]->println();
          } else if (currentByte == 0x08 || currentByte == 0x7F) {
            // backspace or delete
            s[0]->write('\b');
            s[0]->write(' ');
            s[0]->write('\b');
          } else {
            s[0]->write(currentByte);
          }
        }

        //
        // handle backspace/delete: remove the previous character from
        // the buffer instead of storing the control code.
        //
        if (currentByte == 0x08 || currentByte == 0x7F) {
          if (i > 0) {
            --i;
          }
          continue;
        }

        if (currentByte == '\\' && !escape) {
          escape = true;
          continue;
        } else {
          //
          // handle escape sequences. escape was set in the last iteration,
          // the counter was not increased. leading backslash does not appear
          // in the array.
          //
          if (escape) {
            escape = false;
            switch (currentByte) {
              case 'a':
                //
                // alert
                //
                arg[i] = (char)0x07;
                break;
              case 'b':
                //
                // backspace
                //
                arg[i] = (char)0x08;
                break;
              case 'e':
                //
                // escape
                //
                arg[i] = (char)0x1B;
                break;
              case 'f':
                //
                // form feed
                //
                arg[i] = (char)0x0C;
                break;
              case 'n':
                //
                // line feed
                //
                arg[i] = (char)0x0A;
                break;
              case 'r':
                //
                // carriage return
                //
                arg[i] = (char)0x0D;
                break;
              case 't':
                //
                // horizontal tab
                //
                arg[i] = (char)0x09;
                break;
              case 'v':
                //
                // vertical tab
                //
                arg[i] = (char)0x0B;
                break;
              default:
                //
                // all other escape sequences simply require the backslash in front
                // of them to be removed. unsupported escape sequences will be
                // treated as normal characters, without the leading backslash.
                //
                arg[i] = currentByte;
            }
          } else {
            arg[i] = currentByte;
          }
        }
        //
        // we expect only one newline per command
        //
        if (currentByte == '\n' || currentByte == '\r') {
          break;
        }
        ++i;
      } else {
        relay();
        //
        // if we get here that usually means we received an invalid command.
        // however, if the user does not send newlines at the end of a
        // command, we still want to continue execution after 3 seconds.
        //
        if (time > 0 && millis() - time > CMD_TIMEOUT_MS) {
          s[0]->println("continuing after timeout. possibly missing newline at EOL?");
          break;
        }
      }
    }
    arg[i] = '\0';
    //
    // when there's a buffer overflow, the input buffer might be empty although
    // there's still more data on the way
    //
    if (flagged)
      delay(200);

    if (s[0]->available()) {
      //
      // we get here when there's still data in the pipeline but we already
      // exited the read loop. this happens when there's a newline character
      // in the command string before EOL or when we receive more than 2048
      // bytes. We flush the pipeline and flag the command to prevent execution.
      // in case of a buffer overflow, the command is already flagged.
      //
      while (s[0]->available() > 0) {
        s[0]->read();
      }

      if (flagged) {
        s[0]->println("buffer overflow. the command size limit is 1024 bytes");
      } else {
        s[0]->println("malformed command");
      }

      flagged = true;
    }
    if (debug) {
      s[0]->print("received command: ");
      s[0]->print(arg);
      s[0]->println();
    }
    if (!flagged) {
      //
      // after printing a command output to the console, we definitely want
      // to print the sender index again
      //
      firstMessage = true;

      if (strstr(arg, "h ()") == &arg[0]) {
        help();
      } else if (strstr(arg, "c (") == &arg[0]) {
        s[1]->end();
        s[2]->end();
        s[0]->print("re-");
        configurePorts(arg, i);
      } else if (strstr(arg, "1 (") == &arg[0]) {
        if (!injectMode) {
          s[0]->println("device is in realtime mode. data injection is not possible.");
          s[0]->println("to change the mode to inject mode, type \"m (inject)\".");
        } else {
          int slen = 0;
          const char *rp = strrchr(arg, ')');

          if (rp == NULL || rp < &arg[3]) {
            s[0]->println("malformed command");
            flagged = true;
          } else {
            slen = (int)(rp - (arg + 3));
            if (slen > (int)sizeof(send) - 1)
              slen = (int)sizeof(send) - 1;

            memcpy(send, arg + 3, slen);
            send[slen] = '\0';
          }

          if (!flagged) {
            s[0]->print("sending to device 1: ");
            s[0]->print(send);
            s[1]->write((const uint8_t *)send, (size_t)slen);
            if (autoCR && (slen == 0 || (send[slen - 1] != '\r' && send[slen - 1] != '\n'))) {
              s[1]->write((uint8_t)'\r');
            }
            s[0]->println();
          }
        }

      } else if (strstr(arg, "2 (") == &arg[0]) {
        if (!injectMode) {
          s[0]->println("device is in realtime mode. data injection is not possible.");
          s[0]->println("to change the mode to inject mode, type \"m (inject)\".");
        } else {
          int slen = 0;
          const char *rp = strrchr(arg, ')');

          if (rp == NULL || rp < &arg[3]) {
            s[0]->println("malformed command");
            flagged = true;
          } else {
            slen = (int)(rp - (arg + 3));
            if (slen > (int)sizeof(send) - 1)
              slen = (int)sizeof(send) - 1;

            memcpy(send, arg + 3, slen);
            send[slen] = '\0';
          }

          if (!flagged) {
            s[0]->print("sending to device 2: ");
            s[0]->print(send);
            s[2]->write((const uint8_t *)send, (size_t)slen);
            if (autoCR && (slen == 0 || (send[slen - 1] != '\r' && send[slen - 1] != '\n'))) {
              s[2]->write((uint8_t)'\r');
            }
            s[0]->println();
          }
        }
      } else if (strstr(arg, "debug ()") == &arg[0]) {
        if (!debug) {
          debug = true;
          s[0]->println("debug mode enabled");
        } else {
          debug = false;
          s[0]->println("debug mode disabled");
        }
      } else if (strstr(arg, "reset ()") == &arg[0]) {
        softReset();
      } else if (strstr(arg, "m (") == &arg[0]) {
        modeSwitch(arg);
      } else {
        s[0]->println("invalid command");
      }
    }
  }
  //
  // forwarding data between the two serial ports and simultaneously
  // writing it to our console
  //
  relay();
}
