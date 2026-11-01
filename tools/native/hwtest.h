// =====================================================================
//  hwtest -- zero-dependency test runner for native sketch tests.
// ---------------------------------------------------------------------
//  Each TEST runs in its own forked process, so sketch globals and
//  function-local statics start fresh every test. A test that hangs
//  (e.g. a sketch's `for(;;);`) is killed after 5 s and reported.
//
//    TEST(buzzer_silent_when_cool) {
//      sim::ds18b20C[0] = 25;
//      setup();
//      sim::runFor(3000);
//      CHECK_EQ(sim::pinLevel[3], LOW);
//    }
// =====================================================================
#pragma once
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Arduino.h"

namespace hwtest {
struct Case { const char *name; void (*fn)(); };
inline std::vector<Case> &registry() { static std::vector<Case> r; return r; }
struct Reg { Reg(const char *n, void (*f)()) { registry().push_back({n, f}); } };

[[noreturn]] inline void fail(const char *file, int line, const std::string &msg) {
  std::fprintf(stderr, "    %s:%d: %s\n", file, line, msg.c_str());
  std::fprintf(stderr, "    --- serial so far ---\n%s\n    ---------------------\n", sim::serialOut.c_str());
  std::_Exit(1);
}
}  // namespace hwtest

#define TEST(name)                                         \
  static void test_##name();                               \
  static hwtest::Reg reg_##name(#name, test_##name);       \
  static void test_##name()

#define CHECK(cond) \
  do { if (!(cond)) hwtest::fail(__FILE__, __LINE__, "CHECK(" #cond ")"); } while (0)

#define CHECK_EQ(a, b)                                                                     \
  do {                                                                                     \
    auto _a = (a); auto _b = (b);                                                          \
    if (!(_a == _b))                                                                       \
      hwtest::fail(__FILE__, __LINE__, std::string("CHECK_EQ(" #a ", " #b ") got ") +     \
                                           std::to_string(_a) + " vs " + std::to_string(_b)); \
  } while (0)

#define CHECK_CONTAINS(haystack, needle)                                                    \
  do {                                                                                      \
    std::string _h = (haystack);                                                            \
    if (_h.find(needle) == std::string::npos)                                               \
      hwtest::fail(__FILE__, __LINE__, std::string("expected to find \"") + (needle) + "\""); \
  } while (0)

#define CHECK_NOT_CONTAINS(haystack, needle)                                                 \
  do {                                                                                       \
    std::string _h = (haystack);                                                             \
    if (_h.find(needle) != std::string::npos)                                                \
      hwtest::fail(__FILE__, __LINE__, std::string("did not expect \"") + (needle) + "\"");  \
  } while (0)

int main(int argc, char **argv) {
  const char *only = argc > 1 ? argv[1] : nullptr;
  int passed = 0, failed = 0;
  for (const hwtest::Case &c : hwtest::registry()) {
    if (only && !std::strstr(c.name, only)) continue;
    std::fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
      alarm(5);
      sim::reset();
      c.fn();
      std::_Exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
      std::fprintf(stderr, "    timed out after 5 s (infinite loop in sketch?)\n");
    std::printf("  %s  %s\n", ok ? "PASS" : "FAIL", c.name);
    ok ? passed++ : failed++;
  }
  std::printf("\n%d passed, %d failed\n", passed, failed);
  return failed ? 1 : 0;
}
