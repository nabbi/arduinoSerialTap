// Host-buildable unit tests for serialTap parsing logic.
// Build: g++ -std=c++11 -Wall -Wextra -o test_parsing test_parsing.cpp && ./test_parsing

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>

// --- Extracted escape-sequence logic for testability ---
// Returns the resolved character for a given escape letter.
// Mirrors the switch in loop().
static char resolveEscape(char c) {
  switch (c) {
    case 'a': return (char)0x07;
    case 'b': return (char)0x08;
    case 'e': return (char)0x1B;
    case 'f': return (char)0x0C;
    case 'n': return (char)0x0A;
    case 'r': return (char)0x0D;
    case 't': return (char)0x09;
    case 'v': return (char)0x0B;
    default:  return c;
  }
}

// Simulate the input-processing loop: escape sequences + backspace handling.
// Writes result into out (must be at least len+1 bytes). Returns output length.
static int processInput(const char *input, int len, char *out, int outSize) {
  int i = 0;
  bool escape = false;

  for (int pos = 0; pos < len; ++pos) {
    char c = input[pos];

    // backspace/delete
    if (c == 0x08 || c == 0x7F) {
      if (i > 0) --i;
      continue;
    }

    if (c == '\\' && !escape) {
      escape = true;
      continue;
    }

    if (i >= outSize - 1) break;

    if (escape) {
      escape = false;
      out[i] = resolveEscape(c);
    } else {
      out[i] = c;
    }

    if (c == '\n' || c == '\r') break;
    ++i;
  }
  out[i] = '\0';
  return i;
}

// --- Tests ---

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
  tests_run++; \
  printf("  %-40s ", #name); \
  } while(0)

#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

static void test_escape_newline() {
  TEST(escape_newline);
  char out[64];
  int n = processInput("hello\\nworld\n", 13, out, sizeof(out));
  // "hello" + 0x0A + "world" = 11 chars, then stops at \n
  assert(n == 11);
  assert(out[5] == '\n');
  assert(memcmp(out, "hello\nworld", 11) == 0);
  PASS();
}

static void test_escape_tab() {
  TEST(escape_tab);
  char out[64];
  processInput("a\\tb\n", 5, out, sizeof(out));
  assert(out[0] == 'a');
  assert(out[1] == '\t');
  assert(out[2] == 'b');
  PASS();
}

static void test_backspace_removes_char() {
  TEST(backspace_removes_char);
  char out[64];
  // Type "ab", backspace, "c" -> should be "ac"
  char input[] = {'a', 'b', 0x08, 'c', '\n'};
  int n = processInput(input, 5, out, sizeof(out));
  assert(n == 2);
  assert(out[0] == 'a');
  assert(out[1] == 'c');
  PASS();
}

static void test_backspace_at_start() {
  TEST(backspace_at_start_is_noop);
  char out[64];
  char input[] = {0x7F, 'a', '\n'};
  int n = processInput(input, 3, out, sizeof(out));
  assert(n == 1);
  assert(out[0] == 'a');
  PASS();
}

static void test_multiple_backspaces() {
  TEST(multiple_backspaces);
  char out[64];
  char input[] = {'a', 'b', 'c', 0x08, 0x08, 'd', '\n'};
  int n = processInput(input, 7, out, sizeof(out));
  assert(n == 2);
  assert(out[0] == 'a');
  assert(out[1] == 'd');
  PASS();
}

static void test_escaped_backslash() {
  TEST(escaped_backslash);
  char out[64];
  int n = processInput("a\\\\b\n", 5, out, sizeof(out));
  assert(n == 3);
  assert(out[0] == 'a');
  assert(out[1] == '\\');
  assert(out[2] == 'b');
  PASS();
}

static void test_buffer_overflow_protection() {
  TEST(buffer_overflow_protection);
  char out[4];
  int n = processInput("abcdefgh\n", 9, out, sizeof(out));
  assert(n == 3); // outSize-1 = 3
  assert(out[3] == '\0');
  PASS();
}

