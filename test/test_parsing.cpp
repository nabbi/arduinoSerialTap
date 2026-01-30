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

  printf("\n%d/%d tests passed\n", tests_passed, tests_run);
  return (tests_passed == tests_run) ? 0 : 1;
}