static void test_delete_key() {
  TEST(delete_key_0x7F);
  char out[64];
  char input[] = {'x', 'y', 0x7F, 'z', '\n'};
  int n = processInput(input, 5, out, sizeof(out));
  assert(n == 2);
  assert(out[0] == 'x');
  assert(out[1] == 'z');
  PASS();
}

// --- Performance / resiliency tests ---

static void test_max_buffer_throughput() {
  TEST(max_buffer_throughput);
  // Fill a CMD_BUF_SIZE-sized input with printable chars, terminated by \n
  const int SZ = 2048;
  char input[SZ + 1];
  char out[SZ + 1];
  for (int i = 0; i < SZ - 1; i++) input[i] = 'A' + (i % 26);
  input[SZ - 1] = '\n';
  input[SZ] = '\0';
  int n = processInput(input, SZ, out, SZ + 1);
  assert(n == SZ - 1);
  assert(out[0] == 'A');
  assert(out[SZ - 2] == (char)('A' + (SZ - 2) % 26));
  PASS();
}

static void test_buffer_exact_capacity() {
  TEST(buffer_exact_capacity);
  // outSize exactly matches input length + 1 (no room to spare)
  char out[6]; // room for 5 chars + null
  int n = processInput("abcde\n", 6, out, sizeof(out));
  assert(n == 5);
  assert(memcmp(out, "abcde", 5) == 0);
  assert(out[5] == '\0');
  PASS();
}

static void test_buffer_one_byte_short() {
  TEST(buffer_one_byte_short);
  // outSize is 1 byte too small for the full input
  char out[5]; // room for 4 chars + null
  int n = processInput("abcde\n", 6, out, sizeof(out));
  assert(n == 4); // truncated
  assert(out[4] == '\0');
  PASS();
}

static void test_all_escapes_stress() {
  TEST(all_escapes_stress);
  // Input is entirely escape sequences: worst-case for the escape path
  // 512 escape pairs = 1024 input bytes -> 512 output bytes
  const int PAIRS = 512;
  char input[PAIRS * 2 + 1];
  char out[PAIRS + 1];
  for (int i = 0; i < PAIRS; i++) {
    input[i * 2]     = '\\';
    input[i * 2 + 1] = 'n';
  }
  input[PAIRS * 2] = '\0';
  int n = processInput(input, PAIRS * 2, out, sizeof(out));
  assert(n == PAIRS);
  for (int i = 0; i < PAIRS; i++) assert(out[i] == '\n');
  PASS();
}

static void test_all_backspaces_stress() {
  TEST(all_backspaces_stress);
  // Type a char then immediately backspace, repeated many times -> empty output
  const int REPS = 500;
  char input[REPS * 2 + 1];
  for (int i = 0; i < REPS; i++) {
    input[i * 2]     = 'x';
    input[i * 2 + 1] = 0x08;
  }
  input[REPS * 2] = '\n';
  char out[REPS + 1];
  int n = processInput(input, REPS * 2 + 1, out, sizeof(out));
  assert(n == 0);
  PASS();
}

static void test_backspace_heavier_than_input() {
  TEST(backspace_heavier_than_input);
  // More backspaces than chars typed - index must never go negative
  char input[20];
  int pos = 0;
  input[pos++] = 'a';
  for (int i = 0; i < 10; i++) input[pos++] = 0x08;
  input[pos++] = 'z';
  input[pos++] = '\n';
  char out[64];
  int n = processInput(input, pos, out, sizeof(out));
  assert(n == 1);
  assert(out[0] == 'z');
  PASS();
}

static void test_rapid_small_bursts() {
  TEST(rapid_small_bursts);
  // Simulate many tiny command inputs in rapid succession
  char out[64];
  for (int i = 0; i < 10000; i++) {
    char input[4] = { (char)('a' + i % 26), 'b', '\n', '\0' };
    int n = processInput(input, 3, out, sizeof(out));
    assert(n == 2);
    assert(out[0] == (char)('a' + i % 26));
    assert(out[1] == 'b');
  }
  PASS();
}

static void test_alternating_escape_backspace() {
  TEST(alternating_escape_backspace);
  // Pathological: escape produces a char, backspace removes it, repeat
  // Net output should be empty
  const int REPS = 200;
  char input[REPS * 3 + 1];
  for (int i = 0; i < REPS; i++) {
    input[i * 3]     = '\\';
    input[i * 3 + 1] = 'n';
    input[i * 3 + 2] = 0x08;
  }
  input[REPS * 3] = '\n';
  char out[REPS + 1];
  int n = processInput(input, REPS * 3 + 1, out, sizeof(out));
  assert(n == 0);
  PASS();
}

static void test_zero_length_input() {
  TEST(zero_length_input);
  char out[4] = { 'X', 'X', 'X', 'X' };
  int n = processInput("", 0, out, sizeof(out));
  assert(n == 0);
  assert(out[0] == '\0');
  PASS();
}

static void test_only_terminators() {
  TEST(only_terminators);
  // Input is just \r or \n - should produce zero-length result
  char out[4];
  int n = processInput("\n", 1, out, sizeof(out));
  assert(n == 0);
  n = processInput("\r", 1, out, sizeof(out));
  assert(n == 0);
  PASS();
}

// --- Simulated relay buffer tests ---
// These test the bulk-copy pattern used in relay(): read N bytes, write N bytes.

static void test_relay_bulk_copy_full() {
  TEST(relay_bulk_copy_full);
  const int BUF_SZ = 256; // matches SERIAL_RX_BUFFER_SIZE
  uint8_t src[BUF_SZ], dst[BUF_SZ];
  for (int i = 0; i < BUF_SZ; i++) src[i] = (uint8_t)(i & 0xFF);
  memcpy(dst, src, BUF_SZ);
  for (int i = 0; i < BUF_SZ; i++) assert(dst[i] == (uint8_t)(i & 0xFF));
  PASS();
}

static void test_relay_partial_reads() {
  TEST(relay_partial_reads);
  // Simulate fragmented reads: copy in chunks of varying size
  const int TOTAL = 256;
  uint8_t src[TOTAL], dst[TOTAL];
  for (int i = 0; i < TOTAL; i++) src[i] = (uint8_t)i;
  int copied = 0;
  int chunks[] = { 1, 3, 7, 15, 31, 63, 127, 9 }; // sums to 256
  for (int c = 0; c < 8; c++) {
    memcpy(dst + copied, src + copied, chunks[c]);
    copied += chunks[c];
  }
  assert(copied == TOTAL);
  assert(memcmp(src, dst, TOTAL) == 0);
  PASS();
}

static void test_relay_sustained_throughput() {
  TEST(relay_sustained_throughput);
  // Simulate sustained relay: 1000 rounds of full-buffer copies
  const int BUF_SZ = 256;
  uint8_t buf[BUF_SZ];
  for (int i = 0; i < BUF_SZ; i++) buf[i] = (uint8_t)i;
  uint8_t dst[BUF_SZ];
  for (int round = 0; round < 1000; round++) {
    memcpy(dst, buf, BUF_SZ);
    // Verify first and last byte each round
    assert(dst[0] == 0);
    assert(dst[BUF_SZ - 1] == (uint8_t)(BUF_SZ - 1));
  }
  PASS();
}

int main() {
  printf("serialTap parsing tests:\n");

  test_escape_newline();
  test_escape_tab();
  test_backspace_removes_char();
  test_backspace_at_start();
  test_multiple_backspaces();
  test_escaped_backslash();
  test_buffer_overflow_protection();
  test_delete_key();

  printf("\nperformance / resiliency tests:\n");

  test_max_buffer_throughput();
  test_buffer_exact_capacity();
  test_buffer_one_byte_short();
  test_all_escapes_stress();
  test_all_backspaces_stress();
  test_backspace_heavier_than_input();
  test_rapid_small_bursts();
  test_alternating_escape_backspace();
  test_zero_length_input();
  test_only_terminators();
  test_relay_bulk_copy_full();
  test_relay_partial_reads();
  test_relay_sustained_throughput();

  printf("\n%d/%d tests passed\n", tests_passed, tests_run);
  return (tests_passed == tests_run) ? 0 : 1;
}
